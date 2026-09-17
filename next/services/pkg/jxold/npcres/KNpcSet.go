package npcres

import (
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// ReplaceNameFile is NPC_REPLACE_NAME_FILE of KNpcSet.cpp: the editor's (Chinese) placement
// name -> the name the server shows (TCVN3).  A loose file of the old server folder.
const ReplaceNameFile = `Settings\npc\replacename_npc.txt`

// ReplaceNameFileLang is where the Linux (VNG) server keeps the same table, per language.
const ReplaceNameFileLang = `lang\vn\replacename_npc.txt`

// ParseReplaceNames reads replacename_npc.txt (KNpcSet::Init filling gNpcNameMap): keys keep
// the GBK bytes of the placement name, values are UTF-8.
func ParseReplaceNames(data []byte) map[string]string {
	tab := ParseTab(data)
	out := map[string]string{}
	for row := 2; row <= tab.Height(); row++ {
		src := tab.Get(row, 1)
		if src == "" {
			continue
		}
		out[src] = text.TCVN3ToUTF8([]byte(tab.Get(row, 2)))
	}
	return out
}

// PlacementName is the name a placed npc gets (KNpcSet::Add(int, KSPNpc*)): the template's name,
// replaced through the table when the placement's own name has an entry.
func PlacementName(replace map[string]string, placementName, templateName string) string {
	if v, ok := replace[placementName]; ok && v != "" {
		return v
	}
	return templateName
}

// StandDir is KNpcRes::GetNormalNpcStandDir: the facing (0..63) a client-only npc's stand
// frame encodes, given the stand sprite's frame count.
func StandDir(frame, totalFrames int) int {
	if totalFrames <= 0 {
		return 0
	}
	frame %= totalFrames
	if frame < 0 {
		frame += totalFrames
	}
	return (MaxNpcDir * frame) / totalFrames
}
