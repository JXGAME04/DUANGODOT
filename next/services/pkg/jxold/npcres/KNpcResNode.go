package npcres

import (
	"bufio"
	"bytes"
	"fmt"
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// Layout constants of KNpcResNode.h / GameDataDef.h.
const (
	MaxBodyPart     = 5 // head, equipment, weapon, horse, mantle
	MaxBodyPartSect = 4 // parts per group
	MaxPart         = MaxBodyPart * MaxBodyPartSect
	NormalNpcPartNo = 5  // a normal npc has one image, kept in this slot
	MaxSortDir      = 16 // rows of a draw-order block
	MaxNpcDir       = 64 // logical directions
)

// CLIENTACTION of KNpc.h: what a character is doing, and the column order of the normal npc
// tables (npc动作表.txt).
const (
	DoFightStand = iota
	DoStand
	DoStand1
	DoFightWalk
	DoWalk
	DoFightRun
	DoRun
	DoHurt
	DoDeath
	DoAttack
	DoAttack1
	DoMagic
	DoSit
	DoJump
	DoCount
)

// Table files (CoreUseNameDef.h); the names are UTF-8 here and encoded to GBK for the archive.
const (
	ResIniPath        = `\settings\npcres`
	KindFile          = `\settings\npcres\人物类型.txt`
	NormalResFile     = `\settings\npcres\普通npc资源.txt`
	NormalSprInfoFile = `\settings\npcres\普通npc资源信息.txt`
	PlayerShadowFile  = `\settings\npcres\主角动作阴影对应表.txt`
	ActionNameFile    = `动作编号表.txt`
	NpcActionFile     = `npc动作表.txt`
	TemplateFile      = `\settings\npcs.txt`
	PlayerBaseFile    = `\settings\npc\player\BaseValue.ini`
	sprInfoSuffix     = `信息`
)

// SprInfo is one sprite file with the frame layout the info tables give it (CSPR_INFO).
type SprInfo struct {
	File     string // game path (ASCII), "" when the part has no image for this action
	Frames   int
	Dirs     int
	Interval int
	Color    uint32
}

// Part is one body part of a main character with the sprite of every equipment / action.
type Part struct {
	Index  int
	Name   string
	Equips [][]SprInfo // [equip][action]
}

// SortRow is one line of the draw-order table: the frame it applies to (-1 = any) and the part
// indices back to front.
type SortRow struct {
	Frame int
	Parts []int
}

// SortAct is an action's own draw order (CSortTable::SActTableOff).
type SortAct struct {
	UseDefault bool      // no DirN lines: fall back to [DEFAULT] except for listed frames
	Dirs       []SortRow // MaxSortDir rows when !UseDefault
	Lines      []SortRow // per-frame rows (LineN)
}

// SortTable is 贴图顺序表.txt (CSortTable).
type SortTable struct {
	PartNum int
	Default []SortRow // MaxSortDir rows
	Acts    map[int]*SortAct
}

// Node is KNpcResNode: everything needed to draw one kind of character.
type Node struct {
	Name    string
	Special bool // main character composed of parts (SpecialNpc) vs one image (NormalNpc)
	ResPath string
	Actions []SprInfo // normal npc: per doing
	Shadow  []SprInfo // per doing (normal) or per action (special)
	Parts   [MaxPart]*Part
	PartNum int
	NoHorse [][]int // [equip][doing] -> action index (-1 = none)
	OnHorse [][]int
	Sort    *SortTable
}

// List is KNpcResList: the action name tables plus the nodes loaded so far.
type List struct {
	read       func(gbkPath string) ([]byte, error)
	Actions    []string // 动作编号表.txt: the main characters' actions (columns of the part tables)
	NpcActions []string // npc动作表.txt: the doings of normal npcs
	kinds      *TabFile
	nodes      map[string]*Node
	resDirGBK  string // ResIniPath + `\` in GBK: table cells already hold GBK file names
	infoGBK    string // sprInfoSuffix in GBK
}

// Load reads the action tables through read (game path in GBK bytes -> content).
func Load(read func(gbkPath string) ([]byte, error)) (*List, error) {
	l := &List{read: read, nodes: map[string]*Node{}}
	var err error
	for utf8s, dst := range map[string]*string{ResIniPath + `\`: &l.resDirGBK, sprInfoSuffix: &l.infoGBK} {
		g, err := text.UTF8ToGBK(utf8s)
		if err != nil {
			return nil, err
		}
		*dst = string(g)
	}
	if l.Actions, err = l.actionNames(ResIniPath + `\` + ActionNameFile); err != nil {
		return nil, err
	}
	if l.NpcActions, err = l.actionNames(ResIniPath + `\` + NpcActionFile); err != nil {
		return nil, err
	}
	data, err := l.readUTF8(KindFile)
	if err != nil {
		return nil, err
	}
	l.kinds = ParseTab(data)
	return l, nil
}

func (l *List) readUTF8(utf8Path string) ([]byte, error) {
	g, err := text.UTF8ToGBK(utf8Path)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", utf8Path, err)
	}
	data, err := l.read(string(g))
	if err != nil {
		return nil, fmt.Errorf("%s: %w", utf8Path, err)
	}
	return data, nil
}

// actionNames is CActionName::Init: the first column of every data row.
func (l *List) actionNames(utf8Path string) ([]string, error) {
	data, err := l.readUTF8(utf8Path)
	if err != nil {
		return nil, err
	}
	tab := ParseTab(data)
	names := make([]string, 0, tab.Height())
	for row := 2; row <= tab.Height(); row++ {
		names = append(names, tab.Get(row, 1))
	}
	return names, nil
}

// ActionNo is CActionName::GetActionNo for the main-character actions.
func (l *List) ActionNo(name string) int {
	for i, n := range l.Actions {
		if n == name {
			return i
		}
	}
	return -1
}

// Kinds lists every character name of 人物类型.txt.
func (l *List) Kinds() []string {
	out := make([]string, 0, l.kinds.Height())
	for row := 2; row <= l.kinds.Height(); row++ {
		out = append(out, l.kinds.Get(row, 1))
	}
	return out
}

// Node loads (once) the resource node of a character name (KNpcResList::AddNpcRes).
func (l *List) Node(name string) (*Node, error) {
	if n, ok := l.nodes[name]; ok {
		return n, nil
	}
	row := l.kinds.FindRow(name)
	if row < 0 {
		return nil, fmt.Errorf("npcres: %q not in 人物类型.txt", name)
	}
	n := &Node{Name: name}
	n.Special = l.kinds.GetByName(row, "CharacterType") == "SpecialNpc"
	n.ResPath = l.kinds.GetByName(row, "ResFilePath")
	var err error
	if n.Special {
		err = l.loadSpecial(n, row)
	} else {
		err = l.loadNormal(n)
	}
	if err != nil {
		return nil, err
	}
	l.nodes[name] = n
	return n, nil
}

// loadSpecial is the SpecialNpc branch of KNpcResNode::Init.
func (l *List) loadSpecial(n *Node, row int) error {
	partList := l.kinds.GetByName(row, "PartFileName")
	if partList == "" {
		return fmt.Errorf("npcres: %s has no PartFileName", n.Name)
	}
	data, err := l.readGBKName(partList)
	if err != nil {
		return err
	}
	parts := ParseTab(data)
	for i := 0; i < MaxBodyPart; i++ {
		for j := 0; j < MaxBodyPartSect; j++ {
			sect := parts.Get(i+2, j+3)
			if sect == "" {
				continue
			}
			idx := i*MaxBodyPartSect + j
			n.Parts[idx] = &Part{Index: idx, Name: sect}
			n.PartNum++
		}
	}
	actionCount := len(l.Actions)
	for _, p := range n.Parts {
		if p == nil {
			continue
		}
		resName := l.kinds.GetByName(row, p.Name)
		if resName == "" {
			continue
		}
		infoName := strings.TrimSuffix(resName, ".txt") + l.infoGBK + ".txt"
		sect, err := l.readGBKName(resName)
		if err != nil {
			return err
		}
		info, err := l.readGBKName(infoName)
		if err != nil {
			return err
		}
		sectTab, infoTab := ParseTab(sect), ParseTab(info)
		equipCount := sectTab.Height() - 1
		if equipCount <= 0 {
			continue
		}
		colorCol := sectTab.FindColumn("ChangeColor")
		p.Equips = make([][]SprInfo, equipCount)
		for j := 0; j < equipCount; j++ {
			var color uint32
			if colorCol > 0 {
				if s := sectTab.Get(j+2, colorCol); s != "" {
					if v, err := strconv.ParseUint(s, 16, 32); err == nil {
						color = uint32(v) | 0xff000000
					}
				}
			}
			p.Equips[j] = make([]SprInfo, actionCount)
			for k := 0; k < actionCount; k++ {
				si := &p.Equips[j][k]
				si.File = ComposePathAndName(n.ResPath, sectTab.Get(j+2, k+2))
				si.Frames, si.Dirs, si.Interval = parseSprInfo(infoTab.Get(j+2, k+2), 16, 16, 0)
				si.Color = color
			}
		}
	}
	if n.NoHorse, err = l.equipStyleTable(l.kinds.GetByName(row, "WeaponActionTab1")); err != nil {
		return err
	}
	if n.OnHorse, err = l.equipStyleTable(l.kinds.GetByName(row, "WeaponActionTab2")); err != nil {
		return err
	}
	if sortName := l.kinds.GetByName(row, "ActionRenderOrderTab"); sortName != "" {
		// the cell may list alternatives ("a.txt;anew.txt"): the first one that loads wins,
		// like the engine's file opener
		var data []byte
		var err error
		for _, cand := range strings.Split(sortName, ";") {
			if cand = strings.TrimSpace(cand); cand == "" {
				continue
			}
			if data, err = l.readGBKName(cand); err == nil {
				break
			}
		}
		if err != nil {
			return err
		}
		n.Sort = parseSortTable(data, l.Actions, n.PartNum)
	}
	// 主角动作阴影对应表.txt: a shadow sprite per action, "16,8,1" when the info is missing
	n.Shadow = make([]SprInfo, actionCount)
	if data, err := l.readUTF8(PlayerShadowFile); err == nil {
		tab := ParseTab(data)
		if r := tab.FindRow(n.Name); r > 0 {
			for i := 0; i < actionCount; i++ {
				si := &n.Shadow[i]
				si.File = ComposePathAndName(n.ResPath, tab.Get(r, 2+i*2))
				si.Frames, si.Dirs, si.Interval = parseSprInfo(tab.Get(r, 3+i*2), 16, 8, 1)
			}
		}
	}
	return nil
}

// loadNormal is the NormalNpc branch of KNpcResNode::Init: one image per doing, the shadow
// derived from the image name.
func (l *List) loadNormal(n *Node) error {
	resData, err := l.readUTF8(NormalResFile)
	if err != nil {
		return err
	}
	infoData, _ := l.readUTF8(NormalSprInfoFile)
	resTab, infoTab := ParseTab(resData), ParseTab(infoData)
	row := resTab.FindRow(n.Name)
	if row < 0 {
		return fmt.Errorf("npcres: %q not in 普通npc资源.txt", n.Name)
	}
	infoRow := infoTab.FindRow(n.Name)
	n.PartNum = 1
	n.Actions = make([]SprInfo, len(l.NpcActions))
	n.Shadow = make([]SprInfo, len(l.NpcActions))
	for i, act := range l.NpcActions {
		si := &n.Actions[i]
		si.File = ComposePathAndName(n.ResPath, resTab.GetByName(row, act))
		si.Frames, si.Dirs, si.Interval = parseSprInfo(infoTab.GetByName(infoRow, act), 16, 8, 0)
		n.Shadow[i] = SprInfo{File: ShadowName(si.File), Frames: si.Frames, Dirs: si.Dirs, Interval: si.Interval}
	}
	return nil
}

// equipStyleTable reads a 关联表 (CEquipStyleTable): rows = equipment kinds, columns = doings,
// cells = action names resolved to indices.
func (l *List) equipStyleTable(fileName string) ([][]int, error) {
	if fileName == "" {
		return nil, nil
	}
	data, err := l.readGBKName(fileName)
	if err != nil {
		return nil, err
	}
	tab := ParseTab(data)
	height, width := tab.Height()-1, tab.Width()-1
	if height <= 0 || width <= 0 {
		return nil, nil
	}
	out := make([][]int, height)
	for i := 0; i < height; i++ {
		out[i] = make([]int, width)
		for j := 0; j < width; j++ {
			out[i][j] = l.ActionNo(tab.Get(i+2, j+2))
		}
	}
	return out, nil
}

// readGBKName reads a table of the npcres folder by the file name as a table cell holds it
// (GBK bytes, no re-encoding).
func (l *List) readGBKName(fileName string) ([]byte, error) {
	data, err := l.read(l.resDirGBK + fileName)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", text.GBKToUTF8([]byte(fileName)), err)
	}
	return data, nil
}

// ActNo is KNpcResNode::GetActNo: the action a doing maps to for a weapon kind.
func (n *Node) ActNo(doing, equip int, ride bool) int {
	if !n.Special {
		return doing
	}
	table := n.NoHorse
	if ride {
		table = n.OnHorse
	}
	if equip < 0 || equip >= len(table) || doing < 0 || doing >= len(table[equip]) {
		return -1
	}
	return table[equip][doing]
}

// Sort is CSortTable::GetSort: the part draw order for an action, sprite direction and frame.
func (s *SortTable) Sort(act, dir, frame int) []int {
	if s == nil || dir < 0 || dir >= len(s.Default) {
		return nil
	}
	a := s.Acts[act]
	if a == nil {
		return s.Default[dir].Parts
	}
	for _, line := range a.Lines {
		if line.Frame == frame {
			return line.Parts
		}
	}
	if a.UseDefault || dir >= len(a.Dirs) {
		return s.Default[dir].Parts
	}
	return a.Dirs[dir].Parts
}

// ComposePathAndName is KNpcResNode::ComposePathAndName: "\<path>\<name>".
func ComposePathAndName(path, name string) string {
	if name == "" {
		return ""
	}
	if path == "" {
		return name
	}
	out := path
	if !strings.HasPrefix(out, `\`) {
		out = `\` + out
	}
	if !strings.HasSuffix(out, `\`) {
		out += `\`
	}
	return out + name
}

// ShadowName is KNpcResNode::GetShadowName: "<name>b.spr" next to the image.
func ShadowName(spr string) string {
	if spr == "" {
		return ""
	}
	if i := strings.LastIndexByte(spr, '.'); i >= 0 {
		spr = spr[:i]
	}
	return spr + "b.spr"
}

// parseSprInfo reads "frames,dirs,interval" (optionally quoted) with the old defaults.
func parseSprInfo(s string, defFrames, defDirs, defInterval int) (int, int, int) {
	fields := strings.Split(unquote(s), ",")
	get := func(i, def int) int {
		if i >= len(fields) {
			return def
		}
		v, err := strconv.Atoi(strings.TrimSpace(fields[i]))
		if err != nil {
			return def
		}
		return v
	}
	return get(0, defFrames), get(1, defDirs), get(2, defInterval)
}

// parseSortTable is CSortTable::GetTable: [DEFAULT] Dir1..16, then per action section either
// its own Dir1..16 (missing ones copied from DEFAULT) and/or Line1.. rows "<frame>,<parts...>".
func parseSortTable(data []byte, actions []string, partNum int) *SortTable {
	if partNum < 1 {
		partNum = 1
	}
	ini := ParseIni(data)
	s := &SortTable{PartNum: partNum, Acts: map[int]*SortAct{}}
	def := ini["default"]
	s.Default = make([]SortRow, MaxSortDir)
	for i := 0; i < MaxSortDir; i++ {
		s.Default[i] = sortStrToNum(def[fmt.Sprintf("dir%d", i+1)], partNum)
	}
	for act, name := range actions {
		sec, ok := ini[strings.ToLower(name)]
		if !ok {
			continue
		}
		a := &SortAct{UseDefault: true}
		for j := 0; j < MaxSortDir; j++ {
			if _, ok := sec[fmt.Sprintf("dir%d", j+1)]; ok {
				a.UseDefault = false
				break
			}
		}
		if !a.UseDefault {
			a.Dirs = make([]SortRow, MaxSortDir)
			for j := 0; j < MaxSortDir; j++ {
				if v, ok := sec[fmt.Sprintf("dir%d", j+1)]; ok {
					a.Dirs[j] = sortStrToNum(v, partNum)
				} else {
					a.Dirs[j] = s.Default[j]
				}
			}
		}
		for j := 1; ; j++ {
			v, ok := sec[fmt.Sprintf("line%d", j)]
			if !ok {
				break
			}
			a.Lines = append(a.Lines, sortStrToNum(v, partNum))
		}
		if a.UseDefault && len(a.Lines) == 0 {
			continue // the old code leaves nActOff == 0: plain default
		}
		s.Acts[act] = a
	}
	return s
}

// sortStrToNum is CSortTable::SortStrToNum: the numbers of "<frame>,<part>,<part>...".
func sortStrToNum(s string, partNum int) SortRow {
	row := SortRow{Frame: -1, Parts: make([]int, partNum)}
	for i := range row.Parts {
		row.Parts[i] = -1
	}
	if s == "" {
		return row
	}
	pos := 0
	for _, f := range strings.Split(s, ",") {
		f = strings.TrimSpace(f)
		if f == "" {
			continue
		}
		v, err := strconv.Atoi(f)
		if err != nil {
			continue
		}
		if pos == 0 {
			row.Frame = v
		} else if pos-1 < partNum {
			row.Parts[pos-1] = v
		}
		pos++
	}
	return row
}

// ParseIni reads "[section]" / "key=value" lines; section and key names are lower-cased.
func ParseIni(data []byte) map[string]map[string]string {
	out := map[string]map[string]string{}
	section := ""
	sc := bufio.NewScanner(bytes.NewReader(data))
	for sc.Scan() {
		line := strings.TrimSpace(sc.Text())
		if line == "" || line[0] == ';' || line[0] == '#' {
			continue
		}
		if line[0] == '[' {
			if end := strings.IndexByte(line, ']'); end > 0 {
				section = strings.ToLower(strings.TrimSpace(line[1:end]))
				if _, ok := out[section]; !ok {
					out[section] = map[string]string{}
				}
			}
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq <= 0 {
			continue
		}
		if _, ok := out[section]; !ok {
			out[section] = map[string]string{}
		}
		out[section][strings.ToLower(strings.TrimSpace(line[:eq]))] = strings.TrimSpace(line[eq+1:])
	}
	return out
}

// PlayerFrames is [Male]/[Female] of BaseValue.ini (KNpcSet::LoadPlayerBaseValue).
type PlayerFrames struct {
	WalkFrame  int `json:"walk_frame"`
	RunFrame   int `json:"run_frame"`
	StandFrame int `json:"stand_frame"`
}

// ParsePlayerBase reads BaseValue.ini; missing values use the old defaults (15).
func ParsePlayerBase(data []byte) map[string]PlayerFrames {
	ini := ParseIni(data)
	get := func(sec, key string, def int) int {
		if v, ok := ini[sec][key]; ok {
			if n, err := strconv.Atoi(v); err == nil {
				return n
			}
		}
		return def
	}
	out := map[string]PlayerFrames{}
	for sec, name := range map[string]string{"male": "male", "female": "female"} {
		out[name] = PlayerFrames{
			WalkFrame:  get(sec, "walkframe", 15),
			RunFrame:   get(sec, "runframe", 15),
			StandFrame: get(sec, "standframe", 15),
		}
	}
	return out
}
