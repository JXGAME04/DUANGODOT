// jxassets - reads the old JX client archives and exports assets for the Godot client.
//
//	jxassets [-client <bin/Client>] [-out <path>] <command> <args...>
//
//	list   <pak-file>                index of one archive
//	find   <gamepath>                which archive holds a file
//	cat    <gamepath>                dump a plain file to stdout (or -out file)
//	spr    <gamepath>                decode a sprite: -out <dir> gets <name>.png + <name>.json
//	map    <mapid | gamepath>        summary of a world: rect, regions present, images used
//	region <mapid | gamepath> <x> <y> parsed Region_C.dat as JSON
//	objects <mapid | gamepath> <x> <y> [text]  raw cover/buildin records + sprite headers of one region
//	export-npcres [mapid...] -out <dir>   npc/character appearance tables + sprites used on those maps
//	export-items -out <dir>          the item tables of the old server (settings/item, every version) as JSON
//	export-item-res -out <dir>       settings/item/*res.txt + goldequipres.txt (what worn pieces look like) -> items/item_res.json
//	export-player -out <dir>         settings/npc/player of the old server (level_exp, level_add, stamina.ini,
//	                                 basevalue.ini, newplayerini%02d) as player.json for the zone and the gateway
//	export-skills -out <dir>         settings/skills.txt of the old server (every row, every column the JX2
//	                                 server reads) as skills.json for the zone's KSkillManager
//	export-missles -out <dir>        settings/missles.txt of the old server (the missile templates the skills
//	                                 fire) as missles.json for the zone's KMissleTable
//	export-state-gfx -out <dir>      settings/npcres/状态图形对照表.txt of the client (the picture a state puts on a
//	                                 character) as npcres/state_gfx.json + the sprites the skills' StateSpecialId use
//	export-menu-state -out <dir>     settings/npcres/界面状态与图形对照表.txt of the client (the sign over a player's head:
//	                                 team open / trade open / trading) as npcres/menu_state.json + the sprites
//	export-sounds -out <dir>         the .wav files the skills (ManCastSnd / FMCastSnd of skills.json), the missiles
//	                                 (SndFile1..4 of missles/missle_res.json) and the characters' actions (主角动作声音表.txt,
//	                                 npc动作声音表.txt for the exported npcres) name -> sounds/<id>.wav + sounds/sounds.json,
//	                                 npcres/action_sounds.json
//
// Game paths are UTF-8 on the command line and encoded to GBK for hashing (the archives use
// the original Chinese paths); hex:<bytes> passes raw bytes.  A map id refers to Settings/MapList.ini.
package main

import (
	"bytes"
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/export"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/item"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/missle"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/player"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/skill"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/spr"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/wor"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"golang.org/x/text/encoding/simplifiedchinese"
)

var (
	flagClient = flag.String("client", "", "old client folder containing package.ini and Data/*.pak")
	flagOut    = flag.String("out", "", "output file or directory")
	flagLevel  = flag.String("log-level", "info", "log level")
	flagTpl    = flag.String("templates", "", "export-npcres: extra npc template ids (comma separated), e.g. the zone's test npcs")
	flagRows   = flag.String("equip-rows", "2:1,2;3:8", "export-npcres: extra equipment rows per part group, \"group:row,row;group:row\" (0 head, 1 body, 2 weapon, 3 horse, 4 mantle) or \"all\" (every row the item tables name); a horse row also exports the on-horse actions")
	flagAll    = flag.Bool("all", false, "export-objdata: every row's picture, not only the ones items and money use")
	flagTheme  = flag.String("theme", "", "export-ui: a fragment of the theme folder name (1024, 800); default: the largest")
	flagLang   = flag.String("lang", "vn", "export-skill-desc: the \\lang\\<lang> folder of the client (gamecl.exe 2.0 picks it by its language index at 0x80ec60: vn)")
	flagServer = flag.String("server", "", "old server folder(s) 'a;b' (package.ini + pak/maps.pak, Settings, script): the first with a pak serves the regions, plain files come from the first that has them; default: the Server folder next to the client")
)

func fail(format string, args ...any) {
	fmt.Fprintf(os.Stderr, format+"\n", args...)
	os.Exit(1)
}

func gamePath(arg string) string {
	if strings.HasPrefix(arg, "hex:") {
		b, err := hex.DecodeString(arg[4:])
		if err != nil {
			fail("bad hex path: %v", err)
		}
		return string(b)
	}
	b, err := text.UTF8ToGBK(arg)
	if err != nil {
		fail("cannot encode path to GBK: %v", err)
	}
	return string(b)
}

