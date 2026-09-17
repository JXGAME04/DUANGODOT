package main

// census: read EVERY entry of every archive and say what it is.
//
// An archive keeps no names, so "is the login window in there?" cannot be answered by looking a
// path up - only by opening everything.  This does that once and writes the answer down: which
// entries decompress, and what each one turns out to be (sprite, picture, sound, text, compiled
// Lua ...).  It is also the self-check of the archive reader: an entry that does not decompress
// is counted and listed, never skipped silently.

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// sniff names the format of a decompressed entry from its first bytes.
func sniff(b []byte) string {
	has := func(off int, magic string) bool {
		return len(b) >= off+len(magic) && string(b[off:off+len(magic)]) == magic
	}
	switch {
	case len(b) == 0:
		return "empty"
	case has(0, "SPR\x00"):
		return "spr"
	case has(0, "\xff\xd8\xff"):
		return "jpg"
	case has(0, "\x89PNG"):
		return "png"
	case has(0, "BM") && len(b) > 14:
		return "bmp"
	case has(0, "DDS "):
		return "dds"
	case has(0, "RIFF") && has(8, "WAVE"):
		return "wav"
	case has(0, "OggS"):
		return "ogg"
	case has(0, "ID3"), len(b) > 2 && b[0] == 0xff && b[1]&0xe0 == 0xe0:
		return "mp3"
	case has(0, "FWS"), has(0, "CWS"), has(0, "ZWS"):
		return "swf"
	case has(0, "\x1bLua"):
		return "lua-bytecode"
	case has(0, "\x00\x01\x00\x00"), has(0, "OTTO"), has(0, "ttcf"):
		return "font"
	case has(0, "PACK"):
		return "pak"
	case has(0, "MZ"):
		return "exe"
	}
	if !textLike(b) {
		return "binary"
	}
	body := b
	if len(body) > 8192 {
		body = body[:8192]
	}
	s := string(body)
	switch {
	case strings.Contains(s, "function ") || strings.Contains(s, "Include(") || strings.Contains(s, "local "):
		return "text-lua"
	case iniLike(s):
		return "text-ini"
	case strings.Count(s, "\t") > strings.Count(s, "\n"):
		return "text-tab"
	}
	return "text"
}

