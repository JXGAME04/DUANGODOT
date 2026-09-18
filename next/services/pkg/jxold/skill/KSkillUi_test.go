package skill

import (
	"path/filepath"
	"strings"
	"testing"
)

func TestSkillUiFollowsTheClientLoader(t *testing.T) {
	head := "门派Id\t派别\tTitle\t低级绝学1\t低级绝学2\t低级绝学3\t高级绝学1\t高级绝学2\t高级绝学3\t入门1\t入门2\t入门3\t10级1\t10级2\t10级3\t20级1\t20级2\t20级3\t30级1\t30级2\t30级3\t40级1\t40级2\t40级3\t50级1\t50级2\t50级3\t镇派1\t镇派2\t镇派3\t60级1\t60级2\t60级3\t120级1\t120级2\t120级3\t150级1\t150级2\t150级3"
	rows := []string{
		"0\t0\tQuyen Phap\t271\t\t\t318\t\t\t14\t\t\t8\t\t\t15\t\t\t16\t\t\t20\t\t\t21\t\t\t273\t\t\t\t\t\t\t\t\t\t\t",
		"2\t0\tPhi Dao\t249\t\t\t339\t\t\t45\t\t\t43\t347\t\t303\t\t\t50\t343\t\t345\t\t\t349\t\t\t48\t\t\t351\t\t\t\t\t\t\t\t",
		"11\t0\tBad faction\t1\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		"0\t3\tBad branch\t2\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
		"0\t1\tBong Phap\t11\t\t\t319\t\t\t10\t\t\t4\t\t\t15\t\t\t16\t\t\t20\t\t\t21\t\t\t273\t\t\t\t\t\t\t\t\t\t\t",
	}
	tab := ParseSkillUi([]byte(head + "\r\n" + strings.Join(rows, "\r\n") + "\r\n"))
	if len(tab.Rows) != 3 || tab.Skipped != 2 {
		t.Fatalf("rows %d skipped %d", len(tab.Rows), tab.Skipped)
	}
	if tab.Rows[0].Faction != 0 || tab.Rows[0].Branch != 0 || tab.Rows[0].Title != "Quyen Phap" {
		t.Fatalf("row 0 %+v", tab.Rows[0])
	}
	// 0x00607860: tier = (column - 4) / 3, slot = (column - 4) % 3
	if p, ok := tab.PlaceOf(271); !ok || p.Tier != 0 || p.Slot != 0 {
		t.Fatalf("271 %+v %v", p, ok)
	}
	if p, ok := tab.PlaceOf(347); !ok || p.Tier != 3 || p.Slot != 1 {
		t.Fatalf("347 (10级2 of Phi Dao) %+v %v", p, ok)
	}
	if p, ok := tab.PlaceOf(351); !ok || p.Tier != 9 || p.Slot != 0 {
		t.Fatalf("351 (60级1) %+v %v", p, ok)
	}
	// a skill named by two branches keeps the first place; an unknown one has none
	if p, ok := tab.PlaceOf(15); !ok || p.Tier != 4 || p.Slot != 0 {
		t.Fatalf("15 %+v %v", p, ok)
	}
	if _, ok := tab.PlaceOf(999); ok {
		t.Fatal("999 is nowhere")
	}
	if tab.Rows[0].Cells[8][0] != 273 || tab.Rows[1].Cells[5][1] != 343 {
		t.Fatalf("cells %v %v", tab.Rows[0].Cells[8], tab.Rows[1].Cells[5])
	}
	p := filepath.Join(t.TempDir(), "skill_ui.json")
	if err := tab.Write(p); err != nil {
		t.Fatal(err)
	}
}
