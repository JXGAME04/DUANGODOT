package npcres

import "testing"

func TestParseSkillsKeepsTheFirstRowOfAnId(t *testing.T) {
	data := "SkillName\tSkillId\tSkillStyle\tAttackRadius\tIsMelee\tTargetEnemy\tTargetSelf\n" +
		"Cong kich\t53\t0\t75\t1\t1\t0\n" +
		"Tri lieu\t101\t0\t400\t0\t0\t1\n" +
		"Cong kich lai\t53\t0\t999\t0\t1\t0\n" +
		"hong\tabc\t0\t1\t0\t0\t0\n"
	s := ParseSkills([]byte(data))
	if len(s) != 2 {
		t.Fatalf("skills: %d, want 2", len(s))
	}
	if s[53].AttackRadius != 75 || !s[53].IsMelee || !s[53].TargetEnemy || s[53].TargetSelf {
		t.Fatalf("skill 53: %+v", s[53])
	}
	if s[101].AttackRadius != 400 || s[101].IsMelee || !s[101].TargetSelf || s[101].Name != "Tri lieu" {
		t.Fatalf("skill 101: %+v", s[101])
	}
}

func TestAtoiIsTheCAtoi(t *testing.T) {
	cases := map[string]int{"": 0, "12": 12, " 7": 7, "-3x": -3, "1|0": 1, "abc": 0, "+5": 5}
	for in, want := range cases {
		if got := Atoi(in); got != want {
			t.Errorf("Atoi(%q) = %d, want %d", in, got, want)
		}
	}
}

func TestParseTemplatesReadsTheServerSideColumns(t *testing.T) {
	hdr := "Name\tKind\tCamp\tSeries\tNpcResType\tSkill1\tLevel1\tSkill2\tLevel2\tSkill3\tLevel3\tSkill4\tLevel4\tWalkSpeed\tRunSpeed\tAttackSpeed\tCastSpeed\tVisionRadius\tActiveRadius\tAIMode\tAIParam1\tAIParam2\tAIParam3\tAIParam4\tAIParam5\tAIParam6\tAIParam7\tAIParam8\tAIParam9\tAIMaxTime\n"
	row := "Heo rung\t0\t5\t0\tani018\t53\t1|0\t197\t0|10\t\t\t53\t\t6\t7\t18\t20\t400\t700\t4\t80\t60\t20\t0\t0\t20\t0\t0\t0\t36\n"
	ts := ParseTemplates([]byte(hdr + row))
	if len(ts) != 1 {
		t.Fatalf("templates: %d", len(ts))
	}
	tp := ts[0] // id 0 = the first data row
	if tp.Camp != 5 || tp.AIMode != 4 || tp.AIMaxTime != 36 || tp.VisionRadius != 400 || tp.ActiveRadius != 700 {
		t.Fatalf("ai columns: %+v", tp)
	}
	want := [10]int{80, 60, 20, 0, 0, 20, 0, 0, 0, 5}
	if tp.AIParam != want {
		t.Fatalf("ai params %v, want %v (AIParam10 defaults to 5)", tp.AIParam, want)
	}
	if tp.WalkSpeed != 6 || tp.RunSpeed != 7 || tp.AttackFrame != 18 || tp.CastFrame != 20 {
		t.Fatalf("speeds/frames: %+v", tp)
	}
	if tp.Skills[1] != (TemplateSkill{ID: 53, LevelA: 1, LevelB: 0}) {
		t.Fatalf("skill 1: %+v", tp.Skills[1])
	}
	if tp.Skills[2] != (TemplateSkill{ID: 197, LevelA: 0, LevelB: 10}) {
		t.Fatalf("skill 2: %+v", tp.Skills[2])
	}
	if tp.Skills[3].ID != 0 || tp.Skills[4].ID != 0 {
		t.Fatalf("empty skill or level must leave the slot empty (SetNpcSkill needs both): %+v %+v", tp.Skills[3], tp.Skills[4])
	}
}

func TestParseTemplatesKeepsTheLevelScriptAndItsCells(t *testing.T) {
	hdr := "Name\tKind\tLevelScript\tLifeParam\tLifeParam1\tLifeReplenish\tSkill1\tLevel1\tFireResist\n"
	row := "Heo rung\t0\t\\script\\npclevelscript\\Animal.lua\t100\t0.5\t0|0.05\t53\t1|0\t\n"
	ts := ParseTemplates([]byte(hdr + row))
	tp := ts[0]
	if tp.LevelScript != "\\script\\npclevelscript\\animal.lua" {
		t.Fatalf("level script %q (must be lower-cased like the old strlwr)", tp.LevelScript)
	}
	want := map[string]string{"LifeParam": "100", "LifeParam1": "0.5", "LifeReplenish": "0|0.05", "Level1": "1|0"}
	for k, v := range want {
		if tp.Cells[k] != v {
			t.Errorf("cell %s = %q, want %q", k, tp.Cells[k], v)
		}
	}
	if _, ok := tp.Cells["FireResist"]; ok {
		t.Error("an empty cell must not be exported")
	}
}
