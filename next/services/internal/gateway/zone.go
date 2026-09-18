package gateway

import (
	"context"
	"net"
	"sync"
	"sync/atomic"
	"time"

	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// zoneLink is the gateway side of the gateway<->zone protocol: one TCP connection per zone
// with automatic reconnect.  Outbound frames go through a channel so any goroutine can send.
type zoneLink struct {
	srv   *Server
	addr  string
	ready atomic.Bool

	mu   sync.Mutex
	conn net.Conn
	out  chan []byte
	ack  *jxpb.ZoneHelloAck
}

func newZoneLink(srv *Server, addr string) *zoneLink {
	return &zoneLink{srv: srv, addr: addr, out: make(chan []byte, 4096)}
}

func (z *zoneLink) info() *jxpb.ZoneHelloAck {
	z.mu.Lock()
	defer z.mu.Unlock()
	if z.ack == nil {
		return &jxpb.ZoneHelloAck{}
	}
	return proto.Clone(z.ack).(*jxpb.ZoneHelloAck)
}

// send queues a frame for the zone; false when the link is down.
func (z *zoneLink) send(id jxpb.MsgId, m proto.Message) bool {
	if !z.ready.Load() && id != jxpb.MsgId_GZ_ZONE_HELLO {
		return false
	}
	payload, err := proto.Marshal(m)
	if err != nil {
		log.Error("zone", "marshal failed", log.F("msg", int32(id)), log.F("error", err))
		return false
	}
	select {
	case z.out <- frame.Encode(uint16(id), 0, payload):
		return true
	default:
		log.Error("zone", "zone outbound queue full, dropping", log.F("msg", int32(id)))
		return false
	}
}

func (z *zoneLink) run(ctx context.Context) {
	backoff := 500 * time.Millisecond
	for ctx.Err() == nil {
		err := z.session(ctx)
		if ctx.Err() != nil {
			return
		}
		log.Warn("zone", "zone link down, reconnecting", log.F("addr", z.addr), log.F("error", err), log.F("retry_ms", backoff.Milliseconds()))
		select {
		case <-time.After(backoff):
		case <-ctx.Done():
			return
		}
		if backoff < 5*time.Second {
			backoff *= 2
		}
	}
}

// session runs one connection until it fails.
func (z *zoneLink) session(ctx context.Context) error {
	d := net.Dialer{Timeout: 5 * time.Second}
	conn, err := d.DialContext(ctx, "tcp", z.addr)
	if err != nil {
		return err
	}
	if tc, ok := conn.(*net.TCPConn); ok {
		_ = tc.SetNoDelay(true)
	}
	z.mu.Lock()
	z.conn = conn
	z.mu.Unlock()
	log.Info("zone", "connected to zone", log.F("addr", z.addr))

	// drain stale frames queued while the link was down
	for len(z.out) > 0 {
		<-z.out
	}
	writeErr := make(chan error, 1)
	stop := make(chan struct{})
	go func() {
		for {
			select {
			case b := <-z.out:
				_ = conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
				if _, err := conn.Write(b); err != nil {
					writeErr <- err
					return
				}
			case <-stop:
				return
			}
		}
	}()
	defer func() {
		close(stop)
		z.ready.Store(false)
		_ = conn.Close()
		z.srv.clearPendingSaves() // whatever the zone still owed is lost with the link
		z.srv.eachSession(func(s *session) { s.zoneLost() })
	}()

	hello, _ := proto.Marshal(&jxpb.ZoneHello{ProtocolVersion: uint32(jxpb.Protocol_PROTOCOL_VERSION), GatewayId: z.srv.cfg.ID})
	z.out <- frame.Encode(uint16(jxpb.MsgId_GZ_ZONE_HELLO), 0, hello)

	r := frame.NewReader(conn, frame.MaxInternalPayload)
	frames := make(chan frame.Frame, 256)
	readErr := make(chan error, 1)
	go func() {
		for {
			f, err := r.Read()
			if err != nil {
				readErr <- err
				return
			}
			frames <- f
		}
	}()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case err := <-writeErr:
			return err
		case err := <-readErr:
			return err
		case f := <-frames:
			z.handle(f)
		}
	}
}

