// Package spr decodes the old JX sprite format (KSprite / SPRHEAD): 8-bit paletted frames with a
// run-length encoding of [count][alpha][count palette indices when alpha != 0], row major.
// Three storage variants exist and all end up as RGBA frames here:
//   - classic file: SPRHEAD, palette, SPROFFS table, RLE frames
//   - "zip" file (Reserved[1] & 0x100): frames are zlib streams of (index, alpha) pixel pairs
//   - frame-compressed pak entries (pak.MethodFrame): decoded by pak-aware code in frame.go
package spr

import (
	"encoding/binary"
	"errors"
	"fmt"
)

const (
	HeaderSize = 32
	magicSPR   = 0x00525053 // "SPR\0"
)

// Header is SPRHEAD.
type Header struct {
	Width, Height    int
	CenterX, CenterY int
	Frames, Colors   int
	Directions       int
	Interval         int
	Reserved         [6]uint16
}

// Frame is one decoded image.  OffsetX/Y place the frame inside the sprite box.
type Frame struct {
	Width, Height    int
	OffsetX, OffsetY int
	RGBA             []byte // Width*Height*4
}

type Sprite struct {
	Header
	Palette [][3]uint8
	Frames  []Frame
}

// ParseHeader reads SPRHEAD from the start of data.
func ParseHeader(data []byte) (Header, error) {
	var h Header
	if len(data) < HeaderSize {
		return h, errors.New("spr: short header")
	}
	if binary.LittleEndian.Uint32(data[0:])&0x00ffffff != magicSPR {
		return h, errors.New("spr: bad magic")
	}
	u := func(i int) int { return int(binary.LittleEndian.Uint16(data[i:])) }
	h.Width, h.Height = u(4), u(6)
	h.CenterX, h.CenterY = u(8), u(10)
	h.Frames, h.Colors = u(12), u(14)
	h.Directions, h.Interval = u(16), u(18)
	for i := range h.Reserved {
		h.Reserved[i] = binary.LittleEndian.Uint16(data[20+i*2:])
	}
	return h, nil
}

// IsZip reports the zlib-per-frame variant.
func (h Header) IsZip() bool { return h.Reserved[1]&0x100 != 0 }

func parsePalette(data []byte, colors int) ([][3]uint8, error) {
	if len(data) < HeaderSize+colors*3 {
		return nil, errors.New("spr: short palette")
	}
	pal := make([][3]uint8, colors)
	for i := 0; i < colors; i++ {
		pal[i] = [3]uint8{data[HeaderSize+i*3], data[HeaderSize+i*3+1], data[HeaderSize+i*3+2]}
	}
	return pal, nil
}

// Decode parses a complete classic or zip sprite file.
func Decode(data []byte) (*Sprite, error) {
	h, err := ParseHeader(data)
	if err != nil {
		return nil, err
	}
	pal, err := parsePalette(data, h.Colors)
	if err != nil {
		return nil, err
	}
	tab := HeaderSize + h.Colors*3
	if len(data) < tab+h.Frames*8 {
		return nil, errors.New("spr: short offset table")
	}
	base := tab + h.Frames*8
	s := &Sprite{Header: h, Palette: pal, Frames: make([]Frame, h.Frames)}
	for i := 0; i < h.Frames; i++ {
		off := int(binary.LittleEndian.Uint32(data[tab+i*8:]))
		length := int(binary.LittleEndian.Uint32(data[tab+i*8+4:]))
		if base+off+length > len(data) || length < 0 {
			return nil, fmt.Errorf("spr: frame %d out of range", i)
		}
		blob := data[base+off : base+off+length]
		var fr Frame
		if h.IsZip() {
			fr, err = decodeZipFrame(blob, pal)
		} else {
			fr, err = DecodeFrame(blob, pal)
		}
		if err != nil {
			return nil, fmt.Errorf("spr: frame %d: %w", i, err)
		}
		s.Frames[i] = fr
	}
	return s, nil
}

// DecodeFrame decodes one SPRFRAME (8 byte header + RLE data).
func DecodeFrame(blob []byte, pal [][3]uint8) (Frame, error) {
	if len(blob) < 8 {
		return Frame{}, errors.New("short frame header")
	}
	f := Frame{
		Width:   int(binary.LittleEndian.Uint16(blob[0:])),
		Height:  int(binary.LittleEndian.Uint16(blob[2:])),
		OffsetX: int(binary.LittleEndian.Uint16(blob[4:])),
		OffsetY: int(binary.LittleEndian.Uint16(blob[6:])),
	}
	rgba, err := DecodeRLE(blob[8:], f.Width, f.Height, pal)
	if err != nil {
		return Frame{}, err
	}
	f.RGBA = rgba
	return f, nil
}

// DecodeRLE expands the run stream into RGBA.  Runs never cross a row in well formed files but
// the decoder only cares about the total pixel count, like the original blitter.
func DecodeRLE(src []byte, w, h int, pal [][3]uint8) ([]byte, error) {
	n := w * h
	out := make([]byte, n*4)
	p, i := 0, 0
	for p < n {
		if i+2 > len(src) {
			return nil, fmt.Errorf("rle: truncated at pixel %d/%d", p, n)
		}
		count, alpha := int(src[i]), src[i+1]
		i += 2
		if count == 0 {
			return nil, fmt.Errorf("rle: zero run at byte %d", i-2)
		}
		if p+count > n {
			count = n - p
		}
		if alpha == 0 {
			p += count
			continue
		}
		if i+count > len(src) {
			return nil, fmt.Errorf("rle: truncated indices at pixel %d/%d", p, n)
		}
		for k := 0; k < count; k++ {
			idx := int(src[i+k])
			var c [3]uint8
			if idx < len(pal) {
				c = pal[idx]
			}
			o := (p + k) * 4
			out[o], out[o+1], out[o+2], out[o+3] = c[0], c[1], c[2], alpha
		}
		i += count
		p += count
	}
	return out, nil
}

// decodeZipFrame: [u32 rawSize][zlib: SPRFRAME header + (index, alpha) pairs]
func decodeZipFrame(blob []byte, pal [][3]uint8) (Frame, error) {
	if len(blob) < 4 {
		return Frame{}, errors.New("short zip frame")
	}
	size := int(binary.LittleEndian.Uint32(blob[0:]))
	raw, err := inflate(blob[4:], size)
	if err != nil {
		return Frame{}, err
	}
	if len(raw) < 8 {
		return Frame{}, errors.New("short zip frame header")
	}
	f := Frame{
		Width:   int(binary.LittleEndian.Uint16(raw[0:])),
		Height:  int(binary.LittleEndian.Uint16(raw[2:])),
		OffsetX: int(binary.LittleEndian.Uint16(raw[4:])),
		OffsetY: int(binary.LittleEndian.Uint16(raw[6:])),
	}
	n := f.Width * f.Height
	if len(raw) < 8+n*2 {
		return Frame{}, errors.New("short zip pixel data")
	}
	f.RGBA = make([]byte, n*4)
	for p := 0; p < n; p++ {
		idx, alpha := int(raw[8+p*2]), raw[8+p*2+1]
		if alpha < 8 {
			alpha = 0
		} else if alpha >= 248 {
			alpha = 255
		}
		var c [3]uint8
		if idx < len(pal) {
			c = pal[idx]
		}
		f.RGBA[p*4], f.RGBA[p*4+1], f.RGBA[p*4+2], f.RGBA[p*4+3] = c[0], c[1], c[2], alpha
	}
	return f, nil
}
