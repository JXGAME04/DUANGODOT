package npcres

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

func TestFileNameToIDMatchesTheTrapIdsOfTheMapEditor(t *testing.T) {
	// the south gate trap of Phượng Tường in Region_S.dat carries this id (jxassets traps 1)
	gbk, err := text.UTF8ToGBK(`\script\西北南区\凤翔\连接trap\凤翔南门.lua`)
	if err != nil {
		t.Fatal(err)
	}
	if got := FileNameToID(gbk); got != 0x122ff2f0 {
		t.Fatalf("id %08x, want 122ff2f0", got)
	}
	// forward slashes count as backslashes (the linux build of g_FileName2Id)
	if FileNameToID([]byte(`/script/a.lua`)) != FileNameToID([]byte(`\script\a.lua`)) {
		t.Fatal("slash kinds must hash the same")
	}
	if string(LowerASCII([]byte("AbC\xce\xf7"))) != "abc\xce\xf7" {
		t.Fatalf("LowerASCII touched non-ascii bytes: %q", LowerASCII([]byte("AbC\xce\xf7")))
	}
}

func TestScriptIndexKeysTheLowerCasedGamePath(t *testing.T) {
	dir := t.TempDir()
	if err := os.MkdirAll(filepath.Join(dir, "Maps", "Traps"), 0o755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "Maps", "Traps", "Gate.lua"), []byte("function main(sel) end"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "readme.txt"), []byte("no"), 0o644); err != nil {
		t.Fatal(err)
	}
	idx := ScriptIndex(dir)
	want := FileNameToID([]byte(`\script\maps\traps\gate.lua`))
	if got := idx[want]; got != `\script\Maps\Traps\Gate.lua` {
		t.Fatalf("index[%08x] = %q; index %v", want, got, idx)
	}
	if _, ok := idx[FileNameToID([]byte(`\script\readme.txt`))]; ok {
		t.Fatal("only .lua files belong to the index")
	}
	if !ScriptExists(dir, `\Maps\Traps\Gate.lua`) || ScriptExists(dir, `\Maps\nothere.lua`) {
		t.Fatal("ScriptExists")
	}
}
