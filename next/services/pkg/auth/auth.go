// Package auth decides who may log in.  It is the account server of the old game
// (Sword3PaySys S3AccServer, S3PAccount::Login) reduced to what the gateway needs: password
// check, frozen accounts, game time, and a lock after too many wrong passwords.  The
// outcomes keep the meaning of the old LOGIN_R_* codes (Bishop LoginDef.h) so the client can
// show the same messages.
package auth

import (
	"context"
	"errors"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

var (
	ErrAccountOrPassword = errors.New("auth: account or password wrong") // E_ACCOUNT_OR_PASSWORD -> LOGIN_R_ACCOUNT_OR_PASSWORD_ERROR
	ErrFrozen            = errors.New("auth: account frozen")            // E_ACCOUNT_FREEZE -> LOGIN_R_FREEZE
	ErrNoGameTime        = errors.New("auth: no game time left")         // E_ACCOUNT_NODEPOSIT -> LOGIN_R_TIMEOUT
	ErrBusy              = errors.New("auth: too many failed logins")    // ACTION_FAILED -> LOGIN_R_FAILED
	ErrWeakPassword      = errors.New("auth: password too short")        // LOGIN_PASSWORD_MIN_LEN
)

// Authenticator is what the gateway calls for every LoginReq.
type Authenticator interface {
	// Login verifies account/password for a client at remote (ip:port, for the log and the
	// last-login record) and returns the account or one of the Err* values above.
	Login(ctx context.Context, account, password, remote string) (*persist.Account, error)
	// Mode is "dev" (first login registers the account) or "strict" (registered accounts only);
	// the client shows it on the login screen.
	Mode() string
}

// Options tune the account server.
type Options struct {
	AutoRegister bool          // dev: the first login of a new name creates the account with that password
	MinPassword  int           // LOGIN_PASSWORD_MIN_LEN: 6 in the old PaySys; dev builds use 1 (0 = 6)
	MaxFails     int           // wrong passwords per account before it is locked (0 = 5)
	LockFor      time.Duration // how long the account stays locked (0 = 60 s)
	MinGameTime  time.Duration // an account with less game time left than this cannot log in (old: 1800 s); applies when ExpiresAtMs != 0
	Now          func() time.Time
}

func (o *Options) defaults() {
	if o.MinPassword <= 0 {
		o.MinPassword = 6
	}
	if o.MaxFails <= 0 {
		o.MaxFails = 5
	}
	if o.LockFor <= 0 {
		o.LockFor = 60 * time.Second
	}
	if o.MinGameTime <= 0 {
		o.MinGameTime = 30 * time.Minute
	}
	if o.Now == nil {
		o.Now = time.Now
	}
}

// DevOptions is the development mode: any new account name registers itself on first login,
// short passwords are fine.
func DevOptions() Options { return Options{AutoRegister: true, MinPassword: 1} }

// StrictOptions is the production mode: accounts come from jxaccount (or a future web
// registration), passwords follow the old minimum length.
func StrictOptions() Options { return Options{MinPassword: 6} }
