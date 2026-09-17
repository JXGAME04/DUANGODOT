package export

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// NpcResBundle is npcres/npcs.json: every npc template plus the player frame counts.
type NpcResBundle struct {
	Templates  map[string]TemplateInfo        `json:"templates"`
	Player     map[string]npcres.PlayerFrames `json:"player"` // "male" / "female"
	NpcActions []string                       `json:"npc_actions"`
	Actions    []string                       `json:"actions"`
}

// TemplateInfo is what the client needs from a row of npcs.txt.
type TemplateInfo struct {
	Name        string `json:"name"`
	Res         string `json:"res"`
	Kind        int    `json:"kind"`
	Series      int    `json:"series"`
	StandFrame  int    `json:"stand_frame"`
	StandFrame1 int    `json:"stand_frame1"`
	WalkFrame   int    `json:"walk_frame"`
	RunFrame    int    `json:"run_frame"`
	DeathFrame  int    `json:"death_frame"`
	Helm        int    `json:"helm"`
	Armor       int    `json:"armor"`
	Weapon      int    `json:"weapon"`
	Horse       int    `json:"horse"`
	Ride        bool   `json:"ride"`
	Stature     int    `json:"stature"` // height of the name above the feet (KNpc::GetNpcPate)
	// combat numbers the zone simulates with (KNpc::Load); life/damage are raw params for now
	AttackFrame int `json:"attack_frame"`
	HurtFrame   int `json:"hurt_frame"`
	HitRecover  int `json:"hit_recover"`
	ReviveFrame int `json:"revive_frame"`
	LifeParam   int `json:"life_param"`
	MinDamage   int `json:"min_damage"`
	MaxDamage   int `json:"max_damage"`
	Defense     int `json:"defense"`
}

// ResFile is npcres/res/<name>.json: one KNpcResNode.
type ResFile struct {
	Name    string             `json:"name"`
	Special bool               `json:"special"`
	Actions []ResSprite        `json:"actions,omitempty"` // normal npc: per doing (npc_actions order)
	Parts   []ResPart          `json:"parts,omitempty"`   // main character
	NoHorse [][]int            `json:"no_horse,omitempty"`
	OnHorse [][]int            `json:"on_horse,omitempty"`
	Sort    *ResSort           `json:"sort,omitempty"`
	Shadow  []ResSprite        `json:"shadow,omitempty"` // main character: per action
	Equips  map[string]int     `json:"equips,omitempty"` // exported equipment per part group
}

// ResSprite is a sprite reference: S is the atlas id ("" when not exported yet).
type ResSprite struct {
	S        string `json:"s"`
	File     string `json:"file,omitempty"`
	Frames   int    `json:"frames"`
	Dirs     int    `json:"dirs"`
	Interval int    `json:"interval"`
	Color    uint32 `json:"color,omitempty"`
	Shadow   string `json:"shadow,omitempty"` // normal npc: shadow atlas id
}

// ResPart is one body part with its sprites per equipment number.
type ResPart struct {
	Index  int                    `json:"index"`
	Name   string                 `json:"name"`
	Equips map[string][]ResSprite `json:"equips"` // equip number -> per action
}

// ResSort is the draw-order table (CSortTable) as data.
type ResSort struct {
	PartNum int                   `json:"part_num"`
	Default [][]int               `json:"default"` // 16 dirs -> parts back to front
	Acts    map[string]ResSortAct `json:"acts"`    // action index -> override
}

type ResSortAct struct {
	UseDefault bool    `json:"use_default"`
	Dirs       [][]int `json:"dirs,omitempty"`
	Lines      [][]int `json:"lines,omitempty"` // [frame, parts...]
}

// NpcResOptions selects what to export.
type NpcResOptions struct {
	Names  []string    // resource names (rows of 人物类型.txt)
	Doings []int       // doings whose sprites are exported (npcres.Do*)
	Equips map[int]int // equipment per part group for main characters (0 head, 1 body, 2 weapon, 3 horse, 4 mantle); missing = none
}

// NpcRes writes npcres/npcs.json and npcres/res/<name>.json plus the sprite atlases of the
// selected doings.  Returns the number of resource files written.
func (e *Exporter) NpcRes(list *npcres.List, templates []npcres.Template, player map[string]npcres.PlayerFrames, opt NpcResOptions) (int, error) {
	dir := filepath.Join(e.Out, "npcres")
	if err := os.MkdirAll(filepath.Join(dir, "res"), 0o755); err != nil {
		return 0, err
	}
	if err := os.MkdirAll(filepath.Join(e.Out, "sprites"), 0o755); err != nil {
		return 0, err
	}
	bundle := NpcResBundle{Templates: map[string]TemplateInfo{}, Player: player, NpcActions: list.NpcActions, Actions: list.Actions}
	for _, t := range templates {
		if t.ID == 0 || t.Name == "" {
			continue
		}
		bundle.Templates[strconv.Itoa(t.ID)] = TemplateInfo{Name: t.Name, Res: t.ResType, Kind: t.Kind, Series: t.Series,
			StandFrame: t.StandFrame, StandFrame1: t.StandFrame1, WalkFrame: t.WalkFrame, RunFrame: t.RunFrame, DeathFrame: t.DeathFrame,
			Helm: t.HelmType, Armor: t.ArmorType, Weapon: t.WeaponType, Horse: t.HorseType, Ride: t.RideHorse, Stature: t.Stature,
			AttackFrame: t.AttackFrame, HurtFrame: t.HurtFrame, HitRecover: t.HitRecover, ReviveFrame: t.ReviveFrame,
			LifeParam: t.LifeParam, MinDamage: t.MinDamage, MaxDamage: t.MaxDamage, Defense: t.Defense}
	}
	data, err := json.MarshalIndent(bundle, "", " ")
	if err != nil {
		return 0, err
	}
	if err := os.WriteFile(filepath.Join(dir, "npcs.json"), data, 0o644); err != nil {
		return 0, err
	}
	written := 0
	for _, name := range opt.Names {
		node, err := list.Node(name)
		if err != nil {
			log.Warn("npcres", "resource skipped", log.F("name", name), log.F("error", err))
			continue
		}
		rf := e.resFile(node, opt)
		data, err := json.Marshal(rf)
		if err != nil {
			return written, err
		}
		if err := os.WriteFile(filepath.Join(dir, "res", name+".json"), data, 0o644); err != nil {
			return written, err
		}
		written++
	}
	return written, nil
}

