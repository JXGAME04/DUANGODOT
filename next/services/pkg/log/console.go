package log

// The console is for the person running the server; the file is for tools.  The file keeps the
// JSON lines of docs/LOGGING.md.  The console prints one sentence per line,
//
//	14:32:05.123 THÔNG TIN    [mạng]        Người chơi vừa kết nối · địa chỉ client=127.0.0.1:50123 · phiên=42
//
// in the language of the catalogue: config/log.vi.json maps every English `msg`, category and
// field name of the code to Vietnamese, and what it does not know stays English.  Same format,
// same catalogue as jx::log in C++, so jx_zone and the gateway read alike side by side.

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"
)

// Catalog is what the text console says instead of the English of the code.
type Catalog struct {
	Levels     map[string]string `json:"levels"`
	Categories map[string]string `json:"categories"`
	Fields     map[string]string `json:"fields"`
	Messages   map[string]string `json:"messages"`
}

// Size is how many words and sentences the catalogue translates.
func (c *Catalog) Size() int {
	if c == nil {
		return 0
	}
	return len(c.Levels) + len(c.Categories) + len(c.Fields) + len(c.Messages)
}

// LoadCatalog reads config/log.<language>.json: `path` when given, else the file of that name
// below the working directory or one of its parents (a server is started from next/, a test from
// its package folder).  A missing file is no error: the console then speaks English.
func LoadCatalog(path, language string) *Catalog {
	if path == "" {
		path = findCatalog(language)
	}
	if path == "" {
		return nil
	}
	data, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	var c Catalog
	if json.Unmarshal(data, &c) != nil {
		return nil
	}
	return &c
}

func findCatalog(language string) string {
	if language == "" || language == "en" {
		return ""
	}
	dir, err := os.Getwd()
	if err != nil {
		return ""
	}
	name := filepath.Join("config", "log."+language+".json")
	for up := 0; up < 6; up++ {
		if _, err := os.Stat(filepath.Join(dir, name)); err == nil {
			return filepath.Join(dir, name)
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			break
		}
		dir = parent
	}
	return ""
}

var englishLabels = [...]string{"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"}

// ANSI colours of the level label, the ones spdlog uses on the C++ side.
var levelColours = [...]string{"\x1b[37m", "\x1b[36m", "\x1b[32m", "\x1b[33;1m", "\x1b[31;1m", "\x1b[1;41m", ""}

func lookup(table map[string]string, key string) string {
	if v, ok := table[key]; ok && v != "" {
		return v
	}
	return key
}

func pad(b *strings.Builder, written string, width int) {
	for n := utf8.RuneCountInString(written); n < width; n++ {
		b.WriteByte(' ')
	}
}

// FormatText is the text-console form of one line.  `colour` wraps the level label in ANSI codes.
func FormatText(c *Catalog, ts time.Time, l Level, cat string, ctx Context, msg string, fields []Field, colour bool) string {
	if c == nil {
		c = &Catalog{}
	}
	if l < LevelTrace || l > LevelOff {
		l = LevelInfo
	}
	var b strings.Builder
	b.Grow(96 + len(msg) + 24*len(fields))
	b.WriteString(ts.Local().Format("15:04:05.000"))
	b.WriteByte(' ')
	label := englishLabels[l]
	if v, ok := c.Levels[l.String()]; ok && v != "" {
		label = v
	}
	if colour {
		b.WriteString(levelColours[l])
	}
	b.WriteString(label)
	if colour {
		b.WriteString("\x1b[0m")
	}
	pad(&b, label, 12)
	b.WriteByte(' ')
	// a category is translated whole, else by its first part: "zone.fight" reads "zone" + ".fight"
	shown := lookup(c.Categories, cat)
	if shown == cat {
		if dot := strings.IndexByte(cat, '.'); dot > 0 {
			shown = lookup(c.Categories, cat[:dot]) + cat[dot:]
		}
	}
	tag := "[" + shown + "]"
	b.WriteString(tag)
	pad(&b, tag, 14)
	b.WriteString(lookup(c.Messages, msg))
	add := func(key, value string) {
		b.WriteString(" · ")
		b.WriteString(lookup(c.Fields, key))
		b.WriteByte('=')
		b.WriteString(value)
	}
	for _, f := range fields {
		add(f.Key, f.Value)
	}
	if ctx.Sid != 0 {
		add("sid", strconv.FormatUint(ctx.Sid, 10))
	}
	if ctx.Pid != 0 {
		add("pid", strconv.FormatUint(ctx.Pid, 10))
	}
	if ctx.Zone != 0 {
		add("zone", strconv.FormatUint(uint64(ctx.Zone), 10))
	}
	return b.String()
}

// isTerminal reports whether f is a console window rather than a file or a pipe.
func isTerminal(f *os.File) bool {
	if f == nil {
		return false
	}
	st, err := f.Stat()
	return err == nil && st.Mode()&os.ModeCharDevice != 0
}
