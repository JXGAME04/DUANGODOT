package persist

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"sync"
	"time"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// FileStore keeps accounts in <dir>/accounts.json and every character in
// <dir>/chars/<player_id>.json (protojson, readable and diff-able).  Writes are atomic
// (temp file + rename) and write-through, so a crash never loses an acknowledged save.
type FileStore struct {
	dir string
	mu  sync.Mutex
	db  fileDB
	// player id -> role (cache of chars/*.json)
	chars map[uint64]*jxpb.RoleData
	names map[string]uint64 // normalized character name -> player id
}

type fileDB struct {
	NextAccount uint64              `json:"next_account"`
	NextPlayer  uint64              `json:"next_player"`
	Accounts    map[string]*Account `json:"accounts"` // key: normalized account name
}

// OpenFileStore loads or creates the store at dir.
func OpenFileStore(dir string) (*FileStore, error) {
	s := &FileStore{dir: dir, chars: map[uint64]*jxpb.RoleData{}, names: map[string]uint64{}}
	if err := os.MkdirAll(filepath.Join(dir, "chars"), 0o755); err != nil {
		return nil, err
	}
	s.db = fileDB{NextAccount: 1, NextPlayer: 1, Accounts: map[string]*Account{}}
	data, err := os.ReadFile(s.accountsPath())
	if err == nil {
		if err := json.Unmarshal(data, &s.db); err != nil {
			return nil, fmt.Errorf("persist: %s: %w", s.accountsPath(), err)
		}
		if s.db.Accounts == nil {
			s.db.Accounts = map[string]*Account{}
		}
	} else if !os.IsNotExist(err) {
		return nil, err
	}
	entries, err := os.ReadDir(filepath.Join(dir, "chars"))
	if err != nil {
		return nil, err
	}
	migrated := 0
	for _, e := range entries {
		if e.IsDir() || filepath.Ext(e.Name()) != ".json" {
			continue
		}
		raw, err := os.ReadFile(filepath.Join(dir, "chars", e.Name()))
		if err != nil {
			return nil, err
		}
		role := &jxpb.RoleData{}
		if err := protojson.Unmarshal(raw, role); err != nil {
			return nil, fmt.Errorf("persist: %s: %w", e.Name(), err)
		}
		// an old record is upgraded once, here, and written back; a newer one stops the server
		// instead of being loaded half understood
		changed, err := MigrateRole(role)
		if err != nil {
			return nil, fmt.Errorf("persist: %s: %w", e.Name(), err)
		}
		if changed {
			if err := s.saveCharLocked(role); err != nil {
				return nil, err
			}
			migrated++
		}
		s.chars[role.PlayerId] = role
		s.names[NormalizeName(role.Name)] = role.PlayerId
		if role.PlayerId >= s.db.NextPlayer {
			s.db.NextPlayer = role.PlayerId + 1
		}
	}
	log.Info("db", "file store opened", log.F("dir", dir), log.F("accounts", len(s.db.Accounts)), log.F("chars", len(s.chars)),
		log.F("role_version", CurrentRoleVersion), log.F("migrated", migrated))
	return s, nil
}

func (s *FileStore) accountsPath() string { return filepath.Join(s.dir, "accounts.json") }
func (s *FileStore) charPath(pid uint64) string {
	return filepath.Join(s.dir, "chars", strconv.FormatUint(pid, 10)+".json")
}

func writeAtomic(path string, data []byte) error {
	tmp := path + ".tmp"
	if err := os.WriteFile(tmp, data, 0o644); err != nil {
		return err
	}
	return os.Rename(tmp, path)
}

func (s *FileStore) saveAccountsLocked() error {
	data, err := json.MarshalIndent(&s.db, "", "  ")
	if err != nil {
		return err
	}
	return writeAtomic(s.accountsPath(), data)
}

func (s *FileStore) saveCharLocked(role *jxpb.RoleData) error {
	data, err := protojson.MarshalOptions{Multiline: true, Indent: "  "}.Marshal(role)
	if err != nil {
		return err
	}
	return writeAtomic(s.charPath(role.PlayerId), data)
}

func cloneAccount(a *Account) *Account {
	c := *a
	c.Chars = append([]uint64(nil), a.Chars...)
	return &c
}

// CreateAccount implements Store.
func (s *FileStore) CreateAccount(_ context.Context, name, passwordHash string) (*Account, error) {
	if err := ValidateAccountName(name); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	key := NormalizeName(name)
	if _, ok := s.db.Accounts[key]; ok {
		return nil, ErrExists
	}
	a := &Account{ID: s.db.NextAccount, Name: name, PasswordHash: passwordHash, CreatedAtMs: time.Now().UnixMilli()}
	s.db.NextAccount++
	s.db.Accounts[key] = a
	if err := s.saveAccountsLocked(); err != nil {
		delete(s.db.Accounts, key)
		return nil, err
	}
	log.Info("db", "account created", log.F("account", name), log.F("account_id", a.ID))
	return cloneAccount(a), nil
}

// Account implements Store.
func (s *FileStore) Account(_ context.Context, name string) (*Account, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	a, ok := s.db.Accounts[NormalizeName(name)]
	if !ok {
		return nil, ErrNotFound
	}
	return cloneAccount(a), nil
}

