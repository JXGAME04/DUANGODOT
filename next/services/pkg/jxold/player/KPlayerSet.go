// Package player reads the player tables of the old server (settings/npc/player) the way
// KPlayerSet / KLevelAdd of the JX2 server did (jx_linux_y: the loader 0x080C4FD0, the ini
// reader 0x080A0810, the new-character templates of the Bishop's CPlayerCreator) - see
// docs/LINUX-SERVER.md §10.3.  jxassets export-player writes the result as player.json; the zone
// (KPlayerSet.h) and the gateway (a new character's first numbers) read that.
package player

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
)

const (
	MaxLevel   = 200 // rows of level_exp.txt the JX2 server keeps (KLevelAdd: 200; JX1 had 150)
	MaxSeries  = 5
	MaxReborn  = 7             // the 1..7 转 columns of level_exp.txt
	ExpCap     = 2_000_000_000 // 0x77359400: what a bad cell becomes ("level exp error ... please call program")
	NewPlayers = 10            // newplayerini00..09: series * 2 + sex (CPlayerCreator::GetRoleData)
)

// LevelExp is one row of level_exp.txt as KLevelAdd keeps it: the experience the level needs
// (column 2 + 10 000 x column 3) and the same for a character reborn 1..7 times (column 3+k x
// 10 000) - KLevelAdd::GetLevelExp (0x080C3FF0).
type LevelExp struct {
	Exp    int64    `json:"exp"`
	Reborn [7]int64 `json:"reborn"`
}

// LevelAdd is one row of level_add.txt (a series): what a level and an attribute point add,
// the resistance per level (x level / 100, see Resist), the stamina base per sex.
type LevelAdd struct {
	LifePerLevel          int `json:"life_per_level"`
	StaminaMalePerLevel   int `json:"stamina_male_per_level"`
	StaminaFemalePerLevel int `json:"stamina_female_per_level"`
	ManaPerLevel          int `json:"mana_per_level"`
	LifePerVitality       int `json:"life_per_vitality"`
	StaminaPerVitality    int `json:"stamina_per_vitality"`
	ManaPerEnergy         int `json:"mana_per_energy"`
	LeadExpShare          int `json:"lead_exp_share"`
	FireRes               int `json:"fire_res"`
	ColdRes               int `json:"cold_res"`
	PoisonRes             int `json:"poison_res"`
	LightingRes           int `json:"lighting_res"`
	PhysicsRes            int `json:"physics_res"`
	StaminaMaleBase       int `json:"stamina_male_base"`
	StaminaFemaleBase     int `json:"stamina_female_base"`
}

// Stamina is [stamina] of stamina.ini (KPlayerSet+0x14bc..): the natural stamina gain per
// tick, the loss while running in the three modes, the sit regeneration in per-mille of the
// maximum.  The defaults are the ones the server uses when the file is missing (0x080A0DE5).
type Stamina struct {
	NormalAdd      int `json:"normal_add"`
	ExerciseRunSub int `json:"exercise_run_sub"`
	FightRunSub    int `json:"fight_run_sub"`
	KillRunSub     int `json:"kill_run_sub"`
	SitAdd         int `json:"sit_add"`
}

// BaseValue is [Common] of basevalue.ini: the frame counts of a player's actions.
type BaseValue struct {
	HurtFrame   int `json:"hurt_frame"`
	RunSpeed    int `json:"run_speed"`
	WalkSpeed   int `json:"walk_speed"`
	AttackFrame int `json:"attack_frame"`
	CastFrame   int `json:"cast_frame"`
}

// NewPlayerItem is an [ITEMn] block of newplayerini%02d.ini: what a new character starts with.
type NewPlayerItem struct {
	Genre      int `json:"genre"`      // iequipclasscode (KPlayerDBFuns.cpp: nItemClass = iequipclasscode; iequipcode (4) is read by nobody)
	Detail     int `json:"detail"`     // idetailtype
	Particular int `json:"particular"` // iparticulartype
	Level      int `json:"level"`      // ilevel
	Series     int `json:"series"`     // iseries
	Version    int `json:"version"`    // iequipversion
	Room       int `json:"room"`       // ilocal (3 = the bag)
	X          int `json:"x"`
	Y          int `json:"y"`
	Seed       int `json:"seed"` // irandseed
}

// NewPlayerSkill is one Sn/Ln pair of [FSKILLS]: a fight skill a new character knows.
type NewPlayerSkill struct {
	ID    int `json:"id"`
	Level int `json:"level"`
}

