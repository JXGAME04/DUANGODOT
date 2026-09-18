package persist

import (
	"context"
	"errors"
	"fmt"
	"net/url"
	"regexp"
	"strings"
	"time"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgconn"
	"github.com/jackc/pgx/v5/pgxpool"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// PgStore keeps accounts and characters in PostgreSQL (M9 / O1 of the handover, ADR-007 T9).
//
// The file store is for one developer's machine: it reads every character into memory at start
// (11 461 files took 31 seconds) and rewrites the whole account file for a login.  A server with
// tens of thousands of characters needs a login to be one indexed query and a save to be one row
// in a transaction, so a crash between two saves loses nothing that was acknowledged.
//
// Plain SQL through pgx, no ORM (ADR-007).  A character is stored as the protojson of its
// RoleData - the same text the file store writes, readable in psql and queryable as jsonb - with
// its data_version beside it, so an old record is upgraded by MigrateRole when it is read and a
// record from a newer server is refused (ErrNewerData), exactly as the file store does.
//
// Ids come from sequences (accounts.id, characters.player_id) and character names are unique
// without regard to case (name_key), as they were in the files.
type PgStore struct {
	pool *pgxpool.Pool
}

const pgSchemaVersion = 1

// pgSchema is applied at every open; every statement is idempotent.
const pgSchema = `
CREATE TABLE IF NOT EXISTS schema_version (
  version INT NOT NULL
);
CREATE TABLE IF NOT EXISTS accounts (
  id            BIGSERIAL PRIMARY KEY,
  name          TEXT NOT NULL,
  name_key      TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL DEFAULT '',
  password      TEXT NOT NULL DEFAULT '',
  created_at_ms BIGINT NOT NULL,
  frozen        BOOLEAN NOT NULL DEFAULT FALSE,
  frozen_text   TEXT NOT NULL DEFAULT '',
  expires_at_ms BIGINT NOT NULL DEFAULT 0,
  last_login_ms BIGINT NOT NULL DEFAULT 0,
  last_addr     TEXT NOT NULL DEFAULT '',
  logins        BIGINT NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS characters (
  player_id    BIGSERIAL PRIMARY KEY,
  account_id   BIGINT NOT NULL REFERENCES accounts(id),
  name         TEXT NOT NULL,
  name_key     TEXT NOT NULL UNIQUE,
  data         JSONB NOT NULL,
  data_version INT NOT NULL,
  updated_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS characters_account ON characters (account_id, player_id);
`

// OpenPgStore connects to dsn (postgres://user:password@host:port/db?sslmode=...), creates the
// tables when they are missing and checks the schema version.
func OpenPgStore(ctx context.Context, dsn string) (*PgStore, error) {
	cfg, err := pgxpool.ParseConfig(dsn)
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	pool, err := pgxpool.NewWithConfig(ctx, cfg)
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	if err := pool.Ping(ctx); err != nil {
		pool.Close()
		return nil, fmt.Errorf("persist: pg: %s: %w", RedactDSN(dsn), err)
	}
	s := &PgStore{pool: pool}
	if err := s.migrate(ctx); err != nil {
		pool.Close()
		return nil, err
	}
	var accounts, chars int64
	_ = pool.QueryRow(ctx, "SELECT (SELECT count(*) FROM accounts), (SELECT count(*) FROM characters)").Scan(&accounts, &chars)
	log.Info("db", "postgres store opened", log.F("dsn", RedactDSN(dsn)), log.F("accounts", accounts), log.F("chars", chars),
		log.F("schema", pgSchemaVersion), log.F("role_version", CurrentRoleVersion))
	return s, nil
}

func (s *PgStore) migrate(ctx context.Context) error {
	if _, err := s.pool.Exec(ctx, pgSchema); err != nil {
		return fmt.Errorf("persist: pg schema: %w", err)
	}
	var version int
	err := s.pool.QueryRow(ctx, "SELECT version FROM schema_version LIMIT 1").Scan(&version)
	switch {
	case errors.Is(err, pgx.ErrNoRows):
		if _, err := s.pool.Exec(ctx, "INSERT INTO schema_version (version) VALUES ($1)", pgSchemaVersion); err != nil {
			return fmt.Errorf("persist: pg schema: %w", err)
		}
	case err != nil:
		return fmt.Errorf("persist: pg schema: %w", err)
	case version > pgSchemaVersion:
		return fmt.Errorf("persist: pg schema version %d is newer than this server knows (%d)", version, pgSchemaVersion)
	}
	return nil
}

var dsnPassword = regexp.MustCompile(`(?i)(password=)\S+`)

// RedactDSN is the connection string without its password: what may go into a log line.  Both
// forms pgx accepts are covered: a URL and the key=value list.
func RedactDSN(dsn string) string {
	if strings.Contains(dsn, "://") {
		if u, err := url.Parse(dsn); err == nil && u.User != nil {
			if _, has := u.User.Password(); has {
				u.User = url.UserPassword(u.User.Username(), "***")
			}
			return strings.ReplaceAll(u.String(), "%2A%2A%2A", "***")
		}
	}
	return dsnPassword.ReplaceAllString(dsn, "${1}***")
}

func isUnique(err error) bool {
	var pgErr *pgconn.PgError
	return errors.As(err, &pgErr) && pgErr.Code == "23505"
}

const accountColumns = `a.id, a.name, a.password_hash, a.password, a.created_at_ms, a.frozen, a.frozen_text,
  a.expires_at_ms, a.last_login_ms, a.last_addr, a.logins,
  COALESCE((SELECT array_agg(c.player_id ORDER BY c.player_id) FROM characters c WHERE c.account_id = a.id), '{}')`

func scanAccount(row pgx.Row) (*Account, error) {
	var a Account
	var id, created, expires, lastLogin, logins int64
	var chars []int64
	if err := row.Scan(&id, &a.Name, &a.PasswordHash, &a.Password, &created, &a.Frozen, &a.FrozenText,
		&expires, &lastLogin, &a.LastAddr, &logins, &chars); err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return nil, ErrNotFound
		}
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	a.ID, a.CreatedAtMs, a.ExpiresAtMs, a.LastLoginMs, a.Logins = uint64(id), created, expires, lastLogin, uint64(logins)
	a.Chars = make([]uint64, len(chars))
	for i, c := range chars {
		a.Chars[i] = uint64(c)
	}
	return &a, nil
}

