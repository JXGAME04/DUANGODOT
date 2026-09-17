// Package wor reads the old map format: <map>.wor (INI) plus one Region_C.dat per 512x1024
// region under <map>\v_YYY\XXX_Region_C.dat (KScenePlaceRegionC / KRegion in the old core).
//
// Scene coordinates: x in pixels, y in "scene units" where screen_y = y / 2 (the old renderer
// squashes the map vertically by two).  Obstacle and trap grids are 16 x 32 cells of 32 x 32
// scene units per region.  Ground tiles sit on a 16 x 16 grid of 32 x 32 *screen* pixels.
package wor

import (
	"encoding/binary"
	"errors"
	"fmt"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
)

const (
	RegionWidth     = 512  // scene units == screen px
	RegionHeight    = 1024 // scene units (512 screen px)
	CellsX          = 16
	CellsY          = 32
	CellSize        = 32
	GroundCellsX    = 16
	GroundCellsY    = 16
	GroundCellW     = RegionWidth / GroundCellsX      // 32 px
	GroundCellH     = RegionHeight / 2 / GroundCellsY // 32 px
	resourceNameLen = 128
	sectionObstacle = 0
	sectionTrap     = 1
	sectionNpc      = 2
	sectionObj      = 3
	sectionGround   = 4
	sectionBuildin  = 5
	sectionCount    = 6
	obstacleBytes   = CellsX * CellsY * 4
)

// World is the parsed .wor file.
type World struct {
	Path                     string // game path without extension, e.g. `\maps\西北南区\凤翔` (GBK bytes)
	Left, Top, Right, Bottom int    // region index rectangle (inclusive)
	IsInDoor                 bool
	Ini                      map[string]map[string]string
}

func (w *World) RegionCols() int { return w.Right - w.Left + 1 }
func (w *World) RegionRows() int { return w.Bottom - w.Top + 1 }

// RegionPath returns the Region_C.dat game path for region (x, y).
func (w *World) RegionPath(x, y int) string {
	return fmt.Sprintf("%s\\v_%03d\\%03d_Region_C.dat", w.Path, y, x)
}

// ParseWorld parses the INI text of a .wor file.
func ParseWorld(path string, data []byte) (*World, error) {
	w := &World{Path: path, Ini: map[string]map[string]string{}}
	section := ""
	for _, line := range strings.Split(string(data), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || line[0] == ';' {
			continue
		}
		if line[0] == '[' && strings.HasSuffix(line, "]") {
			section = strings.ToUpper(line[1 : len(line)-1])
			w.Ini[section] = map[string]string{}
			continue
		}
		k, v, ok := strings.Cut(line, "=")
		if !ok || section == "" {
			continue
		}
		w.Ini[section][strings.ToLower(strings.TrimSpace(k))] = strings.TrimSpace(v)
	}
	main := w.Ini["MAIN"]
	if main == nil {
		return nil, errors.New("wor: no [MAIN] section")
	}
	parts := strings.Split(main["rect"], ",")
	if len(parts) != 4 {
		return nil, fmt.Errorf("wor: bad rect %q", main["rect"])
	}
	var vals [4]int
	for i, p := range parts {
		n, err := strconv.Atoi(strings.TrimSpace(p))
		if err != nil {
			return nil, fmt.Errorf("wor: bad rect %q", main["rect"])
		}
		vals[i] = n
	}
	w.Left, w.Top, w.Right, w.Bottom = vals[0], vals[1], vals[2], vals[3]
	w.IsInDoor = main["isindoor"] == "1"
	if w.RegionCols() <= 0 || w.RegionRows() <= 0 {
		return nil, fmt.Errorf("wor: empty rect %q", main["rect"])
	}
	return w, nil
}

// LoadWorld reads and parses \<path>.wor from the archives.
func LoadWorld(set *pak.Set, mapPath string) (*World, error) {
	data, err := set.ReadFile(mapPath + ".wor")
	if err != nil {
		return nil, err
	}
	return ParseWorld(mapPath, data)
}

// GroundNode is one ground tile: frame nFrame of sprite Image drawn at ground cell (H, V).
type GroundNode struct {
	H, V  int
	Frame int
	Image string // game path (GBK bytes)
}

// CoverObject lies on the ground above the tiles (roads, carpets ...).
type CoverObject struct {
	X, Y          int // scene coordinates
	Image         string
	Width, Height int
	Frame         int
	Order         int
	Layer         int
}

