package export

// The old client writes every piece of text with its own bitmap fonts: KUiBase::LoadScheme reads
// [FontList] of the theme's font settings and hands each file to CreateAFont under the number the
// layouts use in Font= (Font=14 is \font\vn\gbk_fs14.fnt).  This file writes those fonts out so the
// Godot client draws the same glyphs at the same pitch - text is most of what a window shows, and
// a system font in its place is visible at a glance.
//
// A glyph carries its own outline (pkg/jxold/font): the letter is painted in the text colour, the
// outline in the border colour.  Each size therefore becomes TWO fonts with identical metrics,
//
//	ui/font/chu-14.fnt       + chu-14.png        the letters
//	ui/font/chu-14-vien.fnt  + chu-14-vien.png   their outlines
//
// in the plain-text BMFont format (AngelCode), which Godot loads with FontFile.load_bitmap_font and
// every other engine and tool understands as well.  ui/font/font.json lists the sizes.
//
// The pitch is the old one: KFont2::TextOut advances half a cell for every single-byte character
// (m_nFontHalfWidth, alternating floor and ceiling of Width/2), whatever the letter.

import (
	"encoding/json"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/font"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// UiFont describes one exported size.
type UiFont struct {
	Size     int    `json:"size"`      // the number layouts use in Font=
	GamePath string `json:"game_path"` // where it came from
	SameAs   int    `json:"same_as,omitempty"`
	Letters  string `json:"letters,omitempty"` // chu-14.fnt
	Outline  string `json:"outline,omitempty"` // chu-14-vien.fnt
	CellW    int    `json:"cell_w,omitempty"`
	CellH    int    `json:"cell_h,omitempty"`
	Advance  int    `json:"advance,omitempty"` // pixels per single-byte character
	Glyphs   int    `json:"glyphs,omitempty"`
}

// fontSettingFiles are the places the font list lives in, newest client first: the 2.0 client keeps
// it per language under the theme, JX1 has one 公共.ini per scheme.
func fontSettingFiles(theme string) []string {
	return []string{theme + `\vn\fontsetting.ini`, theme + `\fontsetting.ini`, theme + `\公共.ini`}
}

// UiFonts exports every font the theme lists.
func (e *Exporter) UiFonts(theme string) ([]UiFont, error) {
	var data []byte
	var source string
	for _, p := range fontSettingFiles(theme) {
		gbk, err := text.UTF8ToGBK(p)
		if err != nil {
			continue
		}
		if b, err := e.Set.ReadFile(string(gbk)); err == nil {
			data, source = b, p
			break
		}
	}
	if data == nil {
		return nil, fmt.Errorf("khong co danh sach font cua %s", theme)
	}
	var list map[string]string
	for _, sec := range parseIniOrdered(data) {
		if sec.name == "fontlist" {
			list = sec.values
		}
	}
	if list == nil {
		return nil, fmt.Errorf("%s khong co [FontList]", source)
	}
	dir := filepath.Join(e.Out, "ui", "font")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return nil, err
	}
	var out []UiFont
	for i := 0; i < atoiOr(list["count"], 0); i++ {
		id := atoiOr(list[strconv.Itoa(i)], 0)
		file := strings.TrimSpace(list[strconv.Itoa(i)+"_file"])
		if id <= 0 || file == "" {
			continue
		}
		f := UiFont{Size: id, GamePath: file}
		if strings.HasPrefix(file, "#") { // "#12": this size draws with the glyphs of size 12
			f.SameAs = atoiOr(file[1:], 0)
			out = append(out, f)
			continue
		}
		raw, err := e.Set.ReadFile(file)
		if err != nil {
			log.Warn("asset", "font missing", log.F("size", id), log.F("path", file))
			continue
		}
		parsed, err := font.Parse(raw)
		if err != nil {
			log.Warn("asset", "font unreadable", log.F("size", id), log.F("path", file), log.F("error", err))
			continue
		}
		stem := fmt.Sprintf("chu-%d", id)
		n, err := writeBMFont(dir, stem, id, parsed)
		if err != nil {
			return nil, err
		}
		f.Letters, f.Outline = stem+".fnt", stem+"-vien.fnt"
		f.CellW, f.CellH, f.Advance, f.Glyphs = parsed.Width, parsed.Height, parsed.Width/2, n
		out = append(out, f)
		e.Exported += 2
		log.Info("asset", "font exported", log.F("size", id), log.F("source", file),
			log.F("cell", fmt.Sprintf("%dx%d", parsed.Width, parsed.Height)), log.F("glyphs", n))
	}
	sort.Slice(out, func(a, b int) bool { return out[a].Size < out[b].Size })
	blob, err := json.MarshalIndent(map[string]any{"source": source, "fonts": out}, "", "  ")
	if err != nil {
		return nil, err
	}
	return out, os.WriteFile(filepath.Join(dir, "font.json"), blob, 0o644)
}