// NewPlayer is newplayerini%02d.ini (index = series * 2 + sex, CPlayerCreator::GetRoleData):
// the role data every new character of that series and sex starts from.  Present reports
// whether the file exists (the Linux server lacks 01 and 04: those fall back to the other sex
// of the same series, whose numbers are identical - only the starting weapon differs).
type NewPlayer struct {
	Present        bool             `json:"present"`
	Series         int              `json:"series"`          // ifiveprop
	Strength       int              `json:"strength"`        // ipower
	Dexterity      int              `json:"dexterity"`       // iagility
	Vitality       int              `json:"vitality"`        // iouter
	Energy         int              `json:"energy"`          // iinside
	Lucky          int              `json:"lucky"`           // iluck
	AttributePoint int              `json:"attribute_point"` // ileftprop
	SkillPoint     int              `json:"skill_point"`     // ileftfight
	LifeMax        int              `json:"life_max"`        // imaxlife  (= vitality x LifePerVitality + LifePerLevel)
	ManaMax        int              `json:"mana_max"`        // imaxinner (= energy x ManaPerEnergy + ManaPerLevel)
	StaminaMax     int              `json:"stamina_max"`     // imaxstamina (the server recomputes it: GetStaminaBase)
	Level          int              `json:"level"`           // ifightlevel
	Exp            int64            `json:"exp"`             // ifightexp
	RevivalID      int              `json:"revival_id"`      // irevivalid
	HelmRes        int              `json:"helm_res"`        // ihelmres (appearance)
	ArmorRes       int              `json:"armor_res"`       // iarmorres
	WeaponRes      int              `json:"weapon_res"`      // iweaponres
	Items          []NewPlayerItem  `json:"items"`
	Skills         []NewPlayerSkill `json:"skills"`
}

// Set is everything of settings/npc/player the zone and the gateway need.
type Set struct {
	Source    string                `json:"source"`
	LevelExp  [MaxLevel]LevelExp    `json:"level_exp"` // index = level - 1
	LevelAdd  [MaxSeries]LevelAdd   `json:"level_add"` // index = series
	Stamina   Stamina               `json:"stamina"`
	BaseValue BaseValue             `json:"basevalue"`
	NewPlayer [NewPlayers]NewPlayer `json:"new_player"` // index = series * 2 + sex
	Missing   []string              `json:"missing,omitempty"`
}

