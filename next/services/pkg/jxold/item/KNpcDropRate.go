package item

// The drop table of an npc (KNpcTemplate::m_pItemDropRate of the old core, KItemDropRate of the
// JX2 server): the file the DropRateFile column of NpcS.txt names, e.g.
// \settings\item\npcdroprate.ini.  The keys and their defaults are the ones the loader of the
// Linux server reads (jx_linux_y 0x080A3B80, KIniFile::GetInteger with its default for every key;
// docs/LINUX-SERVER.md §9):
//
//	[Main]  Count RandRange MagicRate MoneyRate=20 MoneyScale=50 MinItemLevelScale=20
//	        MaxItemLevelScale=10 MaxItemLevel=10 MinItemLevel=1 Series=-1 EnchasableRate=0
//	        MinSocket=1 MaxSocket=1 IsTeamShare=0 TeamShareRate=0
//	[i]     Genre Quality Detail Particular RandRate MinItemLevel=-1 MaxItemLevel=-1 Series=-1
//	        EnchasableRate=-1 MinSocket=-1 MaxSocket=-1 MagicLevel1..6
//
// An entry's -1 means "as [Main] says".  The zone rolls with them the way KNpc::LoseSingleItem /
// GenRandomItem (0x08083BB0) did; see server/zone KNpcDropRate.

import (
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// DropEntry is one [i] section.
type DropEntry struct {
	Genre          int    `json:"genre"`
	Quality        int    `json:"quality"`
	Detail         int    `json:"detail"`
	Particular     int    `json:"particular"`
	RandRate       int    `json:"rate"`
	MinItemLevel   int    `json:"min_level"`
	MaxItemLevel   int    `json:"max_level"`
	Series         int    `json:"series"`
	EnchasableRate int    `json:"enchasable_rate"`
	MinSocket      int    `json:"min_socket"`
	MaxSocket      int    `json:"max_socket"`
	MagicLevel     [6]int `json:"magic_level"`
}

// DropRate is one drop table file.
type DropRate struct {
	Source            string      `json:"source"`
	Count             int         `json:"count"`
	RandRange         int         `json:"rand_range"`
	MagicRate         int         `json:"magic_rate"`
	MoneyRate         int         `json:"money_rate"`
	MoneyScale        int         `json:"money_scale"`
	MinItemLevelScale int         `json:"min_level_scale"`
	MaxItemLevelScale int         `json:"max_level_scale"`
	MinItemLevel      int         `json:"min_level"`
	MaxItemLevel      int         `json:"max_level"`
	Series            int         `json:"series"`
	EnchasableRate    int         `json:"enchasable_rate"`
	MinSocket         int         `json:"min_socket"`
	MaxSocket         int         `json:"max_socket"`
	IsTeamShare       int         `json:"team_share"`
	TeamShareRate     int         `json:"team_share_rate"`
	Entries           []DropEntry `json:"entries"`
}

// ParseDropRate reads a drop table file (GBK / mixed text, "key=value" under "[section]" lines).
func ParseDropRate(source string, data []byte) *DropRate {
	sections := map[string]map[string]string{}
	var order []string
	cur := ""
	for _, raw := range strings.Split(string(data), "\n") {
		line := strings.TrimSpace(strings.TrimRight(raw, "\r"))
		if line == "" || line[0] == ';' || strings.HasPrefix(line, "//") {
			continue
		}
		if line[0] == '[' {
			end := strings.IndexByte(line, ']')
			if end < 0 {
				continue
			}
			cur = strings.ToLower(strings.TrimSpace(line[1:end]))
			if _, ok := sections[cur]; !ok {
				sections[cur] = map[string]string{}
				order = append(order, cur)
			}
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq <= 0 || cur == "" {
			continue
		}
		sections[cur][strings.ToLower(strings.TrimSpace(line[:eq]))] = strings.TrimSpace(line[eq+1:])
	}
	get := func(sec, key string, def int) int {
		v, ok := sections[sec][strings.ToLower(key)]
		if !ok || v == "" {
			return def
		}
		// KIniFile::GetInteger: atoi - the leading number, the rest ignored
		end := 0
		for end < len(v) && (v[end] == '-' || v[end] == '+' || (v[end] >= '0' && v[end] <= '9')) {
			end++
		}
		n, err := strconv.Atoi(v[:end])
		if err != nil {
			return def
		}
		return n
	}
	t := &DropRate{Source: text.GBKToUTF8([]byte(source))}
	t.Count = get("main", "Count", 0)
	t.RandRange = get("main", "RandRange", 0)
	t.MagicRate = get("main", "MagicRate", 0)
	t.MoneyRate = get("main", "MoneyRate", 20)
	t.MoneyScale = get("main", "MoneyScale", 50)
	t.MinItemLevelScale = get("main", "MinItemLevelScale", 20)
	t.MaxItemLevelScale = get("main", "MaxItemLevelScale", 10)
	t.MaxItemLevel = get("main", "MaxItemLevel", 10)
	t.MinItemLevel = get("main", "MinItemLevel", 1)
	t.Series = get("main", "Series", -1)
	t.EnchasableRate = get("main", "EnchasableRate", 0)
	t.MinSocket = get("main", "MinSocket", 1)
	t.MaxSocket = get("main", "MaxSocket", 1)
	t.IsTeamShare = get("main", "IsTeamShare", 0)
	t.TeamShareRate = get("main", "TeamShareRate", 0)
	// the loader reads sections "1".."Count" whether or not they are in the file
	for i := 1; i <= t.Count; i++ {
		sec := strconv.Itoa(i)
		e := DropEntry{
			Genre: get(sec, "Genre", 0), Quality: get(sec, "Quality", 0), Detail: get(sec, "Detail", 0),
			Particular: get(sec, "Particular", 0), RandRate: get(sec, "RandRate", 0),
			MinItemLevel: get(sec, "MinItemLevel", -1), MaxItemLevel: get(sec, "MaxItemLevel", -1),
			Series: get(sec, "Series", -1), EnchasableRate: get(sec, "EnchasableRate", -1),
			MinSocket: get(sec, "MinSocket", -1), MaxSocket: get(sec, "MaxSocket", -1),
		}
		for m := 0; m < 6; m++ {
			e.MagicLevel[m] = get(sec, "MagicLevel"+strconv.Itoa(m+1), 0)
		}
		t.Entries = append(t.Entries, e)
	}
	_ = order
	return t
}
