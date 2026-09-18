// KWeaponSkill.go - \settings\武器物理攻击对照表.txt of the JX2 server, read the way the loader at
// 0x0805F18D of jx_linux_y reads it (docs/LINUX-SERVER.md §16): the columns DetailType,
// ParticularType and PhysicsSkillID; a row of DetailType 0 fills melee[particular] (0x0830AF00),
// DetailType 1 ranged[particular] (0x0830B0A0), DetailType -1 the bare-hand skill (0x0830B230).
// A skill outside 1..1999 or a particular outside 0..99 is dropped like the loader drops it.
package skill

import (
	"encoding/json"
	"os"
	"path/filepath"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
)

// WeaponSkillRow is one row kept: the weapon (detail, particular) and its physical skill.
type WeaponSkillRow struct {
	Detail     int `json:"detail"`
	Particular int `json:"particular"`
	Skill      int `json:"skill"`
}

// WeaponSkillTable is weapon_skill.json for the zone's KWeaponSkillTable.
type WeaponSkillTable struct {
	Source  string           `json:"source"`
	Rows    []WeaponSkillRow `json:"rows"`
	Skipped int              `json:"skipped"` // rows the loader would not keep
}

// MaxWeaponParticular is the last ParticularType the loader stores (0x0805F20B: `cmp eax, 0x63`).
const MaxWeaponParticular = 99

// ParseWeaponSkill reads the table's bytes the way 0x0805F1F9..0x0805F2FB walks the rows.
func ParseWeaponSkill(data []byte) *WeaponSkillTable {
	tab := npcres.ParseTab(data)
	t := &WeaponSkillTable{}
	for row := 2; row <= tab.Height(); row++ {
		detail := tabInt(tab, row, "DetailType", 0)
		particular := tabInt(tab, row, "ParticularType", 0)
		skill := tabInt(tab, row, "PhysicsSkillID", 0)
		keep := false
		switch detail {
		case 0, 1:
			keep = particular >= 0 && particular <= MaxWeaponParticular && skill >= 1 && skill <= MaxSkill-1
		case -1:
			keep = skill >= 1 && skill <= MaxSkill-1
		}
		if !keep {
			t.Skipped++
			continue
		}
		t.Rows = append(t.Rows, WeaponSkillRow{Detail: detail, Particular: particular, Skill: skill})
	}
	return t
}

// LoadWeaponSkill reads the file from disk.
func LoadWeaponSkill(path string) (*WeaponSkillTable, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	t := ParseWeaponSkill(data)
	t.Source = path
	return t, nil
}

// SkillOf is KNpc 0x08079A90: the physical skill of a weapon, 0 when the table has none.
func (t *WeaponSkillTable) SkillOf(detail, particular int) int {
	for _, r := range t.Rows {
		if r.Detail == detail && (detail == -1 || r.Particular == particular) {
			return r.Skill
		}
	}
	return 0
}

// Write stores the table as JSON.
func (t *WeaponSkillTable) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}
