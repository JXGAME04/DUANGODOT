package player

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
)

// ChatCostTypes is how many [type] sections chatcost.ini has: the loader of the JX2 server
// (jx_linux_y 0x080A0C40) reads sections "0".."4" into KNpcSet 0x8BACAC0 + 0x1440 + type * 16.
const ChatCostTypes = 5

// ChatCostRow is one [type] of \settings\npc\player\chatcost.ini: Level (the level a speaker needs),
// Money (taken from the bag), ManaPercent (of the mana maximum, taken from the mana) and
// StaminaPercent (read at 0x080A0D49, charged by nothing).  The relay's channel table names the
// type of each channel (relay_channcfg.ini: [team] 0, [faction] 3, [tong] 0, [screen] 0, [broadcast]
// CITY 2; relay_channel.ini [WORLD] 4), and 0x080502A0 charges it before a line is spread.
type ChatCostRow struct {
	Level          int `json:"level"`
	Money          int `json:"money"`
	ManaPercent    int `json:"mana_percent"`
	StaminaPercent int `json:"stamina_percent"`
}

// ChatCost is chatcost.ini as chat_cost.json for the zone's KChatCostTable (docs/LINUX-SERVER.md §19).
type ChatCost struct {
	Source string                     `json:"source,omitempty"`
	Rows   [ChatCostTypes]ChatCostRow `json:"rows"`
}

// ParseChatCost reads the ini the way 0x080A0C40 does: KIniFile::GetInteger of Level / Money /
// ManaPercent / StaminaPercent in the sections "0".."4"; a missing key is 0.
func ParseChatCost(data []byte) ChatCost {
	ini := npcres.ParseIni(data)
	var t ChatCost
	for i := 0; i < ChatCostTypes; i++ {
		sec := fmt.Sprintf("%d", i)
		t.Rows[i] = ChatCostRow{
			Level:          iniInt(ini, sec, "Level", 0),
			Money:          iniInt(ini, sec, "Money", 0),
			ManaPercent:    iniInt(ini, sec, "ManaPercent", 0),
			StaminaPercent: iniInt(ini, sec, "StaminaPercent", 0),
		}
	}
	return t
}

// Write saves the table as JSON.
func (t *ChatCost) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, blob, 0o644)
}