// AccountByID implements Store.
func (s *FileStore) AccountByID(_ context.Context, id uint64) (*Account, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	a := s.accountByIDLocked(id)
	if a == nil {
		return nil, ErrNotFound
	}
	return cloneAccount(a), nil
}

// UpdateAccount implements Store.
func (s *FileStore) UpdateAccount(_ context.Context, acc *Account) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	a := s.accountByIDLocked(acc.ID)
	if a == nil {
		return ErrNotFound
	}
	saved := *a
	a.PasswordHash, a.Password = acc.PasswordHash, acc.Password
	a.Frozen, a.FrozenText = acc.Frozen, acc.FrozenText
	a.ExpiresAtMs, a.LastLoginMs, a.LastAddr, a.Logins = acc.ExpiresAtMs, acc.LastLoginMs, acc.LastAddr, acc.Logins
	if err := s.saveAccountsLocked(); err != nil {
		*a = saved
		return err
	}
	return nil
}

// Accounts implements Store (sorted by id).
func (s *FileStore) Accounts(_ context.Context) ([]*Account, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	out := make([]*Account, 0, len(s.db.Accounts))
	for _, a := range s.db.Accounts {
		out = append(out, cloneAccount(a))
	}
	sort.Slice(out, func(i, j int) bool { return out[i].ID < out[j].ID })
	return out, nil
}

func (s *FileStore) accountByIDLocked(id uint64) *Account {
	for _, a := range s.db.Accounts {
		if a.ID == id {
			return a
		}
	}
	return nil
}

// Characters implements Store.
func (s *FileStore) Characters(_ context.Context, accountID uint64) ([]*jxpb.RoleData, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	a := s.accountByIDLocked(accountID)
	if a == nil {
		return nil, ErrNotFound
	}
	out := make([]*jxpb.RoleData, 0, len(a.Chars))
	for _, pid := range a.Chars {
		if r, ok := s.chars[pid]; ok {
			out = append(out, proto.Clone(r).(*jxpb.RoleData))
		}
	}
	return out, nil
}

// Character implements Store.
func (s *FileStore) Character(_ context.Context, playerID uint64) (*jxpb.RoleData, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	r, ok := s.chars[playerID]
	if !ok {
		return nil, ErrNotFound
	}
	return proto.Clone(r).(*jxpb.RoleData), nil
}

// NewRole builds the starting RoleData for a fresh character (the numbers are placeholders
// until the formula tables from the old Core are ported).
func NewRole(playerID, accountID uint64, name string, series, sex uint32) *jxpb.RoleData {
	now := time.Now().UnixMilli()
	return &jxpb.RoleData{
		PlayerId:    playerID,
		AccountId:   accountID,
		Name:        name,
		Level:       1,
		Series:      series,
		Sex:         sex,
		Position:    &jxpb.RolePosition{ZoneId: 0}, // 0 = let the zone choose its spawn point
		Stats:       &jxpb.RoleStats{Hp: 100, HpMax: 100, Mp: 50, MpMax: 50, Stamina: 100, StaminaMax: 100, Strength: 10, Dexterity: 10, Vitality: 10, Energy: 10, MoveSpeed: 200},
		CreatedAtMs: uint64(now),
		DataVersion: CurrentRoleVersion,
	}
}

// CreateCharacter implements Store.
func (s *FileStore) CreateCharacter(_ context.Context, accountID uint64, name string, series, sex uint32) (*jxpb.RoleData, error) {
	if err := ValidateName(name); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	a := s.accountByIDLocked(accountID)
	if a == nil {
		return nil, ErrNotFound
	}
	if _, taken := s.names[NormalizeName(name)]; taken {
		return nil, ErrExists
	}
	role := NewRole(s.db.NextPlayer, accountID, name, series, sex)
	s.db.NextPlayer++
	if err := s.saveCharLocked(role); err != nil {
		return nil, err
	}
	a.Chars = append(a.Chars, role.PlayerId)
	if err := s.saveAccountsLocked(); err != nil {
		return nil, err
	}
	s.chars[role.PlayerId] = role
	s.names[NormalizeName(name)] = role.PlayerId
	log.Info("db", "character created", log.F("account_id", accountID), log.F("pid", role.PlayerId), log.F("name", name))
	return proto.Clone(role).(*jxpb.RoleData), nil
}

// SaveCharacter implements Store.
func (s *FileStore) SaveCharacter(_ context.Context, role *jxpb.RoleData) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	old, ok := s.chars[role.PlayerId]
	if !ok {
		return ErrNotFound
	}
	if old.AccountId != role.AccountId {
		return ErrForbidden
	}
	if role.DataVersion > CurrentRoleVersion {
		return ErrNewerData
	}
	copyRole := proto.Clone(role).(*jxpb.RoleData)
	if _, err := MigrateRole(copyRole); err != nil { // a round trip through an old zone never downgrades
		return err
	}
	if err := s.saveCharLocked(copyRole); err != nil {
		return err
	}
	s.chars[role.PlayerId] = copyRole
	return nil
}

// Close implements Store.
func (s *FileStore) Close() error { return nil }
