package player

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
)

// RevivePos is \settings\revivepos.ini of the old server: one section per map id ("[20]") holding
// the revive / reference points the JX2 server looks up - KSubWorldSet 0x080F6D20 reads section
// "%u" (the map), key "%d" (the id) as "x,y" in absolute Mps through KIniFile 0x0821F1F0 - and a
// "region=a,b" line naming the ids a..b that belong to the map (the Bishop's CPlayerCreator picked a
// new character's revival id among its map's ids; the old switch hard-coded 10 for map 20 and 19
// for 53, the first of each region).  KPlayer::LoadFrom 0x080C171D puts a character that still
// has cUseRevive (a fresh one) at the point (irevivalid, irevivalx); one the table lacks sends it
// to map 57 at 50976,102208 (0x080C2038).  The zone resolves KPlayer::Revive(0) and the script
// SetRevPos(map, ref) through the same table.
type RevivePos struct {
	Source string               `json:"source"`
	Maps   map[string]ReviveMap `json:"maps"` // key: the map id
}

// ReviveMap is one [map] section.
type ReviveMap struct {
	Region [2]int            `json:"region"` // "region=a,b": the ids a..b of this map (0,0 when the line is missing)
	Points map[string][2]int `json:"points"` // key: the id, value: x, y in absolute Mps
}

const (
	DefaultLoginMap = 57     // KPlayer::LoadFrom 0x080C2038: where a character goes when its revive point is unknown
	DefaultLoginX   = 50976  // 0xc720
	DefaultLoginY   = 102208 // 0x18f40
)

// ParseRevivePos reads the ini text: sections whose name is a number, "region=a,b" and "<id>=x,y"
// lines; anything else (comments, other keys, malformed pairs) is skipped like KIniFile would.
func ParseRevivePos(data []byte) *RevivePos {
	r := &RevivePos{Maps: map[string]ReviveMap{}}
	data = bytes.TrimPrefix(data, []byte{0xEF, 0xBB, 0xBF})
	cur := ""
	var m ReviveMap
	flush := func() {
		if cur != "" {
			r.Maps[cur] = m
		}
	}
	for _, raw := range strings.Split(string(data), "\n") {
		line := strings.TrimSpace(strings.TrimRight(raw, "\r"))
		if line == "" || line[0] == ';' || line[0] == '/' || line[0] == '#' {
			continue
		}
		if line[0] == '[' {
			flush()
			cur = ""
			end := strings.IndexByte(line, ']')
			if end < 0 {
				continue
			}
			name := strings.TrimSpace(line[1:end])
			if _, err := strconv.Atoi(name); err != nil {
				continue // not a map section
			}
			cur = name
			if old, ok := r.Maps[cur]; ok {
				m = old
			} else {
				m = ReviveMap{Points: map[string][2]int{}}
			}
			continue
		}
		if cur == "" {
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq < 0 {
			continue
		}
		key := strings.TrimSpace(line[:eq])
		parts := strings.Split(line[eq+1:], ",")
		if len(parts) != 2 {
			continue
		}
		a, errA := strconv.Atoi(strings.TrimSpace(parts[0]))
		b, errB := strconv.Atoi(strings.TrimSpace(parts[1]))
		if errA != nil || errB != nil {
			continue
		}
		if key == "region" {
			m.Region = [2]int{a, b}
			continue
		}
		if _, err := strconv.Atoi(key); err != nil {
			continue
		}
		m.Points[key] = [2]int{a, b}
	}
	flush()
	return r
}

// Point is KSubWorldSet 0x080F6D20: the x, y of point ref of the map, ok = false when the ini has
// no such section or key.
func (r *RevivePos) Point(mapID, ref int) (x, y int, ok bool) {
	if r == nil {
		return 0, 0, false
	}
	m, found := r.Maps[strconv.Itoa(mapID)]
	if !found {
		return 0, 0, false
	}
	p, found := m.Points[strconv.Itoa(ref)]
	if !found {
		return 0, 0, false
	}
	return p[0], p[1], true
}

// Region is the "region=a,b" line of the map: the ids that belong to it, ok = false without one.
func (r *RevivePos) Region(mapID int) (lo, hi int, ok bool) {
	if r == nil {
		return 0, 0, false
	}
	m, found := r.Maps[strconv.Itoa(mapID)]
	if !found || (m.Region[0] == 0 && m.Region[1] == 0) {
		return 0, 0, false
	}
	return m.Region[0], m.Region[1], true
}

// MapIDs lists the maps of the table in order (for reports and tests).
func (r *RevivePos) MapIDs() []int {
	ids := make([]int, 0, len(r.Maps))
	for k := range r.Maps {
		if id, err := strconv.Atoi(k); err == nil {
			ids = append(ids, id)
		}
	}
	sort.Ints(ids)
	return ids
}

// Write stores the table as JSON (the zone's KRevivePosTable and the gateway read it back).
func (r *RevivePos) Write(path string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	data, err := json.MarshalIndent(r, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, append(data, '\n'), 0o644)
}

// ReadRevivePos loads a revive_pos.json written by Write.
func ReadRevivePos(path string) (*RevivePos, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	r := &RevivePos{}
	if err := json.Unmarshal(data, r); err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	if r.Maps == nil {
		r.Maps = map[string]ReviveMap{}
	}
	return r, nil
}
