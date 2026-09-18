package item

import (
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/oldgame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

func writeTab(t *testing.T, dir, name string, rows ...string) {
	t.Helper()
	if err := os.WriteFile(filepath.Join(dir, name), []byte(strings.Join(rows, "\r\n")+"\r\n"), 0o644); err != nil {
		t.Fatal(err)
	}
}

func TestLoadReadsTheColumnsTheOldCoreRead(t *testing.T) {
	dir := t.TempDir()
	name, _ := text.UTF8ToTCVN3("Thiết Trúc")
	intro, _ := text.UTF8ToTCVN3("kiếm nhỏ")
	// MeleeWeapon.txt: the 46 columns of KBPT_Equipment::LoadRecord (13 is skipped by the core)
	head := make([]string, 46)
	for i := range head {
		head[i] = "c"
	}
	row := make([]string, 46)
	copy(row, []string{string(name), "0", "0", "0", `\spr\item\a.spr`, "9", "1", "3", string(intro), "2", "100", "1", "0",
		"28", "4", "4", "29", "7", "7", "", "", "", "31", "20", "20"})
	row[34], row[35] = "36", "0" // req 1: type 36 value 0
	row[36], row[37] = "32", "20"
	writeTab(t, dir, "MeleeWeapon.txt", strings.Join(head, "\t"), strings.Join(row, "\t"))
	writeTab(t, dir, "potion.txt", strings.Repeat("c\t", 19),
		string(name)+"\t1\t0\t0\t\\spr\\p.spr\t18\t1\t1\t"+string(intro)+"\t\t50\t1\t0\t153\t10\t100\t\t\t")
	writeTab(t, dir, "questkey.txt", "名称\tItemGenre\tDetailType\t动画文件名\t对应物件索引\t宽度\t高度\t说明文字\tParticularType",
		string(name)+"\t4\t0\t\\spr\\q.spr\t41\t1\t1\tx\t7")
	writeTab(t, dir, "magicattrib.txt", strings.TrimRight(strings.Repeat("c\t", 23), "\t"),
		string(name)+"\t1\t\t1\t126\t5\t10\t-1\t-1\t6\t6\tabc\t10000\t10000\t0\t0\t0\t0\t0\t0\t0\t0\t3")

	s, err := Load(dir, "")
	if err != nil {
		t.Fatal(err)
	}
	if s.Version != "" {
		t.Fatalf("a plain folder has no version, got %q", s.Version)
	}
	melee := s.Equipment["meleeweapon"]
	if len(melee) != 1 {
		t.Fatalf("melee rows %d", len(melee))
	}
	e := melee[0]
	if e.Row != 1 || e.Name != "Thiết Trúc" || e.Intro != "kiếm nhỏ" || e.Image != `\spr\item\a.spr` || e.ObjIdx != 9 || e.Width != 1 || e.Height != 3 {
		t.Fatalf("equipment head: %+v", e)
	}
	if e.Series != 2 || e.Price != 100 || e.Level != 1 {
		t.Fatalf("equipment numbers: %+v", e)
	}
	if len(e.Basics) != 3 || e.Basics[0] != (Basic{28, Range{4, 4}}) || e.Basics[2] != (Basic{31, Range{20, 20}}) {
		t.Fatalf("basics (empty type skipped): %+v", e.Basics)
	}
	if len(e.Reqs) != 2 || e.Reqs[0] != (Req{36, 0}) || e.Reqs[1] != (Req{32, 20}) {
		t.Fatalf("reqs: %+v", e.Reqs)
	}
	if len(s.Medicine) != 1 || s.Medicine[0].Price != 50 || len(s.Medicine[0].Attribs) != 1 || s.Medicine[0].Attribs[0] != (MedAttrib{153, 10, 100}) {
		t.Fatalf("medicine: %+v", s.Medicine)
	}
	if len(s.Quest) != 1 || s.Quest[0].Particular != 7 || s.Quest[0].ObjIdx != 41 || s.Quest[0].CanSell != 0 {
		t.Fatalf("quest (JX2 header: column 9 is ParticularType): %+v", s.Quest)
	}
	if len(s.Magic) != 1 || s.Magic[0].Pos != 1 || s.Magic[0].Kind != 126 || s.Magic[0].Ranges[0] != (Range{5, 10}) || s.Magic[0].Ranges[1] != (Range{-1, -1}) {
		t.Fatalf("magic: %+v", s.Magic)
	}
	if len(s.Magic[0].DropRates) != 11 || s.Magic[0].DropRates[0] != 10000 || s.Magic[0].DropRates[10] != 3 {
		t.Fatalf("drop rates follow the file's width: %+v", s.Magic[0].DropRates)
	}
	for _, m := range []string{"goldequip", "townportal", "magicattrib_ge", "suite_activate_count", "magicscript", "armor"} {
		found := false
		for _, x := range s.Missing {
			found = found || x == m
		}
		if !found {
			t.Errorf("%s should be reported missing: %v", m, s.Missing)
		}
	}
	if s.Count() != 3 {
		t.Fatalf("count %d", s.Count())
	}
}

// The real tables of the reference server, when the machine has them: every version folder
// loads, the gold table carries the set numbers, names come out as Vietnamese.
func TestReferenceServerTables(t *testing.T) {
	root := oldgame.ServerJX1()
	if root == "" {
		t.Skip("no reference server folder")
	}
	dir := filepath.Join(root, "settings", "item")
	if _, err := os.Stat(dir); err != nil {
		t.Skip("no settings/item")
	}
	base, err := Load(dir, "")
	if err != nil {
		t.Fatal(err)
	}
	if len(base.Equipment["meleeweapon"]) == 0 || len(base.Medicine) == 0 || len(base.Quest) == 0 {
		t.Fatalf("base tables empty: %d melee, %d medicine, %d quest", len(base.Equipment["meleeweapon"]), len(base.Medicine), len(base.Quest))
	}
	for _, e := range base.Equipment["meleeweapon"][:3] {
		if strings.ContainsRune(e.Name, '�') || e.Name == "" {
			t.Errorf("name not decoded: %q", e.Name)
		}
	}
	for _, v := range Versions(dir) {
		s, err := Load(filepath.Join(dir, v), v)
		if err != nil {
			t.Fatal(err)
		}
		if s.Version != v || len(s.Gold) == 0 || len(s.GoldMagic) == 0 || len(s.Suites) == 0 {
			t.Errorf("version %s: gold %d gold magic %d suites %d", v, len(s.Gold), len(s.GoldMagic), len(s.Suites))
		}
		withSet := 0
		for _, g := range s.Gold {
			if len(g.MagicIDs) != 6 {
				t.Errorf("version %s gold row %d: magic ids %v", v, g.Row, g.MagicIDs)
				break
			}
			if g.GroupID != 0 {
				withSet++
			}
		}
		if withSet == 0 {
			t.Errorf("version %s: no gold piece belongs to a set", v)
		}
	}
}
