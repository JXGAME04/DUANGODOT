package player

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// KillEventRow is one row of \settings\npc\player\event_killnpc.txt the way the loader of the JX2 server reads it
// (jx_linux_y 0x08156760, from the second row): 事件ID (1), 事件响应脚本 (2), 事件响应函数 (3), 任务ID (4, the task
// value the count is mirrored into), 只响应一次 (5), 杀NPC总数 (6), 地图ID (7), NPC模板ID (8), NPC能力 (9: -1 any, 0
// player, 1 npc, 2 gold, 3 boss), NPC级别 (10), NPC名字 (11), 说明 (12).  A blank number is -1 like KTabFile::GetInteger
// with that default; the task value is 0 when blank (the loader keeps rows up to 0x176f).
type KillEventRow struct {
	ID          int    `json:"id"`
	Script      string `json:"script"`
	Function    string `json:"function"`
	TaskID      int    `json:"task_id"`
	OnlyOnce    bool   `json:"only_once"`
	Total       int    `json:"total"`
	Map         int    `json:"map"`
	NpcTemplate int    `json:"npc_template"`
	Power       int    `json:"power"`
	Level       int    `json:"level"`
	NpcName     string `json:"npc_name,omitempty"`
}

// KillEvents is event_killnpc.txt as kill_events.json for the zone's KKillEventTable (docs/LINUX-SERVER.md §23).
type KillEvents struct {
	Source string         `json:"source,omitempty"`
	Rows   []KillEventRow `json:"rows"`
}

// ParseKillEvents reads the table: the rows from the second on, the numbers by position with -1 for a blank (the id
// and the task value 0), the script path as the file has it, the npc name decoded for the reader.
func ParseKillEvents(data []byte) KillEvents {
	tab := npcres.ParseTab(data)
	var t KillEvents
	for row := 2; row <= tab.Height(); row++ {
		id := killInt(tab.Get(row, 1), -1)
		if id < 0 {
			continue
		}
		t.Rows = append(t.Rows, KillEventRow{
			ID:          id,
			Script:      strings.TrimSpace(tab.Get(row, 2)),
			Function:    strings.TrimSpace(tab.Get(row, 3)),
			TaskID:      killInt(tab.Get(row, 4), 0),
			OnlyOnce:    killInt(tab.Get(row, 5), 0) != 0,
			Total:       killInt(tab.Get(row, 6), -1),
			Map:         killInt(tab.Get(row, 7), -1),
			NpcTemplate: killInt(tab.Get(row, 8), -1),
			Power:       killInt(tab.Get(row, 9), -1),
			Level:       killInt(tab.Get(row, 10), -1),
			NpcName:     text.DecodeMixed([]byte(tab.Get(row, 11))),
		})
	}
	return t
}

// Write saves the table as JSON.
func (t *KillEvents) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}

func killInt(s string, def int) int {
	n, err := strconv.Atoi(strings.TrimSpace(s))
	if err != nil {
		return def
	}
	return n
}
