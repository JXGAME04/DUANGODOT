package gateway

import (
	"context"
	"net"
	"testing"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
)

func TestHelloAckTellsAuthModeAndHeartbeat(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{HeartbeatTimeout: 7 * time.Second}, nil)
	c := dial(t, srv.Addr())
	hello, login := c.login("player1", "secret")
	if hello.AuthMode != "dev" || hello.HeartbeatS != 7 {
		t.Fatalf("HelloAck %+v", hello)
	}
	if login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", login)
	}
}

func TestSecondLoginReplacesTheFirstSession(t *testing.T) {
	zone := startFakeZone(t)
	srv, _ := startGateway(t, zone.ln.Addr().String())

	c1 := dial(t, srv.Addr())
	hello1, login1 := c1.login("dup", "pw")
	if login1.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("first login %+v", login1)
	}
	c1.enter()
	if srv.OnlineCount() != 1 {
		t.Fatalf("online %d", srv.OnlineCount())
	}

	c2 := dial(t, srv.Addr())
	_, login2 := c2.login("DUP", "pw")
	if login2.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("second login must win: %+v", login2)
	}
	kick := c1.expectKick(3 * time.Second)
	if kick.Reason != jxpb.Result_RESULT_REPLACED {
		t.Fatalf("first session kicked with %v", kick.Reason)
	}
	c1.expectClosed(3 * time.Second)
	if r := zone.waitClose(t, hello1.Sid, 3*time.Second); r != closeKicked {
		t.Fatalf("zone told reason %d, want kicked", r)
	}
	// the account stays bound to the second session, which can play
	c2.enter()
	deadline := time.Now().Add(2 * time.Second)
	for srv.OnlineCount() != 1 && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	if srv.OnlineCount() != 1 {
		t.Fatalf("online %d after replace", srv.OnlineCount())
	}
}

func TestDuplicateLoginRefusedLikeTheOldPaySys(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{RefuseDuplicateLogin: true}, nil)

	c1 := dial(t, srv.Addr())
	if _, login := c1.login("dup", "pw"); login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("first login %+v", login)
	}
	c2 := dial(t, srv.Addr())
	_, login2 := c2.login("dup", "pw")
	if login2.Result != jxpb.Result_RESULT_ACCOUNT_IN_USE {
		t.Fatalf("second login got %v, want ACCOUNT_IN_USE", login2.Result)
	}
	// the first session is untouched
	c1.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: 5})
	var pong jxpb.Pong
	c1.expect(jxpb.MsgId_G2C_PONG, &pong)
	// the second connection may retry after the first leaves
	c1.conn.Close()
	deadline := time.Now().Add(2 * time.Second)
	for srv.OnlineCount() != 0 && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	c2.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "dup", Password: "pw"})
	c2.expect(jxpb.MsgId_G2C_LOGIN_RES, login2)
	if login2.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("retry after logout %+v", login2)
	}
}

func TestLoginResultsFollowTheOldCodes(t *testing.T) {
	zone := startFakeZone(t)
	srv, store, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{MaxLoginTries: 3}, nil)
	strict := auth.New(store, auth.StrictOptions())
	srv.auth = strict // registered accounts only from here on
	ctx := context.Background()
	if _, err := strict.Register(ctx, "frozen1", "secret1"); err != nil {
		t.Fatal(err)
	}
	acc, _ := store.Account(ctx, "frozen1")
	acc.Frozen, acc.FrozenText = true, "vi phạm điều khoản"
	_ = store.UpdateAccount(ctx, acc)

	c := dial(t, srv.Addr())
	hello, login := c.login("unknown", "secret1")
	if hello.AuthMode != "strict" || login.Result != jxpb.Result_RESULT_UNAUTHORIZED {
		t.Fatalf("unknown account: %+v %+v", hello, login)
	}
	c.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "frozen1", Password: "secret1"})
	c.expect(jxpb.MsgId_G2C_LOGIN_RES, login)
	if login.Result != jxpb.Result_RESULT_ACCOUNT_FROZEN || login.Text != "vi phạm điều khoản" {
		t.Fatalf("frozen account: %+v", login)
	}
	// third failure on this connection: kicked
	c.send(jxpb.MsgId_C2G_LOGIN, &jxpb.LoginReq{Account: "frozen1", Password: "wrong"})
	c.expect(jxpb.MsgId_G2C_LOGIN_RES, login)
	if login.Result != jxpb.Result_RESULT_UNAUTHORIZED {
		t.Fatalf("wrong password: %+v", login)
	}
	if kick := c.expectKick(3 * time.Second); kick.Reason != jxpb.Result_RESULT_SERVER_BUSY {
		t.Fatalf("kick %+v", kick)
	}
}

func TestRateLimitKicksFlooders(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{RateMsgs: 10, RateBurst: 20}, nil)
	c := dial(t, srv.Addr())
	if _, login := c.login("flood", "pw"); login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", login)
	}
	for i := 0; i < 60; i++ {
		c.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: uint64(i)})
	}
	if kick := c.expectKick(3 * time.Second); kick.Reason != jxpb.Result_RESULT_RATE_LIMITED {
		t.Fatalf("kick %+v", kick)
	}
	c.expectClosed(3 * time.Second)
}

func TestSilentPlayerTimesOut(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{HeartbeatTimeout: 300 * time.Millisecond}, nil)
	c := dial(t, srv.Addr())
	hello, login := c.login("quiet", "pw")
	if login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", login)
	}
	// the lobby is patient (IdleTimeout), only the world needs heartbeats
	time.Sleep(500 * time.Millisecond)
	c.enter()
	c.expectClosed(3 * time.Second)
	if r := zone.waitClose(t, hello.Sid, 3*time.Second); r != closeTimeout {
		t.Fatalf("zone told reason %d, want timeout", r)
	}
}

func TestShutdownKicksAndWaitsForTheFinalSave(t *testing.T) {
	zone := startFakeZone(t)
	zone.saveDelay = 400 * time.Millisecond
	srv, store, cancel, done := startGatewayWith(t, zone.ln.Addr().String(), Config{ShutdownWait: 3 * time.Second}, nil)
	c := dial(t, srv.Addr())
	hello, login := c.login("saver", "pw")
	if login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", login)
	}
	c.enter()

	start := time.Now()
	cancel()
	if kick := c.expectKick(3 * time.Second); kick.Reason != jxpb.Result_RESULT_SERVER_SHUTDOWN {
		t.Fatalf("kick %+v", kick)
	}
	select {
	case <-done:
	case <-time.After(8 * time.Second):
		t.Fatal("gateway did not stop")
	}
	if r, ok := zone.closeReason(hello.Sid); !ok || r != closeShutdown {
		t.Fatalf("zone told reason %d (%v), want shutdown", r, ok)
	}
	role, err := store.Character(context.Background(), 1)
	if err != nil || role.Level != 9 {
		t.Fatalf("final save not persisted before exit (took %v): %v %+v", time.Since(start), err, role)
	}
	// the listening socket is closed first, so a client arriving during the shutdown never
	// gets in: either the dial is refused or the connection is closed without a HelloAck
	late, err := net.DialTimeout("tcp", srv.Addr(), time.Second)
	if err != nil {
		return
	}
	defer late.Close()
	c2 := &testClient{t: t, conn: late, r: frame.NewReader(late, frame.MaxClientPayload)}
	c2.send(jxpb.MsgId_C2G_HELLO, &jxpb.Hello{ProtocolVersion: 1})
	c2.expectClosed(2 * time.Second)
}
