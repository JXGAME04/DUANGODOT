package gateway

import (
	"math/rand"
	"testing"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
)

// A client sending nonsense must never take the gateway down or disturb other players: it is
// kicked or its message is ignored, and everybody else keeps playing.  (The old cluster read
// packets straight into structs, so one malformed packet could crash the whole process.)
func TestGarbageTrafficNeverBreaksTheGateway(t *testing.T) {
	zone := startFakeZone(t)
	// a generous rate limit: this test is about malformed content, not about flooding
	srv, _, _, _ := startGatewayWith(t, zone.ln.Addr().String(), Config{RateMsgs: 5000, RateBurst: 5000}, nil)

	good := dial(t, srv.Addr())
	if _, login := good.login("player_ok", "pw"); login.Result != jxpb.Result_RESULT_OK {
		t.Fatalf("login %+v", login)
	}
	good.enter()

	rnd := rand.New(rand.NewSource(20260917))
	for round := 0; round < 40; round++ {
		bad, err := dialRaw(srv.Addr())
		if err != nil {
			t.Fatalf("round %d: %v", round, err)
		}
		// random message ids with random payloads, in every session state
		for i := 0; i < 8; i++ {
			payload := make([]byte, rnd.Intn(64))
			rnd.Read(payload)
			id := uint16(rnd.Intn(11000))
			if rnd.Intn(4) == 0 { // sometimes a real id with a nonsense body
				id = uint16(jxpb.MsgId_C2G_HELLO) + uint16(rnd.Intn(8))
			}
			if _, err := bad.Write(frame.Encode(id, uint16(rnd.Intn(4)), payload)); err != nil {
				break
			}
		}
		// and a header that lies about its length
		_, _ = bad.Write([]byte{0xFF, 0xFF, 0xFF, 0x7F, 0x01, 0x00, 0x00, 0x00, 0x41})
		_ = bad.Close()
	}

	// the good session is untouched: ping still answered, the world still relays
	deadline := time.Now().Add(5 * time.Second)
	for time.Now().Before(deadline) {
		good.send(jxpb.MsgId_C2G_PING, &jxpb.Ping{ClientMs: 7})
		var pong jxpb.Pong
		good.expect(jxpb.MsgId_G2C_PONG, &pong)
		if pong.ClientMs == 7 {
			break
		}
	}
	good.send(jxpb.MsgId_C2G_MOVE, &jxpb.MoveReq{Target: &jxpb.Vec2{X: 5, Y: 6}, Seq: 3})
	var mv jxpb.EntityMove
	good.expect(jxpb.MsgId_G2C_ENTITY_MOVE, &mv)
	if mv.Seq != 3 {
		t.Fatalf("world traffic broken after the garbage: %+v", &mv)
	}
	if !srv.ZoneReady() {
		t.Fatal("zone link lost")
	}
	// every garbage connection was closed by the gateway, none is left behind
	deadline = time.Now().Add(5 * time.Second)
	for srv.SessionCount() > 1 && time.Now().Before(deadline) {
		time.Sleep(20 * time.Millisecond)
	}
	if n := srv.SessionCount(); n != 1 {
		t.Fatalf("%d sessions still open, want 1", n)
	}
}
