package auth

import (
	"crypto/rand"
	"crypto/subtle"
	"encoding/base64"
	"errors"
	"fmt"
	"strings"

	"golang.org/x/crypto/argon2"
)

// Password hashing: argon2id in the PHC string format
//
//	$argon2id$v=19$m=19456,t=2,p=1$<salt>$<hash>
//
// (OWASP 2024 parameters: 19 MiB, 2 passes, 1 lane).  The old game stored the MD5 of the
// password (KSG_PASSWORD) which is not acceptable today; there is no compatibility to keep
// because accounts are not migrated from the old database.
const (
	argonTime    = 2
	argonMemory  = 19 * 1024 // KiB
	argonThreads = 1
	argonKeyLen  = 32
	saltLen      = 16
)

var errBadHash = errors.New("auth: malformed password hash")

// HashPassword returns the encoded argon2id hash of password with a fresh random salt.
func HashPassword(password string) (string, error) {
	salt := make([]byte, saltLen)
	if _, err := rand.Read(salt); err != nil {
		return "", err
	}
	return encodeHash(salt, argon2.IDKey([]byte(password), salt, argonTime, argonMemory, argonThreads, argonKeyLen), argonTime, argonMemory, argonThreads), nil
}

func encodeHash(salt, key []byte, t, m uint32, p uint8) string {
	b64 := base64.RawStdEncoding
	return fmt.Sprintf("$argon2id$v=19$m=%d,t=%d,p=%d$%s$%s", m, t, p, b64.EncodeToString(salt), b64.EncodeToString(key))
}

func decodeHash(encoded string) (salt, key []byte, t, m uint32, p uint8, err error) {
	parts := strings.Split(encoded, "$")
	if len(parts) != 6 || parts[0] != "" || parts[1] != "argon2id" || parts[2] != "v=19" {
		return nil, nil, 0, 0, 0, errBadHash
	}
	var pp uint32
	if _, err := fmt.Sscanf(parts[3], "m=%d,t=%d,p=%d", &m, &t, &pp); err != nil || m == 0 || t == 0 || pp == 0 || pp > 255 {
		return nil, nil, 0, 0, 0, errBadHash
	}
	b64 := base64.RawStdEncoding
	if salt, err = b64.DecodeString(parts[4]); err != nil || len(salt) == 0 {
		return nil, nil, 0, 0, 0, errBadHash
	}
	if key, err = b64.DecodeString(parts[5]); err != nil || len(key) == 0 {
		return nil, nil, 0, 0, 0, errBadHash
	}
	return salt, key, t, m, uint8(pp), nil
}

// VerifyPassword reports whether password matches the encoded hash (constant time on the
// hash comparison; a malformed hash never matches).
func VerifyPassword(encoded, password string) bool {
	salt, key, t, m, p, err := decodeHash(encoded)
	if err != nil {
		return false
	}
	got := argon2.IDKey([]byte(password), salt, t, m, p, uint32(len(key)))
	return subtle.ConstantTimeCompare(got, key) == 1
}
