package wor

import (
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// oldClient returns the old client folder (bin/Client of the swrod3 checkout) or skips.
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

// The smithy of Phượng Tường (region 101,99): the header counts classify the records the way
// KScenePlaceRegionC::GetBuildinObjs hands them to the sorting tree (point, line, tree, above).
func TestRegionSortKindsFollowTheFileHeader(t *testing.T) {
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
	w, err := LoadWorld(set, string(mapPath))
	if err != nil {
		t.Fatal(err)
	}
	r, err := LoadRegion(set, w, 101, 99)
	if err != nil {
		t.Fatal(err)
	}
	if !r.HasData || len(r.Buildins) != 18 {
		t.Fatalf("region 101,99: hasData=%v buildins=%d", r.HasData, len(r.Buildins))
	}
	if r.NumPoint != 2 || r.NumLine != 14 || r.NumTree != 0 || r.NumAbove != 2 {
		t.Fatalf("counts point=%d line=%d tree=%d above=%d", r.NumPoint, r.NumLine, r.NumTree, r.NumAbove)
	}
	for i, b := range r.Buildins {
		want := KindLine
		switch {
		case i < 2:
			want = KindPoint
		case i >= 16:
			want = KindAbove
		}
		if b.Kind != want {
			t.Errorf("record %d (%s): kind %s, want %s", i, text.GBKToUTF8([]byte(b.Image)), b.Kind, want)
		}
		// the SPBIO_P_SORTMANNER bits agree with the header order
		sortBits := b.Props & 0x0300
		if (want == KindLine && sortBits != 0x0100) || (want == KindPoint && sortBits != 0) {
			t.Errorf("record %d: props %#x disagree with kind %s", i, b.Props, want)
		}
	}
	// the chimney smoke is an animated above-head object drawn from its reference spot oPos1
	smoke := r.Buildins[16]
	if name := text.GBKToUTF8([]byte(smoke.Image)); !strings.HasSuffix(name, `短烟.spr`) {
		t.Fatalf("record 16 is %q", name)
	}
	if smoke.AniSpeed != 1 || smoke.NumFrames != 6 || smoke.OPos[0] != [3]int32{51756, 102131, 201} {
		t.Errorf("smoke record: ani=%d frames=%d opos=%v", smoke.AniSpeed, smoke.NumFrames, smoke.OPos[0])
	}
	// the forge is a static line-sorted object whose four image corners project to a rectangle
	forge := r.Buildins[8]
	if name := text.GBKToUTF8([]byte(forge.Image)); !strings.HasSuffix(name, `fitmenta037_v2.spr`) || forge.Frame != 0 {
		t.Fatalf("record 8 is %q frame %d", name, forge.Frame)
	}
	if forge.AniSpeed != 0 || forge.Pos[0] != [3]int32{51758, 102105, 69} || forge.OPos[1] != [3]int32{51830, 102161, 0} {
		t.Errorf("forge record: ani=%d pos1=%v opos2=%v", forge.AniSpeed, forge.Pos[0], forge.OPos[1])
	}
}
