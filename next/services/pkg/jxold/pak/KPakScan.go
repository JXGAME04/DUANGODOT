package pak

// An archive keeps no file names, only the hash of each name (KPakList::FileNameToId), so there
// is no way to list what is inside.  That stops a client we have no .ini list for - the VLTK 2.0
// one - from being read at all.
//
// Two ways round it, and this file has both:
//
//   1. Read every entry and keep the ones that are text.  A layout .ini decompresses to plain
//      bytes starting with '[' or ';', so the screens can be recovered without knowing a name.
//   2. Every path a file mentions is a name: hash it and the entry it belongs to is identified.
//      Starting from the text found in step 1 and following what it names, one archive names the
//      next, and the index fills in.

import (
	"regexp"
	"strings"
)

// TextFile is one entry of an archive that turned out to be readable text.
type TextFile struct {
	ID    uint32
	Pak   string // the archive it came from
	Size  int
	Body  []byte   // the raw bytes, still GBK
	Paths []string // the game paths it mentions, as written
	Head  string   // the first section name, e.g. "[Main]"
}

// looksLikeText accepts a buffer that is plain single-byte text: ASCII plus the GBK range, with
// no NULs and hardly any control bytes.  A sprite or a map fails on the first NUL.
func looksLikeText(b []byte) bool {
	if len(b) < 8 {
		return false
	}
	n := len(b)
	if n > 4096 {
		n = 4096
	}
	odd := 0
	for _, c := range b[:n] {
		switch {
		case c == 0:
			return false
		case c == '\r' || c == '\n' || c == '\t':
		case c < 0x20 || c == 0x7f:
			odd++
		}
	}
	return odd*40 < n // under 2.5 % oddities
}

// pathRe finds the game paths a file mentions: anything from a backslash to a known extension.
// The bytes are GBK, so the middle of a path may be any byte above 0x7f.
var pathRe = regexp.MustCompile(`(?i)\\[^\\\r\n="';]*(\\[^\\\r\n="';]*)*\.(spr|ini|txt|jpg|bmp|tga|png|cur|wav|mp3|lua|dat|wor|set)`)

// ScanText walks every entry of the archives and returns those that are readable text, newest
// archive first.  Frame sprites are skipped outright; anything that fails to decompress is
// ignored, because an archive holds plenty of formats this does not need to understand.
func (s *Set) ScanText() []TextFile {
	var out []TextFile
	for _, f := range s.Files {
		for _, e := range f.Entries() {
			if e.IsFrame() || e.Size <= 0 || e.Size > 1<<20 {
				continue
			}
			switch e.Method() {
			case MethodNone, MethodUCL, MethodUCL2:
			default:
				continue
			}
			body, err := f.Extract(e.Offset, e.Stored, int(e.Size), e.Method())
			if err != nil || !looksLikeText(body) {
				continue
			}
			t := TextFile{ID: e.ID, Pak: f.Path, Size: len(body), Body: body}
			for _, m := range pathRe.FindAll(body, -1) {
				t.Paths = append(t.Paths, string(m))
			}
			if i := strings.IndexByte(string(body), '['); i >= 0 {
				if j := strings.IndexByte(string(body[i:]), ']'); j > 0 && j < 64 {
					t.Head = string(body[i : i+j+1])
				}
			}
			out = append(out, t)
		}
	}
	return out
}

// NameIndex maps the id of every entry that some file named to the name it was given.  Feed it
// the paths ScanText collected (plus any other guesses) and it says which ones are really there.
func (s *Set) NameIndex(paths []string) map[uint32]string {
	out := map[uint32]string{}
	for _, p := range paths {
		id := FileNameToID(p)
		for _, f := range s.Files {
			if _, ok := f.Find(id); ok {
				out[id] = NormalizePath(p)
				break
			}
		}
	}
	return out
}
