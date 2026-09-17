package gateway

import (
	"context"
	"net"
	"sync"
	"testing"
	"time"

	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

// fakeZone speaks the internal protocol like jx_zone: acks the handshake and sessions,
// echoes moves as EntityMove to the mover and records saves.
type fakeZone struct {
	ln     net.Listener
	mu     sync.Mutex
	saves  []*jxpb.PlayerSave
	closes []uint64
	next   uint64
}

func startFakeZone(t *testing.T) *fakeZone {
	t.Helper()
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	z := &fakeZone{ln: ln, next: 100}
	go func() {
		for {
			c, err := ln.Accept()
			if err != nil {
				return
			}
			go z.serve(c)
		}
	}()
	t.Cleanup(func() { _ = ln.Close() })
	return z
}

func (z *fakeZone) serve(c net.Conn) {
	defer c.Close()
	r := frame.NewReader(c, frame.MaxInternalPayload)
	send := func(id jxpb.MsgId, m proto.Message) {
		b, _ := proto.Marshal(m)
		_ = frame.Write(c, uint16(id), 0, b)
	}
	for {
		f, err := r.Read()
		if err != nil {
			return
		}
		switch jxpb.MsgId(f.MsgID) {
		case jxpb.MsgId_GZ_ZONE_HELLO:
			send(jxpb.MsgId_ZG_ZONE_HELLO_ACK, &jxpb.ZoneHelloAck{ProtocolVersion: 1, ZoneId: 1, ZoneName: "Fake", TickHz: 20, Capacity: 10})
		case jxpb.MsgId_GZ_SESSION_OPEN:
			var open jxpb.SessionOpen
			_ = proto.Unmarshal(f.Payload, &open)
			z.mu.Lock()
			z.next++
			eid := z.next
			z.mu.Unlock()
			send(jxpb.MsgId_ZG_SESSION_OPEN_ACK, &jxpb.SessionOpenAck{Sid: open.Sid, Result: jxpb.Result_RESULT_OK, EntityId: eid, Pos: &jxpb.Vec2{X: 10, Y: 20}})
			spawn, _ := proto.Marshal(&jxpb.EntitySpawn{Entities: []*jxpb.EntityInfo{{EntityId: eid, Name: open.Role.Name, EntityType: jxpb.EntityType_ENTITY_PLAYER}}})
			send(jxpb.MsgId_ZG_ZONE_PACKET, &jxpb.ZonePacket{Sids: []uint64{open.Sid}, MsgId: uint32(jxpb.MsgId_G2C_ENTITY_SPAWN), Payload: spawn})
		case jxpb.MsgId_GZ_CLIENT_PACKET:
			var cp jxpb.ClientPacket
			_ = proto.Unmarshal(f.Payload, &cp)
			if cp.MsgId == uint32(jxpb.MsgId_C2G_MOVE) {
				var mv jxpb.MoveReq
				_ = proto.Unmarshal(cp.Payload, &mv)
				echo, _ := proto.Marshal(&jxpb.EntityMove{EntityId: 1, Pos: &jxpb.Vec2{X: 10, Y: 20}, Target: mv.Target, Seq: mv.Seq, Tick: 5})
				send(jxpb.MsgId_ZG_ZONE_PACKET, &jxpb.ZonePacket{Sids: []uint64{cp.Sid}, MsgId: uint32(jxpb.MsgId_G2C_ENTITY_MOVE), Payload: echo})
			}
		case jxpb.MsgId_GZ_SESSION_CLOSE:
			var cl jxpb.SessionClose
			_ = proto.Unmarshal(f.Payload, &cl)
			z.mu.Lock()
			z.closes = append(z.closes, cl.Sid)
			z.mu.Unlock()
			send(jxpb.MsgId_ZG_PLAYER_SAVE, &jxpb.PlayerSave{Sid: cl.Sid, Final: true, Role: &jxpb.RoleData{PlayerId: 1, AccountId: 1, Name: "Hero", Level: 9, Position: &jxpb.RolePosition{ZoneId: 1, Pos: &jxpb.Vec2{X: 10, Y: 20}}}})
		}
	}
}

// testClient is a minimal synchronous client.
type testClient struct {
	t    *testing.T
	conn net.Conn
	r    *frame.Reader
}

func dial(t *testing.T, addr string) *testClient {
	t.Helper()
	c, err := net.Dial("tcp", addr)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = c.Close() })
	return &testClient{t: t, conn: c, r: frame.NewReader(c, frame.MaxClientPayload)}
}

func (c *testClient) send(id jxpb.MsgId, m proto.Message) {
	c.t.Helper()
	b, err := proto.Marshal(m)
	if err != nil {
		c.t.Fatal(err)
	}
	if err := frame.Write(c.conn, uint16(id), 0, b); err != nil {
		c.t.Fatal(err)
	}
}

