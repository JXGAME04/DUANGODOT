package persist

import (
	"context"
	"os"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// storeScenario is the contract every Store keeps, run against the file store and, when
// JX_TEST_PG names a database (CI does), against PostgreSQL.  reopen returns a fresh handle on
// the same data, the way a restarted gateway would get one.
func storeScenario(t *testing.T, s Store, reopen func(t *testing.T) Store) {
	t.Helper()
	ctx := context.Background()

	acc, err := s.CreateAccount(ctx, "tester", "$hash")
	if err != nil || acc.ID == 0 || acc.Name != "tester" || acc.PasswordHash != "$hash" {
		t.Fatalf("CreateAccount: %v %+v", err, acc)
	}
	if _, err := s.CreateAccount(ctx, "TESTER", "other"); err != ErrExists {
		t.Fatal("account names must be unique case-insensitively")
	}
	again, _ := s.Account(ctx, "TESTER")
	if again == nil || again.ID != acc.ID || again.PasswordHash != "$hash" {
		t.Fatalf("account lookup must be case-insensitive: %+v", again)
	}
	if _, err := s.CreateAccount(ctx, "a b", "x"); err != ErrInvalidName {
		t.Fatal("bad account name accepted")
	}
	if _, err := s.Account(ctx, "nobody"); err != ErrNotFound {
		t.Fatalf("unknown account: %v", err)
	}
	again.Frozen, again.FrozenText, again.Logins, again.LastAddr = true, "test", 3, "127.0.0.1"
	if err := s.UpdateAccount(ctx, again); err != nil {
		t.Fatal(err)
	}
	if byID, err := s.AccountByID(ctx, acc.ID); err != nil || !byID.Frozen || byID.FrozenText != "test" || byID.Logins != 3 {
		t.Fatalf("UpdateAccount not applied: %v %+v", err, byID)
	}
	if err := s.UpdateAccount(ctx, &Account{ID: 99999}); err != ErrNotFound {
		t.Fatal("update of an unknown account accepted")
	}
	if list, _ := s.Accounts(ctx); len(list) != 1 || list[0].Name != "tester" {
		t.Fatalf("Accounts: %+v", list)
	}

	role, err := s.CreateCharacter(ctx, acc.ID, NewCharacter{Name: "ĐạiHiệp", Series: 1, Sex: 0, NativePlace: 53})
	if err != nil || role.PlayerId == 0 || role.Name != "ĐạiHiệp" || role.Level != 1 || role.Stats.MoveSpeed != 200 || role.NativePlace != 53 {
		t.Fatalf("CreateCharacter: %v %+v", err, role)
	}
	if role.AccountId != acc.ID || role.DataVersion != CurrentRoleVersion {
		t.Fatalf("new role: account %d version %d", role.AccountId, role.DataVersion)
	}
	if _, err := s.CreateCharacter(ctx, acc.ID, NewCharacter{Name: "đạihiệp", Series: 1, Sex: 0}); err != ErrExists {
		t.Fatal("duplicate name (case-insensitive) accepted")
	}
	if _, err := s.CreateCharacter(ctx, acc.ID, NewCharacter{Name: "x", Series: 1, Sex: 0}); err != ErrInvalidName {
		t.Fatal("short name accepted")
	}
	if _, err := s.CreateCharacter(ctx, 999999, NewCharacter{Name: "Nobody", Series: 1, Sex: 0}); err != ErrNotFound {
		t.Fatal("unknown account accepted")
	}
	if a, _ := s.AccountByID(ctx, acc.ID); len(a.Chars) != 1 || a.Chars[0] != role.PlayerId {
		t.Fatalf("the account must list its character: %+v", a)
	}

	role.Level = 7
	role.Position.ZoneId = 1
	role.Position.Pos = nil
	if err := s.SaveCharacter(ctx, role); err != nil {
		t.Fatal(err)
	}
	role.AccountId = 42
	if err := s.SaveCharacter(ctx, role); err != ErrForbidden {
		t.Fatal("save with another account accepted")
	}
	role.AccountId = acc.ID
	role.DataVersion = CurrentRoleVersion + 1
	if err := s.SaveCharacter(ctx, role); err != ErrNewerData {
		t.Fatalf("a record of a newer server must be refused: %v", err)
	}
	role.DataVersion = CurrentRoleVersion
	if err := s.SaveCharacter(ctx, &jxpb.RoleData{PlayerId: role.PlayerId + 100000, AccountId: acc.ID, DataVersion: CurrentRoleVersion}); err != ErrNotFound {
		t.Fatalf("save of an unknown character: %v", err)
	}

	// a restarted gateway sees everything that was acknowledged
	s2 := reopen(t)
	chars, err := s2.Characters(ctx, acc.ID)
	if err != nil || len(chars) != 1 || chars[0].Level != 7 || chars[0].Name != "ĐạiHiệp" || chars[0].NativePlace != 53 {
		t.Fatalf("Characters after reopen: %v %+v", err, chars)
	}
	if reopened, err := s2.Account(ctx, "tester"); err != nil || !reopened.Frozen || reopened.PasswordHash != "$hash" || reopened.Logins != 3 {
		t.Fatalf("account fields after reopen: %v %+v", err, reopened)
	}
	second, err := s2.CreateCharacter(ctx, acc.ID, NewCharacter{Name: "Second", Series: 2, Sex: 1})
	if err != nil {
		t.Fatal(err)
	}
	chars, _ = s2.Characters(ctx, acc.ID)
	if len(chars) != 2 || chars[1].PlayerId != second.PlayerId || second.PlayerId <= role.PlayerId {
		t.Fatalf("player ids must continue after reopen: %+v", chars)
	}
	if got, err := s2.Character(ctx, second.PlayerId); err != nil || got.Name != "Second" || got.Series != 2 || got.Sex != 1 {
		t.Fatalf("Character: %v %+v", err, got)
	}
	if _, err := s2.Character(ctx, 77777777); err != ErrNotFound {
		t.Fatal("missing character")
	}
	if _, err := s2.Characters(ctx, 999999); err != ErrNotFound {
		t.Fatal("characters of an unknown account")
	}
	if err := s2.Close(); err != nil {
		t.Fatal(err)
	}
}

func TestFileStoreRoundTrip(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	dir := t.TempDir()
	s, err := OpenFileStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	storeScenario(t, s, func(t *testing.T) Store {
		if err := s.Flush(); err != nil { // a real shutdown flushes; the test does the same
			t.Fatal(err)
		}
		s2, err := OpenFileStore(dir)
		if err != nil {
			t.Fatal(err)
		}
		return s2
	})
}

// TestPgStoreRoundTrip needs a database: JX_TEST_PG=postgres://user:pass@host:5432/db (the CI
// job starts one).  Without it the test is skipped, not failed.
func TestPgStoreRoundTrip(t *testing.T) {
	dsn := os.Getenv("JX_TEST_PG")
	if dsn == "" {
		t.Skip("JX_TEST_PG not set: no PostgreSQL to test against")
	}
	_ = log.Init(log.Options{Level: log.LevelWarn})
	ctx := context.Background()
	s, err := OpenPgStore(ctx, dsn)
	if err != nil {
		t.Fatal(err)
	}
	if err := s.Reset(ctx); err != nil {
		t.Fatal(err)
	}
	storeScenario(t, s, func(t *testing.T) Store {
		s2, err := OpenPgStore(ctx, dsn) // a second pool on the same database: what a restart gets
		if err != nil {
			t.Fatal(err)
		}
		return s2
	})
	s.Close()
}

func TestRedactDSN(t *testing.T) {
	cases := map[string]string{
		"postgres://jx:secret@db.local:5432/jx?sslmode=disable": "postgres://jx:***@db.local:5432/jx?sslmode=disable",
		"postgres://jx@db.local/jx":                             "postgres://jx@db.local/jx",
		"host=localhost user=jx password=secret dbname=jx":      "host=localhost user=jx password=*** dbname=jx",
	}
	for in, want := range cases {
		if got := RedactDSN(in); got != want {
			t.Errorf("RedactDSN(%q) = %q, want %q", in, got, want)
		}
	}
}
