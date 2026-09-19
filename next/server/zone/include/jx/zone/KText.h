// KText.h - the bytes of the old scripts and tables as UTF-8 for the new client.
//
// The Vietnamese of the old data is TCVN3 (ABC): one byte per letter, only the lowercase diacritics and seven capital
// bases; the 2.0 client draws those bytes straight through its \font\vn\gbk_fs*.fnt.  Chinese (folder names, comments,
// the scripts the localisation did not touch) is GBK.  A Lua string a script hands to Say / Talk / Msg2Player is one of
// the two, and one line may hold both (a quoted Vietnamese sentence in a Chinese script); the Go exporter's
// text.DecodeMixed (services/pkg/jxold/text/KTextTCVN3.go) cuts a line at '"' and '-' - bytes no GBK pair and no
// TCVN3 letter ever holds - and guesses each piece on its own.  This is the same rule in C++ (docs/LINUX-SERVER.md §20).
#pragma once

#include <string>
#include <string_view>

namespace jx::zone::text {

// the letter a TCVN3 byte stands for (0 when the byte is not a letter); bytes below 0x80 are ASCII
char32_t tcvn3_rune(unsigned char b) noexcept;

// true when every high byte is a TCVN3 letter or one of Word's "smart" punctuation bytes and no run of high bytes is
// longer than three (Vietnamese never stacks more): the loose test of the exporter
bool is_tcvn3_loose(std::string_view b) noexcept;

// GBK -> UTF-8 (code page 936 on Windows; the bytes as they are elsewhere)
std::string gbk_to_utf8(std::string_view b);

// TCVN3 -> UTF-8: a byte without a letter becomes U+FFFD
std::string tcvn3_to_utf8(std::string_view b);

// one line that may hold TCVN3 and GBK at once (DecodeMixed of the exporter); pure ASCII comes back as it is
std::string decode_mixed(std::string_view b);

}   // namespace jx::zone::text
