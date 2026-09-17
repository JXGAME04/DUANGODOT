package gateway

import (
	"sync"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"google.golang.org/protobuf/proto"
)

// KSendQueue is the outbound queue of one client session (MASTER SPEC 31, 69, 70).
//
// A crowd standing on one spot produces far more updates than a slow client can read.  The old
// answer here was to close the connection ("slow consumer"), which throws a player out of the
// game for having a bad line.  The queue instead throws away what is already obsolete, by class
// of frame (frameClass):
//
//   - a newer position of an entity replaces the older one that has not been sent yet
//     (an unsent movement is worthless the moment a newer one arrives);
//   - so does a newer life value or a newer swing / flinch of the same entity: life is sent as
//     an absolute value, so only a floating damage number is lost, never the state (N5);
//   - a batch of far movements (EntityMoves) is not replaced - each carries different entities -
//     but may be thrown away like a move: the client is a third of a second behind on scenery;
//   - everything else - spawns, despawns, death and revival, chat, acks - is never dropped,
//     because the client's state depends on it (SPEC 69: state before cosmetic movement).
//
// When the queue is full, the oldest movement goes first, then the oldest combat cosmetic; only
// a queue full of frames that may not be dropped closes the session.
type KSendQueue struct {
	mu    sync.Mutex
	items []sendItem
	limit int
	// where the unsent frame of a (class, entity) sits in items, so coalescing does not scan.  At
	// 5000 players this queue took 726 000 pushes a second and the scan was the gateway's hot loop.
	at map[coalesceKey]int
	// stats
	dropped  uint64
	coalesce uint64
}

// frameClass says what may happen to a frame that has not been sent yet.
type frameClass uint8

const (
	classState    frameClass = iota // delivered, or the session closes: spawn, despawn, death, revive, chat, acks
	classMove                       // EntityMove: a newer one of the same entity replaces it; the first to go when full
	classFarMoves                   // EntityMoves: never replaced (each batch carries other entities), dropped like a move
	classCombat                     // EntityLife / attack / hurt: a newer one of the same entity replaces it; dropped after the moves
)

// a life value and a swing of the same entity are both combat cosmetics, but one must not
// replace the other: the key carries the message id
type coalesceKey struct {
	msgID  uint16
	entity uint64
}

type sendItem struct {
	class  frameClass
	msgID  uint16
	entity uint64 // for classMove / classCombat: the entity the frame is about
	data   []byte
}

func newSendQueue(limit int) *KSendQueue {
	if limit <= 0 {
		limit = 256
	}
	return &KSendQueue{limit: limit, at: make(map[coalesceKey]int, 16)}
}

// push appends a frame.  ok is false when the queue is full of frames that may not be dropped
// (the caller then closes the session).
func (q *KSendQueue) push(msgID uint16, class frameClass, entity uint64, data []byte) (ok bool) {
	q.mu.Lock()
	defer q.mu.Unlock()
	// a newer frame of the same kind about the same entity: replace the old one in place, the
	// client only cares about the latest
	replaceable := (class == classMove || class == classCombat) && entity != 0
	if replaceable {
		if i, ok := q.at[coalesceKey{msgID, entity}]; ok {
			q.items[i].data = data
			q.coalesce++
			return true
		}
	}
	if len(q.items) >= q.limit {
		// full: throw away the oldest movement; failing that the oldest combat cosmetic; never
		// anything else
		idx := -1
		for i := range q.items {
			if c := q.items[i].class; c == classMove || c == classFarMoves {
				idx = i
				break
			}
		}
		if idx < 0 {
			for i := range q.items {
				if q.items[i].class == classCombat {
					idx = i
					break
				}
			}
		}
		if idx < 0 {
			return false
		}
		q.items = append(q.items[:idx], q.items[idx+1:]...)
		q.dropped++
		q.reindex()
	}
	if replaceable {
		q.at[coalesceKey{msgID, entity}] = len(q.items)
	}
	q.items = append(q.items, sendItem{class: class, msgID: msgID, entity: entity, data: data})
	return true
}

// reindex rebuilds the index after items shifted; only the rare "queue full" path needs it.
func (q *KSendQueue) reindex() {
	clear(q.at)
	for i := range q.items {
		if c := q.items[i].class; (c == classMove || c == classCombat) && q.items[i].entity != 0 {
			q.at[coalesceKey{q.items[i].msgID, q.items[i].entity}] = i
		}
	}
}

// pop takes the next frame to write; ok is false when the queue is empty.
func (q *KSendQueue) pop() (data []byte, ok bool) {
	q.mu.Lock()
	defer q.mu.Unlock()
	if len(q.items) == 0 {
		return nil, false
	}
	item := q.items[0]
	q.items = append(q.items[:0], q.items[1:]...)
	q.reindex()
	return item.data, true
}

// drain moves every queued frame into dst and empties the queue, taking the lock once.
//
// Popping one frame at a time cost a mutex round trip and an O(n) shift of the slice for each of
// the 508 000 frames a second this gateway pushed at 5000 players; draining is one lock and one
// copy per round instead.
func (q *KSendQueue) drain(dst [][]byte) [][]byte {
	q.mu.Lock()
	defer q.mu.Unlock()
	for i := range q.items {
		dst = append(dst, q.items[i].data)
		q.items[i].data = nil // do not keep the frame alive through the queue's own storage
	}
	q.items = q.items[:0]
	clear(q.at)
	return dst
}

func (q *KSendQueue) len() int {
	q.mu.Lock()
	defer q.mu.Unlock()
	return len(q.items)
}

func (q *KSendQueue) stats() (dropped, coalesced uint64) {
	q.mu.Lock()
	defer q.mu.Unlock()
	return q.dropped, q.coalesce
}

// classify says what may happen to a frame from the zone while it waits in a client's queue,
// and which entity it is about.  It reads the payload once per zone packet, before the fan-out
// to every session that receives it.
func classify(msgID uint16, payload []byte) (frameClass, uint64) {
	switch jxpb.MsgId(msgID) {
	case jxpb.MsgId_G2C_ENTITY_MOVE:
		var m jxpb.EntityMove
		if proto.Unmarshal(payload, &m) == nil && m.EntityId != 0 {
			return classMove, m.EntityId
		}
	case jxpb.MsgId_G2C_ENTITY_MOVES:
		return classFarMoves, 0
	case jxpb.MsgId_G2C_ENTITY_LIFE:
		var m jxpb.EntityLife
		if proto.Unmarshal(payload, &m) == nil && m.EntityId != 0 {
			return classCombat, m.EntityId
		}
	case jxpb.MsgId_G2C_ENTITY_ACTION:
		var m jxpb.EntityAction
		if proto.Unmarshal(payload, &m) == nil && m.EntityId != 0 && (m.Action == jxpb.Action_ACTION_ATTACK || m.Action == jxpb.Action_ACTION_HURT) {
			return classCombat, m.EntityId
		}
	}
	return classState, 0
}
