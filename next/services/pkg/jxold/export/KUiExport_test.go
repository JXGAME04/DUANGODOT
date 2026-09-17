package export

import (
	"encoding/json"
	"image/png"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/oldgame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
)

// vltk20 opens the VLTK 2.0 client, the reference of the login flow, or skips.
func vltk20(t *testing.T) *pak.Set {
	t.Helper()
	dir := oldgame.ClientVLTK20()
	if dir == "" {
		t.Skip("VLTK 2.0 client not found (JX_VLTK20_CLIENT, JX_OLD_CLIENT or config/oldgame.local.json)")
	}
	set, err := pak.OpenClientSet(dir)
	if err != nil {
		t.Fatal(err)
	}
	return set
}

func widgetOf(t *testing.T, s *UiScreen, key string) UiWidget {
	t.Helper()
	for _, w := range s.Widgets {
		if w.Key == key {
			return w
		}
	}
	t.Fatalf("%s has no [%s]", s.Name, key)
	return UiWidget{}
}

// Every window of the login flow is found under the name gamecl.exe asks for, in the theme folder
// \Ui\Setting.ini names - no guessing from section names.
func TestLoginWindowsAreReadByTheNameTheGameAsksFor(t *testing.T) {
	set := vltk20(t)
	defer set.Close()
	out := t.TempDir()
	ex := New(set, out)
	theme, w, h := ex.UiTheme("")
	if theme != `\Ui\ui3_1024` || w != 1024 || h != 768 {
		t.Fatalf("theme %s %dx%d, want \\Ui\\ui3_1024 1024x768", theme, w, h)
	}
	if small, sw, sh := ex.UiTheme("800"); small != `\Ui\ui3_800` || sw != 800 || sh != 600 {
		t.Errorf("-theme 800 gave %s %dx%d", small, sw, sh)
	}
	screens := map[string]*UiScreen{}
	for _, def := range LoginScreens {
		s, err := ex.UiByName(def, theme, w, h)
		if err != nil {
			t.Fatalf("%s (%s): %v", def.Name, def.Class, err)
		}
		screens[def.Name] = s
	}
	// most of them only exist in the nested archive; a patch archive may carry a newer copy of some
	if src := screens["dang-nhap"].Source; !strings.Contains(src, "reslst.dat") {
		t.Errorf("the login layout came from %s, expected the nested archive", src)
	}

	// KUiSelServer: the values the executable reads, untouched
	sel := screens["chon-may-chu"]
	main := widgetOf(t, sel, "main")
	for k, want := range map[string]string{"left": "113", "top": "130", "width": "574", "height": "396", "positiontype": "1", "loginbg": "Login"} {
		if got := main.Values[k]; got != want {
			t.Errorf("[Main] %s = %q, want %q", k, got, want)
		}
	}
	left := widgetOf(t, sel, "leftlist")
	if left.Values["font"] != "14" || left.Values["selbordercolor"] != "255,0,0" || left.Values["praybordercolor"] != "30,30,30" {
		t.Errorf("[LeftList] lost a value: %v", left.Values)
	}
	row := left.Images["sprimg"]
	if row == nil || len(row.Frames) != 2 {
		t.Fatalf("[LeftList] SprImg should be a two-frame row picture (normal, picked): %+v", row)
	}
	ok := widgetOf(t, sel, "login")
	if ok.Slug != "nut-xac-dinh" {
		t.Errorf("[Login] of the server window is its OK button, filed as %q", ok.Slug)
	}
	if ok.Values["up"] != "0" || ok.Values["down"] != "1" || ok.Values["overframe"] != "3" {
		t.Errorf("[Login] button frames: %v", ok.Values)
	}
	img := ok.Images["image"]
	if img == nil || len(img.Frames) < 4 {
		t.Fatalf("[Login] needs every frame of its sprite (Up, Down, OverFrame=3): %+v", img)
	}
	if img.File != "chon-may-chu/nut-xac-dinh.png" {
		t.Errorf("the OK button is filed as %q", img.File)
	}

	// a frame is cut out of the atlas and placed inside the sprite's box: both must fit
	f, err := os.Open(filepath.Join(out, "ui", filepath.FromSlash(img.File)))
	if err != nil {
		t.Fatal(err)
	}
	atlas, err := png.DecodeConfig(f)
	f.Close()
	if err != nil {
		t.Fatal(err)
	}
	for n, fr := range img.Frames {
		if fr.X+fr.W > atlas.Width || fr.Y+fr.H > atlas.Height {
			t.Errorf("frame %d (%+v) lies outside the %dx%d atlas", n, fr, atlas.Width, atlas.Height)
		}
		if fr.OffsetX+fr.W > img.Width || fr.OffsetY+fr.H > img.Height {
			t.Errorf("frame %d (%+v) does not fit the %dx%d box of the sprite", n, fr, img.Width, img.Height)
		}
	}

	// the login window: Vietnamese text decoded from TCVN3, paths from GBK
	login := screens["dang-nhap"]
	if got := widgetOf(t, login, "remembertxt").Values["text"]; got != "Nhớ tài khoản" {
		t.Errorf("[RememberTxt] Text = %q", got)
	}
	if got := widgetOf(t, login, "main").Values["image"]; !strings.Contains(got, `剑一登陆修改`) {
		t.Errorf("[Main] Image path not decoded from GBK: %q", got)
	}
	if widgetOf(t, login, "login").Slug != "nut-dang-nhap" {
		t.Error("[Login] of the login window is the login button")
	}

	// the character windows build their figures by name: 5 elements x 2 sexes x 3 views
	for _, name := range []string{"chon-nhan-vat", "tao-nhan-vat"} {
		if n := len(screens[name].Portraits); n != 30 {
			t.Errorf("%s: %d character pictures, want 30", name, n)
		}
	}
	figure := screens["tao-nhan-vat"].Portraits["metal_male_1"]
	if figure == nil || figure.File != PortraitFolder+"/vai-kim-nam-1.png" || len(figure.Frames) < 2 || figure.Interval <= 0 {
		t.Fatalf("the near figure of a Metal man should be an animation in the shared folder: %+v", figure)
	}
	if _, err := os.Stat(filepath.Join(out, "ui", PortraitFolder, "vai-kim-nam-1.png")); err != nil {
		t.Errorf("the figures are filed under Vietnamese names: %v", err)
	}
	if screens["chon-nhan-vat"].Portraits["metal_male_1"] != figure {
		t.Error("both character windows name the same figures: they are written once")
	}

	// the JSON on disk is what the client reads
	var back UiScreen
	blob, err := os.ReadFile(filepath.Join(out, "ui", "chon-may-chu", "bo-cuc.json"))
	if err != nil {
		t.Fatal(err)
	}
	if err := json.Unmarshal(blob, &back); err != nil {
		t.Fatal(err)
	}
	if back.Class != "KUiSelServer" || back.Theme != "ui3_1024" || len(back.Widgets) != len(sel.Widgets) {
		t.Errorf("bo-cuc.json does not round-trip: %s %s %d widgets", back.Class, back.Theme, len(back.Widgets))
	}
}

