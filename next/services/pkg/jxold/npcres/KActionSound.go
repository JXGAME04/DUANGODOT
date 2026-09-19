package npcres

import "strings"

// The action sound tables KNpcResNode::Init reads (KNpcResNode.cpp 2004: PLAYER_SOUND_FILE / NPC_SOUND_FILE; the 2.0
// client names them in its npcres file table at 0x0081F01B / 0x0081F017): what a character says when an action
// starts (KNpcRes::PlaySound, gamecl.exe 0x006DFA20, while the action is under 5 % done).
const (
	PlayerSoundFile = "\\settings\\npcres\\主角动作声音表.txt"  // rows = the player's action names, columns MainMan / MainLady
	NpcSoundFile    = "\\settings\\npcres\\npc动作声音表.txt" // rows = npc resource names, columns = the npc action names
	ResSoundPath    = "sound"                            // RES_SOUND_FILE_PATH: ComposePathAndName puts the files under \sound\
)

// SoundPath is KNpcResNode::ComposePathAndName(RES_SOUND_FILE_PATH, name): "" stays "", else \sound\<name>.
func SoundPath(name string) string {
	name = strings.TrimSpace(name)
	if name == "" {
		return ""
	}
	return "\\" + ResSoundPath + "\\" + name
}

// ParsePlayerSoundTable reads 主角动作声音表.txt: for each column (MainMan, MainLady ...) the sound of every action
// row, keyed by the action name of column 1 (KNpcResNode::Init: FindColumn(npc name), rows i + 2 in action order).
func ParsePlayerSoundTable(data []byte) map[string]map[string]string {
	tab := ParseTab(data)
	out := map[string]map[string]string{}
	for col := 2; col <= tab.Width(); col++ {
		name := tab.Get(1, col)
		if name == "" {
			continue
		}
		m := map[string]string{}
		for row := 2; row <= tab.Height(); row++ {
			action := tab.Get(row, 1)
			if p := SoundPath(tab.Get(row, col)); action != "" && p != "" {
				m[action] = p
			}
		}
		if len(m) > 0 {
			out[name] = m
		}
	}
	return out
}

// ParseNpcSoundTable reads npc动作声音表.txt: for each resource row the sound of every action column
// (KNpcResNode::Init: FindRow(npc name), GetString(row, action name)).
func ParseNpcSoundTable(data []byte) map[string]map[string]string {
	tab := ParseTab(data)
	out := map[string]map[string]string{}
	for row := 2; row <= tab.Height(); row++ {
		res := tab.Get(row, 1)
		if res == "" {
			continue
		}
		m := map[string]string{}
		for col := 2; col <= tab.Width(); col++ {
			action := tab.Get(1, col)
			if p := SoundPath(tab.Get(row, col)); action != "" && p != "" {
				m[action] = p
			}
		}
		if len(m) > 0 {
			out[res] = m
		}
	}
	return out
}
