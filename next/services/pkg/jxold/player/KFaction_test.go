package player

import (
	"path/filepath"
	"testing"
)

func TestFactionFollowsTheLinuxLoader(t *testing.T) {
	ini := "[Name]\r\nNew=Moi nhap giang ho \r\nOld=giang ho hiep khach\r\n\r\n;少林派 金 中派\r\n[0]\r\nName=shaolin\r\nShowName=Thieu Lam phai\r\nSeries=S_GOLD\r\nCamp=C_JUSTICE\r\n\r\n[2]\r\nName=tangmen\r\nShowName=Duong Mon\r\nSeries=S_WOOD\r\nCamp=C_BALANCE\r\n\r\n[3]\r\nName=wudu\r\nShowName=Ngu Doc Giao\r\nSeries=S_WOOD\r\nCamp=C_EVIL\r\n\r\n[10]\r\nName=huashan\r\nShowName=Hoa Son phai\r\nSeries=S_NOWHERE\r\nCamp=C_NOWHERE\r\n\r\n[11]\r\nName=beyond\r\n"
	f := ParseFaction([]byte(ini))
	// 0x08060D48..: section "%d", Name / ShowName / Series / Camp
	if e := f.Factions[0]; e.Index != 0 || e.Name != "shaolin" || e.ShowName != "Thieu Lam phai" || e.Series != 0 || e.Camp != 1 {
		t.Fatalf("shaolin %+v", e)
	}
	if e := f.Factions[2]; e.Name != "tangmen" || e.Series != 1 || e.Camp != 3 {
		t.Fatalf("tangmen %+v", e)
	}
	if e := f.Factions[3]; e.Series != 1 || e.Camp != 2 {
		t.Fatalf("wudu %+v", e)
	}
	// an unknown Series keeps 0, an unknown Camp keeps C_JUSTICE (0x08060CF5 / 0x08060CFC)
	if e := f.Factions[10]; e.Name != "huashan" || e.Series != 0 || e.Camp != 1 {
		t.Fatalf("huashan %+v", e)
	}
	// a missing section stays at its defaults; a twelfth section is not an entry
	if e := f.Factions[1]; e.Index != 1 || e.Name != "" || e.Camp != 1 {
		t.Fatalf("entry 1 %+v", e)
	}
	if f.NewName != "Moi nhap giang ho " || f.OldName != "giang ho hiep khach" {
		t.Fatalf("names %q %q", f.NewName, f.OldName)
	}
	// 0x08060C00: exact code name, -1 otherwise
	if f.IDByName("shaolin") != 0 || f.IDByName("wudu") != 3 || f.IDByName("Shaolin") != -1 || f.IDByName("") != -1 || f.IDByName("beyond") != -1 {
		t.Fatal("IDByName")
	}
	f.AddSkills([]byte("FactionId\tSkillId\tszName\tDesc\r\n0\t14\t\t\r\n0\t8\t\t\r\n2\t45\t\t\r\n11\t1\t\t\r\nx\t2\t\t\r\n"))
	if got := f.Skills["0"]; len(got) != 2 || got[0] != 14 || got[1] != 8 {
		t.Fatalf("skills of 0: %v", got)
	}
	if got := f.Skills["2"]; len(got) != 1 || got[0] != 45 {
		t.Fatalf("skills of 2: %v", got)
	}
	if _, bad := f.Skills["11"]; bad {
		t.Fatal("faction 11 is beyond the table")
	}
	if err := f.Write(filepath.Join(t.TempDir(), "faction.json")); err != nil {
		t.Fatal(err)
	}
}
