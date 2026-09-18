package player

import (
	"bytes"
	"encoding/json"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// Faction is \settings\faction\门派设定.ini of the JX2 server (KFaction / g_Faction of the old
// core, Core/Src/KFaction.h): the eleven factions the way KFactionSet::Init 0x08060C70 of
// jx_linux_y reads them - sections "%d" for 0..10, keys Name (the code name the scripts pass to
// SetFaction, 64 bytes), ShowName (128 bytes), Series (S_GOLD S_WOOD S_WATER S_FIRE S_EARTH ->
// 0..4; anything else keeps 0), Camp (C_BEGIN C_JUSTICE C_EVIL C_BALANCE C_FREE C_ANIMAL C_EVENT ->
// 0..6; anything else keeps 1).  A section that is missing leaves the entry at its defaults
// (index i, series 0, camp 1, empty names).  The [Name] section (New / Old) is what the string
// table calls G_FACTION_NEW / G_FACTION_OLD: the name KPlayerFaction gives a character that never
// joined / that left (0x080C2680; the string table file itself is not in the data at hand).
//
// Skills lists \settings\faction\factionskill.txt (FactionId, SkillId): the flat list the
// server's script/global/factionskill.lua loads into G_FactionSkill - every skill a faction hands
// out over the levels (AddMagic at level 0 in faction_def.lua AddFacSkill).
type Faction struct {
	Source   string                     `json:"source"`
	Factions [FactionCount]FactionEntry `json:"factions"`
	NewName  string                     `json:"new_name"` // [Name] New: a character that never joined
	OldName  string                     `json:"old_name"` // [Name] Old: a character that left
	Skills   map[string][]int           `json:"skills"`   // faction id -> skill ids (factionskill.txt), may be empty
}

// FactionEntry is one faction: index, series, camp, the two names.
type FactionEntry struct {
	Index    int    `json:"index"`
	Series   int    `json:"series"`
	Camp     int    `json:"camp"`
	Name     string `json:"name"`      // code name ("shaolin"): what the scripts and the record compare
	ShowName string `json:"show_name"` // UTF-8 (the file is TCVN3 / cp1258 mixed)
}

const (
	FactionCount       = 11 // 0x08060CF0: eleven entries of 0xcc bytes
	FactionSeriesCount = 5
	FactionCampCount   = 7
	FactionDefaultCamp = 1 // C_JUSTICE: 0x08060CFC
)

var (
	factionSeriesNames = []string{"S_GOLD", "S_WOOD", "S_WATER", "S_FIRE", "S_EARTH"}
	factionCampNames   = []string{"C_BEGIN", "C_JUSTICE", "C_EVIL", "C_BALANCE", "C_FREE", "C_ANIMAL", "C_EVENT"}
)

// ParseFaction reads the ini text.  Keys are matched without case like KIniFile; a value's
// trailing spaces are kept (the scripts compare "Mới nhập giang hồ " with its space).
func ParseFaction(data []byte) *Faction {
	f := &Faction{Skills: map[string][]int{}}
	for i := range f.Factions {
		f.Factions[i] = FactionEntry{Index: i, Camp: FactionDefaultCamp}
	}
	data = bytes.TrimPrefix(data, []byte{0xEF, 0xBB, 0xBF})
	section := ""
	for _, raw := range strings.Split(string(data), "\n") {
		line := strings.TrimRight(raw, "\r")
		trimmed := strings.TrimSpace(line)
		if trimmed == "" || trimmed[0] == ';' || trimmed[0] == '/' || trimmed[0] == '#' {
			continue
		}
		if trimmed[0] == '[' {
			end := strings.IndexByte(trimmed, ']')
			if end < 0 {
				section = ""
				continue
			}
			section = strings.ToLower(strings.TrimSpace(trimmed[1:end]))
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq < 0 || section == "" {
			continue
		}
		key := strings.ToLower(strings.TrimSpace(line[:eq]))
		value := strings.TrimLeft(line[eq+1:], " \t")
		if section == "name" {
			switch key {
			case "new":
				f.NewName = text.DecodeMixed([]byte(value))
			case "old":
				f.OldName = text.DecodeMixed([]byte(value))
			}
			continue
		}
		idx, err := strconv.Atoi(section)
		if err != nil || idx < 0 || idx >= FactionCount {
			continue
		}
		e := &f.Factions[idx]
		switch key {
		case "name":
			e.Name = strings.TrimSpace(value)
		case "showname":
			e.ShowName = text.DecodeMixed([]byte(value))
		case "series":
			e.Series = indexOfName(factionSeriesNames, strings.TrimSpace(value), e.Series)
		case "camp":
			e.Camp = indexOfName(factionCampNames, strings.TrimSpace(value), e.Camp)
		}
	}
	return f
}

func indexOfName(names []string, value string, def int) int {
	for i, n := range names {
		if n == value {
			return i
		}
	}
	return def
}

// AddSkills reads factionskill.txt (FactionId, SkillId per row) into Skills, in file order.
func (f *Faction) AddSkills(data []byte) {
	tab := npcres.ParseTab(data)
	for row := 2; row <= tab.Height(); row++ {
		fac, err := strconv.Atoi(strings.TrimSpace(tab.Get(row, 1)))
		if err != nil || fac < 0 || fac >= FactionCount {
			continue
		}
		id, err := strconv.Atoi(strings.TrimSpace(tab.Get(row, 2)))
		if err != nil || id <= 0 {
			continue
		}
		key := strconv.Itoa(fac)
		f.Skills[key] = append(f.Skills[key], id)
	}
}

// IDByName is KFactionSet 0x08060C00: the index whose Name matches exactly, -1 for none or an
// empty name (the caller's series above 4 refuses too: series is the player's, checked there).
func (f *Faction) IDByName(name string) int {
	if name == "" {
		return -1
	}
	for i := range f.Factions {
		if f.Factions[i].Name == name {
			return i
		}
	}
	return -1
}

// Write stores the table as JSON.
func (f *Faction) Write(path string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	data, err := json.MarshalIndent(f, "", " ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, append(data, '\n'), 0o644)
}
