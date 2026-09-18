#pragma once

// KMissle / KMissleSet of the old core (Core/Src/KMissle.h) the way the JX2 server flies them
// (jx_linux_y; docs/LINUX-SERVER.md §13).  The names are the old members'; the offsets in the
// comments are those of the JX2 missile (0x188 bytes, the array at [0x0836EA50], one template per
// row of \settings\missles.txt at 0x0830ED80 + id x 0x188):
//
//   loader           0x0805D210 / 0x08074300   MissleId 1..999, the 18 columns the server reads
//   KSkill::CreateMissle       0x080EA310      the template copied (0x08074AA0), the skill's fields,
//                                              the missle_* attributes of the level
//   KMissleSet::Add            0x08076F00      a slot, the position (Mps2Map 0x080EF7F0)
//   the frame                  0x08076950      launcher / target checks, the life, the start,
//                                              PrePareFly 0x08076550, Activate 0x080760E0, the fly event
//   KMissle::OnFly             0x080758E0      TestBarrier, CheckCollision 0x08075770, the move kinds,
//                                              CheckBeyondRegion 0x08074F10
//   ProcessCollision           0x08075630 / 0x08075710, ProcessDamage 0x080753F0
//   DoCollision 0x08075340, DoVanish 0x08075210, the events 0x080EE810
//
// Positions: the old missile keeps a region, a cell in it and a 1/1024 offset inside the cell; the
// zone keeps the absolute cell (32 units, KSubWorld+0x24/+0x28 of the binary) and the same offset,
// 0..32768 inclusive as 0x08074FFA leaves it, so a missile on a cell edge belongs to the cell the
// binary would give it.  Everything the code base reads goes through KSubWorld::missle_pos.

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "jx/ids.hpp"
#include "jx/zone/KRegion.h"
#include "jx/zone/KSkill.h"

namespace jx::zone {

inline constexpr int kMaxMissleTemplate = 999;             // MissleId 1..999 (0x0805D264)
inline constexpr int kMissleCell = 32;                     // the cell the ranges count (CELL_LENGTH)
inline constexpr int kMissleCellUnits = kMissleCell << 10; // a cell in 1/1024 units
inline constexpr int kMissleFlyStep = 10;                  // [0x082E1B00]: a straight missile moves in steps of this many units
inline constexpr int kMissleMaxHitHeight = 20;             // 0x0807578B: higher than this (in cells) a missile hits nothing
inline constexpr int kMissleFollowFrames = 8;              // 0x08075D61: a following missile aims again once its counter passes this
inline constexpr int kMissleCircleRadius = 50;             // 0x080759E2: a circling missile turns at speed + 50
inline constexpr int kMaxMissles = 4096;                   // slots per map ([0x0830CA64] of the binary is read at run time)

// eMissleMoveKind of SkillDef.h (MISSLE_MMK_*), the MoveKind column
enum KMissleMoveKind : int {
    missle_move_stand = 0,
    missle_move_line = 1,
    missle_move_random = 2,
    missle_move_circle = 3,
    missle_move_helix = 4,
    missle_move_follow = 5,
    missle_move_motion = 6,
    missle_move_parabola = 7,
    missle_move_roll_back = 100,
};

// eMisslesForm of SkillDef.h, the MisslesForm column (the jump table 0x08258478 of CastMissles)
enum KMisslesForm : int {
    missles_form_wall = 0,        // CastWall 0x080EBB20: a row across the direction, Param1 apart
    missles_form_line = 1,        // CastLine 0x080EC2F0 along it, or CastExtractiveLineMissle 0x080EBF00 for a MoveKind 7 child
    missles_form_spread = 2,      // CastSpread 0x080EB150: a fan
    missles_form_circle = 3,      // CastCircle 0x080EB720: a ring
    missles_form_random = 4,      // nothing (the start event only)
    missles_form_zone = 5,        // CastZone 0x080EC690 at the launcher, the target kept
    missles_form_at_target = 6,   // CastZone at the target
    missles_form_at_firer = 7,    // CastZone at the launcher, no target
};

// eMissleStatus (+0xf0)
enum KMissleStatus : int {
    missle_status_wait = 0,
    missle_status_fly = 1,
    missle_status_vanished = 2,
};

// The columns of missles.txt the server reads (0x08074300; an empty cell is 0 like
// KTabFile::GetInteger) and where they land in the missile.
struct KMissleTemplate {
    int id = 0;                   // MissleId       +0xf4 / +0xf8
    std::string name;             // MissleName (not read by the server; for the logs)
    int height = 0;               // MissleHeight << 10   +0x1c
    int move_kind = 0;            // MoveKind             +0x14
    int follow_kind = 0;          // FollowKind           +0x18
    int life_time = 0;            // LifeTime             +0x24
    int speed = 0;                // Speed                +0x28
    int response_skill = 0;       // ResponseSkill        +0x2c (the skill id takes its place in flight)
    int collide_range = 0;        // CollidRange          +0x40 (cells)
    bool collide_vanish = false;  // ColVanish            byte +0x11
    bool range_damage = false;    // IsRangeDmg           byte +0x10
    int damage_range = 0;         // DmgRange             +0x44 (cells)
    int z_acceleration = 0;       // Zacc                 +0xec
    int height_speed = 0;         // Zspeed               +0x20
    int miss_rate = 0;            // MissRate             +0x154
    int param1 = 0;               // Param1               +0x11c
    int param2 = 0;               // Param2               +0x120
    int param3 = 0;               // Param3               +0x124
    bool auto_explode = false;    // AutoExplode          byte +7
    int damage_interval = 0;      // DmgInterval          +0x5c (frames between two blows of one missile)

