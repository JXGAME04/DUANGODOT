package export

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
)

// The login window is the one screen every player sees first, so its numbers are pinned here
// against \Ui\Ui3\login.ini as the old client reads it: the account box sits at 351,238 with a
// 80-character limit, the password box says Type=1, and the Login button names three frames.
func TestLoginScreenKeepsTheOldRectangles(t *testing.T) {
	dir := oldClient(t)
	set, err := pak.OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	ex := New(set, t.TempDir())
	screen, err := ex.Ui("login", "Ui3", "\xb5\xc7\xc2\xbd.ini")
	if err != nil {
		t.Skipf("this client has no login scheme: %v", err)
	}
	if screen.Width != 800 || screen.Height != 600 {
		t.Fatalf("canvas %dx%d, want 800x600", screen.Width, screen.Height)
	}
	byName := map[string]UiWidget{}
	for _, w := range screen.Widgets {
		byName[w.Name] = w
	}
	acc, ok := byName["account"]
	if !ok {
		t.Fatal("no Account section")
	}
	if acc.Left != 351 || acc.Top != 238 || acc.Width != 156 || acc.Height != 18 {
		t.Errorf("account box %d,%d %dx%d, want 351,238 156x18", acc.Left, acc.Top, acc.Width, acc.Height)
	}
	if acc.MaxLen != 80 || acc.Password {
		t.Errorf("account box MaxLen=%d password=%v, want 80 false", acc.MaxLen, acc.Password)
	}
	if pw := byName["password"]; !pw.Password || pw.Type != 1 {
		t.Errorf("password box password=%v type=%d, want true 1", pw.Password, pw.Type)
	}
	login := byName["login"]
	if login.Up == nil || login.Down == nil || login.Over == nil {
		t.Fatalf("login button has no frames: %+v", login)
	}
	if *login.Up != 0 || *login.Down != 1 || *login.Over != 2 {
		t.Errorf("login frames %d/%d/%d, want 0/1/2", *login.Up, *login.Down, *login.Over)
	}
	if login.Sprite == "" {
		t.Error("login button sprite was not exported")
	}
	// the window itself is written first, because the old client draws it first
	if screen.Widgets[0].Name != "main" {
		t.Errorf("first section %q, want main", screen.Widgets[0].Name)
	}
}

// The create screen builds its character pictures by name; all thirty must be there or a player
// picking an element sees an empty frame.
func TestCharacterPicturesCoverEveryElementAndSex(t *testing.T) {
	dir := oldClient(t)
	set, err := pak.OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	out := t.TempDir()
	ex := New(set, out)
	screen, err := ex.Ui("select_role", "Ui3", "\xd1\xa1\xd3\xce\xcf\xb7\xb4\xe6\xb5\xb5\xc8\xcb\xce\xef.ini")
	if err != nil {
		t.Skipf("this client has no select scheme: %v", err)
	}
	if len(screen.Portraits) != 30 {
		t.Fatalf("%d pictures, want 30 (5 elements x 2 sexes x 3 views)", len(screen.Portraits))
	}
	for _, key := range []string{"metal_male_0", "water_female_1", "earth_male_2"} {
		id, ok := screen.Portraits[key]
		if !ok {
			t.Errorf("%s missing", key)
			continue
		}
		if _, err := os.Stat(filepath.Join(out, "sprites", id+".png")); err != nil {
			t.Errorf("%s -> %s was not written: %v", key, id, err)
		}
	}
}

// The ini keeps its order and nothing a section said is thrown away.
func TestIniKeepsOrderAndUnknownKeys(t *testing.T) {
	secs := parseIniOrdered([]byte("; a comment\r\n[Main]\r\nLeft=0\r\nStartPos=800,0\r\n\r\n[OK]\r\nLeft=51\r\nTrans=0\r\n"))
	if len(secs) != 2 || secs[0].name != "main" || secs[1].name != "ok" {
		t.Fatalf("sections %+v, want main then ok", secs)
	}
	if secs[0].values["startpos"] != "800,0" {
		t.Errorf("startpos %q", secs[0].values["startpos"])
	}
	if c := parseColor("22,15,9"); c == nil || c.R != 22 || c.G != 15 || c.B != 9 {
		t.Errorf("colour %+v, want 22,15,9", c)
	}
	if parseColor("22,15") != nil {
		t.Error("a two-part colour should be refused")
	}
}
