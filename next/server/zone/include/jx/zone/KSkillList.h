#pragma once

// KSkillList of the old core (Core/Src/KSkillList.h) the way the JX2 server keeps it - the list
// at KNpc+0x248 of jx_linux_y (docs/LINUX-SERVER.md §15), read function by function: 80 cells of
// 0x30 bytes (cell 0 unused), the forbid flag, the KSkillLevelIncNode list of the "+N skill
// levels" attributes (allskill_v 139) and the per-skill damage enhance map that the held skills'
// addskilldamage entries fill (KNpc+0x115c, read by CreateMissleMagicAttribsData).
//
// The list is data plus the rules on it.  What the binary reaches through globals (g_SkillManager,
// Npc[], the subworld frame) comes in through KSkillListHost, so the list is testable alone and
// the world supplies the casts of passive (style 3) skills and the removal of their states.

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "jx/zone/KMagicAttrib.h"

namespace jx::zone {

class KSkill;
class KSkillManager;

inline constexpr int kMaxNpcSkill = 80;   // MAX_NPCSKILL: cells 1..79 are searched (FindSame stops at 0x50)

// NPCSKILL of the old core, the JX2 cell of 0x30 bytes (KNpc+0x250 + 0x30 x cell)
struct KNpcSkill {
    int id = 0;                         // +0x00 SkillId
    int level = 0;                      // +0x04 SkillLevel: the learned level
    int max_times = 0;                  // +0x08 MaxTimes (zeroed by SetNpcSkill / Add, never read)
    int remain_times = 0;               // +0x0c RemainTimes (the same)
    std::uint64_t next_cast_time = 0;   // +0x10 NextCastTime: the frame the skill may be cast again
    int cool_down_time = 0;             // +0x14 the length of the last cool down (frames)
    int current_level = 0;              // +0x18 CurrentSkillLevel: level + the increments
    bool only_inc = false;              // +0x1c the cell lives on increments only (not saved, its cool down kept)
    int exp = 0;                        // +0x20 the skill experience of an IsExpSkill
    int max_level = 0;                  // +0x24 MaxLevel of the row (+ the reborn addon when added)
    int req_level = 0;                  // +0x28 ReqLevel of the instance at the added level
    int forbidden = 0;                  // +0x2c 1 = cannot be cast (ForbitSkill / SetAForbitSkill)
};

// KSkillLevelIncNode (typeinfo 0x0825830C): one "+inc levels" for a skill (0 = every skill)
struct KSkillLevelIncNode {
    int skill_id = 0;   // +0x10
    int inc = 0;        // +0x14
};

// What the list needs from outside - the binary reads g_SkillManager, Npc[m_nNpcIndex] and casts
// through KSkill::Cast; the zone hands those in.
struct KSkillListHost {
    KSkillManager* skills = nullptr;                    // GetSkill / InstanceSkill (0x08C544A0 + InstanceSkill 0x080E6E10)
    int npc_level = 0;                                  // Npc[m_nNpcIndex].m_Level (+0x20)
    std::function<void(const KSkill&)> cast_passive;    // Cast(sk, idx, -1, idx, 0, 0, 1): a style 3 skill on oneself
    std::function<void(int skill_id)> remove_state;     // RemoveStateSkillEffect(npc, id, 0)
};

// The record of the fight skill list in the role data: {int16 id, int16 level, int32 exp}
// (KSkillList 0x080E48D0 writes it, KPlayer::LoadPlayerFightSkillList 0x080C0240 reads it)
struct KSkillSaved {
    int id = 0;
    int level = 0;
    int exp = 0;
};

class KSkillList {
public:
    static constexpr int kMaxCurrentLevel = 64;   // a current level never goes above 64 (0x080E56A0)

    int npc_index = 0;                                // +0x0 m_nNpcIndex (unused by the zone)
    int forbid_all = 0;                               // +0x4 what a new cell starts with (ForbitSkill)
    std::array<KNpcSkill, kMaxNpcSkill> skills{};    // +0x8 the cells, 1..79
    std::vector<KSkillLevelIncNode> inc_nodes;        // +0xf08 KList<KSkillLevelIncNode>
    std::map<int, int> enhance;                       // +0xf14 (KNpc+0x115c): skill id -> damage enhance percent

