// Helpers to send protobuf messages over a Connection and to parse frame payloads.
#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <google/protobuf/message_lite.h>

#include "jx/frame.hpp"
#include "jx/net/connection.hpp"

namespace jx::net {

// Serializes msg into a complete frame (shared so it can be sent to many connections).
inline std::shared_ptr<const std::vector<std::uint8_t>> frame_of(std::uint16_t msg_id,
                                                                 const google::protobuf::MessageLite& msg)
{
    const std::size_t size = msg.ByteSizeLong();
    auto buf = std::make_shared<std::vector<std::uint8_t>>(frame::kMinFrameSize + size);
    std::uint8_t* p = buf->data();
    frame::put_u32(p, static_cast<std::uint32_t>(frame::kHeaderSize + size));
    frame::put_u16(p + 4, msg_id);
    frame::put_u16(p + 6, 0);
    if (size != 0) msg.SerializeWithCachedSizesToArray(p + frame::kMinFrameSize);
    return buf;
}

template <class Msg>
inline void send(Connection& c, std::uint16_t msg_id, const Msg& msg)
{
    c.send_raw(frame_of(msg_id, msg));
}

template <class Msg>
inline bool parse(const frame::View& view, Msg& out)
{
    return out.ParseFromArray(view.payload.data(), static_cast<int>(view.payload.size()));
}

template <class Msg>
inline bool parse(std::span<const std::uint8_t> bytes, Msg& out)
{
    return out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()));
}

template <class Msg>
inline bool parse(const std::string& bytes, Msg& out)
{
    return out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()));
}

} // namespace jx::net
