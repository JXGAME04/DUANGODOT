package persist

import (
	"context"
	"errors"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/player"
	"os"
	"path/filepath"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

func TestMigrateRoleFromUnversioned(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	r := &jxpb.RoleData{PlayerId: 3, Name: "Cũ"} // written before data_version existed
	changed, err := MigrateRole(r)
	if err != nil || !changed {
		t.Fatalf("migrate: %v %v", changed, err)
	}
	if r.DataVersion != CurrentRoleVersion || r.Level != 1 || r.Position == nil {
		t.Fatalf("v1 step: %+v", r)
	}
	if r.Stats.HpMax != 100 || r.Stats.Hp != 100 || r.Stats.MoveSpeed != 200 || r.Stats.StaminaMax != 100 {
		t.Fatalf("v2 step: %+v", r.Stats)
	}
	if changed, err := MigrateRole(r); changed || err != nil {
		t.Fatalf("second run must be a no-op: %v %v", changed, err)
	}
}

func TestMigrateRoleRepairsBrokenStats(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	r := &jxpb.RoleData{PlayerId: 4, Name: "Hỏng", Level: 7, DataVersion: 1,
		Position: &jxpb.RolePosition{ZoneId: 1},
		Stats:    &jxpb.RoleStats{Hp: 999, HpMax: 300, Mp: -5, MpMax: 0, MoveSpeed: 0}}
	if _, err := MigrateRole(r); err != nil {
		t.Fatal(err)
	}
	if r.Stats.Hp != 300 || r.Stats.MpMax != 50 || r.Stats.Mp != 50 || r.Stats.MoveSpeed != 200 {
		t.Fatalf("stats not repaired: %+v", r.Stats)
	}
	if r.Level != 7 { // the v1 step must not touch a record that is already at v1
		t.Fatalf("level changed: %+v", r)
	}
}

func TestNewerRecordIsRefused(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	r := &jxpb.RoleData{PlayerId: 5, DataVersion: CurrentRoleVersion + 1}
	if _, err := MigrateRole(r); !errors.Is(err, ErrNewerData) {
		t.Fatalf("newer record accepted: %v", err)
	}

	dir := t.TempDir()
	s, err := OpenFileStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	ctx := context.Background()
	acc, _ := s.CreateAccount(ctx, "owner", "$h")
	role, err := s.CreateCharacter(ctx, acc.ID, NewCharacter{Name: "HiệpKhách", Series: 1, Sex: 0})
	if err != nil {
		t.Fatal(err)
	}
	if role.DataVersion != CurrentRoleVersion {
		t.Fatalf("new character must carry the current version: %+v", role)
	}
	role.DataVersion = CurrentRoleVersion + 5
	if err := s.SaveCharacter(ctx, role); !errors.Is(err, ErrNewerData) {
		t.Fatalf("save of a newer record accepted: %v", err)
	}
}

func TestStoreMigratesOnOpenAndWritesBack(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	dir := t.TempDir()
	if err := os.MkdirAll(filepath.Join(dir, "chars"), 0o755); err != nil {
		t.Fatal(err)
	}
	// a character file as the first builds wrote it: no data_version, no stats
	const old = `{"playerId":"7","accountId":"1","name":"Lão Tướng","level":12}`
	if err := os.WriteFile(filepath.Join(dir, "chars", "7.json"), []byte(old), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "accounts.json"),
		[]byte(`{"next_account":2,"next_player":8,"accounts":{"a":{"id":1,"name":"a","chars":[7]}}}`), 0o644); err != nil {
		t.Fatal(err)
	}
	s, err := OpenFileStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	if err := s.Flush(); err != nil {
		t.Fatal(err)
	}
	role, err := s.Character(context.Background(), 7)
	if err != nil || role.DataVersion != CurrentRoleVersion || role.Stats.HpMax != 100 || role.Level != 12 {
		t.Fatalf("not migrated: %v %+v", err, role)
	}
	// written back: a second open finds nothing left to do
	raw, _ := os.ReadFile(filepath.Join(dir, "chars", "7.json"))
	if len(raw) == len(old) {
		t.Fatal("record was not written back")
	}
	if _, err := OpenFileStore(dir); err != nil {
		t.Fatal(err)
	}

	// a record from a newer server stops the store instead of being downgraded
	if err := os.WriteFile(filepath.Join(dir, "chars", "8.json"),
		[]byte(`{"playerId":"8","accountId":"1","name":"Tương Lai","dataVersion":99}`), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := OpenFileStore(dir); !errors.Is(err, ErrNewerData) {
		t.Fatalf("newer record loaded: %v", err)
	}
}

func TestMigrationThreeReplacesThePlaceholderPoints(t *testing.T) {
	set := &player.Set{}
	set.LevelAdd[0] = player.LevelAdd{LifePerLevel: 4, StaminaMalePerLevel: 9, StaminaFemalePerLevel: 8, ManaPerLevel: 1, LifePerVitality: 8, ManaPerEnergy: 1, StaminaMaleBase: 180, StaminaFemaleBase: 180}
	set.NewPlayer[0] = player.NewPlayer{Present: true, Strength: 35, Dexterity: 25, Vitality: 25, Energy: 15, LifeMax: 204, ManaMax: 16, Level: 1}
	SetNewPlayerSet(set)
	defer SetNewPlayerSet(nil)
	// a level-9 record with the old placeholders (10/10/10/10, 100 / 50)
	r := &jxpb.RoleData{PlayerId: 1, Name: "Cu", Level: 9, Series: 0, Sex: 0, DataVersion: 2,
		Stats: &jxpb.RoleStats{Hp: 100, HpMax: 100, Mp: 50, MpMax: 50, Stamina: 100, StaminaMax: 100, Strength: 10, Dexterity: 10, Vitality: 10, Energy: 10, MoveSpeed: 200}}
	changed, err := MigrateRole(r)
	if err != nil || !changed || r.DataVersion != CurrentRoleVersion {
		t.Fatalf("migrate: %v %v %d", err, changed, r.DataVersion)
	}
	s := r.Stats
	if s.Strength != 35 || s.Dexterity != 25 || s.Vitality != 25 || s.Energy != 15 {
		t.Fatalf("points %+v", s)
	}
	if s.HpMax != 204+4*8 || s.MpMax != 16+8 || s.StaminaMax != 180+9*8 || s.AttributePoint != 5*8 || s.SkillPoint != 8 {
		t.Fatalf("level 9 numbers %+v", s)
	}
	// real numbers are left alone
	r2 := &jxpb.RoleData{PlayerId: 2, Name: "Da", Level: 3, DataVersion: 2,
		Stats: &jxpb.RoleStats{Hp: 300, HpMax: 300, Mp: 20, MpMax: 20, Stamina: 100, StaminaMax: 100, Strength: 40, Dexterity: 12, Vitality: 12, Energy: 12, MoveSpeed: 200}}
	if _, err := MigrateRole(r2); err != nil || r2.Stats.Strength != 40 || r2.Stats.HpMax != 300 {
		t.Fatalf("real numbers changed: %+v", r2.Stats)
	}
}
