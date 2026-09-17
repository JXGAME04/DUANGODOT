package persist

import (
	"context"
	"testing"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

func TestFileStoreRoundTrip(t *testing.T) {
	_ = log.Init(log.Options{Level: log.LevelWarn})
	dir := t.TempDir()
	ctx := context.Background()

	s, err := OpenFileStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	acc, err := s.CreateAccount(ctx, "tester", "$hash")
	if err != nil || acc.ID != 1 || acc.Name != "tester" || acc.PasswordHash != "$hash" {
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
	again.Frozen, again.FrozenText, again.Logins, again.LastAddr = true, "test", 3, "127.0.0.1"
	if err := s.UpdateAccount(ctx, again); err != nil {
		t.Fatal(err)
	}
	if byID, err := s.AccountByID(ctx, acc.ID); err != nil || !byID.Frozen || byID.FrozenText != "test" || byID.Logins != 3 {
		t.Fatalf("UpdateAccount not applied: %v %+v", err, byID)
	}
	if err := s.UpdateAccount(ctx, &Account{ID: 99}); err != ErrNotFound {
		t.Fatal("update of an unknown account accepted")
	}
	if list, _ := s.Accounts(ctx); len(list) != 1 || list[0].Name != "tester" {
		t.Fatalf("Accounts: %+v", list)
	}

	role, err := s.CreateCharacter(ctx, acc.ID, "Đại Hiệp", 1, 0)
	if err != nil || role.PlayerId != 1 || role.Name != "Đại Hiệp" || role.Level != 1 || role.Stats.MoveSpeed != 200 {
		t.Fatalf("CreateCharacter: %v %+v", err, role)
	}
	if _, err := s.CreateCharacter(ctx, acc.ID, "đại hiệp", 1, 0); err != ErrExists {
		t.Fatal("duplicate name (case-insensitive) accepted")
	}
	if _, err := s.CreateCharacter(ctx, acc.ID, "x", 1, 0); err != ErrInvalidName {
		t.Fatal("short name accepted")
	}
	if _, err := s.CreateCharacter(ctx, 999, "Nobody", 1, 0); err != ErrNotFound {
		t.Fatal("unknown account accepted")
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

	// reopen from disk (a real shutdown flushes; the test does the same)
	if err := s.Flush(); err != nil {
		t.Fatal(err)
	}
	s2, err := OpenFileStore(dir)
	if err != nil {
		t.Fatal(err)
	}
	chars, err := s2.Characters(ctx, acc.ID)
	if err != nil || len(chars) != 1 || chars[0].Level != 7 || chars[0].Name != "Đại Hiệp" {
		t.Fatalf("Characters after reopen: %v %+v", err, chars)
	}
	if reopened, err := s2.Account(ctx, "tester"); err != nil || !reopened.Frozen || reopened.PasswordHash != "$hash" {
		t.Fatalf("account fields after reopen: %v %+v", err, reopened)
	}
	if _, err := s2.CreateCharacter(ctx, acc.ID, "Second", 2, 1); err != nil {
		t.Fatal(err)
	}
	chars, _ = s2.Characters(ctx, acc.ID)
	if len(chars) != 2 || chars[1].PlayerId != 2 {
		t.Fatalf("player ids must continue after reopen: %+v", chars)
	}
	if _, err := s2.Character(ctx, 77); err != ErrNotFound {
		t.Fatal("missing character")
	}
}

func TestValidateName(t *testing.T) {
	good := []string{"Ab", "Đại Hiệp", "Tiểu Long Nữ", "abcdefghijklmnop"}
	bad := []string{"", "a", " ab", "ab ", "abcdefghijklmnopq", "a\tb", "\xff\xfe", "a\x00b"}
	for _, n := range good {
		if err := ValidateName(n); err != nil {
			t.Errorf("%q rejected", n)
		}
	}
	for _, n := range bad {
		if err := ValidateName(n); err == nil {
			t.Errorf("%q accepted", n)
		}
	}
}
