package npcres

import (
	"strings"
	"testing"
)

// The two action sound tables read the way KNpcResNode::Init did: the player's by column (MainMan / MainLady) and
// action row, the npcs' by resource row and action column; names become \sound\<name> (ComposePathAndName).
func TestParseActionSoundTables(t *testing.T) {
	player := strings.Join([]string{
		"ActionName\tMainMan\tMainLady",
		"FreeStand1\t\t",
		"FreeWalk\tsound_m39.wav\tsound_m39.wav",
		"FreeWound\tsound_m01.wav\tsound_m20.wav",
	}, "\r\n") + "\r\n"
	p := ParsePlayerSoundTable([]byte(player))
	if len(p) != 2 || p["MainMan"]["FreeWalk"] != `\sound\sound_m39.wav` || p["MainLady"]["FreeWound"] != `\sound\sound_m20.wav` {
		t.Fatalf("player table: %v", p)
	}
	if _, ok := p["MainMan"]["FreeStand1"]; ok {
		t.Fatalf("an empty cell names no sound")
	}
	npc := strings.Join([]string{
		"NpcList\tFightStand\tNormalStand2\tWound\tDie\tAttack1",
		"enemy003\t\t\tsound_e003_bat.wav\tsound_e003_die.wav\tsound_e003_at.wav",
		"enemy004\t\t\t\t\t",
		"ani011\t\tsound_a011_pst.wav\tsound_a011_bat.wav\tsound_a011_die.wav\tsound_a011_at.wav",
	}, "\n")
	n := ParseNpcSoundTable([]byte(npc))
	if len(n) != 2 || n["enemy003"]["Wound"] != `\sound\sound_e003_bat.wav` || n["ani011"]["NormalStand2"] != `\sound\sound_a011_pst.wav` {
		t.Fatalf("npc table: %v", n)
	}
	if _, ok := n["enemy004"]; ok {
		t.Fatalf("a silent row is left out")
	}
	if SoundPath("  ") != "" || SoundPath("x.wav") != `\sound\x.wav` {
		t.Fatalf("SoundPath")
	}
}
