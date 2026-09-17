// Wire framing shared by every TCP link (client<->gateway, gateway<->zone).  docs/PROTOCOL.md.
//
//   +----------+----------+----------+------------------+
//   | u32 len  | u16 msg  | u16 flag | payload (len-4)  |   all little-endian
//   +----------+----------+----------+------------------+
//   len   = bytes after the length field (4 header bytes + payload)
//   msg   = jx.pb.MsgId
//   flag  = bit0 payload compressed (reserved, phase 4), bit1 encrypted (reserved)
//
// The same layout is implemented in Go (pkg/frame) and GDScript (autoload/Net.gd); the test
// vectors in tests/test_frame.cpp are the contract all three must pass.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace jx::frame {

inline constexpr std::size_t kLengthSize = 4;
inline constexpr std::size_t kHeaderSize = 4;           // msg + flags, counted inside len
inline constexpr std::size_t kMinFrameSize = kLengthSize + kHeaderSize;
inline constexpr std::uint32_t kMaxClientPayload = 64u * 1024u;          // client -> gateway
inline constexpr std::uint32_t kMaxInternalPayload = 4u * 1024u * 1024u; // gateway <-> zone

inline constexpr std::uint16_t kFlagCompressed = 0x0001;
inline constexpr std::uint16_t kFlagEncrypted = 0x0002;

struct Frame {
    std::uint16_t msg_id = 0;
    std::uint16_t flags = 0;
    std::vector<std::uint8_t> payload;
};

struct View {
    std::uint16_t msg_id = 0;
    std::uint16_t flags = 0;
    std::span<const std::uint8_t> payload;
};

// Appends one complete frame to 'out'.
void encode(std::vector<std::uint8_t>& out, std::uint16_t msg_id, std::span<const std::uint8_t> payload,
            std::uint16_t flags = 0);
std::vector<std::uint8_t> encode(std::uint16_t msg_id, std::span<const std::uint8_t> payload,
                                 std::uint16_t flags = 0);
inline std::span<const std::uint8_t> as_bytes(std::string_view s) noexcept
{
    return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(s.data()), s.size());
}
inline std::vector<std::uint8_t> encode(std::uint16_t msg_id, std::string_view payload, std::uint16_t flags = 0)
{
    return encode(msg_id, as_bytes(payload), flags);
}
inline void encode(std::vector<std::uint8_t>& out, std::uint16_t msg_id, std::string_view payload, std::uint16_t flags = 0)
{
    encode(out, msg_id, as_bytes(payload), flags);
}

enum class ParseStatus { ok, need_more, too_large, corrupt };

// Incremental parser: feed() bytes as they arrive, then next() until it returns need_more.
class Parser {
public:
    explicit Parser(std::uint32_t max_payload = kMaxInternalPayload) : max_payload_(max_payload) {}

    void feed(std::span<const std::uint8_t> bytes);

    // Extracts the next complete frame.  'view' points into the parser's buffer and is valid
    // until the next call to feed() or next().
    ParseStatus next(View& view);

    std::size_t buffered() const noexcept { return buffer_.size() - consumed_; }
    void reset();

private:
    void compact();
    std::vector<std::uint8_t> buffer_;
    std::size_t consumed_ = 0;
    std::uint32_t max_payload_;
};

// Little-endian helpers (the wire is always little-endian regardless of host).
inline void put_u16(std::uint8_t* p, std::uint16_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v & 0xFFu);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
}
inline void put_u32(std::uint8_t* p, std::uint32_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v & 0xFFu);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<std::uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<std::uint8_t>((v >> 24) & 0xFFu);
}
inline std::uint16_t get_u16(const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}
inline std::uint32_t get_u32(const std::uint8_t* p) noexcept
{
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

} // namespace jx::frame