// The data the windows show and the fonts they write it with.
func TestLoginDataAndFonts(t *testing.T) {
	set := vltk20(t)
	defer set.Close()
	out := t.TempDir()
	ex := New(set, out)
	if n, err := ex.UiTable("tan-thu-thon", `\Settings\NativePlaceList.ini`); err != nil || n < 8 {
		t.Errorf("starting villages: %d sections, %v", n, err)
	}
	n, err := ex.UiStrings("chuoi-client", `\lang\vn\stringtable_client.txt`)
	if err != nil || n < 500 {
		t.Fatalf("client string table: %d strings, %v", n, err)
	}
	var table struct {
		Strings map[string]string `json:"strings"`
	}
	blob, _ := os.ReadFile(filepath.Join(out, "ui", "du-lieu", "chuoi-client.json"))
	if err := json.Unmarshal(blob, &table); err != nil {
		t.Fatal(err)
	}
	if got := table.Strings["G_STR_SERVERLIST_STATUS1"]; got != "(Đầy)" {
		t.Errorf("G_STR_SERVERLIST_STATUS1 = %q, want (Đầy)", got)
	}

	fonts, err := ex.UiFonts(`\Ui\ui3_1024`)
	if err != nil {
		t.Fatal(err)
	}
	bySize := map[int]UiFont{}
	for _, f := range fonts {
		bySize[f.Size] = f
	}
	for size, advance := range map[int]int{12: 6, 14: 7, 16: 8} {
		f := bySize[size]
		if f.Advance != advance || f.CellW != size || f.Glyphs < 150 {
			t.Errorf("font %d: %+v", size, f)
		}
		fnt, err := os.ReadFile(filepath.Join(out, "ui", "font", f.Letters))
		if err != nil {
			t.Fatal(err)
		}
		// 'ơ' (U+01A1 = 417) is TCVN3 byte 0xAC: the font must draw it and advance half a cell
		if !strings.Contains(string(fnt), "char id=417 ") || !strings.Contains(string(fnt), "xadvance="+strconv.Itoa(advance)) {
			t.Errorf("chu-%d.fnt lacks the letter o-horn or the old pitch", size)
		}
	}
	if bySize[13].SameAs != 12 {
		t.Errorf("font 13 shares the glyphs of 12 in the 2.0 client: %+v", bySize[13])
	}
}

