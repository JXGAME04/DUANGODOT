package item

import (
	"bufio"
	"bytes"
	"encoding/json"
	"os"
	"strconv"
	"strings"
)

// AbradePartNum is the number of body slots a mode holds in KItemSet (0x0806D560 walks parts 0..14).
const AbradePartNum = 15

// abradeParts are the keys of a mode's section in ITEM_PART order (KItemSet::Init 0x0806E250 stores
// Head at +0x20, Body +0x24, Belt +0x28, Weapon +0x2c, Foot +0x30, Cuff +0x34, Amulet +0x38, Ring1
// +0x3c, Ring2 +0x40, Pendant +0x44, Horse +0x48, Mask +0x4c); the three JX2 slots after them (mantle,
// signet, shipin) have no key and stay 0.
var abradeParts = [...]string{"Head", "Body", "Belt", "Weapon", "Foot", "Cuff", "Amulet", "Ring1", "Ring2", "Pendant", "Horse", "Mask"}

// AbradeRepair is [Repair] of AbradeRate.ini (KItemSet+0x188, +0x18c, +0x190): the repair price
// scales and the durability line below which the client warns.
type AbradeRepair struct {
	ItemPriceScale  int `json:"item_price_scale"`
	MagicPriceScale int `json:"magic_price_scale"`
	WarningBaseline int `json:"warning_baseline"`
}

// AbradeRateModes holds one one-in-N chance per body slot for each of the three things a player
// does: an attack ([Attack], mode 0: KNpc::CastSkill 0x08088350), a hit taken ([Defend], mode 1:
// KNpc::ReceiveDamage 0x0808B148), a step ([Move], mode 2: 0x0807C2F0).  0 = never wears.
type AbradeRateModes struct {
	Attack [AbradePartNum]int `json:"attack"`
	Defend [AbradePartNum]int `json:"defend"`
	Move   [AbradePartNum]int `json:"move"`
}

// AbradeRate is \settings\item\AbradeRate.ini as KItemSet::Init of the JX2 server (jx_linux_y
// 0x0806E250) reads it: [Repair], the three modes, and the same three again as [AdvPlatina_Attack /
// AdvPlatina_Defend / AdvPlatina_Move] (KItemSet+0xd4, +0x110, +0x14c) that KItemSet 0x0806D560 uses
// for an amulet (detail 4) whose tier byte (+0x344) is above 5.  -> abrade_rate.json for the zone's
// KAbradeRate (docs/LINUX-SERVER.md §16.3).
type AbradeRate struct {
	Source  string          `json:"source,omitempty"`
	Repair  AbradeRepair    `json:"repair"`
	Rate    AbradeRateModes `json:"rate"`
	AdvRate AbradeRateModes `json:"adv_rate"`
}

// parseAbradeIni reads "[section]" / "key=value" lines; section and key names are lower-cased,
// ';' lines are comments (KIniFile).
func parseAbradeIni(data []byte) map[string]map[string]string {
	out := map[string]map[string]string{}
	section := ""
	sc := bufio.NewScanner(bytes.NewReader(bytes.TrimPrefix(data, []byte{0xef, 0xbb, 0xbf})))
	for sc.Scan() {
		line := strings.TrimSpace(sc.Text())
		if line == "" || line[0] == ';' || line[0] == '#' {
			continue
		}
		if line[0] == '[' {
			if end := strings.IndexByte(line, ']'); end > 0 {
				section = strings.ToLower(strings.TrimSpace(line[1:end]))
				if _, ok := out[section]; !ok {
					out[section] = map[string]string{}
				}
			}
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq <= 0 || section == "" {
			continue
		}
		out[section][strings.ToLower(strings.TrimSpace(line[:eq]))] = strings.TrimSpace(line[eq+1:])
	}
	return out
}

// ParseAbradeRate reads the ini the way KItemSet::Init does: KIniFile::GetInteger of every key,
// a missing key is 0.
func ParseAbradeRate(data []byte) AbradeRate {
	ini := parseAbradeIni(data)
	get := func(section, key string) int {
		v, _ := strconv.Atoi(strings.TrimSpace(ini[strings.ToLower(section)][strings.ToLower(key)]))
		return v
	}
	fill := func(section string, into *[AbradePartNum]int) {
		for i, p := range abradeParts {
			into[i] = get(section, p)
		}
	}
	var t AbradeRate
	t.Repair = AbradeRepair{get("Repair", "ItemPriceScale"), get("Repair", "MagicPriceScale"), get("Repair", "WarningBaseline")}
	fill("Attack", &t.Rate.Attack)
	fill("Defend", &t.Rate.Defend)
	fill("Move", &t.Rate.Move)
	fill("AdvPlatina_Attack", &t.AdvRate.Attack)
	fill("AdvPlatina_Defend", &t.AdvRate.Defend)
	fill("AdvPlatina_Move", &t.AdvRate.Move)
	return t
}

// Write saves the table as JSON.
func (t *AbradeRate) Write(path string) error {
	blob, err := json.MarshalIndent(t, "", " ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, append(blob, '\n'), 0o644)
}
