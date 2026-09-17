package gateway

import (
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"google.golang.org/protobuf/proto"
)

const (
	msgMove   = uint16(jxpb.MsgId_G2C_ENTITY_MOVE)
	msgMoves  = uint16(jxpb.MsgId_G2C_ENTITY_MOVES)
	msgLife   = uint16(jxpb.MsgId_G2C_ENTITY_LIFE)
	msgAction = uint16(jxpb.MsgId_G2C_ENTITY_ACTION)
	msgSpawn  = uint16(jxpb.MsgId_G2C_ENTITY_SPAWN)
	msgChat   = uint16(jxpb.MsgId_G2C_CHAT_MSG)
)

func drainStrings(q *KSendQueue) []string {
	var got []string
	for {
		d, ok := q.pop()
		if !ok {
			return got
		}
		got = append(got, string(d))
	}
}

// SPEC 70: when a client cannot keep up, the server throws away what is obsolete - never the
// state the client needs, and never the player.
func TestSendQueueDropsObsoleteMovementOnly(t *testing.T) {
	q := newSendQueue(4)

	// a newer position of the same entity replaces the one still waiting
	if !q.push(msgMove, classMove, 7, []byte("old")) || !q.push(msgMove, classMove, 7, []byte("new")) {
		t.Fatal("push failed")
	}
	if q.len() != 1 {
		t.Fatalf("queue holds %d frames, want 1 coalesced", q.len())
	}
	data, ok := q.pop()
	if !ok || string(data) != "new" {
		t.Fatalf("got %q, want the newest position", data)
	}
	_, coalesced := q.stats()
	if coalesced != 1 {
		t.Fatalf("coalesced %d", coalesced)
	}

	// a full queue drops the oldest movement to make room, and keeps everything else
	q = newSendQueue(3)
	q.push(msgSpawn, classState, 0, []byte("spawn"))
	q.push(msgMove, classMove, 1, []byte("move1"))
	q.push(msgMove, classMove, 2, []byte("move2"))
	if !q.push(msgChat, classState, 0, []byte("chat")) {
		t.Fatal("a full queue must make room for state the client needs")
	}
	dropped, _ := q.stats()
	if dropped != 1 {
		t.Fatalf("dropped %d, want 1", dropped)
	}
	got := drainStrings(q)
	if len(got) != 3 || got[0] != "spawn" || got[1] != "move2" || got[2] != "chat" {
		t.Fatalf("queue content %v", got)
	}

	// a queue full of undroppable frames closes the session instead of lying to the client
	q = newSendQueue(2)
	q.push(msgSpawn, classState, 0, []byte("a"))
	q.push(msgChat, classState, 0, []byte("b"))
	if q.push(msgSpawn, classState, 0, []byte("c")) {
		t.Fatal("must refuse when nothing may be dropped")
	}
}

// N5: combat cosmetics coalesce per entity too, and go after the movement when the queue is full.
func TestSendQueueCoalescesCombatAndDropsItAfterMovement(t *testing.T) {
	q := newSendQueue(8)
	// a newer life value of the same entity replaces the unsent one (life is absolute)
	q.push(msgLife, classCombat, 7, []byte("life 90"))
	q.push(msgLife, classCombat, 7, []byte("life 80"))
	// ... but a swing of the same entity is another frame, not the same one
	q.push(msgAction, classCombat, 7, []byte("swing"))
	q.push(msgAction, classCombat, 7, []byte("swing again"))
	if got := drainStrings(q); len(got) != 2 || got[0] != "life 80" || got[1] != "swing again" {
		t.Fatalf("queue content %v", got)
	}

	// full: the movement goes first, then the combat cosmetic, never the spawn
	q = newSendQueue(3)
	q.push(msgSpawn, classState, 0, []byte("spawn"))
	q.push(msgLife, classCombat, 1, []byte("life"))
	q.push(msgMove, classMove, 2, []byte("move"))
	if !q.push(msgChat, classState, 0, []byte("chat")) {
		t.Fatal("room must be made")
	}
	if got := drainStrings(q); len(got) != 3 || got[0] != "spawn" || got[1] != "life" || got[2] != "chat" {
		t.Fatalf("after dropping the move: %v", got)
	}
	q = newSendQueue(2)
	q.push(msgSpawn, classState, 0, []byte("spawn"))
	q.push(msgLife, classCombat, 1, []byte("life"))
	if !q.push(msgChat, classState, 0, []byte("chat")) {
		t.Fatal("room must be made from the combat cosmetic")
	}
	if got := drainStrings(q); len(got) != 2 || got[0] != "spawn" || got[1] != "chat" {
		t.Fatalf("after dropping the life: %v", got)
	}

	// a batch of far moves is never replaced by a newer batch (other entities), but is dropped like a move
	q = newSendQueue(2)
	q.push(msgMoves, classFarMoves, 0, []byte("batch1"))
	q.push(msgMoves, classFarMoves, 0, []byte("batch2"))
	if q.len() != 2 {
		t.Fatalf("batches coalesced: %d", q.len())
	}
	if !q.push(msgChat, classState, 0, []byte("chat")) {
		t.Fatal("room must be made from a batch")
	}
	if got := drainStrings(q); len(got) != 2 || got[0] != "batch2" || got[1] != "chat" {
		t.Fatalf("after dropping a batch: %v", got)
	}
}

