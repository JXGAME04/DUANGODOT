// Package font reads the bitmap fonts of the old client ("ASF" files such as \font\vn\gbk_fs14.fnt).
//
// The format is the one Represent\iRepresent\Font\KFontData.cpp loads and Engine\Src\KDrawFont.cpp
// draws:
//
//	KFontHead   char Id[4] = "ASF\0" | u32 Size (bytes of glyph data) | u32 Count | u16 Width | u16 Height
//	u32 Offset[Count]      0 = no such character; otherwise an offset into the glyph data
//	u8  Data[Size]         for one glyph: Width x Height pixels, row after row, as runs
//	                       (level << 5 | length), length 1..31, a run may go on into the next row
//
// A pixel's level is 0 for nothing, 7 for the letter itself and 1..6 for the outline that is baked
// into the glyph: g_DrawFontWithBorder paints level 7 in the text colour and every other level in
// the border colour.  The Vietnamese fonts are indexed by the TCVN3 byte (GetCharacterDataVN), the
// Chinese ones by (first-0x81)*190 + (next-0x40) - (next>>7).
package font

import (
	"encoding/binary"
	"errors"
	"fmt"
)

// Letter and Outline are the two things a glyph pixel can be besides empty.
const (
	Empty   = 0
	Outline = 1 // any level 1..6
	Letter  = 7
)

// Font is one decoded ASF file.
type Font struct {
	Width, Height int // the cell every glyph is drawn in
	Count         int // slots in the offset table (512 in the 2.0 client: the 256 TCVN3 bytes, twice)
	offsets       []uint32
	data          []byte
}

var ErrNotASF = errors.New("font: not an ASF file")

// Parse reads an ASF font.
func Parse(b []byte) (*Font, error) {
	if len(b) < 16 || string(b[:3]) != "ASF" || b[3] != 0 {
		return nil, ErrNotASF
	}
	size := int(binary.LittleEndian.Uint32(b[4:]))
	count := int(binary.LittleEndian.Uint32(b[8:]))
	f := &Font{
		Width:  int(binary.LittleEndian.Uint16(b[12:])),
		Height: int(binary.LittleEndian.Uint16(b[14:])),
		Count:  count,
	}
	if count <= 0 || size <= 0 || f.Width <= 0 || f.Height <= 0 || f.Width > 256 || f.Height > 256 {
		return nil, fmt.Errorf("font: bad header (count %d, size %d, cell %dx%d)", count, size, f.Width, f.Height)
	}
	table := 16 + 4*count
	if count > (len(b)-16)/4 || table+size > len(b) {
		return nil, fmt.Errorf("font: file of %d bytes is too short for %d glyphs and %d bytes of data", len(b), count, size)
	}
	f.offsets = make([]uint32, count)
	for i := range f.offsets {
		f.offsets[i] = binary.LittleEndian.Uint32(b[16+4*i:])
	}
	f.data = b[table : table+size]
	return f, nil
}

// Has reports whether the font draws anything for this slot.
func (f *Font) Has(index int) bool {
	return index >= 0 && index < f.Count && f.offsets[index] != 0 && int(f.offsets[index]) < len(f.data)
}

// Glyph returns the cell of one character, Width*Height levels (0, 1..6 or 7), or nil when the font
// has no such character.
func (f *Font) Glyph(index int) []uint8 {
	if !f.Has(index) {
		return nil
	}
	want := f.Width * f.Height
	cell := make([]uint8, 0, want)
	for p := int(f.offsets[index]); len(cell) < want && p < len(f.data); p++ {
		level, run := f.data[p]>>5, int(f.data[p]&0x1f)
		for ; run > 0 && len(cell) < want; run-- {
			cell = append(cell, level)
		}
	}
	for len(cell) < want { // a glyph cut short by the end of the file: the rest is empty
		cell = append(cell, Empty)
	}
	return cell
}
