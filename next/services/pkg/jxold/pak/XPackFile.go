// Package pak reads the old JX client archives (XPackFile: 'PACK' header, an index of hashed
// file names and UCL/zlib compressed entries).  Files are addressed by the hash of their
// lower-cased path (FileNameToID), never by name, so callers must know the names they want.
package pak

import (
	"bytes"
	"compress/zlib"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/nrv2b"
)

const (
	MethodNone   = 0x00000000
	MethodUCL    = 0x01000000
	MethodBZip2  = 0x02000000
	MethodFrame  = 0x10000000 // sprite stored frame by frame (see ReadSprite)
	MethodUCL2   = 0x20000000
	methodMask   = 0x0f000000
	filterMask   = 0xff000000
	sizeMask     = 0x00ffffff
	headerSize   = 32
	indexSize    = 16
	signaturePAK = 0x4b434150 // 'PACK'
)

var ErrNotFound = errors.New("pak: file not found")

// Entry is one file inside an archive.
type Entry struct {
	ID       uint32
	Offset   uint32
	Size     int32  // uncompressed size
	Stored   uint32 // compressed size in the archive (low 24 bits of the flag)
	Flags    uint32 // method bits (MethodUCL, MethodFrame, ...)
	pakIndex int
}

// Method returns the compression method bits without the frame flag (XPackFile keeps the
// whole top byte: TYPE_UCL 0x01, TYPE_BZIP2 0x02, TYPE_UCL_2ND 0x20).
func (e Entry) Method() uint32 { return MethodOf(e.Flags) }

// MethodOf extracts the method from a flag word (entry or per-frame).
func MethodOf(flags uint32) uint32 { return flags & (filterMask &^ MethodFrame) }

// IsFrame reports whether the entry is a frame-compressed sprite.
func (e Entry) IsFrame() bool { return e.Flags&MethodFrame != 0 }

// File is one open .pak archive.
type File struct {
	Path    string
	f       *os.File
	size    int64
	entries []Entry // sorted by ID as stored
	index   map[uint32]int
}

// Open reads the header and index of a .pak.
func Open(path string) (*File, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	st, err := f.Stat()
	if err != nil {
		f.Close()
		return nil, err
	}
	var hdr [headerSize]byte
	if _, err := io.ReadFull(f, hdr[:]); err != nil {
		f.Close()
		return nil, fmt.Errorf("pak: %s: header: %w", path, err)
	}
	if binary.LittleEndian.Uint32(hdr[0:]) != signaturePAK {
		f.Close()
		return nil, fmt.Errorf("pak: %s: bad signature", path)
	}
	count := binary.LittleEndian.Uint32(hdr[4:])
	indexOff := binary.LittleEndian.Uint32(hdr[8:])
	if count == 0 || int64(indexOff)+int64(count)*indexSize > st.Size() {
		f.Close()
		return nil, fmt.Errorf("pak: %s: bad index", path)
	}
	raw := make([]byte, int(count)*indexSize)
	if _, err := f.ReadAt(raw, int64(indexOff)); err != nil {
		f.Close()
		return nil, fmt.Errorf("pak: %s: index: %w", path, err)
	}
	p := &File{Path: path, f: f, size: st.Size(), entries: make([]Entry, count), index: make(map[uint32]int, count)}
	for i := range p.entries {
		b := raw[i*indexSize:]
		flag := binary.LittleEndian.Uint32(b[12:])
		p.entries[i] = Entry{
			ID:     binary.LittleEndian.Uint32(b[0:]),
			Offset: binary.LittleEndian.Uint32(b[4:]),
			Size:   int32(binary.LittleEndian.Uint32(b[8:])),
			Stored: flag & sizeMask,
			Flags:  flag & filterMask,
		}
		p.index[p.entries[i].ID] = i
	}
	return p, nil
}

// Close releases the file.
func (p *File) Close() error { return p.f.Close() }

// Entries returns the index (sorted by id).
func (p *File) Entries() []Entry { return p.entries }

// Find looks an id up.
func (p *File) Find(id uint32) (Entry, bool) {
	i, ok := p.index[id]
	if !ok {
		return Entry{}, false
	}
	return p.entries[i], true
}

// Lookup finds a file by its game path (e.g. `\maps\x\y.wor`).
func (p *File) Lookup(path string) (Entry, bool) { return p.Find(FileNameToID(path)) }

