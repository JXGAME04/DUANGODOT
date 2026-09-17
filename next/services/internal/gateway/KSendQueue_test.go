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
