// Package npcres reads the character/npc appearance tables of the old client
// (Settings/npcres/*.txt, Settings/npcs.txt) the way KNpcResList / KNpcResNode / KNpcTemplate
// did, so the exporter and the Godot client compose sprites exactly like the old game.
package npcres

import (
	"bytes"
	"strings"
)

// TabFile is KTabFile of the old engine: a tab separated table whose first row names the
// columns and whose first column names the rows.  Rows and columns are 1-based like the old
// API (row 1 = header, column 1 = row name) so the old lookups port unchanged.  Cells keep the
// raw bytes of the file (GBK / TCVN3 as stored).
type TabFile struct {
	rows [][]string
	cols map[string]int
}

// ParseTab splits the file into rows and cells; empty lines are dropped.
func ParseTab(data []byte) *TabFile {
	t := &TabFile{cols: map[string]int{}}
	data = bytes.TrimPrefix(data, []byte{0xEF, 0xBB, 0xBF}) // the client's tables may carry a UTF-8 BOM
	for _, line := range bytes.Split(data, []byte{'\n'}) {
		line = bytes.TrimRight(line, "\r")
		if len(bytes.TrimSpace(line)) == 0 {
			continue
		}
		t.rows = append(t.rows, strings.Split(string(line), "\t"))
	}
	if len(t.rows) > 0 {
		for i, c := range t.rows[0] {
			t.cols[strings.TrimSpace(c)] = i + 1
		}
	}
	return t
}

// Height is the number of rows including the header (KTabFile::GetHeight).
func (t *TabFile) Height() int { return len(t.rows) }

// Width is the number of columns of the header including the row-name column.
func (t *TabFile) Width() int {
	if len(t.rows) == 0 {
		return 0
	}
	return len(t.rows[0])
}

// Get returns a cell by 1-based row and column, "" when out of range.
func (t *TabFile) Get(row, col int) string {
	if row < 1 || row > len(t.rows) || col < 1 || col > len(t.rows[row-1]) {
		return ""
	}
	return strings.TrimSpace(t.rows[row-1][col-1])
}

// GetByName returns a cell by 1-based row and header name.
func (t *TabFile) GetByName(row int, column string) string {
	c, ok := t.cols[column]
	if !ok {
		return ""
	}
	return t.Get(row, c)
}

// FindRow returns the 1-based row whose first cell equals name, -1 when missing
// (KTabFile::FindRow; row 1 is the header, so the first data row is 2).
func (t *TabFile) FindRow(name string) int {
	for i, r := range t.rows {
		if len(r) > 0 && strings.TrimSpace(r[0]) == name {
			return i + 1
		}
	}
	return -1
}

// FindColumn returns the 1-based column with that header, -1 when missing.
func (t *TabFile) FindColumn(name string) int {
	if c, ok := t.cols[name]; ok {
		return c
	}
	return -1
}

// unquote strips the surrounding double quotes some cells carry ("120,8,1").
func unquote(s string) string {
	s = strings.TrimSpace(s)
	if len(s) >= 2 && s[0] == '"' && s[len(s)-1] == '"' {
		return s[1 : len(s)-1]
	}
	return strings.TrimPrefix(s, `"`)
}