func (z *zoneLink) handle(f frame.Frame) {
	id := jxpb.MsgId(f.MsgID)
	log.Trace("zone.recv", "from zone", log.F("msg", int32(id)), log.F("bytes", len(f.Payload)))
	switch id {
	case jxpb.MsgId_ZG_ZONE_HELLO_ACK:
		var ack jxpb.ZoneHelloAck
		if err := proto.Unmarshal(f.Payload, &ack); err != nil {
			log.Error("zone", "bad ZoneHelloAck", log.F("error", err))
			return
		}
		z.mu.Lock()
		z.ack = &ack
		z.mu.Unlock()
		// The zone hands every gateway link its own high bits for session ids, so two gateways in
		// front of one zone never number the same session twice.
		z.srv.sidPrefix.Store(ack.SessionPrefix)
		z.ready.Store(true)
		log.Info("zone", "zone ready", log.F("zone", ack.ZoneId), log.F("name", ack.ZoneName), log.F("tick_hz", ack.TickHz),
			log.F("capacity", ack.Capacity), log.F("session_prefix", ack.SessionPrefix))

	case jxpb.MsgId_ZG_SESSION_OPEN_ACK:
		var ack jxpb.SessionOpenAck
		if err := proto.Unmarshal(f.Payload, &ack); err != nil {
			log.Error("zone", "bad SessionOpenAck", log.F("error", err))
			return
		}
		if s := z.srv.session(ack.Sid); s != nil {
			s.onZoneAck(&ack)
		} else {
			// client left while entering: tell the zone
			z.send(jxpb.MsgId_GZ_SESSION_CLOSE, &jxpb.SessionClose{Sid: ack.Sid, Reason: 0})
		}

	case jxpb.MsgId_ZG_ZONE_PACKET:
		var zp jxpb.ZonePacket
		if err := proto.Unmarshal(f.Payload, &zp); err != nil {
			log.Error("zone", "bad ZonePacket", log.F("error", err))
			return
		}
		b := frame.Encode(uint16(zp.MsgId), 0, zp.Payload) // encode once, share read-only
		z.srv.stats.ZonePackets.Add(1)
		// what a client's queue may do with the frame while it waits (KSendQueue): a newer
		// position / life / swing of the same entity replaces it instead of piling up (SPEC 70)
		class, entity := classify(uint16(zp.MsgId), zp.Payload)
		for _, sid := range zp.Sids {
			if s := z.srv.session(sid); s != nil && s.getState() == stWorld {
				z.srv.stats.ZoneFanout.Add(1)
				s.sendFrame(uint16(zp.MsgId), class, entity, b)
			}
		}

	case jxpb.MsgId_ZG_PLAYER_SAVE:
		var save jxpb.PlayerSave
		if err := proto.Unmarshal(f.Payload, &save); err != nil || save.Role == nil {
			log.Error("zone", "bad PlayerSave", log.F("error", err))
			return
		}
		// written by the save workers, never here on the link (KSaveQueue); a final save tells
		// the server it arrived once it is in the store
		z.srv.saves.push(save.Sid, save.Role, save.Final)

	case jxpb.MsgId_ZG_ZONE_STATS:
		var st jxpb.ZoneStats
		if err := proto.Unmarshal(f.Payload, &st); err == nil {
			log.Info("zone", "zone stats", log.F("zone", st.ZoneId), log.F("tick", st.Tick), log.F("players", st.Players), log.F("entities", st.Entities), log.F("tick_ms_avg", st.TickMsAvg), log.F("tick_ms_max", st.TickMsMax))
		}

	default:
		log.Warn("zone", "unknown message from zone", log.F("msg", int32(id)))
	}
}
