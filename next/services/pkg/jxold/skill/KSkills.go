// Package skill reads \settings\skills.txt of the old server the way the JX2 server does
// (jx_linux_y): KSkillManager::Init (0x080E7200) walks every row, keeps the rows whose SkillId
// is 1..2000 and whose SkillStyle is not negative, and stores {row, style, MaxLevel, the row's
// data} in m_SkillInfo[id-1] - so the LAST row of an id wins; KSkill::GetInfoFromTabFile
// (0x080E9200) reads the 60 columns the server uses.  The cells are kept as strings so the zone
// (KSkill.h) reads them with the binary's own defaults (KTabFile::GetInteger: an empty cell is
// the default, otherwise strtol).  jxassets export-skills writes the table as skills.json.
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
	// File is the game path of the table (KSkillManager::Init loads it with KTabFile::Load).
	File = `\settings\skills.txt`
	// MaxSkill is 0x7d0: the size of the JX2 manager's m_SkillInfo (JX1's MAX_SKILL was 2400).
	MaxSkill = 2000
	// ThiefStyle is SKILL_SS_Thief: those rows the manager hands to KThiefSkill and
	// \settings\thiefskill.txt (0x080E6F18); one row on the Linux server.
	ThiefStyle = 13
)

// PathColumns hold GBK game paths (sprites, sounds, the two Lua scripts).
var PathColumns = map[string]bool{
	"SkillIcon": true, "PreCastSpr": true, "ManCastSnd": true, "FMCastSnd": true,
	"LvlSetScript": true, "LevelUpScript": true,
}

// TextColumns hold the Vietnamese text of the table (TCVN3, sometimes mixed with GBK).
var TextColumns = map[string]bool{
	"SkillName": true, "Property": true, "SkillDesc": true, "Param1Memo": true, "Param2Memo": true,
}

// ScriptColumns are lower-cased like the binary does before hashing them (g_FileName2Id of
// the ASCII lower case; 0x080E9DB7 / 0x080E9E30 only touch 'A'..'Z').
var ScriptColumns = map[string]bool{"LvlSetScript": true, "LevelUpScript": true}

// Row is one usable row of the table: the m_SkillInfo entry (row, style, max level) and every
// non-empty cell by column name, decoded to UTF-8.
type Row struct {
	Row      int               `json:"row"`       // 1-based row of skills.txt (row 1 is the header): m_nTabFileRowId
	ID       int               `json:"id"`        // SkillId
	Style    int               `json:"style"`     // SkillStyle (eSKillStyle)
	MaxLevel int               `json:"max_level"` // MaxLevel (KSkillManager::GetSkillMaxLevel)
	Cells    map[string]string `json:"cells"`
}

// Table is the whole file in row order.
type Table struct {
	Source  string   `json:"source"`
	Columns []string `json:"columns"`
	Rows    []Row    `json:"rows"`
	Skipped int      `json:"skipped"` // rows with an id outside 1..2000 or a negative style
}

// Parse reads the raw bytes of skills.txt.
func Parse(data []byte) *Table {
	tab := npcres.ParseTab(data)
	t := &Table{}
	for c := 1; c <= tab.Width(); c++ {
		t.Columns = append(t.Columns, tab.Get(1, c))
	}
	for row := 2; row <= tab.Height(); row++ {
		id := tabInt(tab, row, "SkillId", -1)
		if id < 1 || id > MaxSkill {
			t.Skipped++
			continue
		}
		style := tabInt(tab, row, "SkillStyle", 0)
		if style < 0 {
			t.Skipped++
			continue
		}
		r := Row{Row: row, ID: id, Style: style, MaxLevel: tabInt(tab, row, "MaxLevel", 0), Cells: map[string]string{}}
		for c, name := range t.Columns {
			raw := tab.Get(row, c+1)
			if raw == "" || name == "" {
				continue
			}
			r.Cells[name] = decodeCell(name, raw)
		}
		t.Rows = append(t.Rows, r)
	}
	return t
}

// Load reads the file from disk.
func Load(path string) (*Table, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	t := Parse(data)
	t.Source = path
	return t, nil
}

// Info is KSkillManager::m_SkillInfo[id-1]: the last row of the id, nil when the id has none.
func (t *Table) Info(id int) *Row {
	var found *Row
	for i := range t.Rows {
		if t.Rows[i].ID == id {
			found = &t.Rows[i]
		}
	}
	return found
}

// Write stores the table as JSON.
func (t *Table) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}

// Read loads a skills.json written by Write.
func Read(path string) (*Table, error) {
	blob, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	t := &Table{}
	if err := json.Unmarshal(blob, t); err != nil {
		return nil, err
	}
	return t, nil
}

// decodeCell turns the raw bytes of a cell into UTF-8 by what the column holds.
func decodeCell(column, raw string) string {
	switch {
	case PathColumns[column]:
		s := text.GBKToUTF8([]byte(raw))
		if ScriptColumns[column] {
			s = asciiLower(s)
		}
		return s
	case TextColumns[column]:
		return text.DecodeMixed([]byte(raw))
	default:
		return raw
	}
}

// asciiLower is the binary's own lower-casing (only 'A'..'Z' change; GBK bytes stay).
func asciiLower(s string) string {
	b := []byte(s)
	for i, c := range b {
		if c >= 'A' && c <= 'Z' {
			b[i] = c + 'a' - 'A'
		}
	}
	return string(b)
}

// tabInt is KTabFile::GetInteger (jx_linux_y 0x08228170): the default for an empty cell,
// otherwise strtol - the leading number, 0 when the cell starts with something else.
func tabInt(tab *npcres.TabFile, row int, column string, def int) int {
	s := strings.TrimSpace(tab.GetByName(row, column))
	if s == "" {
		return def
	}
	end := 0
	if end < len(s) && (s[end] == '-' || s[end] == '+') {
		end++
	}
	for end < len(s) && s[end] >= '0' && s[end] <= '9' {
		end++
	}
	n, err := strconv.Atoi(s[:end])
	if err != nil {
		return 0
	}
	return n
}
