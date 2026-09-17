#pragma once

#include <cstdint>
#include <string>

#include "jx/ids.hpp"
#include "jx/zone/aoi.hpp"

namespace jx::zone {

// Positions are simulated in fixed point (kSub sub-units per world unit) with integer math only,
// so every platform produces bit-identical results for the same inputs (replayable ticks).
inline constexpr std::int64_t kSub = 256;

enum class EntityKind : std::uint8_t { player = 1, npc = 2, monster = 3, drop = 4 };

struct Entity {
    EntityId id;
    EntityKind kind = EntityKind::npc;
    std::string name;

    std::int64_t fx = 0, fy = 0;   // current position (sub-units)
    std::int64_t tx = 0, ty = 0;   // move target (sub-units)
    bool moving = false;
    std::uint32_t speed = 0;       // world units per second
    std::uint32_t move_seq = 0;    // last MoveReq.seq (players)

    std::uint32_t level = 1;
    std::uint32_t series = 0;
    std::uint32_t sex = 0;
    std::uint32_t template_id = 0;

    std::uint64_t sid = 0;         // gateway session (players only)
    std::uint64_t player_id = 0;

    // simple wander behaviour for test npcs (0 = static)
    std::int32_t wander_radius = 0;
    Pos home;
    std::uint64_t next_wander_tick = 0;

    [[nodiscard]] Pos pos() const noexcept
    {
        return Pos{static_cast<std::int32_t>(fx / kSub), static_cast<std::int32_t>(fy / kSub)};
    }
    [[nodiscard]] Pos target() const noexcept
    {
        return Pos{static_cast<std::int32_t>(tx / kSub), static_cast<std::int32_t>(ty / kSub)};
    }
    void set_pos(Pos p) noexcept
    {
        fx = static_cast<std::int64_t>(p.x) * kSub;
        fy = static_cast<std::int64_t>(p.y) * kSub;
        tx = fx;
        ty = fy;
        moving = false;
    }
    void set_target(Pos p) noexcept
    {
        tx = static_cast<std::int64_t>(p.x) * kSub;
        ty = static_cast<std::int64_t>(p.y) * kSub;
        moving = (tx != fx || ty != fy);
    }
};

} // namespace jx::zone
