#include "jx/frame.hpp"

#include <cstring>

namespace jx::frame {

void encode(std::vector<std::uint8_t>& out, std::uint16_t msg_id, std::span<const std::uint8_t> payload,
            std::uint16_t flags)
{
    const std::size_t start = out.size();
    out.resize(start + kMinFrameSize + payload.size());
    std::uint8_t* p = out.data() + start;
    put_u32(p, static_cast<std::uint32_t>(kHeaderSize + payload.size()));
    put_u16(p + 4, msg_id);
    put_u16(p + 6, flags);
    if (!payload.empty()) std::memcpy(p + kMinFrameSize, payload.data(), payload.size());
}

std::vector<std::uint8_t> encode(std::uint16_t msg_id, std::span<const std::uint8_t> payload, std::uint16_t flags)
{
    std::vector<std::uint8_t> out;
    out.reserve(kMinFrameSize + payload.size());
    encode(out, msg_id, payload, flags);
    return out;
}

void Parser::feed(std::span<const std::uint8_t> bytes)
{
    if (bytes.empty()) return;
    compact();
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

ParseStatus Parser::next(View& view)
{
    const std::size_t avail = buffer_.size() - consumed_;
    if (avail < kLengthSize) return ParseStatus::need_more;
    const std::uint8_t* p = buffer_.data() + consumed_;
    const std::uint32_t len = get_u32(p);
    if (len < kHeaderSize) return ParseStatus::corrupt;
    if (len - kHeaderSize > max_payload_) return ParseStatus::too_large;
    if (avail < kLengthSize + len) return ParseStatus::need_more;

    view.msg_id = get_u16(p + 4);
    view.flags = get_u16(p + 6);
    view.payload = std::span<const std::uint8_t>(p + kMinFrameSize, len - kHeaderSize);
    consumed_ += kLengthSize + len;
    return ParseStatus::ok;
}

void Parser::reset()
{
    buffer_.clear();
    consumed_ = 0;
}

void Parser::compact()
{
    if (consumed_ == 0) return;
    if (consumed_ >= buffer_.size()) {
        buffer_.clear();
    } else {
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(consumed_));
    }
    consumed_ = 0;
}

} // namespace jx::frame
