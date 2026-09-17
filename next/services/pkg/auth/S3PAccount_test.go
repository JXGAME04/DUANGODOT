package auth

import (
	"context"
	"errors"
	"strings"
	"testing"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

func openStore(t *testing.T) *persist.FileStore {
	t.Helper()
	_ = log.Init(log.Options{Level: log.LevelWarn})
	s, err := persist.OpenFileStore(t.TempDir())
	if err != nil {
		t.Fatal(err)
	}
	return s
}

func TestPasswordHash(t *testing.T) {
	h, err := HashPassword("mật khẩu 123")
	if err != nil || !strings.HasPrefix(h, "$argon2id$v=19$m=19456,t=2,p=1$") {
		t.Fatalf("hash %q %v", h, err)
	}
	if !VerifyPassword(h, "mật khẩu 123") {
		t.Fatal("right password rejected")
	}
	if VerifyPassword(h, "mật khẩu 124") || VerifyPassword(h, "") {
		t.Fatal("wrong password accepted")
	}
	h2, _ := HashPassword("mật khẩu 123")
	if h2 == h {
		t.Fatal("salt must differ between hashes")
	}
	for _, bad := range []string{"", "plain", "$argon2id$v=19$m=0,t=2,p=1$AAAA$BBBB", "$argon2i$v=19$m=19456,t=2,p=1$AAAA$BBBB", "$argon2id$v=19$m=19456,t=2,p=1$*$BBBB"} {
		if VerifyPassword(bad, "x") {
			t.Fatalf("malformed hash %q accepted", bad)
		}
	}
}

func TestDevModeRegistersOnFirstLogin(t *testing.T) {
	s := openStore(t)
	a := New(s, DevOptions())
	ctx := context.Background()
	if a.Mode() != "dev" {
		t.Fatal(a.Mode())
	}
	acc, err := a.Login(ctx, "thu1", "123", "127.0.0.1:5")
	if err != nil || acc.Name != "thu1" || acc.Logins != 1 || acc.LastAddr != "127.0.0.1:5" {
		t.Fatalf("first login: %v %+v", err, acc)
	}
	stored, _ := s.Account(ctx, "thu1")
	if stored.Password != "" || !strings.HasPrefix(stored.PasswordHash, "$argon2id$") {
		t.Fatalf("password must be stored hashed only: %+v", stored)
	}
	if _, err := a.Login(ctx, "THU1", "123", ""); err != nil {
		t.Fatalf("second login: %v", err)
	}
	if _, err := a.Login(ctx, "thu1", "124", ""); !errors.Is(err, ErrAccountOrPassword) {
		t.Fatalf("wrong password: %v", err)
	}
	if _, err := a.Login(ctx, "bad name!", "x", ""); !errors.Is(err, ErrAccountOrPassword) {
		t.Fatalf("bad name: %v", err)
	}
}

func TestStrictModeNeedsRegistration(t *testing.T) {
	s := openStore(t)
	a := New(s, StrictOptions())
	ctx := context.Background()
	if a.Mode() != "strict" {
		t.Fatal(a.Mode())
	}
	if _, err := a.Login(ctx, "nobody", "secret1", ""); !errors.Is(err, ErrAccountOrPassword) {
		t.Fatalf("unknown account: %v", err)
	}
	if _, err := a.Register(ctx, "user1", "short"); !errors.Is(err, ErrWeakPassword) {
		t.Fatalf("weak password accepted: %v", err)
	}
	if _, err := a.Register(ctx, "user1", "secret1"); err != nil {
		t.Fatal(err)
	}
	if _, err := a.Register(ctx, "user1", "secret1"); !errors.Is(err, persist.ErrExists) {
		t.Fatalf("duplicate registration: %v", err)
	}
	if _, err := a.Login(ctx, "user1", "secret1", ""); err != nil {
		t.Fatal(err)
	}
	if err := a.SetPassword(ctx, "user1", "secret2"); err != nil {
		t.Fatal(err)
	}
	if _, err := a.Login(ctx, "user1", "secret1", ""); !errors.Is(err, ErrAccountOrPassword) {
		t.Fatal("old password still works")
	}
	if _, err := a.Login(ctx, "user1", "secret2", ""); err != nil {
		t.Fatal(err)
	}
}

func TestFrozenAndGameTime(t *testing.T) {
	s := openStore(t)
	now := time.Date(2026, 9, 17, 12, 0, 0, 0, time.UTC)
	opt := DevOptions()
	opt.Now = func() time.Time { return now }
	a := New(s, opt)
	ctx := context.Background()
	acc, err := a.Login(ctx, "gm1", "x", "")
	if err != nil {
		t.Fatal(err)
	}
	acc.Frozen, acc.FrozenText = true, "gian lận"
	_ = s.UpdateAccount(ctx, acc)
	if _, err := a.Login(ctx, "gm1", "x", ""); !errors.Is(err, ErrFrozen) {
		t.Fatalf("frozen account logged in: %v", err)
	}
	acc.Frozen = false
	acc.ExpiresAtMs = now.Add(10 * time.Minute).UnixMilli() // less than the 30 minute minimum of the old PaySys
	_ = s.UpdateAccount(ctx, acc)
	if _, err := a.Login(ctx, "gm1", "x", ""); !errors.Is(err, ErrNoGameTime) {
		t.Fatalf("expired account logged in: %v", err)
	}
	acc.ExpiresAtMs = now.Add(2 * time.Hour).UnixMilli()
	_ = s.UpdateAccount(ctx, acc)
	if _, err := a.Login(ctx, "gm1", "x", ""); err != nil {
		t.Fatalf("account with time left: %v", err)
	}
}

func TestLockAfterFailedLogins(t *testing.T) {
	s := openStore(t)
	now := time.Date(2026, 9, 17, 12, 0, 0, 0, time.UTC)
	opt := DevOptions()
	opt.MaxFails, opt.LockFor = 3, 30*time.Second
	opt.Now = func() time.Time { return now }
	a := New(s, opt)
	ctx := context.Background()
	if _, err := a.Login(ctx, "victim", "right", ""); err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 3; i++ {
		if _, err := a.Login(ctx, "victim", "wrong", ""); !errors.Is(err, ErrAccountOrPassword) {
			t.Fatalf("attempt %d: %v", i, err)
		}
	}
	if _, err := a.Login(ctx, "victim", "right", ""); !errors.Is(err, ErrBusy) {
		t.Fatalf("locked account accepted: %v", err)
	}
	now = now.Add(31 * time.Second)
	if _, err := a.Login(ctx, "victim", "right", ""); err != nil {
		t.Fatalf("lock did not expire: %v", err)
	}
}

func TestClearTextPasswordsAreHashedOnStart(t *testing.T) {
	s := openStore(t)
	ctx := context.Background()
	acc, err := s.CreateAccount(ctx, "old1", "")
	if err != nil {
		t.Fatal(err)
	}
	acc.Password = "123" // what the first dev builds stored
	_ = s.UpdateAccount(ctx, acc)
	a := New(s, DevOptions())
	stored, _ := s.Account(ctx, "old1")
	if stored.Password != "" || stored.PasswordHash == "" {
		t.Fatalf("migration did not hash: %+v", stored)
	}
	if _, err := a.Login(ctx, "old1", "123", ""); err != nil {
		t.Fatalf("login after migration: %v", err)
	}
	if _, err := a.Login(ctx, "old1", "321", ""); !errors.Is(err, ErrAccountOrPassword) {
		t.Fatal("wrong password accepted after migration")
	}
}
