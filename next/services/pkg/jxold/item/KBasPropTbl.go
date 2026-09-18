// Package item reads the item tables of the old server (settings/item/*.txt) the way
// KLibOfBPT / KBPT_* of the old Core did (Core/Src/KBasPropTbl.cpp): every table by column
// NUMBER, exactly the columns the old LoadRecord read, so a row means what it meant.
//
// The JX2 server of D:\ServerLinux keeps several complete sets of these tables, one per item
// version (settings/item/000 .. 004) beside the plain settings/item; an item remembers the
// version it was made from.  A Set is one such folder.
//
// Strings (names, descriptions) are TCVN3 in the Vietnamese files and are decoded to UTF-8;
// image paths stay as the game wrote them (\spr\item\...).
package item

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// ITEMGENRE of KItem.h
const (
	GenreEquip       = 0
	GenreMedicine    = 1
	GenreMine        = 2
	GenreMaterials   = 3
	GenreTask        = 4
	GenreTownPortal  = 5
	GenreMagicScript = 6
	GenreBroken      = 7
)

// EQUIPDETAILTYPE of KItem.h: the twelve equipment tables, in this order (KLibOfBPT::Init)
var EquipTables = []string{
	"meleeweapon", "rangeweapon", "armor", "ring", "amulet", "boot", "belt", "helm", "cuff", "pendant", "horse", "mask",
}

// Range is KMINMAXPAIR.
type Range struct {
	Min int `json:"min"`
	Max int `json:"max"`
}

// Basic is KEQCP_BASIC: one base property of an equipment (type + range).
type Basic struct {
	Type  int   `json:"type"`
	Range Range `json:"range"`
}

// Req is KEQCP_REQ: one requirement (type + value).
type Req struct {
	Type int `json:"type"`
	Para int `json:"para"`
}

// Equipment is KBASICPROP_EQUIPMENT (+ the gold columns when the table has them): columns 1..46
// of MeleeWeapon.txt .. Mask.txt, 47..57 of GoldEquip.txt.
type Equipment struct {
	Row        int     `json:"row"` // 1-based data row = the item's index in its table (nParticularType picks it)
	Name       string  `json:"name"`
	Genre      int     `json:"genre"`
	Detail     int     `json:"detail"`
	Particular int     `json:"particular"`
	Image      string  `json:"image"`
	ObjIdx     int     `json:"obj"`
	Width      int     `json:"w"`
	Height     int     `json:"h"`
	Intro      string  `json:"intro"`
	Series     int     `json:"series"`
	Price      int     `json:"price"`
	Level      int     `json:"level"`
	Stackable  int     `json:"stackable"` // column 13 "是否叠放", not read by the old core; kept
	Basics     []Basic `json:"basics"`    // 7, only the ones with a type
	Reqs       []Req   `json:"reqs"`      // 6, only the ones with a type
	// GoldEquip.txt only
	MagicIDs    []int `json:"magic_ids,omitempty"` // 6 rows of magicattrib_ge.txt (1-based), 0 = none
	GroupID     int   `json:"group,omitempty"`     // the set the piece belongs to (所在套装)
	ExGroupID   int   `json:"ex_group,omitempty"`
	GroupSerial int   `json:"group_serial,omitempty"`
	MagicEx     []int `json:"magic_ex,omitempty"`
}

// MedAttrib is KMEDATTRIB.
type MedAttrib struct {
	Attrib int `json:"attrib"`
	Value  int `json:"value"`
	Time   int `json:"time"` // game loops
}

// Medicine is KBASICPROP_MEDICINE (potion.txt).
type Medicine struct {
	Row        int         `json:"row"`
	Name       string      `json:"name"`
	Genre      int         `json:"genre"`
	Detail     int         `json:"detail"`
	Particular int         `json:"particular"`
	Image      string      `json:"image"`
	ObjIdx     int         `json:"obj"`
	Width      int         `json:"w"`
	Height     int         `json:"h"`
	Intro      string      `json:"intro"`
	Price      int         `json:"price"`
	Level      int         `json:"level"`
	Stackable  int         `json:"stackable"` // 是否叠放 (column 13): eating takes one off the stack instead of the item
	Attribs    []MedAttrib `json:"attribs"`   // the Linux server reads the first two only
}

