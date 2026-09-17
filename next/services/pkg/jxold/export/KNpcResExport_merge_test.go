package export

import (
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
)

func TestMergeAppearanceTakesTheDrawingSideFromTheClientTable(t *testing.T) {
	server := []npcres.Template{
		{}, // row 0 unused
		{ID: 1, Name: "Heo rừng", Kind: 0, ResType: "ani018", StandFrame: 20, AttackFrame: 18, HurtFrame: 12, Stature: 60, ArmorType: 0},
		{ID: 2, Name: "Thợ rèn", Kind: 3, ResType: "passerby001", StandFrame: 30, AttackFrame: 0, Stature: 100},
		{ID: 3, Name: "Quái xa", Kind: 0, ResType: "enemy072", StandFrame: 15, AttackFrame: 30},
	}
	client := []npcres.Template{
		{},
		{ID: 1, Name: "Heo Rừng", Kind: 0, ResType: "ani018", StandFrame: 14, AttackFrame: 99, HurtFrame: 99, Stature: 80, ArmorType: 5},
		{ID: 2, Name: "", ResType: "other"}, // empty client row: nothing taken
	}
	merged, changed := MergeAppearance(server, client)
	if changed != 1 {
		t.Fatalf("changed = %d, want 1 (only id 1 differs in the drawing columns)", changed)
	}
	m1 := merged[1]
	if m1.Name != "Heo rừng" || m1.AttackFrame != 18 || m1.HurtFrame != 12 || m1.Kind != 0 {
		t.Fatalf("the server row must keep the simulation side: %+v", m1)
	}
	if m1.StandFrame != 14 || m1.Stature != 80 || m1.ArmorType != 5 {
		t.Fatalf("the client row must give the drawing side: %+v", m1)
	}
	if merged[2] != server[2] {
		t.Fatalf("an empty client row must leave the server row alone: %+v", merged[2])
	}
	if merged[3] != server[3] {
		t.Fatalf("an id the client lacks must stay: %+v", merged[3])
	}
	if server[1].StandFrame != 20 {
		t.Fatal("the server slice must not be modified in place")
	}
	if out, n := MergeAppearance(server, nil); n != 0 || len(out) != len(server) {
		t.Fatalf("no client table: %d %d", n, len(out))
	}
}
