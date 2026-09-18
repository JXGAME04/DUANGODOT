package export

// The pictures of items.  Every row of an item table names its sprite (the 动画文件名 column,
// `\spr\item\...spr`): the bag draws the sprite in the item's cells (KWndObjContainer::OnDraw of the
// old client through KUiBase::GetObjImage), the ground draws the object of ObjData.txt instead.
// The sprites are written the way the window pictures are (KUiExport.go): one atlas .png per
// sprite plus, in items/images.json, the box and where each frame is - so the Godot client draws
// an item icon with the same KUiImage it draws a button with.
//
// The file of a picture mirrors the game path below items/images/, lower-cased, so the 2 305
// pictures of the shipped tables keep readable names (item/equip/armor/obj-ma-cloth11-3.png).

import (
	"encoding/json"
	"fmt"
	"image/png"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// ItemImageIndex is items/images.json: the game path of a picture (as the tables spell it,
// UTF-8) -> the picture written.
type ItemImageIndex struct {
	Images map[string]*UiImage `json:"images"`
	// game paths the client had no sprite for: a bag shows the name in the cells instead
	Missing []string `json:"missing,omitempty"`
}

// ItemImages writes the sprite of every game path in `paths` under <out>/items/images and the
// index next to them.  Returns how many were written and how many the client does not have.
func (e *Exporter) ItemImages(paths []string) (written, missing int, err error) {
	dir := filepath.Join(e.Out, "items", "images")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return 0, 0, err
	}
	index := ItemImageIndex{Images: map[string]*UiImage{}}
	seen := map[string]bool{}
	for _, shown := range paths {
		if shown == "" || seen[shown] {
			continue
		}
		seen[shown] = true
		gbk, err := text.UTF8ToGBK(shown)
		if err != nil {
			index.Missing = append(index.Missing, shown)
			continue
		}
		s := e.uiSprite(string(gbk))
		if s == nil || len(s.Frames) == 0 {
			index.Missing = append(index.Missing, shown)
			continue
		}
		rel := itemImageFile(shown)
		full := filepath.Join(dir, filepath.FromSlash(rel))
		if err := os.MkdirAll(filepath.Dir(full), 0o755); err != nil {
			return written, missing, err
		}
		atlas, meta := s.PackAtlas(shown)
		out, err := os.Create(full)
		if err == nil {
			err = png.Encode(out, atlas)
			if cerr := out.Close(); err == nil {
				err = cerr
			}
		}
		if err != nil {
			return written, missing, fmt.Errorf("%s: %w", rel, err)
		}
		index.Images[shown] = &UiImage{GamePath: shown, File: "images/" + rel, Width: s.Width, Height: s.Height,
			Interval: s.Interval, Frames: meta.Frames}
		written++
	}
	sort.Strings(index.Missing)
	missing = len(index.Missing)
	blob, err := json.MarshalIndent(index, "", " ")
	if err != nil {
		return written, missing, err
	}
	if err := os.WriteFile(filepath.Join(e.Out, "items", "images.json"), blob, 0o644); err != nil {
		return written, missing, err
	}
	log.Info("asset", "item images exported", log.F("written", written), log.F("missing", missing), log.F("dir", dir))
	return written, missing, nil
}

// itemImageFile turns `\Spr\item\equip\armor\obj-ma-cloth11-3.spr` into
// `item/equip/armor/obj-ma-cloth11-3.png`: the game path below \spr, lower-cased, forward slashes.
func itemImageFile(gamePath string) string {
	p := strings.ToLower(strings.ReplaceAll(gamePath, `\`, "/"))
	p = strings.TrimPrefix(p, "/")
	p = strings.TrimPrefix(p, "spr/")
	p = strings.TrimSuffix(p, ".spr")
	// what is left must be a clean relative path: no "..", no drive, no empty parts
	parts := strings.Split(p, "/")
	clean := parts[:0]
	for _, part := range parts {
		part = strings.Map(func(r rune) rune {
			switch {
			case r >= 'a' && r <= 'z', r >= '0' && r <= '9', r == '-', r == '_', r == '.':
				return r
			}
			return '_'
		}, part)
		if part == "" || part == "." || part == ".." {
			continue
		}
		clean = append(clean, part)
	}
	if len(clean) == 0 {
		clean = []string{"khong-ten"}
	}
	return strings.Join(clean, "/") + ".png"
}