// itemTableSets is settings/item of the old server and every version folder under it (the JX2
// server keeps 000..004 next to the plain tables): where the item tables are read from.
// clientItemSets loads the CLIENT's copies of the item tables (\settings\item\NNN\*.txt in its
// archives) into a temp folder and reads them like a server folder: the text the player sees.
func clientItemSets(set *pak.Set) []*item.Set {
	var out []*item.Set
	for _, version := range []string{"000", "001", "002", "003", "004"} {
		tmp := filepath.Join(os.TempDir(), "jxnext-clientitems", version)
		if err := os.MkdirAll(tmp, 0o755); err != nil {
			fail("%s: %v", tmp, err)
		}
		have := 0
		for _, name := range item.TableFiles() {
			data, err := set.ReadFile(gamePath(`\settings\item\` + version + `\` + name + `.txt`))
			if err != nil {
				continue
			}
			if err := os.WriteFile(filepath.Join(tmp, name+".txt"), data, 0o644); err != nil {
				fail("%s: %v", name, err)
			}
			have++
		}
		if have == 0 {
			continue
		}
		cset, err := item.Load(tmp, version)
		if err != nil {
			fmt.Printf("  client %s: %v\n", version, err)
			continue
		}
		out = append(out, cset)
	}
	return out
}

func itemTableSets() []struct{ dir, version, file string } {
	// the client is not needed for this: only ask for it when nothing names the server
	sdir := *flagServer
	if sdir == "" {
		sdir = os.Getenv("JX_OLD_SERVER")
	}
	if sdir == "" {
		sdir = findServer(findClient())
	}
	if sdir == "" {
		fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
	}
	var itemDir string
	for _, root := range serverRoots(sdir) {
		for _, sub := range []string{"settings/item", "Settings/item", "Settings/Item"} {
			if st, err := os.Stat(filepath.Join(root, sub)); err == nil && st.IsDir() {
				itemDir = filepath.Join(root, sub)
				break
			}
		}
		if itemDir != "" {
			break
		}
	}
	if itemDir == "" {
		fail("no settings/item under %s", sdir)
	}
	sets := []struct{ dir, version, file string }{{itemDir, "", "base"}}
	for _, v := range item.Versions(itemDir) {
		sets = append(sets, struct{ dir, version, file string }{filepath.Join(itemDir, v), v, "v" + v})
	}
	return sets
}

func findClient() string {
	if *flagClient != "" {
		return *flagClient
	}
	if env := os.Getenv("JX_OLD_CLIENT"); env != "" {
		return env
	}
	for _, rel := range []string{"../bin/Client", "../../bin/Client", "bin/Client"} {
		for _, ini := range []string{"package.ini", "config.ini"} {
			if _, err := os.Stat(filepath.Join(rel, ini)); err == nil {
				p, _ := filepath.Abs(rel)
				return p
			}
		}
	}
	fail("old client folder not found: pass -client or set JX_OLD_CLIENT (folder with package.ini or config.ini)")
	return ""
}

// gbkAsLatin1 is the file name a Windows extraction gave a GBK named file: every byte of the GBK
// encoding read as one cp1252 / Latin-1 letter ("门派设定" -> "ÃÅÅÉÉè¶¨").
func gbkAsLatin1(s string) string {
	raw, err := simplifiedchinese.GBK.NewEncoder().Bytes([]byte(s))
	if err != nil {
		return s
	}
	out := make([]rune, 0, len(raw))
	for _, b := range raw {
		out = append(out, rune(b))
	}
	return string(out)
}

// primaryDir is the reference folder of a "reference;fallback" client chain (plain files such as
// Settings/MapList.ini are only looked up there).
func primaryDir(dir string) string {
	return strings.TrimSpace(strings.Split(dir, ";")[0])
}

// reportFallback logs which files the fallback client folders had to serve.
func reportFallback(set *pak.Set) {
	max := 20
	if *flagLevel == "debug" {
		max = 1 << 20
	}
	if n, sample := set.FallbackReport(max); n > 0 {
		log.Info("asset", "files served by the fallback client folder(s)", log.F("files", n), log.F("sample", text.GBKToUTF8([]byte(strings.Join(sample, " | ")))))
	}
}

func openSet(dir string) *pak.Set {
	set, err := pak.OpenClientSet(dir)
	if err != nil {
		fail("%v", err)
	}
	return set
}

// mapList returns Settings/MapList.ini: the plain file of a JX1 client, else the copy inside the
// archives (\settings\maplist.ini; the VLTK 2.0 client ships no plain settings folder).
func mapList(dir string, set *pak.Set) ([]byte, error) {
	if data, err := os.ReadFile(filepath.Join(primaryDir(dir), "Settings", "MapList.ini")); err == nil {
		return data, nil
	}
	if set != nil {
		if data, err := set.ReadFile(gamePath(`\settings\maplist.ini`)); err == nil {
			return data, nil
		}
	}
	return nil, fmt.Errorf("no Settings/MapList.ini in %s and none in its archives", dir)
}

// mapDirPath normalises a MapList entry: newer lists (Linux server, VLTK 2.0) omit the \maps\ root
// (`西北南区\凤翔` instead of `\maps\西北南区\凤翔`).
func mapDirPath(p string) string {
	q := strings.ToLower(strings.ReplaceAll(p, "/", `\`))
	if strings.HasPrefix(q, `\maps\`) || strings.HasPrefix(q, `maps\`) {
		return p
	}
	return `\maps\` + strings.TrimPrefix(p, `\`)
}

// mapPath resolves "<id>" through the map list or passes a game path through.
func mapPath(dir string, set *pak.Set, arg string) (string, string) {
	if id, err := strconv.Atoi(arg); err == nil {
		data, err := mapList(dir, set)
		if err != nil {
			fail("MapList.ini: %v", err)
		}
		re := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `=(.*)$`)
		m := re.FindSubmatch(data)
		if m == nil {
			fail("map %d not in MapList.ini", id)
		}
		p := mapDirPath(strings.TrimSpace(strings.TrimRight(string(m[1]), "\r")))
		name := ""
		if nm := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `_name=(.*)$`).FindSubmatch(data); nm != nil {
			name = text.TCVN3ToUTF8([]byte(strings.TrimSpace(strings.TrimRight(string(nm[1]), "\r"))))
		}
		return p, name
	}
	return gamePath(arg), ""
}

// mapSettings reads the per-map keys of [List] in maplist.ini the JX2 server's map loader keeps in its
// KSubWorld (jx_linux_y 0x080F1416..: `%d_AutoGoldenNpc` GetInteger 0 -> +0x63e7c, `%d_GoldenType` GetInteger 0
// (negative -> 0) -> +0x63e84, `%d_GoldenDropRate` GetString -> the drop table +0x63e80, `%d_NormalDropRate`
// GetString -> the drop table +0x63e88).  The rules come from the reference server's file (-server /
// config/oldgame.local.json); the client's copy serves when there is none.
func mapSettings(dir string, set *pak.Set, id int) *export.MapSettings {
	var data []byte
	if sdir := findServer(dir); sdir != "" {
		if d, _, err := readServerFile(sdir, `settings\maplist.ini`, `Settings\MapList.ini`); err == nil {
			data = d
		}
	}
	if data == nil {
		d, err := mapList(dir, set)
		if err != nil {
			return nil
		}
		data = d
	}
	key := func(name string) string {
		re := regexp.MustCompile(`(?mi)^` + strconv.Itoa(id) + `_` + name + `\s*=(.*)$`)
		m := re.FindSubmatch(data)
		if m == nil {
			return ""
		}
		return strings.TrimSpace(strings.TrimRight(string(m[1]), "\r"))
	}
	num := func(name string, def int) int {
		v := key(name)
		if v == "" {
			return def
		}
		return npcres.Atoi(v)
	}
	s := &export.MapSettings{
		MapType:          key("MapType"),
		AutoGoldenNpc:    num("AutoGoldenNpc", 0),
		GoldenType:       num("GoldenType", 0),
		GoldenDropRate:   strings.ToLower(key("GoldenDropRate")),
		NormalDropRate:   strings.ToLower(key("NormalDropRate")),
		NpcSeriesAuto:    num("NpcSeriesAuto", 0),
		NpcAutoLevelFlag: num("NpcAutoLevelFlag", 0),
		NpcAutoLevelMax:  num("NpcAutoLevelMax", 1),
		NpcAutoLevelMin:  num("NpcAutoLevelMin", 1),
	}
	if s.GoldenType < 0 {
		s.GoldenType = 0 // 0x080F149C
	}
	if s.NpcSeriesAuto != 0 { // 0x080F1203: the weights are read only with the flag (0x080F1BBF zeroes them otherwise)
		for i, name := range []string{"NpcSeriesMetal", "NpcSeriesWood", "NpcSeriesWater", "NpcSeriesFire", "NpcSeriesEarth"} {
			s.NpcSeries[i] = num(name, 0)
		}
	}
	return s
}

// resolveImage finds the archive path of a sprite referenced by map data (names are stored
// relative to the sprite root).
// findServer returns the old server folder (package.ini + pak/maps.pak with the Region_S files):
// -server, JX_OLD_SERVER, or the Server folder next to the client.
func findServer(clientDir string) string {
	if *flagServer != "" {
		return *flagServer
	}
	if env := os.Getenv("JX_OLD_SERVER"); env != "" {
		return env
	}
	cand := filepath.Join(filepath.Dir(filepath.Clean(primaryDir(clientDir))), "Server") // no chain: the folder next to the client
	if _, err := os.Stat(filepath.Join(cand, "package.ini")); err == nil {
		return cand
	}
	return ""
}

// prepareExporter wires what map npcs need: the templates (names), the old server's archive
// (Region_S.dat with the real npcs), replacename_npc.txt and the stand frame counts for the
// facing of client-only npcs.  Returns the function that closes the server archive.
func prepareExporter(ex *export.Exporter, clientDir string, set *pak.Set) func() {
	sdir := findServer(clientDir)
	ex.Templates = loadTemplates(set, sdir)
	closer := func() {}
	if sdir == "" {
		log.Warn("asset", "old server folder not found: map npcs come from the client archive only (pass -server)")
	} else if s, err := openServerSet(sdir); err != nil {
		log.Warn("asset", "server archive unavailable", log.F("dir", sdir), log.F("error", err))
	} else {
		ex.ServerSet = s
		closer = s.Close
		ex.ScriptNames = scriptIndex(sdir)
		log.Info("asset", "script index for the traps", log.F("scripts", len(ex.ScriptNames)))
		if data, p, err := readServerFile(sdir, npcres.ReplaceNameFile, npcres.ReplaceNameFileLang); err == nil {
			ex.ReplaceNames = npcres.ParseReplaceNames(data)
			log.Info("asset", "npc names replaced through", log.F("file", p))
		} else {
			log.Warn("asset", "replacename_npc.txt missing", log.F("error", err))
		}
	}
	if list, err := npcres.Load(set.ReadFile); err == nil {
		cache := map[int]int{}
		ex.StandFrames = func(id int) int {
			if v, ok := cache[id]; ok {
				return v
			}
			frames := 0
			if id >= 0 && id < len(ex.Templates) {
				if node, err := list.Node(ex.Templates[id].ResType); err == nil && !node.Special && npcres.DoStand < len(node.Actions) {
					frames = node.Actions[npcres.DoStand].Frames
				}
			}
			cache[id] = frames
			return frames
		}
	} else {
		log.Warn("asset", "npcres tables unavailable, client npcs face down", log.F("error", err))
	}
	return closer
}

// loadTemplates reads Settings/npcs.txt from the archives (nil when it is missing) so map npcs
// get their in-game names.
// loadTemplates reads npcs.txt: the old server's plain Settings/npcs.txt when a server folder is
// known (the table the live server ran with), else the client's copy inside the archives.
// serverRoots splits the -server chain "a;b" (the Linux server first, the project server as
// the fallback for what it lacks, e.g. the per-map trap scripts).
func serverRoots(serverDir string) []string {
	var out []string
	for _, r := range strings.Split(serverDir, ";") {
		if r = strings.TrimSpace(r); r != "" {
			out = append(out, r)
		}
	}
	return out
}

// openServerSet opens the region archive of the first root that has a package.ini.
func openServerSet(serverDir string) (*pak.Set, error) {
	var last error = fmt.Errorf("no server folder")
	for _, r := range serverRoots(serverDir) {
		if s, err := pak.OpenSet(filepath.Join(r, "package.ini")); err == nil {
			return s, nil
		} else {
			last = err
		}
	}
	return nil, last
}

// readServerFile reads the first of the candidate files (game paths with backslashes) found
// under the old server folder chain.
func readServerFile(serverDir string, candidates ...string) ([]byte, string, error) {
	var last error
	for _, root := range serverRoots(serverDir) {
		for _, c := range candidates {
			p := filepath.Join(root, filepath.FromSlash(strings.ReplaceAll(c, `\`, "/")))
			data, err := os.ReadFile(p)
			if err == nil {
				return data, p, nil
			}
			last = err
		}
	}
	return nil, "", last
}

// scriptIndex maps g_FileName2Id -> script path over the script folders of the chain (first wins).
func scriptIndex(serverDir string) map[uint32]string {
	out := map[uint32]string{}
	for _, root := range serverRoots(serverDir) {
		for id, name := range npcres.ScriptIndex(filepath.Join(root, "script")) {
			if _, dup := out[id]; !dup {
				out[id] = name
			}
		}
	}
	return out
}

// loadDropRates reads every drop table the templates name (the DropRateFile column) and the ones the
// maps name (`<id>_NormalDropRate` / `<id>_GoldenDropRate` of maplist.ini: the JX2 server swaps a placed
// monster's table for the map's, KNpcSet::Add 0x0809FD30, and for the golden one while it is gold,
// 0x08086073) from the server folder; a table that is not there is reported once and the npc drops nothing.
func loadDropRates(templates []npcres.Template, serverDir string, mapTables []string) map[string]*item.DropRate {
	out := map[string]*item.DropRate{}
	if serverDir == "" {
		return out
	}
	missing := map[string]bool{}
	read := func(p string, who string) {
		if p == "" || out[p] != nil || missing[p] {
			return
		}
		rel := strings.TrimPrefix(strings.ReplaceAll(p, `\`, "/"), "/")
		data, _, err := readServerFile(serverDir, rel)
		if err != nil {
			missing[p] = true
			log.Warn("asset", "drop table missing", log.F("path", p), log.F("named_by", who))
			return
		}
		out[p] = item.ParseDropRate(p, data)
	}
	for _, t := range templates {
		read(t.DropRateFile, "template "+strconv.Itoa(t.ID))
	}
	for _, p := range mapTables {
		read(p, "maplist.ini")
	}
	log.Info("asset", "drop tables read", log.F("tables", len(out)), log.F("missing", len(missing)))
	return out
}

// mapListDropRates lists the `<id>_NormalDropRate` / `<id>_GoldenDropRate` files of maplist.ini (the server's
// copy first), lower-cased like the templates' DropRateFile, each once.
func mapListDropRates(dir string, set *pak.Set) []string {
	var data []byte
	if sdir := findServer(dir); sdir != "" {
		if d, _, err := readServerFile(sdir, `settings\maplist.ini`, `Settings\MapList.ini`); err == nil {
			data = d
		}
	}
	if data == nil {
		d, err := mapList(dir, set)
		if err != nil {
			return nil
		}
		data = d
	}
	seen := map[string]bool{}
	var out []string
	for _, m := range regexp.MustCompile(`(?mi)^\d+_(?:NormalDropRate|GoldenDropRate)\s*=(.*)$`).FindAllSubmatch(data, -1) {
		p := strings.ToLower(strings.TrimSpace(strings.TrimRight(string(m[1]), "\r")))
		if p == "" || seen[p] {
			continue
		}
		seen[p] = true
		out = append(out, p)
	}
	sort.Strings(out)
	return out
}

// loadTemplates reads npcs.txt.  With a server folder the server's plain Settings/npcs.txt rules
// (the table the live server ran with) and the client archive's own copy only supplies what the
// client draws (export.MergeAppearance); without one the client's copy is all there is.
func loadTemplates(set *pak.Set, serverDir string) []npcres.Template {
	var client []npcres.Template
	if data, err := set.ReadFile(gamePath(npcres.TemplateFile)); err == nil {
		client = npcres.ParseTemplates(data)
	}
	if serverDir != "" {
		if data, p, err := readServerFile(serverDir, `Settings\npcs.txt`); err == nil {
			server := npcres.ParseTemplates(data)
			merged, changed := export.MergeAppearance(server, client)
			log.Info("asset", "npc templates from the server folder", log.F("file", p), log.F("rows", len(server)),
				log.F("client_rows", len(client)), log.F("appearance_from_client", changed))
			return merged
		}
	}
	if client == nil {
		log.Warn("asset", "npcs.txt missing, map npcs keep their editor names")
	}
	return client
}

// loadSkills reads skills.txt (the server folder first, then the client archives) for the
// attack radius of the npc skills; nil when neither has it.
func loadSkills(set *pak.Set, serverDir string) map[int]npcres.Skill {
	if serverDir != "" {
		if data, p, err := readServerFile(serverDir, `Settings\skills.txt`); err == nil {
			s := npcres.ParseSkills(data)
			log.Info("asset", "skills from the server folder", log.F("file", p), log.F("count", len(s)))
			return s
		}
	}
	if data, err := set.ReadFile(gamePath(npcres.SkillFile)); err == nil {
		return npcres.ParseSkills(data)
	}
	log.Warn("asset", "skills.txt missing: npc skill radii unknown, the ai attacks at the default 30 units")
	return nil
}

func resolveImage(set *pak.Set, name string) (string, bool) {
	for _, prefix := range []string{"", `\spr`, `\spr\`} {
		p := prefix + name
		if _, _, ok := set.Lookup(p); ok {
			return p, true
		}
	}
	return "", false
}

func main() {
	flag.Parse()
	_ = log.Init(log.Options{Level: log.ParseLevel(*flagLevel), Console: true, Process: "jxassets"})
	args := flag.Args()
	// allow flags after the positional arguments too: jxassets cat path -out file
	for i, a := range args {
		if strings.HasPrefix(a, "-") {
			_ = flag.CommandLine.Parse(args[i:])
			args = args[:i]
			break
		}
	}
	if len(args) < 1 {
		fail("usage: jxassets [-client DIR] [-out PATH] list|find|cat|spr|map|region|objects|export-minimap ...")
	}
	switch args[0] {
	case "list":
		if len(args) < 2 {
			fail("list <pak-file>")
		}
		f, err := pak.Open(args[1])
		if err != nil {
			fail("%v", err)
		}
		defer f.Close()
		fmt.Printf("%s: %d entries\n", args[1], len(f.Entries()))
		for _, e := range f.Entries() {
			fmt.Printf("%08x off=%-10d size=%-9d stored=%-9d flags=%08x\n", e.ID, e.Offset, e.Size, e.Stored, e.Flags)
		}

	case "find", "cat", "spr":
		if len(args) < 2 {
			fail("%s <gamepath>", args[0])
		}
		set := openSet(findClient())
		defer set.Close()
		p := gamePath(args[1])
		f, e, ok := set.Lookup(p)
		if !ok {
			fail("not found: %s (id %08x, normalised %q)", args[1], pak.FileNameToID(p), pak.NormalizePath(p))
		}
		switch args[0] {
		case "find":
			fmt.Printf("%s: id %08x in %s (size %d, stored %d, flags %08x)\n", args[1], e.ID, filepath.Base(f.Path), e.Size, e.Stored, e.Flags)
		case "cat":
			data, err := f.Read(e)
			if err != nil {
				fail("%v", err)
			}
			if *flagOut != "" {
				if err := os.WriteFile(*flagOut, data, 0o644); err != nil {
					fail("%v", err)
				}
				fmt.Printf("wrote %d bytes to %s\n", len(data), *flagOut)
			} else {
				os.Stdout.Write(data)
			}
		case "spr":
			s, err := spr.ReadFromPak(f, e)
			if err != nil {
				fail("%v", err)
			}
			if *flagOut == "" {
				fail("spr needs -out <dir>")
			}
			if err := os.MkdirAll(*flagOut, 0o755); err != nil {
				fail("%v", err)
			}
			base := filepath.Join(*flagOut, strings.TrimSuffix(filepath.Base(strings.ReplaceAll(args[1], `\`, "/")), ".spr"))
			if err := s.WriteAtlas(base, args[1]); err != nil {
				fail("%v", err)
			}
			fmt.Printf("%s: %dx%d center %d,%d frames %d dirs %d colors %d zip=%v -> %s.png/.json\n", args[1], s.Width, s.Height, s.CenterX, s.CenterY, len(s.Frames), s.Directions, s.Colors, s.IsZip(), base)
		}

	case "map":
		if len(args) < 2 {
			fail("map <mapid|gamepath>")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		p, name := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		fmt.Printf("map %s (%s): rect %d,%d-%d,%d = %dx%d regions, indoor=%v\n", text.GBKToUTF8([]byte(p)), name, w.Left, w.Top, w.Right, w.Bottom, w.RegionCols(), w.RegionRows(), w.IsInDoor)
		present, nodes, npcs, bios := 0, 0, 0, 0
		images := map[string]int{}
		for y := w.Top; y <= w.Bottom; y++ {
			for x := w.Left; x <= w.Right; x++ {
				r, err := wor.LoadRegion(set, w, x, y)
				if err != nil {
					fail("region %d,%d: %v", x, y, err)
				}
				if !r.HasData {
					continue
				}
				present++
				nodes += len(r.Ground)
				npcs += len(r.Npcs)
				bios += len(r.Buildins)
				for _, g := range r.Ground {
					images[g.Image]++
				}
				for _, c := range r.Covers {
					images[c.Image]++
				}
				for _, b := range r.Buildins {
					images[b.Image]++
				}
			}
		}
		fmt.Printf("regions with data: %d, ground nodes: %d, npcs: %d, buildin objects: %d, distinct images: %d\n", present, nodes, npcs, bios, len(images))
		missing := 0
		shown := 0
		for img, n := range images {
			resolved, ok := resolveImage(set, img)
			if !ok {
				missing++
			}
			if shown < 12 {
				fmt.Printf("  %-6d %s -> %s\n", n, text.GBKToUTF8([]byte(img)), map[bool]string{true: text.GBKToUTF8([]byte(resolved)), false: "MISSING"}[ok])
				shown++
			}
		}
		fmt.Printf("images missing from archives: %d\n", missing)

	case "export-minimap":
		// The picture the 2.0 minimap draws: "<map root>24.jpg" (KScenePlaceMapC::Load, PLACE_MAP_FILE_NAME_APPEND;
		// gamecl.exe 0x7b580c "%s24.jpg"), kept in data/minimap.pak; one region of the map = 32 x 32 of its pixels
		// (MAP_A_REGION_NUM_MAP_PIXEL_H/V), so a pixel is 16 scene units across and 32 down.
		if len(args) < 2 {
			fail("export-minimap <mapid>|all [-out <assets dir>]")
		}
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		ids := []string{args[1]}
		if args[1] == "all" {
			data, err := mapList(dir, set)
			if err != nil {
				fail("MapList.ini: %v", err)
			}
			ids = nil
			for _, m := range regexp.MustCompile(`(?m)^([0-9]+)=`).FindAllSubmatch(data, -1) {
				ids = append(ids, string(m[1]))
			}
		}
		done, missing := 0, 0
		for _, idText := range ids {
			p, _ := mapPath(dir, set, idText)
			data, err := set.ReadFile(p + "24.jpg")
			if err != nil {
				missing++
				if args[1] != "all" {
					fail("%s24.jpg: %v", p, err)
				}
				continue
			}
			d := filepath.Join(out, "maps", idText)
			if err := os.MkdirAll(d, 0o755); err != nil {
				fail("%v", err)
			}
			if err := os.WriteFile(filepath.Join(d, "minimap.jpg"), data, 0o644); err != nil {
				fail("%v", err)
			}
			done++
		}
		fmt.Println("export-minimap:", done, "anh,", missing, "map khong co anh ->", filepath.Join(out, "maps"))
	case "export-map":
		if len(args) < 2 {
			fail("export-map <mapid> [-out <assets dir>]")
		}
		id, err := strconv.Atoi(args[1])
		if err != nil {
			fail("export-map needs a numeric map id from MapList.ini")
		}
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		p, name := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		var spawn [2]int
		if data, err := mapList(dir, set); err == nil {
			if m := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `_MapPos=(\d+),(\d+)`).FindSubmatch(data); m != nil {
				// MapPos is in minimap pixels; keep zero so the exporter picks a walkable cell
				_ = m
			}
		}
		ex := export.New(set, out)
		defer prepareExporter(ex, dir, set)()
		ex.Settings = mapSettings(dir, set, id)
		info, err := ex.Map(id, name, w, spawn)
		if err != nil {
			fail("%v", err)
		}
		reportFallback(set)
		fmt.Printf("map %d (%s): %dx%d regions (%d with data), scene %dx%d, spawn %d,%d, npcs %d, sprites exported %d (total %d)\n",
			info.ID, info.Name, info.RegionCols, info.RegionRows, len(info.Regions), info.SceneW, info.SceneH, info.Spawn[0], info.Spawn[1], len(info.Npcs), ex.Exported, info.Sprites)
		fmt.Printf("written to %s\n", filepath.Join(out, "maps", strconv.Itoa(id)))

	case "export-all":
		// every "<id>=<path>" of Settings/MapList.ini; sprites are shared across maps so one
		// process with one exporter cache is much faster than one run per map
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		data, err := mapList(dir, set)
		if err != nil {
			fail("MapList.ini: %v", err)
		}
		ex := export.New(set, out)
		defer prepareExporter(ex, dir, set)()
		re := regexp.MustCompile(`(?m)^(\d+)=(.*)$`)
		ok, missing, failed := 0, 0, 0
		start := time.Now()
		for _, m := range re.FindAllSubmatch(data, -1) {
			id, _ := strconv.Atoi(string(m[1]))
			p := mapDirPath(strings.TrimSpace(strings.TrimRight(string(m[2]), "\r")))
			name := ""
			if nm := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `_name=(.*)$`).FindSubmatch(data); nm != nil {
				name = text.TCVN3ToUTF8([]byte(strings.TrimSpace(strings.TrimRight(string(nm[1]), "\r"))))
			}
			w, err := wor.LoadWorld(set, p)
			if err != nil {
				missing++
				log.Warn("asset", "map skipped", log.F("id", id), log.F("name", name), log.F("path", text.GBKToUTF8([]byte(p))), log.F("error", err))
				continue
			}
			t0 := time.Now()
			ex.Settings = mapSettings(dir, set, id)
			info, err := ex.Map(id, name, w, [2]int{})
			if err != nil {
				failed++
				log.Error("asset", "map export failed", log.F("id", id), log.F("name", name), log.F("error", err))
				continue
			}
			ok++
			log.Info("asset", "map exported", log.F("id", id), log.F("name", name), log.F("regions", len(info.Regions)), log.F("npcs", len(info.Npcs)),
				log.F("sprites_total", info.Sprites), log.F("ms", time.Since(t0).Milliseconds()))
		}
		reportFallback(set)
		fmt.Printf("export-all: %d maps ok, %d skipped (no .wor), %d failed, %d sprites, %s\n", ok, missing, failed, ex.Exported, time.Since(start).Round(time.Second))

	case "census":
		// census [-out dir]: open every entry of every archive, say what it is, keep the text ones
		out := *flagOut
		if out == "" {
			out = "build/census"
		}
		set := openSet(findClient())
		defer set.Close()
		cmdCensus(set, out, true)

	case "check-trace":
		// check-trace <trace.tsv>: does this reader find every file the real game found?
		if len(args) < 2 {
			fail("check-trace <trace.tsv>")
		}
		set := openSet(findClient())
		defer set.Close()
		cmdCheckTrace(set, args[1])

	case "cat-id":
		// cat-id <hex id> [-out file]: one entry by its hash, for entries nobody knows the name of
		if len(args) < 2 {
			fail("cat-id <hex id>")
		}
		id64, err := strconv.ParseUint(strings.TrimPrefix(args[1], "0x"), 16, 32)
		if err != nil {
			fail("bad id %q: %v", args[1], err)
		}
		set := openSet(findClient())
		defer set.Close()
		f, e, ok := set.Find(uint32(id64))
		if !ok {
			fail("id %08x is in none of the archives", id64)
		}
		data, err := f.Read(e)
		if err != nil {
			fail("%v", err)
		}
		if *flagOut != "" {
			if err := os.WriteFile(*flagOut, data, 0o644); err != nil {
				fail("%v", err)
			}
			fmt.Printf("id %08x: %d byte tu %s -> %s\n", id64, len(data), filepath.Base(f.Path), *flagOut)
		} else {
			os.Stdout.Write(data)
		}

	case "grep-sections":
		// grep-sections <name>...: the text entries that carry every one of these [sections]
		if len(args) < 2 {
			fail("grep-sections <section>...")
		}
		set := openSet(findClient())
		defer set.Close()
		grepSections(set, args[1:])

	case "scan-text":
		// scan-text: the archives keep no file names, so read every entry and keep the ones that
		// are text.  That is how a client we have no .ini list for gives up its layouts.
		out := *flagOut
		if out == "" {
			out = "build/scan"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		found := set.ScanText()
		if err := os.MkdirAll(out, 0o755); err != nil {
			fail("%v", err)
		}
		var every []string
		for _, t := range found {
			every = append(every, t.Paths...)
		}
		names := set.NameIndex(every)
		fmt.Printf("scan-text: %d tep van ban trong %d kho, %d duong dan duoc goi ten\n", len(found), len(set.Files), len(names))
		for _, t := range found {
			name := names[t.ID]
			label := fmt.Sprintf("%08x", t.ID)
			if name != "" {
				label = strings.NewReplacer(`\`, "_", "/", "_", ":", "_").Replace(text.GBKToUTF8([]byte(name)))
				label = strings.TrimPrefix(label, "_")
			}
			if err := os.WriteFile(filepath.Join(out, label), []byte(text.GBKToUTF8(t.Body)), 0o644); err != nil {
				fail("%v", err)
			}
			if *flagLevel == "debug" || t.Head != "" {
				fmt.Printf("  %-8s %6d  %-12s %s\n", fmt.Sprintf("%08x", t.ID), t.Size,
					text.GBKToUTF8([]byte(t.Head)), text.GBKToUTF8([]byte(name)))
			}
		}
		// the names recovered, so a later run can start from a list instead of guessing
		var lines []string
		for _, n := range names {
			lines = append(lines, text.GBKToUTF8([]byte(n)))
		}
		sort.Strings(lines)
		if err := os.WriteFile(filepath.Join(out, "duong-dan.txt"), []byte(strings.Join(lines, "\n")), 0o644); err != nil {
			fail("%v", err)
		}
		fmt.Printf("ghi ra %s\n", out)

	case "export-items":
		// The item tables (KBasPropTbl.cpp of the old core: MeleeWeapon.txt .. Mask.txt, potion,
		// questkey, townportal, magicattrib, goldequip + magicattrib_ge + suites, magicscript),
		// read by column number the way the old loaders did, one JSON per set: the plain
		// settings/item as "base" and every version folder 000.. of the JX2 server.
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sets := itemTableSets()
		total := 0
		magic := map[string]any{} // the prefix / suffix rows of every set, for the client's tooltip
		for _, s := range sets {
			set, err := item.Load(s.dir, s.version)
			if err != nil {
				fail("%s: %v", s.dir, err)
			}
			p := filepath.Join(out, "items", s.file+".json")
			if err := set.Write(p); err != nil {
				fail("%s: %v", p, err)
			}
			total += set.Count()
			magic[s.file] = set.MagicRows()
			fmt.Printf("  %-6s %5d vat pham, %3d ma phap, %3d hoang kim, %3d bo  -> %s%s\n", s.file, set.Count(), len(set.Magic), len(set.Gold), len(set.Suites), p,
				map[bool]string{true: "  (thieu: " + strings.Join(set.Missing, ",") + ")", false: ""}[len(set.Missing) > 0])
		}
		// items/magic.json: what KItem::GetDesc of the 2.0 client needs to print "[min-max]"
		// after a magic line (KLibOfBPT::GetMagicRange over the m_CMAIT candidates of the piece)
		blob, err := json.Marshal(map[string]any{"source": "settings/item/<version>/magicattrib.txt", "sets": magic})
		if err != nil {
			fail("magic.json: %v", err)
		}
		if err := os.WriteFile(filepath.Join(out, "items", "magic.json"), blob, 0o644); err != nil {
			fail("magic.json: %v", err)
		}
		fmt.Printf("export-items: %d bo, %d dong vat pham; items/magic.json cho chu thich\n", len(sets), total)
		// the CLIENT's own copies (\settings\item\NNN\*.txt inside its archives): the names and
		// descriptions the player reads are the client's - its 004 says "Chủy thủ bằng sắt, sát
		// thương kém." where the server's row says "Loại kiếm nhỏ bằng sắt, khả năng sát thương kém."
		if cdir := findClient(); cdir != "" {
			if set, err := pak.OpenClientSet(cdir); err == nil {
				defer set.Close()
				csets := clientItemSets(set)
				for _, cset := range csets {
					blob, err := json.Marshal(cset.ToDisplay())
					if err != nil {
						fail("client_v%s.json: %v", cset.Version, err)
					}
					p := filepath.Join(out, "items", "client_v"+cset.Version+".json")
					if err := os.WriteFile(p, blob, 0o644); err != nil {
						fail("%s: %v", p, err)
					}
					fmt.Printf("  client %s: %5d vat pham -> %s\n", cset.Version, cset.Count(), p)
				}
				if len(csets) == 0 {
					fmt.Println("  client: khong co \\settings\\item\\NNN trong kho cua client - ten hien ra dung bang server")
				}
			}
		}

	case "export-player":
		// The player tables of the old server (settings/npc/player): the experience of every
		// level, what a level and an attribute point add per series, the resistance per level,
		// stamina.ini, basevalue.ini and the ten new-character templates - read the way KLevelAdd
		// / KPlayerSet of jx_linux_y read them (docs/LINUX-SERVER.md §10.3) -> <out>/player.json
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		var dir string
		for _, root := range serverRoots(sdir) {
			for _, sub := range []string{"settings/npc/player", "Settings/npc/player", "Settings/Npc/Player", "settings/player", "Settings/Player"} {
				if st, err := os.Stat(filepath.Join(root, sub)); err == nil && st.IsDir() {
					dir = filepath.Join(root, sub)
					break
				}
			}
			if dir != "" {
				break
			}
		}
		if dir == "" {
			fail("no settings/npc/player under %s", sdir)
		}
		set, err := player.Load(dir)
		if err != nil {
			fail("%s: %v", dir, err)
		}
		p := filepath.Join(out, "player.json")
		if err := set.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		have := 0
		for i := range set.NewPlayer {
			if set.NewPlayer[i].Present {
				have++
			}
		}
		fmt.Printf("export-player: cap 1..%d (cap 2 can %d, cap 10 can %d), %d he, %d/%d mau nhan vat moi -> %s%s\n",
			player.MaxLevel, set.GetLevelExp(1, 0), set.GetLevelExp(9, 0), len(set.LevelAdd), have, player.NewPlayers, p,
			map[bool]string{true: "  (thieu: " + strings.Join(set.Missing, ",") + ")", false: ""}[len(set.Missing) > 0])

	case "export-skills":
		// \settings\skills.txt of the old server, every row and every column, read the way
		// KSkillManager::Init (jx_linux_y 0x080E7200) and KSkill::GetInfoFromTabFile (0x080E9200)
		// read it -> <out>/skills.json for the zone (docs/LINUX-SERVER.md §11)
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		var file string
		for _, root := range serverRoots(sdir) {
			for _, rel := range []string{"settings/skills.txt", "Settings/Skills.txt", "settings/Skills.txt", "Settings/skills.txt"} {
				if st, err := os.Stat(filepath.Join(root, rel)); err == nil && !st.IsDir() {
					file = filepath.Join(root, rel)
					break
				}
			}
			if file != "" {
				break
			}
		}
		if file == "" {
			fail("no settings/skills.txt under %s", sdir)
		}
		table, err := skill.Load(file)
		if err != nil {
			fail("%s: %v", file, err)
		}
		// attribconstdata.ini next to the table (KSkillManager::Init reads it right after)
		attribConst := ""
		for _, name := range []string{"attribconstdata.ini", "AttribConstData.ini", "Attribconstdata.ini"} {
			if st, err := os.Stat(filepath.Join(filepath.Dir(file), name)); err == nil && !st.IsDir() {
				attribConst = filepath.Join(filepath.Dir(file), name)
				break
			}
		}
		if attribConst != "" {
			if table.AttribData, err = skill.LoadAttribConst(attribConst); err != nil {
				fail("%s: %v", attribConst, err)
			}
		} else {
			fmt.Printf("export-skills: khong thay attribconstdata.ini canh %s (bang tra/bo qua ky nang se trong)\n", file)
		}
		p := filepath.Join(out, "skills.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		scripts := map[string]bool{}
		for _, r := range table.Rows {
			if s := r.Cells["LvlSetScript"]; s != "" {
				scripts[s] = true
			}
		}
		fmt.Printf("export-skills: %d dong ky nang (%d cot, %d script cap, bo qua %d dong, %d muc attribconstdata) tu %s -> %s\n",
			len(table.Rows), len(table.Columns), len(scripts), table.Skipped, len(table.AttribData), file, p)

	case "export-missles":
		// \settings\missles.txt of the old server, every row with a MissleId 1..999 and every
		// column, read the way 0x0805D210 / 0x08074300 of jx_linux_y read it -> <out>/missles.json
		// for the zone's KMissleTable (docs/LINUX-SERVER.md §13)
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		var file string
		for _, root := range serverRoots(sdir) {
			for _, rel := range []string{"settings/missles.txt", "Settings/Missles.txt", "settings/Missles.txt", "Settings/missles.txt"} {
				if st, err := os.Stat(filepath.Join(root, rel)); err == nil && !st.IsDir() {
					file = filepath.Join(root, rel)
					break
				}
			}
			if file != "" {
				break
			}
		}
		if file == "" {
			fail("no settings/missles.txt under %s", sdir)
		}
		table, err := missle.Load(file)
		if err != nil {
			fail("%s: %v", file, err)
		}
		p := filepath.Join(out, "missles.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-missles: %d dong dan (%d cot, bo qua %d dong) tu %s -> %s\n", len(table.Rows), len(table.Columns), table.Skipped, file, p)

	case "export-abrade-rate":
		// \settings\item\AbradeRate.ini of the old server, read the way KItemSet::Init 0x0806E250 of
		// jx_linux_y reads it -> <out>/abrade_rate.json for the zone's KAbradeRate (docs/LINUX-SERVER.md
		// §16.3): how fast a worn piece wears on an attack, a hit, a step
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		data, file, err := readServerFile(sdir, "settings/item/AbradeRate.ini", "Settings/Item/AbradeRate.ini", "settings/Item/AbradeRate.ini", "Settings/item/AbradeRate.ini")
		if err != nil {
			fail("no settings/item/AbradeRate.ini under %s: %v", sdir, err)
		}
		table := item.ParseAbradeRate(data)
		table.Source = file
		p := filepath.Join(out, "abrade_rate.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-abrade-rate: bang hao mon %s -> %s\n", file, p)

	case "export-revive-pos":
		// \settings\revivepos.ini of the old server: the revive / reference points of every map, the
		// way KSubWorldSet 0x080F6D20 of jx_linux_y reads them ("%u" section, "%d" key -> "x,y") plus
		// the "region=a,b" ids of each map -> <out>/revive_pos.json: where a fresh character is born
		// (the gateway's NewRole, KPlayer::LoadFrom 0x080C171D) and where KPlayer::Revive(0) / SetRevPos go
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		data, file, err := readServerFile(sdir, "settings/revivepos.ini", "Settings/revivepos.ini", "Settings/RevivePos.ini", "settings/RevivePos.ini")
		if err != nil {
			fail("no settings/revivepos.ini under %s: %v", sdir, err)
		}
		table := player.ParseRevivePos(data)
		table.Source = file
		p := filepath.Join(out, "revive_pos.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-revive-pos: diem hoi sinh %s (%d map) -> %s\n", file, len(table.Maps), p)

	case "export-chat-cost":
		// \settings\npc\player\chatcost.ini of the old server, read the way 0x080A0C40 of jx_linux_y reads it
		// (sections "0".."4": Level / Money / ManaPercent / StaminaPercent) -> <out>/chat_cost.json for the
		// zone's KChatCostTable (docs/LINUX-SERVER.md §19): what a line on the city / faction / world channels costs
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		data, file, err := readServerFile(sdir, "settings/npc/player/chatcost.ini", "Settings/npc/player/chatcost.ini", "settings/npc/player/ChatCost.ini",
			"Settings/Npc/Player/ChatCost.ini")
		if err != nil {
			fail("no settings/npc/player/chatcost.ini under %s: %v", sdir, err)
		}
		table := player.ParseChatCost(data)
		table.Source = file
		p := filepath.Join(out, "chat_cost.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-chat-cost: chi phi chat %s -> %s\n", file, p)

	case "export-task-def":
		// \settings\task\player_task_def.txt of the old server, read the way the loader 0x081C6E00 of jx_linux_y reads it
		// (rows from the third, columns 1 / 2 / 4 / 5: first id, last id, SYNC_FLAG, CLIENT_FLAG) -> <out>/task_def.json for
		// the zone's KTaskDefTable (docs/LINUX-SERVER.md §21): which task values the client is told about and may set
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		data, file, err := readServerFile(sdir, "settings/task/player_task_def.txt", "Settings/task/player_task_def.txt", "Settings/Task/player_task_def.txt")
		if err != nil {
			fail("no settings/task/player_task_def.txt under %s: %v", sdir, err)
		}
		table := player.ParseTaskDef(data)
		table.Source = file
		p := filepath.Join(out, "task_def.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-task-def: dinh nghia gia tri nhiem vu %s (%d dong, %d id dong bo) -> %s\n", file, len(table.Rows), table.SyncCount(), p)

	case "export-task-tables":
		// \settings\task of the old server the way the task system of jx_linux_y loads it (0x081725F0: task_id.txt, task_type.txt
		// and the condition / entity / award / talk table of every kind, task_event.txt) -> <out>/task_tables.json for the
		// zone's KTaskManager (docs/LINUX-SERVER.md §22): what the TASKSYS library of the scripts reads.  The cells keep the
		// bytes of the files (as Latin-1 runes): the scripts compare them with their own bytes
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		var source string
		read := func(rel string) ([]byte, error) {
			data, file, err := readServerFile(sdir, rel, strings.ToLower(rel))
			if err == nil && source == "" {
				source = filepath.Dir(filepath.Dir(file))
			}
			return data, err
		}
		table, err := player.ParseTaskTables(read)
		if err != nil {
			fail("no settings/task under %s: %v", sdir, err)
		}
		table.Source = source
		p := filepath.Join(out, "task_tables.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-task-tables: he nhiem vu %s (%d nhiem vu, %d loai, %d su kien) -> %s\n", source, len(table.Tasks), len(table.Types), len(table.Events), p)

	case "export-kill-events":
		// \settings\npc\player\event_killnpc.txt of the old server the way 0x08156760 of jx_linux_y reads it -> <out>/kill_events.json
		// for the zone's KKillEventTable (docs/LINUX-SERVER.md §23): what a kill counts for the events a script registered
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		data, file, err := readServerFile(sdir, "settings/npc/player/event_killnpc.txt", "Settings/npc/player/event_killnpc.txt", "Settings/Npc/Player/event_killnpc.txt")
		if err != nil {
			fail("no settings/npc/player/event_killnpc.txt under %s: %v", sdir, err)
		}
		table := player.ParseKillEvents(data)
		table.Source = file
		p := filepath.Join(out, "kill_events.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-kill-events: su kien giet quai %s (%d dong) -> %s\n", file, len(table.Rows), p)

	case "export-faction":
		// \settings\faction\门派设定.ini of the old server: the eleven factions the way
		// KFactionSet::Init 0x08060C70 of jx_linux_y reads them (Name / ShowName / Series / Camp per
		// "%d" section) plus factionskill.txt (the flat FactionId -> SkillId list its scripts load)
		// -> <out>/faction.json for the zone's KFaction (SetFaction of the script api, the camp of a
		// member, GetFaction) and for the client's skill book (the branch pages of the faction)
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		// the file name is Chinese: on a Windows extraction it may carry the GBK bytes as cp1252 letters
		mojibake := gbkAsLatin1("门派设定") + ".ini"
		data, file, err := readServerFile(sdir, "settings/faction/门派设定.ini", "Settings/faction/门派设定.ini",
			"settings/faction/"+mojibake, "Settings/faction/"+mojibake, "settings/faction/faction.ini")
		if err != nil {
			fail("no settings/faction/门派设定.ini under %s: %v", sdir, err)
		}
		table := player.ParseFaction(data)
		table.Source = file
		if skills, sfile, err := readServerFile(sdir, "settings/faction/factionskill.txt", "Settings/faction/factionskill.txt"); err == nil {
			table.AddSkills(skills)
			fmt.Printf("export-faction: danh sach ky nang mon phai %s\n", sfile)
		} else {
			fmt.Printf("export-faction: khong co settings/faction/factionskill.txt (%v) - khong co danh sach ky nang\n", err)
		}
		p := filepath.Join(out, "faction.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		named := 0
		for _, e := range table.Factions {
			if e.Name != "" {
				named++
			}
		}
		fmt.Printf("export-faction: mon phai %s (%d/%d co ten) -> %s\n", file, named, len(table.Factions), p)

	case "export-weapon-skill":
		// \settings\武器物理攻击对照表.txt (DetailType, ParticularType, PhysicsSkillID), read the way 0x0805F18D
		// of jx_linux_y reads it -> <out>/weapon_skill.json for the zone's KWeaponSkillTable
		// (docs/LINUX-SERVER.md §16) and the client's default mouse skills (gamecl.exe 2.0 loads the
		// same file at 0x005CB396 into the tables 0x9bcbf0 / 0x9bca60 / 0x9bca5c; docs/CLIENT-2.0.md
		// §7.1).  The old server folder is tried first, then the 2.0 client's archives (the server
		// on this machine lacks the file, the client carries it).
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		var data []byte
		var file string
		sdir := *flagServer
		if sdir == "" {
			sdir = os.Getenv("JX_OLD_SERVER")
		}
		if sdir == "" {
			sdir = findServer(findClient())
		}
		if sdir != "" {
			data, file, _ = readServerFile(sdir, "settings/武器物理攻击对照表.txt", "Settings/武器物理攻击对照表.txt", "settings/weaponskill.txt")
		}
		if data == nil {
			if cdir := findClient(); cdir != "" {
				set := openSet(cdir)
				p := gamePath("\\settings\\武器物理攻击对照表.txt")
				if f, e, ok := set.Lookup(p); ok {
					if d, err := f.Read(e); err == nil {
						data, file = d, filepath.Base(f.Path)+":"+p
					}
				}
				set.Close()
			}
		}
		if data == nil {
			fail("no settings/武器物理攻击对照表.txt under the old server folder or in the client's archives")
		}
		table := skill.ParseWeaponSkill(data)
		table.Source = file
		p := filepath.Join(out, "weapon_skill.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-weapon-skill: %d dong vu khi -> ky nang (bo qua %d) tu %s -> %s\n", len(table.Rows), table.Skipped, file, p)

	case "export-skill-desc":
		// the texts KSkill::GetDesc 0x006FBC90 of the 2.0 client puts in a skill's tip: the G_* string table
		// \lang\<lang>\stringtable_core.txt (loaded by 0x005DBA60), [Descript] of \settings\magicdesc.ini (KMagicDesc
		// 0x0060A2B0) and [SkillAttrib] / [SkillType] / [WeaponLimit] of \settings\gamesetting.ini (docs/CLIENT-2.0.md §10)
		// -> <out>/text/skill_desc.json.  The Vietnamese files are TCVN3; the ini keys come out lower-cased (ParseIni).
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		read := func(p string) []byte {
			f, e, ok := set.Lookup(gamePath(p))
			if !ok {
				fail("not found in the client's archives: %s", p)
			}
			data, err := f.Read(e)
			if err != nil {
				fail("%s: %v", p, err)
			}
			return data
		}
		decode := func(b []byte) string {
			b = bytes.TrimSpace(b)
			if text.IsTCVN3(b) {
				return text.TCVN3ToUTF8(b)
			}
			return text.DecodeMixed(b)
		}
		strTable := map[string]string{}
		for i, line := range bytes.Split(read("\\lang\\"+*flagLang+"\\stringtable_core.txt"), []byte{'\n'}) {
			line = bytes.TrimRight(line, "\r")
			tab := bytes.IndexByte(line, '\t')
			if tab <= 0 || (i == 0 && string(line[:tab]) == "key") {
				continue
			}
			// the table writes a line break as the two characters backslash-n (G_Skills_35 " (Cong kich gan ) \n"):
			// KStringTable of the client turns them into real breaks before sprintf sees them
			strTable[string(line[:tab])] = strings.ReplaceAll(decode(line[tab+1:]), "\\n", "\n")
		}
		section := func(ini map[string]map[string]string, name string) map[string]string {
			m := map[string]string{}
			for k, v := range ini[strings.ToLower(name)] {
				m[k] = decode([]byte(v))
			}
			return m
		}
		magic := npcres.ParseIni(read("\\settings\\magicdesc.ini"))
		game := npcres.ParseIni(read("\\settings\\gamesetting.ini"))
		descript := section(magic, "Descript")
		skillAttrib := section(game, "SkillAttrib")
		weaponLimit := section(game, "WeaponLimit")
		doc := map[string]any{
			"source":       "the client's archives: \\lang\\" + *flagLang + "\\stringtable_core.txt, \\settings\\magicdesc.ini, \\settings\\gamesetting.ini",
			"lang":         *flagLang,
			"strings":      strTable,
			"descript":     descript,
			"skill_attrib": skillAttrib,
			"skill_type":   section(game, "SkillType"),
			"weapon_limit": weaponLimit,
		}
		p := filepath.Join(out, "text", "skill_desc.json")
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err != nil {
			fail("%v", err)
		}
		data, err := json.MarshalIndent(doc, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(p, data, 0o644); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-skill-desc: %d chuoi G_*, %d mo ta thuoc tinh, %d SkillAttrib, %d WeaponLimit -> %s\n", len(strTable), len(descript), len(skillAttrib), len(weaponLimit), p)

	case "export-missle-res":
		// the drawing side of \settings\missles.txt for the client (KMissle::Init of the old core, KMissle.cpp 242..282:
		// AnimFile1..4 / AnimFileInfo "frames,dirs,interval" / SndFile1..4 for the statuses wait, fly, vanish, collision,
		// the B set MultiShow picks at random, LoopPlay, SubLoop, SubStart, SubStop) -> <out>/missles/missle_res.json,
		// plus the sprites of the missiles the skills of <out>/skills.json fire (their ChildSkillId).  docs/CLIENT-2.0.md §11
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		f, e, ok := set.Lookup(gamePath("\\settings\\missles.txt"))
		if !ok {
			fail("no settings/missles.txt in the client's archives")
		}
		data, err := f.Read(e)
		if err != nil {
			fail("missles.txt: %v", err)
		}
		tab := npcres.ParseTab(data)
		want := map[int]bool{}
		if sk, err := os.ReadFile(filepath.Join(out, "skills.json")); err == nil {
			var doc struct {
				Rows []struct {
					Cells map[string]string `json:"cells"`
				} `json:"rows"`
			}
			if json.Unmarshal(sk, &doc) == nil {
				for _, r := range doc.Rows {
					if v, err := strconv.Atoi(strings.TrimSpace(r.Cells["ChildSkillId"])); err == nil && v > 0 {
						want[v] = true
					}
				}
			}
		}
		ex := export.New(set, out)
		type anim struct {
			Sprite   string `json:"sprite"`
			Frames   int    `json:"frames"`
			Dirs     int    `json:"dirs"`
			Interval int    `json:"interval"`
			Sound    string `json:"sound"`
		}
		parseInfo := func(s string) (int, int, int) {
			parts := strings.Split(strings.Trim(strings.TrimSpace(s), "\""), ",")
			get := func(i, def int) int {
				if i < len(parts) {
					if v, err := strconv.Atoi(strings.TrimSpace(parts[i])); err == nil {
						return v
					}
				}
				return def
			}
			frames, dirs, interval := get(0, 100), get(1, 16), get(2, 1)
			if interval <= 0 {
				interval = 1
			}
			return frames, dirs, interval
		}
		rows := map[string]any{}
		sprites := 0
		for r := 2; r <= tab.Height(); r++ {
			id, err := strconv.Atoi(strings.TrimSpace(tab.GetByName(r, "MissleId")))
			if err != nil || id <= 0 {
				continue
			}
			if len(want) > 0 && !want[id] {
				continue
			}
			mk := func(prefix string) []anim {
				list := make([]anim, 4)
				for i := 0; i < 4; i++ {
					n := strconv.Itoa(i + 1)
					path := strings.TrimSpace(tab.GetByName(r, "AnimFile"+prefix+n))
					if path == "" {
						continue
					}
					frames, dirs, interval := parseInfo(tab.GetByName(r, "AnimFileInfo"+prefix+n))
					sid := ex.SpriteID(path)
					if sid != "" {
						sprites++
					}
					list[i] = anim{Sprite: sid, Frames: frames, Dirs: dirs, Interval: interval, Sound: text.DecodeMixed([]byte(tab.GetByName(r, "SndFile"+prefix+n)))}
				}
				return list
			}
			num := func(col string) int {
				v, _ := strconv.Atoi(strings.TrimSpace(tab.GetByName(r, col)))
				return v
			}
			rows[strconv.Itoa(id)] = map[string]any{
				"name": text.DecodeMixed([]byte(tab.GetByName(r, "MissleName"))), "loop": num("LoopPlay") != 0, "sub_loop": num("SubLoop") != 0,
				"sub_start": num("SubStart"), "sub_stop": num("SubStop"), "multi_show": num("MultiShow") != 0, "height": num("MissleHeight"),
				"anims": mk(""), "anims_b": mk("B"),
			}
		}
		p := filepath.Join(out, "missles", "missle_res.json")
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err != nil {
			fail("%v", err)
		}
		doc := map[string]any{"source": "the client's archives: \\settings\\missles.txt (the drawing columns)", "rows": rows}
		js, err := json.MarshalIndent(doc, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(p, js, 0o644); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-missle-res: %d dan (theo ChildSkillId cua %d ky nang), %d sprite (%d moi) -> %s\n", len(rows), len(want), sprites, ex.Exported, p)

	case "export-state-gfx":
		// the state pictures (\settings\npcres\状态图形对照表.txt, CStateMagicTable of the old client, loader 0x006AE200 of
		// gamecl.exe 2.0): one row per StateSpecialId - the sprite, Head / Body / Foot / MiniMap, Loop, the frames a Body
		// picture spends behind the character, frames / dirs / interval, split -> <out>/npcres/state_gfx.json, plus the
		// sprites of the ids the skills of <out>/skills.json name (all of them with -all).  docs/CLIENT-2.0.md §14
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		data, err := set.ReadFile(gamePath(npcres.StateMagicTable))
		if err != nil {
			fail("no settings/npcres/状态图形对照表.txt in the client's archives: %v", err)
		}
		table := npcres.ParseStateMagicTable(data)
		want := map[int]bool{}
		if sk, err := os.ReadFile(filepath.Join(out, "skills.json")); err == nil {
			var doc struct {
				Rows []struct {
					Cells map[string]string `json:"cells"`
				} `json:"rows"`
			}
			if json.Unmarshal(sk, &doc) == nil {
				for _, r := range doc.Rows {
					if v, err := strconv.Atoi(strings.TrimSpace(r.Cells["StateSpecialId"])); err == nil && v > 0 {
						want[v] = true
					}
				}
			}
		}
		ex := export.New(set, out)
		rows := map[string]any{}
		sprites, special := 0, 0
		for _, m := range table {
			sid := ""
			if m.IsSpecial() {
				special++
			} else if *flagAll || want[m.ID] {
				sid = ex.SpriteID(m.File)
				if sid != "" {
					sprites++
				}
			}
			rows[strconv.Itoa(m.ID)] = map[string]any{
				"name": m.Name, "type": m.Type, "special": m.IsSpecial(), "sprite": sid, "file": text.GBKToUTF8([]byte(m.File)),
				"loop": m.Loop, "behind_start": m.BackStart, "behind_end": m.BackEnd, "frames": m.Frames, "dirs": m.Dirs,
				"interval": m.Interval, "split": m.Split,
			}
		}
		p := filepath.Join(out, "npcres", "state_gfx.json")
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err != nil {
			fail("%v", err)
		}
		doc := map[string]any{
			"source": "the client's archives: " + npcres.StateMagicTable + " (CStateMagicTable; gamecl.exe 2.0 loader 0x006AE200)",
			"types":  map[string]int{"head": npcres.StateMagicHead, "body": npcres.StateMagicBody, "foot": npcres.StateMagicFoot, "minimap": npcres.StateMagicMiniMap},
			"rows":   rows,
		}
		js, err := json.MarshalIndent(doc, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(p, js, 0o644); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-state-gfx: %d trang thai (%d Special, %d ky nang dung), %d sprite (%d moi) -> %s\n", len(rows), special, len(want), sprites, ex.Exported, p)

	case "export-menu-state":
		// \settings\npcres\界面状态与图形对照表.txt of the client (KPlayerMenuStateGraph::Init, gamecl.exe 2.0 0x00703F40 /
		// 0x00704102): the sign over a player's head by menu state - row 1 team open (menustate01), 2 trade open (02),
		// 3 trading (03), 4 sleeping (04), 5 stall (02), 6 stall trading (03) -> <out>/npcres/menu_state.json + the sprites.
		// docs/CLIENT-2.0.md §22
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		const table = `\settings\npcres\界面状态与图形对照表.txt`
		data, err := set.ReadFile(gamePath(table))
		if err != nil {
			fail("no %s in the client's archives: %v", table, err)
		}
		ex := export.New(set, out)
		rows := map[string]any{}
		state := 0
		for i, line := range strings.Split(string(data), "\n") {
			line = strings.TrimRight(line, "\r")
			if i == 0 || strings.TrimSpace(line) == "" {
				continue
			}
			cells := strings.Split(line, "\t")
			if len(cells) < 2 {
				continue
			}
			state++
			file := strings.TrimSpace(cells[1])
			rows[strconv.Itoa(state)] = map[string]any{
				"name": text.GBKToUTF8([]byte(strings.TrimSpace(cells[0]))), "file": text.GBKToUTF8([]byte(file)), "sprite": ex.SpriteID(file),
			}
		}
		p := filepath.Join(out, "npcres", "menu_state.json")
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err != nil {
			fail("%v", err)
		}
		js, err := json.MarshalIndent(map[string]any{"source": "the client's archives: " + table + " (KPlayerMenuStateGraph, gamecl.exe 2.0 0x00703F40)", "rows": rows}, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(p, js, 0o644); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-menu-state: %d trang thai, %d sprite -> %s\n", len(rows), ex.Exported, p)

	case "export-npc-gold":
		// \settings\npc\NpcGoldTemplate.txt of the old server (KNpcGoldTemplate::Init jx_linux_y 0x0809CCC0): the kinds of
		// gold monster a placed monster rolls when it revives (KNpcGold::SetGoldTypeAndBackData 0x0809D8D0) -> <out>/npcres/
		// npc_gold.json for the zone; the 2.0 client's own copy of the table (gamecl.exe 0x006E35C0) is counted for the
		// colour of the name (0x005F2401: a kind above its count is drawn as a boss).  docs/LINUX-SERVER.md §16.12
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		sdir := findServer(dir)
		var lookup npcres.NpcGoldSkillLookup
		if sdir != "" {
			if sk, _, err := readServerFile(sdir, `settings\skills.txt`, `Settings\Skills.txt`); err == nil {
				lookup = npcres.SkillNameLookup(sk)
			}
		}
		if lookup == nil {
			if sk, err := set.ReadFile(gamePath(npcres.SkillFile)); err == nil {
				lookup = npcres.SkillNameLookup(sk)
			}
		}
		source := ""
		var rows []npcres.NpcGoldTemplate
		if sdir != "" {
			if data, p, err := readServerFile(sdir, `settings\npc\npcgoldtemplate.txt`, `settings\npc\NpcGoldTemplate.txt`); err == nil {
				rows = npcres.ParseNpcGoldTemplate(data, lookup)
				source = p
			}
		}
		clientRows := 0
		if data, err := set.ReadFile(gamePath(npcres.NpcGoldTemplateFile)); err == nil {
			client := npcres.ParseNpcGoldTemplate(data, lookup)
			clientRows = len(client)
			if rows == nil {
				rows = client
				source = "the client's archives: " + npcres.NpcGoldTemplateFile
			}
		}
		if rows == nil {
			fail("no settings/npc/NpcGoldTemplate.txt under %s nor in the client's archives", sdir)
		}
		p := filepath.Join(out, "npcres", "npc_gold.json")
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err != nil {
			fail("%v", err)
		}
		doc := map[string]any{
			"source":      source + " (KNpcGoldTemplate::Init jx_linux_y 0x0809CCC0)",
			"rows":        rows,
			"client_rows": clientRows,
		}
		js, err := json.MarshalIndent(doc, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(p, js, 0o644); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-npc-gold: %d loai quai vang (client %d) -> %s\n", len(rows), clientRows, p)

	case "export-sounds":
		// the sounds the fights play (KSoundCache / KWavSound of the old engine): the cast sounds of skills.txt
		// (ManCastSnd / FMCastSnd, KSkill::PlayCastSound gamecl.exe 0x006F6D90), the missile sounds of missles.txt
		// (SndFile1..4 / SndFileB1..4, KMissleRes::PlaySound 0x00717ED0) and the action sounds of the characters
		// (主角动作声音表.txt for MainMan / MainLady, npc动作声音表.txt for the npcs whose appearance <out>/npcres/res holds;
		// KNpcRes::PlaySound 0x006DFA20), copied out of the archives as they are (PCM .wav) -> <out>/sounds/<id>.wav,
		// listed by game path in <out>/sounds/sounds.json; the action tables -> <out>/npcres/action_sounds.json.
		// docs/CLIENT-2.0.md §15
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		want := map[string]bool{}
		add := func(s string) {
			s = strings.TrimSpace(s)
			if s == "" || s == "0" {
				return
			}
			want[strings.ToLower(s)] = true
		}
		if sk, err := os.ReadFile(filepath.Join(out, "skills.json")); err == nil {
			var doc struct {
				Rows []struct {
					Cells map[string]string `json:"cells"`
				} `json:"rows"`
			}
			if json.Unmarshal(sk, &doc) == nil {
				for _, r := range doc.Rows {
					add(r.Cells["ManCastSnd"])
					add(r.Cells["FMCastSnd"])
				}
			}
		}
		if mr, err := os.ReadFile(filepath.Join(out, "missles", "missle_res.json")); err == nil {
			var doc struct {
				Rows map[string]struct {
					Anims []struct {
						Sound string `json:"sound"`
					} `json:"anims"`
					AnimsB []struct {
						Sound string `json:"sound"`
					} `json:"anims_b"`
				} `json:"rows"`
			}
			if json.Unmarshal(mr, &doc) == nil {
				for _, r := range doc.Rows {
					for _, a := range r.Anims {
						add(a.Sound)
					}
					for _, a := range r.AnimsB {
						add(a.Sound)
					}
				}
			}
		}
		// the action sounds: the two main characters and every exported npc appearance (npcres/res/<name>.json)
		actionSounds := map[string]any{}
		if data, err := set.ReadFile(gamePath(npcres.PlayerSoundFile)); err == nil {
			player := npcres.ParsePlayerSoundTable(data)
			for _, m := range player {
				for _, p := range m {
					add(p)
				}
			}
			actionSounds["player"] = player
		} else {
			log.Warn("asset", "player action sound table missing", log.F("file", npcres.PlayerSoundFile))
		}
		if data, err := set.ReadFile(gamePath(npcres.NpcSoundFile)); err == nil {
			all := npcres.ParseNpcSoundTable(data)
			kept := map[string]map[string]string{}
			if entries, err := os.ReadDir(filepath.Join(out, "npcres", "res")); err == nil {
				for _, e := range entries {
					name := strings.TrimSuffix(e.Name(), ".json")
					if m, ok := all[name]; ok {
						kept[name] = m
						for _, p := range m {
							add(p)
						}
					}
				}
			}
			actionSounds["npc"] = kept
		} else {
			log.Warn("asset", "npc action sound table missing", log.F("file", npcres.NpcSoundFile))
		}
		dir := filepath.Join(out, "sounds")
		if err := os.MkdirAll(dir, 0o755); err != nil {
			fail("%v", err)
		}
		files := map[string]string{}
		written, missing := 0, 0
		paths := make([]string, 0, len(want))
		for p := range want {
			paths = append(paths, p)
		}
		sort.Strings(paths)
		for _, p := range paths {
			g, err := text.UTF8ToGBK(p)
			if err != nil {
				log.Warn("asset", "sound path not GBK", log.F("path", p))
				missing++
				continue
			}
			f, e, ok := set.Lookup(string(g))
			if !ok {
				log.Warn("asset", "sound missing", log.F("path", p))
				missing++
				continue
			}
			id := fmt.Sprintf("%08x", e.ID)
			ext := strings.ToLower(filepath.Ext(p))
			if ext != ".wav" && ext != ".mp3" && ext != ".ogg" {
				ext = ".wav"
			}
			target := filepath.Join(dir, id+ext)
			if _, err := os.Stat(target); err != nil {
				data, err := f.Read(e)
				if err != nil {
					log.Warn("asset", "sound read failed", log.F("path", p), log.F("error", err))
					missing++
					continue
				}
				if err := os.WriteFile(target, data, 0o644); err != nil {
					fail("%s: %v", target, err)
				}
				written++
			}
			files[p] = id + ext
		}
		doc := map[string]any{
			"source": "the client's archives (sound.pak): ManCastSnd / FMCastSnd of skills.txt, SndFile1..4 / SndFileB1..4 of missles.txt",
			"files":  files,
		}
		js, err := json.MarshalIndent(doc, "", "  ")
		if err != nil {
			fail("%v", err)
		}
		if err := os.WriteFile(filepath.Join(dir, "sounds.json"), js, 0o644); err != nil {
			fail("%v", err)
		}
		if len(actionSounds) > 0 {
			actionSounds["source"] = "the client's archives: " + npcres.PlayerSoundFile + " (MainMan / MainLady by action name), " + npcres.NpcSoundFile + " (npc resource by action name); paths under \\sound\\ (KNpcResNode::ComposePathAndName)"
			ajs, err := json.MarshalIndent(actionSounds, "", "  ")
			if err != nil {
				fail("%v", err)
			}
			if err := os.MkdirAll(filepath.Join(out, "npcres"), 0o755); err != nil {
				fail("%v", err)
			}
			if err := os.WriteFile(filepath.Join(out, "npcres", "action_sounds.json"), ajs, 0o644); err != nil {
				fail("%v", err)
			}
		}
		fmt.Printf("export-sounds: %d tieng (%d moi, %d thieu) -> %s; hanh dong -> npcres/action_sounds.json\n", len(files), written, missing, filepath.Join(dir, "sounds.json"))

	case "export-objdata":
		// The objects of the ground (\settings\obj\ObjData.txt + MoneyObj.txt of the old server):
		// data for the zone, sprites for the client -> <out>/objdata.json, <out>/sprites.  With
		// -all every row's picture is written; by default the rows the item tables and the money
		// piles use (a dropped sword, a pile of coins).
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		sdir := findServer(dir)
		if sdir == "" {
			fail("no old server folder: -server, JX_OLD_SERVER or config/oldgame.local.json")
		}
		objData, p1, err := readServerFile(sdir, "settings/obj/ObjData.txt", "Settings/Obj/ObjData.txt")
		if err != nil {
			fail("ObjData.txt: %v", err)
		}
		moneyObj, _, err := readServerFile(sdir, "settings/obj/MoneyObj.txt", "Settings/Obj/MoneyObj.txt")
		if err != nil {
			fail("MoneyObj.txt: %v", err)
		}
		var only map[int]bool
		if !*flagAll {
			only = map[int]bool{}
			for _, s := range itemTableSets() {
				set, err := item.Load(s.dir, s.version)
				if err != nil {
					fail("%s: %v", s.dir, err)
				}
				for _, id := range set.ObjIDs() {
					only[id] = true
				}
			}
			for _, line := range strings.Split(string(moneyObj), "\n")[1:] {
				cols := strings.Split(strings.TrimSpace(line), "\t")
				if len(cols) >= 2 {
					if id, err := strconv.Atoi(strings.TrimSpace(cols[1])); err == nil {
						only[id] = true
					}
				}
			}
		}
		set := openSet(dir)
		defer set.Close()
		ex := export.New(set, out)
		bundle, err := ex.ObjData(objData, moneyObj, only)
		if err != nil {
			fail("%v", err)
		}
		fmt.Printf("export-objdata: %d doi tuong (%s), %d muc tien, %d sprite -> %s\n", len(bundle.Objects), p1, len(bundle.Money), ex.Exported, filepath.Join(out, "objdata.json"))

	case "export-item-images":
		// The sprite of every item the tables name (the 动画文件名 column), out of the old client's
		// archives, as atlas .png + items/images.json - what the bag and the equipment window draw.
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		var paths []string
		for _, s := range itemTableSets() {
			set, err := item.Load(s.dir, s.version)
			if err != nil {
				fail("%s: %v", s.dir, err)
			}
			paths = append(paths, set.ImagePaths()...)
		}
		set := openSet(findClient())
		defer set.Close()
		// the client's own tables name their pictures too (items/client_vNNN.json)
		for _, cset := range clientItemSets(set) {
			paths = append(paths, cset.ImagePaths()...)
		}
		ex := export.New(set, out)
		written, missing, err := ex.ItemImages(paths)
		if err != nil {
			fail("%v", err)
		}
		fmt.Printf("export-item-images: %d anh ghi ra %s, %d anh client khong co (items/images.json)\n", written, filepath.Join(out, "items", "images"), missing)

	case "export-skill-ui":
		// \settings\skillui\skillui.txt of the 2.0 client (its archives): where the skill book shows
		// each skill - faction, branch, then 12 tiers x 3 slots of skill ids, read the way gamecl.exe
		// 0x00607860 reads it -> <out>/skill_ui.json for the Godot skill book (UiSkills.gd)
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		set := openSet(findClient())
		defer set.Close()
		const skillUiFile = `\settings\skillui\skillui.txt`
		data, err := set.ReadFile(skillUiFile)
		if err != nil {
			fail("no %s in the client's archives (a VLTK 2.0 client): %v", skillUiFile, err)
		}
		table := skill.ParseSkillUi(data)
		table.Source = skillUiFile
		p := filepath.Join(out, "skill_ui.json")
		if err := table.Write(p); err != nil {
			fail("%s: %v", p, err)
		}
		fmt.Printf("export-skill-ui: %d dong (%d ky nang co cho) -> %s\n", len(table.Rows), len(table.Place), p)

	case "export-skill-images":
		// The icon of every skill (the SkillIcon column of settings/skills.txt, `\spr\Ui\技能图标\...spr`)
		// out of the old client's archives, into the same items/images store the bag icons use
		// (KUiSkills / KUiPlayerBar draw them through KUiBase::GetObjImage as well).  Reads the
		// skills.json export-skills wrote next to it.
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		table, err := skill.Read(filepath.Join(out, "skills.json"))
		if err != nil {
			fail("no skills.json under %s (run export-skills first): %v", out, err)
		}
		var paths []string
		seen := map[string]bool{}
		for _, r := range table.Rows {
			p := r.Cells["SkillIcon"]
			if p == "" || seen[p] {
				continue
			}
			seen[p] = true
			paths = append(paths, p)
		}
		set := openSet(findClient())
		defer set.Close()
		ex := export.New(set, out)
		written, missing, err := ex.ItemImages(paths)
		if err != nil {
			fail("%v", err)
		}
		fmt.Printf("export-skill-images: %d bieu tuong ky nang ghi vao %s, %d anh client khong co\n", written, filepath.Join(out, "items", "images"), missing)

	case "export-ui":
		// The windows of the login flow, as JSON layouts plus the pictures they name.
		//
		// Each window is read BY THE NAME THE GAME ASKS FOR - <theme>\UiNewLogin\<file>.ini, the
		// names KUi*::LoadScheme of gamecl.exe passes to KIniFile::Load - out of the nested
		// archive \reslst.dat (docs/VLTK20-CLIENT.md).  The theme folder comes from
		// \Ui\Setting.ini like in the game; -theme 800 picks the small one.
		out := *flagOut
		if out == "" {
			out = "client/assets"
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		ex := export.New(set, out)
		theme, width, height := ex.UiTheme(*flagTheme)
		fmt.Printf("theme %s (%dx%d)\n", theme, width, height)
		ok, missing := 0, 0
		for _, def := range append(append([]export.UiScreenDef{}, export.LoginScreens...), export.GameScreens...) {
			screen, err := ex.UiByName(def, theme, width, height)
			if err != nil {
				missing++
				fmt.Printf("  %-20s %-20s THIEU: %v\n", def.Name, def.Class, err)
				continue
			}
			ok++
			pictures, frames := 0, 0
			for _, w := range screen.Widgets {
				for _, img := range w.Images {
					pictures++
					frames += len(img.Frames)
				}
			}
			fmt.Printf("  %-20s %-20s %2d o, %2d anh (%3d khung hinh), %2d dang nhan vat\n", def.Name, def.Class, len(screen.Widgets), pictures, frames, len(screen.Portraits))
		}
		// the data those windows show: starting villages, the five elements, the login messages
		for _, t := range []struct{ name, path string }{
			{"tan-thu-thon", `\Settings\NativePlaceList.ini`},
			{"ngu-hanh", `\Ui\五行.ini`},
			{"thong-diep", `\Ui\Setting.ini`},
			// the theme's shared settings (KUiBase::Init: fonts, [ObjContColor] of the item cells, cursors)
			{"cong-cong", theme + `\公共.ini`},
			// KMagicDesc: the sentence of every attribute an item can carry ("#d1+" = value 1 with its sign)
			{"mo-ta-ma-phap", `\settings\magicdesc.ini`},
		} {
			n, err := ex.UiTable(t.name, t.path)
			if err != nil {
				missing++
				fmt.Printf("  %-20s THIEU: %v\n", t.name, err)
				continue
			}
			fmt.Printf("  %-20s %d muc  <- %s\n", t.name, n, t.path)
		}
		// the attribute ids -> names of the JX2 build, the key into mo-ta-ma-phap
		if err := ex.UiNames("ten-ma-phap", item.MagicAttribNames); err != nil {
			fmt.Printf("  %-20s LOI: %v\n", "ten-ma-phap", err)
		} else {
			fmt.Printf("  %-20s %d ten  <- jx_linux_y KMagicDesc\n", "ten-ma-phap", len(item.MagicAttribNames))
		}
		// the fixed texts the windows look up by key, and the bitmap fonts all text is drawn with
		if n, err := ex.UiStrings("chuoi-client", `\lang\vn\stringtable_client.txt`); err != nil {
			fmt.Printf("  %-20s THIEU: %v\n", "chuoi-client", err)
		} else {
			fmt.Printf("  %-20s %d chuoi  <- %s\n", "chuoi-client", n, `\lang\vn\stringtable_client.txt`)
		}
		// the core's own texts (KItem::GetDesc reads G_ITEM_* and G_S_* from it: the item tooltip)
		if n, err := ex.UiStrings("chuoi-core", `\lang\vn\stringtable_core.txt`); err != nil {
			fmt.Printf("  %-20s THIEU: %v\n", "chuoi-core", err)
		} else {
			fmt.Printf("  %-20s %d chuoi  <- %s\n", "chuoi-core", n, `\lang\vn\stringtable_core.txt`)
		}
		fonts, err := ex.UiFonts(theme)
		if err != nil {
			missing++
			fmt.Printf("  %-20s THIEU: %v\n", "font", err)
		}
		for _, f := range fonts {
			if f.SameAs > 0 {
				fmt.Printf("  font %-15d dung chung net chu voi co %d\n", f.Size, f.SameAs)
			} else {
				fmt.Printf("  font %-15d o %dx%d, moi chu cach %d px, %d chu  <- %s\n", f.Size, f.CellW, f.CellH, f.Advance, f.Glyphs, f.GamePath)
			}
		}
		reportFallback(set)
		fmt.Printf("export-ui: %d man, %d thieu, %d anh -> %s\n", ok, missing, ex.Exported, filepath.Join(out, "ui"))
		if ok == 0 {
			os.Exit(1)
		}

	case "export-item-res":
		// export-item-res: the five appearance tables of KItemSet (0x08068D90: MeleeRes +0, RangeRes +0x20, ArmorRes +0x40,
		// HelmRes +0x60, HorseRes +0x80) and goldequipres.txt as the zone reads them (KTabFile::GetInteger by row / column,
		// KItemChangeRes 0x08068AC0) -> <out>/items/item_res.json
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		if *flagOut == "" {
			fail("export-item-res needs -out <client/assets>")
		}
		var server *pak.Set
		if sdir := findServer(dir); sdir != "" {
			if s, err := openServerSet(sdir); err == nil {
				server = s
				defer server.Close()
			}
		}
		readAny := func(p string) ([]byte, error) {
			if server != nil {
				if data, err := server.ReadFile(p); err == nil {
					return data, nil
				}
			}
			return set.ReadFile(p)
		}
		out := map[string]any{}
		for _, t := range []struct{ key, path string }{
			{"melee", npcres.MeleeResFile}, {"range", npcres.RangeResFile}, {"armor", npcres.ArmorResFile},
			{"helm", npcres.HelmResFile}, {"horse", npcres.HorseResFile}, {"gold", npcres.GoldResFile},
		} {
			data, err := readAny(t.path)
			if err != nil {
				fail("%s: %v", t.path, err)
			}
			out[t.key] = npcres.ParseTab(data).IntRows()
		}
		if err := os.MkdirAll(filepath.Join(*flagOut, "items"), 0o755); err != nil {
			fail("%v", err)
		}
		data, _ := json.MarshalIndent(out, "", " ")
		if err := os.WriteFile(filepath.Join(*flagOut, "items", "item_res.json"), data, 0o644); err != nil {
			fail("%v", err)
		}
		fmt.Printf("item res: melee %d, range %d, armor %d, helm %d, horse %d, gold %d rows -> %s\n",
			len(out["melee"].([][]int)), len(out["range"].([][]int)), len(out["armor"].([][]int)), len(out["helm"].([][]int)),
			len(out["horse"].([][]int)), len(out["gold"].([][]int)), filepath.Join(*flagOut, "items", "item_res.json"))
	case "export-npcres":
		// export-npcres [mapid...]: appearance tables (npcs.txt, Settings/npcres) plus the sprites
		// of the npcs placed on those maps and of the two main characters -> <out>/npcres, <out>/sprites
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		if *flagOut == "" {
			fail("export-npcres needs -out <client/assets>")
		}
		list, err := npcres.Load(set.ReadFile)
		if err != nil {
			fail("%v", err)
		}
		templates := loadTemplates(set, findServer(dir))
		if templates == nil {
			fail("npcs.txt: not in the server folder nor in the client archives")
		}
		player := map[string]npcres.PlayerFrames{}
		if data, err := set.ReadFile(gamePath(npcres.PlayerBaseFile)); err == nil {
			player = npcres.ParsePlayerBase(data)
		}
		ids := args[1:]
		if len(ids) == 0 {
			ids = []string{"1"}
		}
		var placed []int
		var server *pak.Set
		if sdir := findServer(dir); sdir != "" {
			if s, err := openServerSet(sdir); err == nil {
				server = s
				defer server.Close()
			}
		}
		for _, arg := range ids {
			p, _ := mapPath(dir, set, arg)
			w, err := wor.LoadWorld(set, p)
			if err != nil {
				fail("%v", err)
			}
			for y := w.Top; y <= w.Bottom; y++ {
				for x := w.Left; x <= w.Right; x++ {
					r, err := wor.LoadRegion(set, w, x, y)
					if err != nil {
						fail("region %d,%d: %v", x, y, err)
					}
					for _, n := range r.Npcs {
						placed = append(placed, int(n.TemplateID))
					}
					if server != nil {
						sr, err := wor.LoadServerRegion(server, w, x, y)
						if err != nil {
							fail("server region %d,%d: %v", x, y, err)
						}
						for _, n := range sr.Npcs {
							placed = append(placed, int(n.TemplateID))
						}
					}
				}
			}
		}
		for _, s := range strings.Split(*flagTpl, ",") {
			if id, err := strconv.Atoi(strings.TrimSpace(s)); err == nil {
				placed = append(placed, id)
			}
		}
		names := append([]string{"MainMan", "MainLady"}, export.ResNamesOf(templates, placed)...)
		e := export.New(set, *flagOut)
		// what a character wearing nothing looks like: g_ItemChangeRes (KItemList.cpp:1054)
		icr, err := npcres.LoadItemChangeRes(set.ReadFile)
		if err != nil {
			fail("item appearance tables: %v", err)
		}
		opt := export.NpcResOptions{
			DropRates: loadDropRates(templates, findServer(dir), mapListDropRates(dir, set)),
			Names:     names,
			Doings:    []int{npcres.DoStand, npcres.DoStand1, npcres.DoWalk, npcres.DoRun, npcres.DoHurt, npcres.DoDeath, npcres.DoAttack, npcres.DoAttack1, npcres.DoSit},
			Equips:    icr.DefaultEquips(),
			// the rows of -equip-rows (default: weapon rows 1 / 2 = the level 1..5 melee weapons of MeleeRes.txt, horse row 8 =
			// every "normal horse" of HorseRes.txt (col 2 = 10))
			ExtraEquips: equipRows(*flagRows, icr),
			Skills:      loadSkills(set, findServer(dir)),
		}
		n, err := e.NpcRes(list, templates, player, opt)
		if err != nil {
			fail("%v", err)
		}
		reportFallback(set)
		fmt.Printf("npcres: %d resources for %d placed npcs (%d templates), sprites exported %d\nwritten to %s\n",
			n, len(placed), len(templates), e.Exported, filepath.Join(*flagOut, "npcres"))
	case "traps":
		// traps <mapid|gamepath>: the trap cells of the server region files (Region_S.dat, KRegion::LoadServerTrap)
		// and the script each id resolves to under <server>/script (g_FileName2Id of the game path)
		if len(args) < 2 {
			fail("traps <mapid|gamepath>")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		p, _ := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		sdir := findServer(dir)
		if sdir == "" {
			fail("traps need the old server folder (-server)")
		}
		server, err := openServerSet(sdir)
		if err != nil {
			fail("server archive: %v", err)
		}
		defer server.Close()
		names := scriptIndex(sdir)
		fmt.Printf("script files indexed: %d\n", len(names))
		total, known := 0, 0
		for y := w.Top; y <= w.Bottom; y++ {
			for x := w.Left; x <= w.Right; x++ {
				sr, err := wor.LoadServerRegion(server, w, x, y)
				if err != nil {
					fail("server region %d,%d: %v", x, y, err)
				}
				for _, t := range sr.Traps {
					total++
					name := names[t.TrapID]
					if name != "" {
						known++
					}
					fmt.Printf("region %d,%d cell %d,%d n=%d id %08x %s\n", x, y, t.X, t.Y, t.NumCell, t.TrapID, text.GBKToUTF8([]byte(name)))
				}
			}
		}
		fmt.Printf("traps: %d runs, %d with a known script\n", total, known)
	case "npcs":
		// npcs <mapid|gamepath> [x y]: npc placements of a map (or one region) from the server
		// archive (Npc_S: the real npcs) and the client archive (Npc_C: client-only extras)
		if len(args) < 2 {
			fail("npcs <mapid|gamepath> [x y]")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		templates := loadTemplates(set, findServer(dir))
		p, _ := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		var server *pak.Set
		if sdir := findServer(dir); sdir != "" {
			if s, err := openServerSet(sdir); err == nil {
				server = s
				defer server.Close()
			} else {
				log.Warn("asset", "server archive unavailable", log.F("dir", sdir), log.F("error", err))
			}
		}
		x0, y0, x1, y1 := w.Left, w.Top, w.Right, w.Bottom
		if len(args) >= 4 {
			x0, _ = strconv.Atoi(args[2])
			y0, _ = strconv.Atoi(args[3])
			x1, y1 = x0, y0
		}
		tplName := func(id int32) string {
			if int(id) > 0 && int(id) < len(templates) {
				return templates[id].Name
			}
			return "?"
		}
		total := 0
		for y := y0; y <= y1; y++ {
			for x := x0; x <= x1; x++ {
				if server != nil {
					r, err := wor.LoadServerRegion(server, w, x, y)
					if err != nil {
						fail("server region %d,%d: %v", x, y, err)
					}
					for _, n := range r.Npcs {
						fmt.Printf("S %03d_%03d tpl=%-5d %-24s kind=%d lvl=%d frame=%d at %d,%d (%s) script=%s\n", x, y, n.TemplateID, tplName(n.TemplateID),
							n.Kind, n.Level, n.Frame, n.X, n.Y, text.TCVN3ToUTF8([]byte(n.Name)), text.GBKToUTF8([]byte(strings.TrimRight(n.Script, "\x00"))))
						total++
					}
				}
				r, err := wor.LoadRegion(set, w, x, y)
				if err != nil {
					fail("region %d,%d: %v", x, y, err)
				}
				for _, n := range r.Npcs {
					fmt.Printf("C %03d_%03d tpl=%-5d %-24s kind=%d lvl=%d frame=%d at %d,%d\n", x, y, n.TemplateID, tplName(n.TemplateID), n.Kind, n.Level, n.Frame, n.X, n.Y)
					total++
				}
			}
		}
		fmt.Printf("%d npc placements\n", total)
	case "objects":
		// objects <mapid|gamepath> <x> <y> [image-substring]: every cover / buildin record of one
		// region together with the sprite header, for checking positions against the old renderer
		if len(args) < 4 {
			fail("objects <mapid|gamepath> <x> <y> [image-substring]")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		p, _ := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		x, _ := strconv.Atoi(args[2])
		y, _ := strconv.Atoi(args[3])
		r, err := wor.LoadRegion(set, w, x, y)
		if err != nil {
			fail("%v", err)
		}
		want := ""
		if len(args) > 4 {
			want = args[4]
		}
		type frameView struct {
			W, H, OffsetX, OffsetY int
		}
		type sprView struct {
			Width, Height, CenterX, CenterY, Frames, Directions, Interval int
			Reserved                                                      [6]uint16
			Frame                                                         []frameView
		}
		headers := map[string]*sprView{}
		sprite := func(img string) *sprView {
			if v, ok := headers[img]; ok {
				return v
			}
			var v *sprView
			if resolved, ok := resolveImage(set, img); ok {
				if f, entry, ok := set.Lookup(resolved); ok {
					if s, err := spr.ReadFromPak(f, entry); err == nil {
						v = &sprView{Width: s.Width, Height: s.Height, CenterX: s.CenterX, CenterY: s.CenterY, Frames: s.Header.Frames,
							Directions: s.Directions, Interval: s.Interval, Reserved: s.Reserved}
						for _, fr := range s.Frames {
							v.Frame = append(v.Frame, frameView{fr.Width, fr.Height, fr.OffsetX, fr.OffsetY})
						}
					}
				}
			}
			headers[img] = v
			return v
		}
		type objView struct {
			Kind   string
			Image  string
			Record any
			Sprite *sprView
		}
		out := []objView{}
		for _, c := range r.Covers {
			u := text.GBKToUTF8([]byte(c.Image))
			if want != "" && !strings.Contains(u, want) {
				continue
			}
			c.Image = u
			out = append(out, objView{Kind: "cover", Image: u, Record: c, Sprite: sprite(c.Image)})
		}
		for _, b := range r.Buildins {
			u := text.GBKToUTF8([]byte(b.Image))
			if want != "" && !strings.Contains(u, want) {
				continue
			}
			img := b.Image
			b.Image = u
			out = append(out, objView{Kind: "buildin", Image: u, Record: b, Sprite: sprite(img)})
		}
		enc := json.NewEncoder(os.Stdout)
		enc.SetIndent("", " ")
		if err := enc.Encode(out); err != nil {
			fail("%v", err)
		}
	case "region":
		if len(args) < 4 {
			fail("region <mapid|gamepath> <x> <y>")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		p, _ := mapPath(dir, set, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		x, _ := strconv.Atoi(args[2])
		y, _ := strconv.Atoi(args[3])
		r, err := wor.LoadRegion(set, w, x, y)
		if err != nil {
			fail("%v", err)
		}
		type view struct {
			X, Y     int
			HasData  bool
			Ground   int
			Covers   int
			Buildins int
			Npcs     []string
			Blocked  int
			Images   []string
		}
		v := view{X: r.X, Y: r.Y, HasData: r.HasData, Ground: len(r.Ground), Covers: len(r.Covers), Buildins: len(r.Buildins)}
		for _, n := range r.Npcs {
			v.Npcs = append(v.Npcs, fmt.Sprintf("%s tpl=%d at %d,%d", text.TCVN3ToUTF8([]byte(n.Name)), n.TemplateID, n.X, n.Y))
		}
		for cx := 0; cx < wor.CellsX; cx++ {
			for cy := 0; cy < wor.CellsY; cy++ {
				if r.Obstacle[cx][cy] != 0 {
					v.Blocked++
				}
			}
		}
		seen := map[string]bool{}
		for _, g := range r.Ground {
			if !seen[g.Image] {
				seen[g.Image] = true
				v.Images = append(v.Images, text.GBKToUTF8([]byte(g.Image)))
			}
		}
		for _, b := range r.Buildins {
			if !seen[b.Image] {
				seen[b.Image] = true
				v.Images = append(v.Images, text.GBKToUTF8([]byte(b.Image)))
			}
		}
		out, _ := json.MarshalIndent(v, "", "  ")
		fmt.Println(string(out))

	default:
		fail("unknown command %q", args[0])
	}
}

// equipRows resolves "-equip-rows": "all" = every row the five appearance tables can name (a look for whatever a character
// wears - hundreds of sprites per part; the gold / platina maps are not walked), else the "group:row,row;..." list.
func equipRows(spec string, icr *npcres.ItemChangeRes) map[int][]int {
	if strings.TrimSpace(spec) != "all" {
		return parseEquipRows(spec)
	}
	out := map[int][]int{}
	for g, rows := range icr.AllRows() {
		out[g] = rows
	}
	return out
}

// parseEquipRows reads "-equip-rows": "group:row,row;group:row" -> group -> rows (bad pieces are skipped).
func parseEquipRows(spec string) map[int][]int {
	out := map[int][]int{}
	for _, piece := range strings.Split(spec, ";") {
		g, rows, ok := strings.Cut(strings.TrimSpace(piece), ":")
		if !ok {
			continue
		}
		group, err := strconv.Atoi(strings.TrimSpace(g))
		if err != nil {
			continue
		}
		for _, r := range strings.Split(rows, ",") {
			if row, err := strconv.Atoi(strings.TrimSpace(r)); err == nil {
				out[group] = append(out[group], row)
			}
		}
	}
	return out
}
