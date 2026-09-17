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

// Account is a login identity: the Account_info row of the old Sword3PaySys database
// (cAccName, cPassword, iClientID, dLoginDate, deposit) without the billing part.
type Account struct {
	ID           uint64   `json:"id"`
	Name         string   `json:"name"`
	PasswordHash string   `json:"password_hash,omitempty"` // argon2id, made by auth.HashPassword
	Password     string   `json:"password,omitempty"`      // clear text of the first dev builds; auth hashes and clears it on start
	Chars        []uint64 `json:"chars"`
	CreatedAtMs  int64    `json:"created_at_ms"`
	Frozen       bool     `json:"frozen,omitempty"`        // E_ACCOUNT_FREEZE: a GM locked the account
	FrozenText   string   `json:"frozen_text,omitempty"`   // why (shown to the player)
	ExpiresAtMs  int64    `json:"expires_at_ms,omitempty"` // game time left (E_ACCOUNT_NODEPOSIT); 0 = unlimited
	LastLoginMs  int64    `json:"last_login_ms,omitempty"`
	LastAddr     string   `json:"last_addr,omitempty"`
	Logins       uint64   `json:"logins,omitempty"`
}

// Store is what the gateway needs from persistence.
type Store interface {
	// CreateAccount registers name with an already hashed password (ErrExists when taken).
	CreateAccount(ctx context.Context, name, passwordHash string) (*Account, error)
	Account(ctx context.Context, name string) (*Account, error)
	AccountByID(ctx context.Context, id uint64) (*Account, error)
	// UpdateAccount stores the mutable fields of acc (password hash, frozen, game time, last
	// login); the id, name and character list never change through it.
	UpdateAccount(ctx context.Context, acc *Account) error
	Accounts(ctx context.Context) ([]*Account, error)
	Characters(ctx context.Context, accountID uint64) ([]*jxpb.RoleData, error)
	Character(ctx context.Context, playerID uint64) (*jxpb.RoleData, error)
	CreateCharacter(ctx context.Context, accountID uint64, c NewCharacter) (*jxpb.RoleData, error)
	SaveCharacter(ctx context.Context, role *jxpb.RoleData) error
	Close() error
}

// NewCharacter is what a player chooses in the create window (KUiNewPlayer, after
// KUiSelNativePlace): KRoleChiefInfo of the old game.
type NewCharacter struct {
	Name        string
	Series      uint32 // 0 Kim, 1 Mộc, 2 Thủy, 3 Hỏa, 4 Thổ
	Sex         uint32 // 0 nam, 1 nữ
	NativePlace uint32 // map id of the starting village (Settings/NativePlaceList.ini), 0 = not chosen
}

// ErrInvalidChoice: an element or a sex the game does not have, or a pair it does not allow.
var ErrInvalidChoice = errors.New("persist: invalid character choice")

const (
	seriesMetal = 0
	seriesWater = 2
	seriesCount = 5
)

// Validate checks the choice the way KUiNewPlayer::UpdateProperty made it: five elements, two
// sexes, Kim only for men and Thủy only for women.  The window enforces this too, but a window is
// not a rule - a rewritten client must not be able to make a character the game never had.
func (c NewCharacter) Validate() error {
	if err := ValidateName(c.Name); err != nil {
		return err
	}
	if c.Series >= seriesCount || c.Sex > 1 {
		return ErrInvalidChoice
	}
	if (c.Series == seriesMetal && c.Sex != 0) || (c.Series == seriesWater && c.Sex != 1) {
		return ErrInvalidChoice
	}
	return nil
}

// ValidateName checks a character name: valid UTF-8, 2..16 runes, printable, and no blank anywhere
// (KUiNewPlayer::GetInputInfo refused every character up to 0x20 - sentence 17 of the login flow).
// A name with a space reads like two words in chat, breaks "/command name" and lets "Admin " pass
// for "Admin".  Vietnamese names with diacritics are fine.
func ValidateName(name string) error {
	if !utf8.ValidString(name) {
		return ErrInvalidName
	}
	n := utf8.RuneCountInString(name)
	if n < 2 || n > 16 {
		return ErrInvalidName
	}
	for _, r := range name {
		if !unicode.IsPrint(r) || unicode.IsControl(r) || unicode.IsSpace(r) {
			return ErrInvalidName
		}
	}
	return nil
}

// ValidateAccountName accepts ASCII letters, digits, '_', '.', '-' (3..32 chars).  The old
// PaySys allowed 4..30 characters; three lets the dev accounts of the tests through.
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
