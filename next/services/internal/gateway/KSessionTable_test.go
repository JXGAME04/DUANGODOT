package gateway

import (
	"sync"
	"testing"
)

func TestSessionTableStoresAndFinds(t *testing.T) {
	var tab KSessionTable
	a, b := &session{sid: 1}, &session{sid: 9000}

	if tab.get(1) != nil || tab.len() != 0 {
		t.Fatal("an empty table must find nothing")
	}
	tab.put(1, a)
	tab.put(9000, b) // far past the first chunk: the table has to grow
	if tab.get(1) != a || tab.get(9000) != b {
		t.Fatal("stored sessions are not found again")
	}
	if tab.get(2) != nil || tab.get(1<<20) != nil {
		t.Fatal("an id that was never stored must give nil")
	}
	if tab.len() != 2 {
		t.Fatalf("len = %d, want 2", tab.len())
	}

	tab.remove(1)
	if tab.get(1) != nil {
		t.Fatal("a removed session is still there")
	}
	if tab.len() != 1 {
		t.Fatalf("len after remove = %d, want 1", tab.len())
	}
	tab.remove(1) // removing twice must not move the count again
	if tab.len() != 1 {
		t.Fatalf("len after removing twice = %d, want 1", tab.len())
	}
}

func TestSessionTableVisitsEverySession(t *testing.T) {
	var tab KSessionTable
	want := map[uint64]bool{}
	for _, sid := range []uint64{0, 1, 4095, 4096, 4097, 20000} {
		tab.put(sid, &session{sid: sid})
		want[sid] = true
	}
	got := map[uint64]bool{}
	tab.each(func(s *session) { got[s.sid] = true })
	if len(got) != len(want) {
		t.Fatalf("each visited %d sessions, want %d", len(got), len(want))
	}
	for sid := range want {
		if !got[sid] {
			t.Fatalf("each skipped %d", sid)
		}
	}
}

// The fanout reads the table from one goroutine while clients connect and leave on others; a
// reader must never see a torn value or a stale chunk while the table grows.
func TestSessionTableReadsWhileItGrows(t *testing.T) {
	var tab KSessionTable
	const n = 20000
	done := make(chan struct{})
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		for {
			select {
			case <-done:
				return
			default:
			}
			for sid := uint64(0); sid < n; sid++ {
				if s := tab.get(sid); s != nil && s.sid != sid {
					t.Errorf("get(%d) gave the session of %d", sid, s.sid)
					return
				}
			}
		}
	}()

	for sid := uint64(0); sid < n; sid++ {
		tab.put(sid, &session{sid: sid})
	}
	close(done)
	wg.Wait()

	if tab.len() != n {
		t.Fatalf("len = %d, want %d", tab.len(), n)
	}
	for _, sid := range []uint64{0, 4095, 4096, n - 1} {
		if s := tab.get(sid); s == nil || s.sid != sid {
			t.Fatalf("get(%d) = %v after growing", sid, s)
		}
	}
}

// The zone gives every gateway link a prefix in the top bits of its session ids, so a raw id is a
// huge sparse number.  Using it as an array index asked Go for a terabyte and killed the gateway
// on the first client that connected; the table has to index by the dense low bits.
func TestSessionTableHandlesPrefixedIds(t *testing.T) {
	const prefix = uint64(2) << 48
	var tab KSessionTable
	for i := uint64(1); i <= 5; i++ {
		tab.put(prefix|i, &session{sid: prefix | i})
	}
	if tab.len() != 5 {
		t.Fatalf("len = %d, want 5", tab.len())
	}
	for i := uint64(1); i <= 5; i++ {
		s := tab.get(prefix | i)
		if s == nil || s.sid != prefix|i {
			t.Fatalf("get(%d) = %v", prefix|i, s)
		}
	}
	// The same low bits under a different prefix belong to another gateway, never to this one.
	if s := tab.get((uint64(3) << 48) | 1); s != nil {
		t.Fatalf("a foreign prefix found session %d", s.sid)
	}
	if s := tab.get(1); s != nil {
		t.Fatalf("a bare id found session %d", s.sid)
	}
	tab.remove(prefix | 3)
	if tab.get(prefix|3) != nil || tab.len() != 4 {
		t.Fatal("remove did not clear the prefixed slot")
	}
}
