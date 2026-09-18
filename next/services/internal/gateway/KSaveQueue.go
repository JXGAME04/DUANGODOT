package gateway

import (
	"context"
	"sync"
	"sync/atomic"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

// KSaveQueue writes characters to the store off the zone link.
//
// The zone hands the gateway a PlayerSave per character, spread over the save interval (one
// player per tick slot, KGameServer).  Writing it where it arrives - on the goroutine that reads
// the zone link - would stall every packet of every player behind one database round trip.
// So saves go through this queue: a few workers write them, the newest state of a character
// replaces an older one still waiting (there is nothing to gain from writing both), and the
// final save of a character that left is never replaced away and is tried again when the
// database refuses it - that one is the player's progress.
//
// The owner's rule for a server of 20 000 players: save per character, never in a burst.  The
// zone spreads the saves; this queue keeps a burst that reaches the gateway anyway (a mass
// logout, a reconnecting zone) from turning into 20 000 concurrent writes: `workers` at a time,
// the rest wait here, and a backlog above warnAt is logged so it is seen.
type KSaveQueue struct {
	store   persist.Store
	timeout time.Duration
	onFinal func(sid uint64) // called after a final save was written (or given up on)

	mu      sync.Mutex
	cond    *sync.Cond
	pending map[uint64]*saveItem // player id -> its waiting save (the newest)
	order   []uint64             // player ids in arrival order
	closed  bool
	wg      sync.WaitGroup

	// stats
	Saves     atomic.Uint64 // writes that succeeded
	Errors    atomic.Uint64 // writes that failed (a periodic one is retried by the next interval)
	Coalesced atomic.Uint64 // saves replaced by a newer one before they were written
	Lost      atomic.Uint64 // final saves the database refused three times: progress lost, logged
	SaveNs    atomic.Uint64 // time spent writing, for the average
	peak      atomic.Int64  // longest backlog seen since the last stats line
	warnedAt  time.Time
}

type saveItem struct {
	sid   uint64
	role  *jxpb.RoleData
	final bool
	tries int
}

const (
	saveWarnAt      = 1000 // backlog that gets a warning line
	saveFinalTries  = 3
	saveRetryPause  = 200 * time.Millisecond
	saveWarnEvery   = 10 * time.Second
	saveDefaultWork = 4
)

func newSaveQueue(store persist.Store, workers int, timeout time.Duration, onFinal func(sid uint64)) *KSaveQueue {
	if workers <= 0 {
		workers = saveDefaultWork
	}
	if timeout <= 0 {
		timeout = 10 * time.Second
	}
	q := &KSaveQueue{store: store, timeout: timeout, onFinal: onFinal, pending: map[uint64]*saveItem{}}
	q.cond = sync.NewCond(&q.mu)
	for i := 0; i < workers; i++ {
		q.wg.Add(1)
		go q.worker()
	}
	return q
}

// push queues the newest state of a character.  A save already waiting for the same character
// is replaced; `final` sticks once set.
func (q *KSaveQueue) push(sid uint64, role *jxpb.RoleData, final bool) {
	q.mu.Lock()
	if q.closed {
		q.mu.Unlock()
		// the queue is draining for shutdown: write it here, on the caller, rather than lose it
		q.write(&saveItem{sid: sid, role: role, final: final})
		return
	}
	if it, ok := q.pending[role.PlayerId]; ok {
		it.role = role
		it.final = it.final || final
		it.sid = sid
		q.Coalesced.Add(1)
		q.mu.Unlock()
		return
	}
	q.pending[role.PlayerId] = &saveItem{sid: sid, role: role, final: final}
	q.order = append(q.order, role.PlayerId)
	n := int64(len(q.order))
	if n > q.peak.Load() {
		q.peak.Store(n)
	}
	warn := n >= saveWarnAt && time.Since(q.warnedAt) > saveWarnEvery
	if warn {
		q.warnedAt = time.Now()
	}
	q.mu.Unlock()
	q.cond.Signal()
	if warn {
		log.Warn("db", "save backlog", log.F("waiting", n))
	}
}

// Len is how many characters wait to be written.
func (q *KSaveQueue) Len() int {
	q.mu.Lock()
	defer q.mu.Unlock()
	return len(q.order)
}

// Peak returns the longest backlog since the last call and resets it.
func (q *KSaveQueue) Peak() int64 { return q.peak.Swap(int64(q.Len())) }

func (q *KSaveQueue) worker() {
	defer q.wg.Done()
	for {
		q.mu.Lock()
		for len(q.order) == 0 && !q.closed {
			q.cond.Wait()
		}
		if len(q.order) == 0 && q.closed {
			q.mu.Unlock()
			return
		}
		pid := q.order[0]
		q.order = q.order[1:]
		it := q.pending[pid]
		delete(q.pending, pid)
		q.mu.Unlock()
		q.write(it)
	}
}

// write does one save, with the retries a final save deserves.
func (q *KSaveQueue) write(it *saveItem) {
	ctx := log.WithContext(context.Background(), log.Context{Sid: it.sid, Pid: it.role.PlayerId})
	for {
		started := time.Now()
		tctx, cancel := context.WithTimeout(context.Background(), q.timeout)
		err := q.store.SaveCharacter(tctx, it.role)
		cancel()
		q.SaveNs.Add(uint64(time.Since(started)))
		if err == nil {
			q.Saves.Add(1)
			log.DebugCtx(ctx, "db", "character saved", log.F("final", it.final), log.F("level", it.role.Level))
			break
		}
		q.Errors.Add(1)
		it.tries++
		if !it.final || it.tries >= saveFinalTries {
			if it.final {
				q.Lost.Add(1)
				log.ErrorCtx(ctx, "db", "final save lost", log.F("error", err), log.F("tries", it.tries))
			} else {
				log.ErrorCtx(ctx, "db", "save failed", log.F("error", err))
			}
			break
		}
		log.WarnCtx(ctx, "db", "final save retried", log.F("error", err), log.F("try", it.tries))
		time.Sleep(saveRetryPause)
	}
	if it.final && q.onFinal != nil {
		q.onFinal(it.sid)
	}
}

// Close stops taking new work into the queue, writes everything still waiting and returns.
func (q *KSaveQueue) Close() {
	q.mu.Lock()
	q.closed = true
	q.mu.Unlock()
	q.cond.Broadcast()
	q.wg.Wait()
}
