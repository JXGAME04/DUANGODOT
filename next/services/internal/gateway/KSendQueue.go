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
	return &KSendQueue{limit: limit}
}

// push appends a frame.  ok is false when the queue is full of frames that may not be dropped
// (the caller then closes the session).
func (q *KSendQueue) push(msgID uint16, entity uint64, data []byte) (ok bool) {
	q.mu.Lock()
	defer q.mu.Unlock()
	// a newer position of the same entity: replace the old frame in place, the client only
	// cares about the latest one
	if entity != 0 {
		for i := range q.items {
			if q.items[i].entity == entity && q.items[i].msgID == msgID {
				q.items[i].data = data
				q.coalesce++
				return true
			}
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
	}
	q.items = append(q.items, sendItem{msgID: msgID, entity: entity, data: data})
	return true
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
	return item.data, true
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