// CreateAccount implements Store.
func (s *PgStore) CreateAccount(ctx context.Context, name, passwordHash string) (*Account, error) {
	if err := ValidateAccountName(name); err != nil {
		return nil, err
	}
	a := &Account{Name: name, PasswordHash: passwordHash, CreatedAtMs: time.Now().UnixMilli(), Chars: []uint64{}}
	var id int64
	err := s.pool.QueryRow(ctx,
		"INSERT INTO accounts (name, name_key, password_hash, created_at_ms) VALUES ($1, $2, $3, $4) RETURNING id",
		name, NormalizeName(name), passwordHash, a.CreatedAtMs).Scan(&id)
	if err != nil {
		if isUnique(err) {
			return nil, ErrExists
		}
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	a.ID = uint64(id)
	log.Info("db", "account created", log.F("account", name), log.F("account_id", a.ID))
	return a, nil
}

// Account implements Store.
func (s *PgStore) Account(ctx context.Context, name string) (*Account, error) {
	return scanAccount(s.pool.QueryRow(ctx, "SELECT "+accountColumns+" FROM accounts a WHERE a.name_key = $1", NormalizeName(name)))
}

// AccountByID implements Store.
func (s *PgStore) AccountByID(ctx context.Context, id uint64) (*Account, error) {
	return scanAccount(s.pool.QueryRow(ctx, "SELECT "+accountColumns+" FROM accounts a WHERE a.id = $1", int64(id)))
}

// UpdateAccount implements Store.
func (s *PgStore) UpdateAccount(ctx context.Context, acc *Account) error {
	tag, err := s.pool.Exec(ctx, `UPDATE accounts SET password_hash = $2, password = $3, frozen = $4, frozen_text = $5,
  expires_at_ms = $6, last_login_ms = $7, last_addr = $8, logins = $9 WHERE id = $1`,
		int64(acc.ID), acc.PasswordHash, acc.Password, acc.Frozen, acc.FrozenText, acc.ExpiresAtMs, acc.LastLoginMs, acc.LastAddr, int64(acc.Logins))
	if err != nil {
		return fmt.Errorf("persist: pg: %w", err)
	}
	if tag.RowsAffected() == 0 {
		return ErrNotFound
	}
	return nil
}

// Accounts implements Store (sorted by id).
func (s *PgStore) Accounts(ctx context.Context) ([]*Account, error) {
	rows, err := s.pool.Query(ctx, "SELECT "+accountColumns+" FROM accounts a ORDER BY a.id")
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	defer rows.Close()
	var out []*Account
	for rows.Next() {
		a, err := scanAccount(rows)
		if err != nil {
			return nil, err
		}
		out = append(out, a)
	}
	return out, rows.Err()
}

func decodeRole(raw []byte) (*jxpb.RoleData, error) {
	role := &jxpb.RoleData{}
	if err := protojson.Unmarshal(raw, role); err != nil {
		return nil, fmt.Errorf("persist: pg: character json: %w", err)
	}
	// an old record is upgraded as it is read (the file store writes it back at open; here it
	// is written back by the next save, which every character gets within a minute of playing)
	if _, err := MigrateRole(role); err != nil {
		return nil, err
	}
	return role, nil
}

func encodeRole(role *jxpb.RoleData) ([]byte, error) {
	return protojson.MarshalOptions{}.Marshal(role)
}

// Characters implements Store.
func (s *PgStore) Characters(ctx context.Context, accountID uint64) ([]*jxpb.RoleData, error) {
	var exists bool
	if err := s.pool.QueryRow(ctx, "SELECT EXISTS (SELECT 1 FROM accounts WHERE id = $1)", int64(accountID)).Scan(&exists); err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	if !exists {
		return nil, ErrNotFound
	}
	rows, err := s.pool.Query(ctx, "SELECT data FROM characters WHERE account_id = $1 ORDER BY player_id", int64(accountID))
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	defer rows.Close()
	out := []*jxpb.RoleData{}
	for rows.Next() {
		var raw []byte
		if err := rows.Scan(&raw); err != nil {
			return nil, fmt.Errorf("persist: pg: %w", err)
		}
		role, err := decodeRole(raw)
		if err != nil {
			return nil, err
		}
		out = append(out, role)
	}
	return out, rows.Err()
}

// Character implements Store.
func (s *PgStore) Character(ctx context.Context, playerID uint64) (*jxpb.RoleData, error) {
	var raw []byte
	err := s.pool.QueryRow(ctx, "SELECT data FROM characters WHERE player_id = $1", int64(playerID)).Scan(&raw)
	if errors.Is(err, pgx.ErrNoRows) {
		return nil, ErrNotFound
	}
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	return decodeRole(raw)
}

// CreateCharacter implements Store: one transaction - the account must exist, the id comes from
// the sequence, the name must be free.
func (s *PgStore) CreateCharacter(ctx context.Context, accountID uint64, c NewCharacter) (*jxpb.RoleData, error) {
	if err := c.Validate(); err != nil {
		return nil, err
	}
	tx, err := s.pool.Begin(ctx)
	if err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	defer tx.Rollback(ctx) //nolint:errcheck // a committed tx ignores the rollback
	var exists bool
	if err := tx.QueryRow(ctx, "SELECT EXISTS (SELECT 1 FROM accounts WHERE id = $1)", int64(accountID)).Scan(&exists); err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	if !exists {
		return nil, ErrNotFound
	}
	var pid int64
	if err := tx.QueryRow(ctx, "SELECT nextval('characters_player_id_seq')").Scan(&pid); err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	role := NewRole(uint64(pid), accountID, c.Name, c.Series, c.Sex, c.NativePlace)
	raw, err := encodeRole(role)
	if err != nil {
		return nil, err
	}
	_, err = tx.Exec(ctx, "INSERT INTO characters (player_id, account_id, name, name_key, data, data_version) VALUES ($1, $2, $3, $4, $5, $6)",
		pid, int64(accountID), c.Name, NormalizeName(c.Name), raw, int32(role.DataVersion))
	if err != nil {
		if isUnique(err) {
			return nil, ErrExists
		}
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	if err := tx.Commit(ctx); err != nil {
		return nil, fmt.Errorf("persist: pg: %w", err)
	}
	log.Info("db", "character created", log.F("account_id", accountID), log.F("pid", role.PlayerId), log.F("name", c.Name),
		log.F("series", c.Series), log.F("sex", c.Sex), log.F("native_place", c.NativePlace))
	return role, nil
}

// SaveCharacter implements Store.
func (s *PgStore) SaveCharacter(ctx context.Context, role *jxpb.RoleData) error {
	if role.DataVersion > CurrentRoleVersion {
		return ErrNewerData
	}
	var owner int64
	err := s.pool.QueryRow(ctx, "SELECT account_id FROM characters WHERE player_id = $1", int64(role.PlayerId)).Scan(&owner)
	if errors.Is(err, pgx.ErrNoRows) {
		return ErrNotFound
	}
	if err != nil {
		return fmt.Errorf("persist: pg: %w", err)
	}
	if uint64(owner) != role.AccountId {
		return ErrForbidden
	}
	copyRole := proto.Clone(role).(*jxpb.RoleData)
	if _, err := MigrateRole(copyRole); err != nil { // a round trip through an old zone never downgrades
		return err
	}
	raw, err := encodeRole(copyRole)
	if err != nil {
		return err
	}
	_, err = s.pool.Exec(ctx, "UPDATE characters SET data = $2, data_version = $3, updated_at = now() WHERE player_id = $1 AND account_id = $4",
		int64(role.PlayerId), raw, int32(copyRole.DataVersion), owner)
	if err != nil {
		return fmt.Errorf("persist: pg: %w", err)
	}
	return nil
}

// Close implements Store.
func (s *PgStore) Close() error {
	s.pool.Close()
	return nil
}

// Reset drops every row - for tests only.
func (s *PgStore) Reset(ctx context.Context) error {
	_, err := s.pool.Exec(ctx, "TRUNCATE characters, accounts RESTART IDENTITY")
	return err
}
