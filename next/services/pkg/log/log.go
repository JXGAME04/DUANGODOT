// Package log implements the JX NEXT structured logging standard (next/docs/LOGGING.md):
// one JSON object per line, field order ts,lvl,cat,proc,sid,pid,zone,tick,msg,<fields>,
// per-category levels resolved by longest dotted prefix, and a ring buffer of the last N
// lines that is dumped next to the log file by Fatal.  Same behaviour as jx::log in C++.
package log

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

// Level mirrors jx::log::Level.
type Level int

const (
	LevelTrace Level = iota
	LevelDebug
	LevelInfo
	LevelWarn
	LevelError
	LevelFatal
	LevelOff
)

var levelNames = [...]string{"trace", "debug", "info", "warn", "error", "fatal", "off"}

func (l Level) String() string {
	if l < LevelTrace || l > LevelOff {
		return "info"
	}
	return levelNames[l]
}

// ParseLevel returns the level for a name; unknown names map to info like the C++ side.
func ParseLevel(s string) Level {
	switch strings.TrimSpace(s) {
	case "trace":
		return LevelTrace
	case "debug":
		return LevelDebug
	case "info":
		return LevelInfo
	case "warn", "warning":
		return LevelWarn
	case "error":
		return LevelError
	case "fatal":
		return LevelFatal
	case "off":
		return LevelOff
	}
	return LevelInfo
}

// Field is one extra key/value on a line; values are always strings on the wire.
type Field struct {
	Key   string
	Value string
}

// F builds a Field from any value (fmt.Sprint formatting).
func F(key string, v any) Field { return Field{Key: key, Value: fmt.Sprint(v)} }

// Options configures Init.
type Options struct {
	Level        Level  // default level
	Levels       string // "net=trace,zone.tick=debug,=warn"
	Console      bool
	File         string // empty = no file
	RotateBytes  int64  // 0 = 32 MiB
	RotateFiles  int    // 0 = 5
	RingCapacity int    // 0 = 10000
	Process      string // "gateway", "jxbot", ...
}

// Context carries correlation ids merged into every line written with the *Ctx functions.
type Context struct {
	Sid  uint64
	Pid  uint64
	Zone uint32
	Tick uint64
}

type ctxKey struct{}

// WithContext attaches a logging Context to ctx.
func WithContext(ctx context.Context, c Context) context.Context {
	return context.WithValue(ctx, ctxKey{}, c)
}

// FromContext returns the logging Context stored in ctx (zero value when absent).
func FromContext(ctx context.Context) Context {
	if ctx == nil {
		return Context{}
	}
	c, _ := ctx.Value(ctxKey{}).(Context)
	return c
}

type state struct {
	mu       sync.Mutex
	opts     Options
	levels   map[string]Level
	ring     []string
	ringNext int
	ringLen  int
	file     *os.File
	fileSize int64
	console  *os.File
}

var s = &state{levels: map[string]Level{"": LevelInfo}, console: os.Stdout}

// Init (re)configures the package.  Safe to call more than once (tests).
func Init(o Options) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if o.RotateBytes <= 0 {
		o.RotateBytes = 32 << 20
	}
	if o.RotateFiles <= 0 {
		o.RotateFiles = 5
	}
	if o.RingCapacity <= 0 {
		o.RingCapacity = 10000
	}
	if s.file != nil {
		_ = s.file.Close()
		s.file = nil
	}
	s.opts = o
	s.levels = map[string]Level{"": o.Level}
	s.ring = make([]string, o.RingCapacity)
	s.ringNext, s.ringLen = 0, 0
	if o.File != "" {
		if err := os.MkdirAll(filepath.Dir(o.File), 0o755); err != nil {
			return err
		}
		f, err := os.OpenFile(o.File, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0o644)
		if err != nil {
			return err
		}
		st, _ := f.Stat()
		s.file = f
		s.fileSize = st.Size()
	}
	if o.Levels != "" {
		applyLevels(o.Levels)
	}
	return nil
}

// Shutdown flushes and closes the file sink.
func Shutdown() {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.file != nil {
		_ = s.file.Close()
		s.file = nil
	}
}

// SetLevel sets the level of one category ("" = default).
func SetLevel(category string, l Level) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.levels[category] = l
}

// SetLevels parses "cat=level,cat2=level,=default".
func SetLevels(spec string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	applyLevels(spec)
}

func applyLevels(spec string) {
	for _, item := range strings.Split(spec, ",") {
		if eq := strings.IndexByte(item, '='); eq >= 0 {
			s.levels[strings.TrimSpace(item[:eq])] = ParseLevel(item[eq+1:])
		} else if strings.TrimSpace(item) != "" {
			s.levels[""] = ParseLevel(item)
		}
	}
}

// EffectiveLevel resolves a category by its longest configured dotted prefix.
func EffectiveLevel(category string) Level {
	s.mu.Lock()
	defer s.mu.Unlock()
	return effectiveLocked(category)
}

func effectiveLocked(category string) Level {
	key := category
	for {
		if l, ok := s.levels[key]; ok {
			return l
		}
		dot := strings.LastIndexByte(key, '.')
		if dot < 0 {
			break
		}
		key = key[:dot]
	}
	if l, ok := s.levels[""]; ok {
		return l
	}
	return LevelInfo
}

// Enabled reports whether a line at level l in category would be written.
func Enabled(category string, l Level) bool {
	if l == LevelOff {
		return false
	}
	return l >= EffectiveLevel(category)
}

