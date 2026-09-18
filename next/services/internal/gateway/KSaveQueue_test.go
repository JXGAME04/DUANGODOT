package gateway

import (
	"context"
	"errors"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

// slowStore counts saves, holds each one for `hold`, fails the first `failFirst` saves of a
// player, and records how many writes ran at the same time.
type slowStore struct {
	persist.Store
	mu        sync.Mutex
	hold      time.Duration
	failFirst int
	tries     map[uint64]int
	levels    map[uint64]uint32 // last level written per player
	running   atomic.Int64
	maxRun    atomic.Int64
	started   chan struct{}
}

func newSlowStore(hold time.Duration) *slowStore {
	return &slowStore{hold: hold, tries: map[uint64]int{}, levels: map[uint64]uint32{}, started: make(chan struct{}, 1024)}
}

func (s *slowStore) SaveCharacter(_ context.Context, role *jxpb.RoleData) error {
	n := s.running.Add(1)
	for {
		m := s.maxRun.Load()
		if n <= m || s.maxRun.CompareAndSwap(m, n) {
			break
		}
	}
	select {
	case s.started <- struct{}{}:
	default:
	}
	time.Sleep(s.hold)
	s.running.Add(-1)
	s.mu.Lock()
	defer s.mu.Unlock()
	s.tries[role.PlayerId]++
	if s.tries[role.PlayerId] <= s.failFirst {
		return errors.New("database says no")
	}
	s.levels[role.PlayerId] = role.Level
	return nil
}

func (s *slowStore) level(pid uint64) (uint32, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	l, ok := s.levels[pid]
	return l, ok
}

func TestSaveQueueCoalescesPerCharacterAndDrainsOnClose(t *testing.T) {
	st := newSlowStore(20 * time.Millisecond)
	var finals []uint64
	var fmu sync.Mutex
	q := newSaveQueue(st, 2, time.Second, func(sid uint64) {
		fmu.Lock()
		finals = append(finals, sid)
		fmu.Unlock()
	})
	// one character saved three times while the workers are busy: only the newest state is written
	q.push(1, &jxpb.RoleData{PlayerId: 1, AccountId: 1, Level: 1}, false)
	q.push(2, &jxpb.RoleData{PlayerId: 2, AccountId: 2, Level: 1}, false)
	<-st.started // a worker holds player 1 (or 2)
	<-st.started
	q.push(3, &jxpb.RoleData{PlayerId: 3, AccountId: 3, Level: 1}, false)
	q.push(3, &jxpb.RoleData{PlayerId: 3, AccountId: 3, Level: 2}, false)
	q.push(3, &jxpb.RoleData{PlayerId: 3, AccountId: 3, Level: 3}, true) // the last one is final: it must stick
	q.push(4, &jxpb.RoleData{PlayerId: 4, AccountId: 4, Level: 9}, true)
	q.Close()
	if l, ok := st.level(3); !ok || l != 3 {
		t.Fatalf("player 3 written as level %d (ok %v), want the newest 3", l, ok)
	}
	if st.tries[3] != 1 {
		t.Fatalf("player 3 written %d times, want 1 (coalesced)", st.tries[3])
	}
	if q.Coalesced.Load() != 2 || q.Saves.Load() != 4 || q.Errors.Load() != 0 {
		t.Fatalf("stats: coalesced %d saves %d errors %d", q.Coalesced.Load(), q.Saves.Load(), q.Errors.Load())
	}
	fmu.Lock()
	defer fmu.Unlock()
	if len(finals) != 2 {
		t.Fatalf("final saves reported: %v", finals)
	}
	if st.maxRun.Load() > 2 {
		t.Fatalf("%d writes ran at once with 2 workers", st.maxRun.Load())
	}
	// after Close a late save is still written, on the caller
	q.push(5, &jxpb.RoleData{PlayerId: 5, AccountId: 5, Level: 1}, false)
	if _, ok := st.level(5); !ok {
		t.Fatal("a save after Close was lost")
	}
}

func TestSaveQueueRetriesAFinalSaveAndGivesUpOnAPeriodicOne(t *testing.T) {
	st := newSlowStore(0)
	st.failFirst = 2
	var finals int32
	q := newSaveQueue(st, 1, time.Second, func(uint64) { atomic.AddInt32(&finals, 1) })
	q.push(1, &jxpb.RoleData{PlayerId: 1, AccountId: 1, Level: 5}, true)  // fails twice, third try succeeds
	q.push(2, &jxpb.RoleData{PlayerId: 2, AccountId: 2, Level: 5}, false) // fails once, not retried: the next interval brings another
	q.Close()
	if st.tries[1] != 3 {
		t.Fatalf("final save tried %d times, want 3", st.tries[1])
	}
	if l, ok := st.level(1); !ok || l != 5 {
		t.Fatal("the final save must end up in the store")
	}
	if st.tries[2] != 1 {
		t.Fatalf("periodic save tried %d times, want 1", st.tries[2])
	}
	if q.Saves.Load() != 1 || q.Errors.Load() != 3 || q.Lost.Load() != 0 || atomic.LoadInt32(&finals) != 1 {
		t.Fatalf("stats: saves %d errors %d lost %d finals %d", q.Saves.Load(), q.Errors.Load(), q.Lost.Load(), finals)
	}

	// a final save the database refuses three times is reported lost, and still reported as arrived
	// (the session must not wait for ever)
	st2 := newSlowStore(0)
	st2.failFirst = 10
	var finals2 int32
	q2 := newSaveQueue(st2, 1, time.Second, func(uint64) { atomic.AddInt32(&finals2, 1) })
	q2.push(1, &jxpb.RoleData{PlayerId: 1, AccountId: 1}, true)
	q2.Close()
	if q2.Lost.Load() != 1 || st2.tries[1] != saveFinalTries || atomic.LoadInt32(&finals2) != 1 {
		t.Fatalf("lost %d tries %d finals %d", q2.Lost.Load(), st2.tries[1], finals2)
	}
}
