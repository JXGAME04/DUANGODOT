package gateway

import (
	"sync"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
)

// KSendQueue is the outbound queue of one client session (MASTER SPEC 31, 69, 70).
//
// A crowd standing on one spot produces far more updates than a slow client can read.  The old
// answer here was to close the connection ("slow consumer"), which throws a player out of the
// game for having a bad line.  The queue instead throws away what is already obsolete:
//
//   - a newer position of an entity replaces the older one that has not been sent yet
//     (an unsent movement is worthless the moment a newer one arrives);
//   - everything else - spawns, despawns, damage, chat, acks - is never dropped, because the
//     client's state depends on it (SPEC 69: combat and state before cosmetic movement).
//
// Only when the queue is full of messages that may not be dropped is the session closed.
type KSendQueue struct {
	mu    sync.Mutex
	items []sendItem
	limit int
	// where the unsent position of an entity sits in items, so coalescing does not scan.  At 5000
	// players this queue took 726 000 pushes a second and the scan was the gateway's hot loop.
	moveAt map[uint64]int
	// stats
	dropped  uint64
	coalesce uint64
}

type sendItem struct {
	msgID uint16
	// entity whose position this frame carries (0 = not a movement): a newer frame for the
	// same entity replaces this one
	entity uint64
	data   []byte
}

func newSendQueue(limit int) *KSendQueue {
	if limit <= 0 {
		limit = 256
	}
	return &KSendQueue{limit: limit, moveAt: make(map[uint64]int, 16)}
}

// push appends a frame.  ok is false when the queue is full of frames that may not be dropped
// (the caller then closes the session).
func (q *KSendQueue) push(msgID uint16, entity uint64, data []byte) (ok bool) {
	q.mu.Lock()
	defer q.mu.Unlock()
	// a newer position of the same entity: replace the old frame in place, the client only
	// cares about the latest one
	if entity != 0 {
		if i, ok := q.moveAt[entity]; ok && q.items[i].msgID == msgID {
			q.items[i].data = data
			q.coalesce++
			return true
		}
	}
	if len(q.items) >= q.limit {
		// full: throw away the oldest movement frame, never anything else
		idx := -1
		for i := range q.items {
			if q.items[i].entity != 0 {
				idx = i
				break
			}
		}
		if idx < 0 {
			return false
		}
		q.items = append(q.items[:idx], q.items[idx+1:]...)
		q.dropped++
		q.reindex()
	}
	if entity != 0 {
		q.moveAt[entity] = len(q.items)
	}
	q.items = append(q.items, sendItem{msgID: msgID, entity: entity, data: data})
	return true
}

// reindex rebuilds moveAt after items shifted; only the rare "queue full" path needs it.
func (q *KSendQueue) reindex() {
	clear(q.moveAt)
	for i := range q.items {
		if q.items[i].entity != 0 {
			q.moveAt[q.items[i].entity] = i
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
	clear(q.moveAt)
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

// droppableEntity says which entity a frame is about when the frame is a position update that
// may be replaced by a newer one; 0 means "must be delivered".
func droppableEntity(msgID uint16, entity uint64) uint64 {
	if jxpb.MsgId(msgID) == jxpb.MsgId_G2C_ENTITY_MOVE {
		return entity
	}
	return 0
}
