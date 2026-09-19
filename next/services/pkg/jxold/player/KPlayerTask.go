package player

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strconv"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// TaskDefRow is one row of \settings\task\player_task_def.txt: TASK_ID_FIRST, TASK_ID_LAST (0 = the first alone),
// TASK_NAME, SYNC_FLAG (1: the zone tells the client the value whenever a script changes it, and all of them when the
// character enters the world), CLIENT_FLAG (1: the client may set the value itself), TASK_DESCRIBE.
type TaskDefRow struct {
	First  int    `json:"first"`
	Last   int    `json:"last"`
	Name   string `json:"name,omitempty"`
	Sync   bool   `json:"sync"`
	Client bool   `json:"client"`
}

// TaskDef is player_task_def.txt as task_def.json for the zone's KTaskDefTable (docs/LINUX-SERVER.md §21).
type TaskDef struct {
	Source string       `json:"source,omitempty"`
	Rows   []TaskDefRow `json:"rows"`
}

// ParseTaskDef reads the table the way the loader of the JX2 server does (jx_linux_y 0x081C6E00, this = 0x978bf60):
// from row 3 on (row 1 names the columns, row 2 describes them), columns 1, 2, 4 and 5 by position as integers
// (KTabFile::GetInteger: a blank or a word is 0), a row whose first id is 0 skipped, a last id of 0 meaning the first
// alone, a flag set when the cell is exactly 1.  The name (column 3, GBK / TCVN3 bytes) is decoded for the reader; the
// zone does not use it.  The rows keep the order of the file: the zone applies them in that order and a later row
// overwrites the flags of an id an earlier one named.
func ParseTaskDef(data []byte) TaskDef {
	tab := npcres.ParseTab(data)
	var t TaskDef
	for row := 3; row <= tab.Height(); row++ {
		first := taskTabInt(tab, row, 1)
		if first == 0 {
			continue
		}
		last := taskTabInt(tab, row, 2)
		if last == 0 {
			last = first
		}
		t.Rows = append(t.Rows, TaskDefRow{
			First:  first,
			Last:   last,
			Name:   text.DecodeMixed([]byte(tab.Get(row, 3))),
			Sync:   taskTabInt(tab, row, 4) == 1,
			Client: taskTabInt(tab, row, 5) == 1,
		})
	}
	return t
}

// SyncCount is how many ids the SYNC_FLAG rows name (what one login sends as 0xa7 packets).
func (t *TaskDef) SyncCount() int {
	n := 0
	seen := map[int]bool{}
	for _, r := range t.Rows {
		if !r.Sync || seen[r.First] {
			continue // the ranges are keyed by their first id: a second row of the same first is not kept
		}
		seen[r.First] = true
		if r.Last >= r.First {
			n += r.Last - r.First + 1
		}
	}
	return n
}

// Write saves the table as JSON.
func (t *TaskDef) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}

func taskTabInt(tab *npcres.TabFile, row, col int) int {
	n, err := strconv.Atoi(tab.Get(row, col))
	if err != nil {
		return 0
	}
	return n
}
