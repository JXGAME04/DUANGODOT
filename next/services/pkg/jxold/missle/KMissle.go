// Package missle reads \settings\missles.txt of the old server the way the JX2 server does
// (jx_linux_y 0x0805D210): every row whose MissleId is 1..999 fills the template of that id
// (0x0830ED80 + id x 0x188, the last row of an id winning) through 0x08074300, which reads
// MissleHeight, MoveKind, FollowKind, LifeTime, Speed, ResponseSkill, CollidRange, ColVanish,
// IsRangeDmg, DmgRange, Zacc, Zspeed, MissRate, Param1..3, AutoExplode and DmgInterval with
// KTabFile::GetInteger (an empty cell is 0).  The cells are kept as strings so the zone
// (KMissle.h) reads them with the binary's defaults; jxassets export-missles writes the table as
// missles.json.
package missle

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
	// File is the game path of the table.
	File = `\settings\missles.txt`
	// MaxMissle is the size of the template array (MissleId 1..999).
	MaxMissle = 999
)

// PathColumns hold GBK game paths (the sprites and sounds of the client's missiles).
var PathColumns = map[string]bool{
	"AnimFile1": true, "SndFile1": true, "AnimFile2": true, "SndFile2": true, "AnimFile3": true, "SndFile3": true,
	"AnimFile4": true, "SndFile4": true, "AnimFileB1": true, "SndFileB1": true, "AnimFileB2": true, "SndFileB2": true,
	"AnimFileB3": true, "SndFileB3": true, "AnimFileB4": true, "SndFileB4": true,
}

// TextColumns hold the Chinese names of the missiles (GBK).
var TextColumns = map[string]bool{"MissleName": true}

// Row is one usable row of the table: its id and every non-empty cell by column name, decoded to UTF-8.
type Row struct {
	Row   int               `json:"row"` // 1-based row of missles.txt (row 1 is the header)
	ID    int               `json:"id"`  // MissleId
	Cells map[string]string `json:"cells"`
}

// Table is the whole file in row order.
type Table struct {
	Source  string   `json:"source"`
	Columns []string `json:"columns"`
	Rows    []Row    `json:"rows"`
	Skipped int      `json:"skipped"` // rows with an id outside 1..999
}

// Parse reads the table's bytes the way 0x0805D210 walks the rows.
func Parse(data []byte) *Table {
	tab := npcres.ParseTab(data)
	t := &Table{}
	for c := 1; c <= tab.Width(); c++ {
		t.Columns = append(t.Columns, tab.Get(1, c))
	}
	for row := 2; row <= tab.Height(); row++ {
		id := tabInt(tab, row, "MissleId", -1)
		if id < 1 || id > MaxMissle {
			t.Skipped++
			continue
		}
		r := Row{Row: row, ID: id, Cells: map[string]string{}}
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

// Template is the row an id ends with (the last one wins), nil when the id has none.
func (t *Table) Template(id int) *Row {
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

// Read loads a missles.json written by Write.
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

func decodeCell(column, raw string) string {
	if PathColumns[column] || TextColumns[column] {
		return text.GBKToUTF8([]byte(raw))
	}
	return raw
}

// tabInt is KTabFile::GetInteger: the default for an empty cell, otherwise strtol.
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
	v, err := strconv.Atoi(s[:end])
	if err != nil {
		return 0
	}
	return v
}
