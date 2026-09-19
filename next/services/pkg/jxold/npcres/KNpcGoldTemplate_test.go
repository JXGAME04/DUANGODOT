package npcres

import (
	"strings"
	"testing"
)

func TestParseNpcGoldTemplate(t *testing.T) {
	head := strings.Join([]string{"类型", "Exp", "Life", "LifeReplenish", "AttackRating", "Defense", "MinDamage", "MaxDamage", "Treasure",
		"WalkSpeed", "RunSpeed", "AttackSpeed", "CastSpeed", "SkillName", "SkillLevel", "FireResist", "FireResistMax", "ColdResist", "ColdResistMax",
		"LightingResist", "LightingResistMax", "PoisonResist", "PoisonResistMax", "PhycicsResist", "PhycicsResistMax", "AiMode",
		"AiParam1", "AiParam2", "AiParam3", "AiParam4", "AiParam5", "AiParam6", "AiParam7", "AiParam8", "AiParam9", "AiParam10", "AiMaxTime",
		"PhysicalDamageBase", "PhysicalMagicBase"}, "\t")
	rows := []string{
		head,
		// row 1: every number, the skill with a name of the table (trailing blank like the real file)
		"Kim \t100\t1000\t100\t150\t200\t150\t150\t4\t1\t1\t0\t0\tVong tron  mien dich \t60\t75\t95\t75\t95\t75\t95\t75\t95\t75\t95\t2\t80\t25\t15\t100\t25\t25\t25\t20\t50\t\t6\t100\t100",
		// row 2: empty cells -> the defaults of GetInteger; life 0 -> the error line and 1
		"Moc\t\t0\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		// an empty name ends the table, whatever follows
		"\t100\t100",
		"Hoa\t100\t100",
	}
	skills := "SkillId\tSkillName\n1\tx\n539\tVong tron  mien dich \n540\tVong tron  mien dich \n"
	got := ParseNpcGoldTemplate([]byte(strings.Join(rows, "\n")), SkillNameLookup([]byte(skills)))
	if len(got) != 2 {
		t.Fatalf("rows = %d, want 2 (the empty name ends the table)", len(got))
	}
	a := got[0]
	if a.Name != "Kim" || a.Exp != 100 || a.Life != 1000 || a.LifeReplenish != 100 || a.AttackRating != 150 || a.Defense != 200 ||
		a.MinDamage != 150 || a.MaxDamage != 150 || a.Treasure != 4 || a.WalkSpeed != 1 || a.RunSpeed != 1 || a.AttackSpeed != 0 || a.CastSpeed != 0 {
		t.Fatalf("row 1 numbers: %+v", a)
	}
	if a.SkillID != 539 || a.SkillName != "Vong tron  mien dich" || a.SkillLevel != "60" {
		t.Fatalf("row 1 skill: id %d name %q level %q (the first row of the name wins)", a.SkillID, a.SkillName, a.SkillLevel)
	}
	if a.FireResist != 75 || a.FireResistMax != 95 || a.ColdResist != 75 || a.ColdResistMax != 95 || a.LightResist != 75 || a.LightResistMax != 95 ||
		a.PoisonResist != 75 || a.PoisonResistMax != 95 || a.PhysicsResist != 75 || a.PhysicsResistMax != 95 {
		t.Fatalf("row 1 resists: %+v", a)
	}
	if a.AiMode != 2 || a.AiParams != [10]int{80, 25, 15, 100, 25, 25, 25, 20, 50, 0} || a.AiMaxTime != 6 {
		t.Fatalf("row 1 ai: mode %d params %v max %d", a.AiMode, a.AiParams, a.AiMaxTime)
	}
	b := got[1]
	if b.Exp != 1 || b.Life != 1 || b.LifeReplenish != 1 || b.AttackRating != 1 || b.Defense != 1 || b.MinDamage != 1 || b.MaxDamage != 1 {
		t.Fatalf("row 2 percent defaults: %+v", b)
	}
	if b.Treasure != 0 || b.WalkSpeed != 0 || b.SkillID != 0 || b.SkillName != "" || b.FireResist != 0 || b.AiMode != 0 || b.AiMaxTime != 100 {
		t.Fatalf("row 2 zero defaults / AiMaxTime 100: %+v", b)
	}
}

func TestParseNpcGoldTemplateStopsAt30(t *testing.T) {
	var sb strings.Builder
	sb.WriteString("类型\tExp\n")
	for i := 0; i < 40; i++ {
		sb.WriteString("r\t5\n")
	}
	if n := len(ParseNpcGoldTemplate([]byte(sb.String()), nil)); n != MaxNpcGoldTemplate {
		t.Fatalf("rows = %d, want %d", n, MaxNpcGoldTemplate)
	}
}
