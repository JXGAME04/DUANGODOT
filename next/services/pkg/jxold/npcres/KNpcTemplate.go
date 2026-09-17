package npcres

import (
	"strconv"

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
	WalkSpeed   int    `json:"walk_speed"`
	RunSpeed    int    `json:"run_speed"`
	Stature     int    `json:"stature"`
	// combat (KNpcTemplate::Init): the AttackSpeed column is the attack length in frames
	AttackFrame int `json:"attack_frame"`
	HitRecover  int `json:"hit_recover"`
	ReviveFrame int `json:"revive_frame"`
	LifeParam   int `json:"life_param"` // raw *Param columns, scaled by the level scripts in the old game
	MinDamage   int `json:"min_damage"`
	MaxDamage   int `json:"max_damage"`
	Defense     int `json:"defense"`
}

// ParseTemplates reads npcs.txt.  Missing numbers fall back to the old defaults
// (KNpcTemplate::Init: 15 frames).
func ParseTemplates(data []byte) []Template {
	tab := ParseTab(data)
	out := make([]Template, 0, tab.Height())
	num := func(row int, col string, def int) int {
		s := tab.GetByName(row, col)
		if s == "" {
			return def
		}
		n, err := strconv.Atoi(s)
		if err != nil {
			return def
		}
		return n
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
		t.HitRecover = num(row, "HitRecover", 0)
		t.ReviveFrame = num(row, "ReviveFrame", 2400)
		t.LifeParam = num(row, "LifeParam", 1)
		t.MinDamage = num(row, "MinDamageParam", 1)
		t.MaxDamage = num(row, "MaxDamageParam", 3)
		t.Defense = num(row, "DefenseParam", 0)
		out = append(out, t)
	}
	return out
}
