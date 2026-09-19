package player

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
)

// The task system of the JX2 server ("TASKSYS" of IncludeLib; jx_linux_y 0x9786620, Init 0x081725F0): task_id.txt
// names the tasks, task_type.txt names the kinds and the four tables of each kind (condition / entity / award / talk),
// task_event.txt the events.  Every table is read by the same loader (0x08170990): the rows from the second one on,
// grouped by their first column, the other columns kept as the cells the scripts read by (row, column) - docs/LINUX-SERVER.md §22.
//
// The cells keep the bytes of the files (GBK / TCVN3) as Latin-1 runes, so the JSON is lossless: the scripts compare
// them with the same bytes from their own sources.

// TaskTableRow is one row of a task table without its key column.
type TaskTableRow struct {
	Key   string   `json:"key"`
	Cells []string `json:"cells"`
}

// TaskRecord is one row of task_id.txt (TaskID, TaskName, EventID, TaskType, CanCancel, TaskText): the loader 0x08171390
// reads columns 1..4 typed and keeps the row; its position (from 0) is the ordinal that picks the status bits.
type TaskRecord struct {
	ID    int      `json:"id"`
	Name  string   `json:"name"`
	Event int      `json:"event"`
	Type  string   `json:"type"`
	Cells []string `json:"cells"` // the columns after TaskID (TaskId(id, 1, col) of the scripts)
}

// TaskType is one row of task_type.txt with its four tables loaded.
type TaskType struct {
	Name      string         `json:"name"`
	Condition []TaskTableRow `json:"condition"`
	Entity    []TaskTableRow `json:"entity"`
	Award     []TaskTableRow `json:"award"`
	Talk      []TaskTableRow `json:"talk"`
}

// TaskTables is the whole task system as task_tables.json for the zone's KTaskManager.
type TaskTables struct {
	Source string         `json:"source,omitempty"`
	Tasks  []TaskRecord   `json:"tasks"`
	Events []TaskTableRow `json:"events"`
	Types  []TaskType     `json:"types"`
}

// Latin1 carries raw bytes as runes U+0000..U+00FF (the zone turns them back into the same bytes).
func Latin1(b []byte) string {
	r := make([]rune, len(b))
	for i, c := range b {
		r[i] = rune(c)
	}
	return string(r)
}

// ParseTaskTable reads a table the way 0x08170990 does: the rows from the second on (the first names the columns),
// grouped by the column `keyCol` (1-based), the other columns as the cells in file order.  The width is the header's.
func ParseTaskTable(data []byte, keyCol int) []TaskTableRow {
	tab := npcres.ParseTab(data)
	width := tab.Width()
	var rows []TaskTableRow
	for row := 2; row <= tab.Height(); row++ {
		key := tab.Get(row, keyCol)
		cells := make([]string, 0, width)
		for col := 1; col <= width; col++ {
			if col == keyCol {
				continue
			}
			cells = append(cells, Latin1([]byte(tab.Get(row, col))))
		}
		rows = append(rows, TaskTableRow{Key: Latin1([]byte(key)), Cells: cells})
	}
	return rows
}

// ParseTaskRecords reads task_id.txt the way 0x08171390 does: from the second row, TaskID (1) and EventID (3) as integers,
// TaskName (2) and TaskType (4) as text, the row kept whole (minus the id) for TaskId().
func ParseTaskRecords(data []byte) []TaskRecord {
	rows := ParseTaskTable(data, 1)
	tab := npcres.ParseTab(data)
	var out []TaskRecord
	for i, r := range rows {
		row := i + 2
		out = append(out, TaskRecord{
			ID:    taskTabInt(tab, row, 1),
			Name:  Latin1([]byte(tab.Get(row, 2))),
			Event: taskTabInt(tab, row, 3),
			Type:  Latin1([]byte(tab.Get(row, 4))),
			Cells: r.Cells,
		})
	}
	return out
}

// ParseTaskTables reads the whole system through `read`, which hands back a file of the old server by its path
// relative to the server folder ("settings/task/task_id.txt"; the paths of task_type.txt come with backslashes).
func ParseTaskTables(read func(rel string) ([]byte, error)) (TaskTables, error) {
	var t TaskTables
	ids, err := read("settings/task/task_id.txt")
	if err != nil {
		return t, fmt.Errorf("task_id.txt: %w", err)
	}
	t.Tasks = ParseTaskRecords(ids)
	if ev, err := read("settings/task/task_event.txt"); err == nil {
		t.Events = ParseTaskTable(ev, 1)
	}
	types, err := read("settings/task/task_type.txt")
	if err != nil {
		return t, fmt.Errorf("task_type.txt: %w", err)
	}
	tab := npcres.ParseTab(types)
	for row := 2; row <= tab.Height(); row++ {
		name := tab.Get(row, 1)
		if name == "" {
			continue
		}
		kind := TaskType{Name: Latin1([]byte(name))}
		files := []struct {
			col int
			dst *[]TaskTableRow
		}{{2, &kind.Condition}, {3, &kind.Entity}, {4, &kind.Award}, {5, &kind.Talk}}
		for _, f := range files {
			rel := strings.ReplaceAll(tab.Get(row, f.col), `\`, "/")
			if rel == "" {
				continue
			}
			data, err := read(rel)
			if err != nil {
				continue // a kind without one of its files: the table stays empty, like an unreadable file in the old loader
			}
			*f.dst = ParseTaskTable(data, 1)
		}
		t.Types = append(t.Types, kind)
	}
	return t, nil
}

// Write saves the tables as JSON.
func (t *TaskTables) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}

// TaskKeyHash is the hash the temp values of a task are keyed by (jx_linux_y 0x0821DF00: SetTmpValue / GetTmpValue turn
// the type name into it): for every byte c (signed) at position n (from 1): x = c * n + h, x -= 0x8000000b when at
// least that, h = x * -17; the result xor 0x12345678; an empty name is 0x12345678 itself.
func TaskKeyHash(s string) uint32 {
	var h uint32
	n := uint32(0)
	for i := 0; i < len(s); i++ {
		n++
		c := int32(int8(s[i]))
		x := uint32(c*int32(n)) + h
		if x >= 0x8000000b {
			x -= 0x8000000b
		}
		h = x * 0xffffffef
	}
	return h ^ 0x12345678
}