// ReadRaw returns stored bytes without decompressing.
func (p *File) ReadRaw(off uint32, n uint32) ([]byte, error) {
	if int64(off)+int64(n) > p.size {
		return nil, fmt.Errorf("pak: read beyond end (off %d len %d)", off, n)
	}
	buf := make([]byte, n)
	if _, err := p.f.ReadAt(buf, int64(off)); err != nil {
		return nil, err
	}
	return buf, nil
}

// Extract decompresses one block with the given method (XPackFile::ExtractRead).
func (p *File) Extract(off uint32, stored uint32, size int, method uint32) ([]byte, error) {
	raw, err := p.ReadRaw(off, stored)
	if err != nil {
		return nil, err
	}
	switch method {
	case MethodNone:
		if int(stored) != size {
			return nil, fmt.Errorf("pak: stored size %d != size %d for uncompressed entry", stored, size)
		}
		return raw, nil
	case MethodUCL, MethodUCL2:
		return nrv2b.Decompress(raw, size)
	default:
		return nil, fmt.Errorf("pak: unsupported compression method 0x%08x", method)
	}
}

// Read returns the whole uncompressed content of a plain entry.  For frame sprites use ReadSprite.
func (p *File) Read(e Entry) ([]byte, error) {
	if e.IsFrame() {
		return nil, errors.New("pak: frame-compressed sprite, use ReadSprite")
	}
	return p.Extract(e.Offset, e.Stored, int(e.Size), e.Method())
}

// Set is the ordered list of archives from package.ini; the first archive that has an id wins.
type Set struct {
	Files []*File
	// archives from index fallbackFrom on belong to fallback folders (OpenClientSet "a;b"):
	// every file served from them is counted in FallbackHits so an export can say what did
	// not come from the reference client.
	fallbackFrom int
	FallbackHits map[string]int
}

// OpenClientSet opens the archive list of an old game folder: package.ini (JX1 clients and
// servers) or config.ini (the VLTK 2.0 client keeps its [Package] list inside the main ini).
// Several folders separated by ';' form a chain: the first is the reference, the others only
// serve files the reference lacks (their hits are reported, see FallbackReport).
func OpenClientSet(dirs string) (*Set, error) {
	var set *Set
	for _, dir := range strings.Split(dirs, ";") {
		dir = strings.TrimSpace(dir)
		if dir == "" {
			continue
		}
		var last error
		var one *Set
		// packageclasscial.ini is the VLTK 2.0 client's full list: config.ini names 11 archives,
		// that one names 13 and adds res1.pak and res2.pak (2,9 GB of the client's own resources).
		for _, ini := range []string{"package.ini", "packageclasscial.ini", "config.ini"} {
			p := filepath.Join(dir, ini)
			if _, err := os.Stat(p); err != nil {
				last = err
				continue
			}
			one, last = OpenSet(p)
			break
		}
		if one == nil {
			if set != nil {
				set.Close()
			}
			return nil, fmt.Errorf("pak: no package.ini or config.ini in %s (%v)", dir, last)
		}
		if set == nil {
			set = one
			set.fallbackFrom = len(set.Files)
			continue
		}
		set.Files = append(set.Files, one.Files...)
	}
	if set == nil {
		return nil, fmt.Errorf("pak: no client folder given")
	}
	return set, nil
}

// FallbackReport tells how many distinct files were served by fallback archives and lists
// up to max of them.
func (s *Set) FallbackReport(max int) (int, []string) {
	names := make([]string, 0, len(s.FallbackHits))
	for p := range s.FallbackHits {
		names = append(names, p)
	}
	sort.Strings(names)
	n := len(names)
	if len(names) > max {
		names = names[:max]
	}
	return n, names
}