// The order of the file is kept and values are read the way KIniFile did.
func TestLayoutParsing(t *testing.T) {
	body := []byte("; a comment\r\n[Main]\r\nLeft=0\r\nWidth=1024\r\nHeight=768 ;px\r\nStartPos=800,0\r\n\r\n[OK]\r\nLeft=51\r\nTrans=1\r\nLeft=52\r\n")
	if got := SectionNames(body); len(got) != 2 || got[0] != "main" || got[1] != "ok" {
		t.Fatalf("sections %v, want [main ok]", got)
	}
	if w, h := CanvasOf(body); w != 1024 || h != 768 {
		t.Errorf("canvas %dx%d, want 1024x768 (GetInteger reads the leading number)", w, h)
	}
	secs := parseIniOrdered(body)
	if secs[1].raw != "OK" || secs[1].values["left"] != "52" || len(secs[1].order) != 2 {
		t.Errorf("[OK] parsed as %+v (a repeated key keeps its place and takes the last value)", secs[1])
	}
}

// Text is TCVN3, paths are GBK, and both live in one file.
func TestDecodeValue(t *testing.T) {
	for raw, want := range map[string]string{
		"plain":                            "plain",
		"Nh\xed t\xb5i kho\xb6n":           "Nhớ tài khoản",
		"\\Spr\\Ui4\\\xb5\xc7\xc2\xbd.spr": `\Spr\Ui4\登陆.spr`,
	} {
		if got := DecodeValue(raw); got != want {
			t.Errorf("DecodeValue(%q) = %q, want %q", raw, got, want)
		}
	}
}

// Every section gets a file name a person can read, and a section that means something else in
// another window is named for what it is there.
func TestSlugsAreReadable(t *testing.T) {
	for section, want := range map[string]string{
		"ok": "nut-xac-dinh", "cancel": "nut-huy", "gold": "the-kim", "male": "nam",
		"password": "o-mat-khau", "Login_Butterfly_9": "login-butterfly-9",
	} {
		if got := uiSlug(section); got != want {
			t.Errorf("uiSlug(%q) = %q, want %q", section, got, want)
		}
	}
	if a, b := uiSlugIn("chon-may-chu", "login"), uiSlugIn("dang-nhap", "login"); a != "nut-xac-dinh" || b != "nut-dang-nhap" {
		t.Errorf("[Login] is %q when choosing a server and %q when logging in", a, b)
	}
	b := &uiBuilder{}
	if first, second := b.freeStem("nut", `\a.spr`), b.freeStem("nut", `\b.spr`); first != "nut" || second != "nut-2" {
		t.Errorf("two pictures must not share a file: %q, %q", first, second)
	}
	if again := b.freeStem("nut", `\a.spr`); again != "nut" {
		t.Errorf("the same picture keeps its name: %q", again)
	}
}
