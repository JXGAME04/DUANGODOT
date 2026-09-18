package skill

import "testing"

func TestParseWeaponSkillKeepsTheRowsTheLoaderKeeps(t *testing.T) {
	hdr := "Name\tDetailType\tParticularType\tPhysicsSkillID\n"
	rows := "Dao\t0\t2\t53\n" + // melee particular 2 -> 53
		"Thuong\t0\t7\t1\n" + // melee particular 7 -> 1
		"Cung\t1\t3\t2\n" + // ranged particular 3 -> 2
		"Tay\t-1\t0\t53\n" + // bare hands
		"Xau\t0\t100\t53\n" + // particular above 99: dropped (0x0805F20B)
		"Xau\t0\t1\t2000\n" + // skill 2000: dropped
		"Xau\t2\t1\t53\n" // another detail: dropped
	tb := ParseWeaponSkill([]byte(hdr + rows))
	if len(tb.Rows) != 4 || tb.Skipped != 3 {
		t.Fatalf("rows %+v skipped %d", tb.Rows, tb.Skipped)
	}
	if tb.SkillOf(0, 2) != 53 || tb.SkillOf(0, 7) != 1 || tb.SkillOf(1, 3) != 2 || tb.SkillOf(-1, 99) != 53 || tb.SkillOf(0, 5) != 0 {
		t.Fatalf("lookup %+v", tb.Rows)
	}
}

func TestParseWeaponSkillSkipsTheBOM(t *testing.T) {
	// \settings\武器物理攻击对照表.txt inside the 2.0 client's archives starts with EF BB BF
	data := append([]byte{0xEF, 0xBB, 0xBF}, []byte("DetailType\tParticularType\tPhysicsSkillID\r\n-1\t0\t53\r\n0\t2\t1\r\n1\t0\t2\r\n")...)
	tb := ParseWeaponSkill(data)
	if len(tb.Rows) != 3 || tb.Rows[0].Detail != -1 || tb.Rows[0].Skill != 53 || tb.Rows[1].Particular != 2 || tb.Rows[2].Skill != 2 {
		t.Fatalf("rows = %+v", tb.Rows)
	}
}
