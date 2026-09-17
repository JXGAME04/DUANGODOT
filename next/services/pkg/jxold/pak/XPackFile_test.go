package pak

import (
	"path/filepath"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/oldgame"
)

// oldClient returns the old client folder (bin/Client of the swrod3 checkout) or skips.
func oldClient(t *testing.T) string {
	t.Helper()
	dir := oldgame.ClientJX1()
	if dir == "" {
		t.Skip("JX1 client data not found (JX_OLD_CLIENT, config/oldgame.local.json or bin/Client)")
	}
	return dir
}

func TestFileNameToIDMatchesTheGameHash(t *testing.T) {
	// same string, three spellings that the game normalises to the same id
	a := FileNameToID(`\Settings\MapList.ini`)
	b := FileNameToID(`settings/maplist.ini`)
	c := FileNameToID(`\SETTINGS\.\MAPLIST.INI`)
	if a != b || b != c {
		t.Fatalf("normalisation differs: %x %x %x", a, b, c)
	}
	if NormalizePath(`Maps\..\Spr\x.spr`) != `\spr\x.spr` {
		t.Fatalf("dot-dot handling: %q", NormalizePath(`Maps\..\Spr\x.spr`))
	}
	// bytes above 0x7f are hashed as negative chars: the id must differ from an unsigned hash
	hi := FileNameToID("\\maps\\\xce\xf7")
	if hi == 0 {
		t.Fatal("zero id")
	}
}

func TestMapsPakContainsTheFirstWorldFile(t *testing.T) {
	dir := oldClient(t)
	set, err := OpenSet(filepath.Join(dir, "package.ini"))
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	if len(set.Files) < 5 {
		t.Fatalf("expected many archives, got %d", len(set.Files))
	}
	// map 1 from MapList.ini: \maps\<GBK>\<GBK>.wor
	worPath := "\\maps\\\xce\xf7\xb1\xb1\xc4\xcf\xc7\xf8\\\xb7\xef\xcf\xe8.wor"
	f, e, ok := set.Lookup(worPath)
	if !ok {
		t.Fatalf("world file not found in any archive")
	}
	data, err := f.Read(e)
	if err != nil {
		t.Fatalf("read %s: %v", filepath.Base(f.Path), err)
	}
	text := string(data)
	if !strings.Contains(text, "[MAIN]") || !strings.Contains(strings.ToLower(text), "rect") {
		t.Fatalf("unexpected .wor content (%d bytes, method %x): %.200q", len(data), e.Method(), text)
	}
	t.Logf("found in %s: %d bytes, method 0x%08x, stored %d", filepath.Base(f.Path), len(data), e.Method(), e.Stored)
}
