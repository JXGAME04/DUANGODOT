// Package export converts an old map (wor + regions + sprites) into the JX NEXT map bundle:
//
//	<out>/maps/<id>/map.json          size, rect, region list, spawn, npcs     (zone + client)
//	<out>/maps/<id>/obstacle.bin      1 byte per 32x32 cell, row major          (zone + client)
//	<out>/maps/<id>/r<XXX>_<YYY>.json ground tiles / cover / buildin objects    (client)
//	<out>/sprites/<id8>.png + .json   one atlas per distinct sprite             (client)
//
// Coordinates in the bundle are *screen* pixels of the old renderer: x = scene x,
// y = scene y / 2.  The zone simulates in scene units (see docs/MAPS.md).
package export

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/spr"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/wor"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// MapInfo is map.json.
type MapInfo struct {
	ID         int       `json:"id"`
	Name       string    `json:"name"`
	Source     string    `json:"source"` // old game path (UTF-8)
	RegionLeft int       `json:"region_left"`
	RegionTop  int       `json:"region_top"`
	RegionCols int       `json:"region_cols"`
	RegionRows int       `json:"region_rows"`
	RegionW    int       `json:"region_w"` // scene units
	RegionH    int       `json:"region_h"`
	CellSize   int       `json:"cell_size"`
	CellsX     int       `json:"cells_x"` // obstacle grid size (whole map)
	CellsY     int       `json:"cells_y"`
	SceneW     int       `json:"scene_w"` // scene units
	SceneH     int       `json:"scene_h"`
	Spawn      [2]int    `json:"spawn"` // scene units
	Indoor     bool      `json:"indoor"`
	Regions    []string  `json:"regions"` // "XXX_YYY" of regions that have data
	Npcs       []NpcInfo `json:"npcs"`
	Sprites    int       `json:"sprites"`
}

// NpcInfo is a static npc placement.
type NpcInfo struct {
	TemplateID int    `json:"template_id"`
	Name       string `json:"name"`
	X          int    `json:"x"` // scene units
	Y          int    `json:"y"`
	Frame      int    `json:"frame"`
	Kind       int    `json:"kind"`
	Script     string `json:"script,omitempty"`
}

// Tile is one ground tile in screen pixels relative to the region origin.
type Tile struct {
	X      int    `json:"x"`
	Y      int    `json:"y"`
	Sprite string `json:"s"` // sprite id (file base name under sprites/)
	Frame  int    `json:"f"`
}

// Object is a cover or buildin object; Y sorting uses SortY (screen px, absolute).
type Object struct {
	X      int    `json:"x"` // screen px, absolute
	Y      int    `json:"y"`
	SortY  int    `json:"sy"`
	Sprite string `json:"s"`
	Frame  int    `json:"f"`
	Frames int    `json:"n,omitempty"` // animated when > 1
	Layer  string `json:"l"`           // "cover" | "object" | "above"
}

// RegionFile is r<XXX>_<YYY>.json.
type RegionFile struct {
	X       int      `json:"x"`
	Y       int      `json:"y"`
	OriginX int      `json:"origin_x"` // screen px of the region's top-left
	OriginY int      `json:"origin_y"`
	Tiles   []Tile   `json:"tiles"`
	Objects []Object `json:"objects"`
}

// Exporter carries the archive set and the sprite cache between regions.
type Exporter struct {
	Set      *pak.Set
	Out      string
	sprites  map[string]string // game path -> sprite id
	failed   map[string]bool
	Exported int
}

func New(set *pak.Set, out string) *Exporter {
	return &Exporter{Set: set, Out: out, sprites: map[string]string{}, failed: map[string]bool{}}
}

