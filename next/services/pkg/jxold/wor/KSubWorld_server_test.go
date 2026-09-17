package wor

import (
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/oldgame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// oldServer returns the old server folder (bin/Server: package.ini + pak/maps.pak) or skips.
func oldServer(t *testing.T) string {
	t.Helper()
	dir := oldgame.ServerJX1()
	if dir == "" {
		t.Skip("old server data not found (JX_OLD_SERVER, config/oldgame.local.json or bin/Server)")
	}
	return dir
}

// The server archive's Region_S.dat carries the real npcs (KRegion::LoadServerNpc): the smithy
// region of Phượng Tường holds the blacksmith and the herb girl at absolute scene coordinates.
func TestServerRegionHoldsTheRealNpcs(t *testing.T) {
	dir := oldServer(t)
	set, err := pak.OpenSet(filepath.Join(dir, "package.ini"))
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	mapPath, err := text.UTF8ToGBK(`\maps\西北南区\凤翔`)
	if err != nil {
		t.Fatal(err)
	}
	w, err := LoadWorld(set, string(mapPath))
	if err != nil {
		t.Fatal(err)
	}
	r, err := LoadServerRegion(set, w, 101, 99)
	if err != nil {
		t.Fatal(err)
	}
	if len(r.Npcs) != 2 {
		t.Fatalf("server region 101,99: %d npcs", len(r.Npcs))
	}
	var smith *Npc
	for i := range r.Npcs {
		if r.Npcs[i].TemplateID == 199 {
			smith = &r.Npcs[i]
		}
	}
	if smith == nil {
		t.Fatalf("no blacksmith (template 199) in %+v", r.Npcs)
	}
	if smith.X < 101*RegionWidth || smith.X >= 102*RegionWidth || smith.Y < 99*RegionHeight || smith.Y >= 100*RegionHeight {
		t.Errorf("blacksmith at %d,%d: positions are absolute scene coordinates of the region", smith.X, smith.Y)
	}
	if smith.Kind != 3 || smith.Level != 1 {
		t.Errorf("blacksmith kind %d level %d (kind_dialoger = 3)", smith.Kind, smith.Level)
	}
	if got := text.GBKToUTF8([]byte(smith.Script)); got == "" || got[0] != '\\' {
		t.Errorf("blacksmith script %q", got)
	}
	// a region without a server file is empty, not an error
	empty, err := LoadServerRegion(set, w, 0, 0)
	if err != nil || len(empty.Npcs) != 0 || empty.HasData {
		t.Errorf("missing server region: %v %+v", err, empty)
	}
}
