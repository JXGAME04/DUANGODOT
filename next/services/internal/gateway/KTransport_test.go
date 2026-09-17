package gateway

import (
	"testing"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/transport"
)

// The same session code must serve a web/mobile client over WebSocket and a PC client over
// raw TCP at the same time: one gateway, two doors, one protocol.
func TestWebSocketClientPlaysLikeATCPClient(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{ListenWS: "127.0.0.1:0"}, nil)
	if srv.AddrWS() == "" {
		t.Fatal("no websocket door")
	}

	conn, err := transport.DialWebSocket("ws://"+srv.AddrWS()+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = conn.Close() })
	web := &testClient{t: t, conn: conn, r: frame.NewReader(conn, frame.MaxClientPayload)}

	hello, login := web.login("web_player", "pw")
	if hello.Sid == 0 || login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login over websocket: %+v %+v", hello, login)
	}
	enter := web.enter()
	if enter.EntityId == 0 {
		t.Fatalf("enter over websocket: %+v", enter)
	}
	var spawn jxpb.EntitySpawn
	web.expect(jxpb.MsgId_G2C_ENTITY_SPAWN, &spawn)

	web.send(jxpb.MsgId_C2G_MOVE, &jxpb.MoveReq{Target: &jxpb.Vec2{X: 120, Y: 240}, Seq: 4})
	var mv jxpb.EntityMove
	web.expect(jxpb.MsgId_G2C_ENTITY_MOVE, &mv)
	if mv.Seq != 4 || mv.Target.X != 120 {
		t.Fatalf("move over websocket: %+v", &mv)
	}

	// a TCP client on the other door of the same gateway shares the world
	pc := dial(t, srv.Addr())
	if _, l := pc.login("pc_player", "pw"); l.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("tcp login: %+v", l)
	}
	pc.enter()
	if srv.OnlineCount() != 2 {
		t.Fatalf("online %d, want 2", srv.OnlineCount())
	}

	// chat over the websocket is relayed like any other frame
	web.send(jxpb.MsgId_C2G_CHAT, &jxpb.ChatReq{Text: "xin chào từ trình duyệt"})
	web.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: 11})
	var pong jxpb.Pong
	web.expect(jxpb.MsgId_G2C_PONG, &pong)
	if pong.ClientMs != 11 {
		t.Fatalf("pong over websocket: %+v", &pong)
	}
	// and the kick path closes the websocket too
	web2conn, err := transport.DialWebSocket("ws://"+srv.AddrWS()+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer web2conn.Close()
	web2 := &testClient{t: t, conn: web2conn, r: frame.NewReader(web2conn, frame.MaxClientPayload)}
	if _, l := web2.login("web_player", "pw"); l.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("second login: %+v", l)
	}
	if k := web.expectKick(3 * time.Second); k.Reason != jxpb.Result_RESULT_REPLACED {
		t.Fatalf("kick over websocket: %+v", k)
	}
}