// spriteID exports a sprite once and returns its id ("" when it cannot be decoded).
func (e *Exporter) spriteID(gamePath string) string {
	if id, ok := e.sprites[gamePath]; ok {
		return id
	}
	if e.failed[gamePath] {
		return ""
	}
	f, entry, ok := e.Set.Lookup(gamePath)
	if !ok {
		log.Warn("asset", "sprite missing", log.F("path", text.GBKToUTF8([]byte(gamePath))))
		e.failed[gamePath] = true
		return ""
	}
	id := fmt.Sprintf("%08x", entry.ID)
	base := filepath.Join(e.Out, "sprites", id)
	if _, err := os.Stat(base + ".json"); err == nil {
		e.sprites[gamePath] = id
		return id
	}
	s, err := spr.ReadFromPak(f, entry)
	if err != nil {
		log.Warn("asset", "sprite decode failed", log.F("path", text.GBKToUTF8([]byte(gamePath))), log.F("error", err))
		e.failed[gamePath] = true
		return ""
	}
	if err := s.WriteAtlas(base, text.GBKToUTF8([]byte(gamePath))); err != nil {
		log.Error("asset", "sprite write failed", log.F("path", base), log.F("error", err))
		e.failed[gamePath] = true
		return ""
	}
	e.sprites[gamePath] = id
	e.Exported++
	return id
}

// Map exports one world.  spawn is in scene units (from MapList.ini or the first walkable cell).
func (e *Exporter) Map(id int, name string, w *wor.World, spawn [2]int) (*MapInfo, error) {
	dir := filepath.Join(e.Out, "maps", fmt.Sprintf("%d", id))
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return nil, err
	}
	if err := os.MkdirAll(filepath.Join(e.Out, "sprites"), 0o755); err != nil {
		return nil, err
	}
	// generated data: the Godot editor must not import it (loaded at run time instead)
	if err := os.WriteFile(filepath.Join(e.Out, ".gdignore"), []byte{}, 0o644); err != nil {
		return nil, err
	}
	info := &MapInfo{
		ID: id, Name: name, Source: text.GBKToUTF8([]byte(w.Path)),
		RegionLeft: w.Left, RegionTop: w.Top, RegionCols: w.RegionCols(), RegionRows: w.RegionRows(),
		RegionW: wor.RegionWidth, RegionH: wor.RegionHeight, CellSize: wor.CellSize,
		CellsX: w.RegionCols() * wor.CellsX, CellsY: w.RegionRows() * wor.CellsY,
		SceneW: w.RegionCols() * wor.RegionWidth, SceneH: w.RegionRows() * wor.RegionHeight,
		Spawn: spawn, Indoor: w.IsInDoor,
	}
	obstacle := make([]byte, info.CellsX*info.CellsY)
	for ry := w.Top; ry <= w.Bottom; ry++ {
		for rx := w.Left; rx <= w.Right; rx++ {
			r, err := wor.LoadRegion(e.Set, w, rx, ry)
			if err != nil {
				return nil, fmt.Errorf("region %d,%d: %w", rx, ry, err)
			}
			if !r.HasData {
				continue
			}
			key := fmt.Sprintf("%03d_%03d", rx, ry)
			info.Regions = append(info.Regions, key)
			// obstacle grid of the whole map
			for cx := 0; cx < wor.CellsX; cx++ {
				for cy := 0; cy < wor.CellsY; cy++ {
					if v := r.Obstacle[cx][cy]; v != 0 {
						gx := (rx-w.Left)*wor.CellsX + cx
						gy := (ry-w.Top)*wor.CellsY + cy
						obstacle[gy*info.CellsX+gx] = byte(v & 0xff)
					}
				}
			}
			rf := e.region(w, r)
			data, err := json.Marshal(rf)
			if err != nil {
				return nil, err
			}
			if err := os.WriteFile(filepath.Join(dir, "r"+key+".json"), data, 0o644); err != nil {
				return nil, err
			}
			for _, n := range r.Npcs {
				info.Npcs = append(info.Npcs, NpcInfo{
					TemplateID: int(n.TemplateID), Name: text.TCVN3ToUTF8([]byte(n.Name)),
					X: (rx-w.Left)*wor.RegionWidth + int(n.X), Y: (ry-w.Top)*wor.RegionHeight + int(n.Y),
					Frame: n.Frame, Kind: n.Kind, Script: text.GBKToUTF8([]byte(n.Script)),
				})
			}
		}
	}
	if info.Spawn == [2]int{0, 0} {
		info.Spawn = firstWalkable(obstacle, info.CellsX, info.CellsY)
	}
	info.Sprites = len(e.sprites)
	if err := os.WriteFile(filepath.Join(dir, "obstacle.bin"), obstacle, 0o644); err != nil {
		return nil, err
	}
	data, err := json.MarshalIndent(info, "", " ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(dir, "map.json"), data, 0o644); err != nil {
		return nil, err
	}
	return info, nil
}

