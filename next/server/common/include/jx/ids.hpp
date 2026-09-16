// Strongly typed ids.  A SessionId can never be passed where a PlayerId is expected; 0 is "none".
#pragma once

#include <atomic>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include <fmt/format.h>

namespace jx {

template <class Tag>
struct Id {
    std::uint64_t value = 0;

    constexpr Id() noexcept = default;
    constexpr explicit Id(std::uint64_t v) noexcept : value(v) {}

    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr explicit operator bool() const noexcept { return valid(); }
    constexpr auto operator<=>(const Id&) const noexcept = default;
};

struct SessionTag {};
struct PlayerTag {};
struct EntityTag {};
struct ZoneTag {};

using SessionId = Id<SessionTag>;   // one per client connection, assigned by the gateway
using PlayerId  = Id<PlayerTag>;    // persistent character id (database)
using EntityId  = Id<EntityTag>;    // runtime id of anything in a zone (player, npc, item on ground)
using ZoneId    = Id<ZoneTag>;

template <class Tag>
std::string to_string(Id<Tag> id)
{
    return fmt::format("{}", id.value);
}

struct IdHash {
    template <class Tag>
    std::size_t operator()(Id<Tag> id) const noexcept
    {
        return std::hash<std::uint64_t>{}(id.value);
    }
};

// Thread-safe monotonic generator; never returns 0.
class IdGenerator {
public:
    explicit IdGenerator(std::uint64_t first = 1) noexcept : next_(first == 0 ? 1 : first) {}

    template <class IdT>
    IdT next() noexcept
    {
        return IdT{next_.fetch_add(1, std::memory_order_relaxed)};
    }
    [[nodiscard]] std::uint64_t peek() const noexcept { return next_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::uint64_t> next_;
};

// Start value unique across process restarts without a database: microseconds since the epoch
// shifted left 16 bits (65536 ids per microsecond before two runs could overlap).
inline std::uint64_t seed_from_time() noexcept
{
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
    return static_cast<std::uint64_t>(us) << 16u;
}

} // namespace jx

template <class Tag>
struct fmt::formatter<jx::Id<Tag>> : fmt::formatter<std::uint64_t> {
    template <class Ctx>
    auto format(jx::Id<Tag> id, Ctx& ctx) const
    {
        return fmt::formatter<std::uint64_t>::format(id.value, ctx);
    }
};
