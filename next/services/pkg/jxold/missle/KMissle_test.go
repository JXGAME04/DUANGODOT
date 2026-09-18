package missle

import (
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

func TestParseKeepsTheRowsTheLoaderKeeps(t *testing.T) {
	name, err := text.UTF8ToGBK("长兵物理攻击")
	if err != nil {
		t.Fatal(err)
	}
	hdr := "MissleId\tMissleName\tMoveKind\tMissleHeight\tSpeed\tLifeTime\tDmgInterval\tAnimFile1\n"
	rows := "64\t" + string(name) + "\t1\t10\t20\t6\t6\t\\spr\\a.spr\n" +
		"65\tB\t1\t10\t16\t20\t\t\n" + // empty cells are not exported (the zone applies 0)
		"64\tC\t0\t\t\t\t\t\n" + // the same id again: the last row wins (0x0805D28F)
		"0\tX\t\t\t\t\t\t\n" + // id 0: skipped
		"1000\tY\t\t\t\t\t\t\n" // above 999: skipped
	tb := Parse([]byte(hdr + rows))
	if len(tb.Columns) != 8 || tb.Columns[7] != "AnimFile1" {
		t.Fatalf("columns: %v", tb.Columns)
	}
	if len(tb.Rows) != 3 || tb.Skipped != 2 {
		t.Fatalf("rows %d skipped %d, want 3 / 2", len(tb.Rows), tb.Skipped)
	}
	r64 := tb.Rows[0]
	if r64.Row != 2 || r64.ID != 64 || r64.Cells["MissleName"] != "长兵物理攻击" {
		t.Fatalf("row 64: %+v", r64)
	}
	if r64.Cells["MissleHeight"] != "10" || r64.Cells["DmgInterval"] != "6" || r64.Cells["AnimFile1"] != `\spr\a.spr` {
		t.Errorf("cells: %v", r64.Cells)
	}
	if _, ok := tb.Rows[1].Cells["DmgInterval"]; ok {
		t.Error("an empty cell must not be exported")
	}
	if last := tb.Template(64); last == nil || last.Row != 4 || last.Cells["MissleName"] != "C" {
		t.Fatalf("the last row of an id must win: %+v", last)
	}
	if tb.Template(0) != nil || tb.Template(1000) != nil {
		t.Error("skipped rows must not be found")
	}
}
