// KSkillUi.go - \settings\skillui\skillui.txt of the VLTK 2.0 client: where the skill book shows
// each fight skill.  The loader of gamecl.exe (0x00607860, KTabFile) reads every row as
// (faction id < 11, branch < 3, title, then 12 tiers x 3 slots of skill ids: 低级绝学 / 高级绝学 /
// 入门 / 10级 / 20级 / 30级 / 40级 / 50级 / 镇派 / 60级 / 120级 / 150级) into a map skill id ->
// slot -> tier; the fight page (KUiFightSkill, 战斗技能分页.ini) asks GDI 0x414 with skill x 10 +
// slot for every held skill (0x004938E0) and puts it in box [slot * 10 + tier] - ten tier columns
// 51 px apart, three slot rows 58 px apart (0x00494030 / 0x004933A0).  Tiers 120 / 150 lie beyond
// the ten columns the 1024 layout draws.
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
	SkillUiTiers    = 12 // the tier groups of the header
	SkillUiSlots    = 3  // slots per tier ("...1", "...2", "...3")
	SkillUiFactions = 11 // faction ids the loader accepts (< 0xb)
	SkillUiBranches = 3  // branches per faction (< 3)
)

// SkillUiTierNames are the tier groups in header order (the client's own labels come from the
// page layout, [ColTitle] Title_1..Title_12 of 战斗技能分页.ini).
var SkillUiTierNames = []string{"low_secret", "high_secret", "entry", "lv10", "lv20", "lv30", "lv40", "lv50", "faction", "lv60", "lv120", "lv150"}

// SkillUiRow is one line: a faction's branch and its skills by tier and slot (0 = empty).
type SkillUiRow struct {
	Faction int                             `json:"faction"`
	Branch  int                             `json:"branch"`
	Title   string                          `json:"title"` // UTF-8 (the file is GBK / TCVN3 mixed)
	Cells   [SkillUiTiers][SkillUiSlots]int `json:"cells"`
}

// SkillUiPlace is where a skill sits: the tier column and the slot row.
type SkillUiPlace struct {
	Tier int `json:"tier"`
	Slot int `json:"slot"`
}

// SkillUiTable is skill_ui.json.
type SkillUiTable struct {
	Source  string                  `json:"source"`
	Tiers   []string                `json:"tiers"`
	Rows    []SkillUiRow            `json:"rows"`
	Place   map[string]SkillUiPlace `json:"place"`   // skill id -> its place (the first row that names it)
	Skipped int                     `json:"skipped"` // rows with a faction or branch the loader refuses
}

// ParseSkillUi reads the tab file the way 0x00607860 does: column 1 faction (< 11), column 2
// branch (< 3), column 3 the title, then 36 skill ids; an id of 0 leaves the cell empty.
func ParseSkillUi(data []byte) *SkillUiTable {
	tab := npcres.ParseTab(data)
	t := &SkillUiTable{Tiers: append([]string(nil), SkillUiTierNames...), Place: map[string]SkillUiPlace{}}
	for row := 2; row <= tab.Height(); row++ {
		faction, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 1)))
		branch, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 2)))
		if faction < 0 || faction >= SkillUiFactions || branch < 0 || branch >= SkillUiBranches {
			t.Skipped++
			continue
		}
		r := SkillUiRow{Faction: faction, Branch: branch, Title: text.DecodeMixed([]byte(tab.Get(row, 3)))}
		for tier := 0; tier < SkillUiTiers; tier++ {
			for slot := 0; slot < SkillUiSlots; slot++ {
				id, _ := strconv.Atoi(strings.TrimSpace(tab.Get(row, 4+tier*SkillUiSlots+slot)))
				if id <= 0 {
					continue
				}
				r.Cells[tier][slot] = id
				key := strconv.Itoa(id)
				if _, done := t.Place[key]; !done {
					t.Place[key] = SkillUiPlace{Tier: tier, Slot: slot}
				}
			}
		}
		t.Rows = append(t.Rows, r)
	}
	return t
}

// PlaceOf is the lookup of GDI 0x414: the tier and slot of a skill, ok = false when no row names it.
func (t *SkillUiTable) PlaceOf(skillID int) (SkillUiPlace, bool) {
	p, ok := t.Place[strconv.Itoa(skillID)]
	return p, ok
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
