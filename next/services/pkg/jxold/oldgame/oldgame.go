// Package oldgame finds the old game's folders on a developer machine, for the tests that read
// real data and skip without it.
//
// There are two kinds of client and the tests differ in which one they need: the JX1 client of the
// swrod3 checkout (package.ini + Data/*.pak) and the VLTK 2.0 client (config.ini + data/*.pak with
// the nested archive \reslst.dat).  Pointing JX_OLD_CLIENT at one of them used to break the tests
// written for the other; now every test names the kind it wants and gets a folder of that kind or
// skips.
//
// Looked at, in this order: JX_OLD_CLIENT / JX_OLD_SERVER (a chain "a;b" is fine),
// JX_VLTK20_CLIENT, config/oldgame.local.json of this checkout (client, client_fallback, server,
// server_fallback), bin/Client and bin/Server next to next/, and the 2.0 client's default install
// folder.
package oldgame

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
)

// nextRoot walks up from the working directory to the folder that holds config/oldgame.example.json.
func nextRoot() string {
	dir, err := os.Getwd()
	if err != nil {
		return ""
	}
	for {
		if _, err := os.Stat(filepath.Join(dir, "config", "oldgame.example.json")); err == nil {
			return dir
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			return ""
		}
		dir = parent
	}
}

func localConfig(root string) map[string]string {
	out := map[string]string{}
	if root == "" {
		return out
	}
	data, err := os.ReadFile(filepath.Join(root, "config", "oldgame.local.json"))
	if err != nil {
		return out
	}
	_ = json.Unmarshal(data, &out)
	return out
}

func candidates(env []string, keys []string, rel string, extra ...string) []string {
	var out []string
	for _, name := range env {
		for _, p := range strings.Split(os.Getenv(name), ";") {
			if p = strings.TrimSpace(p); p != "" {
				out = append(out, p)
			}
		}
	}
	root := nextRoot()
	cfg := localConfig(root)
	for _, k := range keys {
		if p := strings.TrimSpace(cfg[k]); p != "" {
			out = append(out, p)
		}
	}
	if root != "" {
		out = append(out, filepath.Join(filepath.Dir(root), rel))
	}
	return append(out, extra...)
}

func exists(parts ...string) bool {
	_, err := os.Stat(filepath.Join(parts...))
	return err == nil
}

func clientCandidates() []string {
	var extra []string
	if home, err := os.UserHomeDir(); err == nil {
		extra = append(extra, filepath.Join(home, "Level Up Games", "Vo Lam Truyen Ky 2.0"))
	}
	return candidates([]string{"JX_OLD_CLIENT", "JX_VLTK20_CLIENT"}, []string{"client", "client_fallback"}, filepath.Join("bin", "Client"), extra...)
}

// ClientJX1 returns a JX1 client folder (package.ini), "" when there is none.
func ClientJX1() string {
	for _, c := range clientCandidates() {
		if exists(c, "package.ini") {
			return c
		}
	}
	return ""
}

// ClientVLTK20 returns a VLTK 2.0 client folder (config.ini + data/font.pak), "" when there is none.
func ClientVLTK20() string {
	for _, c := range clientCandidates() {
		if exists(c, "config.ini") && exists(c, "data", "font.pak") {
			return c
		}
	}
	return ""
}

// ServerJX1 returns the old Windows server folder (package.ini + pak/maps.pak), "" when there is none.
func ServerJX1() string {
	for _, c := range candidates([]string{"JX_OLD_SERVER"}, []string{"server", "server_fallback"}, filepath.Join("bin", "Server")) {
		if exists(c, "package.ini") {
			return c
		}
	}
	return ""
}