// expect reads frames until one with id arrives (others are skipped) and decodes it into m.
func (c *testClient) expect(id jxpb.MsgId, m proto.Message) {
	c.t.Helper()
	_ = c.conn.SetReadDeadline(time.Now().Add(5 * time.Second))
	for {
		f, err := c.r.Read()
		if err != nil {
			c.t.Fatalf("waiting for %v: %v", id, err)
		}
		if jxpb.MsgId(f.MsgID) == jxpb.MsgId_G2C_KICK && id != jxpb.MsgId_G2C_KICK {
			var k jxpb.Kick
			_ = proto.Unmarshal(f.Payload, &k)
			c.t.Fatalf("kicked while waiting for %v: %v %s", id, k.Reason, k.Text)
		}
		if jxpb.MsgId(f.MsgID) != id {
			continue
		}
		if err := proto.Unmarshal(f.Payload, m); err != nil {
			c.t.Fatal(err)
		}
		return
	}
}

func startGateway(t *testing.T, zoneAddr string) (*Server, persist.Store) {
	t.Helper()
	_ = log.Init(log.Options{Level: log.LevelWarn})
	store, err := persist.OpenFileStore(t.TempDir())
	if err != nil {
		t.Fatal(err)
	}
	srv := New(Config{ID: "gw-test", Listen: "127.0.0.1:0", ZoneAddr: zoneAddr, MaxChars: 2}, store, &auth.Dev{Store: store})
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan struct{})
	go func() { _ = srv.Run(ctx); close(done) }()
	t.Cleanup(func() {
		cancel()
		select {
		case <-done:
		case <-time.After(5 * time.Second):
			t.Error("gateway did not stop")
		}
	})
	deadline := time.Now().Add(5 * time.Second)
	for (srv.Addr() == "" || !srv.ZoneReady()) && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	if srv.Addr() == "" || !srv.ZoneReady() {
		t.Fatal("gateway not ready")
	}
	return srv, store
}

func TestFullClientFlow(t *testing.T) {
	zone := startFakeZone(t)
	srv, store := startGateway(t, zone.ln.Addr().String())

	c := dial(t, srv.Addr())
	c.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: 1, ClientVersion: "test", Platform: "go"})
	var hello jxpb.HelloAck
	c.expect(jxpb.MsgId_G2C_HELLO_ACK, &hello)
	if hello.Sid == 0 || hello.ProtocolVersion != 1 {
		t.Fatalf("bad HelloAck %+v", &hello)
	}

	c.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "player1", Password: "secret"})
	var login jxpb.LoginRes
	c.expect(jxpb.MsgId_G2C_LOGIN_RES, &login)
	if login.Result != jxpb.Result_RESULT_OK || login.AccountId == 0 {
		t.Fatalf("login failed %+v", &login)
	}

	c.send(jxpb.MsgId_C2G_CHAR_LIST, &jxpb.CharListReq{})
	var list jxpb.CharListRes
	c.expect(jxpb.MsgId_G2C_CHAR_LIST_RES, &list)
	if len(list.Chars) != 0 || list.MaxChars != 2 {
		t.Fatalf("expected empty list %+v", &list)
	}

	c.send(jxpb.MsgId_C2G_CHAR_CREATE, &jxpb.CharCreateReq{Name: "Hero", Series: 1})
	var created jxpb.CharCreateRes
	c.expect(jxpb.MsgId_G2C_CHAR_CREATE_RES, &created)
	if created.Result != jxpb.Result_RESULT_OK || created.Summary.Name != "Hero" {
		t.Fatalf("create failed %+v", &created)
	}
	c.send(jxpb.MsgId_C2G_CHAR_CREATE, &jxpb.CharCreateReq{Name: "hero"})
	c.expect(jxpb.MsgId_G2C_CHAR_CREATE_RES, &created)
	if created.Result != jxpb.Result_RESULT_ALREADY_EXISTS {
		t.Fatalf("duplicate accepted %+v", &created)
	}
	c.send(jxpb.MsgId_C2G_CHAR_CREATE, &jxpb.CharCreateReq{Name: "x"})
	c.expect(jxpb.MsgId_G2C_CHAR_CREATE_RES, &created)
	if created.Result != jxpb.Result_RESULT_INVALID_NAME {
		t.Fatalf("bad name accepted %+v", &created)
	}

	c.send(jxpb.MsgId_C2G_ENTER_WORLD, &jxpb.EnterWorldReq{PlayerId: 999})
	var enter jxpb.EnterWorldRes
	c.expect(jxpb.MsgId_G2C_ENTER_WORLD_RES, &enter)
	if enter.Result != jxpb.Result_RESULT_NOT_FOUND {
		t.Fatalf("foreign character accepted %+v", &enter)
	}

	c.send(jxpb.MsgId_C2G_CHAR_LIST, &jxpb.CharListReq{})
	c.expect(jxpb.MsgId_G2C_CHAR_LIST_RES, &list)
	if len(list.Chars) != 1 {
		t.Fatalf("expected one character %+v", &list)
	}
	pid := list.Chars[0].PlayerId
	c.send(jxpb.MsgId_C2G_ENTER_WORLD, &jxpb.EnterWorldReq{PlayerId: pid})
	c.expect(jxpb.MsgId_G2C_ENTER_WORLD_RES, &enter)
	if enter.Result != jxpb.Result_RESULT_OK || enter.EntityId == 0 || enter.ZoneName != "Fake" || enter.TickHz != 20 || enter.Pos.X != 10 {
		t.Fatalf("enter failed %+v", &enter)
	}
	var spawn jxpb.EntitySpawn
	c.expect(jxpb.MsgId_G2C_ENTITY_SPAWN, &spawn)
	if len(spawn.Entities) != 1 || spawn.Entities[0].Name != "Hero" {
		t.Fatalf("spawn %+v", &spawn)
	}

	c.send(jxpb.MsgId_C2G_MOVE, &jxpb.MoveReq{Target: &jxpb.Vec2{X: 300, Y: 400}, Seq: 9})
	var mv jxpb.EntityMove
	c.expect(jxpb.MsgId_G2C_ENTITY_MOVE, &mv)
	if mv.Seq != 9 || mv.Target.X != 300 {
		t.Fatalf("move echo %+v", &mv)
	}

	c.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: 123})
	var pong jxpb.Pong
	c.expect(jxpb.MsgId_G2C_PONG, &pong)
	if pong.ClientMs != 123 || pong.ServerMs == 0 {
		t.Fatalf("pong %+v", &pong)
	}

	// leaving: zone gets SessionClose and its PlayerSave is persisted
	c.send(jxpb.MsgId_C2G_LEAVE_WORLD, &jxpb.LeaveWorldReq{})
	deadline := time.Now().Add(3 * time.Second)
	for time.Now().Before(deadline) {
		role, err := store.Character(context.Background(), pid)
		if err == nil && role.Level == 9 {
			break
		}
		time.Sleep(10 * time.Millisecond)
	}
	role, _ := store.Character(context.Background(), pid)
	if role.Level != 9 || role.Position.Pos.X != 10 {
		t.Fatalf("save not persisted: %+v", role)
	}
	zone.mu.Lock()
	closes := append([]uint64(nil), zone.closes...)
	zone.mu.Unlock()
	if len(closes) != 1 || closes[0] != hello.Sid {
		t.Fatalf("SessionClose not sent: %v", closes)
	}

	// back in the lobby the list still works
	c.send(jxpb.MsgId_C2G_CHAR_LIST, &jxpb.CharListReq{})
	c.expect(jxpb.MsgId_G2C_CHAR_LIST_RES, &list)
	if len(list.Chars) != 1 || list.Chars[0].Level != 9 {
		t.Fatalf("list after leave %+v", &list)
	}
}

