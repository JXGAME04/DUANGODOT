package spr

import (
	"encoding/json"
	"image"
	"image/png"
	"os"
	"sort"
)

// AtlasFrame is where a frame landed in the atlas plus its placement inside the sprite box.
type AtlasFrame struct {
	X       int `json:"x"`
	Y       int `json:"y"`
	W       int `json:"w"`
	H       int `json:"h"`
	OffsetX int `json:"ox"`
	OffsetY int `json:"oy"`
}

// AtlasMeta is the JSON written next to the PNG; the Godot loader builds AtlasTextures from it.
type AtlasMeta struct {
	Source     string       `json:"source"`
	Width      int          `json:"width"`
	Height     int          `json:"height"`
	CenterX    int          `json:"center_x"`
	CenterY    int          `json:"center_y"`
	Directions int          `json:"directions"`
	Interval   int          `json:"interval"`
	Frames     []AtlasFrame `json:"frames"`
}

// PackAtlas shelf-packs every frame into one RGBA image (sorted by height, 1px padding).
func (s *Sprite) PackAtlas(source string) (*image.NRGBA, AtlasMeta) {
	meta := AtlasMeta{Source: source, Width: s.Width, Height: s.Height, CenterX: s.CenterX, CenterY: s.CenterY,
		Directions: s.Directions, Interval: s.Interval, Frames: make([]AtlasFrame, len(s.Frames))}
	order := make([]int, len(s.Frames))
	for i := range order {
		order[i] = i
	}
	sort.SliceStable(order, func(a, b int) bool { return s.Frames[order[a]].Height > s.Frames[order[b]].Height })

	total := 0
	widest := 1
	for _, f := range s.Frames {
		total += (f.Width + 1) * (f.Height + 1)
		if f.Width+1 > widest {
			widest = f.Width + 1
		}
	}
	maxW := 256
	for maxW*maxW < total*3/2 && maxW < 4096 {
		maxW *= 2
	}
	for maxW < widest { // a single frame may be wider than the area heuristic suggests
		maxW *= 2
	}
	x, y, shelf := 0, 0, 0
	height := 0
	for _, i := range order {
		f := &s.Frames[i]
		if x+f.Width+1 > maxW {
			x = 0
			y += shelf + 1
			shelf = 0
		}
		meta.Frames[i] = AtlasFrame{X: x, Y: y, W: f.Width, H: f.Height, OffsetX: f.OffsetX, OffsetY: f.OffsetY}
		x += f.Width + 1
		if f.Height > shelf {
			shelf = f.Height
		}
		if y+shelf > height {
			height = y + shelf
		}
	}
	if height == 0 {
		height = 1
	}
	img := image.NewNRGBA(image.Rect(0, 0, maxW, height))
	for i, f := range s.Frames {
		af := meta.Frames[i]
		for row := 0; row < f.Height; row++ {
			dst := img.Pix[(af.Y+row)*img.Stride+af.X*4:]
			copy(dst[:f.Width*4], f.RGBA[row*f.Width*4:(row+1)*f.Width*4])
		}
	}
	return img, meta
}

// WriteAtlas saves <base>.png and <base>.json.
func (s *Sprite) WriteAtlas(base, source string) error {
	img, meta := s.PackAtlas(source)
	pf, err := os.Create(base + ".png")
	if err != nil {
		return err
	}
	if err := png.Encode(pf, img); err != nil {
		pf.Close()
		return err
	}
	if err := pf.Close(); err != nil {
		return err
	}
	data, err := json.MarshalIndent(meta, "", " ")
	if err != nil {
		return err
	}
	return os.WriteFile(base+".json", data, 0o644)
}
