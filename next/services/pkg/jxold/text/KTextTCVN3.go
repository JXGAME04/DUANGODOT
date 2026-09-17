// Package text converts the legacy single-byte encodings of the old data files to UTF-8.
// Vietnamese strings are TCVN3 (ABC) - one byte per letter, only lowercase diacritics plus the
// seven capital bases; paths and some tables are GBK (handled by golang.org/x/text).
package text

import (
	"strings"
	"unicode/utf8"

	"golang.org/x/text/encoding/simplifiedchinese"
)

// tcvn3 maps bytes 0x80..0xff to runes (0 = unassigned).  Verified against the shipped
// binaries of this project (see the swordonline-dev skill, vn_to_octal.py).
var tcvn3 = [128]rune{
	0xa1 - 0x80: 'Ă', 0xa2 - 0x80: 'Â', 0xa3 - 0x80: 'Ê', 0xa4 - 0x80: 'Ô', 0xa5 - 0x80: 'Ơ', 0xa6 - 0x80: 'Ư', 0xa7 - 0x80: 'Đ',
	0xa8 - 0x80: 'ă', 0xa9 - 0x80: 'â', 0xaa - 0x80: 'ê', 0xab - 0x80: 'ô', 0xac - 0x80: 'ơ', 0xad - 0x80: 'ư', 0xae - 0x80: 'đ',
	0xb5 - 0x80: 'à', 0xb6 - 0x80: 'ả', 0xb7 - 0x80: 'ã', 0xb8 - 0x80: 'á', 0xb9 - 0x80: 'ạ',
	0xbb - 0x80: 'ằ', 0xbc - 0x80: 'ẳ', 0xbd - 0x80: 'ẵ', 0xbe - 0x80: 'ắ', 0xc6 - 0x80: 'ặ',
	0xc7 - 0x80: 'ầ', 0xc8 - 0x80: 'ẩ', 0xc9 - 0x80: 'ẫ', 0xca - 0x80: 'ấ', 0xcb - 0x80: 'ậ',
	0xcc - 0x80: 'è', 0xce - 0x80: 'ẻ', 0xcf - 0x80: 'ẽ', 0xd0 - 0x80: 'é', 0xd1 - 0x80: 'ẹ',
	0xd2 - 0x80: 'ề', 0xd3 - 0x80: 'ể', 0xd4 - 0x80: 'ễ', 0xd5 - 0x80: 'ế', 0xd6 - 0x80: 'ệ',
	0xd7 - 0x80: 'ì', 0xd8 - 0x80: 'ỉ', 0xdc - 0x80: 'ĩ', 0xdd - 0x80: 'í', 0xde - 0x80: 'ị',
	0xdf - 0x80: 'ò', 0xe1 - 0x80: 'ỏ', 0xe2 - 0x80: 'õ', 0xe3 - 0x80: 'ó', 0xe4 - 0x80: 'ọ',
	0xe5 - 0x80: 'ồ', 0xe6 - 0x80: 'ổ', 0xe7 - 0x80: 'ỗ', 0xe8 - 0x80: 'ố', 0xe9 - 0x80: 'ộ',
	0xea - 0x80: 'ờ', 0xeb - 0x80: 'ở', 0xec - 0x80: 'ỡ', 0xed - 0x80: 'ớ', 0xee - 0x80: 'ợ',
	0xef - 0x80: 'ù', 0xf1 - 0x80: 'ủ', 0xf2 - 0x80: 'ũ', 0xf3 - 0x80: 'ú', 0xf4 - 0x80: 'ụ',
	0xf5 - 0x80: 'ừ', 0xf6 - 0x80: 'ử', 0xf7 - 0x80: 'ữ', 0xf8 - 0x80: 'ứ', 0xf9 - 0x80: 'ự',
	0xfa - 0x80: 'ỳ', 0xfb - 0x80: 'ỷ', 0xfc - 0x80: 'ỹ', 0xfd - 0x80: 'ý', 0xfe - 0x80: 'ỵ',
}

var utf8ToTCVN3 = func() map[rune]byte {
	m := make(map[rune]byte, 80)
	for i, r := range tcvn3 {
		if r != 0 {
			m[r] = byte(i + 0x80)
		}
	}
	return m
}()

// TCVN3Rune returns the letter one TCVN3 byte stands for, 0 when the byte has none.  The old
// bitmap fonts are indexed by this byte, so this is also the way from a glyph slot to Unicode.
func TCVN3Rune(b byte) rune {
	if b < 0x80 {
		return rune(b)
	}
	return tcvn3[b-0x80]
}

// TCVN3ToUTF8 decodes a TCVN3 string.  Bytes without a mapping are kept as U+FFFD.
func TCVN3ToUTF8(b []byte) string {
	var sb strings.Builder
	sb.Grow(len(b) * 2)
	for _, c := range b {
		if c < 0x80 {
			sb.WriteByte(c)
			continue
		}
		if r := tcvn3[c-0x80]; r != 0 {
			sb.WriteRune(r)
		} else {
			sb.WriteRune(utf8.RuneError)
		}
	}
	return sb.String()
}

// UTF8ToTCVN3 encodes; ok is false when a rune has no TCVN3 form (e.g. uppercase Ế).
func UTF8ToTCVN3(s string) (out []byte, ok bool) {
	ok = true
	out = make([]byte, 0, len(s))
	for _, r := range s {
		if r < 0x80 {
			out = append(out, byte(r))
			continue
		}
		if b, found := utf8ToTCVN3[r]; found {
			out = append(out, b)
		} else {
			ok = false
			out = append(out, '?')
		}
	}
	return out, ok
}

// IsTCVN3 reports whether every high byte of b has a TCVN3 mapping and no run of high bytes is
// longer than 3 (Vietnamese never exceeds three accented letters in a row; GBK pairs do).
func IsTCVN3(b []byte) bool {
	run := 0
	for _, c := range b {
		if c < 0x80 {
			run = 0
			continue
		}
		if tcvn3[c-0x80] == 0 {
			return false
		}
		run++
		if run > 3 {
			return false
		}
	}
	return true
}

// GBKToUTF8 decodes a GBK string (map folder names, Chinese comments).
func GBKToUTF8(b []byte) string {
	out, err := simplifiedchinese.GBK.NewDecoder().Bytes(b)
	if err != nil {
		return string(b)
	}
	return string(out)
}

// UTF8ToGBK encodes for hashing archive paths.
func UTF8ToGBK(s string) ([]byte, error) {
	return simplifiedchinese.GBK.NewEncoder().Bytes([]byte(s))
}
