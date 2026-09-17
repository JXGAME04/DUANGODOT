package gateway

import (
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
)

// SPEC 70: when a client cannot keep up, the server throws away what is obsolete - never the
// state the client needs, and never the player.
func TestSendQueueDropsObsoleteMovementOnly(t *testing.T) {
	q := newSendQueue(4)
	const move = uint16(jxpb.MsgId_G2C_ENTITY_MOVE)

	// a newer position of the same entity replaces the one still waiting
	if !q.push(move, 7, []byte("old")) || !q.push(move, 7, []byte("new")) {
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
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_SPAWN), 0, []byte("spawn"))
	q.push(move, 1, []byte("move1"))
	q.push(move, 2, []byte("move2"))
	if !q.push(uint16(jxpb.MsgId_G2C_ENTITY_LIFE), 0, []byte("damage")) {
		t.Fatal("a full queue must make room for state the client needs")
	}
	dropped, _ := q.stats()
	if dropped != 1 {
		t.Fatalf("dropped %d, want 1", dropped)
	}
	var got []string
	for {
		d, ok := q.pop()
		if !ok {
			break
		}
		got = append(got, string(d))
	}
	if len(got) != 3 || got[0] != "spawn" || got[1] != "move2" || got[2] != "damage" {
		t.Fatalf("queue content %v", got)
	}

	// a queue full of undroppable frames closes the session instead of lying to the client
	q = newSendQueue(2)
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_SPAWN), 0, []byte("a"))
	q.push(uint16(jxpb.MsgId_G2C_CHAT_MSG), 0, []byte("b"))
	if q.push(uint16(jxpb.MsgId_G2C_ENTITY_LIFE), 0, []byte("c")) {
		t.Fatal("must refuse when nothing may be dropped")
	}
}

func TestDroppableEntity(t *testing.T) {
	if droppableEntity(uint16(jxpb.MsgId_G2C_ENTITY_MOVE), 5) != 5 {
		t.Fatal("a movement frame is droppable")
	}
	for _, id := range []jxpb.MsgId{jxpb.MsgId_G2C_ENTITY_SPAWN, jxpb.MsgId_G2C_ENTITY_DESPAWN,
		jxpb.MsgId_G2C_ENTITY_LIFE, jxpb.MsgId_G2C_ENTITY_ACTION, jxpb.MsgId_G2C_CHANGE_MAP, jxpb.MsgId_G2C_KICK} {
		if droppableEntity(uint16(id), 5) != 0 {
			t.Fatalf("%v must never be dropped", id)
		}
	}
}

func TestDrainTakesEverythingInOrder(t *testing.T) {
	q := newSendQueue(8)
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_SPAWN), 0, []byte("spawn"))
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_MOVE), 7, []byte("move1"))
	q.push(uint16(jxpb.MsgId_G2C_CHAT_MSG), 0, []byte("chat"))

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
		q.push(uint16(jxpb.MsgId_G2C_CHAT_MSG), 0, []byte("x"))
		q.push(uint16(jxpb.MsgId_G2C_CHAT_MSG), 0, []byte("y"))
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
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_MOVE), 7, []byte("old"))
	q.push(uint16(jxpb.MsgId_G2C_ENTITY_MOVE), 7, []byte("new"))
	out := q.drain(nil)
	if len(out) != 1 || string(out[0]) != "new" {
		t.Fatalf("coalescing broke: %v", out)
	}
	if _, coalesced := q.stats(); coalesced != 1 {
		t.Fatalf("coalesced = %d, want 1", coalesced)
	}
}
