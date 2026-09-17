// jxbot - headless test client: logs in through the gateway, enters the world and walks around.
//
//	jxbot -gateway 127.0.0.1:17100 -bots 5 -duration 30s          load / soak
//	jxbot -gateway 127.0.0.1:17100 -once                            smoke test for CI (exit 0 = ok)
//
// Every bot logs JSON lines (proc=jxbot) so the whole path client->gateway->zone can be checked
// from logs alone.
package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"math/rand"
	"net"
	"os"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/transport"
)

type stats struct {
	spawns, despawns, moves, chats, pongs atomic.Int64
	errors                                atomic.Int64
}

type bot struct {
	name   string
	addr   string
	conn   net.Conn
	r      *frame.Reader
	frames chan frame.Frame
	readEr chan error
	ctx    context.Context
	st     *stats

	sid      uint64
	entityID uint64
	pos      *jxpb.Vec2
	seq      uint32
}

func (b *bot) logctx() context.Context {
	return log.WithContext(context.Background(), log.Context{Sid: b.sid})
}

func (b *bot) send(id jxpb.MsgId, m proto.Message) error {
	payload, err := proto.Marshal(m)
	if err != nil {
		return err
	}
	_ = b.conn.SetWriteDeadline(time.Now().Add(5 * time.Second))
	return frame.Write(b.conn, uint16(id), 0, payload)
}

// connect dials the gateway over whichever transport the address names: "host:port" (raw TCP),
// "tls://host:port", "ws://host:port/ws" or "wss://host:port/ws".
func (b *bot) connect() error {
	var c net.Conn
	var err error
	switch {
	case strings.HasPrefix(b.addr, "ws://"):
		c, err = transport.DialWebSocket(b.addr, 5*time.Second)
	case strings.HasPrefix(b.addr, "wss://"):
		c, err = transport.DialWebSocketInsecure(b.addr, 5*time.Second)
	case strings.HasPrefix(b.addr, "tls://"):
		c, err = transport.DialTLSInsecure(strings.TrimPrefix(b.addr, "tls://"), 5*time.Second)
	default:
		d := net.Dialer{Timeout: 5 * time.Second}
		c, err = d.DialContext(b.ctx, "tcp", strings.TrimPrefix(b.addr, "tcp://"))
	}
	if err != nil {
		return err
	}
	b.conn = c
	b.r = frame.NewReader(c, frame.MaxClientPayload)
	b.frames = make(chan frame.Frame, 1024)
	b.readEr = make(chan error, 1)
	go func() {
		for {
			f, err := b.r.Read()
			if err != nil {
				b.readEr <- err
				return
			}
			b.frames <- f
		}
	}()
	return nil
}

// expect waits for msg id, handling world traffic on the side.
func (b *bot) expect(id jxpb.MsgId, m proto.Message, timeout time.Duration) error {
	deadline := time.After(timeout)
	for {
		select {
		case f := <-b.frames:
			if jxpb.MsgId(f.MsgID) == id {
				return proto.Unmarshal(f.Payload, m)
			}
			if err := b.handleWorld(f); err != nil {
				return err
			}
		case err := <-b.readEr:
			return err
		case <-deadline:
			return fmt.Errorf("timeout waiting for %v", id)
		case <-b.ctx.Done():
			return b.ctx.Err()
		}
	}
}

func (b *bot) handleWorld(f frame.Frame) error {
	switch jxpb.MsgId(f.MsgID) {
	case jxpb.MsgId_G2C_ENTITY_SPAWN:
		var m jxpb.EntitySpawn
		if err := proto.Unmarshal(f.Payload, &m); err != nil {
			return err
		}
		b.st.spawns.Add(int64(len(m.Entities)))
		log.DebugCtx(b.logctx(), "world", "spawn", log.F("count", len(m.Entities)))
	case jxpb.MsgId_G2C_ENTITY_DESPAWN:
		var m jxpb.EntityDespawn
		if err := proto.Unmarshal(f.Payload, &m); err != nil {
			return err
		}
		b.st.despawns.Add(int64(len(m.EntityIds)))
	case jxpb.MsgId_G2C_ENTITY_MOVE:
		var m jxpb.EntityMove
		if err := proto.Unmarshal(f.Payload, &m); err != nil {
			return err
		}
		b.st.moves.Add(1)
		if m.EntityId == b.entityID {
			b.pos = m.Pos
			log.TraceCtx(b.logctx(), "world", "own move", log.F("x", m.Pos.X), log.F("y", m.Pos.Y), log.F("seq", m.Seq), log.F("tick", m.Tick))
		}
	case jxpb.MsgId_G2C_CHAT_MSG:
		var m jxpb.ChatMsg
		if err := proto.Unmarshal(f.Payload, &m); err != nil {
			return err
		}
		b.st.chats.Add(1)
		log.DebugCtx(b.logctx(), "world", "chat", log.F("from", m.Name), log.F("text", m.Text))
	case jxpb.MsgId_G2C_PONG:
		b.st.pongs.Add(1)
	case jxpb.MsgId_G2C_KICK:
		var k jxpb.Kick
		_ = proto.Unmarshal(f.Payload, &k)
		return fmt.Errorf("kicked: %v %s", k.Reason, k.Text)
	default:
		log.WarnCtx(b.logctx(), "net", "unexpected message", log.F("msg", f.MsgID))
	}
	return nil
}

