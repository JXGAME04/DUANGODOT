package skill

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

func TestParseKeepsTheRowsTheManagerKeeps(t *testing.T) {
	gbkPath, err := text.UTF8ToGBK(`\script\skill\Special\长兵物理攻击.lua`)
	if err != nil {
		t.Fatal(err)
	}
	tcvnName, ok := text.UTF8ToTCVN3("Công kích")
	if !ok {
		t.Fatal("cannot encode the test name as TCVN3")
	}
	hdr := "SkillName\tSkillId\tSkillStyle\tMaxLevel\tAttackRadius\tLvlSetScript\tLvlSetting1\tLvlData1\n"
	rows := string(tcvnName) + "\t1\t0\t\t100\t" + string(gbkPath) + "\tphysicsenhance_p\t\n" + // TCVN3 name, GBK path
		"Cu\t4\t3\t20\t\t\\script\\skill\\shaolin.lua\taddphysicsdamage_p\tshaolin_gunfa\n" +
		"Cu moi\t4\t3\t21\t\t\\script\\skill\\shaolin.lua\taddphysicsdamage_p\tshaolin_gunfa_b\n" + // same id: the last row wins
		"Xau\t0\t0\t\t\t\t\t\n" + // id 0: skipped
		"Xau\t2001\t0\t\t\t\t\t\n" + // above MaxSkill: skipped
		"Am\t9\t-1\t\t\t\t\t\n" + // negative style: skipped
		"Trom\t7\t13\t1\t\t\t\t\n" // a thief skill row is kept (the manager handles it apart)
	tb := Parse([]byte(hdr + rows))
	if len(tb.Columns) != 8 || tb.Columns[5] != "LvlSetScript" {
		t.Fatalf("columns: %v", tb.Columns)
	}
	if len(tb.Rows) != 4 || tb.Skipped != 3 {
		t.Fatalf("rows %d skipped %d, want 4 / 3", len(tb.Rows), tb.Skipped)
	}
	r1 := tb.Info(1)
	if r1 == nil || r1.Row != 2 || r1.Style != 0 || r1.MaxLevel != 0 {
		t.Fatalf("skill 1: %+v", r1)
	}
	if r1.Cells["SkillName"] != "Công kích" {
		t.Errorf("TCVN3 name: %q", r1.Cells["SkillName"])
	}
	if r1.Cells["LvlSetScript"] != `\script\skill\special\长兵物理攻击.lua` {
		t.Errorf("GBK script path lower-cased: %q", r1.Cells["LvlSetScript"])
	}
	if _, ok := r1.Cells["LvlData1"]; ok {
		t.Error("an empty cell must not be exported (the zone applies the default)")
	}
	if r1.Cells["AttackRadius"] != "100" || r1.Cells["LvlSetting1"] != "physicsenhance_p" {
		t.Errorf("cells: %v", r1.Cells)
	}
	r4 := tb.Info(4)
	if r4 == nil || r4.Row != 4 || r4.MaxLevel != 21 || r4.Cells["LvlData1"] != "shaolin_gunfa_b" {
		t.Fatalf("the last row of an id must win (m_SkillInfo is overwritten row by row): %+v", r4)
	}
	if r := tb.Info(7); r == nil || r.Style != ThiefStyle {
		t.Fatalf("thief row: %+v", r)
	}
	if tb.Info(9) != nil || tb.Info(2001) != nil {
		t.Error("skipped rows must not be found")
	}
}

func TestTabIntIsKTabFileGetInteger(t *testing.T) {
	tb := Parse([]byte("SkillName\tSkillId\tSkillStyle\tMaxLevel\nA\t5\t2x\t\n"))
	r := tb.Info(5)
	if r == nil || r.Style != 2 || r.MaxLevel != 0 {
		t.Fatalf("strtol / default: %+v", r)
	}
}

func TestWriteAndRead(t *testing.T) {
	tb := Parse([]byte("SkillName\tSkillId\tSkillStyle\tMaxLevel\nA\t5\t2\t20\n"))
	p := filepath.Join(t.TempDir(), "skills.json")
	if err := tb.Write(p); err != nil {
		t.Fatal(err)
	}
	back, err := Read(p)
	if err != nil {
		t.Fatal(err)
	}
	if len(back.Rows) != 1 || back.Rows[0].ID != 5 || back.Rows[0].Cells["SkillName"] != "A" {
		t.Fatalf("round trip: %+v", back)
	}
	if _, err := os.Stat(p); err != nil {
		t.Fatal(err)
	}
}

func TestParseAttribConstReadsCountAndData(t *testing.T) {
	ini := "; the sections KSkillManager::Init reads\n[ignoreskill_p]\nCount=4\n;state\nData0=724\nData1=15\nData2=67\nData3=64\n\n[staticmagicshield_v]\nCount=1\nData0=721\n\n[empty]\nCount=0\n\n[short]\nCount=3\nData0=5\n"
	d := ParseAttribConst([]byte(ini))
	if got := d["ignoreskill_p"]; len(got) != 4 || got[0] != 724 || got[1] != 15 || got[3] != 64 {
		t.Fatalf("ignoreskill_p: %v", got)
	}
	if got := d["staticmagicshield_v"]; len(got) != 1 || got[0] != 721 {
		t.Fatalf("staticmagicshield_v: %v", got)
	}
	if _, ok := d["empty"]; ok {
		t.Error("a Count of 0 has no data")
	}
	if got := d["short"]; len(got) != 3 || got[0] != 5 || got[1] != 0 || got[2] != 0 {
		t.Fatalf("a missing Data reads as 0 (KIniFile::GetInteger default): %v", got)
	}
}
