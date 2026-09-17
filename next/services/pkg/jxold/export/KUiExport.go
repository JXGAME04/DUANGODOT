package export

// The old client draws every window from an .ini under \Ui\<scheme>\: one section per widget with
// its rectangle, its sprite and, for an edit box, its font and colours.  KUiBase::GetSchemePath
// picks the scheme, then each screen loads its own file (KUiLogin::LoadScheme reads 登陆.ini and
// hands each section to KWndEdit/KWndButton::Init).
//
// This turns one of those files into JSON the Godot client can lay out directly, and exports every
// sprite it names.  Coordinates stay exactly as the old client had them - the window is a fixed
// 800x600 canvas that the client scales - so a screen built from this file is the old screen, not
// an approximation of it.

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"

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
	Left   int    `json:"left"`
	Top    int    `json:"top"`
	Width  int    `json:"width"`
	Height int    `json:"height"`

	// A button or a picture: the sprite and which frame each state uses.
	Sprite     string `json:"sprite,omitempty"`      // exported atlas id, "" when it has none
	SpritePath string `json:"sprite_path,omitempty"` // the old game path, for tracing
	Image      string `json:"image,omitempty"`       // a plain .jpg/.bmp background, copied out unchanged
	Up         *int   `json:"up,omitempty"`
	Down       *int   `json:"down,omitempty"`
	Over       *int   `json:"over,omitempty"`
	OverFrame  *int   `json:"over_frame,omitempty"`

	// An edit box.
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
	Name    string     `json:"name"`   // "login", "select_role", "new_role"
	Source  string     `json:"source"` // the old game path, UTF-8
	Scheme  string     `json:"scheme"` // \Ui\Ui3
	Width   int        `json:"width"`  // the canvas the coordinates belong to (800x600)
	Height  int        `json:"height"`
	Widgets []UiWidget `json:"widgets"`
	// The character pictures the select/create screens build by name, keyed "<series>_<sex>_<n>":
	// KUiSelPlayer::GetRoleImageName makes "<PlayerImgPrefix>_<series>_<sex>_<n>.spr", with n = 0
	// for the small portrait and 1 / 2 for the near and far figure.
	Portraits map[string]string `json:"portraits,omitempty"`
}

// known keys are read into the typed fields; the rest lands in Extra.
var uiTypedKeys = map[string]bool{
	"left": true, "top": true, "width": true, "height": true, "image": true,
	"up": true, "down": true, "over": true, "overframe": true,
	"font": true, "halign": true, "type": true, "password": true, "multiline": true,
	"maxlen": true, "color": true, "bordercolor": true,
}

func atoiOr(s string, def int) int {
	v, err := strconv.Atoi(strings.TrimSpace(s))
	if err != nil {
		return def
	}
	return v
}

func intPtr(m map[string]string, key string) *int {
	s, ok := m[key]
	if !ok {
		return nil
	}
	v := atoiOr(s, 0)
	return &v
}

// parseColor reads "255,252,178".
func parseColor(s string) *UiColor {
	parts := strings.Split(s, ",")
	if len(parts) < 3 {
		return nil
	}
	return &UiColor{R: atoiOr(parts[0], 0), G: atoiOr(parts[1], 0), B: atoiOr(parts[2], 0)}
}

