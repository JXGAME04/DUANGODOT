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
//
// Game paths are UTF-8 on the command line and encoded to GBK for hashing (the archives use
// the original Chinese paths); hex:<bytes> passes raw bytes.  A map id refers to Settings/MapList.ini.
package main

import (
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/export"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/npcres"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/spr"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/wor"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

var (
	flagClient = flag.String("client", "", "old client folder containing package.ini and Data/*.pak")
	flagOut    = flag.String("out", "", "output file or directory")
	flagLevel  = flag.String("log-level", "info", "log level")
	flagTpl    = flag.String("templates", "", "export-npcres: extra npc template ids (comma separated), e.g. the zone's test npcs")
	flagServer = flag.String("server", "", "old server folder (package.ini + pak/maps.pak) for the server-side region files; default: the Server folder next to the client")
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

func findClient() string {
	if *flagClient != "" {
		return *flagClient
	}
	if env := os.Getenv("JX_OLD_CLIENT"); env != "" {
		return env
	}
	for _, rel := range []string{"../bin/Client", "../../bin/Client", "bin/Client"} {
		if _, err := os.Stat(filepath.Join(rel, "package.ini")); err == nil {
			p, _ := filepath.Abs(rel)
			return p
		}
	}
	fail("old client folder not found: pass -client or set JX_OLD_CLIENT")
	return ""
}

func openSet(dir string) *pak.Set {
	set, err := pak.OpenSet(filepath.Join(dir, "package.ini"))
	if err != nil {
		fail("%v", err)
	}
	return set
}

// mapPath resolves "<id>" through Settings/MapList.ini or passes a game path through.
func mapPath(dir, arg string) (string, string) {
	if id, err := strconv.Atoi(arg); err == nil {
		data, err := os.ReadFile(filepath.Join(dir, "Settings", "MapList.ini"))
		if err != nil {
			fail("MapList.ini: %v", err)
		}
		re := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `=(.*)$`)
		m := re.FindSubmatch(data)
		if m == nil {
			fail("map %d not in MapList.ini", id)
		}
		p := strings.TrimSpace(strings.TrimRight(string(m[1]), "\r"))
		name := ""
		if nm := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `_name=(.*)$`).FindSubmatch(data); nm != nil {
			name = text.TCVN3ToUTF8([]byte(strings.TrimSpace(strings.TrimRight(string(nm[1]), "\r"))))
		}
		return p, name
	}
	return gamePath(arg), ""
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
	cand := filepath.Join(filepath.Dir(filepath.Clean(clientDir)), "Server")
	if _, err := os.Stat(filepath.Join(cand, "package.ini")); err == nil {
		return cand
	}
	return ""
}

// prepareExporter wires what map npcs need: the templates (names), the old server's archive
// (Region_S.dat with the real npcs), replacename_npc.txt and the stand frame counts for the
// facing of client-only npcs.  Returns the function that closes the server archive.
func prepareExporter(ex *export.Exporter, clientDir string, set *pak.Set) func() {
	ex.Templates = loadTemplates(set)
	closer := func() {}
	sdir := findServer(clientDir)
	if sdir == "" {
		log.Warn("asset", "old server folder not found: map npcs come from the client archive only (pass -server)")
	} else if s, err := pak.OpenSet(filepath.Join(sdir, "package.ini")); err != nil {
		log.Warn("asset", "server archive unavailable", log.F("dir", sdir), log.F("error", err))
	} else {
		ex.ServerSet = s
		closer = s.Close
		if data, err := os.ReadFile(filepath.Join(sdir, filepath.FromSlash(strings.ReplaceAll(npcres.ReplaceNameFile, `\`, "/")))); err == nil {
			ex.ReplaceNames = npcres.ParseReplaceNames(data)
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
			if id > 0 && id < len(ex.Templates) {
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
func loadTemplates(set *pak.Set) []npcres.Template {
	data, err := set.ReadFile(gamePath(npcres.TemplateFile))
	if err != nil {
		log.Warn("asset", "npcs.txt missing, map npcs keep their editor names", log.F("error", err))
		return nil
	}
	return npcres.ParseTemplates(data)
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
		fail("usage: jxassets [-client DIR] [-out PATH] list|find|cat|spr|map|region|objects ...")
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
		p, name := mapPath(dir, args[1])
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
		p, name := mapPath(dir, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		var spawn [2]int
		if data, err := os.ReadFile(filepath.Join(dir, "Settings", "MapList.ini")); err == nil {
			if m := regexp.MustCompile(`(?m)^` + strconv.Itoa(id) + `_MapPos=(\d+),(\d+)`).FindSubmatch(data); m != nil {
				// MapPos is in minimap pixels; keep zero so the exporter picks a walkable cell
				_ = m
			}
		}
		ex := export.New(set, out)
		defer prepareExporter(ex, dir, set)()
		info, err := ex.Map(id, name, w, spawn)
		if err != nil {
			fail("%v", err)
		}
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
		data, err := os.ReadFile(filepath.Join(dir, "Settings", "MapList.ini"))
		if err != nil {
			fail("MapList.ini: %v", err)
		}
		set := openSet(dir)
		defer set.Close()
		ex := export.New(set, out)
		defer prepareExporter(ex, dir, set)()
		re := regexp.MustCompile(`(?m)^(\d+)=(.*)$`)
		ok, missing, failed := 0, 0, 0
		start := time.Now()
		for _, m := range re.FindAllSubmatch(data, -1) {
			id, _ := strconv.Atoi(string(m[1]))
			p := strings.TrimSpace(strings.TrimRight(string(m[2]), "\r"))
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
		fmt.Printf("export-all: %d maps ok, %d skipped (no .wor), %d failed, %d sprites, %s\n", ok, missing, failed, ex.Exported, time.Since(start).Round(time.Second))

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
		tplData, err := set.ReadFile(gamePath(npcres.TemplateFile))
		if err != nil {
			fail("npcs.txt: %v", err)
		}
		templates := npcres.ParseTemplates(tplData)
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
			if s, err := pak.OpenSet(filepath.Join(sdir, "package.ini")); err == nil {
				server = s
				defer server.Close()
			}
		}
		for _, arg := range ids {
			p, _ := mapPath(dir, arg)
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
			Names:  names,
			Doings: []int{npcres.DoStand, npcres.DoStand1, npcres.DoWalk, npcres.DoRun},
			Equips: icr.DefaultEquips(),
		}
		n, err := e.NpcRes(list, templates, player, opt)
		if err != nil {
			fail("%v", err)
		}
		fmt.Printf("npcres: %d resources for %d placed npcs (%d templates), sprites exported %d\nwritten to %s\n",
			n, len(placed), len(templates), e.Exported, filepath.Join(*flagOut, "npcres"))
	case "npcs":
		// npcs <mapid|gamepath> [x y]: npc placements of a map (or one region) from the server
		// archive (Npc_S: the real npcs) and the client archive (Npc_C: client-only extras)
		if len(args) < 2 {
			fail("npcs <mapid|gamepath> [x y]")
		}
		dir := findClient()
		set := openSet(dir)
		defer set.Close()
		templates := loadTemplates(set)
		p, _ := mapPath(dir, args[1])
		w, err := wor.LoadWorld(set, p)
		if err != nil {
			fail("%v", err)
		}
		var server *pak.Set
		if sdir := findServer(dir); sdir != "" {
			if s, err := pak.OpenSet(filepath.Join(sdir, "package.ini")); err == nil {
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
		p, _ := mapPath(dir, args[1])
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
			Reserved                                                       [6]uint16
			Frame                                                          []frameView
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
		p, _ := mapPath(dir, args[1])
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