// OpenSet reads a package.ini ([Package] Path=..., 0=a.pak, 1=b.pak ...) relative to its folder.
// Only the [Package] section counts (config.ini of VLTK 2.0 carries other sections too).
func OpenSet(iniPath string) (*Set, error) {
	data, err := os.ReadFile(iniPath)
	if err != nil {
		return nil, err
	}
	dir := ""
	var names []string
	section := ""
	for _, line := range strings.Split(string(data), "\n") {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "[") {
			section = strings.ToLower(line)
			continue
		}
		if section != "" && section != "[package]" {
			continue
		}
		k, v, ok := strings.Cut(line, "=")
		if !ok {
			continue
		}
		k = strings.TrimSpace(k)
		v = strings.TrimSpace(v)
		if strings.EqualFold(k, "Path") {
			dir = strings.Trim(strings.ReplaceAll(v, "\\", "/"), "/")
			continue
		}
		if _, err := fmt.Sscanf(k, "%d", new(int)); err == nil && v != "" {
			names = append(names, v)
		}
	}
	base := iniPath[:strings.LastIndexAny(iniPath, `\/`)+1]
	s := &Set{}
	for _, n := range names {
		f, err := Open(base + dir + "/" + n)
		if err != nil {
			continue // missing update paks are normal
		}
		s.Files = append(s.Files, f)
	}
	if len(s.Files) == 0 {
		return nil, fmt.Errorf("pak: no archives found via %s", iniPath)
	}
	return s, nil
}

// Add appends archives (lowest priority last).
func (s *Set) Add(f *File) { s.Files = append(s.Files, f) }

// Close closes every archive.
func (s *Set) Close() {
	for _, f := range s.Files {
		f.Close()
	}
}

// Find returns the entry and the archive holding it.
func (s *Set) Find(id uint32) (*File, Entry, bool) {
	for _, f := range s.Files {
		if e, ok := f.Find(id); ok {
			return f, e, true
		}
	}
	return nil, Entry{}, false
}

// Lookup finds by game path (fallback hits are recorded, see OpenClientSet).
func (s *Set) Lookup(path string) (*File, Entry, bool) {
	id := FileNameToID(path)
	for i, f := range s.Files {
		if e, ok := f.Find(id); ok {
			if s.fallbackFrom > 0 && i >= s.fallbackFrom {
				if s.FallbackHits == nil {
					s.FallbackHits = map[string]int{}
				}
				s.FallbackHits[NormalizePath(path)]++
			}
			return f, e, true
		}
	}
	return nil, Entry{}, false
}

// ReadFile returns the uncompressed content of a plain file by game path.
func (s *Set) ReadFile(path string) ([]byte, error) {
	f, e, ok := s.Lookup(path)
	if !ok {
		return nil, fmt.Errorf("%w: %s", ErrNotFound, path)
	}
	return f.Read(e)
}

// FileNameToID is KPakList::FileNameToId applied to the normalised pack path: leading
// separators dropped, one backslash prepended, '/' -> '\', ASCII lower-cased; bytes are hashed
// as *signed* chars exactly like the C code, so GBK/TCVN3 bytes above 0x7f count as negative.
func FileNameToID(path string) uint32 {
	norm := NormalizePath(path)
	var id uint32
	for i := 0; i < len(norm); i++ {
		c := int32(int8(norm[i]))
		if c >= 'A' && c <= 'Z' {
			c += 'a' - 'A'
		}
		id = ((id + uint32(int32(i+1)*c)) % 0x8000000b) * 0xffffffef
	}
	return id ^ 0x12345678
}

// NormalizePath mirrors g_GetPackPath: strip leading separators, remove "." and ".." parts,
// lower-case ASCII, then prefix a single backslash (as KPakList::FindElemFile does).
func NormalizePath(path string) string {
	p := strings.ReplaceAll(path, "/", `\`)
	p = strings.TrimLeft(p, `\`)
	parts := strings.Split(p, `\`)
	var out []string
	for _, part := range parts {
		switch part {
		case "", ".":
			continue
		case "..":
			if len(out) > 0 {
				out = out[:len(out)-1]
			}
		default:
			out = append(out, part)
		}
	}
	b := []byte(`\` + strings.Join(out, `\`))
	for i, c := range b {
		if c >= 'A' && c <= 'Z' {
			b[i] = c + 'a' - 'A'
		}
	}
	return string(b)
}

// Inflate is zlib for the "zip" sprite variant.
func Inflate(src []byte, size int) ([]byte, error) {
	r, err := zlib.NewReader(bytes.NewReader(src))
	if err != nil {
		return nil, err
	}
	defer r.Close()
	out := make([]byte, size)
	if _, err := io.ReadFull(r, out); err != nil {
		return nil, err
	}
	return out, nil
}

// SortedIDs returns the ids of an archive in ascending order (debugging aid).
func (p *File) SortedIDs() []uint32 {
	ids := make([]uint32, len(p.entries))
	for i, e := range p.entries {
		ids[i] = e.ID
	}
	sort.Slice(ids, func(i, j int) bool { return ids[i] < ids[j] })
	return ids
}
