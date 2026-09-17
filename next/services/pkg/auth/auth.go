// Package auth decides who may log in.  Dev is the development authenticator: the first login
// of an account name registers it with the given password, later logins must repeat it.
// Production auth (tokens, external accounts) implements the same interface (phase 3).
package auth

import (
	"context"
	"errors"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

var ErrUnauthorized = errors.New("auth: unauthorized")

type Authenticator interface {
	Login(ctx context.Context, account, password string) (*persist.Account, error)
}

type Dev struct {
	Store persist.Store
}

func (d *Dev) Login(ctx context.Context, account, password string) (*persist.Account, error) {
	if err := persist.ValidateAccountName(account); err != nil {
		return nil, ErrUnauthorized
	}
	acc, err := d.Store.EnsureAccount(ctx, account, password)
	if err != nil {
		return nil, err
	}
	if acc.Password != password {
		return nil, ErrUnauthorized
	}
	return acc, nil
}
