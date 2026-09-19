package npcres

import (
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// NpcGoldTemplateFile is \settings\npc\NpcGoldTemplate.txt: the kinds of "gold" (elite) monster of the JX2
// server, loaded by KNpcGoldTemplate::Init (jx_linux_y 0x0809CCC0; the 2.0 client has the same loader at
// gamecl.exe 0x006E35C0 for the count it colours names by).  One row is one kind; a placed monster that
// revives rolls one of them (docs/LINUX-SERVER.md §16.12).
const NpcGoldTemplateFile = `\settings\npc\NpcGoldTemplate.txt`

// MaxNpcGoldTemplate is how many rows either loader keeps (the server's array of 30 x 0xac bytes, the
// client's 30 x 0x7c; the loop stops at line 32).
const MaxNpcGoldTemplate = 30

// NpcGoldTemplate is one row as the server keeps it (0xac bytes; the offsets are those of the record).
// The percent columns multiply the monster's numbers (x / 100), the speed columns are added, the ai columns
// replace the monster's ai while it is gold.  Columns 38..47 (PhysicalDamageBase ...) exist in the file
// but neither loader reads them.
type NpcGoldTemplate struct {
	Name             string  `json:"name"`           // column 1 类型 (TCVN3 -> UTF-8): only checked for "" (the end of the rows); neither binary stores it
	Exp              int     `json:"exp"`            // column 2 -> +0x00 (default 1): m_Experience percent
	Life             int     `json:"life"`           // column 3 -> +0x04 (default 1): life max percent; <= 0 logs "GoldTemplate:%d settings is error! life <= 0" and becomes 1
	LifeReplenish    int     `json:"life_replenish"` // column 4 -> +0x08 (default 1)
	AttackRating     int     `json:"attack_rating"`  // column 5 -> +0x0c (default 1)
	Defense          int     `json:"defense"`        // column 6 -> +0x10 (default 1)
	MinDamage        int     `json:"min_damage"`     // column 7 -> +0x14 (default 1): the minimum of every damage block
	MaxDamage        int     `json:"max_damage"`     // column 8 -> +0x18 (default 1): the maximum of every damage block (and poison damage)
	Treasure         int     `json:"treasure"`       // column 9 -> +0x1c (default 0): replaces m_CurrentTreasure (the drop rolls)
	WalkSpeed        int     `json:"walk_speed"`     // column 10 -> +0x20: added to m_CurrentWalkSpeed
	RunSpeed         int     `json:"run_speed"`      // column 11 -> +0x24: added to m_CurrentRunSpeed
	AttackSpeed      int     `json:"attack_speed"`   // column 12 -> +0x28: added to both attack speeds
	CastSpeed        int     `json:"cast_speed"`     // column 13 -> +0x2c: added to both cast speeds
	SkillName        string  `json:"skill_name"`     // column 14: the SkillName of a skills.txt row (UTF-8 here)
	SkillID          int     `json:"skill_id"`       // -> +0x30: the SkillId of that row (0x080A1D80: the first row whose name is the same, 0 when none)
	SkillLevel       string  `json:"skill_level"`    // column 15 -> +0x34 (32 bytes): a level cell for GetNpcLevelData("Level5", ...) - "60" is 60
	FireResist       int     `json:"fire_resist"`    // column 16 -> +0x54 (default 0): the current fire resist is multiplied by it TWICE (0x0809D969..0x0809D998)
	FireResistMax    int     `json:"fire_resist_max"`
	ColdResist       int     `json:"cold_resist"`
	ColdResistMax    int     `json:"cold_resist_max"`
	LightResist      int     `json:"light_resist"`
	LightResistMax   int     `json:"light_resist_max"`
	PoisonResist     int     `json:"poison_resist"`
	PoisonResistMax  int     `json:"poison_resist_max"`
	PhysicsResist    int     `json:"physics_resist"`
	PhysicsResistMax int     `json:"physics_resist_max"` // column 25 -> +0x78
	AiMode           int     `json:"ai_mode"`            // column 26 -> +0x7c: replaces m_AiMode
	AiParams         [10]int `json:"ai_params"`          // columns 27..36 -> +0x80..+0xa4: replace m_AiParam[0..9]
	AiMaxTime        int     `json:"ai_max_time"`        // column 37 -> +0xa8 (default 100): replaces m_AIMAXTime (a byte in the npc)
}

// NpcGoldSkillLookup resolves column 14 like 0x080A1D80: the SkillId of the first skills.txt row whose SkillName
// has the same length and the same letters (0x08226FA0: strlen, then a case-insensitive compare).  The raw
// (TCVN3) cell is handed over; nil means no resolving (SkillID stays 0).
type NpcGoldSkillLookup func(rawName string) int

// ParseNpcGoldTemplate reads the table like KNpcGoldTemplate::Init 0x0809CCC0: line 2 onwards, at most 30
// rows, the first empty 类型 cell ends the table; an empty number cell gives the column's default.
func ParseNpcGoldTemplate(data []byte, skill NpcGoldSkillLookup) []NpcGoldTemplate {
	tab := ParseTab(data)
	num := func(row, col, def int) int {
		s := tab.Get(row, col)
		if strings.TrimSpace(s) == "" {
			return def
		}
		return Atoi(s)
	}
	out := make([]NpcGoldTemplate, 0, MaxNpcGoldTemplate)
	for row := 2; row <= tab.Height() && len(out) < MaxNpcGoldTemplate; row++ {
		raw := tab.Get(row, 1)
		if raw == "" {
			break // 0x0809D3B9: the first row without a name ends the table
		}
		t := NpcGoldTemplate{
			Name:             text.TCVN3ToUTF8([]byte(raw)),
			Exp:              num(row, 2, 1),
			Life:             num(row, 3, 1),
			LifeReplenish:    num(row, 4, 1),
			AttackRating:     num(row, 5, 1),
			Defense:          num(row, 6, 1),
			MinDamage:        num(row, 7, 1),
			MaxDamage:        num(row, 8, 1),
			Treasure:         num(row, 9, 0),
			WalkSpeed:        num(row, 10, 0),
			RunSpeed:         num(row, 11, 0),
			AttackSpeed:      num(row, 12, 0),
			CastSpeed:        num(row, 13, 0),
			SkillLevel:       tab.Get(row, 15),
			FireResist:       num(row, 16, 0),
			FireResistMax:    num(row, 17, 0),
			ColdResist:       num(row, 18, 0),
			ColdResistMax:    num(row, 19, 0),
			LightResist:      num(row, 20, 0),
			LightResistMax:   num(row, 21, 0),
			PoisonResist:     num(row, 22, 0),
			PoisonResistMax:  num(row, 23, 0),
			PhysicsResist:    num(row, 24, 0),
			PhysicsResistMax: num(row, 25, 0),
			AiMode:           num(row, 26, 0),
			AiMaxTime:        num(row, 37, 100),
		}
		if t.Life <= 0 {
			t.Life = 1 // 0x0809D432: the error line, then 1
		}
		if rawSkill := tab.Get(row, 14); rawSkill != "" {
			t.SkillName = text.TCVN3ToUTF8([]byte(rawSkill))
			if skill != nil {
				t.SkillID = skill(rawSkill)
			}
		}
		for i := 0; i < 10; i++ {
			t.AiParams[i] = num(row, 27+i, 0)
		}
		out = append(out, t)
	}
	return out
}

// SkillNameLookup builds the NpcGoldSkillLookup of a skills.txt: the first row of a name wins (0x080A1D80 walks
// the rows from line 2 and stops at the first hit).
func SkillNameLookup(skillsTxt []byte) NpcGoldSkillLookup {
	tab := ParseTab(skillsTxt)
	nameCol, idCol := tab.FindColumn("SkillName"), tab.FindColumn("SkillId")
	if nameCol < 0 || idCol < 0 {
		return nil
	}
	return func(raw string) int {
		want := strings.TrimSpace(raw)
		if want == "" {
			return 0
		}
		for row := 2; row <= tab.Height(); row++ {
			if strings.EqualFold(tab.Get(row, nameCol), want) {
				return Atoi(tab.Get(row, idCol))
			}
		}
		return 0
	}
}