// Quest is KBASICPROP_QUEST (questkey.txt).  The old core read column 9 as bCanSell and 10 as
// nMaxStack; the JX2 files head column 9 "ParticularType" - the header decides (see load).
type Quest struct {
	Row        int    `json:"row"`
	Name       string `json:"name"`
	Genre      int    `json:"genre"`
	Detail     int    `json:"detail"`
	Image      string `json:"image"`
	ObjIdx     int    `json:"obj"`
	Width      int    `json:"w"`
	Height     int    `json:"h"`
	Intro      string `json:"intro"`
	Particular int    `json:"particular"`
	CanSell    int    `json:"can_sell"`
	MaxStack   int    `json:"max_stack"`
}

// TownPortal is KBASICPROP_TOWNPORTAL (townportal.txt).
type TownPortal struct {
	Row    int    `json:"row"`
	Name   string `json:"name"`
	Genre  int    `json:"genre"`
	Image  string `json:"image"`
	ObjIdx int    `json:"obj"`
	Width  int    `json:"w"`
	Height int    `json:"h"`
	Price  int    `json:"price"`
	Intro  string `json:"intro"`
}

// MagicAttrib is KMAGICATTRIB_TABFILE (magicattrib.txt): a prefix / suffix an equipment may roll.
type MagicAttrib struct {
	Row       int     `json:"row"`
	Name      string  `json:"name"`
	Pos       int     `json:"pos"`    // 1 = prefix, 0 = suffix (m_nPos)
	Class     int     `json:"class"`  // series required, -1 = any
	Level     int     `json:"level"`  // level required
	Kind      int     `json:"kind"`   // nPropKind: the attribute it changes
	Ranges    []Range `json:"ranges"` // 3 parameter ranges
	Intro     string  `json:"intro"`
	DropRates []int   `json:"drop_rates"` // per equipment detail type, columns 13.. (10 in JX1, 11 with the horse in JX2)
}

// GoldMagic is MAATTRIB_GOLDEQUIP (magicattrib_ge.txt): columns 5..11 only were read; the name
// and the intro are kept for a reader.
type GoldMagic struct {
	Row    int     `json:"row"`
	Name   string  `json:"name"`
	Kind   int     `json:"kind"`
	Ranges []Range `json:"ranges"`
	Intro  string  `json:"intro"`
}

// MagicLimit is one row of magicattrib_limit.txt of the JX2 server (jx_linux_y loader 0x0806BE00):
// a new item may roll attribute Type only with parameter k strictly inside (Min[k], Max[k]); a
// Max of -1 (the default of a missing column) turns that check off.  The 004 folder forbids
// allskill_v (139) with 0..0.
type MagicLimit struct {
	Type int    `json:"type"`
	Min  [3]int `json:"min"`
	Max  [3]int `json:"max"`
	Note string `json:"note,omitempty"`
}

// MagicRow is the compact form of a magicattrib.txt row the client keeps for its tooltip
// (items/magic.json): [kind, pos, class, level, min1, max1, rate_type0..rate_typeN].
type MagicRow []int

// MagicRows lists every prefix / suffix of the set in that compact form.
func (s *Set) MagicRows() []MagicRow {
	out := make([]MagicRow, 0, len(s.Magic))
	for _, m := range s.Magic {
		r := MagicRow{m.Kind, m.Pos, m.Class, m.Level, 0, 0}
		if len(m.Ranges) > 0 {
			r[4], r[5] = m.Ranges[0].Min, m.Ranges[0].Max
		}
		r = append(r, m.DropRates...)
		out = append(out, r)
	}
	return out
}

// SuiteActivate is KEQCP_REQ of suite_activate_count.txt: pieces needed to wake a set bonus.
type SuiteActivate struct {
	Suite int `json:"suite"`
	Count int `json:"count"`
}

