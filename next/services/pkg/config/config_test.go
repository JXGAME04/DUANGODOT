package config

import (
	"os"
	"path/filepath"
	"testing"
)

const sample = `{
    // comments are allowed
    "log": { "level": "info", "file": "zone.log", "rotate_bytes": 1048576, "url": "http://x/y" },
    "net": { "port": 15622, "tls": false, "host": "127.0.0.1" },
    "tick": { "hz": 20.0 }
}`

func load(t *testing.T) *Config {
	t.Helper()
	c, err := FromJSON([]byte(sample))
	if err != nil {
		t.Fatal(err)
	}
	return c
}

func TestGetters(t *testing.T) {
	c := load(t)
	if c.String("log.level", "x") != "info" || c.String("log.url", "") != "http://x/y" {
		t.Fatal("string")
	}
	if c.Int("net.port", 0) != 15622 || c.Int("log.rotate_bytes", 0) != 1048576 || c.Int("tick.hz", 0) != 20 {
		t.Fatal("int")
	}
	if c.Bool("net.tls", true) || c.String("net.port", "") != "15622" {
		t.Fatal("bool/stringify")
	}
	if !c.Has("net.host") || c.Has("net.missing") || c.String("net.missing", "def") != "def" {
		t.Fatal("has/default")
	}
	if c.Int("net.host", 9) != 9 || !c.Bool("log.level", true) {
		t.Fatal("wrong type must return default")
	}
}

func TestOverridesAreAutoTyped(t *testing.T) {
	c := load(t)
	c.ApplyOverrides(map[string]string{"log.level": "debug", "net.port": "16666", "net.tls": "true", "new.section.key": "hello"})
	if c.String("log.level", "") != "debug" || c.Int("net.port", 0) != 16666 || !c.Bool("net.tls", false) || c.String("new.section.key", "") != "hello" {
		t.Fatal("override values")
	}
	if _, isNum := mustGet(t, c, "net.port").(float64); !isNum {
		t.Fatal("net.port must be a JSON number")
	}
	if _, isBool := mustGet(t, c, "net.tls").(bool); !isBool {
		t.Fatal("net.tls must be a JSON bool")
	}
	c.Set("log", "plain")
	c.Set("log.level", "warn")
	if c.String("log.level", "") != "warn" {
		t.Fatal("scalar parent must be replaced by an object")
	}
}

func mustGet(t *testing.T, c *Config, path string) any {
	t.Helper()
	v, ok := c.Get(path)
	if !ok {
		t.Fatalf("missing %s", path)
	}
	return v
}

func TestStringValuesParse(t *testing.T) {
	c := New()
	c.Set("a", `"42"`)
	c.Set("b", `"yes"`)
	c.Set("c", `"off"`)
	c.Set("d", `"12abc"`)
	if c.Int("a", 0) != 42 || !c.Bool("b", false) || c.Bool("c", true) || c.Int("d", -1) != -1 || !c.Bool("d", true) {
		t.Fatal("string parsing")
	}
}

func TestEnvMapping(t *testing.T) {
	cases := map[string]string{"JX_LOG__LEVEL": "log.level", "JX_NET__PORT": "net.port", "JX_LOG__ROTATE_BYTES": "log.rotate_bytes", "JX_A__B__C": "a.b.c", "JX_": "", "PATH": ""}
	for k, want := range cases {
		if got := EnvKeyToPath(k, "JX_"); got != want {
			t.Errorf("%s: got %q want %q", k, got, want)
		}
	}
	c := load(t)
	c.ApplyEnvList([]string{"JX_LOG__LEVEL=trace", "JX_NET__PORT=17000", "HOME=/nowhere", "JX_=bad", "=weird"}, "JX_")
	if c.String("log.level", "") != "trace" || c.Int("net.port", 0) != 17000 || c.Has("home") {
		t.Fatal("env list")
	}
	t.Setenv("JX_TEST__FROM_ENV", "on")
	c2 := New()
	c2.ApplyEnv("JX_")
	if !c2.Bool("test.from_env", false) {
		t.Fatal("real environment")
	}
}

func TestFilesAndErrors(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "zone.json")
	if err := os.WriteFile(path, []byte(sample), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := FromFile(path)
	if err != nil || c.Int("net.port", 0) != 15622 {
		t.Fatal("from file", err)
	}
	if _, err := FromFile(filepath.Join(dir, "missing.json")); err == nil {
		t.Fatal("missing file must fail")
	}
	if _, err := FromJSON([]byte("{ not json")); err == nil {
		t.Fatal("bad json must fail")
	}
	if _, err := FromJSON([]byte("[1,2]")); err == nil {
		t.Fatal("array root must fail")
	}
}

// The settings log line must never carry a password: not under a key that says so, and not
// inside a connection string either.
func TestFlattenRedactsCredentials(t *testing.T) {
	c, err := FromJSON([]byte(`{"gateway": {"db": "postgres://jx:s3cret@db:5432/jx?sslmode=disable", "dsn2": "host=db user=jx password=s3cret", "password": "x", "listen": ":19100"}}`))
	if err != nil {
		t.Fatal(err)
	}
	got := map[string]string{}
	for _, kv := range c.Flatten() {
		got[kv[0]] = kv[1]
	}
	if got["gateway.db"] != "postgres://jx:***@db:5432/jx?sslmode=disable" {
		t.Errorf("url credentials not redacted: %q", got["gateway.db"])
	}
	if got["gateway.dsn2"] != "host=db user=jx password=***" {
		t.Errorf("key=value password not redacted: %q", got["gateway.dsn2"])
	}
	if got["gateway.password"] != "***" || got["gateway.listen"] != ":19100" {
		t.Errorf("masking by key: %v", got)
	}
}
