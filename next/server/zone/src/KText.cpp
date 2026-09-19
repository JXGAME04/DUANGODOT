#include "jx/zone/KText.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace jx::zone::text {

namespace {

// tcvn3 of the exporter: the runes of bytes 0x80..0xff (0 = no letter)
constexpr std::array<char32_t, 128> make_tcvn3()
{
    std::array<char32_t, 128> t{};
    const auto set = [&](unsigned char b, char32_t r) { t[static_cast<std::size_t>(b - 0x80)] = r; };
    set(0xa1, char32_t(0x0102)); set(0xa2, char32_t(0x00C2)); set(0xa3, char32_t(0x00CA)); set(0xa4, char32_t(0x00D4)); set(0xa5, char32_t(0x01A0)); set(0xa6, char32_t(0x01AF)); set(0xa7, char32_t(0x0110));
    set(0xa8, char32_t(0x0103)); set(0xa9, char32_t(0x00E2)); set(0xaa, char32_t(0x00EA)); set(0xab, char32_t(0x00F4)); set(0xac, char32_t(0x01A1)); set(0xad, char32_t(0x01B0)); set(0xae, char32_t(0x0111));
    set(0xb5, char32_t(0x00E0)); set(0xb6, char32_t(0x1EA3)); set(0xb7, char32_t(0x00E3)); set(0xb8, char32_t(0x00E1)); set(0xb9, char32_t(0x1EA1));
    set(0xbb, char32_t(0x1EB1)); set(0xbc, char32_t(0x1EB3)); set(0xbd, char32_t(0x1EB5)); set(0xbe, char32_t(0x1EAF)); set(0xc6, char32_t(0x1EB7));
    set(0xc7, char32_t(0x1EA7)); set(0xc8, char32_t(0x1EA9)); set(0xc9, char32_t(0x1EAB)); set(0xca, char32_t(0x1EA5)); set(0xcb, char32_t(0x1EAD));
    set(0xcc, char32_t(0x00E8)); set(0xce, char32_t(0x1EBB)); set(0xcf, char32_t(0x1EBD)); set(0xd0, char32_t(0x00E9)); set(0xd1, char32_t(0x1EB9));
    set(0xd2, char32_t(0x1EC1)); set(0xd3, char32_t(0x1EC3)); set(0xd4, char32_t(0x1EC5)); set(0xd5, char32_t(0x1EBF)); set(0xd6, char32_t(0x1EC7));
    set(0xd7, char32_t(0x00EC)); set(0xd8, char32_t(0x1EC9)); set(0xdc, char32_t(0x0129)); set(0xdd, char32_t(0x00ED)); set(0xde, char32_t(0x1ECB));
    set(0xdf, char32_t(0x00F2)); set(0xe1, char32_t(0x1ECF)); set(0xe2, char32_t(0x00F5)); set(0xe3, char32_t(0x00F3)); set(0xe4, char32_t(0x1ECD));
    set(0xe5, char32_t(0x1ED3)); set(0xe6, char32_t(0x1ED5)); set(0xe7, char32_t(0x1ED7)); set(0xe8, char32_t(0x1ED1)); set(0xe9, char32_t(0x1ED9));
    set(0xea, char32_t(0x1EDD)); set(0xeb, char32_t(0x1EDF)); set(0xec, char32_t(0x1EE1)); set(0xed, char32_t(0x1EDB)); set(0xee, char32_t(0x1EE3));
    set(0xef, char32_t(0x00F9)); set(0xf1, char32_t(0x1EE7)); set(0xf2, char32_t(0x0169)); set(0xf3, char32_t(0x00FA)); set(0xf4, char32_t(0x1EE5));
    set(0xf5, char32_t(0x1EEB)); set(0xf6, char32_t(0x1EED)); set(0xf7, char32_t(0x1EEF)); set(0xf8, char32_t(0x1EE9)); set(0xf9, char32_t(0x1EF1));
    set(0xfa, char32_t(0x1EF3)); set(0xfb, char32_t(0x1EF7)); set(0xfc, char32_t(0x1EF9)); set(0xfd, char32_t(0x00FD)); set(0xfe, char32_t(0x1EF5));
    return t;
}

constexpr std::array<char32_t, 128> kTcvn3 = make_tcvn3();

// cp1252Punct of the exporter: the "smart" punctuation Word leaves in a TCVN3 file
char32_t cp1252_punct(unsigned char b) noexcept
{
    switch (b) {
    case 0x82: return char32_t(0x201A);
    case 0x84: return char32_t(0x201E);
    case 0x85: return char32_t(0x2026);
    case 0x8B: return char32_t(0x2039);
    case 0x91: return char32_t(0x2018);
    case 0x92: return char32_t(0x2019);
    case 0x93: return char32_t(0x201C);
    case 0x94: return char32_t(0x201D);
    case 0x95: return char32_t(0x2022);
    case 0x96: return char32_t(0x2013);
    case 0x97: return char32_t(0x2014);
    case 0x9B: return char32_t(0x203A);
    default: return 0;
    }
}

void put_utf8(std::string& out, char32_t r)
{
    if (r < 0x80) {
        out.push_back(static_cast<char>(r));
    } else if (r < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (r >> 6)));
        out.push_back(static_cast<char>(0x80 | (r & 0x3F)));
    } else if (r < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (r >> 12)));
        out.push_back(static_cast<char>(0x80 | ((r >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (r & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (r >> 18)));
        out.push_back(static_cast<char>(0x80 | ((r >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((r >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (r & 0x3F)));
    }
}

}   // namespace

char32_t tcvn3_rune(unsigned char b) noexcept
{
    if (b < 0x80) return static_cast<char32_t>(b);
    return kTcvn3[static_cast<std::size_t>(b - 0x80)];
}

bool is_tcvn3_loose(std::string_view b) noexcept
{
    int run = 0;
    for (const char ch : b) {
        const auto c = static_cast<unsigned char>(ch);
        if (c < 0x80) {
            run = 0;
            continue;
        }
        if (cp1252_punct(c) == 0 && kTcvn3[static_cast<std::size_t>(c - 0x80)] == 0) return false;
        if (++run > 3) return false;
    }
    return true;
}

std::string gbk_to_utf8(std::string_view b)
{
#ifdef _WIN32
    if (b.empty()) return {};
    const int wlen = MultiByteToWideChar(936, MB_ERR_INVALID_CHARS, b.data(), static_cast<int>(b.size()), nullptr, 0);
    if (wlen <= 0) return std::string(b);
    std::wstring wide(static_cast<std::size_t>(wlen), L'\0');
    MultiByteToWideChar(936, 0, b.data(), static_cast<int>(b.size()), wide.data(), wlen);
    const int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return std::string(b);
    std::string out(static_cast<std::size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, out.data(), ulen, nullptr, nullptr);
    return out;
#else
    return std::string(b);   // no code page 936 here: the bytes as they are
#endif
}

std::string tcvn3_to_utf8(std::string_view b)
{
    std::string out;
    out.reserve(b.size() * 2);
    for (const char ch : b) {
        const auto c = static_cast<unsigned char>(ch);
        if (c < 0x80) {
            out.push_back(ch);
        } else if (kTcvn3[static_cast<std::size_t>(c - 0x80)] != 0) {
            put_utf8(out, kTcvn3[static_cast<std::size_t>(c - 0x80)]);
        } else if (const char32_t p = cp1252_punct(c); p != 0) {
            put_utf8(out, p);
        } else {
            put_utf8(out, char32_t(0xFFFD));
        }
    }
    return out;
}

std::string decode_mixed(std::string_view b)
{
    bool high = false;
    for (const char ch : b) {
        if (static_cast<unsigned char>(ch) >= 0x80) {
            high = true;
            break;
        }
    }
    if (!high) return std::string(b);
    std::string out;
    out.reserve(b.size() * 2);
    std::size_t start = 0;
    const auto flush = [&](std::size_t end) {
        if (end <= start) return;
        const std::string_view seg = b.substr(start, end - start);
        out += is_tcvn3_loose(seg) ? tcvn3_to_utf8(seg) : gbk_to_utf8(seg);
    };
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b[i] == '"' || b[i] == '-') {   // bytes no GBK pair and no TCVN3 letter ever holds
            flush(i);
            out.push_back(b[i]);
            start = i + 1;
        }
    }
    flush(b.size());
    return out;
}

}   // namespace jx::zone::text
