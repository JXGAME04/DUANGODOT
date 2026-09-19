package npcres

import "testing"

// AllRows: every row the five tables can select, per part group, without repeats, no "no horse" (-1).
func TestAllRows(t *testing.T) {
	tab := func(rows ...string) *TabFile {
		data := "h1\th2\n"
		for _, r := range rows {
			data += r + "\n"
		}
		return ParseTab([]byte(data))
	}
	r := &ItemChangeRes{
		melee:  tab("1\t2", "2\t3", "3\t3"),
		ranged: tab("2\t21"),
		armor:  tab("1\t20", "2\t2"),
		helm:   tab("1\t20"),
		horse:  tab("1\t0", "2\t10", "3\t10"),
	}
	got := r.AllRows()
	want := map[int][]int{0: {18}, 1: {18, 0}, 2: {0, 1, 19}, 3: {8}}
	for g, rows := range want {
		if len(got[g]) != len(rows) {
			t.Fatalf("group %d: got %v want %v", g, got[g], rows)
		}
		for i := range rows {
			if got[g][i] != rows[i] {
				t.Fatalf("group %d: got %v want %v", g, got[g], rows)
			}
		}
	}
}