func TestClassify(t *testing.T) {
	mv, _ := proto.Marshal(&jxpb.EntityMove{EntityId: 5})
	if c, e := classify(msgMove, mv); c != classMove || e != 5 {
		t.Fatalf("move: %v %d", c, e)
	}
	if c, _ := classify(msgMoves, nil); c != classFarMoves {
		t.Fatalf("moves batch: %v", c)
	}
	life, _ := proto.Marshal(&jxpb.EntityLife{EntityId: 5, Life: 10})
	if c, e := classify(msgLife, life); c != classCombat || e != 5 {
		t.Fatalf("life: %v %d", c, e)
	}
	for _, a := range []jxpb.Action{jxpb.Action_ACTION_ATTACK, jxpb.Action_ACTION_HURT} {
		act, _ := proto.Marshal(&jxpb.EntityAction{EntityId: 5, Action: a})
		if c, e := classify(msgAction, act); c != classCombat || e != 5 {
			t.Fatalf("%v: %v %d", a, c, e)
		}
	}
	// death and revival are state: never replaced, never dropped
	for _, a := range []jxpb.Action{jxpb.Action_ACTION_DEATH, jxpb.Action_ACTION_REVIVE} {
		act, _ := proto.Marshal(&jxpb.EntityAction{EntityId: 5, Action: a})
		if c, _ := classify(msgAction, act); c != classState {
			t.Fatalf("%v must be state, got %v", a, c)
		}
	}
	for _, id := range []jxpb.MsgId{jxpb.MsgId_G2C_ENTITY_SPAWN, jxpb.MsgId_G2C_ENTITY_DESPAWN, jxpb.MsgId_G2C_CHANGE_MAP,
		jxpb.MsgId_G2C_KICK, jxpb.MsgId_G2C_CHAT_MSG} {
		if c, _ := classify(uint16(id), nil); c != classState {
			t.Fatalf("%v must never be dropped", id)
		}
	}
}

func TestDrainTakesEverythingInOrder(t *testing.T) {
	q := newSendQueue(8)
	q.push(msgSpawn, classState, 0, []byte("spawn"))
	q.push(msgMove, classMove, 7, []byte("move1"))
	q.push(msgChat, classState, 0, []byte("chat"))

	var got []string
	for _, b := range q.drain(nil) {
		got = append(got, string(b))
	}
	if len(got) != 3 || got[0] != "spawn" || got[1] != "move1" || got[2] != "chat" {
		t.Fatalf("drained %v", got)
	}
	if q.len() != 0 {
		t.Fatalf("queue still holds %d", q.len())
	}
	if n := len(q.drain(nil)); n != 0 {
		t.Fatalf("draining an empty queue gave %d", n)
	}
}

func TestDrainReusesTheCallersSlice(t *testing.T) {
	q := newSendQueue(8)
	dst := make([][]byte, 0, 4)
	for round := 0; round < 3; round++ {
		q.push(msgChat, classState, 0, []byte("x"))
		q.push(msgChat, classState, 0, []byte("y"))
		dst = q.drain(dst[:0])
		if len(dst) != 2 {
			t.Fatalf("round %d drained %d", round, len(dst))
		}
	}
	if cap(dst) != 4 {
		t.Fatalf("drain grew the slice to %d: it should reuse it", cap(dst))
	}
}

func TestDrainStillCoalescesMovement(t *testing.T) {
	q := newSendQueue(8)
	q.push(msgMove, classMove, 7, []byte("old"))
	q.push(msgMove, classMove, 7, []byte("new"))
	out := q.drain(nil)
	if len(out) != 1 || string(out[0]) != "new" {
		t.Fatalf("coalescing broke: %v", out)
	}
	if _, coalesced := q.stats(); coalesced != 1 {
		t.Fatalf("coalesced = %d, want 1", coalesced)
	}
}