func Trace(cat, msg string, fields ...Field) {
	Write(context.Background(), LevelTrace, cat, msg, fields)
}
func Debug(cat, msg string, fields ...Field) {
	Write(context.Background(), LevelDebug, cat, msg, fields)
}
func Info(cat, msg string, fields ...Field) { Write(context.Background(), LevelInfo, cat, msg, fields) }
func Warn(cat, msg string, fields ...Field) { Write(context.Background(), LevelWarn, cat, msg, fields) }
func Error(cat, msg string, fields ...Field) {
	Write(context.Background(), LevelError, cat, msg, fields)
}

func TraceCtx(ctx context.Context, cat, msg string, fields ...Field) {
	Write(ctx, LevelTrace, cat, msg, fields)
}
func DebugCtx(ctx context.Context, cat, msg string, fields ...Field) {
	Write(ctx, LevelDebug, cat, msg, fields)
}
func InfoCtx(ctx context.Context, cat, msg string, fields ...Field) {
	Write(ctx, LevelInfo, cat, msg, fields)
}
func WarnCtx(ctx context.Context, cat, msg string, fields ...Field) {
	Write(ctx, LevelWarn, cat, msg, fields)
}
func ErrorCtx(ctx context.Context, cat, msg string, fields ...Field) {
	Write(ctx, LevelError, cat, msg, fields)
}

// Fatal writes the line, dumps the ring buffer to <file>.crash.log (or jx.crash.log in the
// working directory) and flushes.  It does not exit: the caller decides, like jx::log::fatal.
func Fatal(cat, msg string, fields ...Field) {
	Write(context.Background(), LevelFatal, cat, msg, fields)
	s.mu.Lock()
	path := "jx.crash.log"
	if s.opts.File != "" {
		path = strings.TrimSuffix(s.opts.File, filepath.Ext(s.opts.File)) + ".crash.log"
	}
	s.mu.Unlock()
	_ = DumpRing(path)
}

// Write is the low-level entry point used by the helpers above.
func Write(ctx context.Context, l Level, cat, msg string, fields []Field) {
	if !Enabled(cat, l) {
		return
	}
	c := FromContext(ctx)
	now := time.Now().UTC()

	s.mu.Lock()
	defer s.mu.Unlock()
	line := format(now, l, cat, c, msg, fields)
	pushRingLocked(line)
	if s.opts.Console && s.console != nil {
		_, _ = s.console.WriteString(line)
		_, _ = s.console.WriteString("\n")
	}
	if s.file != nil {
		if s.fileSize+int64(len(line))+1 > s.opts.RotateBytes {
			rotateLocked()
		}
		if s.file != nil {
			n, _ := s.file.WriteString(line + "\n")
			s.fileSize += int64(n)
		}
	}
}

func format(ts time.Time, l Level, cat string, c Context, msg string, fields []Field) string {
	var b strings.Builder
	b.Grow(160 + 32*len(fields))
	b.WriteString(`{"ts":"`)
	b.WriteString(ts.Format("2006-01-02T15:04:05.000000Z"))
	b.WriteString(`","lvl":"`)
	b.WriteString(l.String())
	b.WriteString(`","cat":`)
	writeJSONString(&b, cat)
	if s.opts.Process != "" {
		b.WriteString(`,"proc":`)
		writeJSONString(&b, s.opts.Process)
	}
	if c.Sid != 0 {
		fmt.Fprintf(&b, `,"sid":%d`, c.Sid)
	}
	if c.Pid != 0 {
		fmt.Fprintf(&b, `,"pid":%d`, c.Pid)
	}
	if c.Zone != 0 {
		fmt.Fprintf(&b, `,"zone":%d`, c.Zone)
	}
	if c.Tick != 0 {
		fmt.Fprintf(&b, `,"tick":%d`, c.Tick)
	}
	b.WriteString(`,"msg":`)
	writeJSONString(&b, msg)
	for _, f := range fields {
		b.WriteByte(',')
		writeJSONString(&b, f.Key)
		b.WriteByte(':')
		writeJSONString(&b, f.Value)
	}
	b.WriteByte('}')
	return b.String()
}

func writeJSONString(b *strings.Builder, v string) {
	enc, err := json.Marshal(v)
	if err != nil {
		b.WriteString(`""`)
		return
	}
	b.Write(enc)
}

func pushRingLocked(line string) {
	if len(s.ring) == 0 {
		return
	}
	s.ring[s.ringNext] = line
	s.ringNext = (s.ringNext + 1) % len(s.ring)
	if s.ringLen < len(s.ring) {
		s.ringLen++
	}
}

// Ring returns the buffered lines, oldest first.
func Ring() []string {
	s.mu.Lock()
	defer s.mu.Unlock()
	out := make([]string, 0, s.ringLen)
	start := (s.ringNext - s.ringLen + len(s.ring)) % max(len(s.ring), 1)
	for i := 0; i < s.ringLen; i++ {
		out = append(out, s.ring[(start+i)%len(s.ring)])
	}
	return out
}

// DumpRing writes the ring buffer to path (one line each).
func DumpRing(path string) error {
	lines := Ring()
	return os.WriteFile(path, []byte(strings.Join(lines, "\n")+"\n"), 0o644)
}

func rotateLocked() {
	name := s.opts.File
	_ = s.file.Close()
	s.file = nil
	for i := s.opts.RotateFiles - 1; i >= 1; i-- {
		_ = os.Rename(rotatedName(name, i), rotatedName(name, i+1))
	}
	_ = os.Rename(name, rotatedName(name, 1))
	f, err := os.OpenFile(name, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0o644)
	if err != nil {
		return
	}
	s.file = f
	s.fileSize = 0
}

func rotatedName(name string, i int) string {
	ext := filepath.Ext(name)
	return fmt.Sprintf("%s.%d%s", strings.TrimSuffix(name, ext), i, ext)
}