// BuildinObject is a building/tree/etc. sorted with the characters (above-ground object).
type BuildinObject struct {
	Props         uint32
	Pos           [4][3]int32 // ImgPos1..4 (x, y, z)
	Width, Height int
	Image         string
	FlipTime      uint32
	Frame         int
	NumFrames     int
	AniSpeed      int
	Order         int
	OPos          [2][3]int32
	AngleXY       float32
	NodicalY      float32
}

// Trap is a script trigger covering NumCell cells starting at (X, Y).
type Trap struct {
	X, Y, NumCell int
	TrapID        uint32
}

// Npc is a client-side npc placement (Npc_C.dat).
type Npc struct {
	TemplateID int32
	X, Y       int32
	Special    bool
	Name       string // TCVN3/GBK bytes as stored
	Level      int
	Frame      int
	Kind       int
	Camp       int
	Series     int
	Script     string
}

// Region is one parsed Region_C.dat.
type Region struct {
	X, Y     int
	Obstacle [CellsX][CellsY]int32 // 0 = walkable, otherwise Obstacle_* kinds
	Traps    []Trap
	Npcs     []Npc
	Ground   []GroundNode
	Covers   []CoverObject
	Buildins []BuildinObject
	HasData  bool
}

// ParseRegion decodes a combined Region_C.dat.
func ParseRegion(x, y int, data []byte) (*Region, error) {
	r := &Region{X: x, Y: y}
	if len(data) < 4 {
		return r, nil
	}
	n := int(binary.LittleEndian.Uint32(data[0:]))
	if n == 0 || len(data) < 4+n*8 {
		return nil, errors.New("wor: bad section table")
	}
	type sec struct{ off, length int }
	secs := make([]sec, n)
	for i := 0; i < n; i++ {
		secs[i] = sec{int(binary.LittleEndian.Uint32(data[4+i*8:])), int(binary.LittleEndian.Uint32(data[8+i*8:]))}
	}
	head := 4 + n*8
	slice := func(i int) []byte {
		if i >= n || secs[i].length == 0 {
			return nil
		}
		a, b := head+secs[i].off, head+secs[i].off+secs[i].length
		if a < 0 || b > len(data) || a > b {
			return nil
		}
		return data[a:b]
	}
	r.HasData = true
	if ob := slice(sectionObstacle); len(ob) >= obstacleBytes {
		for cx := 0; cx < CellsX; cx++ {
			for cy := 0; cy < CellsY; cy++ {
				r.Obstacle[cx][cy] = int32(binary.LittleEndian.Uint32(ob[(cx*CellsY+cy)*4:]))
			}
		}
	}
	if tr := slice(sectionTrap); len(tr) >= 12 {
		count := int(binary.LittleEndian.Uint32(tr[0:]))
		for i := 0; i < count && 12+i*8+8 <= len(tr); i++ {
			b := tr[12+i*8:]
			r.Traps = append(r.Traps, Trap{X: int(b[0]), Y: int(b[1]), NumCell: int(b[2]), TrapID: binary.LittleEndian.Uint32(b[4:])})
		}
	}
	if np := slice(sectionNpc); len(np) >= 12 {
		count := int(binary.LittleEndian.Uint32(np[0:]))
		p := 12
		for i := 0; i < count; i++ {
			// KSPNpc without the trailing script buffer: 4+4+4+1+3+32+2+2+2+2+1+1+2 = 60 bytes
			if p+60 > len(np) {
				break
			}
			b := np[p:]
			npc := Npc{
				TemplateID: int32(binary.LittleEndian.Uint32(b[0:])),
				X:          int32(binary.LittleEndian.Uint32(b[4:])),
				Y:          int32(binary.LittleEndian.Uint32(b[8:])),
				Special:    b[12] != 0,
				Name:       cstr(b[16:48]),
				Level:      int(int16(binary.LittleEndian.Uint16(b[48:]))),
				Frame:      int(int16(binary.LittleEndian.Uint16(b[50:]))),
				Kind:       int(int16(binary.LittleEndian.Uint16(b[54:]))),
				Camp:       int(b[56]),
				Series:     int(b[57]),
			}
			scriptLen := int(binary.LittleEndian.Uint16(b[58:]))
			p += 60
			if scriptLen > 0 && p+scriptLen <= len(np) {
				npc.Script = string(np[p : p+scriptLen])
				p += scriptLen
			}
			r.Npcs = append(r.Npcs, npc)
		}
	}
	if gr := slice(sectionGround); len(gr) >= 12 {
		numNodes := int(binary.LittleEndian.Uint32(gr[0:]))
		numObj := int(binary.LittleEndian.Uint32(gr[4:]))
		objOff := int(binary.LittleEndian.Uint32(gr[8:]))
		p := 12
		for i := 0; i < numNodes && p+8 <= len(gr); i++ {
			b := gr[p:]
			nameLen := int(binary.LittleEndian.Uint16(b[6:]))
			if p+8+nameLen > len(gr) {
				break
			}
			r.Ground = append(r.Ground, GroundNode{
				H:     int(binary.LittleEndian.Uint16(b[0:])),
				V:     int(binary.LittleEndian.Uint16(b[2:])),
				Frame: int(binary.LittleEndian.Uint16(b[4:])),
				Image: cstr(gr[p+8 : p+8+nameLen]),
			})
			p += 8 + nameLen
		}
		const coverSize = 4 + 4 + resourceNameLen + 2 + 2 + 2 + 1 + 1 + 2 // 146, pack(2)
		p = objOff
		for i := 0; i < numObj && p+coverSize <= len(gr); i++ {
			b := gr[p:]
			r.Covers = append(r.Covers, CoverObject{
				X:      int(int32(binary.LittleEndian.Uint32(b[0:]))),
				Y:      int(int32(binary.LittleEndian.Uint32(b[4:]))),
				Image:  cstr(b[8 : 8+resourceNameLen]),
				Width:  int(binary.LittleEndian.Uint16(b[136:])),
				Height: int(binary.LittleEndian.Uint16(b[138:])),
				Frame:  int(binary.LittleEndian.Uint16(b[140:])),
				Order:  int(b[143]),
				Layer:  int(int16(binary.LittleEndian.Uint16(b[144:]))),
			})
			p += coverSize
		}
	}
	if bi := slice(sectionBuildin); len(bi) >= 16 {
		numBios := int(binary.LittleEndian.Uint32(bi[0:]))
		const bioSize = 4 + 4*12 + 2 + 2 + resourceNameLen + 4 + 2 + 2 + 2 + 2 + 2*12 + 4 + 4 // 228
		p := 16
		for i := 0; i < numBios && p+bioSize <= len(bi); i++ {
			b := bi[p:]
			o := BuildinObject{Props: binary.LittleEndian.Uint32(b[0:])}
			for k := 0; k < 4; k++ {
				for c := 0; c < 3; c++ {
					o.Pos[k][c] = int32(binary.LittleEndian.Uint32(b[4+k*12+c*4:]))
				}
			}
			o.Width = int(int16(binary.LittleEndian.Uint16(b[52:])))
			o.Height = int(int16(binary.LittleEndian.Uint16(b[54:])))
			o.Image = cstr(b[56 : 56+resourceNameLen])
			o.FlipTime = binary.LittleEndian.Uint32(b[184:])
			o.Frame = int(binary.LittleEndian.Uint16(b[188:]))
			o.NumFrames = int(binary.LittleEndian.Uint16(b[190:]))
			o.AniSpeed = int(binary.LittleEndian.Uint16(b[192:]))
			o.Order = int(binary.LittleEndian.Uint16(b[194:]))
			for k := 0; k < 2; k++ {
				for c := 0; c < 3; c++ {
					o.OPos[k][c] = int32(binary.LittleEndian.Uint32(b[196+k*12+c*4:]))
				}
			}
			o.AngleXY = float32frombits(binary.LittleEndian.Uint32(b[220:]))
			o.NodicalY = float32frombits(binary.LittleEndian.Uint32(b[224:]))
			r.Buildins = append(r.Buildins, o)
			p += bioSize
		}
	}
	return r, nil
}

// LoadRegion reads one region; a missing file yields an empty region (maps are sparse).
func LoadRegion(set *pak.Set, w *World, x, y int) (*Region, error) {
	data, err := set.ReadFile(w.RegionPath(x, y))
	if err != nil {
		if errors.Is(err, pak.ErrNotFound) {
			return &Region{X: x, Y: y}, nil
		}
		return nil, err
	}
	return ParseRegion(x, y, data)
}

func cstr(b []byte) string {
	for i, c := range b {
		if c == 0 {
			return string(b[:i])
		}
	}
	return string(b)
}

func float32frombits(u uint32) float32 {
	return *(*float32)(unsafePointer(&u))
}