// login runs Hello -> Login -> CharList/Create -> EnterWorld.
func (b *bot) login(password string) error {
	if err := b.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: uint32(jxpb.Protocol_PROTOCOL_VERSION), ClientVersion: "jxbot", Platform: "go"}); err != nil {
		return err
	}
	var hello jxpb.HelloAck
	if err := b.expect(jxpb.MsgId_G2C_HELLO_ACK, &hello, 5*time.Second); err != nil {
		return err
	}
	b.sid = hello.Sid
	if err := b.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: b.name, Password: password}); err != nil {
		return err
	}
	var login jxpb.LoginRes
	if err := b.expect(jxpb.MsgId_G2C_LOGIN_RES, &login, 5*time.Second); err != nil {
		return err
	}
	if login.Result != jxpb.Result_RESULT_OK {
		return fmt.Errorf("login: %v %s", login.Result, login.Text)
	}
	if err := b.send(jxpb.MsgId_C2G_CHAR_LIST, &jxpb.CharListReq{}); err != nil {
		return err
	}
	var list jxpb.CharListRes
	if err := b.expect(jxpb.MsgId_G2C_CHAR_LIST_RES, &list, 5*time.Second); err != nil {
		return err
	}
	var pid uint64
	if len(list.Chars) > 0 {
		pid = list.Chars[0].PlayerId
	} else {
		if err := b.send(jxpb.MsgId_C2G_CHAR_CREATE, &jxpb.CharCreateReq{Name: strings.Title(b.name), Series: uint32(rand.Intn(5)), Sex: uint32(rand.Intn(2))}); err != nil {
			return err
		}
		var created jxpb.CharCreateRes
		if err := b.expect(jxpb.MsgId_G2C_CHAR_CREATE_RES, &created, 5*time.Second); err != nil {
			return err
		}
		if created.Result != jxpb.Result_RESULT_OK {
			return fmt.Errorf("create character: %v", created.Result)
		}
		pid = created.Summary.PlayerId
	}
	if err := b.send(jxpb.MsgId_C2G_ENTER_WORLD, &jxpb.EnterWorldReq{PlayerId: pid}); err != nil {
		return err
	}
	var enter jxpb.EnterWorldRes
	if err := b.expect(jxpb.MsgId_G2C_ENTER_WORLD_RES, &enter, 10*time.Second); err != nil {
		return err
	}
	if enter.Result != jxpb.Result_RESULT_OK {
		return fmt.Errorf("enter world: %v", enter.Result)
	}
	b.entityID = enter.EntityId
	b.pos = enter.Pos
	log.InfoCtx(b.logctx(), "bot", "in world", log.F("account", b.name), log.F("pid", pid), log.F("entity", b.entityID), log.F("zone", enter.ZoneName), log.F("x", enter.Pos.X), log.F("y", enter.Pos.Y))
	return nil
}

func (b *bot) move(dx, dy int32) error {
	b.seq++
	target := &jxpb.Vec2{X: b.pos.X + dx, Y: b.pos.Y + dy}
	return b.send(jxpb.MsgId_C2G_MOVE, &jxpb.MoveReq{Target: target, Seq: b.seq})
}

// waitArrival waits until an own EntityMove reports pos == target.
func (b *bot) waitArrival(timeout time.Duration) error {
	deadline := time.After(timeout)
	for {
		select {
		case f := <-b.frames:
			if jxpb.MsgId(f.MsgID) == jxpb.MsgId_G2C_ENTITY_MOVE {
				var m jxpb.EntityMove
				if err := proto.Unmarshal(f.Payload, &m); err != nil {
					return err
				}
				b.st.moves.Add(1)
				if m.EntityId == b.entityID {
					b.pos = m.Pos
					if m.Pos.X == m.Target.X && m.Pos.Y == m.Target.Y && m.Seq == b.seq {
						return nil
					}
				}
				continue
			}
			if err := b.handleWorld(f); err != nil {
				return err
			}
		case err := <-b.readEr:
			return err
		case <-deadline:
			return errors.New("timeout waiting for arrival")
		case <-b.ctx.Done():
			return b.ctx.Err()
		}
	}
}

