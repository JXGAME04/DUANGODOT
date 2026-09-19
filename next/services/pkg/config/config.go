// Package config implements the JX NEXT layered configuration (same rules as jx::config):
// JSON file (// comments allowed)  <  environment JX_SECTION__KEY  <  --set section.key=value.
package config

import (
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"sort"
	"strconv"
	"strings"
)

// Config is a tree of JSON values addressed by dotted paths.
type Config struct {
	root map[string]any
}

// New returns an empty configuration.
func New() *Config { return &Config{root: map[string]any{}} }

// FromJSON parses a JSON object; // line comments are stripped first.
func FromJSON(text []byte) (*Config, error) {
	var root map[string]any
	if err := json.Unmarshal(stripComments(text), &root); err != nil {
		return nil, fmt.Errorf("config: invalid JSON: %w", err)
	}
	if root == nil {
		return nil, fmt.Errorf("config: root must be a JSON object")
	}
	return &Config{root: root}, nil
}

// FromFile reads and parses path.
func FromFile(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("config: cannot open %s: %w", path, err)
	}
	c, err := FromJSON(data)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	return c, nil
}

// stripComments removes // comments that are outside of strings.
func stripComments(in []byte) []byte {
	out := make([]byte, 0, len(in))
	inString, escaped := false, false
	for i := 0; i < len(in); i++ {
		ch := in[i]
		if inString {
			out = append(out, ch)
			if escaped {
				escaped = false
			} else if ch == '\\' {
				escaped = true
			} else if ch == '"' {
				inString = false
			}
			continue
		}
		if ch == '"' {
			inString = true
			out = append(out, ch)
			continue
		}
		if ch == '/' && i+1 < len(in) && in[i+1] == '/' {
			for i < len(in) && in[i] != '\n' {
				i++
			}
			out = append(out, '\n')
			continue
		}
		out = append(out, ch)
	}
	return out
}

// SplitPath splits "a.b.c" into keys, dropping empty segments.
func SplitPath(path string) []string {
	var keys []string
	for _, k := range strings.Split(path, ".") {
		if k != "" {
			keys = append(keys, k)
		}
	}
	return keys
}

// Set stores raw at path, auto-typed: valid JSON keeps its type, anything else is a string.
// Missing parents are created; a scalar parent is replaced by an object.
func (c *Config) Set(path, raw string) {
	keys := SplitPath(path)
	if len(keys) == 0 {
		return
	}
	var value any
	if err := json.Unmarshal([]byte(raw), &value); err != nil {
		value = raw
	}
	cur := c.root
	for _, k := range keys[:len(keys)-1] {
		child, ok := cur[k].(map[string]any)
		if !ok {
			child = map[string]any{}
			cur[k] = child
		}
		cur = child
	}
	cur[keys[len(keys)-1]] = value
}

// ApplyOverrides applies path=value pairs (command line --set).
func (c *Config) ApplyOverrides(overrides map[string]string) {
	for k, v := range overrides {
		c.Set(k, v)
	}
}

// EnvKeyToPath maps JX_LOG__LEVEL to log.level ("" when the prefix does not match).
func EnvKeyToPath(key, prefix string) string {
	if len(key) <= len(prefix) || !strings.HasPrefix(key, prefix) {
		return ""
	}
	rest := key[len(prefix):]
	var b strings.Builder
	for i := 0; i < len(rest); i++ {
		if rest[i] == '_' && i+1 < len(rest) && rest[i+1] == '_' {
			b.WriteByte('.')
			i++
			continue
		}
		b.WriteByte(byte(strings.ToLower(string(rest[i]))[0]))
	}
	return b.String()
}

// ApplyEnvList applies KEY=VALUE entries whose key starts with prefix.
func (c *Config) ApplyEnvList(entries []string, prefix string) {
	for _, e := range entries {
		eq := strings.IndexByte(e, '=')
		if eq <= 0 {
			continue
		}
		if p := EnvKeyToPath(e[:eq], prefix); p != "" {
			c.Set(p, e[eq+1:])
		}
	}
}

