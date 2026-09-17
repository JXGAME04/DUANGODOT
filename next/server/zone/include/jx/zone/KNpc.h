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

// KSkillList::m_Skills[1..4] of a npc (Skill1..4 / Level1..4 of npcs.txt) as far as KNpcAI needs it.
struct KNpcSkillSlot {
    int id = 0;
    int level = 0;            // from the level script (Level1..4 cells); 0 = SetActiveSkill fails
    bool known = false;       // in skills.txt; unknown skills leave m_CurrentAttackRadius alone (GetSkill == NULL)
    int attack_radius = 0;    // KSkill::GetAttackRadius
    bool melee = false;
    bool target_self = false;
};

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
    std::uint32_t life = 0;          // m_CurrentLife (never below 0 here; death when a blow exceeds it)
    std::uint32_t life_max = 0;      // m_CurrentLifeMax
    std::uint64_t loop_frames = 0;   // m_LoopFrames: ticks alive, for the periodic state update
    // KNpc::Load from the template (KNpcTemplate); the AttackSpeed column is the attack length
    std::uint32_t attack_frame = 20;
    std::uint32_t cast_frame = 20;   // m_CastFrame: length of a non-melee skill (KNpc::DoSkill)
    std::uint32_t hurt_frame = 10;
    std::uint32_t death_frame = 15;
    std::uint32_t hit_recover = 12;
    std::uint32_t revive_frame = 2400;
    std::uint32_t attack_speed = 0;  // m_CurrentAttackSpeed (percent)
    // the level data (KNpcTemplate::InitNpcLevelData through the level script; placeholders for players)
    std::uint32_t min_damage = 1;    // m_PhysicsDamage.nValue[0]
    std::uint32_t max_damage = 3;    // m_PhysicsDamage.nValue[2]
    std::uint32_t attack_rating = 100;   // m_AttackRating (m_CurrentAttackRating)
    std::uint32_t defend = 0;        // m_Defend (m_CurrentDefend)
    int physics_resist = 0;          // m_PhysicsResist (m_CurrentPhysicsResist)
    int life_replenish = 0;          // m_LifeReplenish: life per GAME_UPDATE_TIME frames (KNpc::ProcessState)
    std::uint32_t exp = 0;           // m_Experience: what killing it is worth
    bool level_data_from_script = false;

    // KNpcAI state (server side of KNpc.h), named after the old members
    int npc_kind = 0;                 // NPCKIND of npcs.txt (0 normal, 2 partner, 3 dialoger, 4 bird, 5 mouse); players are kind_player
    int camp = 4;                     // m_Camp (NPCCAMP; KNpc::Init: camp_free)
    int current_camp = 4;             // m_CurrentCamp
    int ai_mode = 0;                  // m_AiMode (AIMode column; 0 = no ai)
    int ai_param[11] = {};            // m_AiParam[MAX_AI_PARAM]: [0..9] = AIParam1..10, [10] = max skill radius squared
    std::uint32_t ai_max_time = 25;   // m_AIMAXTime: ticks between two decisions
    std::uint64_t next_ai_time = 0;   // m_NextAITime
    int ai_add_life_time = 0;         // m_AiAddLifeTime: heals cast so far
    EntityId people_id;               // m_nPeopleIdx: the enemy locked on, or the last one that hurt us
    int vision_radius = 40;           // m_VisionRadius
    int active_radius = 30;           // m_ActiveRadius
    int current_active_radius = 30;   // m_CurrentActiveRadius
    int current_attack_radius = 30;   // m_CurrentAttackRadius (KNpc::Init 30, then the active skill's radius)
    int active_skill_id = 0;          // m_ActiveSkillID
    bool active_skill_melee = false;
    bool active_skill_self = false;
    KNpcSkillSlot skills[5];          // m_SkillList.m_Skills[1..4]
    int walk_speed = 5;               // m_WalkSpeed: scene units per frame (KNpc::ServeMove)
    int run_speed = 10;
    // players: KPlayer / trap state
    bool fight_mode = false;          // m_FightMode (SetFightState of the gate scripts)
    std::uint32_t trap_script_id = 0; // m_TrapScriptID: the trap under the feet, so a trap fires once per entry

    [[nodiscard]] bool alive() const noexcept { return doing != KDoing::death && doing != KDoing::revive; }
    // m_ProcessAI: the ai only decides while the npc stands or walks (DoSkill / DoAttack / DoHurt /
    // DoDeath clear the flag, OnSkill / OnHurt / Revive set it again).
    [[nodiscard]] bool process_ai() const noexcept { return doing == KDoing::stand || doing == KDoing::walk; }
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
    // The sessions whose client has been sent a spawn for this entity, sorted.  Every update about
    // it goes to exactly these, and so does its despawn (KViewer in KSubWorld.h is the other half).
    std::vector<std::uint64_t> watchers;

    // simple wander behaviour for test npcs without an AIMode (0 = static)
    std::int32_t wander_radius = 0;
    Pos home;                      // m_OriginX / m_OriginY: where the npc was placed
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
