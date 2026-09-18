package export

// The objects that lie on the ground: \settings\obj\ObjData.txt (KObjSet::Init of the old core -
// one row per kind of object: an item on the ground, a pile of money, a box, a corpse...) and
// \settings\obj\MoneyObj.txt (which money picture a pile of a given size gets).  What the zone
// needs of a row (kind, LifeTime, Height) and what the client draws (ImageName, its drop
// animation ImageDropName) go into <out>/objdata.json; the sprites go to <out>/sprites like the
// map's own, so the client draws a dropped sword with the same atlas loader.
//
// Columns are read by name from the header, the numbers the way KTabFile::GetInteger did.

import (
	"encoding/json"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// ObjInfo is one row of ObjData.txt.
type ObjInfo struct {
	ID         int    `json:"id"`
	Name       string `json:"name"`
	Kind       string `json:"kind"`       // Item, Money, Box, Body, Prop, ...
	LifeTime   int    `json:"life_time"`  // frames the object lies there (0 = for ever)
	Height     int    `json:"height"`     // KObj::m_nHeight: how high it sits over the ground
	Layer      int    `json:"layer"`      //
	Image      string `json:"image"`      // sprite id under sprites/ ("" when the client has no such picture)
	ImagePath  string `json:"image_path"` // the game path, for a reader
	Frames     int    `json:"frames"`     // ImageTotalFrame
	Interval   int    `json:"interval"`   // ImageInterval
	CgX        int    `json:"cg_x"`       // ImageCgXpos / Ypos: the reference spot of the picture
	CgY        int    `json:"cg_y"`
	DropImage  string `json:"drop_image,omitempty"` // ImageDropName: the animation of the item landing
	DropFrames int    `json:"drop_frames,omitempty"`
	DropCgX    int    `json:"drop_cg_x,omitempty"`
	DropCgY    int    `json:"drop_cg_y,omitempty"`
	Loop       int    `json:"loop"` // LoopAnimation
}

// MoneyObj is one row of MoneyObj.txt: a pile of at most `Max` coins shows object `Obj`.
type MoneyObj struct {
	Max int `json:"max"`
	Obj int `json:"obj"`
}

type ObjBundle struct {
	Source  string              `json:"source"`
	Objects map[string]*ObjInfo `json:"objects"`
	Money   []MoneyObj          `json:"money"`
}

// ObjData writes objdata.json and the sprites of every row that has a picture the client owns.
// `only` limits the sprites exported to those ids (nil = every row); every row's data is written.
func (e *Exporter) ObjData(objData, moneyObj []byte, only map[int]bool) (*ObjBundle, error) {
	tab := npcres.ParseTab(objData)
	bundle := &ObjBundle{Source: `\settings\obj\ObjData.txt + MoneyObj.txt`, Objects: map[string]*ObjInfo{}}
	num := func(row int, col string, def int) int {
		s := strings.TrimSpace(tab.GetByName(row, col))
		if s == "" {
			return def
		}
		return npcres.Atoi(s)
	}
	for row := 2; row <= tab.Height(); row++ {
		id := num(row, "DataID", -1)
		if id < 0 {
			continue
		}
		o := &ObjInfo{ID: id, Name: text.TCVN3ToUTF8([]byte(tab.GetByName(row, "Name"))), Kind: strings.TrimSpace(tab.GetByName(row, "Kind")),
			LifeTime: num(row, "LifeTime", 0), Height: num(row, "Height", 0), Layer: num(row, "Layer", 0),
			Frames: num(row, "ImageTotalFrame", 1), Interval: num(row, "ImageInterval", 0),
			CgX: num(row, "ImageCgXpos", 0), CgY: num(row, "ImageCgYpos", 0), Loop: num(row, "LoopAnimation", 0),
			DropFrames: num(row, "ImageDropTotalFrame", 0), DropCgX: num(row, "ImageDropCgXpos", 0), DropCgY: num(row, "ImageDropCgYpos", 0)}
		img := strings.TrimSpace(tab.GetByName(row, "ImageName"))
		o.ImagePath = text.GBKToUTF8([]byte(img))
		if img != "" && (only == nil || only[id]) {
			o.Image = e.spriteID(img)
			if drop := strings.TrimSpace(tab.GetByName(row, "ImageDropName")); drop != "" {
				o.DropImage = e.spriteID(drop)
			}
		}
		bundle.Objects[strconv.Itoa(id)] = o
	}
	mt := npcres.ParseTab(moneyObj)
	for row := 2; row <= mt.Height(); row++ {
		max := npcres.Atoi(strings.TrimSpace(mt.Get(row, 1)))
		obj := npcres.Atoi(strings.TrimSpace(mt.Get(row, 2)))
		if obj > 0 {
			bundle.Money = append(bundle.Money, MoneyObj{Max: max, Obj: obj})
		}
	}
	sort.Slice(bundle.Money, func(a, b int) bool { return bundle.Money[a].Max < bundle.Money[b].Max })
	data, err := json.MarshalIndent(bundle, "", " ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(e.Out, "objdata.json"), data, 0o644); err != nil {
		return nil, err
	}
	log.Info("asset", "object data exported", log.F("objects", len(bundle.Objects)), log.F("money_rows", len(bundle.Money)), log.F("sprites", e.Exported))
	return bundle, nil
}
