package npcres

import "strings"

// Tables of KItemChangeRes (CoreUseNameDef.h): item kind / level -> row of the part tables.
const (
	MeleeResFile = `\settings\item\MeleeRes.txt`
	RangeResFile = `\settings\item\RangeRes.txt`
	ArmorResFile = `\settings\item\ArmorRes.txt`
	HelmResFile  = `\settings\item\HelmRes.txt`
	HorseResFile = `\settings\item\HorseRes.txt`
	GoldResFile  = `\settings\item\goldequipres.txt`
)

// EQUIPDETAILTYPE of KItem.h (the ones the appearance code switches on).
const (
	EquipMeleeWeapon = 0
	EquipRangeWeapon = 1
	EquipArmor       = 2
	EquipHelm        = 7
	EquipHorse       = 10
	EquipMantle      = 12
)

// ItemChangeRes is g_ItemChangeRes: which equipment row a worn item (or nothing) selects in
// the main characters' part tables.  Ported line by line from KItemChangeRes.cpp.
type ItemChangeRes struct {
	melee, ranged, armor, helm, horse, gold *TabFile
}

// LoadItemChangeRes is KItemChangeRes::Init.
func LoadItemChangeRes(read func(gbkPath string) ([]byte, error)) (*ItemChangeRes, error) {
	r := &ItemChangeRes{}
	for _, t := range []struct {
		path string
		dst  **TabFile
	}{{MeleeResFile, &r.melee}, {RangeResFile, &r.ranged}, {ArmorResFile, &r.armor}, {HelmResFile, &r.helm}, {HorseResFile, &r.horse}, {GoldResFile, &r.gold}} {
		data, err := read(t.path) // ASCII paths: GBK == UTF-8
		if err != nil {
			return nil, err
		}
		*t.dst = ParseTab(data)
	}
	return r, nil
}

// WeaponRes is KItemChangeRes::GetWeaponRes; level 0 = nothing worn (bare hands).
func (r *ItemChangeRes) WeaponRes(detail, parti, level int) int {
	if level == 0 {
		return r.melee.GetInteger(2, 2, 2) - 2
	}
	row := parti*10 + level + 2
	ret := 2
	switch detail {
	case EquipMeleeWeapon:
		ret = r.melee.GetInteger(row, 2, 2)
	case EquipRangeWeapon:
		ret = r.ranged.GetInteger(row-1, 2, 2) // no "nothing worn" row in that table
	}
	return ret - 2
}

// ArmorRes is KItemChangeRes::GetArmorRes.
func (r *ItemChangeRes) ArmorRes(parti, level int) int {
	if level == 0 {
		return r.armor.GetInteger(2, 2, 19) - 2
	}
	return r.armor.GetInteger(parti*10+level+2, 2, 19) - 2
}

// HelmRes is KItemChangeRes::GetHelmRes.
func (r *ItemChangeRes) HelmRes(parti, level int) int {
	if level == 0 {
		return r.helm.GetInteger(2, 2, 19) - 2
	}
	return r.helm.GetInteger(parti*10+level+2, 2, 19) - 2
}

// HorseRes is KItemChangeRes::GetHorseRes (-1 = no horse).
func (r *ItemChangeRes) HorseRes(parti, level int) int {
	if level == 0 {
		return -1
	}
	return r.horse.GetInteger(parti*10+level+2, 2, 2) - 2
}

// GoldRes is KItemChangeRes::GetGoldRes.
func (r *ItemChangeRes) GoldRes(detail int) int {
	return r.gold.GetInteger(detail+1, 2, 2) - 2
}

// DefaultEquips is what a character wearing nothing shows (KItemList.cpp / KPlayerDBFuns.cpp:
// GetHelmRes(0,0), GetArmorRes(0,0), GetWeaponRes(0,0,0), no horse, no mantle), keyed by the
// part group (0 head, 1 body, 2 weapon, 3 horse, 4 mantle).
func (r *ItemChangeRes) DefaultEquips() map[int]int {
	out := map[int]int{0: r.HelmRes(0, 0), 1: r.ArmorRes(0, 0), 2: r.WeaponRes(0, 0, 0)}
	if h := r.HorseRes(0, 0); h >= 0 {
		out[3] = h
	}
	return out
}

// GetInteger is KTabFile::GetInteger: atoi of the 1-based cell, the default when the cell is
// out of range.
func (t *TabFile) GetInteger(row, col, def int) int {
	if row < 1 || row > len(t.rows) || col < 1 || col > len(t.rows[row-1]) {
		return def
	}
	return atoi(t.rows[row-1][col-1])
}

// atoi mirrors C atoi: optional sign and leading digits, 0 when there are none.
func atoi(s string) int {
	s = strings.TrimSpace(s)
	neg := false
	if s != "" && (s[0] == '-' || s[0] == '+') {
		neg = s[0] == '-'
		s = s[1:]
	}
	n := 0
	for _, c := range s {
		if c < '0' || c > '9' {
			break
		}
		n = n*10 + int(c-'0')
	}
	if neg {
		return -n
	}
	return n
}
