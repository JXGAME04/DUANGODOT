package log

import (
	"context"
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func quiet(t *testing.T, ring int) {
	t.Helper()
	if err := Init(Options{Level: LevelTrace, Console: false, RingCapacity: ring, Process: "test"}); err != nil {
		t.Fatal(err)
	}
}

func lastLine(t *testing.T) map[string]any {
	t.Helper()
	lines := Ring()
	if len(lines) == 0 {
		t.Fatal("ring is empty")
	}
	var m map[string]any
	if err := json.Unmarshal([]byte(lines[len(lines)-1]), &m); err != nil {
		t.Fatalf("bad json %q: %v", lines[len(lines)-1], err)
	}
	return m
}

func TestLineHasStandardFieldsInOrder(t *testing.T) {
	quiet(t, 100)
	Info("net", "connected", F("addr", "127.0.0.1"), F("port", 15622))
	m := lastLine(t)
	if m["lvl"] != "info" || m["cat"] != "net" || m["proc"] != "test" || m["msg"] != "connected" {
		t.Fatalf("unexpected line %v", m)
	}
	if m["addr"] != "127.0.0.1" || m["port"] != "15622" {
		t.Fatalf("fields not stringified: %v", m)
	}
	if _, ok := m["sid"]; ok {
		t.Fatal("sid must be absent without context")
	}
	ts := m["ts"].(string)
	if len(ts) != 27 || ts[10] != 'T' || ts[26] != 'Z' {
		t.Fatalf("bad ts %q", ts)
	}
	raw := Ring()[len(Ring())-1]
	if !strings.HasPrefix(raw, `{"ts":`) || strings.Index(raw, `"lvl"`) > strings.Index(raw, `"cat"`) || strings.Index(raw, `"cat"`) > strings.Index(raw, `"msg"`) {
		t.Fatalf("field order broken: %s", raw)
	}
}

func TestContextIsMerged(t *testing.T) {
	quiet(t, 100)
	ctx := WithContext(context.Background(), Context{Sid: 42, Pid: 7, Zone: 3, Tick: 99})
	DebugCtx(ctx, "zone.tick", "tick")
	m := lastLine(t)
	if m["sid"] != float64(42) || m["pid"] != float64(7) || m["zone"] != float64(3) || m["tick"] != float64(99) {
		t.Fatalf("context missing: %v", m)
	}
	Info("zone", "no ctx")
	if _, ok := lastLine(t)["sid"]; ok {
		t.Fatal("sid leaked")
	}
}

func TestLevelsByLongestPrefix(t *testing.T) {
	quiet(t, 100)
	SetLevels("net=trace, zone.tick=debug, =warn")
	cases := map[string]Level{"net": LevelTrace, "net.recv": LevelTrace, "zone.tick": LevelDebug, "zone.tick.ai": LevelDebug, "zone": LevelWarn, "other": LevelWarn}
	for cat, want := range cases {
		if got := EffectiveLevel(cat); got != want {
			t.Errorf("%s: got %v want %v", cat, got, want)
		}
	}
	if Enabled("zone.tick", LevelTrace) || !Enabled("zone.tick", LevelDebug) || Enabled("zone", LevelInfo) || Enabled("net", LevelOff) {
		t.Fatal("Enabled wrong")
	}
	SetLevel("zone", LevelInfo)
	if EffectiveLevel("zone.ai") != LevelInfo || EffectiveLevel("zone.tick") != LevelDebug {
		t.Fatal("more specific rule must win")
	}
	before := len(Ring())
	Trace("zone.ai", "filtered")
	if len(Ring()) != before {
		t.Fatal("filtered line reached the ring")
	}
	Info("zone.ai", "kept")
	if len(Ring()) != before+1 {
		t.Fatal("kept line missing")
	}
}

func TestLevelNamesRoundTrip(t *testing.T) {
	for l := LevelTrace; l <= LevelOff; l++ {
		if ParseLevel(l.String()) != l {
			t.Errorf("round trip failed for %v", l)
		}
	}
	if ParseLevel("WARNING") != LevelInfo || ParseLevel("warning") != LevelWarn || ParseLevel(" debug ") != LevelDebug {
		t.Fatal("ParseLevel")
	}
}

func TestRingKeepsLastN(t *testing.T) {
	quiet(t, 3)
	for i := 0; i < 5; i++ {
		Info("t", "line"+string(rune('0'+i)))
	}
	lines := Ring()
	if len(lines) != 3 || !strings.Contains(lines[0], `"line2"`) || !strings.Contains(lines[2], `"line4"`) {
		t.Fatalf("ring wrong: %v", lines)
	}
}

func TestFatalDumpsRingNextToFile(t *testing.T) {
	dir := t.TempDir()
	file := filepath.Join(dir, "gateway.log")
	if err := Init(Options{Level: LevelTrace, Console: false, RingCapacity: 100, Process: "test", File: file}); err != nil {
		t.Fatal(err)
	}
	Info("boot", "starting")
	Fatal("boot", "cannot bind", F("port", 15622))
	Shutdown()
	data, err := os.ReadFile(file)
	if err != nil || len(data) == 0 {
		t.Fatalf("log file missing: %v", err)
	}
	crash, err := os.ReadFile(filepath.Join(dir, "gateway.crash.log"))
	if err != nil {
		t.Fatal(err)
	}
	lines := strings.Split(strings.TrimSpace(string(crash)), "\n")
	if len(lines) != 2 || !strings.Contains(lines[1], `"lvl":"fatal"`) || !strings.Contains(lines[1], `"port":"15622"`) {
		t.Fatalf("crash dump wrong: %q", crash)
	}
}

func TestRotation(t *testing.T) {
	dir := t.TempDir()
	file := filepath.Join(dir, "z.log")
	if err := Init(Options{Level: LevelTrace, RingCapacity: 10, File: file, RotateBytes: 300, RotateFiles: 2}); err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 20; i++ {
		Info("t", "a fairly long message to fill the file quickly", F("i", i))
	}
	Shutdown()
	if _, err := os.Stat(filepath.Join(dir, "z.1.log")); err != nil {
		t.Fatal("rotated file z.1.log missing")
	}
	if _, err := os.Stat(filepath.Join(dir, "z.3.log")); err == nil {
		t.Fatal("more files than RotateFiles")
	}
}