// MagicScript is KBASICPROP_MASCRIPT (magicscript.txt): an item that runs a script or a skill.
type MagicScript struct {
	Row        int    `json:"row"`
	Name       string `json:"name"`
	Genre      int    `json:"genre"`
	Detail     int    `json:"detail"`
	Particular int    `json:"particular"`
	Image      string `json:"image"`
	ObjIdx     int    `json:"obj"`
	Width      int    `json:"w"`
	Height     int    `json:"h"`
	Intro      string `json:"intro"`
	Price      int    `json:"price"`
	Script     string `json:"script"`
	SkillID    int    `json:"skill"`
	ShowLevel  int    `json:"show_level"`
	ShortKey   int    `json:"short_key"`
	Stackable  int    `json:"stackable"` // 是否叠放 (column 13)
	MaxStack   int    `json:"max_stack"`
	RegSeries  int    `json:"reg_series"`
}

// Set is one complete folder of item tables (KLibOfBPT).
type Set struct {
	Version    string                 `json:"version"`   // folder name: "000".."004", or "" for settings/item itself
	Equipment  map[string][]Equipment `json:"equipment"` // by table name (EquipTables)
	Gold       []Equipment            `json:"gold"`      // goldequip.txt
	Medicine   []Medicine             `json:"medicine"`
	Quest      []Quest                `json:"quest"`
	TownPortal []TownPortal           `json:"town_portal"`
	Magic      []MagicAttrib          `json:"magic"`
	GoldMagic  []GoldMagic            `json:"gold_magic"`
	Suites     []SuiteActivate        `json:"suites"`
	Limits     []MagicLimit           `json:"magic_limits"`
	Scripts    []MagicScript          `json:"scripts"`
	Missing    []string               `json:"missing,omitempty"` // tables the folder does not have
}

// cell is KTabFile::GetInteger(row, col, default) of the JX2 server (jx_linux_y 0x08227E10):
// a cell that is missing OR EMPTY gives the default - KTabFile::GetValue (0x08227A00) fails on
// a zero-length cell.  The readers (KBPT_*::ReadRow through 0x081ECF40, tools/re/re_tabdesc.py)
// give every column its default: -1 for most numbers, 0 for the series and the price, 1 for the
// level - so an empty 五行属性要求 in magicattrib.txt means "any series" (-1), not Kim (0).
func cell(t *npcres.TabFile, row, col, def int) int {
	s := strings.TrimSpace(t.Get(row, col))
	if s == "" {
		return def
	}
	return atoi(s)
}

func atoi(s string) int {
	s = strings.TrimSpace(s)
	if s == "" {
		return 0
	}
	n, err := strconv.Atoi(s)
	if err != nil {
		f, ferr := strconv.ParseFloat(s, 64)
		if ferr != nil {
			return 0
		}
		return int(f)
	}
	return n
}

// vi decodes a text cell: Vietnamese TCVN3, Chinese GBK, or both in one cell (text.DecodeMixed).
func vi(s string) string {
	return text.DecodeMixed([]byte(strings.TrimSpace(s)))
}

func readTable(dir, name string) (*npcres.TabFile, string, error) {
	// the folders mix cases (MeleeWeapon.txt on Windows, meleeweapon.txt on Linux)
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil, "", err
	}
	want := strings.ToLower(name + ".txt")
	for _, e := range entries {
		if strings.ToLower(e.Name()) == want {
			p := filepath.Join(dir, e.Name())
			data, err := os.ReadFile(p)
			if err != nil {
				return nil, p, err
			}
			return npcres.ParseTab(data), p, nil
		}
	}
	return nil, filepath.Join(dir, name+".txt"), os.ErrNotExist
}