// Load reads the folder (settings/npc/player of the old server; file names in any case).
func Load(dir string) (*Set, error) {
	s := &Set{Source: dir}
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil, err
	}
	names := map[string]string{}
	for _, e := range entries {
		names[strings.ToLower(e.Name())] = filepath.Join(dir, e.Name())
	}
	read := func(name string) ([]byte, bool) {
		p, ok := names[strings.ToLower(name)]
		if !ok {
			s.Missing = append(s.Missing, name)
			return nil, false
		}
		data, err := os.ReadFile(p)
		if err != nil {
			s.Missing = append(s.Missing, name)
			return nil, false
		}
		return data, true
	}

	// level_exp.txt: KLevelAdd::Init 0x080C4FD0 - row i+2 is level i+1; column 2 must be 1..2e9
	// (else 2e9 and a log), column 3 <= 2e9, the reborn columns 4..10 are clamped to 0..2e9
	// and multiplied by 10 000 (the header says 万).
	if data, ok := read("level_exp.txt"); ok {
		t := npcres.ParseTab(data)
		for i := 0; i < MaxLevel; i++ {
			row := i + 2
			exp := cell(t, row, 2, ExpCap)
			if exp < 1 || exp > ExpCap {
				exp = ExpCap
			}
			extra := cell(t, row, 3, 0)
			if extra > ExpCap {
				extra = ExpCap
			}
			s.LevelExp[i].Exp = int64(exp) + 10_000*int64(extra)
			for k := 0; k < MaxReborn; k++ {
				v := cell(t, row, 4+k, 0)
				if v < 0 {
					v = 0
				}
				if v > ExpCap {
					v = ExpCap
				}
				s.LevelExp[i].Reborn[k] = int64(exp) + 10_000*int64(v)
			}
		}
	}
	// level_add.txt: 5 rows (series 0..4), columns 2..16 in the order of the loader
	if data, ok := read("level_add.txt"); ok {
		t := npcres.ParseTab(data)
		for i := 0; i < MaxSeries; i++ {
			row := i + 2
			a := &s.LevelAdd[i]
			a.LifePerLevel = cell(t, row, 2, 0)
			a.StaminaMalePerLevel = cell(t, row, 3, 0)
			a.StaminaFemalePerLevel = cell(t, row, 4, 0)
			a.ManaPerLevel = cell(t, row, 5, 0)
			a.LifePerVitality = cell(t, row, 6, 0)
			a.StaminaPerVitality = cell(t, row, 7, 0)
			a.ManaPerEnergy = cell(t, row, 8, 0)
			a.LeadExpShare = cell(t, row, 9, 0)
			a.FireRes = cell(t, row, 10, 0)
			a.ColdRes = cell(t, row, 11, 0)
			a.PoisonRes = cell(t, row, 12, 0)
			a.LightingRes = cell(t, row, 13, 0)
			a.PhysicsRes = cell(t, row, 14, 0)
			a.StaminaMaleBase = cell(t, row, 15, 0)
			a.StaminaFemaleBase = cell(t, row, 16, 0)
		}
	}
	// stamina.ini: KIniFile::GetInteger with the defaults of 0x080A0B4A.. (NormalAdd 1, SitAdd 3,
	// the run costs 6); a missing file keeps 1 / 6 / 6 / 6 (0x080A0DE5) and SitAdd 3
	s.Stamina = Stamina{NormalAdd: 1, ExerciseRunSub: 6, FightRunSub: 6, KillRunSub: 6, SitAdd: 3}
	if data, ok := read("stamina.ini"); ok {
		ini := npcres.ParseIni(data)
		s.Stamina.NormalAdd = iniInt(ini, "stamina", "NormalAdd", 1)
		s.Stamina.SitAdd = iniInt(ini, "stamina", "SitAdd", 3)
		s.Stamina.ExerciseRunSub = iniInt(ini, "stamina", "ExerciseRunSub", 6)
		s.Stamina.FightRunSub = iniInt(ini, "stamina", "FightRunSub", 6)
		s.Stamina.KillRunSub = iniInt(ini, "stamina", "KillRunSub", 6)
	}
	s.BaseValue = BaseValue{HurtFrame: 12, RunSpeed: 10, WalkSpeed: 5, AttackFrame: 18, CastFrame: 18}
	if data, ok := read("basevalue.ini"); ok {
		ini := npcres.ParseIni(data)
		s.BaseValue.HurtFrame = iniInt(ini, "common", "HurtFrame", 12)
		s.BaseValue.RunSpeed = iniInt(ini, "common", "RunSpeed", 10)
		s.BaseValue.WalkSpeed = iniInt(ini, "common", "WalkSpeed", 5)
		s.BaseValue.AttackFrame = iniInt(ini, "common", "AttackFrame", 18)
		s.BaseValue.CastFrame = iniInt(ini, "common", "CastFrame", 18)
	}
	for i := 0; i < NewPlayers; i++ {
		p, ok := names[fmt.Sprintf("newplayerini%02d.ini", i)]
		if !ok {
			continue
		}
		data, err := os.ReadFile(p)
		if err != nil {
			continue
		}
		s.NewPlayer[i] = parseNewPlayer(data)
	}
	return s, nil
}

// NewPlayerFor is CPlayerCreator::GetRoleData: the template of a series and sex (index series*2
// + sex); when that file is missing the other sex of the series serves (same numbers).
func (s *Set) NewPlayerFor(series, sex int) *NewPlayer {
	if series < 0 || series >= MaxSeries {
		return nil
	}
	i := series*2 + (sex & 1)
	if s.NewPlayer[i].Present {
		return &s.NewPlayer[i]
	}
	j := series*2 + ((sex + 1) & 1)
	if s.NewPlayer[j].Present {
		return &s.NewPlayer[j]
	}
	return nil
}

// GetLevelExp is KLevelAdd::GetLevelExp(level, reborn): -1 outside 1..MaxLevel or reborn > 7.
func (s *Set) GetLevelExp(level, reborn int) int64 {
	if level < 1 || level > MaxLevel || reborn < 0 || reborn > MaxReborn {
		return -1
	}
	if reborn == 0 {
		return s.LevelExp[level-1].Exp
	}
	return s.LevelExp[level-1].Reborn[reborn-1]
}

// GetStaminaBase is 0x080C4120: (level - 1) x StaminaPerLevel(sex) + StaminaBase(sex).
func (s *Set) GetStaminaBase(series, sex, level int) int {
	if series < 0 || series >= MaxSeries || level < 1 || level > MaxLevel {
		return 0
	}
	a := &s.LevelAdd[series]
	if sex == 0 {
		return (level-1)*a.StaminaMalePerLevel + a.StaminaMaleBase
	}
	return (level-1)*a.StaminaFemalePerLevel + a.StaminaFemaleBase
}

