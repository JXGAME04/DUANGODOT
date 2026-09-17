#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "jx/ids.hpp"
#include "jx/zone/KMath.h"
#include "jx/zone/KRegion.h"

namespace jx::zone {

// Positions are simulated in fixed point (kSub sub-units per world unit) with integer math only,
// so every platform produces bit-identical results for the same inputs (replayable ticks).
inline constexpr std::int64_t kSub = 256;

enum class KNpcKind : std::uint8_t { player = 1, npc = 2, monster = 3, drop = 4 };

// KNpc::m_Doing of the old game, the part the zone simulates.
enum class KDoing : std::uint8_t { stand = 0, walk, attack, hurt, death, revive };

struct KNpc {
    EntityId id;
    KNpcKind kind = KNpcKind::npc;
    std::string name;

    std::int64_t fx = 0, fy = 0;   // current position (sub-units)
    std::int64_t tx = 0, ty = 0;   // next waypoint (sub-units)
    std::vector<Pos> path;         // waypoints after the current one; last = final destination
    bool moving = false;
    std::uint32_t dir = 0;         // facing 0..63 (old g_GetDirIndex: 0 = down, clockwise on screen)
    std::uint32_t speed = 0;       // world units per second
    std::uint32_t move_seq = 0;    // last MoveReq.seq (players)

    std::uint32_t level = 1;
    std::uint32_t series = 0;
    std::uint32_t sex = 0;
    std::uint32_t template_id = 0;

    // combat / animation state (KNpc::m_Doing, m_Frames).  One frame = one zone tick: the zone
    // ticks at 18 Hz like the old logic loop, so the frame counts of npcs.txt keep their meaning.
    KDoing doing = KDoing::stand;
    std::uint32_t frame_total = 0;   // m_Frames.nTotalFrame
    std::uint32_t frame_cur = 0;     // m_Frames.nCurrentFrame
    EntityId attack_target;          // kept attacking until it dies or we are told to move
    std::uint32_t approach_tries = 0;   // walks toward an out-of-reach target, a few times at most
    std::uint32_t life = 0;
    std::uint32_t life_max = 0;
    // KNpc::Load from the template (KNpcTemplate); the AttackSpeed column is the attack length
    std::uint32_t attack_frame = 20;
    std::uint32_t hurt_frame = 10;
    std::uint32_t death_frame = 15;
    std::uint32_t hit_recover = 12;
    std::uint32_t revive_frame = 2400;
    std::uint32_t attack_speed = 0;  // m_CurrentAttackSpeed (percent)
    std::uint32_t min_damage = 1;
    std::uint32_t max_damage = 3;

    [[nodiscard]] bool alive() const noexcept { return doing != KDoing::death && doing != KDoing::revive; }
    // KNpc::WaitForFrame: advances the action; true when its frames ran out (counter wraps to 0).
    bool wait_for_frame() noexcept
    {
        ++frame_cur;
        if (frame_cur < frame_total) return false;
        frame_cur = 0;
        return true;
    }
    // KNpc::IsReachFrame
    [[nodiscard]] bool reach_frame(std::uint32_t percent) const noexcept { return frame_cur == frame_total * percent / 100; }

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
        // 64 directions, 0 = down and clockwise on screen, exactly the old g_GetDirIndex (KMath.h)
        const int d = g_GetDirIndex(fx / kSub, fy / kSub, tx / kSub, ty / kSub);
        if (d >= 0) dir = static_cast<std::uint32_t>(d);
    }
};

} // namespace jx::zone
