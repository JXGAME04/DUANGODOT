package persist

import (
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
