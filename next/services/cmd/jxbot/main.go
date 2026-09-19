// jxbot - headless test client: logs in through the gateway, enters the world and walks around.
//
//	jxbot -gateway 127.0.0.1:17100 -bots 5 -duration 30s          load / soak
//	jxbot -gateway 127.0.0.1:17100 -once                            smoke test for CI (exit 0 = ok)
//	jxbot -gateway 127.0.0.1:17100 -partner -prefix auto -first 2   a partner for the Godot client's --auto run: it puts the
//	                                                                trade sign up and says yes to every team invitation and
//	                                                                trade application, locks and confirms (M14)
//
// Every bot logs JSON lines (proc=jxbot) so the whole path client->gateway->zone can be checked
// from logs alone.
package main

import (
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"math/rand"
	"net"
	"os"
	"runtime"
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

// Per bot buffer sizes, lowered when thousands of bots share one process (see main).
var (
	readBufferSize = 64 * 1024
	frameQueue     = 1024
)

type stats struct {
	spawns, despawns, moves, chats, pongs atomic.Int64
	moveBatches                           atomic.Int64 // EntityMoves frames: the far movers, gathered (N3)
	actions, lifes                        atomic.Int64
	errors                                atomic.Int64
	// how long the server took to put a bot in the world: the number a real player feels
	enters, enterMs, enterMaxMs atomic.Int64
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
	// scenario (MASTER SPEC 56, 57, 58)
	hot    bool // stay inside `radius` of the point where the bot entered the world
	radius int32
	attack bool
	home   *jxpb.Vec2 // where the bot entered: the middle of the crowd
	target uint64     // what it is hitting right now
	// the partner of the --auto client (M14): the trade sign up, yes to everything, the lock and the ok in turn
	partner   bool
	assets    string // client/assets: the map spawn the partner walks to
	meetMap   uint32 // the map the --auto client plays on (0 = the bot's own)
	tradeOpen bool   // the sign is up
	trading   bool
	selfLock  bool
	destLock  bool
	selfOk    bool
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
	// A load test opens hundreds of connections at once; the listen backlog of the operating
	// system can refuse a few of them, exactly as it would for real players arriving together.
	// A client retries instead of giving up (MASTER SPEC 58 scenario E).
	var err error
	for attempt := 0; attempt < 5; attempt++ {
		if err = b.dial(); err == nil {
			return b.startReader()
		}
		select {
		case <-b.ctx.Done():
			return b.ctx.Err()
		case <-time.After(time.Duration(50+attempt*150) * time.Millisecond):
		}
	}
	return err
}

func (b *bot) dial() error {
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
	return nil
}

func (b *bot) startReader() error {
	// A load test holds thousands of these in one process: the default 64 KiB read buffer and a
	// 1024 frame channel would be ~1,3 GB at 20 000 bots, all of it in the test client.
	b.r = frame.NewReaderSize(b.conn, frame.MaxClientPayload, readBufferSize)
	b.frames = make(chan frame.Frame, frameQueue)
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
		if b.attack && b.target == 0 {
			for _, e := range m.Entities {
				if e.EntityId != b.entityID && e.EntityType == jxpb.EntityType_ENTITY_MONSTER && e.Life > 0 {
					b.target = e.EntityId
					break
				}
			}
		}
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
		b.applyMove(&m)
	case jxpb.MsgId_G2C_ENTITY_MOVES:
		// the far movers, gathered by the zone (N3): each entry is a whole EntityMove
		var ms jxpb.EntityMoves
		if err := proto.Unmarshal(f.Payload, &ms); err != nil {
			return err
		}
		b.st.moveBatches.Add(1)
		for _, m := range ms.Moves {
			b.applyMove(m)
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
	case jxpb.MsgId_G2C_ENTITY_ACTION:
		// attack / hurt / death: counted, never logged per message - a load client must be able
		// to swallow what it asks for (SPEC 56)
		b.st.actions.Add(1)
	case jxpb.MsgId_G2C_ENTITY_LIFE:
		b.st.lifes.Add(1)
	case jxpb.MsgId_G2C_CHANGE_MAP:
		var m jxpb.ChangeMap
		if err := proto.Unmarshal(f.Payload, &m); err != nil {
			return err
		}
		b.entityID = m.EntityId
		b.pos = m.Pos
		b.home = &jxpb.Vec2{X: m.Pos.X, Y: m.Pos.Y}
		b.target = 0
	case jxpb.MsgId_G2C_KICK:
		var k jxpb.Kick
		_ = proto.Unmarshal(f.Payload, &k)
		return fmt.Errorf("kicked: %v %s", k.Reason, k.Text)
	case jxpb.MsgId_G2C_TEAM_EVENT:
		if !b.partner {
			break
		}
		var ev jxpb.TeamEvent
		if err := proto.Unmarshal(f.Payload, &ev); err != nil {
			return err
		}
		if ev.Event == jxpb.TeamEventKind_TEAM_EV_INVITE {
			// KPlayerTeam::GetInviteReply: the captain's npc and yes
			log.InfoCtx(b.logctx(), "bot", "team invitation accepted", log.F("captain", ev.EntityId), log.F("name", ev.Name))
			_ = b.send(jxpb.MsgId_C2G_TEAM, &jxpb.TeamReq{Cmd: jxpb.TeamCmd_TEAM_REPLY_INVITE, Target: ev.EntityId, Flag: 1})
		}
	case jxpb.MsgId_G2C_TEAM_SELF:
		var ts jxpb.TeamSelf
		if err := proto.Unmarshal(f.Payload, &ts); err != nil {
			return err
		}
		log.InfoCtx(b.logctx(), "bot", "team", log.F("in_team", ts.InTeam), log.F("members", len(ts.Members)), log.F("captain", ts.Captain))
	case jxpb.MsgId_G2C_TRADE_APPLY:
		if !b.partner {
			break
		}
		var ap jxpb.TradeApply
		if err := proto.Unmarshal(f.Payload, &ap); err != nil {
			return err
		}
		log.InfoCtx(b.logctx(), "bot", "trade application accepted", log.F("applicant", ap.EntityId), log.F("name", ap.Name))
		_ = b.send(jxpb.MsgId_C2G_TRADE, &jxpb.TradeReq{Cmd: jxpb.TradeCmd_TRADE_REPLY, Target: ap.EntityId, Arg: 1})
	case jxpb.MsgId_G2C_TRADE_STATE:
		var ts jxpb.TradeState
		if err := proto.Unmarshal(f.Payload, &ts); err != nil {
			return err
		}
		b.trading = ts.State == 2
		b.tradeOpen = ts.State == 1
		if !b.trading {
			b.selfLock, b.destLock, b.selfOk = false, false, false
		}
		log.InfoCtx(b.logctx(), "bot", "trade state", log.F("state", ts.State), log.F("partner", ts.Partner))
	case jxpb.MsgId_G2C_TRADE_SYNC:
		if !b.partner {
			break
		}
		var sy jxpb.TradeSync
		if err := proto.Unmarshal(f.Payload, &sy); err != nil {
			return err
		}
		b.selfLock, b.destLock, b.selfOk = sy.SelfLock, sy.DestLock, sy.SelfOk
		// the partner locks once the client has (the 0x6d decision 2), then confirms once both are locked (decision 1)
		if b.trading && sy.DestLock && !sy.SelfLock {
			log.InfoCtx(b.logctx(), "bot", "trade lock")
			_ = b.send(jxpb.MsgId_C2G_TRADE, &jxpb.TradeReq{Cmd: jxpb.TradeCmd_TRADE_DECISION, Arg: 2})
		} else if b.trading && sy.DestLock && sy.SelfLock && !sy.SelfOk {
			log.InfoCtx(b.logctx(), "bot", "trade ok")
			_ = b.send(jxpb.MsgId_C2G_TRADE, &jxpb.TradeReq{Cmd: jxpb.TradeCmd_TRADE_DECISION, Arg: 1})
		}
	case jxpb.MsgId_G2C_TRADE_END:
		var te jxpb.TradeEnd
		_ = proto.Unmarshal(f.Payload, &te)
		log.InfoCtx(b.logctx(), "bot", "trade end", log.F("ok", te.Ok))
		if b.partner {
			// the sign up again for the next application
			_ = b.send(jxpb.MsgId_C2G_TRADE, &jxpb.TradeReq{Cmd: jxpb.TradeCmd_TRADE_APPLY_OPEN, Text: "bot ban do"})
		}
	default:
		log.DebugCtx(b.logctx(), "net", "unexpected message", log.F("msg", f.MsgID))
	}
	return nil
}

// login runs Hello -> Login -> CharList/Create -> EnterWorld.
func (b *bot) login(password string) error {
	if err := b.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: uint32(jxpb.Protocol_PROTOCOL_VERSION), ClientVersion: "jxbot", Platform: "go"}); err != nil {
		return err
	}
	var hello jxpb.HelloAck
	if err := b.expect(jxpb.MsgId_G2C_HELLO_ACK, &hello, 30*time.Second); err != nil {
		return err
	}
	b.sid = hello.Sid
	if err := b.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: b.name, Password: password}); err != nil {
		return err
	}
	var login jxpb.LoginRes
	// the password check is argon2id (~16 ms of CPU each, on purpose): when hundreds of clients
	// arrive together they queue, and a real client waits instead of giving up (SPEC 58 case E)
	if err := b.expect(jxpb.MsgId_G2C_LOGIN_RES, &login, 60*time.Second); err != nil {
		return err
	}
	if login.Result != jxpb.Result_RESULT_OK {
		return fmt.Errorf("login: %v %s", login.Result, login.Text)
	}
	if err := b.send(jxpb.MsgId_C2G_CHAR_LIST, &jxpb.CharListReq{}); err != nil {
		return err
	}
	var list jxpb.CharListRes
	if err := b.expect(jxpb.MsgId_G2C_CHAR_LIST_RES, &list, 30*time.Second); err != nil {
		return err
	}
	var pid uint64
	if len(list.Chars) > 0 {
		pid = list.Chars[0].PlayerId
	} else {
		if err := b.send(jxpb.MsgId_C2G_CHAR_CREATE, newCharacterReq(strings.Title(b.name), uint32(rand.Intn(5)), uint32(rand.Intn(2)))); err != nil {
			return err
		}
		var created jxpb.CharCreateRes
		if err := b.expect(jxpb.MsgId_G2C_CHAR_CREATE_RES, &created, 30*time.Second); err != nil {
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
	// 30 seconds, like the other steps: a short timeout here turns "the server was slow" into
	// "the client gave up" and hides how slow it really was.  The wait is measured instead.
	enterStart := time.Now()
	if err := b.expect(jxpb.MsgId_G2C_ENTER_WORLD_RES, &enter, 30*time.Second); err != nil {
		return err
	}
	b.st.enterMs.Add(time.Since(enterStart).Milliseconds())
	b.st.enters.Add(1)
	for {
		worst := b.st.enterMaxMs.Load()
		ms := time.Since(enterStart).Milliseconds()
		if ms <= worst || b.st.enterMaxMs.CompareAndSwap(worst, ms) {
			break
		}
	}
	if enter.Result != jxpb.Result_RESULT_OK {
		return fmt.Errorf("enter world: %v", enter.Result)
	}
	b.entityID = enter.EntityId
	b.pos = enter.Pos
	b.home = &jxpb.Vec2{X: enter.Pos.X, Y: enter.Pos.Y}
	log.InfoCtx(b.logctx(), "bot", "in world", log.F("account", b.name), log.F("pid", pid), log.F("entity", b.entityID), log.F("zone", enter.ZoneName), log.F("x", enter.Pos.X), log.F("y", enter.Pos.Y))
	if b.partner && b.assets != "" {
		// the partner goes to the spawn point of the meeting map, where the --auto client starts (its character is born
		// there and saved near it; a bot's own character may be born in another village): "?gm ds NewWorld(map, cell x,
		// cell y)" through the zone's GM chat (KGMCommand ds = DoSct; dev.py starts the zone with gm_chat)
		meet := b.meetMap
		if meet == 0 {
			meet = enter.MapId
		}
		// NewWorld takes absolute Mps cells (KNpc::ChangeWorld x * 32: the map's origin region_left * 512 /
		// region_top * 1024 plus the local spawn pixel, then / 32); the zone's to_local takes the origin off again
		if x, y, ok := mapSpawn(b.assets, meet); ok {
			_ = b.send(jxpb.MsgId_C2G_CHAT, &jxpb.ChatReq{Text: fmt.Sprintf("?gm ds NewWorld(%d, %d, %d)", meet, x/32, y/32)})
			log.InfoCtx(b.logctx(), "bot", "partner to the spawn", log.F("map", meet), log.F("x", x), log.F("y", y))
		}
	}
	return nil
}

// mapSpawn reads the spawn point of an exported map (client/assets/maps/<id>/map.json "spawn": [x, y], local pixels) and
// returns it in absolute Mps pixels: the map's origin (region_left * region_w, region_top * region_h - KMapData.cpp) added.
func mapSpawn(assets string, mapID uint32) (int, int, bool) {
	data, err := os.ReadFile(fmt.Sprintf("%s/maps/%d/map.json", assets, mapID))
	if err != nil {
		return 0, 0, false
	}
	var doc struct {
		Spawn      []int `json:"spawn"`
		RegionLeft int   `json:"region_left"`
		RegionTop  int   `json:"region_top"`
		RegionW    int   `json:"region_w"`
		RegionH    int   `json:"region_h"`
	}
	if json.Unmarshal(data, &doc) != nil || len(doc.Spawn) < 2 {
		return 0, 0, false
	}
	if doc.RegionW == 0 {
		doc.RegionW = 512
	}
	if doc.RegionH == 0 {
		doc.RegionH = 1024
	}
	return doc.RegionLeft*doc.RegionW + doc.Spawn[0], doc.RegionTop*doc.RegionH + doc.Spawn[1], true
}

func (b *bot) move(dx, dy int32) error {
	b.seq++
	target := &jxpb.Vec2{X: b.pos.X + dx, Y: b.pos.Y + dy}
	return b.send(jxpb.MsgId_C2G_MOVE, &jxpb.MoveReq{Target: target, Seq: b.seq})
}

// applyMove takes one EntityMove, whether it came alone or inside an EntityMoves batch.
func (b *bot) applyMove(m *jxpb.EntityMove) {
	b.st.moves.Add(1)
	if m.EntityId == b.entityID {
		b.pos = m.Pos
		log.TraceCtx(b.logctx(), "world", "own move", log.F("x", m.Pos.X), log.F("y", m.Pos.Y), log.F("seq", m.Seq), log.F("tick", m.Tick))
	}
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
			if b.partner {
				// the partner stays where it is (the client must find it around) and keeps its trade sign up
				if !b.tradeOpen && !b.trading {
					_ = b.send(jxpb.MsgId_C2G_TRADE, &jxpb.TradeReq{Cmd: jxpb.TradeCmd_TRADE_APPLY_OPEN, Text: "bot ban do"})
				}
				moveTimer.Reset(1500 * time.Millisecond)
				continue
			}
			dx, dy := int32(rand.Intn(801)-400), int32(rand.Intn(801)-400)
			if b.hot && b.home != nil {
				// a crowd fighting over one spot: never walk further than `radius` from it
				nx, ny := b.pos.X+dx-b.home.X, b.pos.Y+dy-b.home.Y
				if nx*nx+ny*ny > b.radius*b.radius {
					dx, dy = (b.home.X-b.pos.X)/2, (b.home.Y-b.pos.Y)/2
				}
			}
			if err := b.move(dx, dy); err != nil {
				return
			}
			if b.attack && b.target != 0 {
				b.seq++
				_ = b.send(jxpb.MsgId_C2G_ATTACK, &jxpb.AttackReq{Target: b.target, Seq: b.seq})
			}
			moveTimer.Reset(time.Duration(1000+rand.Intn(3000)) * time.Millisecond)
		case <-chatTimer.C:
			if b.partner {
				continue
			}
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
	first := flag.Int("first", 1, "first account number, so several bot processes can share one set of accounts")
	password := flag.String("password", "bot", "account password")
	duration := flag.Duration("duration", 30*time.Second, "how long to wander")
	once := flag.Bool("once", false, "smoke test: login, move once, wait for arrival, exit")
	scenario := flag.String("scenario", "spread", "spread = wander over the map, hot = every bot stays in one small area (MASTER SPEC 57)")
	radius := flag.Int("radius", 600, "hot scenario: how far from the meeting point a bot may walk")
	attack := flag.Bool("attack", false, "attack whatever comes into view (MASTER SPEC 58 scenario C/D)")
	ramp := flag.Duration("ramp", 0, "spread the logins over this long instead of all at once (MASTER SPEC 58 case E)")
	partner := flag.Bool("partner", false, "the partner of the Godot client's --auto run: trade sign up, yes to team invitations and trade applications (M14)")
	assets := flag.String("assets", "client/assets", "partner: the exported assets (the map spawn point it stands at)")
	meetMap := flag.Uint("meet-map", 0, "partner: the map to meet the --auto client on (its spawn point), 0 = the bot's own")
	level := flag.String("log-level", "info", "log level")
	flag.Parse()

	_ = log.Init(log.Options{Level: log.ParseLevel(*level), Console: true, Process: "jxbot"})
	defer log.Shutdown()

	// Thousands of connections in one process: shrink what each one holds, or the test client runs
	// out of memory long before the server does.
	if *n >= 500 {
		readBufferSize = 8 * 1024
		frameQueue = 64
	}

	ctx, cancel := context.WithTimeout(context.Background(), *duration+*ramp+60*time.Second)
	defer cancel()
	st := &stats{}
	leaveAt := time.Now().Add(*ramp + *duration)
	var wg sync.WaitGroup
	failed := atomic.Int64{}
	start := time.Now()

	for i := 0; i < *n; i++ {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			// Evenly spread starts: 5000 clients hitting the door in the same millisecond measures the
			// accept queue, not the game.  Real players arrive over minutes.
			if *ramp > 0 && *n > 1 {
				select {
				case <-time.After(time.Duration(int64(*ramp) * int64(i) / int64(*n))):
				case <-ctx.Done():
					return
				}
			}
			b := &bot{name: fmt.Sprintf("%s%d", *prefix, *first+i), addr: *gw, ctx: ctx, st: st,
				hot: *scenario == "hot", radius: int32(*radius), attack: *attack, partner: *partner, assets: *assets, meetMap: uint32(*meetMap)}
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
			// Everyone leaves at the same moment, so the whole population really is online
			// together for `duration`.  Giving each bot its own timer instead means the first ones
			// have already gone when the last ones arrive, and the peak is never reached.
			wctx, wcancel := context.WithDeadline(ctx, leaveAt)
			b.ctx = wctx
			b.wander()
			wcancel()
		}(i)
	}
	wg.Wait()

	// What the test client itself cost: when a load test stops scaling it matters whether the
	// server or the machine running the bots ran out (MASTER SPEC 55).
	var mem runtime.MemStats
	runtime.ReadMemStats(&mem)
	log.Info("bot", "client cost", log.F("heap_mb", mem.HeapAlloc/(1024*1024)),
		log.F("sys_mb", mem.Sys/(1024*1024)), log.F("goroutines", runtime.NumGoroutine()),
		log.F("gc", mem.NumGC))
	avgEnter := int64(0)
	if c := st.enters.Load(); c > 0 {
		avgEnter = st.enterMs.Load() / c
	}
	log.Info("bot", "enter world", log.F("entered", st.enters.Load()),
		log.F("avg_ms", avgEnter), log.F("max_ms", st.enterMaxMs.Load()))
	log.Info("bot", "summary", log.F("bots", *n), log.F("failed", failed.Load()), log.F("spawns", st.spawns.Load()), log.F("despawns", st.despawns.Load()),
		log.F("moves", st.moves.Load()), log.F("move_batches", st.moveBatches.Load()), log.F("chats", st.chats.Load()), log.F("pongs", st.pongs.Load()),
		log.F("actions", st.actions.Load()), log.F("lifes", st.lifes.Load()), log.F("errors", st.errors.Load()),
		log.F("elapsed_s", fmt.Sprintf("%.1f", time.Since(start).Seconds())))
	if failed.Load() > 0 || st.errors.Load() > 0 {
		os.Exit(1)
	}
}

// newCharacterReq makes the choice a player could make: Kim is for men only, Thủy for women only
// (KUiNewPlayer::UpdateProperty), and the gateway refuses anything else.
func newCharacterReq(name string, series, sex uint32) *jxpb.CharCreateReq {
	switch series {
	case 0:
		sex = 0
	case 2:
		sex = 1
	}
	return &jxpb.CharCreateReq{Name: name, Series: series, Sex: sex, NativePlace: 53}
}
