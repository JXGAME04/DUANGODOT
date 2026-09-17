package font

import (
	"encoding/binary"
	"testing"
)

// build makes an ASF file: `glyphs` maps a slot to its run bytes.
func build(w, h, count int, glyphs map[int][]byte) []byte {
	data := []byte{0} // offset 0 means "no glyph", so nothing may start there
	offsets := make([]uint32, count)
	for slot := 0; slot < count; slot++ {
		if runs, ok := glyphs[slot]; ok {
			offsets[slot] = uint32(len(data))
			data = append(data, runs...)
		}
	}
	out := []byte{'A', 'S', 'F', 0}
	out = binary.LittleEndian.AppendUint32(out, uint32(len(data)))
	out = binary.LittleEndian.AppendUint32(out, uint32(count))
	out = binary.LittleEndian.AppendUint16(out, uint16(w))
	out = binary.LittleEndian.AppendUint16(out, uint16(h))
	for _, o := range offsets {
		out = binary.LittleEndian.AppendUint32(out, o)
	}
	return append(out, data...)
}

func run(level, n int) byte { return byte(level<<5 | n) }

func TestGlyphRunsAreUnpackedTheWayTheOldBlitterReadThem(t *testing.T) {
	// a 4x3 cell: one outline pixel, two letter pixels, then a run of 5 empties that goes on into
	// the next row (g_DrawFontWithBorder carries what is left of a run over the line end)
	raw := build(4, 3, 8, map[int][]byte{
		5: {run(4, 1), run(7, 2), run(0, 5), run(7, 1), run(0, 3)},
	})
	f, err := Parse(raw)
	if err != nil {
		t.Fatal(err)
	}
	if f.Width != 4 || f.Height != 3 || f.Count != 8 {
		t.Fatalf("header %dx%d count %d", f.Width, f.Height, f.Count)
	}
	if f.Has(4) || f.Glyph(4) != nil || f.Has(-1) || f.Has(8) {
		t.Error("an empty slot or one outside the table is not a glyph")
	}
	want := []uint8{4, 7, 7, 0, 0, 0, 0, 0, 7, 0, 0, 0}
	got := f.Glyph(5)
	if len(got) != len(want) {
		t.Fatalf("glyph has %d pixels, want %d", len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Fatalf("pixel %d is level %d, want %d (%v)", i, got[i], want[i], got)
		}
	}
}

func TestBrokenFontsAreRefused(t *testing.T) {
	good := build(4, 3, 4, map[int][]byte{1: {run(7, 12)}})
	for name, raw := range map[string][]byte{
		"empty":         nil,
		"wrong magic":   append([]byte("ASG\x00"), good[4:]...),
		"cut in table":  good[:20],
		"cut in glyphs": good[:len(good)-1],
	} {
		if _, err := Parse(raw); err == nil {
			t.Errorf("%s: accepted", name)
		}
	}
	// a glyph whose runs end early is padded with nothing instead of reading past the data
	short := build(4, 3, 4, map[int][]byte{3: {run(7, 2)}})
	f, err := Parse(short)
	if err != nil {
		t.Fatal(err)
	}
	if g := f.Glyph(3); len(g) != 12 || g[0] != Letter || g[2] != Empty {
		t.Errorf("short glyph: %v", g)
	}
}

func FuzzParse(f *testing.F) {
	f.Add(build(4, 3, 4, map[int][]byte{1: {run(7, 12)}}))
	f.Add([]byte("ASF\x00\xff\xff\xff\xff\xff\xff\xff\x7f\x0e\x00\x0e\x00"))
	f.Fuzz(func(t *testing.T, raw []byte) {
		font, err := Parse(raw)
		if err != nil {
			return
		}
		for i := 0; i < font.Count && i < 600; i++ {
			if g := font.Glyph(i); g != nil && len(g) != font.Width*font.Height {
				t.Fatalf("glyph %d has %d pixels for a %dx%d cell", i, len(g), font.Width, font.Height)
			}
		}
	})
}
