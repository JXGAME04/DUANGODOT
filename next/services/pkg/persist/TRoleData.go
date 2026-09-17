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
const CurrentRoleVersion uint32 = 2

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