func equipmentRow(t *npcres.TabFile, row int, gold bool) Equipment {
	e := Equipment{
		Row: row - 1, Name: vi(t.Get(row, 1)), Genre: atoi(t.Get(row, 2)), Detail: atoi(t.Get(row, 3)), Particular: atoi(t.Get(row, 4)),
		Image: t.Get(row, 5), ObjIdx: atoi(t.Get(row, 6)), Width: atoi(t.Get(row, 7)), Height: atoi(t.Get(row, 8)), Intro: vi(t.Get(row, 9)),
		Series: cell(t, row, 10, 0), Price: cell(t, row, 11, 0), Level: cell(t, row, 12, 1), Stackable: cell(t, row, 13, 0),
	}
	// KBPT_Equip::ReadRow (jx_linux_y 0x081ED830): an empty attribute cell is -1 = none
	for i := 0; i < 7; i++ { // columns 14..34: type, min, max
		c := 14 + i*3
		if typ := cell(t, row, c, -1); typ > 0 {
			e.Basics = append(e.Basics, Basic{Type: typ, Range: Range{cell(t, row, c+1, -1), cell(t, row, c+2, -1)}})
		}
	}
	for i := 0; i < 6; i++ { // columns 35..46: type, value
		c := 35 + i*2
		if typ := cell(t, row, c, -1); typ > 0 {
			e.Reqs = append(e.Reqs, Req{Type: typ, Para: cell(t, row, c+1, -1)})
		}
	}
	if gold {
		for i := 0; i < 6; i++ {
			e.MagicIDs = append(e.MagicIDs, atoi(t.Get(row, 47+i)))
		}
		e.GroupID = atoi(t.Get(row, 53))
		e.ExGroupID = atoi(t.Get(row, 54))
		e.GroupSerial = atoi(t.Get(row, 55))
		if t.Width() >= 57 {
			e.MagicEx = []int{atoi(t.Get(row, 56)), atoi(t.Get(row, 57))}
		}
	}
	return e
}

