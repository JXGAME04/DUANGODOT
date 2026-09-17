package export

// The old client draws every window from an .ini: one section per widget with its rectangle, its
// sprite and, for an edit box, its font and colours.  KUiBase::GetSchemePath picks the folder,
// then each screen loads its own file (KUiLogin::LoadScheme reads its .ini and hands each section
// to KWndEdit/KWndButton::Init).
//
// This turns one of those files into JSON the Godot client can lay out directly, and writes every
// picture it names under a readable Vietnamese path.  Two things are deliberate:
//
//   - The screen is NOT found by file name.  A .pak keeps only the hash of each name, so the VLTK
//     2.0 client cannot be listed at all; its windows are found by what their sections are called
//     (see HasSections).  That also keeps GBK byte escapes out of this source entirely.
//   - Every picture is written as ui/<man-hinh>/<o>-<trang-thai>.png, named after the widget it
//     belongs to, not after a hash.  files[] in the JSON maps each old game path to the file it
//     became, so a picture can still be traced back to the client it came from.
//
// Coordinates stay exactly as the old client had them; the canvas is whatever the window says
// (800x600 for the JX1 scheme, 1024x768 for the 2.0 one).

import (
	"encoding/json"
	"fmt"
	"image"
	"image/png"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/spr"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// UiColor is an RGB triple as the old ini writes it ("255,252,178").
type UiColor struct {
	R int `json:"r"`
	G int `json:"g"`
	B int `json:"b"`
}

// UiWidget is one section of the file.
type UiWidget struct {
	Name   string `json:"name"` // the section name, lower-cased: "account", "ok", ...
	Slug   string `json:"slug"` // the Vietnamese name its pictures are filed under
	Left   int    `json:"left"`
	Top    int    `json:"top"`
	Width  int    `json:"width"`
	Height int    `json:"height"`

	// A button or a picture.  Each state is a file under ui/<screen>/, so the client loads a
	// picture by name and never has to know about atlases or ids.
	Picture   string `json:"picture,omitempty"`   // the resting picture
	Pressed   string `json:"pressed,omitempty"`   // Down=
	Hover     string `json:"hover,omitempty"`     // Over=
	GamePath  string `json:"game_path,omitempty"` // where it came from in the old client
	CheckBox  bool   `json:"checkbox,omitempty"`  // CheckBox=1: a two-state button
	Transback bool   `json:"trans,omitempty"`     // Trans=1: the window itself draws nothing

	// An edit box or a piece of text.
	Font        int      `json:"font,omitempty"`
	HAlign      int      `json:"halign,omitempty"`
	Type        int      `json:"type,omitempty"` // KWndEdit type: 1 = password, 2 = plain
	Password    bool     `json:"password,omitempty"`
	MultiLine   bool     `json:"multi_line,omitempty"`
	MaxLen      int      `json:"max_len,omitempty"`
	Color       *UiColor `json:"color,omitempty"`
	BorderColor *UiColor `json:"border_color,omitempty"`

	// Everything else the section said, so nothing is silently lost.
	Extra map[string]string `json:"extra,omitempty"`
}

// UiScreen is one window of the old client.
type UiScreen struct {
	Name    string     `json:"name"`   // "tao-nhan-vat", "dang-nhap", ...
	Label   string     `json:"label"`  // what to call it in Vietnamese
	Source  string     `json:"source"` // where it was found: archive and id, or a game path
	Width   int        `json:"width"`  // the canvas the coordinates belong to
	Height  int        `json:"height"`
	Widgets []UiWidget `json:"widgets"`
	// The character pictures the select/create screens build by name, keyed "<series>_<sex>_<n>":
	// KUiSelPlayer::GetRoleImageName makes "<PlayerImgPrefix>_<series>_<sex>_<n>.spr", with n = 0
	// for the small portrait and 1 / 2 for the near and far figure.
	Portraits map[string]string `json:"portraits,omitempty"`
	// Every old game path this screen used, and the file it became: the way back to the client.
	Files map[string]string `json:"files,omitempty"`
}

// known keys are read into the typed fields; the rest lands in Extra.
var uiTypedKeys = map[string]bool{
	"left": true, "top": true, "width": true, "height": true, "image": true,
	"up": true, "down": true, "over": true, "overframe": true, "checkbox": true, "trans": true,
	"font": true, "halign": true, "type": true, "password": true, "multiline": true,
	"maxlen": true, "color": true, "bordercolor": true,
}

// uiSlugs gives each section of the login screens a Vietnamese file name.  A section the table
// does not know keeps its own name, lower-cased - still readable, just not translated.
var uiSlugs = map[string]string{
	"main": "nen", "init": "nen", "newplayer": "nen", "selrole": "nen", "login": "nen-dang-nhap",
	"login2": "nen-2", "login3": "nen-3",
	"account": "tai-khoan", "password": "mat-khau", "name": "ten", "namebg": "khung-ten",
	"level": "cap-do", "ok": "nut-xac-dinh", "cancel": "nut-huy", "new": "nut-tao-moi",
	"del": "nut-xoa", "transfer": "nut-chuyen", "remember": "nut-nho-tai-khoan",
	"invisible": "nut-dang-nhap-an", "male": "nam", "female": "nu",
	"gold": "the-kim", "wood": "the-moc", "water": "the-thuy", "fire": "the-hoa", "earth": "the-tho",
	"propertyshow": "mo-ta", "propertybg": "nen-mo-ta", "player": "cho-dung",
	"playerinfobg": "khung-thong-tin", "versiontext": "phien-ban", "setting": "cai-dat",
	"healthgame": "khuyen-cao", "limit16yearsold": "gioi-han-tuoi", "lifetime": "thoi-han",
	"refuselogin": "bao-loi", "refuserole0": "bao-loi-0", "refuserole1": "bao-loi-1",
	"refuserole2": "bao-loi-2",
}

// uiSeries / uiSex are the names KUiSelPlayer::GetRoleImageName builds the file name from, in the
// order the game numbers them (series_metal .. series_earth; 0 = male).
var uiSeries = []struct{ key, vi, cn string }{
	{"metal", "kim", "金"}, {"wood", "moc", "木"}, {"water", "thuy", "水"},
	{"fire", "hoa", "火"}, {"earth", "tho", "土"},
}

var uiSex = []struct{ key, vi, cn string }{{"male", "nam", "男"}, {"female", "nu", "女"}}

func atoiOr(s string, def int) int {
	v, err := strconv.Atoi(strings.TrimSpace(s))
	if err != nil {
		return def
	}
	return v
}

// parseColor reads "255,252,178".
func parseColor(s string) *UiColor {
	parts := strings.Split(s, ",")
	if len(parts) < 3 {
		return nil
	}
	return &UiColor{R: atoiOr(parts[0], 0), G: atoiOr(parts[1], 0), B: atoiOr(parts[2], 0)}
}

func uiSlug(section string) string {
	if s, ok := uiSlugs[section]; ok {
		return s
	}
	keep := strings.Map(func(r rune) rune {
		switch {
		case r >= 'a' && r <= 'z', r >= '0' && r <= '9':
			return r
		case r >= 'A' && r <= 'Z':
			return r + 32
		case r == '_' || r == '-':
			return '-'
		}
		return -1
	}, section)
	if keep == "" {
		return "o"
	}
	return keep
}

// uiBuilder carries what one screen's export needs.
type uiBuilder struct {
	ex     *Exporter
	screen *UiScreen
	dir    string // <out>/ui/<screen>
}

// Ui exports the screen in `data`, the raw bytes of one layout .ini (still GBK).  `source` is
// written into the JSON so a reader can tell which archive and entry it came from.
func (e *Exporter) Ui(name, label, source string, data []byte) (*UiScreen, error) {
	sections := parseIniOrdered(data)
	if len(sections) == 0 {
		return nil, fmt.Errorf("ui %s: khong co section nao", name)
	}
	b := &uiBuilder{
		ex:  e,
		dir: filepath.Join(e.Out, "ui", name),
		screen: &UiScreen{
			Name: name, Label: label, Source: source,
			Width: 800, Height: 600, Files: map[string]string{},
		},
	}
	if err := os.MkdirAll(b.dir, 0o755); err != nil {
		return nil, err
	}
	for _, sec := range sections {
		w := UiWidget{
			Name:   sec.name,
			Slug:   uiSlug(sec.name),
			Left:   atoiOr(sec.values["left"], 0),
			Top:    atoiOr(sec.values["top"], 0),
			Width:  atoiOr(sec.values["width"], 0),
			Height: atoiOr(sec.values["height"], 0),
		}
		// The first section is the window itself: its size is the canvas the rest sits on.
		if len(b.screen.Widgets) == 0 && w.Width > 0 && w.Height > 0 {
			b.screen.Width, b.screen.Height = w.Width, w.Height
		}
		w.CheckBox = atoiOr(sec.values["checkbox"], 0) != 0
		w.Transback = atoiOr(sec.values["trans"], 0) != 0
		if img := sec.values["image"]; img != "" {
			b.pictures(&w, img, sec.values)
		}
		w.Font = atoiOr(sec.values["font"], 0)
		w.HAlign = atoiOr(sec.values["halign"], 0)
		w.Type = atoiOr(sec.values["type"], 0)
		w.Password = atoiOr(sec.values["password"], 0) != 0
		w.MultiLine = atoiOr(sec.values["multiline"], 0) != 0
		w.MaxLen = atoiOr(sec.values["maxlen"], 0)
		if c := sec.values["color"]; c != "" {
			w.Color = parseColor(c)
		}
		if c := sec.values["bordercolor"]; c != "" {
			w.BorderColor = parseColor(c)
		}
		for k, v := range sec.values {
			if uiTypedKeys[k] {
				continue
			}
			if w.Extra == nil {
				w.Extra = map[string]string{}
			}
			// a value may itself be a GBK game path (PlayerImgPrefix, LoginBg)
			w.Extra[k] = text.GBKToUTF8([]byte(v))
		}
		if p := sec.values["playerimgprefix"]; p != "" && b.screen.Portraits == nil {
			b.screen.Portraits = b.portraits(p)
		}
		b.screen.Widgets = append(b.screen.Widgets, w)
	}

	blob, err := json.MarshalIndent(b.screen, "", "  ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(b.dir, "bo-cuc.json"), blob, 0o644); err != nil {
		return nil, err
	}
	log.Info("asset", "ui screen exported", log.F("name", name), log.F("source", source),
		log.F("widgets", len(b.screen.Widgets)), log.F("pictures", len(b.screen.Files)),
		log.F("canvas", fmt.Sprintf("%dx%d", b.screen.Width, b.screen.Height)))
	return b.screen, nil
}

// pictures writes the states a section names.  A button gives Up / Down / Over as frame numbers
// of one sprite; a plain picture is frame 0; a .jpg backdrop is copied unchanged.
func (b *uiBuilder) pictures(w *UiWidget, gamePath string, values map[string]string) {
	w.GamePath = text.GBKToUTF8([]byte(gamePath))
	ext := strings.ToLower(filepath.Ext(gamePath))
	if ext != ".spr" && ext != "" {
		if file := b.copyRaw(w.Slug+ext, gamePath); file != "" {
			w.Picture = file
		}
		return
	}
	w.Picture = b.frame(w.Slug, gamePath, atoiOr(values["up"], 0))
	if _, ok := values["down"]; ok {
		w.Pressed = b.frame(w.Slug+"-nhan", gamePath, atoiOr(values["down"], 1))
	}
	if _, ok := values["over"]; ok {
		w.Hover = b.frame(w.Slug+"-re-chuot", gamePath, atoiOr(values["over"], 2))
	}
}

// frame writes one frame of a sprite as its own .png and returns the file name.
func (b *uiBuilder) frame(file, gamePath string, n int) string {
	s := b.ex.uiSprite(gamePath)
	if s == nil || len(s.Frames) == 0 {
		return ""
	}
	if n < 0 || n >= len(s.Frames) {
		n = 0
	}
	f := s.Frames[n]
	if f.Width <= 0 || f.Height <= 0 {
		return ""
	}
	img := &image.RGBA{Pix: f.RGBA, Stride: f.Width * 4, Rect: image.Rect(0, 0, f.Width, f.Height)}
	name := file + ".png"
	out, err := os.Create(filepath.Join(b.dir, name))
	if err != nil {
		log.Error("asset", "ui picture write failed", log.F("file", name), log.F("error", err))
		return ""
	}
	defer out.Close()
	if err := png.Encode(out, img); err != nil {
		log.Error("asset", "ui picture encode failed", log.F("file", name), log.F("error", err))
		return ""
	}
	b.ex.Exported++
	b.screen.Files[text.GBKToUTF8([]byte(gamePath))+"#"+strconv.Itoa(n)] = name
	return name
}

// copyRaw writes a picture the sprite reader does not handle (the .jpg backdrops) unchanged.
func (b *uiBuilder) copyRaw(name, gamePath string) string {
	data, err := b.ex.Set.ReadFile(gamePath)
	if err != nil {
		log.Warn("asset", "ui picture missing", log.F("path", text.GBKToUTF8([]byte(gamePath))), log.F("error", err))
		return ""
	}
	if err := os.WriteFile(filepath.Join(b.dir, name), data, 0o644); err != nil {
		log.Error("asset", "ui picture write failed", log.F("file", name), log.F("error", err))
		return ""
	}
	b.ex.Exported++
	b.screen.Files[text.GBKToUTF8([]byte(gamePath))] = name
	return name
}

// portraits writes every picture the prefix can name: five elements x two sexes x three views,
// the same set the old screen could ask for.
func (b *uiBuilder) portraits(prefix string) map[string]string {
	out := map[string]string{}
	for _, series := range uiSeries {
		for _, sex := range uiSex {
			for n := 0; n < 3; n++ {
				cn, err := text.UTF8ToGBK(series.cn + "_" + sex.cn)
				if err != nil {
					continue
				}
				path := fmt.Sprintf("%s_%s_%d.spr", prefix, cn, n)
				file := fmt.Sprintf("vai-%s-%s-%d", series.vi, sex.vi, n)
				if got := b.frame(file, path, 0); got != "" {
					out[fmt.Sprintf("%s_%s_%d", series.key, sex.key, n)] = got
				}
			}
		}
	}
	return out
}

// uiSprite decodes a sprite, caching by game path so a screen that names the same picture twice
// only pays for it once.
func (e *Exporter) uiSprite(gamePath string) *spr.Sprite {
	if e.uiCache == nil {
		e.uiCache = map[string]*spr.Sprite{}
	}
	if s, ok := e.uiCache[gamePath]; ok {
		return s
	}
	f, entry, ok := e.Set.Lookup(gamePath)
	if !ok {
		log.Warn("asset", "ui sprite missing", log.F("path", text.GBKToUTF8([]byte(gamePath))))
		e.uiCache[gamePath] = nil
		return nil
	}
	s, err := spr.ReadFromPak(f, entry)
	if err != nil {
		log.Warn("asset", "ui sprite decode failed", log.F("path", text.GBKToUTF8([]byte(gamePath))), log.F("error", err))
		e.uiCache[gamePath] = nil
		return nil
	}
	e.uiCache[gamePath] = s
	return s
}

// iniSection keeps the file order, which the old client relies on: the first section is the window
// and the ones after it are drawn in the order they appear.
type iniSection struct {
	name   string
	values map[string]string
}

func parseIniOrdered(data []byte) []iniSection {
	var out []iniSection
	var cur *iniSection
	for _, line := range strings.Split(strings.ReplaceAll(string(data), "\r\n", "\n"), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || line[0] == ';' || line[0] == '#' {
			continue
		}
		if line[0] == '[' {
			if end := strings.IndexByte(line, ']'); end > 0 {
				out = append(out, iniSection{name: strings.ToLower(strings.TrimSpace(line[1:end])), values: map[string]string{}})
				cur = &out[len(out)-1]
			}
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq <= 0 || cur == nil {
			continue
		}
		cur.values[strings.ToLower(strings.TrimSpace(line[:eq]))] = strings.TrimSpace(line[eq+1:])
	}
	return out
}

// SectionNames lists the sections of a layout, in file order: how a screen is recognised when the
// archive cannot tell us its name.
func SectionNames(data []byte) []string {
	secs := parseIniOrdered(data)
	out := make([]string, 0, len(secs))
	for _, s := range secs {
		out = append(out, s.name)
	}
	return out
}

// CanvasOf returns the window size the first section declares (0,0 when it declares none).
func CanvasOf(data []byte) (int, int) {
	secs := parseIniOrdered(data)
	if len(secs) == 0 {
		return 0, 0
	}
	return atoiOr(secs[0].values["width"], 0), atoiOr(secs[0].values["height"], 0)
}

// HasSections reports whether every name is a section of the layout: the signature that picks one
// screen out of the thousands of text files an archive holds.
func HasSections(data []byte, want []string) bool {
	have := map[string]bool{}
	for _, n := range SectionNames(data) {
		have[n] = true
	}
	for _, n := range want {
		if !have[n] {
			return false
		}
	}
	return true
}
