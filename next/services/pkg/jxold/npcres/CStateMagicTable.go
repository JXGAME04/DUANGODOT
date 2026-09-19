package npcres

import (
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// StateMagicTable is \settings\npcres\状态图形对照表.txt (STATE_MAGIC_TABLE_NAME of the old client): the
// picture a state puts on a character, one row per StateSpecialId of skills.txt.
const StateMagicTable = "\\settings\\npcres\\状态图形对照表.txt"

// The types of a state picture (StateMagicType of KNpcResNode.h; the 2.0 client adds MiniMap).
const (
	StateMagicHead    = 0 // over the head, drawn last (gamecl.exe 0x006DFAC0)
	StateMagicBody    = 1 // at the body: behind it for the frames of [BackStart, BackEnd), in front for the rest
	StateMagicFoot    = 2 // at the feet, under the body
	StateMagicMiniMap = 3 // a mark on the mini map (2.0 only, one row)
)

// StateMagic is one row of the table as the 2.0 client's loader keeps it (gamecl.exe 0x006AE200: arrays
// indexed by row order, the getter 0x006AE540 takes the id 1..count, so Status<n> must be row n + 1).
type StateMagic struct {
	ID        int    `json:"id"`
	File      string `json:"-"`            // the sprite's game path (GBK); "Special" = no picture, the client's own drawing (stun, poison, freeze, burn, confuse)
	Type      int    `json:"type"`         // StateMagicHead / Body / Foot / MiniMap (column 3: Head / Foot / MiniMap, anything else Body)
	Loop      bool   `json:"loop"`         // column 4 "Loop": plays again and again; otherwise once, then the slot is dropped (0x006E063F)
	BackStart int    `json:"behind_start"` // column 5: a Body picture is drawn behind the character for the frames [BackStart, BackEnd)
	BackEnd   int    `json:"behind_end"`   // column 6
	Frames    int    `json:"frames"`       // column 7 (default 1), KSprControl::SetSprFile raises it to Dirs
	Dirs      int    `json:"dirs"`         // column 8 (default 1)
	Interval  int    `json:"interval"`     // column 9 (default 1): logic frames of one pass through a direction's frames (KSprControl::GetNextFrame)
	Split     int    `json:"split"`        // column 10 clamped 1..3 (default 1): how many pieces the picture is cut into; every row is 1
	Name      string `json:"name"`         // column 11, the note (UTF-8)
}

// ParseStateMagicTable reads the table like CStateMagicTable::Init as the 2.0 client has it (0x006AE200):
// the first data row is id 1, the columns by number, an empty cell gives the old default.
func ParseStateMagicTable(data []byte) []StateMagic {
	tab := ParseTab(data)
	out := make([]StateMagic, 0, tab.Height())
	num := func(row, col, def int) int {
		s := tab.Get(row, col)
		if strings.TrimSpace(s) == "" {
			return def
		}
		return Atoi(s)
	}
	for row := 2; row <= tab.Height(); row++ {
		m := StateMagic{ID: row - 1, File: tab.Get(row, 2), Type: StateMagicBody, Split: 1}
		switch tab.Get(row, 3) {
		case "Head":
			m.Type = StateMagicHead
		case "Foot":
			m.Type = StateMagicFoot
		case "MiniMap":
			m.Type = StateMagicMiniMap
		}
		m.Loop = tab.Get(row, 4) == "Loop"
		m.BackStart = num(row, 5, 0)
		m.BackEnd = num(row, 6, 0)
		m.Frames = num(row, 7, 1)
		m.Dirs = num(row, 8, 1)
		m.Interval = num(row, 9, 1)
		m.Split = num(row, 10, 1)
		if m.Split < 1 {
			m.Split = 1
		} else if m.Split > 3 {
			m.Split = 3
		}
		m.Name = text.GBKToUTF8([]byte(tab.Get(row, 11)))
		out = append(out, m)
	}
	return out
}

// IsSpecial tells a row without a sprite: the client draws the state itself ("Special" file name).
func (m StateMagic) IsSpecial() bool { return m.File == "" || m.File == "Special" }
