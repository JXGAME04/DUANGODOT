#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "jx/ids.hpp"
#include "jx/zone/KRegion.h"

namespace jx::zone {

// Positions are simulated in fixed point (kSub sub-units per world unit) with integer math only,
// so every platform produces bit-identical results for the same inputs (replayable ticks).
inline constexpr std::int64_t kSub = 256;

enum class KNpcKind : std::uint8_t { player = 1, npc = 2, monster = 3, drop = 4 };

struct KNpc {
    EntityId id;
    KNpcKind kind = KNpcKind::npc;
    std::string name;

    std::int64_t fx = 0, fy = 0;   // current position (sub-units)
    std::int64_t tx = 0, ty = 0;   // next waypoint (sub-units)
    std::vector<Pos> path;         // waypoints after the current one; last = final destination
    bool moving = false;
    std::uint32_t dir = 0;         // facing 0..7
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
    [[nodiscard]] Pos destination() const noexcept { return path.empty() ? target() : path.back(); }
    void set_pos(Pos p) noexcept
    {
        fx = static_cast<std::int64_t>(p.x) * kSub;
        fy = static_cast<std::int64_t>(p.y) * kSub;
        tx = fx;
        ty = fy;
        path.clear();
        moving = false;
    }
    // Starts moving along waypoints (the first becomes the current target).
    void set_path(std::vector<Pos> waypoints) noexcept
    {
        path = std::move(waypoints);
        if (path.empty()) {
            tx = fx;
            ty = fy;
            moving = false;
            return;
        }
        next_waypoint();
    }
    void set_target(Pos p) noexcept { set_path({p}); }
    // Pops the next waypoint into (tx, ty); returns false when none is left.
    bool next_waypoint() noexcept
    {
        if (path.empty()) {
            moving = false;
            return false;
        }
        const Pos p = path.front();
        path.erase(path.begin());
        tx = static_cast<std::int64_t>(p.x) * kSub;
        ty = static_cast<std::int64_t>(p.y) * kSub;
        moving = (tx != fx || ty != fy);
        if (moving) update_dir();
        return true;
    }
    void update_dir() noexcept
    {
        // 8 directions, 0 = down, counter-clockwise like the old sprites (see docs/MAPS.md)
        const std::int64_t dx = tx - fx, dy = ty - fy;
        if (dx == 0 && dy == 0) return;
        const double a = std::atan2(static_cast<double>(dx), static_cast<double>(dy));   // 0 = +y (down)
        int d = static_cast<int>(std::lround(a / (3.14159265358979323846 / 4.0)));
        d = ((d % 8) + 8) % 8;
        dir = static_cast<std::uint32_t>(d);
    }
};

} // namespace jx::zone