// writeBMFont writes the two fonts of one size and returns how many characters they hold.
func writeBMFont(dir, stem string, size int, f *font.Font) (int, error) {
	// the characters: every TCVN3 byte that means a letter and that the font draws
	type glyph struct {
		r    rune
		cell []uint8
	}
	var glyphs []glyph
	for b := 0x20; b < 0x100 && b < f.Count; b++ {
		r := text.TCVN3Rune(byte(b))
		if r == 0 {
			continue
		}
		if b == 0x20 { // the space has no glyph; it still has to advance
			glyphs = append(glyphs, glyph{r: ' '})
			continue
		}
		if cell := f.Glyph(b); cell != nil {
			glyphs = append(glyphs, glyph{r, cell})
		}
	}
	const cols = 16
	rows := (len(glyphs) + cols - 1) / cols
	sheetW, sheetH := cols*f.Width, rows*f.Height
	letters := image.NewNRGBA(image.Rect(0, 0, sheetW, sheetH))
	outline := image.NewNRGBA(image.Rect(0, 0, sheetW, sheetH))
	white := color.NRGBA{255, 255, 255, 255}
	for i, g := range glyphs {
		ox, oy := (i%cols)*f.Width, (i/cols)*f.Height
		for p, level := range g.cell {
			x, y := ox+p%f.Width, oy+p/f.Width
			switch {
			case level == font.Letter:
				letters.SetNRGBA(x, y, white)
			case level != font.Empty:
				outline.SetNRGBA(x, y, white)
			}
		}
	}
	// The line is as tall as the cell.  The base line only matters to engines that mix fonts on one
	// line; the bottom of the letters without a tail sits two pixels above the cell's edge.
	base := f.Height - 2
	for _, part := range []struct {
		suffix string
		img    *image.NRGBA
	}{{"", letters}, {"-vien", outline}} {
		name := stem + part.suffix
		out, err := os.Create(filepath.Join(dir, name+".png"))
		if err != nil {
			return 0, err
		}
		if err := png.Encode(out, part.img); err != nil {
			out.Close()
			return 0, err
		}
		out.Close()
		var sb strings.Builder
		fmt.Fprintf(&sb, "info face=\"jx-%s\" size=%d bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=0 aa=1 padding=0,0,0,0 spacing=0,0 outline=0\n", name, size)
		fmt.Fprintf(&sb, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=1 packed=0 alphaChnl=0 redChnl=0 greenChnl=0 blueChnl=0\n", f.Height, base, sheetW, sheetH)
		fmt.Fprintf(&sb, "page id=0 file=\"%s.png\"\n", name)
		fmt.Fprintf(&sb, "chars count=%d\n", len(glyphs))
		for i, g := range glyphs {
			w, h := f.Width, f.Height
			if g.cell == nil {
				w, h = 0, 0
			}
			fmt.Fprintf(&sb, "char id=%d x=%d y=%d width=%d height=%d xoffset=0 yoffset=0 xadvance=%d page=0 chnl=15\n",
				g.r, (i%cols)*f.Width, (i/cols)*f.Height, w, h, f.Width/2)
		}
		if err := os.WriteFile(filepath.Join(dir, name+".fnt"), []byte(sb.String()), 0o644); err != nil {
			return 0, err
		}
	}
	return len(glyphs), nil
}