// Load reads one folder of tables; version is the label the set is known by ("" for the plain
// settings/item, "000".. for a version folder).  A table that is missing is listed in Missing,
// not an error: the folders differ (settings/item has no goldequip.txt, 004 has extra ones).
func Load(dir, version string) (*Set, error) {
	if st, err := os.Stat(dir); err != nil || !st.IsDir() {
		return nil, fmt.Errorf("item: %s is not a folder", dir)
	}
	s := &Set{Version: version, Equipment: map[string][]Equipment{}}
	for _, name := range EquipTables {
		t, _, err := readTable(dir, name)
		if err != nil {
			s.Missing = append(s.Missing, name)
			continue
		}
		var rows []Equipment
		for row := 2; row <= t.Height(); row++ {
			rows = append(rows, equipmentRow(t, row, false))
		}
		s.Equipment[name] = rows
	}
	if t, _, err := readTable(dir, "goldequip"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			s.Gold = append(s.Gold, equipmentRow(t, row, true))
		}
	} else {
		s.Missing = append(s.Missing, "goldequip")
	}
	if t, _, err := readTable(dir, "potion"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			m := Medicine{Row: row - 1, Name: vi(t.Get(row, 1)), Genre: atoi(t.Get(row, 2)), Detail: atoi(t.Get(row, 3)), Particular: atoi(t.Get(row, 4)),
				Image: t.Get(row, 5), ObjIdx: atoi(t.Get(row, 6)), Width: atoi(t.Get(row, 7)), Height: atoi(t.Get(row, 8)), Intro: vi(t.Get(row, 9)),
				Price: cell(t, row, 11, 0), Level: cell(t, row, 12, 1), Stackable: cell(t, row, 13, 0)}
			// the old core read two attributes (columns 14..19; KBPT_Medicine::ReadRow 0x081ED430,
			// empty = -1); the JX2 file has five (..28): keep them all
			for c := 14; c+2 <= t.Width(); c += 3 {
				if a := cell(t, row, c, -1); a > 0 {
					m.Attribs = append(m.Attribs, MedAttrib{Attrib: a, Value: cell(t, row, c+1, -1), Time: cell(t, row, c+2, -1)})
				}
			}
			s.Medicine = append(s.Medicine, m)
		}
	} else {
		s.Missing = append(s.Missing, "potion")
	}
	if t, _, err := readTable(dir, "questkey"); err == nil {
		particularAt9 := strings.EqualFold(strings.TrimSpace(t.Get(1, 9)), "ParticularType")
		for row := 2; row <= t.Height(); row++ {
			q := Quest{Row: row - 1, Name: vi(t.Get(row, 1)), Genre: atoi(t.Get(row, 2)), Detail: atoi(t.Get(row, 3)), Image: t.Get(row, 4),
				ObjIdx: atoi(t.Get(row, 5)), Width: atoi(t.Get(row, 6)), Height: atoi(t.Get(row, 7)), Intro: vi(t.Get(row, 8))}
			if particularAt9 {
				q.Particular = atoi(t.Get(row, 9))
			} else {
				q.CanSell = atoi(t.Get(row, 9))
				q.MaxStack = atoi(t.Get(row, 10))
			}
			s.Quest = append(s.Quest, q)
		}
	} else {
		s.Missing = append(s.Missing, "questkey")
	}
	if t, _, err := readTable(dir, "townportal"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			s.TownPortal = append(s.TownPortal, TownPortal{Row: row - 1, Name: vi(t.Get(row, 1)), Genre: atoi(t.Get(row, 2)), Image: t.Get(row, 3),
				ObjIdx: atoi(t.Get(row, 4)), Width: atoi(t.Get(row, 5)), Height: atoi(t.Get(row, 6)), Price: atoi(t.Get(row, 7)), Intro: vi(t.Get(row, 8))})
		}
	} else {
		s.Missing = append(s.Missing, "townportal")
	}
	if t, _, err := readTable(dir, "magicattrib"); err == nil {
		// KBPT_MagicAttrib_TF::ReadRow (jx_linux_y 0x081EEE30): every number defaults to -1
		for row := 2; row <= t.Height(); row++ {
			m := MagicAttrib{Row: row - 1, Name: vi(t.Get(row, 1)), Pos: cell(t, row, 2, -1), Class: cell(t, row, 3, -1), Level: cell(t, row, 4, -1),
				Kind: cell(t, row, 5, -1), Intro: vi(t.Get(row, 12))}
			for i := 0; i < 3; i++ {
				m.Ranges = append(m.Ranges, Range{cell(t, row, 6+i*2, -1), cell(t, row, 7+i*2, -1)})
			}
			for c := 13; c <= t.Width(); c++ {
				m.DropRates = append(m.DropRates, cell(t, row, c, -1))
			}
			s.Magic = append(s.Magic, m)
		}
	} else {
		s.Missing = append(s.Missing, "magicattrib")
	}
	if t, _, err := readTable(dir, "magicattrib_ge"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			g := GoldMagic{Row: row - 1, Name: vi(t.Get(row, 1)), Kind: cell(t, row, 5, -1), Intro: vi(t.Get(row, 12))}
			for i := 0; i < 3; i++ {
				g.Ranges = append(g.Ranges, Range{cell(t, row, 6+i*2, -1), cell(t, row, 7+i*2, -1)})
			}
			s.GoldMagic = append(s.GoldMagic, g)
		}
	} else {
		s.Missing = append(s.Missing, "magicattrib_ge")
	}
	if t, _, err := readTable(dir, "suite_activate_count"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			s.Suites = append(s.Suites, SuiteActivate{Suite: atoi(t.Get(row, 1)), Count: atoi(t.Get(row, 2))})
		}
	} else {
		s.Missing = append(s.Missing, "suite_activate_count")
	}
	if t, _, err := readTable(dir, "magicattrib_limit"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			typ := t.GetInteger(row, 1, 0)
			if typ < 1 || typ > 0x153 { // the server's loader skips those rows
				continue
			}
			l := MagicLimit{Type: typ, Note: vi(t.Get(row, 8))}
			for k := 0; k < 3; k++ {
				l.Min[k] = t.GetInteger(row, 2+k*2, -1)
				l.Max[k] = t.GetInteger(row, 3+k*2, -1)
			}
			s.Limits = append(s.Limits, l)
		}
	} else {
		s.Missing = append(s.Missing, "magicattrib_limit")
	}
	if t, _, err := readTable(dir, "magicscript"); err == nil {
		for row := 2; row <= t.Height(); row++ {
			s.Scripts = append(s.Scripts, MagicScript{Row: row - 1, Name: vi(t.Get(row, 1)), Genre: atoi(t.Get(row, 2)), Detail: atoi(t.Get(row, 3)),
				Particular: atoi(t.Get(row, 4)), Image: t.Get(row, 5), ObjIdx: atoi(t.Get(row, 6)), Width: atoi(t.Get(row, 7)), Height: atoi(t.Get(row, 8)),
				Intro: vi(t.Get(row, 9)), Price: atoi(t.Get(row, 11)), Stackable: atoi(t.Get(row, 13)), Script: t.Get(row, 14), SkillID: atoi(t.Get(row, 15)),
				ShowLevel: atoi(t.Get(row, 18)), ShortKey: atoi(t.Get(row, 19)), MaxStack: atoi(t.Get(row, 21)), RegSeries: atoi(t.Get(row, 23))})
		}
	} else {
		s.Missing = append(s.Missing, "magicscript")
	}
	sort.Strings(s.Missing)
	return s, nil
}