// wander sends random moves and chats until ctx ends.
func (b *bot) wander() {
	moveTimer := time.NewTimer(time.Duration(500+rand.Intn(1500)) * time.Millisecond)
	chatTimer := time.NewTicker(10 * time.Second)
	pingTimer := time.NewTicker(5 * time.Second)
	defer moveTimer.Stop()
	defer chatTimer.Stop()
	defer pingTimer.Stop()
	for {
		select {
		case <-b.ctx.Done():
			return
		case err := <-b.readEr:
			log.WarnCtx(b.logctx(), "bot", "connection lost", log.F("error", err))
			b.st.errors.Add(1)
			return
		case f := <-b.frames:
			if err := b.handleWorld(f); err != nil {
				log.WarnCtx(b.logctx(), "bot", "stopping", log.F("error", err))
				b.st.errors.Add(1)
				return
			}
		case <-moveTimer.C:
			if err := b.move(int32(rand.Intn(801)-400), int32(rand.Intn(801)-400)); err != nil {
				return
			}
			moveTimer.Reset(time.Duration(1000+rand.Intn(3000)) * time.Millisecond)
		case <-chatTimer.C:
			_ = b.send(jxpb.MsgId_C2G_CHAT, &jxpb.ChatReq{Text: "xin chào từ " + b.name})
		case <-pingTimer.C:
			_ = b.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: uint64(time.Now().UnixMilli())})
		}
	}
}

func main() {
	gw := flag.String("gateway", "127.0.0.1:17100", "gateway address: host:port, tls://host:port or ws(s)://host:port/ws")
	n := flag.Int("bots", 1, "number of bots")
	prefix := flag.String("prefix", "bot", "account name prefix")
	password := flag.String("password", "bot", "account password")
	duration := flag.Duration("duration", 30*time.Second, "how long to wander")
	once := flag.Bool("once", false, "smoke test: login, move once, wait for arrival, exit")
	level := flag.String("log-level", "info", "log level")
	flag.Parse()

	_ = log.Init(log.Options{Level: log.ParseLevel(*level), Console: true, Process: "jxbot"})
	defer log.Shutdown()

	ctx, cancel := context.WithTimeout(context.Background(), *duration+30*time.Second)
	defer cancel()
	st := &stats{}
	var wg sync.WaitGroup
	failed := atomic.Int64{}
	start := time.Now()

	for i := 0; i < *n; i++ {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			b := &bot{name: fmt.Sprintf("%s%d", *prefix, i+1), addr: *gw, ctx: ctx, st: st}
			if err := b.connect(); err != nil {
				log.Error("bot", "connect failed", log.F("bot", b.name), log.F("error", err))
				failed.Add(1)
				return
			}
			defer b.conn.Close()
			if err := b.login(*password); err != nil {
				log.Error("bot", "login flow failed", log.F("bot", b.name), log.F("error", err))
				failed.Add(1)
				return
			}
			if *once {
				if err := b.move(100, 0); err != nil {
					failed.Add(1)
					return
				}
				if err := b.waitArrival(10 * time.Second); err != nil {
					log.ErrorCtx(b.logctx(), "bot", "arrival failed", log.F("error", err))
					failed.Add(1)
					return
				}
				log.InfoCtx(b.logctx(), "bot", "smoke ok", log.F("x", b.pos.X), log.F("y", b.pos.Y))
				_ = b.send(jxpb.MsgId_C2G_LEAVE_WORLD, &jxpb.LeaveWorldReq{})
				time.Sleep(200 * time.Millisecond)
				return
			}
			wctx, wcancel := context.WithTimeout(ctx, *duration)
			b.ctx = wctx
			b.wander()
			wcancel()
		}(i)
	}
	wg.Wait()

	log.Info("bot", "summary", log.F("bots", *n), log.F("failed", failed.Load()), log.F("spawns", st.spawns.Load()), log.F("despawns", st.despawns.Load()),
		log.F("moves", st.moves.Load()), log.F("chats", st.chats.Load()), log.F("pongs", st.pongs.Load()), log.F("errors", st.errors.Load()),
		log.F("elapsed_s", fmt.Sprintf("%.1f", time.Since(start).Seconds())))
	if failed.Load() > 0 || st.errors.Load() > 0 {
		os.Exit(1)
	}
}
