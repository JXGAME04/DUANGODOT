package player

import (
	"os"
	"path/filepath"
	"testing"
)

func TestRevivePosFollowsTheIni(t *testing.T) {
	// three sections of the Linux server's file (trailing blanks and CRLF included), one non-map section
	ini := "[Common]\r\nx=1\r\n[1]\r\nregion=0,4\r\n0=51104,102592\r\n1=52448,104704\r\n[20]\r\nregion=10,12\r\n10=113472,199232\r\n11=110656,197888  \r\n12=108064,200512\r\n[153]\r\nregion=59,60\r\n59=52256,103520\r\n60=52256,103520\r\n"
	r := ParseRevivePos([]byte(ini))
	if len(r.Maps) != 3 {
		t.Fatalf("maps %v", r.MapIDs())
	}
	if x, y, ok := r.Point(20, 10); !ok || x != 113472 || y != 199232 {
		t.Fatalf("point 20/10 = %d,%d %v", x, y, ok)
	}
	if x, y, ok := r.Point(20, 11); !ok || x != 110656 || y != 197888 {
		t.Fatalf("point 20/11 = %d,%d %v", x, y, ok)
	}
	if _, _, ok := r.Point(20, 13); ok {
		t.Fatal("13 is not a point of map 20")
	}
	if _, _, ok := r.Point(99, 43); ok {
		t.Fatal("map 99 is not in this table")
	}
	if lo, hi, ok := r.Region(20); !ok || lo != 10 || hi != 12 {
		t.Fatalf("region 20 = %d..%d %v", lo, hi, ok)
	}
	if _, _, ok := r.Region(5); ok {
		t.Fatal("no region for a missing map")
	}
	// the JSON round trip the zone and the gateway read
	p := filepath.Join(t.TempDir(), "revive_pos.json")
	if err := r.Write(p); err != nil {
		t.Fatal(err)
	}
	back, err := ReadRevivePos(p)
	if err != nil {
		t.Fatal(err)
	}
	if x, y, ok := back.Point(153, 60); !ok || x != 52256 || y != 103520 {
		t.Fatalf("round trip 153/60 = %d,%d %v", x, y, ok)
	}
	if _, err := os.Stat(p); err != nil {
		t.Fatal(err)
	}
	var none *RevivePos
	if _, _, ok := none.Point(1, 0); ok {
		t.Fatal("a nil table has no points")
	}
}
