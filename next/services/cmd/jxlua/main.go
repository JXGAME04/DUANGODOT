// jxlua converts the old game's Lua 4.0 script trees into real Lua 5.4 source.
//
//	jxlua convert -in D:\ServerLinux\server1\script -out ..\data\script\server1\script
//	jxlua check   -in ..\data\script\server1\script
//
// The files are rewritten byte for byte except for the Lua syntax itself: the old trees mix GBK
// Chinese and TCVN3 Vietnamese inside the same tree (measured: 480364 one-byte runs of high bytes
// next to 33673 runs of 9 or more), so any transcoding would corrupt one of the two.  Lua syntax is
// ASCII, and the converter only ever touches ASCII outside strings and comments.
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxlua"
)

// Files that are not scripts but that scripts read (openfile) are copied through untouched.
var copyExt = map[string]bool{".dat": true, ".txt": true, ".ini": true, ".csv": true, ".tab": true}

type fileReport struct {
	Path    string   `json:"path"`
	Changes int      `json:"changes"`
	Notes   []string `json:"notes,omitempty"`
	Error   string   `json:"error,omitempty"`
}

type report struct {
	In        string         `json:"in"`
	Out       string         `json:"out"`
	Scripts   int            `json:"scripts"`
	Copied    int            `json:"copied"`
	Skipped   int            `json:"skipped"`
	Rewrites  int            `json:"rewrites"`
	ByKind    map[string]int `json:"by_kind"`
	Attention []fileReport   `json:"attention,omitempty"`
}

func main() {
	if len(os.Args) < 2 {
		usage()
	}
	cmd := os.Args[1]
	fs := flag.NewFlagSet(cmd, flag.ExitOnError)
	in := fs.String("in", "", "script folder to read")
	out := fs.String("out", "", "folder to write the converted scripts into (convert only)")
	reportPath := fs.String("report", "", "write the full JSON report here")
	quiet := fs.Bool("quiet", false, "only print the summary")
	if err := fs.Parse(os.Args[2:]); err != nil {
		os.Exit(2)
	}
	if *in == "" {
		usage()
	}

	switch cmd {
	case "convert":
		if *out == "" {
			usage()
		}
		run(*in, *out, *reportPath, *quiet)
	case "check":
		run(*in, "", *reportPath, *quiet)
	default:
		usage()
	}
}

func usage() {
	fmt.Fprintln(os.Stderr, "usage: jxlua convert -in <script dir> -out <dir> [-report f.json] [-quiet]")
	fmt.Fprintln(os.Stderr, "       jxlua check   -in <script dir> [-report f.json]")
	os.Exit(2)
}

func run(in, out, reportPath string, quiet bool) {
	rep := report{In: in, Out: out, ByKind: map[string]int{}}
	err := filepath.WalkDir(in, func(path string, d os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}
		ext := strings.ToLower(filepath.Ext(path))
		rel, relErr := filepath.Rel(in, path)
		if relErr != nil {
			return relErr
		}
		switch {
		case ext == ".lua":
			rep.Scripts++
			fr := convertOne(path, out, rel, &rep)
			if fr != nil {
				rep.Attention = append(rep.Attention, *fr)
				if !quiet {
					fmt.Printf("  %s: %s\n", rel, strings.Join(append(fr.Notes, fr.Error), " "))
				}
			}
		case copyExt[ext]:
			rep.Copied++
			if out != "" {
				if err := copyFile(path, filepath.Join(out, rel)); err != nil {
					return err
				}
			}
		default:
			rep.Skipped++
		}
		return nil
	})
	if err != nil {
		fmt.Fprintln(os.Stderr, "jxlua:", err)
		os.Exit(1)
	}

	kinds := make([]string, 0, len(rep.ByKind))
	for k := range rep.ByKind {
		kinds = append(kinds, k)
	}
	sort.Slice(kinds, func(i, j int) bool {
		if rep.ByKind[kinds[i]] != rep.ByKind[kinds[j]] {
			return rep.ByKind[kinds[i]] > rep.ByKind[kinds[j]]
		}
		return kinds[i] < kinds[j]
	})
	fmt.Printf("scripts %d, copied %d, skipped %d, rewrites %d, needing attention %d\n",
		rep.Scripts, rep.Copied, rep.Skipped, rep.Rewrites, len(rep.Attention))
	for _, k := range kinds {
		fmt.Printf("  %-28s %7d\n", k, rep.ByKind[k])
	}
	if reportPath != "" {
		if err := os.MkdirAll(filepath.Dir(reportPath), 0o755); err != nil {
			fmt.Fprintln(os.Stderr, "jxlua:", err)
			os.Exit(1)
		}
		b, _ := json.MarshalIndent(rep, "", "  ")
		if err := os.WriteFile(reportPath, b, 0o644); err != nil {
			fmt.Fprintln(os.Stderr, "jxlua:", err)
			os.Exit(1)
		}
		fmt.Println("report:", reportPath)
	}
	if len(rep.Attention) > 0 {
		os.Exit(1)
	}
}

// convertOne converts one script; it returns a report entry only when a human should look at it.
func convertOne(path, out, rel string, rep *report) *fileReport {
	src, err := os.ReadFile(path)
	if err != nil {
		return &fileReport{Path: rel, Error: err.Error()}
	}
	r := jxlua.Convert(string(src))
	rep.Rewrites += r.Total()
	for k, v := range r.Changes {
		rep.ByKind[k] += v
	}
	if out != "" {
		dst := filepath.Join(out, rel)
		if err := os.MkdirAll(filepath.Dir(dst), 0o755); err != nil {
			return &fileReport{Path: rel, Error: err.Error()}
		}
		if err := os.WriteFile(dst, []byte(r.Source), 0o644); err != nil {
			return &fileReport{Path: rel, Error: err.Error()}
		}
	}
	if len(r.Notes) > 0 {
		return &fileReport{Path: rel, Changes: r.Total(), Notes: r.Notes}
	}
	return nil
}

func copyFile(src, dst string) error {
	b, err := os.ReadFile(src)
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(dst), 0o755); err != nil {
		return err
	}
	return os.WriteFile(dst, b, 0o644)
}
