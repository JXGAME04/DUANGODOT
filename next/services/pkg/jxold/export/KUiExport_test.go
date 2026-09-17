package export

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
)

// createScreen is what the character creation window is called in every client: the signature the
// command matches on, since a .pak keeps no file names.
var createScreen = []string{"newplayer", "name", "male", "female", "gold", "wood", "water", "fire", "earth"}

// find returns the biggest layout of the client whose sections match, the way export-ui picks one.
func find(t *testing.T, set *pak.Set, want []string) []byte {
	t.Helper()
	var best []byte
	area := -1
	for _, f := range set.ScanText() {
		if !HasSections(f.Body, want) {
			continue
		}
		if w, h := CanvasOf(f.Body); w*h > area {
			best, area = f.Body, w*h
		}
	}
	return best
}

// The VLTK 2.0 client carries the character creation window and nothing else of the login flow:
// five upright tabs 103x38 down the left edge on a 1024x768 canvas, which is what the running
// game shows.  Getting this wrong means shipping the JX1 screen and calling it 2.0.
func TestCreateScreenIsTheOneTheClientRuns(t *testing.T) {
	dir := oldClient(t)
	set, err := pak.OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	body := find(t, set, createScreen)
	if body == nil {
		t.Skip("this client has no character creation window")
	}
	out := t.TempDir()
	ex := New(set, out)
	screen, err := ex.Ui("tao-nhan-vat", "Tao nhan vat", "test", body)
	if err != nil {
		t.Fatal(err)
	}
	byName := map[string]UiWidget{}
	for _, w := range screen.Widgets {
		byName[w.Name] = w
	}
	// the five elements, in the order the game numbers them
	for i, name := range []string{"gold", "wood", "water", "fire", "earth"} {
		w, ok := byName[name]
		if !ok {
			t.Fatalf("%s missing", name)
		}
		if !w.CheckBox {
			t.Errorf("%s should be a two-state button", name)
		}
		if w.Picture == "" || w.Pressed == "" {
			t.Errorf("%s has no picture for both states: %+v", name, w)
		}
		if i > 0 {
			prev := byName[[]string{"gold", "wood", "water", "fire", "earth"}[i-1]]
			if w.Top <= prev.Top || w.Left != prev.Left {
				t.Errorf("%s does not sit under %s: %d,%d vs %d,%d", name, prev.Name, w.Left, w.Top, prev.Left, prev.Top)
			}
		}
	}
	if n := byName["name"]; n.MaxLen != 16 {
		t.Errorf("the name box keeps MaxLen=16, got %d", n.MaxLen)
	}
	// the pictures are filed under their Vietnamese name, not a hash
	for _, want := range []string{"the-kim.png", "the-kim-nhan.png", "nut-xac-dinh.png", "vai-kim-nam-1.png"} {
		if _, err := os.Stat(filepath.Join(out, "ui", "tao-nhan-vat", want)); err != nil {
			t.Errorf("%s was not written: %v", want, err)
		}
	}
	if len(screen.Portraits) != 30 {
		t.Errorf("%d character pictures, want 30 (5 elements x 2 sexes x 3 views)", len(screen.Portraits))
	}
	// files[] is the way back to the client a picture came from
	if len(screen.Files) < 10 {
		t.Errorf("only %d pictures traced back to the old client", len(screen.Files))
	}
}

// A window is recognised by its sections, and the order of the file is kept.
func TestSignatureAndOrder(t *testing.T) {
	body := []byte("; a comment\r\n[Main]\r\nLeft=0\r\nWidth=1024\r\nHeight=768\r\nStartPos=800,0\r\n\r\n[OK]\r\nLeft=51\r\nTrans=1\r\n")
	if got := SectionNames(body); len(got) != 2 || got[0] != "main" || got[1] != "ok" {
		t.Fatalf("sections %v, want [main ok]", got)
	}
	if w, h := CanvasOf(body); w != 1024 || h != 768 {
		t.Errorf("canvas %dx%d, want 1024x768", w, h)
	}
	if !HasSections(body, []string{"main", "ok"}) {
		t.Error("the signature should match")
	}
	if HasSections(body, []string{"main", "account"}) {
		t.Error("a missing section must not match")
	}
	if c := parseColor("22,15,9"); c == nil || c.R != 22 || c.G != 15 || c.B != 9 {
		t.Errorf("colour %+v, want 22,15,9", c)
	}
	if parseColor("22,15") != nil {
		t.Error("a two-part colour should be refused")
	}
}

// Every section gets a file name a person can read.
func TestSlugsAreReadable(t *testing.T) {
	for section, want := range map[string]string{
		"ok": "nut-xac-dinh", "cancel": "nut-huy", "gold": "the-kim", "male": "nam",
		"password": "mat-khau", "Login_Butterfly_0": "login-butterfly-0",
	} {
		if got := uiSlug(section); got != want {
			t.Errorf("uiSlug(%q) = %q, want %q", section, got, want)
		}
	}
}
