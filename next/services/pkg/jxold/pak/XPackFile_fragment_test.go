package pak

import (
	"bytes"
	"encoding/binary"
	"path/filepath"
	"sort"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/oldgame"
)

// testElem is one element of a synthetic archive.
type testElem struct {
	name      string
	data      []byte
	fragments int // 0 = stored whole, n = cut into n pieces behind a fragment table
}

// buildPak writes a 'PACK' image the way the game's tools do: header, the elements, then the
// index sorted by id.  Nothing is compressed, which is a legal archive (method 0).
func buildPak(t *testing.T, elems []testElem) []byte {
	t.Helper()
	type row struct{ id, off, size, flag uint32 }
	var body bytes.Buffer
	var rows []row
	for _, el := range elems {
		off := uint32(headerSize + body.Len())
		if el.fragments == 0 {
			body.Write(el.data)
			rows = append(rows, row{FileNameToID(el.name), off, uint32(len(el.data)), uint32(len(el.data))})
			continue
		}
		// u32 count | u32 tableOffset | pieces | table
		per := (len(el.data) + el.fragments - 1) / el.fragments
		var pieces [][]byte
		for p := 0; p < len(el.data); p += per {
			end := p + per
			if end > len(el.data) {
				end = len(el.data)
			}
			pieces = append(pieces, el.data[p:end])
		}
		var stored bytes.Buffer
		stored.Write(make([]byte, 8))
		var table bytes.Buffer
		for _, piece := range pieces {
			binary.Write(&table, binary.LittleEndian, uint32(stored.Len()))
			binary.Write(&table, binary.LittleEndian, uint32(len(piece)))
			binary.Write(&table, binary.LittleEndian, uint32(len(piece))|MethodNone)
			stored.Write(piece)
		}
		out := stored.Bytes()
		binary.LittleEndian.PutUint32(out[0:], uint32(len(pieces)))
		binary.LittleEndian.PutUint32(out[4:], uint32(len(out)))
		out = append(out, table.Bytes()...)
		body.Write(out)
		rows = append(rows, row{FileNameToID(el.name), off, uint32(len(el.data)), uint32(len(out))&sizeMask | MethodFrame})
	}
	sort.Slice(rows, func(i, j int) bool { return rows[i].id < rows[j].id })
	var img bytes.Buffer
	hdr := make([]byte, headerSize)
	binary.LittleEndian.PutUint32(hdr[0:], signaturePAK)
	binary.LittleEndian.PutUint32(hdr[4:], uint32(len(rows)))
	binary.LittleEndian.PutUint32(hdr[8:], uint32(headerSize+body.Len()))
	binary.LittleEndian.PutUint32(hdr[12:], headerSize)
	img.Write(hdr)
	img.Write(body.Bytes())
	for _, r := range rows {
		binary.Write(&img, binary.LittleEndian, r.id)
		binary.Write(&img, binary.LittleEndian, r.off)
		binary.Write(&img, binary.LittleEndian, r.size)
		binary.Write(&img, binary.LittleEndian, r.flag)
	}
	return img.Bytes()
}

// An element cut into pieces reads back as one file, byte for byte.
func TestFragmentedElementIsPutBackTogether(t *testing.T) {
	big := bytes.Repeat([]byte("0123456789abcdef"), 4096) // 64 KiB
	big[0], big[len(big)-1] = 'A', 'Z'
	img := buildPak(t, []testElem{
		{name: `\settings\plain.txt`, data: []byte("plain")},
		{name: `\settings\big.txt`, data: big, fragments: 7},
	})
	f, err := OpenBytes("test.pak", img)
	if err != nil {
		t.Fatal(err)
	}
	e, ok := f.Lookup(`\settings\big.txt`)
	if !ok {
		t.Fatal("fragmented element not in the index")
	}
	if !e.IsFrame() {
		t.Fatal("the fragment bit is not set on the element")
	}
	got, err := f.Read(e)
	if err != nil {
		t.Fatalf("read: %v", err)
	}
	if !bytes.Equal(got, big) {
		t.Fatalf("fragments came back wrong: %d bytes, want %d", len(got), len(big))
	}
	plain, _ := f.Lookup(`\settings\plain.txt`)
	if data, err := f.Read(plain); err != nil || string(data) != "plain" {
		t.Fatalf("a plain element broke: %q %v", data, err)
	}
}

