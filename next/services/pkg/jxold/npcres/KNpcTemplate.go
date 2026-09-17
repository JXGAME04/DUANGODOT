package npcres

import (
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// Template is one row of Settings/npcs.txt (KNpcTemplate of the old client).  The row index
// (header = 0) is the template id the map files (Npc_C.dat) and the zone use.
type Template struct {
	ID          int    `json:"id"`
	Name        string `json:"name"` // UTF-8 (the file stores TCVN3)
	Kind        int    `json:"kind"`
	Camp        int    `json:"camp"`
	Series      int    `json:"series"`
	ResType     string `json:"res"` // row of 人物类型.txt (KNpcResNode name)
	ArmorType   int    `json:"armor"`
	HelmType    int    `json:"helm"`
	WeaponType  int    `json:"weapon"`
	HorseType   int    `json:"horse"`
	RideHorse   bool   `json:"ride"`
	StandFrame  int    `json:"stand_frame"`
	StandFrame1 int    `json:"stand_frame1"`
	DeathFrame  int    `json:"death_frame"`
	WalkFrame   int    `json:"walk_frame"`
	RunFrame    int    `json:"run_frame"`
	HurtFrame   int    `json:"hurt_frame"`
	WalkSpeed   int    `json:"walk_speed"` // scene units per logic frame (KNpc::ServeMove)
	RunSpeed    int    `json:"run_speed"`
	Stature     int    `json:"stature"`
	// combat (KNpcTemplate::Init): the AttackSpeed column is the attack length in frames
	AttackFrame int `json:"attack_frame"`
	CastFrame   int `json:"cast_frame"` // the CastSpeed column: length of a non-melee skill
	HitRecover  int `json:"hit_recover"`
	ReviveFrame int `json:"revive_frame"`
	LifeParam   int `json:"life_param"` // raw *Param columns, placeholders when no level script runs
	MinDamage   int `json:"min_damage"`
	MaxDamage   int `json:"max_damage"`
	Defense     int `json:"defense"`
	// server side of the row (KNpcTemplate.cpp #ifdef _SERVER): what KNpcAI decides with
	AIMode       int              `json:"ai_mode"`       // 1..3 active, 4..6 passive, 0 none
	AIParam      [10]int          `json:"ai_param"`      // AIParam1..10 = m_AiParam[0..9]; AIParam10 defaults to 5
	AIMaxTime    int              `json:"ai_max_time"`   // frames between two decisions (default 25)
	VisionRadius int              `json:"vision_radius"` // default 40
	ActiveRadius int              `json:"active_radius"` // default 30
	Skills       [5]TemplateSkill `json:"skills"`        // slots 1..4 = Skill1..4 / Level1..4 (KSkillList::m_Skills); 0 unused
	// the LevelScript column and the raw cells KNpcTemplate::InitNpcLevelData hands to the script
	LevelScript string            `json:"level_script"`
	Cells       map[string]string `json:"cells"`
}

// LevelCells are the columns InitNpcLevelData reads as raw strings for the level script.
var LevelCells = []string{
	"ExpParam", "ExpParam1", "ExpParam2", "ExpParam3",
	"LifeParam", "LifeParam1", "LifeParam2", "LifeParam3",
	"LifeReplenish",
	"ARParam", "ARParam1", "ARParam2", "ARParam3",
	"DefenseParam", "DefenseParam1", "DefenseParam2", "DefenseParam3",
	"MinDamageParam", "MinDamageParam1", "MinDamageParam2", "MinDamageParam3",
	"MaxDamageParam", "MaxDamageParam1", "MaxDamageParam2", "MaxDamageParam3",
	"FireResist", "ColdResist", "LightResist", "PoisonResist", "PhysicsResist",
	"Level1", "Level2", "Level3", "Level4",
}

// TemplateSkill is one of the Skill1..4 / Level1..4 pairs.  A level cell "a|b" means
// a + b * npc level (GetNpcLevelData -> GetData of the level scripts); both cells must be
// present or KNpcTemplate::InitNpcLevelData leaves the slot empty.
type TemplateSkill struct {
	ID     int     `json:"id"`
	LevelA float64 `json:"level_a"`
	LevelB float64 `json:"level_b"`
}

// ParseTemplates reads npcs.txt.  Numbers are read like KTabFile::GetInteger (C atoi; an
// empty cell gives the old default, e.g. 15 frames).
func ParseTemplates(data []byte) []Template {
	tab := ParseTab(data)
	out := make([]Template, 0, tab.Height())
	num := func(row int, col string, def int) int {
		s := tab.GetByName(row, col)
		if strings.TrimSpace(s) == "" {
			return def
		}
		return Atoi(s)
	}
	for row := 1; row <= tab.Height(); row++ {
		t := Template{ID: row - 1}
		if row == 1 {
			out = append(out, t) // the header keeps id 0 unused, like the old direct indexing
			continue
		}
		t.Name = text.TCVN3ToUTF8([]byte(tab.Get(row, 1)))
		t.Kind = num(row, "Kind", 0)
		t.Camp = num(row, "Camp", 0)
		t.Series = num(row, "Series", 0)
		t.ResType = tab.GetByName(row, "NpcResType")
		t.ArmorType = num(row, "ArmorType", 0)
		t.HelmType = num(row, "HelmType", 0)
		t.WeaponType = num(row, "WeaponType", 0)
		t.HorseType = num(row, "HorseType", 0)
		t.RideHorse = num(row, "RideHorse", 0) != 0
		t.StandFrame = num(row, "StandFrame", 15)
		t.StandFrame1 = num(row, "StandFrame1", 15)
		t.DeathFrame = num(row, "DeathFrame", 15)
		t.WalkFrame = num(row, "WalkFrame", 15)
		t.RunFrame = num(row, "RunFrame", 15)
		t.HurtFrame = num(row, "HurtFrame", 15)
		t.WalkSpeed = num(row, "WalkSpeed", 5)
		t.RunSpeed = num(row, "RunSpeed", 10)
		t.Stature = num(row, "Stature", 0)
		t.AttackFrame = num(row, "AttackSpeed", 20)
		t.CastFrame = num(row, "CastSpeed", 20)
		t.HitRecover = num(row, "HitRecover", 0)
		t.ReviveFrame = num(row, "ReviveFrame", 2400)
		t.LifeParam = num(row, "LifeParam", 1)
		t.MinDamage = num(row, "MinDamageParam", 1)
		t.MaxDamage = num(row, "MaxDamageParam", 3)
		t.Defense = num(row, "DefenseParam", 0)
		t.AIMode = num(row, "AIMode", 0)
		for i := 0; i < 10; i++ {
			def := 0
			if i == 9 {
				def = 5
			}
			t.AIParam[i] = num(row, "AIParam"+strconv.Itoa(i+1), def)
		}
		t.AIMaxTime = num(row, "AIMaxTime", 25)
		t.VisionRadius = num(row, "VisionRadius", 40)
		t.ActiveRadius = num(row, "ActiveRadius", 30)
		for slot := 1; slot <= 4; slot++ {
			id := strings.TrimSpace(tab.GetByName(row, "Skill"+strconv.Itoa(slot)))
			level := strings.TrimSpace(tab.GetByName(row, "Level"+strconv.Itoa(slot)))
			if id == "" || level == "" {
				continue
			}
			a, b := parseLevel(level)
			t.Skills[slot] = TemplateSkill{ID: Atoi(id), LevelA: a, LevelB: b}
		}
		t.LevelScript = strings.ToLower(strings.TrimSpace(tab.GetByName(row, "LevelScript")))
		t.Cells = map[string]string{}
		for _, c := range LevelCells {
			if v := strings.TrimSpace(tab.GetByName(row, c)); v != "" {
				t.Cells[c] = v
			}
		}
		out = append(out, t)
	}
	return out
}

// parseLevel splits "a|b" (GetParam of the level scripts); a lone number is "a|0".
func parseLevel(s string) (float64, float64) {
	parts := strings.SplitN(s, "|", 2)
	a, _ := strconv.ParseFloat(strings.TrimSpace(parts[0]), 64)
	b := 0.0
	if len(parts) == 2 {
		b, _ = strconv.ParseFloat(strings.TrimSpace(parts[1]), 64)
	}
	return a, b
}
