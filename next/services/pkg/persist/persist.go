// Package persist defines the storage interface used by the gateway (accounts and characters)
// and a file backed implementation for development.  A PostgreSQL implementation (phase 3)
// implements the same Store interface.
package persist

import (
	"context"
	"errors"
	"strings"
	"unicode"
	"unicode/utf8"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
)

var (
	ErrNotFound    = errors.New("persist: not found")
	ErrExists      = errors.New("persist: already exists")
	ErrInvalidName = errors.New("persist: invalid name")
	ErrForbidden   = errors.New("persist: not owned by account")
)

// Account is a login identity.  Dev auth stores the password in clear text; real auth
// (phase 3) replaces it by a hash and tokens, the interface stays.
type Account struct {
	ID          uint64   `json:"id"`
	Name        string   `json:"name"`
	Password    string   `json:"password"`
	Chars       []uint64 `json:"chars"`
	CreatedAtMs int64    `json:"created_at_ms"`
}

// Store is what the gateway needs from persistence.
type Store interface {
	// EnsureAccount returns the account called name, creating it on first use.
	EnsureAccount(ctx context.Context, name, password string) (*Account, error)
	Account(ctx context.Context, name string) (*Account, error)
	Characters(ctx context.Context, accountID uint64) ([]*jxpb.RoleData, error)
	Character(ctx context.Context, playerID uint64) (*jxpb.RoleData, error)
	CreateCharacter(ctx context.Context, accountID uint64, name string, series, sex uint32) (*jxpb.RoleData, error)
	SaveCharacter(ctx context.Context, role *jxpb.RoleData) error
	Close() error
}

// ValidateName checks a character name: valid UTF-8, 2..16 runes, printable, no leading or
// trailing spaces.  Vietnamese names with diacritics are fine.
func ValidateName(name string) error {
	if !utf8.ValidString(name) {
		return ErrInvalidName
	}
	n := utf8.RuneCountInString(name)
	if n < 2 || n > 16 {
		return ErrInvalidName
	}
	if strings.TrimSpace(name) != name {
		return ErrInvalidName
	}
	for _, r := range name {
		if !unicode.IsPrint(r) || unicode.IsControl(r) {
			return ErrInvalidName
		}
	}
	return nil
}

// ValidateAccountName accepts ASCII letters, digits, '_', '.', '-' (3..32 chars).
func ValidateAccountName(name string) error {
	if len(name) < 3 || len(name) > 32 {
		return ErrInvalidName
	}
	for _, r := range name {
		ok := (r >= 'a' && r <= 'z') || (r >= 'A' && r <= 'Z') || (r >= '0' && r <= '9') || r == '_' || r == '.' || r == '-'
		if !ok {
			return ErrInvalidName
		}
	}
	return nil
}

// NormalizeName is the case-insensitive key used for uniqueness checks.
func NormalizeName(name string) string { return strings.ToLower(strings.TrimSpace(name)) }