func (e *Exporter) resFile(n *npcres.Node, opt NpcResOptions) *ResFile {
	rf := &ResFile{Name: n.Name, Special: n.Special}
	wanted := map[int]bool{}
	if !n.Special {
		for _, d := range opt.Doings {
			wanted[d] = true
		}
		rf.Actions = make([]ResSprite, len(n.Actions))
		for i, si := range n.Actions {
			rs := ResSprite{File: si.File, Frames: si.Frames, Dirs: si.Dirs, Interval: si.Interval}
			if wanted[i] && si.File != "" {
				rs.S = e.spriteID(si.File)
				if sh := n.Shadow[i].File; sh != "" {
					rs.Shadow = e.spriteIDQuiet(sh)
				}
			}
			rf.Actions[i] = rs
		}
		return rf
	}
	weapon := opt.Equips[2]
	for _, d := range opt.Doings {
		if a := n.ActNo(d, weapon, false); a >= 0 {
			wanted[a] = true
		}
	}
	rf.Equips = map[string]int{}
	for g := 0; g < npcres.MaxBodyPart; g++ {
		if eq, ok := opt.Equips[g]; ok {
			rf.Equips[strconv.Itoa(g)] = eq
		}
	}
	for _, p := range n.Parts {
		if p == nil {
			continue
		}
		rp := ResPart{Index: p.Index, Name: p.Name, Equips: map[string][]ResSprite{}}
		eq, ok := opt.Equips[p.Index/npcres.MaxBodyPartSect]
		if ok && eq >= 0 && eq < len(p.Equips) {
			row := make([]ResSprite, len(p.Equips[eq]))
			for a, si := range p.Equips[eq] {
				rs := ResSprite{File: si.File, Frames: si.Frames, Dirs: si.Dirs, Interval: si.Interval, Color: si.Color}
				if wanted[a] && si.File != "" {
					rs.S = e.spriteID(si.File)
				}
				row[a] = rs
			}
			rp.Equips[strconv.Itoa(eq)] = row
		}
		rf.Parts = append(rf.Parts, rp)
	}
	rf.NoHorse = n.NoHorse
	rf.OnHorse = n.OnHorse
	if n.Sort != nil {
		rs := &ResSort{PartNum: n.Sort.PartNum, Acts: map[string]ResSortAct{}}
		for _, row := range n.Sort.Default {
			rs.Default = append(rs.Default, row.Parts)
		}
		for act, a := range n.Sort.Acts {
			ra := ResSortAct{UseDefault: a.UseDefault}
			for _, row := range a.Dirs {
				ra.Dirs = append(ra.Dirs, row.Parts)
			}
			for _, row := range a.Lines {
				ra.Lines = append(ra.Lines, append([]int{row.Frame}, row.Parts...))
			}
			rs.Acts[strconv.Itoa(act)] = ra
		}
		rf.Sort = rs
	}
	rf.Shadow = make([]ResSprite, len(n.Shadow))
	for a, si := range n.Shadow {
		rs := ResSprite{File: si.File, Frames: si.Frames, Dirs: si.Dirs, Interval: si.Interval}
		if wanted[a] && si.File != "" {
			rs.S = e.spriteIDQuiet(si.File)
		}
		rf.Shadow[a] = rs
	}
	return rf
}

// spriteIDQuiet exports an optional sprite (shadows are often missing) without warnings.
func (e *Exporter) spriteIDQuiet(gamePath string) string {
	if _, _, ok := e.Set.Lookup(gamePath); !ok {
		return ""
	}
	return e.spriteID(gamePath)
}

// ResNamesOf lists the resource names the given templates use (deduplicated, in order).
func ResNamesOf(templates []npcres.Template, ids []int) []string {
	seen := map[string]bool{}
	var out []string
	for _, id := range ids {
		if id <= 0 || id >= len(templates) {
			continue
		}
		res := templates[id].ResType
		if res == "" || seen[res] {
			continue
		}
		seen[res] = true
		out = append(out, res)
	}
	return out
}

// String for logs.
func (o NpcResOptions) String() string {
	return fmt.Sprintf("%d names, doings %v, equips %v", len(o.Names), o.Doings, o.Equips)
}