    // ---- lookups (each one counts a search in g_nSkillListFindCount 0x08BC99B4 - not kept)
    [[nodiscard]] int find_same(int id) const noexcept;        // 0x080E4290: the cell of a skill, 0 = none
    [[nodiscard]] int find_free() const noexcept;              // 0x080E42E0: the first cell without a skill, 0 = full
    [[nodiscard]] int get_level(int id) const noexcept;        // 0x080E43F0: SkillLevel, 0 above 63 or without the skill
    // 0x080E4440: CurrentSkillLevel (0 above 63); with_inc = false takes the increments of the
    // nodes for this skill and for every skill off again
    [[nodiscard]] int get_current_level(int id, bool with_inc) const noexcept;
    [[nodiscard]] int get_all_inc() const noexcept;            // 0x080E43B0: the node of every skill (0), or 0
    [[nodiscard]] int get_count() const noexcept;              // 0x080E4380: cells holding a skill
    // 0x080E4B30: the skills held; all = false skips WeaponSkill rows, IsExpSkill rows and cells at level 0
    [[nodiscard]] int get_count(bool all, KSkillManager* mgr) const;
    // 0x080E4C00: the levels of the skills held, WeaponSkill and IsExpSkill rows left out
    [[nodiscard]] int get_total_level(KSkillManager* mgr) const;

    // ---- cool down / forbid
    // 0x080E4540: a cell with a current level, not forbidden, its cool down over, and the npc's
    // level at least the cell's ReqLevel (skipped when level <= 0)
    [[nodiscard]] bool can_cast(int id, std::uint64_t frame, int npc_level) const noexcept;
    [[nodiscard]] bool is_cooling(int id, std::uint64_t frame) const noexcept;             // 0x080E44D0: frame < NextCastTime
    void set_next_cast_time(int id, std::uint64_t frame, int cool_down) noexcept;           // 0x080E4640
    [[nodiscard]] std::uint64_t next_cast_time(int id) const noexcept;                    // 0x080E46A0
    [[nodiscard]] int cool_down_time(int id) const noexcept;                              // 0x080E46F0
    void reduce_cool_time(int id, int frames) noexcept;                                   // 0x080E4740 (reduceskillcd1..3)
    void clear_cool_time(std::uint64_t frame) noexcept;                                   // 0x080E47B0: pending cool downs dropped
    [[nodiscard]] bool is_forbidden(int id) const noexcept;                               // 0x080E45C0
    void set_forbid_all(int v) noexcept;                                                  // 0x080E4610: the flag and every cell held
    void set_forbid(int id, int v) noexcept;                                              // KPlayer::SetAForbitSkill 0x080AE9E0