func (e *Exporter) region(w *wor.World, r *wor.Region) *RegionFile {
	originX := (r.X - w.Left) * wor.RegionWidth
	originY := (r.Y - w.Top) * wor.RegionHeight / 2
	rf := &RegionFile{X: r.X, Y: r.Y, OriginX: originX, OriginY: originY, Tiles: []Tile{}, Objects: []Object{}}
	for _, g := range r.Ground {
		id := e.spriteID(g.Image)
		if id == "" {
			continue
		}
		rf.Tiles = append(rf.Tiles, Tile{X: g.H * wor.GroundCellW, Y: g.V * wor.GroundCellH, Sprite: id, Frame: g.Frame})
	}
	// cover/buildin positions are absolute scene coordinates counted from region index 0,
	// the bundle counts from the map's first region (Left, Top)
	baseX := w.Left * wor.RegionWidth
	baseY := w.Top * wor.RegionHeight / 2 // screen px
	for _, c := range r.Covers {
		id := e.spriteID(c.Image)
		if id == "" {
			continue
		}
		rf.Objects = append(rf.Objects, Object{X: c.X - baseX, Y: c.Y/2 - baseY, SortY: c.Y/2 - baseY, Sprite: id, Frame: c.Frame, Layer: "cover"})
	}
	for _, b := range r.Buildins {
		id := e.spriteID(b.Image)
		if id == "" {
			continue
		}
		// ImgPos1 is the top-left corner of the image in scene space; KRepresentShell2::
		// CoordinateTransform projects (x, y, z) to screen as (x, y/2 - z*887/1024).
		// Objects sort with characters by their base line (oPos1.y).
		sx := int(b.Pos[0][0]) - baseX
		sy := int(b.Pos[0][1])/2 - (int(b.Pos[0][2])*887)>>10 - baseY
		sortY := int(b.OPos[0][1])/2 - baseY
		if b.OPos[0][1] == 0 {
			sortY = int(b.Pos[3][1])/2 - baseY
		}
		layer := "object"
		if b.Order != 0 && b.Order != 0xFFFF {
			layer = "above"
		}
		// Big buildings are cut into slices: every slice is one *frame* of the same sprite placed
		// separately so y-sorting works per slice.  Only nAniSpeed > 0 means "animated"
		// (KScenePlaceRegionC::LoadAboveGroundObjects); everything else must keep its frame.
		frames := 0
		if b.AniSpeed > 0 && b.NumFrames > 1 {
			frames = b.NumFrames
		}
		rf.Objects = append(rf.Objects, Object{X: sx, Y: sy, SortY: sortY, Sprite: id, Frame: b.Frame, Frames: frames, Layer: layer})
	}
	sort.SliceStable(rf.Objects, func(i, j int) bool { return rf.Objects[i].SortY < rf.Objects[j].SortY })
	return rf
}

func firstWalkable(obstacle []byte, cellsX, cellsY int) [2]int {
	// pick the walkable cell closest to the centre
	cx, cy := cellsX/2, cellsY/2
	best, bestD := [2]int{cx * wor.CellSize, cy * wor.CellSize}, -1
	for y := 0; y < cellsY; y++ {
		for x := 0; x < cellsX; x++ {
			if obstacle[y*cellsX+x] != 0 {
				continue
			}
			d := (x-cx)*(x-cx) + (y-cy)*(y-cy)
			if bestD < 0 || d < bestD {
				bestD = d
				best = [2]int{x*wor.CellSize + wor.CellSize/2, y*wor.CellSize + wor.CellSize/2}
			}
		}
	}
	return best
}
