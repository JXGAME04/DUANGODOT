package skill

import (
	"path/filepath"
	"strings"
	"testing"
)

const skillUiHead = "门派Id\t派别\tTitle\t低级绝学1\t低级绝学2\t低级绝学3\t高级绝学1\t高级绝学2\t高级绝学3\t入门1\t入门2\t入门3\t10级1\t10级2\t10级3\t20级1\t20级2\t20级3\t30级1\t30级2\t30级3\t40级1\t40级2\t40级3\t50级1\t50级2\t50级3\t镇派1\t镇派2\t镇派3\t60级1\t60级2\t60级3\t120级1\t120级2\t120级3\t150级1\t150级2\t150级3"

func TestSkillUiFollowsTheClientLoader(t *testing.T) {
	rows := []string{
		// the three Shaolin branches of the real file: 271 / 11 / 19 open each, 15 16 20 21 273 are shared
		"0\t0\tQuyen Phap\t271\t\t\t318\t\t\t14\t\t\t8\t\t\t15\t\t\t16\t\t\t20\t\t\t21\t\t\t273\t\t\t\t\t\t\t\t\t\t\t",
		"0\t1\tBong Phap\t11\t\t\t319\t\t\t10\t\t\t4\t\t\t15\t\t\t16\t\t\t20\t\t\t21\t\t\t273\t\t\t\t\t\t\t\t\t\t\t",
		"0\t2\tDao Phap\t19\t\t\t321\t\t\t10\t\t\t6\t\t\t15\t\t\t16\t\t\t20\t\t\t21\t\t\t273\t\t\t\t\t\t\t\t\t\t\t",
		// Tang Men: two skills in one tier (10级1 / 10级2), a 120级 column the loader never reads
		"2\t0\tPhi Dao\t249\t\t\t339\t\t\t45\t\t\t43\t347\t\t303\t\t\t50\t343\t\t345\t\t\t349\t\t\t48\t\t\t351\t\t\t777\t\t\t\t\t",
	}
	tab := ParseSkillUi([]byte(skillUiHead + "\r\n" + strings.Join(rows, "\r\n") + "\r\n"))
	if len(tab.Rows) != 4 || tab.Skipped != 0 || tab.Truncated {
		t.Fatalf("rows %d skipped %d truncated %v", len(tab.Rows), tab.Skipped, tab.Truncated)
	}
	if tab.Rows[0].Faction != 0 || tab.Rows[0].Branch != 0 || tab.Rows[0].Title != "Quyen Phap" {
		t.Fatalf("row 0 %+v", tab.Rows[0])
	}
	// 0x00607860: the inner key is the branch, the value {slot, tier}
	if p, ok := tab.PlaceOf(271, 0); !ok || p.Tier != 0 || p.Slot != 0 {
		t.Fatalf("271 on branch 0 %+v %v", p, ok)
	}
	if _, ok := tab.PlaceOf(271, 1); ok {
		t.Fatal("271 is the fist branch's, not the staff's")
	}
	if p, ok := tab.PlaceOf(11, 1); !ok || p.Tier != 0 || p.Slot != 0 {
		t.Fatalf("11 on branch 1 %+v %v", p, ok)
	}
	// a shared skill sits on every branch page that names it
	for b := 0; b < 3; b++ {
		if p, ok := tab.PlaceOf(15, b); !ok || p.Tier != 4 || p.Slot != 0 {
			t.Fatalf("15 on branch %d %+v %v", b, p, ok)
		}
	}
	if p, ok := tab.PlaceOf(347, 0); !ok || p.Tier != 3 || p.Slot != 1 {
		t.Fatalf("347 (10级2 of Phi Dao) %+v %v", p, ok)
	}
	if p, ok := tab.PlaceOf(351, 0); !ok || p.Tier != 9 || p.Slot != 0 {
		t.Fatalf("351 (60级1) %+v %v", p, ok)
	}
	// the 120级 column is beyond what 0x00607860 reads (ten tiers)
	if _, ok := tab.PlaceOf(777, 0); ok {
		t.Fatal("777 lies in the 120级 column the loader never reads")
	}
	if _, ok := tab.PlaceOf(999, 0); ok {
		t.Fatal("999 is nowhere")
	}
	if _, ok := tab.PlaceOf(15, 3); ok {
		t.Fatal("branch 3 does not exist")
	}
	if tab.Rows[0].Cells[8][0] != 273 || tab.Rows[3].Cells[5][1] != 343 {
		t.Fatalf("cells %v %v", tab.Rows[0].Cells[8], tab.Rows[3].Cells[5])
	}
	// GDI 0x413: the branch titles of a faction, "" for a branch it lacks
	if tab.TitleOf(0, 2) != "Dao Phap" || tab.TitleOf(2, 0) != "Phi Dao" || tab.TitleOf(2, 1) != "" || tab.TitleOf(11, 0) != "" {
		t.Fatalf("titles %v", tab.Titles)
	}
	p := filepath.Join(t.TempDir(), "skill_ui.json")
	if err := tab.Write(p); err != nil {
		t.Fatal(err)
	}
}

func TestSkillUiStopsLikeTheClient(t *testing.T) {
	// 0x00607903: a faction of 11 ends the loading - the rows after it are not read
	rows := []string{
		"0\t0\tQuyen Phap\t271\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		"11\t0\tBad faction\t1\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		"0\t1\tBong Phap\t11\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
	}
	tab := ParseSkillUi([]byte(skillUiHead + "\r\n" + strings.Join(rows, "\r\n") + "\r\n"))
	if len(tab.Rows) != 1 || tab.Skipped != 1 {
		t.Fatalf("rows %d skipped %d", len(tab.Rows), tab.Skipped)
	}
	if _, ok := tab.PlaceOf(11, 1); ok {
		t.Fatal("the row after the bad one is not loaded")
	}
	// 0x00607ABF: the same skill twice within one branch ends the loading at that cell
	rows = []string{
		"0\t0\tQuyen Phap\t271\t\t\t271\t\t\t14\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		"0\t1\tBong Phap\t11\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
	}
	tab = ParseSkillUi([]byte(skillUiHead + "\r\n" + strings.Join(rows, "\r\n") + "\r\n"))
	if !tab.Truncated || len(tab.Rows) != 1 {
		t.Fatalf("truncated %v rows %d", tab.Truncated, len(tab.Rows))
	}
	if p, ok := tab.PlaceOf(271, 0); !ok || p.Tier != 0 {
		t.Fatalf("the first 271 stays %+v %v", p, ok)
	}
	if _, ok := tab.PlaceOf(14, 0); ok {
		t.Fatal("14 comes after the duplicate: not loaded")
	}
}
