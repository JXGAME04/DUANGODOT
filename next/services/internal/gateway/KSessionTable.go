package gateway

// KSessionTable maps a session id to its session without taking a lock to read.
//
// The zone sends one packet with the list of sessions that must receive it, and one goroutine
// walks that list.  With a map behind a RWMutex that is one lock round trip per recipient, and at
// 10 000 players the zone link produced ~46 million of those in two minutes: the goroutine became
// the gateway's single serialising point, and the ack a player waits for to enter the world queued
// behind the crowd's movement.  Measured: entering the world took 8 seconds on average and up to
// 30 seconds (MASTER SPEC 55, 69).
//
// Session ids come from one counter, so they are dense and increasing.  The table is a list of
// fixed chunks: a chunk is allocated once and never moves, so a reader that already holds the
// chunk list keeps reading correct values while the table grows.
//
// Writes (a client connecting or leaving) still take the mutex; there are a few thousand of those,
// against tens of millions of reads.

import (
	"sync"
	"sync/atomic"
)

// sessionChunk is how many slots one chunk holds.  Big enough that growth is rare, small enough
// that an idle gateway does not hold a megabyte of pointers.
const sessionChunk = 4096

// A session id carries the zone's per-gateway prefix in its top 16 bits (ZoneHelloAck.session_prefix),
// so the id itself is a huge sparse number and must never be used as an array index: indexing by
// the raw id asked Go for a terabyte and killed the gateway on the first client.  The low bits are
// the gateway's own counter, and they are dense.
const sessionIndexMask = (uint64(1) << 48) - 1

func sessionIndex(sid uint64) uint64 { return sid & sessionIndexMask }

type sessionSlots = [sessionChunk]atomic.Pointer[session]

type KSessionTable struct {
	mu     sync.Mutex // only for growing the chunk list
	chunks atomic.Pointer[[]*sessionSlots]
	count  atomic.Int64
}

// get returns the session with this id, or nil.  Lock free.
func (t *KSessionTable) get(sid uint64) *session {
	chunks := t.chunks.Load()
	if chunks == nil {
		return nil
	}
	idx := sessionIndex(sid)
	i, off := idx/sessionChunk, idx%sessionChunk
	if i >= uint64(len(*chunks)) {
		return nil
	}
	// The slot is found by the low bits; the full id decides whether it is really this session.
	if s := (*chunks)[i][off].Load(); s != nil && s.sid == sid {
		return s
	}
	return nil
}

// put stores a session under its id, growing the table when needed.
func (t *KSessionTable) put(sid uint64, sess *session) {
	t.mu.Lock()
	idx := sessionIndex(sid)
	i, off := idx/sessionChunk, idx%sessionChunk
	chunks := t.chunks.Load()
	var list []*sessionSlots
	if chunks != nil {
		list = *chunks
	}
	if i >= uint64(len(list)) {
		grown := make([]*sessionSlots, i+1)
		copy(grown, list) // the chunks themselves are shared, only the list is copied
		for j := len(list); j < len(grown); j++ {
			grown[j] = &sessionSlots{}
		}
		list = grown
		t.chunks.Store(&list)
	}
	list[i][off].Store(sess)
	t.mu.Unlock()
	t.count.Add(1)
}

// remove clears the slot of a session that has ended.
func (t *KSessionTable) remove(sid uint64) {
	chunks := t.chunks.Load()
	if chunks == nil {
		return
	}
	idx := sessionIndex(sid)
	i, off := idx/sessionChunk, idx%sessionChunk
	if i >= uint64(len(*chunks)) {
		return
	}
	if (*chunks)[i][off].Swap(nil) != nil {
		t.count.Add(-1)
	}
}

// len is how many sessions are in the table.
func (t *KSessionTable) len() int { return int(t.count.Load()) }

// each calls fn for every session currently in the table, in id order.
func (t *KSessionTable) each(fn func(*session)) {
	chunks := t.chunks.Load()
	if chunks == nil {
		return
	}
	for _, chunk := range *chunks {
		for i := range chunk {
			if s := chunk[i].Load(); s != nil {
				fn(s)
			}
		}
	}
}