// A fragment table that does not fit the stored bytes is refused, never read out of bounds.
func TestBrokenFragmentTableIsRefused(t *testing.T) {
	img := buildPak(t, []testElem{{name: `\a.bin`, data: bytes.Repeat([]byte{1}, 4096), fragments: 4}})
	f, err := OpenBytes("test.pak", img)
	if err != nil {
		t.Fatal(err)
	}
	e, _ := f.Lookup(`\a.bin`)
	// point the table far past the end of the element
	binary.LittleEndian.PutUint32(img[e.Offset+4:], 1<<30)
	if _, err := f.Read(e); err == nil {
		t.Fatal("a fragment table beyond the element was accepted")
	}
	// and a count no archive could hold
	binary.LittleEndian.PutUint32(img[e.Offset:], 1<<24)
	if _, err := f.Read(e); err == nil {
		t.Fatal("an absurd fragment count was accepted")
	}
}

// The nested archive is searched LAST: a file patched into an outer archive wins, a file only the
// nested one has is still found.  That is the order engineFree.dll uses (archive #13 of 14).
func TestNestedArchiveIsMountedLast(t *testing.T) {
	nested := buildPak(t, []testElem{
		{name: `\ui\ui3_1024\uinewlogin\login.ini`, data: []byte("[Main]\nFrom=nested\n")},
		{name: `\ui\setting.ini`, data: []byte("[Theme]\nFrom=nested\n")},
	})
	outer := buildPak(t, []testElem{
		{name: `\ui\setting.ini`, data: []byte("[Theme]\nFrom=outer\n")},
		{name: NestedArchive, data: nested, fragments: 3},
	})
	f, err := OpenBytes("font.pak", outer)
	if err != nil {
		t.Fatal(err)
	}
	set := &Set{Files: []*File{f}}
	if err := set.mountNested(); err != nil {
		t.Fatal(err)
	}
	if len(set.Files) != 2 || !set.Files[1].Nested {
		t.Fatalf("nested archive not mounted last: %d archives", len(set.Files))
	}
	only, err := set.ReadFile(`\Ui\ui3_1024\UiNewLogin\login.ini`)
	if err != nil || !strings.Contains(string(only), "From=nested") {
		t.Fatalf("file that only the nested archive has: %q %v", only, err)
	}
	both, err := set.ReadFile(`\Ui\Setting.ini`)
	if err != nil || !strings.Contains(string(both), "From=outer") {
		t.Fatalf("the outer archive must win over the nested one: %q %v", both, err)
	}
}

// A JX1 client has no nested archive: opening it must not change.
func TestSetWithoutNestedArchiveIsUnchanged(t *testing.T) {
	f, err := OpenBytes("a.pak", buildPak(t, []testElem{{name: `\x.txt`, data: []byte("x")}}))
	if err != nil {
		t.Fatal(err)
	}
	set := &Set{Files: []*File{f}}
	if err := set.mountNested(); err != nil || len(set.Files) != 1 {
		t.Fatalf("set changed: %d archives, %v", len(set.Files), err)
	}
}

// vltk20Client returns the VLTK 2.0 client folder or skips: JX_VLTK20_CLIENT, else the
// "client" of next/config/oldgame.local.json when that is a 2.0 client.
func vltk20Client(t *testing.T) string {
	t.Helper()
	dir := oldgame.ClientVLTK20()
	if dir == "" {
		t.Skip("VLTK 2.0 client not found (JX_VLTK20_CLIENT, JX_OLD_CLIENT or config/oldgame.local.json)")
	}
	return dir
}

// The real thing: the 2.0 client's login window is only reachable through the nested archive.
func TestVLTK20LoginLayoutComesFromTheNestedArchive(t *testing.T) {
	dir := vltk20Client(t)
	set, err := OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer set.Close()
	last := set.Files[len(set.Files)-1]
	if !last.Nested {
		t.Fatalf("the last archive should be %s, got %s", NestedArchive, last.Path)
	}
	if n := len(last.Entries()); n < 5000 {
		t.Fatalf("%s has only %d entries", NestedArchive, n)
	}
	// \Ui\ui3_1024\UiNewLogin\<GBK: login>.ini - the name KUiLogin::LoadScheme of gamecl.exe asks for
	login := "\\Ui\\ui3_1024\\UiNewLogin\\\xb5\xc7\xc2\xbd.ini"
	f, e, ok := set.Lookup(login)
	if !ok {
		t.Fatal("the 2.0 login layout is not found")
	}
	if !f.Nested {
		t.Fatalf("expected the login layout in the nested archive, found it in %s", f.Path)
	}
	data, err := f.Read(e)
	if err != nil {
		t.Fatal(err)
	}
	for _, section := range []string{"[Main]", "[Account]", "[Password]", "[Login]", "[Cancel]"} {
		if !strings.Contains(string(data), section) {
			t.Fatalf("login layout lacks %s", section)
		}
	}
	t.Logf("%s: %d entries; login layout %d bytes", filepath.Base(last.Path), len(last.Entries()), len(data))
}