func isVersion(name string) bool {
	if len(name) != 3 {
		return false
	}
	for _, c := range name {
		if c < '0' || c > '9' {
			return false
		}
	}
	return true
}

// Versions lists the version folders (000, 001, ...) under an item folder, sorted.
func Versions(dir string) []string {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil
	}
	var out []string
	for _, e := range entries {
		if e.IsDir() && isVersion(e.Name()) {
			out = append(out, e.Name())
		}
	}
	sort.Strings(out)
	return out
}

// Count is how many item rows the set holds, all tables together.
func (s *Set) Count() int {
	n := len(s.Gold) + len(s.Medicine) + len(s.Quest) + len(s.TownPortal) + len(s.Scripts)
	for _, rows := range s.Equipment {
		n += len(rows)
	}
	return n
}

// ObjIDs lists the ground object (ObjData.txt DataID, the 对应物件索引 column) every row names, each once.
func (s *Set) ObjIDs() []int {
	var out []int
	seen := map[int]bool{}
	add := func(id int) {
		if id > 0 && !seen[id] {
			seen[id] = true
			out = append(out, id)
		}
	}
	for _, name := range EquipTables {
		for _, r := range s.Equipment[name] {
			add(r.ObjIdx)
		}
	}
	for _, r := range s.Gold {
		add(r.ObjIdx)
	}
	for _, r := range s.Medicine {
		add(r.ObjIdx)
	}
	for _, r := range s.Quest {
		add(r.ObjIdx)
	}
	for _, r := range s.TownPortal {
		add(r.ObjIdx)
	}
	for _, r := range s.Scripts {
		add(r.ObjIdx)
	}
	return out
}

// ImagePaths lists the sprite every row names (the 动画文件名 column), each once, in table order.
func (s *Set) ImagePaths() []string {
	var out []string
	seen := map[string]bool{}
	add := func(p string) {
		if p != "" && !seen[p] {
			seen[p] = true
			out = append(out, p)
		}
	}
	for _, name := range EquipTables {
		for _, r := range s.Equipment[name] {
			add(r.Image)
		}
	}
	for _, r := range s.Gold {
		add(r.Image)
	}
	for _, r := range s.Medicine {
		add(r.Image)
	}
	for _, r := range s.Quest {
		add(r.Image)
	}
	for _, r := range s.TownPortal {
		add(r.Image)
	}
	for _, r := range s.Scripts {
		add(r.Image)
	}
	return out
}

// Write saves the set as JSON.
func (s *Set) Write(path string) error {
	data, err := json.MarshalIndent(s, "", " ")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, data, 0o644)
}
