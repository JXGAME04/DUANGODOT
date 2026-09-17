package log

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

var when = time.Date(2026, 9, 17, 14, 32, 5, 123_000_000, time.Local)

func TestTextConsoleSpeaksTheCataloguesLanguage(t *testing.T) {
	c := &Catalog{
		Levels:     map[string]string{"info": "THÔNG TIN", "warn": "CẢNH BÁO"},
		Categories: map[string]string{"net": "mạng", "zone": "zone"},
		Fields:     map[string]string{"remote": "địa chỉ client", "sid": "phiên"},
		Messages:   map[string]string{"client connected": "Người chơi vừa kết nối"},
	}
	got := FormatText(c, when, LevelInfo, "net", Context{Sid: 42}, "client connected", []Field{F("remote", "127.0.0.1:50123"), F("transport", "tcp")}, false)
	want := "14:32:05.123 THÔNG TIN    [mạng]        Người chơi vừa kết nối · địa chỉ client=127.0.0.1:50123 · transport=tcp · phiên=42"
	if got != want {
		t.Errorf("text line\n got %q\nwant %q", got, want)
	}
	// what the catalogue does not know stays English - never an empty hole
	got = FormatText(c, when, LevelError, "zone.fight", Context{}, "a brand new message", nil, false)
	if want := "14:32:05.123 ERROR        [zone.fight]  a brand new message"; got != want {
		t.Errorf("untranslated line\n got %q\nwant %q", got, want)
	}
	// no catalogue at all: plain English
	got = FormatText(nil, when, LevelWarn, "net", Context{}, "accept failed", []Field{F("error", "boom")}, false)
	if want := "14:32:05.123 WARN         [net]         accept failed · error=boom"; got != want {
		t.Errorf("english line\n got %q\nwant %q", got, want)
	}
	if coloured := FormatText(c, when, LevelWarn, "net", Context{}, "x", nil, true); !strings.Contains(coloured, "\x1b[33;1mCẢNH BÁO\x1b[0m") {
		t.Errorf("the level label is the only coloured part: %q", coloured)
	}
}

// The catalogue of the repository loads, and the file sink stays JSON whatever the console does.
func TestCatalogueOfTheRepositoryAndTheFileStaysJSON(t *testing.T) {
	c := LoadCatalog("", "vi")
	if c == nil {
		t.Fatal("config/log.vi.json was not found upwards from the package folder")
	}
	if c.Size() < 250 {
		t.Errorf("the catalogue translates only %d entries", c.Size())
	}
	for _, msg := range []string{"gateway listening", "login ok", "client connected", "zone link down, reconnecting"} {
		if c.Messages[msg] == "" {
			t.Errorf("no Vietnamese for %q", msg)
		}
	}
	if LoadCatalog("", "en") != nil || LoadCatalog(filepath.Join(t.TempDir(), "missing.json"), "vi") != nil {
		t.Error("English and a missing file mean: no catalogue")
	}

	file := filepath.Join(t.TempDir(), "gw.log")
	console, err := os.Create(filepath.Join(t.TempDir(), "console.txt"))
	if err != nil {
		t.Fatal(err)
	}
	old := s.console
	s.console = console
	defer func() { s.console = old; _ = Init(Options{}) }()
	if err := Init(Options{Level: LevelInfo, Console: true, File: file, Process: "gateway"}); err != nil {
		t.Fatal(err)
	}
	Info("boot", "gateway listening", F("tcp", 17100))
	Shutdown()
	console.Close()
	text, _ := os.ReadFile(console.Name())
	if !strings.Contains(string(text), "Gateway đã mở cổng cho người chơi · cổng TCP=17100") {
		t.Errorf("console: %q", text)
	}
	if strings.Contains(string(text), "\x1b[") {
		t.Errorf("a console that is a file gets no colour codes: %q", text)
	}
	line, _ := os.ReadFile(file)
	if !strings.Contains(string(line), `"msg":"gateway listening","tcp":"17100"`) {
		t.Errorf("the file must stay JSON in English: %q", line)
	}
}
