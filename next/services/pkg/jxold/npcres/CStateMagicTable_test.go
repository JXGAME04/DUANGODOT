package npcres

import (
	"strings"
	"testing"
)

// The state picture table reads the way the 2.0 client's loader does (gamecl.exe 0x006AE200): by column
// number, ids by row order, the old defaults for empty cells, the split clamped to 1..3.
func TestParseStateMagicTable(t *testing.T) {
	rows := []string{
		"Status\tFileName\ttype\tplay\tbs\tbe\tframes\tdirs\tinterval\tsplit\tnote",
		"Status1\tSpecial\tHead\tLoop\t\t\t8\t1\t50\t1\t" + gbk(t, "眩晕"),
		"Status2\t\\spr\\skill\\a.spr\tFoot\tLoop\t0\t0\t10\t1\t8\t1\t" + gbk(t, "罗汉阵"),
		"Status3\t\\spr\\skill\\b.spr\tBody\t\t4\t12\t20\t8\t36\t9\tshield",
		"Status4\t\\spr\\skill\\c.spr\tMiniMap\tLoop\t\t\t\t\t\t0\tmark",
		"Status5\t\\spr\\skill\\d.spr\tWhatever\tLoop\t0\t0\t13\t1\t12\t1\tbody by default",
	}
	got := ParseStateMagicTable([]byte(strings.Join(rows, "\r\n") + "\r\n"))
	if len(got) != 5 {
		t.Fatalf("rows %d", len(got))
	}
	if !got[0].IsSpecial() || got[0].ID != 1 || got[0].Type != StateMagicHead || !got[0].Loop || got[0].Frames != 8 || got[0].Interval != 50 || got[0].Name != "眩晕" {
		t.Fatalf("row 1: %+v", got[0])
	}
	if got[1].IsSpecial() || got[1].File != `\spr\skill\a.spr` || got[1].Type != StateMagicFoot || got[1].Frames != 10 || got[1].Interval != 8 || got[1].Name != "罗汉阵" {
		t.Fatalf("row 2: %+v", got[1])
	}
	if got[2].Type != StateMagicBody || got[2].Loop || got[2].BackStart != 4 || got[2].BackEnd != 12 || got[2].Frames != 20 || got[2].Dirs != 8 || got[2].Interval != 36 || got[2].Split != 3 {
		t.Fatalf("row 3: %+v", got[2])
	}
	if got[3].Type != StateMagicMiniMap || got[3].Frames != 1 || got[3].Dirs != 1 || got[3].Interval != 1 || got[3].Split != 1 {
		t.Fatalf("row 4 defaults: %+v", got[3])
	}
	if got[4].Type != StateMagicBody || got[4].ID != 5 {
		t.Fatalf("row 5: %+v", got[4])
	}
}
