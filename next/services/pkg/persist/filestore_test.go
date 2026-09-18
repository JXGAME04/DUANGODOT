package persist

import (
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/player"
	"testing"
)

func TestValidateName(t *testing.T) {
	good := []string{"Ab", "ĐạiHiệp", "TiểuLongNữ", "abcdefghijklmnop"}
	// a blank anywhere is refused, like KUiNewPlayer::GetInputInfo did (sentence 17 of the login flow)
	bad := []string{"", "a", " ab", "ab ", "Tiểu Long Nữ", "a b", "abcdefghijklmnopq", "a\tb", "\xff\xfe", "a\x00b"}
	for _, n := range good {
		if err := ValidateName(n); err != nil {
			t.Errorf("%q rejected", n)
		}
	}
	for _, n := range bad {
		if err := ValidateName(n); err == nil {
			t.Errorf("%q accepted", n)
		}
	}
}

// What a player may choose: five elements, two sexes, Kim for men only and Thủy for women only.
func TestNewCharacterChoice(t *testing.T) {
	for _, c := range []NewCharacter{{Name: "KimNam", Series: 0, Sex: 0}, {Name: "ThủyNữ", Series: 2, Sex: 1}, {Name: "HỏaNữ", Series: 3, Sex: 1, NativePlace: 53}} {
		if err := c.Validate(); err != nil {
			t.Errorf("%+v refused: %v", c, err)
		}
	}
	for _, c := range []NewCharacter{{Name: "KimNữ", Series: 0, Sex: 1}, {Name: "ThủyNam", Series: 2, Sex: 0}, {Name: "HệLạ", Series: 5}, {Name: "GiớiLạ", Series: 1, Sex: 2}} {
		if err := c.Validate(); err != ErrInvalidChoice {
			t.Errorf("%+v: %v, want ErrInvalidChoice", c, err)
		}
	}
	if err := (NewCharacter{Name: "Có Trắng"}).Validate(); err != ErrInvalidName {
		t.Errorf("a name with a blank: %v", err)
	}
}

func TestNewRoleStartsFromTheOldTemplates(t *testing.T) {
	// no templates: the placeholder numbers
	SetNewPlayerSet(nil)
	r := NewRole(7, 1, "Ai", 0, 0)
	if r.Stats.Strength != 10 || len(r.Items) != 0 {
		t.Fatalf("placeholder role %+v", r.Stats)
	}
	// newplayerini00 of the Linux server (Shaolin, male): 35/25/25/15, life 204, mana 16,
	// stamina from level_add (base 180), one starting weapon in the bag
	set := &player.Set{}
	set.LevelAdd[0] = player.LevelAdd{LifePerLevel: 4, StaminaMalePerLevel: 9, StaminaFemalePerLevel: 8, ManaPerLevel: 1, LifePerVitality: 8, ManaPerEnergy: 1, StaminaMaleBase: 180, StaminaFemaleBase: 180}
	set.NewPlayer[0] = player.NewPlayer{Present: true, Strength: 35, Dexterity: 25, Vitality: 25, Energy: 15, LifeMax: 204, ManaMax: 16, Level: 1,
		Items: []player.NewPlayerItem{{Genre: 0, Detail: 0, Particular: 4, Level: 1, Version: 2, Room: 3}}}
	SetNewPlayerSet(set)
	defer SetNewPlayerSet(nil)
	r = NewRole(7, 1, "Ai", 0, 1) // the female file is missing: 00 serves
	s := r.Stats
	if s.Strength != 35 || s.Dexterity != 25 || s.Vitality != 25 || s.Energy != 15 || s.HpMax != 204 || s.Hp != 204 || s.MpMax != 16 || s.StaminaMax != 180 {
		t.Fatalf("template role %+v", s)
	}
	if len(r.Items) != 1 || r.Items[0].Room != 0 || r.Items[0].Particular != 4 || r.Items[0].Version != 2 || r.NextItemId != 2 {
		t.Fatalf("starting items %+v", r.Items)
	}
}