    // ---- cells
    // 0x080E4310 (a npc's Skill1..4): level > 0, cell 1..79, id > 0; level = current = max level
    void set_npc_skill(int no, int id, int level) noexcept;
    // 0x080E5420 KSkillList::Add(id, level, exp, maxTimes, remainTimes, maxLevel): the cell of the
    // skill (or a free one) brought to `level` through increase_level; max level = the override,
    // else the row's MaxLevel plus `max_level_addon` (a reborn player's Player+0x8600).  Returns
    // the cell, 0 when refused (level < 0, id <= 0, no free cell).
    int add(int id, int level, int exp, int max_level_override, int max_level_addon, KSkillListHost& host);
    // 0x080E52D0: the level taken away; a cell that still has a current level from increments
    // stays as an only_inc cell, the others are freed
    void remove(int id, KSkillListHost& host);
    // 0x080E5010 KSkillList::IncreaseLevel(idx, delta): level and current level move together,
    // the experience is reset, the enhance map follows, a passive (style 3) skill is cast again
    // or its state removed.  Returns 1, or 0 for a bad cell / row / a current level out of 1..63.
    int increase_level(int idx, int delta, KSkillListHost& host);
    // 0x080E56A0: `delta` on the current level of one cell (idx) or of every cell held (idx = 0),
    // capped at 64: passives are recast or their states removed, the enhance map follows, an
    // only_inc cell that falls to 0 is freed.  Returns 1; 0 when a cell reaches exactly 64 or a
    // row is missing (the binary's quirks kept).
    int change_current_level(int idx, int delta, KSkillListHost& host);
    // 0x080E5BF0 (allskill_v 139: a piece worn / a state): `delta` levels for `id` (0 = every
    // skill).  A skill not held gets an only_inc cell; the node is kept while its sum is not 0.
    int add_level_inc(int id, int delta, KSkillListHost& host);
    // 0x080E4CA0: the addskilldamage entries of the instance at the old level leave the enhance
    // map, those of the new level's instance enter it (rows of style 0..3 / 14 only)
    void update_enhance(int idx, int old_level, int new_level, KSkillManager* mgr);
    // KNpc::ClearAttrib 0x0807F341: every current level above the learned one goes back to it,
    // the enhance map with it (the increments are applied again by the caller)
    void clear_attrib(KSkillManager* mgr);
    // 0x080E4A00 (KPlayer::LevelUp): the passives whose ReqLevel is exactly the npc's level are
    // cast; true when one was
    bool cast_passives_at_level(KSkillListHost& host);
    // 0x080E5370 (RollbackSkill): every skill that is not a WeaponSkill / IsExpSkill row back to
    // level 0; returns the levels taken (the points to give back)
    int rollback(KSkillListHost& host);

    // ---- experience (IsExpSkill rows)
    // 0x080E4F20: exp / the level's skill_skillexp_v in 1/1024 (1024 when reached), 0 otherwise
    [[nodiscard]] int exp_percent(int idx, KSkillManager* mgr) const;
    struct ExpResult {
        bool handled = false;   // the skill is held, an exp skill, at a level 1..63
        int idx = 0;            // its cell
        int old_level = 0;
        int old_percent = 0;
        bool level_up = false;   // the need was reached and the level moved (value[2] bit 0 clear)
        bool level_reached = false;   // the need was reached (the level moved unless value[2] bit 0)
    };
    // 0x080E5D90 KSkillList::AddSkillExp(&attrib, bPercent): attrib.value[0] = skill, value[1] =
    // the experience (percent_mode: in 1/10000 of the level's need), value[2] bit 0 = do not
    // level up.  The list's part only - the packets, the OnLevelUp script and the message are the
    // caller's (KSubWorld::give_skill_exp).
    ExpResult add_skill_exp(const KMagicAttrib& attrib, bool percent_mode, KSkillListHost& host);

    // ---- role data
    [[nodiscard]] std::vector<KSkillSaved> serialize() const;   // 0x080E48D0: the cells held, only_inc ones left out
    // KPlayer::LoadPlayerFightSkillList 0x080C0240: Add(id, level, exp, 0, 0, 0) for each record
    void deserialize(const std::vector<KSkillSaved>& saved, int max_level_addon, KSkillListHost& host);

    [[nodiscard]] const KNpcSkill* cell(int idx) const noexcept
    {
        return idx > 0 && idx < kMaxNpcSkill ? &skills[static_cast<std::size_t>(idx)] : nullptr;
    }

private:
    [[nodiscard]] KNpcSkill& at(int idx) noexcept { return skills[static_cast<std::size_t>(idx)]; }
    [[nodiscard]] const KNpcSkill& at(int idx) const noexcept { return skills[static_cast<std::size_t>(idx)]; }
    [[nodiscard]] static bool valid_skill_id(int id) noexcept { return id >= 1 && id <= 2000; }
    [[nodiscard]] KSkillLevelIncNode* inc_node(int skill_id) noexcept;
    // the instance of a row at a level (GetSkill, then InstanceSkill): nullptr without a manager
    [[nodiscard]] static const KSkill* instance(KSkillManager* mgr, int id, int level);
    // 0x080E5010 / 0x080E56A0: the rows whose enhance entries count (style 0..3 and 14)
    [[nodiscard]] static bool enhance_style(int style) noexcept { return style >= 0 && style <= 14 && (((1 << style) & 0x400f) != 0); }
    void free_cell(int idx) noexcept;
};

} // namespace jx::zone
