package export

import (
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/wor"
)

func oldClient(t *testing.T) string {
	t.Helper()
	if dir := os.Getenv("JX_OLD_CLIENT"); dir != "" {
		return dir
	}
	for _, rel := range []string{"../../../../../bin/Client", "../../../../bin/Client"} {
		p, _ := filepath.Abs(rel)
		if _, err := os.Stat(filepath.Join(p, "package.ini")); err == nil {
			return p
		}
	}
	t.Skip("old client data not found (set JX_OLD_CLIENT)")
	return ""
}

// Region 101,99 of Phượng Tường (the smithy) exported the way the old renderer places things:
// static images at the projected ImgPos1 (FRAME_DRAW), animated ones at oPos1 minus the
// sprite centre (REF_SPOT), base lines in scene units for the sorting tree.
func TestRegionObjectsFollowTheOldPlacementRules(t *testing.T) {
	dir := oldClient(t)
	set, err := pak.OpenSet(filepath.Join(dir, "package.ini"))
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	mapPath, err := text.UTF8ToGBK(`\maps\西北南区\凤翔`)
	if err != nil {
		t.Fatal(err)
	}
	w, err := wor.LoadWorld(set, string(mapPath))
	if err != nil {
		t.Fatal(err)
	}
	if w.Left != 72 || w.Top != 82 {
		t.Fatalf("map rect starts at %d,%d", w.Left, w.Top)
	}
	r, err := wor.LoadRegion(set, w, 101, 99)
	if err != nil {
		t.Fatal(err)
	}
	e := New(set, t.TempDir())
	if err := os.MkdirAll(filepath.Join(e.Out, "sprites"), 0o755); err != nil {
		t.Fatal(err)
	}
	rf := e.region(w, r)
	find := func(suffix string, frame int) *Object {
		for gamePath, id := range e.sprites {
			if !strings.HasSuffix(text.GBKToUTF8([]byte(gamePath)), suffix) {
				continue
			}
			for i := range rf.Objects {
				if rf.Objects[i].Sprite == id && rf.Objects[i].Frame == frame {
					return &rf.Objects[i]
				}
			}
		}
		t.Fatalf("no object %s frame %d", suffix, frame)
		return nil
	}
	const baseX, baseY, baseYScene = 72 * 512, 82 * 512, 82 * 1024

	forge := find("fitmenta037_v2.spr", 0)
	if forge.Layer != "object" || forge.Kind != "l" || forge.Frames != 0 {
		t.Errorf("forge: layer=%s kind=%s frames=%d", forge.Layer, forge.Kind, forge.Frames)
	}
	if forge.X != 51758-baseX || forge.Y != 102105/2-(69*887>>10)-baseY {
		t.Errorf("forge at %d,%d", forge.X, forge.Y)
	}
	if len(forge.P1) != 2 || forge.P1[0] != 51758-baseX || forge.P1[1] != 102105-baseYScene ||
		len(forge.P2) != 2 || forge.P2[0] != 51830-baseX || forge.P2[1] != 102161-baseYScene {
		t.Errorf("forge base line %v -> %v", forge.P1, forge.P2)
	}

	smoke := find("短烟.spr", 0)
	if smoke.Layer != "above" || smoke.Frames != 6 || smoke.Z1 != 201 || smoke.Kind != "" {
		t.Errorf("smoke: layer=%s frames=%d z1=%d kind=%q", smoke.Layer, smoke.Frames, smoke.Z1, smoke.Kind)
	}
	// sprite centre (49, 125): the frame offsets are added by the client per frame
	if smoke.X != 51756-49-baseX || smoke.Y != 102131/2-(201*887>>10)-125-baseY {
		t.Errorf("smoke at %d,%d", smoke.X, smoke.Y)
	}
	if len(smoke.P1) != 2 || smoke.P1[1] != 102131-baseYScene {
		t.Errorf("smoke sort key %v", smoke.P1)
	}

	tree := find("西北.spr", 17)
	if tree.Kind != "p" || len(tree.P1) != 2 || tree.P1[0] != 51829-baseX || tree.P1[1] != 101741-baseYScene || len(tree.P2) != 0 {
		t.Errorf("point object: kind=%s p1=%v p2=%v", tree.Kind, tree.P1, tree.P2)
	}
	// file order is kept (the sorting tree is fed region by region in this order)
	if rf.Objects[len(rf.Objects)-1].Layer != "above" || rf.Objects[len(rf.Objects)-2].Layer != "above" {
		t.Errorf("the two above-head objects must be the last records")
	}
}