    static KMissleTemplate from_cells(const std::unordered_map<std::string, std::string>& cells);
};

// The rows of missles.json (jxassets export-missles), MissleId 1..999, the last row of an id winning
// (0x0805D210 writes the template of every row in turn).
class KMissleTable {
public:
    static std::optional<KMissleTable> load(const std::string& file, std::string* error);
    [[nodiscard]] const KMissleTemplate* find(int id) const;
    // The templates of the basic attacks (rows 64 / 65 of the Linux missles.txt: 长兵物理攻击 and
    // 远程物理攻击, what KSkill::basic_attack fires) for a map without a missile table (tests);
    // nullptr for any other id.
    [[nodiscard]] static const KMissleTemplate* basic_attack(int id);
    [[nodiscard]] std::size_t size() const noexcept { return rows_.size(); }
    void add(KMissleTemplate t);

private:
    std::unordered_map<int, KMissleTemplate> rows_;
};

// One missile of a map (KMissle of the old core; the JX2 fields at the offsets noted).
struct KMissle {
    int index = 0;                       // +0xf4: the slot, 1-based; 0 = free
    int missle_id = 0;                   // m_nMissleId +0xf8
    // what KSkill::CreateMissle takes from the skill (0x080EA36C..)
    int client_send = 0;                 // m_bClientSend      byte +4
    bool is_melee = false;               // m_bIsMelee         byte +6
    bool target_self = false;            // m_bTargetSelf      byte +8  (TargetSelf == 1)
    bool heel_at_parent = false;         // m_bHeelAtParent    byte +9
    bool fly_event = false;              // m_bFlyEvent        byte +0xa
    bool collide_event = false;          // m_bCollideEvent    byte +0xb
    int launcher_camp = 0;               // byte +0xd: m_CurrentCamp of the launcher when fired
    int launcher_pk_mode = 0;            // byte +0xe: KPlayer+0x5a50 of a player launcher (its PK object; 0 until B3)
    bool use_attack_rating = false;      // m_bUseAttackRating byte +0xf
    int skill_id = 0;                    // m_nSkillId         +0x2c
    int relation = 0;                    // m_eRelation        +0x30 (KSkillRelation bits)
    int interrupt_when_move = 0;         // m_nInteruptTypeWhenMove +0x34 (StopWhenMove)
    Pos launcher_src;                    // m_nLauncherSrcPX/PY +0x38 / +0x3c
    int series = 0;                      // +0x50
    int fly_event_time = 0;              // m_nFlyEventTime    +0x54
    bool vanished_event = false;         // m_bVanishedEvent   +0x58
    int do_hurt = 0;                     // m_bDoHurt          +0xc4 (the DoHurt percent)
    int level = 0;                       // m_nLevel           +0xd0
    int relative_pos_type = 0;           // +0x138 (RelativePosType)
    // from the template (0x08074AA0), then the missle_* attributes of the level
    bool auto_explode = false;           // m_bAutoExplode     byte +7
    bool range_damage = false;           // m_bRangeDamage     byte +0x10
    bool collide_vanish = false;         // m_bCollideVanish   byte +0x11 (rolled at creation: 0x0807EE20)
    int move_kind = 0;                   // m_eMoveKind        +0x14
    int follow_kind = 0;                 // m_eFollowKind      +0x18
    int height = 0;                      // m_nHeight          +0x1c (MissleHeight << 10)
    int height_speed = 0;                // m_nHeightSpeed     +0x20
    int life_time = 0;                   // m_nLifeTime        +0x24 (frames; the start delay is added)
    int speed = 0;                       // m_nSpeed           +0x28 (units per frame; halved by slowmissle_b)
    int collide_range = 0;               // m_nCollideRange    +0x40
    int damage_range = 0;                // m_nDamageRange     +0x44
    int damage_interval = 0;             // m_ulDamageInterval +0x5c
    int z_acceleration = 0;              // m_nZAcceleration   +0xec
    int param1 = 0;                      // +0x11c (a following missile counts its frames here)
    int param2 = 0;                      // +0x120 (a following missile counts down to its jump here)
    int param3 = 0;                      // +0x124
    int miss_rate = 0;                   // +0x154
    int rest_hit_count = 0;              // +0x158: missle_hitcount, 0 = unlimited
    // the flight
    int status = missle_status_wait;     // m_eMissleStatus    +0xf0
    int current_life = 0;                // m_nCurrentLife     +0x60: frames since creation
    int start_life_time = 0;             // m_nStartLifeTime   +0x64
    bool on_map = false;                 // m_nRegionId >= 0   +0x100
    int map_x = 0, map_y = 0;            // m_nCurrentMapX/Y   +0x6c / +0x70 (absolute cells here)
    int map_z = 0;                       // m_nCurrentMapZ     +0x74 = height >> 10 at creation
    int x_offset = 0, y_offset = 0;      // m_nXOffset/YOffset +0x78 / +0x7c (1/1024 units, 0..32768)
    Pos ref;                             // m_nRefPX/PY        +0x80 / +0x84: where it was born (the anchor of RelativePosType 1)
    Pos des;                             // m_nDesMapX/Y       +0x88 / +0x8c: where a following missile last saw its target
    int x_factor = 0, y_factor = 0;      // m_nXFactor/YFactor +0xc8 / +0xcc: the flight vector, x1024 per unit of speed
    EntityId follow;                     // m_nFollowNpcIdx / m_dwFollowNpcID +0xd4 / +0xd8: the target
    EntityId launcher;                   // m_nLauncher / m_dwLauncherId +0xdc / +0xe0
    int parent_missle = 0;               // m_nParentMissleIndex +0xe4: 0 = the npc fired it
    int arrive_from = 0, arrive_to = 0;  // +0x128 / +0x12c: the frames a MoveKind 7 missile can hit its spot
    bool must_be_hit = false;            // m_bMustBeHit       byte +0xc: MoveKind 7 - the spot itself is the target
    bool turned = false;                 // +0x130: a roll-back missile came back
    int turn_frame = 0;                  // +0x134: when it does
    std::int64_t rel_x = 0, rel_y = 0;   // +0x13c / +0x140: the way flown from the anchor (RelativePosType)
    int dir_index = 0;                   // m_nDirIndex        +0x144
    int dir = 0;                         // m_nDir             +0x148
    int angle = 0;                       // m_nAngle           +0x14c: the angle of a circling missile
    std::uint64_t born_tick = 0;         // m_dwBornTime       +0x150
    std::uint64_t next_damage_tick = 0;  // m_ulNextCalDamageTime +0x15c
    int last_map_x = -1, last_map_y = -1;   // +0x160 / +0x164: the cell of the last collision (-1 = none)
    std::shared_ptr<const KMissleMagicAttribsList> attribs;   // +0x184: the payload, shared by the missiles of one cast

    [[nodiscard]] bool used() const noexcept { return index > 0; }
    [[nodiscard]] bool flying() const noexcept { return index > 0 && status == missle_status_fly; }
};

} // namespace jx::zone
