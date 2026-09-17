package npcres

import (
	"os"
	"path/filepath"
	"reflect"
	"testing"
	"unicode/utf8"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
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

func gbk(t *testing.T, s string) string {
	t.Helper()
	b, err := text.UTF8ToGBK(s)
	if err != nil {
		t.Fatal(err)
	}
	return string(b)
}

// The tables of the shipped client read the way KNpcResList / KNpcResNode / KNpcTemplate did.
func TestNpcResTablesOfTheOldClient(t *testing.T) {
	dir := oldClient(t)
	set, err := pak.OpenSet(filepath.Join(dir, "package.ini"))
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	list, err := Load(set.ReadFile)
	if err != nil {
		t.Fatal(err)
	}
	if len(list.NpcActions) != DoCount || list.NpcActions[DoStand] != "NormalStand1" || list.NpcActions[DoRun] != "NormalRun" || list.NpcActions[DoJump] != "JunpFly" {
		t.Fatalf("npc actions: %v", list.NpcActions)
	}
	if list.ActionNo("FreeStand2") < 0 || list.ActionNo("NormalWalk") < 0 || list.ActionNo("nope") != -1 {
		t.Fatalf("action names: %d %d", list.ActionNo("FreeStand2"), list.ActionNo("NormalWalk"))
	}

	tplData, err := set.ReadFile(gbk(t, TemplateFile))
	if err != nil {
		t.Fatal(err)
	}
	templates := ParseTemplates(tplData)
	if len(templates) < 2000 {
		t.Fatalf("only %d templates", len(templates))
	}
	first := templates[1]
	if first.ResType != "ani001" || first.StandFrame != 28 || first.WalkFrame != 10 || first.Camp != 5 || !utf8.ValidString(first.Name) || first.Name == "" {
		t.Errorf("template 1: %+v", first)
	}

	// a normal npc: one image per doing, shadow "<name>b.spr"
	enemy, err := list.Node("enemy003")
	if err != nil {
		t.Fatal(err)
	}
	if enemy.Special || enemy.PartNum != 1 {
		t.Fatalf("enemy003 special=%v parts=%d", enemy.Special, enemy.PartNum)
	}
	st := enemy.Actions[DoStand]
	if st.File != `\spr\npcres\enemy\enemy003\enemy003_st.spr` || st.Frames != 48 || st.Dirs != 8 || st.Interval != 200 {
		t.Errorf("enemy003 stand: %+v", st)
	}
	if enemy.Shadow[DoStand].File != `\spr\npcres\enemy\enemy003\enemy003_stb.spr` {
		t.Errorf("enemy003 shadow: %q", enemy.Shadow[DoStand].File)
	}
	if _, _, ok := set.Lookup(st.File); !ok {
		t.Errorf("stand sprite missing from the archives: %s", st.File)
	}

	// the main character: parts, doing -> action through the bare-hand row, draw order, shadow
	man, err := list.Node("MainMan")
	if err != nil {
		t.Fatal(err)
	}
	if !man.Special || man.PartNum != 12 || man.Parts[0] == nil || man.Parts[0].Name != "Head" || man.Parts[5] == nil || man.Parts[5].Name != "Body" || man.Parts[16] == nil || man.Parts[16].Name != "Mantle" {
		t.Fatalf("MainMan parts: num=%d", man.PartNum)
	}
	standAct := man.ActNo(DoStand, 0, false)
	if standAct < 0 || list.Actions[standAct] != "FreeStand2" {
		t.Fatalf("bare hand NormalStand1 -> %d", standAct)
	}
	if walk := man.ActNo(DoWalk, 0, false); walk < 0 || list.Actions[walk] != "NormalWalk" {
		t.Errorf("bare hand NormalWalk -> %d", walk)
	}
	if run := man.ActNo(DoRun, 0, false); run < 0 || list.Actions[run] != "NormalRun" {
		t.Errorf("bare hand NormalRun -> %d", run)
	}
	head := man.Parts[0].Equips[0][standAct]
	if head.File != `\spr\npcres\man\MA_HD_001_ST02.spr` || head.Frames != 120 || head.Dirs != 8 || head.Interval != 1 {
		t.Errorf("head equip 0 FreeStand2: %+v", head)
	}
	if _, _, ok := set.Lookup(head.File); !ok {
		t.Errorf("head sprite missing from the archives: %s", head.File)
	}
	if man.Sort == nil || man.Sort.PartNum != 12 {
		t.Fatalf("sort table: %+v", man.Sort)
	}
	if got := man.Sort.Default[0].Parts; !reflect.DeepEqual(got, []int{14, 16, 13, 1, 4, 7, 9, 5, 6, 12, 8, 0}) {
		t.Errorf("[DEFAULT] Dir1 order: %v", got)
	}
	// FreeStand2 carries its own [FreeStand2] block: its Dir1 wins over the default
	if a := man.Sort.Acts[standAct]; a == nil || a.UseDefault {
		t.Errorf("FreeStand2 has no own draw order block: %+v", a)
	} else if got := man.Sort.Sort(standAct, 0, 0); !reflect.DeepEqual(got, a.Dirs[0].Parts) || reflect.DeepEqual(got, man.Sort.Default[0].Parts) {
		t.Errorf("FreeStand2 Dir1 order: %v", got)
	}
	// an action without a block falls back to [DEFAULT]
	noBlock := -1
	for i := range list.Actions {
		if man.Sort.Acts[i] == nil {
			noBlock = i
			break
		}
	}
	if noBlock < 0 || !reflect.DeepEqual(man.Sort.Sort(noBlock, 3, 7), man.Sort.Default[3].Parts) {
		t.Errorf("default fallback for action %d", noBlock)
	}
	if sh := man.Shadow[standAct]; sh.File != `\spr\npcres\man\MA_YY_999_ST02.spr` || sh.Frames != 120 || sh.Dirs != 8 {
		t.Errorf("shadow FreeStand2: %+v", sh)
	}
	if ShadowName(`\a\b.spr`) != `\a\bb.spr` || ComposePathAndName(`spr\npcres\man`, "x.spr") != `\spr\npcres\man\x.spr` {
		t.Error("shadow / path helpers")
	}
	base, err := set.ReadFile(gbk(t, PlayerBaseFile))
	if err != nil {
		t.Fatal(err)
	}
	pf := ParsePlayerBase(base)
	if pf["male"].StandFrame != 25 || pf["female"].StandFrame != 45 || pf["male"].WalkFrame != 12 || pf["male"].RunFrame != 14 {
		t.Errorf("player base frames: %+v", pf)
	}
}