// textLike is the same rule pak.ScanText uses: no NUL and hardly any control bytes.
func textLike(b []byte) bool {
	if len(b) < 4 {
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
	return odd*40 < n
}

func iniLike(s string) bool {
	for _, line := range strings.Split(s, "\n") {
		line = strings.TrimSpace(line)
		if len(line) > 2 && line[0] == '[' && line[len(line)-1] == ']' {
			return true
		}
	}
	return false
}

// tally counts the entries of one kind and the bytes they unpack to.
type tally struct{ n, bytes int }

func printTally(m map[string]*tally) {
	keys := make([]string, 0, len(m))
	for k := range m {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	for _, k := range keys {
		fmt.Printf("    %-14s %8d muc %10.1f MB\n", k, m[k].n, float64(m[k].bytes)/1e6)
	}
}

// cmdCensus walks the set and writes <out>/census.tsv plus every text entry under <out>/text.
func cmdCensus(set *pak.Set, out string, dumpText bool) {
	if err := os.MkdirAll(filepath.Join(out, "text"), 0o755); err != nil {
		fail("%v", err)
	}
	report, err := os.Create(filepath.Join(out, "census.tsv"))
	if err != nil {
		fail("%v", err)
	}
	defer report.Close()
	fmt.Fprintln(report, "pak\tid\tsize\tstored\tflags\tkind\tnote")

	grand := map[string]*tally{}
	failures := 0
	seen := map[uint32]bool{} // the first archive that has an id wins, exactly as the game reads them
	for _, f := range set.Files {
		perPak := map[string]*tally{}
		name := filepath.Base(f.Path)
		shadowed := 0
		for _, e := range f.Entries() {
			if seen[e.ID] {
				shadowed++
				continue
			}
			seen[e.ID] = true
			kind, note := "", ""
			var body []byte
			if e.Size < 0 || e.Size > 256<<20 {
				kind, note = "failed", fmt.Sprintf("absurd size %d", e.Size)
			} else if e.IsFrame() {
				// stored in pieces: a 2.0 fragment table reads like any file, a JX1 frame sprite does not
				if b, err := f.Read(e); err == nil {
					body = b
					kind = sniff(b) + "+fragments"
				} else {
					kind = "spr-frames"
				}
			} else {
				b, err := f.Read(e)
				if err != nil {
					kind, note = "failed", err.Error()
				} else {
					body = b
					kind = sniff(b)
					if kind == "binary" {
						// the first bytes are how an unknown format gets a name later
						head := b
						if len(head) > 16 {
							head = head[:16]
						}
						note = fmt.Sprintf("% x", head)
					}
				}
			}
			if kind == "failed" {
				failures++
			}
			if dumpText && strings.HasPrefix(kind, "text") {
				ext := strings.TrimPrefix(strings.TrimPrefix(kind, "text"), "-")
				if ext == "" {
					ext = "txt"
				}
				// raw bytes: a file mixes GBK and TCVN3, so converting here would destroy one of them
				_ = os.WriteFile(filepath.Join(out, "text", fmt.Sprintf("%08x.%s", e.ID, ext)), body, 0o644)
			}
			fmt.Fprintf(report, "%s\t%08x\t%d\t%d\t%02x\t%s\t%s\n", name, e.ID, e.Size, e.Stored, e.Flags>>24, kind, note)
			for _, m := range []map[string]*tally{perPak, grand} {
				t := m[kind]
				if t == nil {
					t = &tally{}
					m[kind] = t
				}
				t.n++
				t.bytes += int(e.Size)
			}
		}
		fmt.Printf("%-18s %7d muc (%d bi kho doc truoc che)\n", name, len(f.Entries()), shadowed)
		printTally(perPak)
	}
	fmt.Println("TONG (moi id chi tinh mot lan, o kho duoc doc truoc)")
	printTally(grand)
	fmt.Printf("census: %d muc khong giai nen duoc; bao cao: %s\n", failures, filepath.Join(out, "census.tsv"))
}

// cmdCheckTrace reads a trace of what the real game opened (one "n<TAB>source<TAB>name" line per
// open, the name in the game's own bytes) and says whether this reader finds the same files.
// Every file the game found in an archive and we do not is a hole in our reading of the format:
// that is how the nested archive \reslst.dat was discovered.
func cmdCheckTrace(set *pak.Set, tracePath string) {
	raw, err := os.ReadFile(tracePath)
	if err != nil {
		fail("%v", err)
	}
	agree, missing, extra := 0, 0, 0
	for _, line := range strings.Split(string(raw), "\n") {
		cols := strings.SplitN(strings.TrimRight(line, "\r"), "\t", 3)
		if len(cols) != 3 {
			continue
		}
		source, name := cols[1], cols[2]
		_, _, ok := set.Lookup(name)
		switch {
		case strings.HasPrefix(source, "pak") && ok:
			agree++
		case strings.HasPrefix(source, "pak"):
			missing++
			if missing <= 20 {
				fmt.Printf("  game tim thay, ta KHONG: %s\n", text.GBKToUTF8([]byte(name)))
			}
		case source == "NOT FOUND" && ok:
			extra++
			fmt.Printf("  game KHONG tim thay, ta co: %s\n", text.GBKToUTF8([]byte(name)))
		}
	}
	fmt.Printf("check-trace: khop %d, thieu %d, thua %d\n", agree, missing, extra)
	if missing > 0 {
		os.Exit(1)
	}
}

// grepSections prints every text entry of the set that has all the wanted section names: the way
// a window is found when the archive cannot tell its file name.
func grepSections(set *pak.Set, want []string) {
	for _, t := range set.ScanText() {
		have := map[string]bool{}
		for _, line := range strings.Split(string(t.Body), "\n") {
			line = strings.TrimSpace(line)
			if len(line) > 2 && line[0] == '[' {
				if end := strings.IndexByte(line, ']'); end > 0 {
					have[strings.ToLower(line[1:end])] = true
				}
			}
		}
		ok := true
		for _, w := range want {
			if !have[strings.ToLower(w)] {
				ok = false
				break
			}
		}
		if ok {
			fmt.Printf("%08x  %-16s %6d byte  %s\n", t.ID, filepath.Base(t.Pak), t.Size, text.GBKToUTF8([]byte(t.Head)))
		}
	}
}
