package player

import (
	"os"
	"path/filepath"
	"testing"
)

// A small settings/npc/player folder shaped like the Linux server's (docs/LINUX-SERVER.md
// §10.3): the numbers below are the real first rows of D:\ServerLinux\server1.
func writeFixture(t *testing.T) string {
	t.Helper()
	dir := t.TempDir()
	files := map[string]string{
		"level_exp.txt": "level\texp\textra\tr1\tr2\tr3\tr4\tr5\tr6\tr7\n" +
			"1\t100\n2\t500\n3\t1100\t\t\t\n4\t1900\t1\t2\t3\t4\t5\t6\t7\n" +
			"5\t0\n" + // 0 is outside 1..2e9: the loader complains and stores 2e9
			"6\t3000\t3000000000\n", // extra above 2e9: kept as read by the row, clamped here
		"level_add.txt": "series\tLifePerLevel\tStaminaMalePerLevel\tStaminaFemalePerLevel\tManaPerLevel\tLifePerVitality\tStaminaPerVitality\tManaPerEnergy\tLeadExpShare\tfireres\tcoldres\tpoisonres\tlightingres\tphysicres\tStaminaMaleBase\tStaminaFemaleBase\n" +
			"Kim\t4\t9\t8\t1\t8\t0\t1\t25\t-25\t0\t25\t0\t0\t180\t180\n" +
			"Moc\t3\t9\t8\t2\t5\t0\t3\t25\t0\t0\t0\t25\t-15\t180\t180\n" +
			"Thuy\t3\t9\t8\t2\t6\t0\t3\t25\t25\t0\t0\t-25\t0\t180\t180\n" +
			"Hoa\t3\t9\t8\t1\t7\t0\t2\t25\t0\t-25\t0\t0\t15\t180\t180\n" +
			"Tho\t1\t9\t8\t3\t3\t0\t4\t25\t0\t25\t-25\t0\t0\t180\t180\n",
		"stamina.ini":      "[stamina]\n// comment\nNormalAdd=1\nExerciseRunSub=1\nFightRunSub=1\nKillRunSub=18\nSitAdd=10\n",
		"basevalue.ini":    "[Common]\nHurtFrame=12\nRunSpeed=10\nWalkSpeed=5\nAttackFrame=18\nCastFrame=18\n",
		"newplayerini00.ini": "[ROLE]\nifiveprop=0\nbsex=0\nipower=35\niagility=25\niouter=25\niinside=15\niluck=0\nileftprop=0\nileftfight=0\nifightlevel=1\nimaxinner=16\nimaxlife=204\nimaxstamina=100\n" +
			"[FSKILLS]\nCOUNT=2\nS1=53\nL1=1\nS2=1\nL2=1\n[ITEMS]\nCOUNT=1\n[ITEM1]\niequipclasscode=0\nidetailtype=0\niparticulartype=4\nilevel=1\nilocal=3\niseries=0\niequipversion=2\nirandseed=4\n",
		"newplayerini09.ini": "[ROLE]\nifiveprop=4\nbsex=1\nipower=20\niagility=15\niouter=25\niinside=40\nimaxinner=163\nimaxlife=76\n[ITEMS]\nCOUNT=0\n",
	}
	for name, body := range files {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	return dir
}

func TestLoadReadsTheTablesLikeKLevelAdd(t *testing.T) {
	s, err := Load(writeFixture(t))
	if err != nil {
		t.Fatal(err)
	}
	if got := s.GetLevelExp(1, 0); got != 100 {
		t.Fatalf("level 1 exp %d", got)
	}
	if got := s.GetLevelExp(4, 0); got != 1900+10_000 {
		t.Fatalf("level 4 exp with the extra column %d", got)
	}
	if got := s.GetLevelExp(4, 3); got != 1900+10_000*4 { // reborn k reads column 3+k
		t.Fatalf("level 4 reborn 3 exp %d", got)
	}
	if got := s.GetLevelExp(5, 0); got != ExpCap {
		t.Fatalf("a bad cell becomes the cap: %d", got)
	}
	if got := s.GetLevelExp(6, 0); got != 3000+10_000*ExpCap {
		t.Fatalf("an extra above the cap is clamped: %d", got)
	}
	if got := s.GetLevelExp(0, 0); got != -1 {
		t.Fatalf("level 0 %d", got)
	}
	if got := s.GetLevelExp(7, 0); got != ExpCap {
		t.Fatalf("a missing row reads as the cap (empty cell = default 2e9): %d", got)
	}
	if s.LevelAdd[0].LifePerVitality != 8 || s.LevelAdd[4].ManaPerEnergy != 4 || s.LevelAdd[1].PhysicsRes != -15 {
		t.Fatalf("level_add %+v", s.LevelAdd)
	}
	if got := s.GetStaminaBase(0, 0, 1); got != 180 {
		t.Fatalf("stamina base %d", got)
	}
	if got := s.GetStaminaBase(0, 1, 11); got != 180+10*8 {
		t.Fatalf("female stamina at 11 %d", got)
	}
	if s.Stamina.SitAdd != 10 || s.Stamina.KillRunSub != 18 || s.Stamina.NormalAdd != 1 {
		t.Fatalf("stamina %+v", s.Stamina)
	}
	if s.BaseValue.HurtFrame != 12 || s.BaseValue.AttackFrame != 18 {
		t.Fatalf("basevalue %+v", s.BaseValue)
	}
	p := s.NewPlayerFor(0, 0)
	if p == nil || p.Strength != 35 || p.Dexterity != 25 || p.Vitality != 25 || p.Energy != 15 || p.LifeMax != 204 || p.ManaMax != 16 {
		t.Fatalf("Shaolin template %+v", p)
	}
	if len(p.Items) != 1 || p.Items[0].Particular != 4 || p.Items[0].Room != 3 || p.Items[0].Version != 2 {
		t.Fatalf("starting item %+v", p.Items)
	}
	if len(p.Skills) != 2 || p.Skills[0].ID != 53 {
		t.Fatalf("starting skills %+v", p.Skills)
	}
	// the missing 01 falls back to 00 (same numbers), 08 to 09
	if q := s.NewPlayerFor(0, 1); q == nil || q.Strength != 35 {
		t.Fatalf("metal female falls back to 00: %+v", q)
	}
	if q := s.NewPlayerFor(4, 0); q == nil || q.LifeMax != 76 {
		t.Fatalf("earth male falls back to 09: %+v", q)
	}
	if s.NewPlayerFor(2, 0) != nil {
		t.Fatal("water has no template in the fixture")
	}
	// the JSON round trip the gateway and the zone use
	path := filepath.Join(t.TempDir(), "player.json")
	if err := s.Write(path); err != nil {
		t.Fatal(err)
	}
	back, err := Read(path)
	if err != nil {
		t.Fatal(err)
	}
	if back.GetLevelExp(4, 3) != s.GetLevelExp(4, 3) || back.NewPlayer[0].LifeMax != 204 {
		t.Fatal("json round trip")
	}
}

func TestResistFollowsTheLevelClamp(t *testing.T) {
	// 0x080C4220: perLevel x level / 100; above level 120 a NEGATIVE per-level value is held at 120
	if got := Resist(25, 40, false, 0); got != 10 {
		t.Fatalf("25 x 40 / 100 = %d", got)
	}
	if got := Resist(-25, 160, false, 0); got != -30 {
		t.Fatalf("negative above 120 is clamped at 120: %d", got)
	}
	if got := Resist(25, 160, false, 0); got != 40 {
		t.Fatalf("positive is not clamped: %d", got)
	}
	if got := Resist(-25, 40, true, 0); got != 0 {
		t.Fatalf("a reborn character gets at least the floor: %d", got)
	}
	if got := Resist(25, 0, false, 0); got != 0 {
		t.Fatalf("level 0 %d", got)
	}
}
