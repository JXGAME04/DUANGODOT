package auth

import (
	"context"
	"errors"
	"runtime"
	"sync"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

// S3PAccount is the account server of the old game (Sword3PaySys/S3AccServer/S3PAccount.cpp)
// on top of a persist.Store.  S3PAccount::Login checked, in this order: name+password row,
// nobody else logged in (iClientID), game time left (> 30 min), and returned ACTION_SUCCESS /
// E_ACCOUNT_OR_PASSWORD / E_ACCOUNT_EXIST / E_ACCOUNT_FREEZE / E_ACCOUNT_NODEPOSIT.  The
// "logged in elsewhere" part lives in the gateway (it owns the sessions); everything else is
// here, plus a lock after repeated wrong passwords which the old server did not have.
type S3PAccount struct {
	store persist.Store
	opt   Options
	// MASTER SPEC 58 scenario E (login storm): argon2id costs 19 MiB and ~16 ms of CPU per
	// check.  Letting a thousand logins hash at once eats the memory and every core, and the
	// listener then refuses new connections.  This lets a bounded number through at a time;
	// the rest wait a few milliseconds instead of failing.
	hashSlots chan struct{}

	mu    sync.Mutex
	fails map[string]*failState // normalized account name -> wrong password bookkeeping
}

type failState struct {
	count       int
	last        time.Time
	lockedUntil time.Time
}

// New builds the account server and hashes any clear text password left by the first dev
// builds (persist.Account.Password) so the store never keeps one.
func New(store persist.Store, opt Options) *S3PAccount {
	opt.defaults()
	slots := opt.HashConcurrency
	if slots <= 0 {
		slots = runtime.NumCPU() / 2
		if slots < 2 {
			slots = 2
		}
	}
	a := &S3PAccount{store: store, opt: opt, fails: map[string]*failState{}, hashSlots: make(chan struct{}, slots)}
	a.migrateClearText(context.Background())
	return a
}

// hash runs one password operation, bounded by the number of slots.
func (a *S3PAccount) hash(fn func()) {
	a.hashSlots <- struct{}{}
	defer func() { <-a.hashSlots }()
	fn()
}

// Mode implements Authenticator.
func (a *S3PAccount) Mode() string {
	if a.opt.AutoRegister {
		return "dev"
	}
	return "strict"
}

func (a *S3PAccount) migrateClearText(ctx context.Context) {
	accounts, err := a.store.Accounts(ctx)
	if err != nil {
		log.Error("auth", "cannot list accounts", log.F("error", err))
		return
	}
	for _, acc := range accounts {
		if acc.Password == "" || acc.PasswordHash != "" {
			if acc.Password != "" { // hashed already: just drop the clear text
				acc.Password = ""
				_ = a.store.UpdateAccount(ctx, acc)
			}
			continue
		}
		h, err := HashPassword(acc.Password)
		if err != nil {
			log.Error("auth", "cannot hash password", log.F("account", acc.Name), log.F("error", err))
			continue
		}
		acc.PasswordHash, acc.Password = h, ""
		if err := a.store.UpdateAccount(ctx, acc); err != nil {
			log.Error("auth", "cannot store hashed password", log.F("account", acc.Name), log.F("error", err))
			continue
		}
		log.Info("auth", "account password hashed", log.F("account", acc.Name), log.F("account_id", acc.ID))
	}
}

// Register creates an account (jxaccount add, and the dev auto registration).
func (a *S3PAccount) Register(ctx context.Context, account, password string) (*persist.Account, error) {
	if err := persist.ValidateAccountName(account); err != nil {
		return nil, err
	}
	if len(password) < a.opt.MinPassword {
		return nil, ErrWeakPassword
	}
	var h string
	var err error
	a.hash(func() { h, err = HashPassword(password) })
	if err != nil {
		return nil, err
	}
	acc, err := a.store.CreateAccount(ctx, account, h)
	if err != nil {
		return nil, err
	}
	log.Info("auth", "account registered", log.F("account", acc.Name), log.F("account_id", acc.ID), log.F("mode", a.Mode()))
	return acc, nil
}

// SetPassword replaces the password of an existing account (jxaccount passwd).
func (a *S3PAccount) SetPassword(ctx context.Context, account, password string) error {
	if len(password) < a.opt.MinPassword {
		return ErrWeakPassword
	}
	acc, err := a.store.Account(ctx, account)
	if err != nil {
		return err
	}
	var h string
	a.hash(func() { h, err = HashPassword(password) })
	if err != nil {
		return err
	}
	acc.PasswordHash, acc.Password = h, ""
	return a.store.UpdateAccount(ctx, acc)
}

// Login implements Authenticator (S3PAccount::Login).
func (a *S3PAccount) Login(ctx context.Context, account, password, remote string) (*persist.Account, error) {
	if err := persist.ValidateAccountName(account); err != nil {
		return nil, ErrAccountOrPassword
	}
	key := persist.NormalizeName(account)
	now := a.opt.Now()
	if a.locked(key, now) {
		log.Warn("auth", "login while locked", log.F("account", account), log.F("remote", remote))
		return nil, ErrBusy
	}
	acc, err := a.store.Account(ctx, account)
	switch {
	case errors.Is(err, persist.ErrNotFound):
		if !a.opt.AutoRegister {
			// the old PaySys answered E_ACCOUNT_OR_PASSWORD for an unknown name too (one SQL query)
			a.fail(key, now)
			return nil, ErrAccountOrPassword
		}
		if acc, err = a.Register(ctx, account, password); err != nil {
			if errors.Is(err, persist.ErrExists) { // registered by a parallel login: check normally
				acc, err = a.store.Account(ctx, account)
			}
			if err != nil {
				return nil, err
			}
		} else {
			acc.Password = "" // freshly registered: password known good
		}
	case err != nil:
		return nil, err
	}
	ok := false
	a.hash(func() { ok = VerifyPassword(acc.PasswordHash, password) })
	if !ok {
		a.fail(key, now)
		log.Warn("auth", "wrong password", log.F("account", account), log.F("remote", remote))
		return nil, ErrAccountOrPassword
	}
	if acc.Frozen {
		return nil, ErrFrozen
	}
	if acc.ExpiresAtMs != 0 && time.UnixMilli(acc.ExpiresAtMs).Sub(now) <= a.opt.MinGameTime {
		return nil, ErrNoGameTime
	}
	a.clear(key)
	acc.LastLoginMs, acc.LastAddr, acc.Logins = now.UnixMilli(), remote, acc.Logins+1
	if err := a.store.UpdateAccount(ctx, acc); err != nil {
		log.Error("auth", "cannot record login", log.F("account", account), log.F("error", err))
	}
	return acc, nil
}

func (a *S3PAccount) locked(key string, now time.Time) bool {
	a.mu.Lock()
	defer a.mu.Unlock()
	f := a.fails[key]
	if f == nil {
		return false
	}
	if now.Before(f.lockedUntil) {
		return true
	}
	if now.Sub(f.last) > a.opt.LockFor { // old failures are forgotten
		delete(a.fails, key)
	}
	return false
}

func (a *S3PAccount) fail(key string, now time.Time) {
	a.mu.Lock()
	defer a.mu.Unlock()
	if len(a.fails) > 100000 { // never grow without bound under a name flood
		a.fails = map[string]*failState{}
	}
	f := a.fails[key]
	if f == nil || now.Sub(f.last) > a.opt.LockFor {
		f = &failState{}
		a.fails[key] = f
	}
	f.count++
	f.last = now
	if f.count >= a.opt.MaxFails {
		f.lockedUntil = now.Add(a.opt.LockFor)
		f.count = 0
		log.Warn("auth", "account locked after failed logins", log.F("account", key), log.F("lock_s", a.opt.LockFor.Seconds()))
	}
}

func (a *S3PAccount) clear(key string) {
	a.mu.Lock()
	delete(a.fails, key)
	a.mu.Unlock()
}
