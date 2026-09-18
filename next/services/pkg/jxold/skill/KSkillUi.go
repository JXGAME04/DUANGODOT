// KSkillUi.go - \settings\skillui\skillui.txt of the VLTK 2.0 client: where the skill book shows
// each fight skill.  The loader of gamecl.exe (0x00607860, KTabFile) reads every row as
// (faction id < 11, branch < 3, a 16 byte title, then TEN tiers x 3 slots of skill ids: 低级绝学 /
// 高级绝学 / 入门 / 10级 / 20级 / 30级 / 40级 / 50级 / 镇派 / 60级 - the 120级 / 150级 columns of the
// header are never read, `cmp ebx, 0xa` at 0x00607AF1) into a map skill id -> BRANCH -> {slot, tier}
// (0x00607A22 / 0x00607AC1: the inner key is column 2, the value {ebp = slot, ebx = tier}); the same
// skill twice in the same branch stops the loading right there (0x00607ABF -> the epilogue).  The
// title of (faction, branch) lands in a 16 byte slot at +0x462130 + (faction * 3 + branch) * 16
// (GDI 0x413, 0x00604A60): the label of the branch button of the skill book.
//
// The skill book (KUiSkills, 技能主窗口.ini) owns THREE fight pages (0x004953C0: three KUiFightSkill
// of 0x2c600 bytes), one per branch (0x00494EF0: pages[i].SetBranch(i) -> pad +0x17874), each
// asking GDI 0x414 with skill x 10 + ITS branch for every held skill (0x0049393D) and putting it in
// box [slot * 10 + tier] - ten tier columns 51 px apart, three slot rows 58 px apart (0x00494030 /
// 0x004933A0).  A skill of another branch's row is not on that page; one named by two branches is
// on both.  Skills no branch names go to the common page (0x00493A40).
package skill

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

const (
	SkillUiTiers    = 10 // the tier groups the loader reads (the header has two more: 120级, 150级)
	SkillUiSlots    = 3  // slots per tier ("...1", "...2", "...3")
	SkillUiFactions = 11 // faction ids the loader accepts (< 0xb)
	SkillUiBranches = 3  // branches per faction (< 3)
)

// SkillUiTierNames are the tier groups in header order (the client's own labels come from the
// page layout, [ColTitle] Title_1..Title_10 of 战斗技能分页.ini).
var SkillUiTierNames = []string{"low_secret", "high_secret", "entry", "lv10", "lv20", "lv30", "lv40", "lv50", "faction", "lv60"}

// SkillUiRow is one line: a faction's branch and its skills by tier and slot (0 = empty).
type SkillUiRow struct {
	Faction int                             `json:"faction"`
	Branch  int                             `json:"branch"`
	Title   string                          `json:"title"` // UTF-8 (the file is GBK / TCVN3 mixed); the branch button's label
	Cells   [SkillUiTiers][SkillUiSlots]int `json:"cells"`
}

// SkillUiPlace is where a skill sits on a branch's page: the slot row and the tier column.
type SkillUiPlace struct {
	Slot int `json:"slot"`
	Tier int `json:"tier"`
}

// SkillUiTable is skill_ui.json.
type SkillUiTable struct {
	Source string       `json:"source"`
	Tiers  []string     `json:"tiers"`
	Rows   []SkillUiRow `json:"rows"`
	// skill id -> branch ("0".."2") -> its place: the map of 0x00607860 (GDI 0x414 = skill x 10 + branch)
	Place map[string]map[string]SkillUiPlace `json:"place"`
	// faction id -> the three branch titles ("" = the faction has no such branch): GDI 0x413
	Titles  map[string][SkillUiBranches]string `json:"titles"`
	Skipped int                                `json:"skipped"` // rows with a faction or branch the loader refuses
	// the loader stopped early: a skill named twice in the same branch (0x00607ABF); what follows
	// that cell is not loaded, like the client
	Truncated bool `json:"truncated,omitempty"`
}

// ParseSkillUi reads the tab file the way 0x00607860 does: column 1 faction (< 11), column 2
// branch (< 3), column 3 the title, then 30 skill ids (ten tiers x three slots); an id of 0 leaves
// the cell empty; a skill repeated within one branch ends the loading.
func ParseSkillUi(data []byte) *SkillUiTable {
	tab := npcres.ParseTab(data)
	t := &SkillUiTable{
		Tiers:  append([]string(nil), SkillUiTierNames...),
		Place:  map[string]map[string]SkillUiPlace{},
		Titles: map[string][SkillUiBranches]string{},
	}
	for row := 2; row <= tab.Height(); row++ {
		faction, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 1)))
		branch, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 2)))
		if faction < 0 || faction >= SkillUiFactions || branch < 0 || branch >= SkillUiBranches {
			// 0x00607903 / 0x0060791F: a faction or branch out of range ends the loading
			t.Skipped++
			break
		}
		r := SkillUiRow{Faction: faction, Branch: branch, Title: text.DecodeMixed([]byte(tab.Get(row, 3)))}
		titles := t.Titles[strconv.Itoa(faction)]
		titles[branch] = r.Title
		t.Titles[strconv.Itoa(faction)] = titles
		bkey := strconv.Itoa(branch)
		for tier := 0; tier < SkillUiTiers; tier++ {
			for slot := 0; slot < SkillUiSlots; slot++ {
				id, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 4+tier*SkillUiSlots+slot)))
				if id <= 0 {
					continue
				}
				key := strconv.Itoa(id)
				inner := t.Place[key]
				if inner == nil {
					inner = map[string]SkillUiPlace{}
					t.Place[key] = inner
				}
				if _, dup := inner[bkey]; dup {
					t.Truncated = true
					t.Rows = append(t.Rows, r)
					return t
				}
				r.Cells[tier][slot] = id
				inner[bkey] = SkillUiPlace{Slot: slot, Tier: tier}
			}
		}
		t.Rows = append(t.Rows, r)
	}
	return t
}

// PlaceOf is the lookup of GDI 0x414 (0x00607B40): the slot and tier of a skill on the page of
// `branch`, ok = false when that branch's row does not name it (or branch is not 0..2).
func (t *SkillUiTable) PlaceOf(skillID, branch int) (SkillUiPlace, bool) {
	if branch < 0 || branch >= SkillUiBranches {
		return SkillUiPlace{}, false
	}
	p, ok := t.Place[strconv.Itoa(skillID)][strconv.Itoa(branch)]
	return p, ok
}

// TitleOf is GDI 0x413 (0x00604A60): the title of a faction's branch, "" when there is none.
func (t *SkillUiTable) TitleOf(faction, branch int) string {
	if faction < 0 || faction >= SkillUiFactions || branch < 0 || branch >= SkillUiBranches {
		return ""
	}
	return t.Titles[strconv.Itoa(faction)][branch]
}

// Write stores the table as JSON.
func (t *SkillUiTable) Write(path string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	data, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, append(data, '\n'), 0o644)
}