// Ui exports one screen.  `scheme` is the folder under \Ui (Ui3 in the JX1 client, ui3_1024 in the
// 2.0 one) and `file` the GBK file name inside it; `name` is what the client calls the screen.
func (e *Exporter) Ui(name, scheme, file string) (*UiScreen, error) {
	gamePath := `\Ui\` + scheme + `\` + file
	raw, err := e.Set.ReadFile(gamePath)
	if err != nil {
		return nil, fmt.Errorf("ui %s: %w", text.GBKToUTF8([]byte(gamePath)), err)
	}
	sections := parseIniOrdered(raw)
	if len(sections) == 0 {
		return nil, fmt.Errorf("ui %s: no sections", text.GBKToUTF8([]byte(gamePath)))
	}

	screen := &UiScreen{
		Name:   name,
		Source: text.GBKToUTF8([]byte(gamePath)),
		Scheme: `\Ui\` + scheme,
		Width:  800,
		Height: 600,
	}
	for _, sec := range sections {
		w := UiWidget{
			Name:   sec.name,
			Left:   atoiOr(sec.values["left"], 0),
			Top:    atoiOr(sec.values["top"], 0),
			Width:  atoiOr(sec.values["width"], 0),
			Height: atoiOr(sec.values["height"], 0),
		}
		// The first section is the window itself: its size is the canvas the rest sits on.
		if len(screen.Widgets) == 0 && w.Width > 0 && w.Height > 0 {
			screen.Width, screen.Height = w.Width, w.Height
		}
		if img := sec.values["image"]; img != "" {
			w.SpritePath = text.GBKToUTF8([]byte(img))
			w.Sprite, w.Image = e.uiPicture(img)
		}
		w.Up, w.Down, w.Over, w.OverFrame = intPtr(sec.values, "up"), intPtr(sec.values, "down"),
			intPtr(sec.values, "over"), intPtr(sec.values, "overframe")
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
		if p := sec.values["playerimgprefix"]; p != "" && screen.Portraits == nil {
			screen.Portraits = e.uiPortraits(p)
		}
		screen.Widgets = append(screen.Widgets, w)
	}

	dir := filepath.Join(e.Out, "ui")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return nil, err
	}
	blob, err := json.MarshalIndent(screen, "", "  ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(dir, name+".json"), blob, 0o644); err != nil {
		return nil, err
	}
	log.Info("asset", "ui screen exported", log.F("name", name), log.F("source", screen.Source),
		log.F("widgets", len(screen.Widgets)), log.F("canvas", fmt.Sprintf("%dx%d", screen.Width, screen.Height)))
	return screen, nil
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

// uiPicture exports whatever the Image= key names.  Most of them are .spr and go through the
// ordinary sprite path; the login backdrop is a plain .jpg (ImgType=1), which KWndImage hands
// straight to the image loader, so we copy it out unchanged and let Godot read it.
func (e *Exporter) uiPicture(gamePath string) (sprite, image string) {
	ext := strings.ToLower(filepath.Ext(gamePath))
	if ext == ".spr" || ext == "" {
		return e.spriteID(gamePath), ""
	}
	f, entry, ok := e.Set.Lookup(gamePath)
	if !ok {
		log.Warn("asset", "ui image missing", log.F("path", text.GBKToUTF8([]byte(gamePath))))
		return "", ""
	}
	name := fmt.Sprintf("%08x%s", entry.ID, ext)
	dir := filepath.Join(e.Out, "ui", "images")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		log.Error("asset", "ui image dir failed", log.F("path", dir), log.F("error", err))
		return "", ""
	}
	if _, err := os.Stat(filepath.Join(dir, name)); err == nil {
		return "", name // exported by an earlier screen
	}
	data, err := f.Read(entry)
	if err != nil {
		log.Warn("asset", "ui image read failed", log.F("path", text.GBKToUTF8([]byte(gamePath))), log.F("error", err))
		return "", ""
	}
	if err := os.WriteFile(filepath.Join(dir, name), data, 0o644); err != nil {
		log.Error("asset", "ui image write failed", log.F("path", name), log.F("error", err))
		return "", ""
	}
	e.Exported++
	return "", name
}

// uiSeries / uiSex are the names KUiSelPlayer::GetRoleImageName builds the file name from, in the
// order the game numbers them (series_metal .. series_earth; 0 = male).
var uiSeries = []struct{ key, cn string }{
	{"metal", "金"}, {"wood", "木"}, {"water", "水"}, {"fire", "火"}, {"earth", "土"},
}

var uiSex = []struct{ key, cn string }{{"male", "男"}, {"female", "女"}}

// uiPortraits exports every picture the prefix can name: five elements x two sexes x three
// indices, the same set the old screen can ask for.
func (e *Exporter) uiPortraits(prefix string) map[string]string {
	out := map[string]string{}
	for _, series := range uiSeries {
		for _, sex := range uiSex {
			for n := 0; n < 3; n++ {
				cn, err := text.UTF8ToGBK(series.cn + "_" + sex.cn)
				if err != nil {
					continue
				}
				path := fmt.Sprintf("%s_%s_%d.spr", prefix, cn, n)
				if id := e.spriteIDQuiet(path); id != "" {
					out[fmt.Sprintf("%s_%s_%d", series.key, sex.key, n)] = id
				}
			}
		}
	}
	return out
}
