package persist

import (
	"fmt"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// Character record versioning (the old game's TRoleData in Goddess/BDB had none: a field added
// to the struct silently reinterpreted every saved character, and an older server writing back
// a newer record destroyed the new fields).  Here every record carries data_version:
//
//   - a record older than CurrentRoleVersion is upgraded by MigrateRole on load and written back;
//   - a record newer than CurrentRoleVersion is refused (ErrNewerData) - an older server must
//     never downgrade a character;
//   - adding an optional protobuf field needs no new version; changing the meaning of a field,
//     or repairing existing values, does.
//
// See docs/ADR-003-role-data.md.
const CurrentRoleVersion uint32 = 4

// ErrNewerData is returned when a record was written by a newer server.
var ErrNewerData = fmt.Errorf("persist: character was written by a newer server (data_version > %d)", CurrentRoleVersion)

// roleMigration upgrades a record from version-1 to version.
type roleMigration struct {
	version uint32
	what    string
	apply   func(r *jxpb.RoleData)
}

// roleMigrations runs in order; each step leaves the record at its own version.
var roleMigrations = []roleMigration{
	{
		version: 1,
		what:    "fill the fields of the first unversioned records",
		apply: func(r *jxpb.RoleData) {
			if r.Level == 0 {
				r.Level = 1
			}
			if r.Position == nil {
				r.Position = &jxpb.RolePosition{}
			}
			if r.Stats == nil {
				r.Stats = &jxpb.RoleStats{}
			}
			if r.CreatedAtMs == 0 {
				r.CreatedAtMs = uint64(time.Now().UnixMilli())
			}
		},
	},
	{
		version: 2,
		what:    "repair stats: positive maxima, hp/mp inside them, a usable move speed",
		apply: func(r *jxpb.RoleData) {
			s := r.Stats
			if s.HpMax <= 0 {
				s.HpMax = 100
			}
			if s.MpMax <= 0 {
				s.MpMax = 50
			}
			if s.StaminaMax <= 0 {
				s.StaminaMax = 100
			}
			s.Hp = clampStat(s.Hp, s.HpMax)
			s.Mp = clampStat(s.Mp, s.MpMax)
			s.Stamina = clampStat(s.Stamina, s.StaminaMax)
			if s.MoveSpeed <= 0 {
				s.MoveSpeed = 200
			}
		},
	},
	{
		version: 3,
		what:    "characters made before the player tables: the placeholder points become the newplayerini template of their series and sex",
		apply: func(r *jxpb.RoleData) {
			if NewPlayerSet == nil || !placeholderStats(r.Stats) {
				return
			}
			t := NewPlayerSet.NewPlayerFor(int(r.Series), int(r.Sex))
			if t == nil {
				return
			}
			s := r.Stats
			s.Strength, s.Dexterity, s.Vitality, s.Energy, s.Lucky = int32(t.Strength), int32(t.Dexterity), int32(t.Vitality), int32(t.Energy), int32(t.Lucky)
			// the level's worth of points and life / mana, as if the levels had been earned with the tables
			level := int(r.Level)
			if level < 1 {
				level = 1
			}
			add := NewPlayerSet.LevelAdd[r.Series%uint32(len(NewPlayerSet.LevelAdd))]
			s.HpMax = int32(t.LifeMax + add.LifePerLevel*(level-1))
			s.MpMax = int32(t.ManaMax + add.ManaPerLevel*(level-1))
			s.StaminaMax = int32(NewPlayerSet.GetStaminaBase(int(r.Series), int(r.Sex), level))
			s.Hp, s.Mp, s.Stamina = s.HpMax, s.MpMax, s.StaminaMax
			s.AttributePoint = int32(t.AttributePoint + 5*(level-1))
			s.SkillPoint = int32(t.SkillPoint + (level - 1))
		},
	},
	{
		version: 4,
		what:    "the faction record: the unused faction field read 0 (Shaolin) for every character - a character that never joined carries -1 (KPlayerFaction 0x080C2590)",
		apply: func(r *jxpb.RoleData) {
			if r.FactionCount == 0 {
				r.Faction = -1
				r.FactionLast = -1
			}
		},
	},
}

// placeholderStats reports the numbers NewRole gave before the player tables existed (10 of each
// point, 100 life, 50 mana): a record that still carries them was never played with real numbers.
func placeholderStats(s *jxpb.RoleStats) bool {
	return s != nil && s.Strength == 10 && s.Dexterity == 10 && s.Vitality == 10 && s.Energy == 10 && s.HpMax == 100 && s.MpMax == 50
}

func clampStat(v, max int32) int32 {
	if v <= 0 {
		return max
	}
	if v > max {
		return max
	}
	return v
}

// MigrateRole brings r up to CurrentRoleVersion.  It reports whether anything changed (the
// caller writes the record back) and fails with ErrNewerData for a record from the future.
func MigrateRole(r *jxpb.RoleData) (bool, error) {
	if r == nil {
		return false, ErrNotFound
	}
	if r.DataVersion > CurrentRoleVersion {
		return false, ErrNewerData
	}
	if r.DataVersion == CurrentRoleVersion {
		return false, nil
	}
	from := r.DataVersion
	for _, m := range roleMigrations {
		if m.version > r.DataVersion {
			m.apply(r)
			r.DataVersion = m.version
		}
	}
	r.DataVersion = CurrentRoleVersion
	log.Info("db", "character record migrated", log.F("pid", r.PlayerId), log.F("name", r.Name),
		log.F("from", from), log.F("to", r.DataVersion))
	return true, nil
}