// ApplyEnv applies the real process environment.
func (c *Config) ApplyEnv(prefix string) { c.ApplyEnvList(os.Environ(), prefix) }

// Get returns the raw value at path.
func (c *Config) Get(path string) (any, bool) {
	var cur any = c.root
	for _, k := range SplitPath(path) {
		m, ok := cur.(map[string]any)
		if !ok {
			return nil, false
		}
		cur, ok = m[k]
		if !ok {
			return nil, false
		}
	}
	return cur, true
}

// Has reports whether path exists.
func (c *Config) Has(path string) bool { _, ok := c.Get(path); return ok }

// String returns a string value (numbers and bools are formatted), or def.
func (c *Config) String(path, def string) string {
	v, ok := c.Get(path)
	if !ok {
		return def
	}
	switch x := v.(type) {
	case string:
		return x
	case float64:
		return strconv.FormatFloat(x, 'f', -1, 64)
	case bool:
		return strconv.FormatBool(x)
	}
	return def
}

// Int returns an integer value (strings are parsed), or def.
func (c *Config) Int(path string, def int64) int64 {
	v, ok := c.Get(path)
	if !ok {
		return def
	}
	switch x := v.(type) {
	case float64:
		return int64(x)
	case bool:
		if x {
			return 1
		}
		return 0
	case string:
		if n, err := strconv.ParseInt(strings.TrimSpace(x), 10, 64); err == nil {
			return n
		}
	}
	return def
}

// Bool returns a boolean (numbers != 0, strings 1/true/yes/on), or def.
func (c *Config) Bool(path string, def bool) bool {
	v, ok := c.Get(path)
	if !ok {
		return def
	}
	switch x := v.(type) {
	case bool:
		return x
	case float64:
		return x != 0
	case string:
		switch strings.ToLower(strings.TrimSpace(x)) {
		case "1", "true", "yes", "on":
			return true
		case "0", "false", "no", "off":
			return false
		}
	}
	return def
}

// Dump returns the configuration as indented JSON (for the cfg log line at startup).
func (c *Config) Dump() string {
	b, _ := json.MarshalIndent(c.root, "", "  ")
	return string(b)
}

// Flatten returns every setting as {"gateway.listen", ":19100"}, sorted by key, for a start-up log
// a person can read line by line.  Keys that start with '_' are notes, not settings; a value whose
// key smells of a secret (password, secret, token) is shown as "***".  Same rule as the C++ side.
func (c *Config) Flatten() [][2]string {
	var out [][2]string
	var walk func(prefix string, v any)
	walk = func(prefix string, v any) {
		switch t := v.(type) {
		case map[string]any:
			for k, child := range t {
				if strings.HasPrefix(k, "_") {
					continue
				}
				key := k
				if prefix != "" {
					key = prefix + "." + k
				}
				walk(key, child)
			}
		case []any:
			for i, child := range t {
				walk(prefix+"."+strconv.Itoa(i), child)
			}
		default:
			lower := strings.ToLower(prefix)
			value := fmt.Sprint(t)
			if s, ok := t.(string); ok {
				value = s
			}
			if strings.Contains(lower, "password") || strings.Contains(lower, "secret") || strings.Contains(lower, "token") {
				value = "***"
			}
			value = redactCredentials(value)
			out = append(out, [2]string{prefix, value})
		}
	}
	walk("", c.root)
	sort.Slice(out, func(a, b int) bool { return out[a][0] < out[b][0] })
	return out
}

var (
	urlCredentials = regexp.MustCompile(`(://[^:/@\s]+:)[^@\s]+@`)
	kvPassword     = regexp.MustCompile(`(?i)(password=)\S+`)
)

// redactCredentials hides a password inside a value: the user:password@ of a connection URL
// (gateway.db = postgres://jx:secret@host/db) and password=... of a key=value list.  The key
// does not say "password", the value carries one.
func redactCredentials(value string) string {
	value = urlCredentials.ReplaceAllString(value, "${1}***@")
	return kvPassword.ReplaceAllString(value, "${1}***")
}