func TestWrongStateAndVersionAreKicked(t *testing.T) {
	zone := startFakeZone(t)
	srv, _ := startGateway(t, zone.ln.Addr().String())

	c := dial(t, srv.Addr())
	c.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "x", Password: "y"})
	var kick jxpb.Kick
	c.expect(jxpb.MsgId_G2C_KICK, &kick)
	if kick.Reason != jxpb.Result_RESULT_WRONG_STATE {
		t.Fatalf("expected WRONG_STATE got %v", kick.Reason)
	}

	c2 := dial(t, srv.Addr())
	c2.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: 99})
	c2.expect(jxpb.MsgId_G2C_KICK, &kick)
	if kick.Reason != jxpb.Result_RESULT_VERSION_MISMATCH {
		t.Fatalf("expected VERSION_MISMATCH got %v", kick.Reason)
	}

	c3 := dial(t, srv.Addr())
	c3.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: 1})
	var hello jxpb.HelloAck
	c3.expect(jxpb.MsgId_G2C_HELLO_ACK, &hello)
	c3.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "bad name!", Password: "y"})
	var login jxpb.LoginRes
	c3.expect(jxpb.MsgId_G2C_LOGIN_RES, &login)
	if login.Result != jxpb.Result_RESULT_UNAUTHORIZED {
		t.Fatalf("bad account accepted %+v", &login)
	}
	c3.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "player2", Password: "a"})
	c3.expect(jxpb.MsgId_G2C_LOGIN_RES, &login)
	if login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", &login)
	}
	// second client with the wrong password for the same account
	c4 := dial(t, srv.Addr())
	c4.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: 1})
	c4.expect(jxpb.MsgId_G2C_HELLO_ACK, &hello)
	c4.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "player2", Password: "wrong"})
	c4.expect(jxpb.MsgId_G2C_LOGIN_RES, &login)
	if login.Result != jxpb.Result_RESULT_UNAUTHORIZED {
		t.Fatalf("wrong password accepted %+v", &login)
	}
}
