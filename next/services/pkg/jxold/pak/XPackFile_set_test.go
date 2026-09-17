package pak

import (
	"encoding/binary"
	"os"
	"path/filepath"
	"testing"
)

// writePak writes a minimal uncompressed archive (header PACK, 16-byte index entries) so the
// set logic can be tested without the old client.
func writePak(t *testing.T, path string, files map[string][]byte) {
	t.Helper()
	var data []byte
	type ent struct {
		id, off uint32
		size    int
	}
	var ents []ent
	base := uint32(headerSize)
	for name, content := range files {
		ents = append(ents, ent{FileNameToID(name), base + uint32(len(data)), len(content)})
		data = append(data, content...)
	}
	out := make([]byte, headerSize)
	binary.LittleEndian.PutUint32(out[0:], signaturePAK)
	binary.LittleEndian.PutUint32(out[4:], uint32(len(ents)))
	binary.LittleEndian.PutUint32(out[8:], uint32(headerSize+len(data)))
	out = append(out, data...)
	for _, e := range ents {
		var b [indexSize]byte
		binary.LittleEndian.PutUint32(b[0:], e.id)
		binary.LittleEndian.PutUint32(b[4:], e.off)
		binary.LittleEndian.PutUint32(b[8:], uint32(e.size))
		binary.LittleEndian.PutUint32(b[12:], uint32(e.size)) // stored size, method none
		out = append(out, b[:]...)
	}
	if err := os.WriteFile(path, out, 0o644); err != nil {
		t.Fatal(err)
	}
}

func TestOpenSetReadsOnlyThePackageSection(t *testing.T) {
	dir := t.TempDir()
	if err := os.MkdirAll(filepath.Join(dir, "data"), 0o755); err != nil {
		t.Fatal(err)
	}
	writePak(t, filepath.Join(dir, "data", "a.pak"), map[string][]byte{`\settings\x.txt`: []byte("a")})
	// the VLTK 2.0 config.ini carries other sections with numeric-looking and Path-like keys
	ini := "[Client]\nPath=\\\\wrong\nFPS=0\n\n[Package]\nPath=\\\\data\n0=a.pak\n1=missing.pak\n\n[Resolution]\n0=nonsense.pak\nWidth=1024\n"
	if err := os.WriteFile(filepath.Join(dir, "config.ini"), []byte(ini), 0o644); err != nil {
		t.Fatal(err)
	}
	set, err := OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	if len(set.Files) != 1 {
		t.Fatalf("archives: %d, want 1 (only [Package] counts, missing paks skipped)", len(set.Files))
	}
	if b, err := set.ReadFile(`\settings\x.txt`); err != nil || string(b) != "a" {
		t.Fatalf("read: %q %v", b, err)
	}
	if n, _ := set.FallbackReport(10); n != 0 {
		t.Fatalf("no fallback folder, yet %d fallback hits", n)
	}
}

func TestOpenClientSetChainServesTheReferenceFirst(t *testing.T) {
	ref, fb := t.TempDir(), t.TempDir()
	for _, d := range []string{ref, fb} {
		if err := os.MkdirAll(filepath.Join(d, "data"), 0o755); err != nil {
			t.Fatal(err)
		}
		if err := os.WriteFile(filepath.Join(d, "package.ini"), []byte("[Package]\nPath=\\\\data\n0=a.pak\n"), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	writePak(t, filepath.Join(ref, "data", "a.pak"), map[string][]byte{`\settings\both.txt`: []byte("ref")})
	writePak(t, filepath.Join(fb, "data", "a.pak"), map[string][]byte{`\settings\both.txt`: []byte("fb"), `\settings\only.txt`: []byte("fb-only")})
	set, err := OpenClientSet(ref + ";" + fb)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	if b, _ := set.ReadFile(`\settings\both.txt`); string(b) != "ref" {
		t.Fatalf("the reference must win: %q", b)
	}
	if b, _ := set.ReadFile(`\Settings\Only.txt`); string(b) != "fb-only" {
		t.Fatalf("fallback must serve what the reference lacks: %q", b)
	}
	if _, err := set.ReadFile(`\settings\none.txt`); err == nil {
		t.Fatal("missing everywhere must fail")
	}
	n, names := set.FallbackReport(10)
	if n != 1 || len(names) != 1 || names[0] != `\settings\only.txt` {
		t.Fatalf("fallback report: %d %v", n, names)
	}
	if _, err := OpenClientSet(ref + ";" + filepath.Join(fb, "nowhere")); err == nil {
		t.Fatal("a fallback folder without package.ini/config.ini must be an error")
	}
}
