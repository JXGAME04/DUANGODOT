package npcres

import (
	"strconv"
	"strings"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/text"
)

// SkillFile is Settings/skills.txt, the table KSkillManager loads in the old core.
const SkillFile = `\settings\skills.txt`

// Skill is the part of a skills.txt row the npc AI needs (KSkill::GetAttackRadius and the
// target flags); the damage side of a skill waits for the skill system.
type Skill struct {
	ID           int    `json:"id"`
	Name         string `json:"name"`          // UTF-8 (the file stores TCVN3)
	Style        int    `json:"style"`         // SkillStyle column
	AttackRadius int    `json:"attack_radius"` // scene units: KNpc::m_CurrentAttackRadius while the skill is active
	IsMelee      bool   `json:"melee"`         // IsMelee: the swing uses AttackFrame, anything else CastFrame
	TargetEnemy  bool   `json:"target_enemy"`
	TargetSelf   bool   `json:"target_self"`
}

// ParseSkills reads skills.txt keyed by SkillId; the first row of an id wins (KSkillManager keeps
// one KSkill per id).
func ParseSkills(data []byte) map[int]Skill {
	tab := ParseTab(data)
	out := map[int]Skill{}
	for row := 2; row <= tab.Height(); row++ {
		id, err := strconv.Atoi(strings.TrimSpace(tab.GetByName(row, "SkillId")))
		if err != nil || id <= 0 {
			continue
		}
		if _, dup := out[id]; dup {
			continue
		}
		out[id] = Skill{
			ID:           id,
			Name:         text.TCVN3ToUTF8([]byte(strings.TrimSpace(tab.GetByName(row, "SkillName")))),
			Style:        Atoi(tab.GetByName(row, "SkillStyle")),
			AttackRadius: Atoi(tab.GetByName(row, "AttackRadius")),
			IsMelee:      Atoi(tab.GetByName(row, "IsMelee")) != 0,
			TargetEnemy:  Atoi(tab.GetByName(row, "TargetEnemy")) != 0,
			TargetSelf:   Atoi(tab.GetByName(row, "TargetSelf")) != 0,
		}
	}
	return out
}

// Atoi is the C atoi the old KTabFile::GetInteger relies on: leading blanks, an optional sign and
// the leading digits count, anything else is 0 ("1|0" -> 1, "abc" -> 0).
func Atoi(s string) int {
	i := 0
	for i < len(s) && (s[i] == ' ' || s[i] == '\t') {
		i++
	}
	neg := false
	if i < len(s) && (s[i] == '+' || s[i] == '-') {
		neg = s[i] == '-'
		i++
	}
	n := 0
	for i < len(s) && s[i] >= '0' && s[i] <= '9' {
		n = n*10 + int(s[i]-'0')
		i++
	}
	if neg {
		return -n
	}
	return n
}