// Resist is the GetFireResist.. family (0x080C4220 ..): perLevel x n / 100 where n is the level,
// held at 120 above level 120 when the per-level value is negative; a reborn character gets at
// least `floor` (the global at 0x830CA08, 0 on this server).
func Resist(perLevel, level int, reborn bool, floor int) int {
	if level < 1 || level > MaxLevel {
		return 0
	}
	n := level
	if level > 120 && perLevel < 0 {
		n = 120
	}
	res := perLevel * n / 100
	if reborn && res < floor {
		res = floor
	}
	return res
}

// Write stores the set as JSON.
func (s *Set) Write(path string) error {
	blob, err := json.MarshalIndent(s, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}

// Read loads a player.json written by Write.
func Read(path string) (*Set, error) {
	blob, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	s := &Set{}
	if err := json.Unmarshal(blob, s); err != nil {
		return nil, err
	}
	return s, nil
}

func parseNewPlayer(data []byte) NewPlayer {
	ini := npcres.ParseIni(data)
	role := func(key string, def int) int { return iniInt(ini, "role", key, def) }
	p := NewPlayer{
		Present:        true,
		Series:         role("ifiveprop", 0),
		Strength:       role("ipower", 0),
		Dexterity:      role("iagility", 0),
		Vitality:       role("iouter", 0),
		Energy:         role("iinside", 0),
		Lucky:          role("iluck", 0),
		AttributePoint: role("ileftprop", 0),
		SkillPoint:     role("ileftfight", 0),
		LifeMax:        role("imaxlife", 0),
		ManaMax:        role("imaxinner", 0),
		StaminaMax:     role("imaxstamina", 0),
		Level:          role("ifightlevel", 1),
		Exp:            int64(role("ifightexp", 0)),
		RevivalID:      role("irevivalid", 0),
		HelmRes:        role("ihelmres", 0),
		ArmorRes:       role("iarmorres", 0),
		WeaponRes:      role("iweaponres", 0),
	}
	if p.Level < 1 {
		p.Level = 1
	}
	n := iniInt(ini, "items", "count", 0)
	for i := 1; i <= n; i++ {
		sec := fmt.Sprintf("item%d", i)
		if _, ok := ini[sec]; !ok {
			continue
		}
		it := func(key string, def int) int { return iniInt(ini, sec, key, def) }
		p.Items = append(p.Items, NewPlayerItem{
			Genre: it("iequipclasscode", 0), Detail: it("idetailtype", 0), Particular: it("iparticulartype", 0),
			Level: it("ilevel", 1), Series: it("iseries", 0), Version: it("iequipversion", 0),
			Room: it("ilocal", 0), X: it("ix", 0), Y: it("iy", 0), Seed: it("irandseed", 0),
		})
	}
	n = iniInt(ini, "fskills", "count", 0)
	for i := 1; i <= n; i++ {
		id := iniInt(ini, "fskills", fmt.Sprintf("s%d", i), 0)
		if id <= 0 {
			continue
		}
		p.Skills = append(p.Skills, NewPlayerSkill{ID: id, Level: iniInt(ini, "fskills", fmt.Sprintf("l%d", i), 1)})
	}
	return p
}

// cell is KTabFile::GetInteger: an empty or missing cell gives the default (jx_linux_y 0x08227E10).
func cell(t *npcres.TabFile, row, col, def int) int {
	s := strings.TrimSpace(t.Get(row, col))
	if s == "" {
		return def
	}
	n, err := strconv.Atoi(s)
	if err != nil {
		// strtol: the leading number
		end := 0
		if end < len(s) && (s[end] == '-' || s[end] == '+') {
			end++
		}
		for end < len(s) && s[end] >= '0' && s[end] <= '9' {
			end++
		}
		n, err = strconv.Atoi(s[:end])
		if err != nil {
			return def
		}
	}
	return n
}

// iniInt is KIniFile::GetInteger: the value's leading number, the default when the key is missing.
func iniInt(ini map[string]map[string]string, sec, key string, def int) int {
	v, ok := ini[strings.ToLower(sec)][strings.ToLower(key)]
	if !ok || v == "" {
		return def
	}
	end := 0
	if end < len(v) && (v[end] == '-' || v[end] == '+') {
		end++
	}
	for end < len(v) && v[end] >= '0' && v[end] <= '9' {
		end++
	}
	n, err := strconv.Atoi(v[:end])
	if err != nil {
		return def
	}
	return n
}
