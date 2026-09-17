package npcres

import (
	"io/fs"
	"os"
	"path/filepath"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// FileNameToID is g_FileName2Id of the old engine (KFilePath.cpp): the hash the map editor stored
// for trap scripts and KScriptCache keyed its files by.  Case-sensitive, bytes as signed chars,
// '/' counted as '\'.
func FileNameToID(gbkPath []byte) uint32 {
	var id uint32
	for i, b := range gbkPath {
		c := int64(int8(b))
		if b == '/' {
			c = '\\'
		}
		id = uint32((int64(id)+int64(i+1)*c)%0x8000000b) * 0xffffffef
	}
	return id ^ 0x12345678
}

// ScriptIndex walks <server>/script and maps every .lua to its game path (`\script\...`, GBK
// bytes) under the ids the old engine may have used: the path as written and lower-cased.
func ScriptIndex(scriptDir string) map[uint32]string {
	out := map[uint32]string{}
	root := filepath.Clean(scriptDir)
	_ = filepath.WalkDir(root, func(p string, d fs.DirEntry, err error) error {
		if err != nil || d.IsDir() || !strings.EqualFold(filepath.Ext(p), ".lua") {
			return nil
		}
		rel, err := filepath.Rel(root, p)
		if err != nil {
			return nil
		}
		game := `\script\` + strings.ReplaceAll(rel, "/", `\`)
		gbk := ansiBytes(game)
		// KSortScript.cpp: LoadScriptToSortList hashes the relative path after g_StrLower (ASCII only:
		// strings.ToLower would mangle the GBK bytes)
		low := LowerASCII(gbk)
		out[FileNameToID(low)] = string(gbk)
		if id := FileNameToID(gbk); id != FileNameToID(low) {
			if _, dup := out[id]; !dup {
				out[id] = string(gbk)
			}
		}
		return nil
	})
	return out
}

// ansiBytes gives the bytes an ANSI program sees for a Windows file name.  The Chinese script
// folders were unpacked as raw GBK bytes on a cp1252 system, so their Unicode names are the
// cp1252 view of those bytes: every char < 256 is its byte, the cp1252 specials (0x80..0x9F)
// map back, and a real Chinese char (a folder created with a Chinese locale) goes through GBK.
func ansiBytes(s string) []byte {
	out := make([]byte, 0, len(s))
	for _, r := range s {
		switch {
		case r < 0x80 || (r >= 0xA0 && r < 0x100):
			out = append(out, byte(r))
		default:
			if b, ok := cp1252Specials[r]; ok {
				out = append(out, b)
			} else if g, err := text.UTF8ToGBK(string(r)); err == nil {
				out = append(out, g...)
			} else {
				out = append(out, '?')
			}
		}
	}
	return out
}

var cp1252Specials = map[rune]byte{
	0x20AC: 0x80, 0x0081: 0x81, 0x201A: 0x82, 0x0192: 0x83, 0x201E: 0x84, 0x2026: 0x85, 0x2020: 0x86, 0x2021: 0x87,
	0x02C6: 0x88, 0x2030: 0x89, 0x0160: 0x8A, 0x2039: 0x8B, 0x0152: 0x8C, 0x008D: 0x8D, 0x017D: 0x8E, 0x008F: 0x8F,
	0x0090: 0x90, 0x2018: 0x91, 0x2019: 0x92, 0x201C: 0x93, 0x201D: 0x94, 0x2022: 0x95, 0x2013: 0x96, 0x2014: 0x97,
	0x02DC: 0x98, 0x2122: 0x99, 0x0161: 0x9A, 0x203A: 0x9B, 0x0153: 0x9C, 0x009D: 0x9D, 0x017E: 0x9E, 0x0178: 0x9F,
}

// LowerASCII is g_StrLower: only A-Z change, every other byte (GBK pairs) stays.
func LowerASCII(b []byte) []byte {
	out := make([]byte, len(b))
	for i, c := range b {
		if c >= 'A' && c <= 'Z' {
			c += 'a' - 'A'
		}
		out[i] = c
	}
	return out
}

// ScriptExists reports whether a game path (`\script\...`) exists under the server folder.
func ScriptExists(serverDir, gamePath string) bool {
	rel := strings.TrimPrefix(strings.ReplaceAll(gamePath, `\`, "/"), "/")
	_, err := os.Stat(filepath.Join(serverDir, filepath.FromSlash(rel)))
	return err == nil
}
