#pragma once

// KPlayer of the old core - the part that turns a character's points, level and equipment into
// the numbers its KNpc fights with, ported from the JX2 server binary function by function
// (jx_linux_y, docs/LINUX-SERVER.md §10.2 / §10.4; offsets of KPlayer noted).  The zone keeps
// it inside the player's KNpc (Player[Npc.m_nPlayerIdx] of the old code, without the index).

#include <array>
#include <cstddef>
#include <cstdint>

#include "jx/ids.hpp"
#include "jx/zone/KFaction.h"

namespace jx::pb {
class RoleData;
}

namespace jx::zone {

struct KNpc;
class KItemList;
class KPlayerSet;

struct KPlayer {
    // the base points (what the player put there) and the current ones (base + equipment,
    // skills, states; KPlayer::UpdataCurData starts them equal)
    int strength = 0;          // m_nStrength      +0x5930
    int dexterity = 0;         // m_nDexterity     +0x5934
    int vitality = 0;          // m_nVitality      +0x5938
    int energy = 0;            // m_nEngergy       +0x593c
    int lucky = 0;             // m_nLucky         +0x5940
    int cur_strength = 0;      // m_nCurStrength   +0x5948
    int cur_dexterity = 0;     // +0x594c
    int cur_vitality = 0;      // +0x5950
    int cur_energy = 0;        // +0x5954
    int cur_lucky = 0;         // m_nCurLucky      +0x5958
    int attribute_point = 0;   // m_nAttributePoint +0x5924 (5 per level)
    int skill_point = 0;       // m_nSkillPoint     +0x5928 (1 per level)
    std::int64_t exp = 0;      // m_nExp           +0x595c
    std::int64_t next_level_exp = 0;   // m_nNextLevelExp +0x5964: what the current level needs to become the next
    int reborn = 0;            // +0x86b8 (<= 7): the experience table and the resistance floor
    int skill_max_level_addons = 0;   // +0x8600 (SetSkillMaxLevelAddons, <= 99): what a reborn character may add to every skill's MaxLevel
    bool loaded = false;       // LoadFrom ran (a player's npc; false for every other npc)
    // the revive point: KPlayer+0x20 (map), +0x28 / +0x2c (x, y) - SetTempRevPos 0x08110790 writes them, SetRevPos
    // 0x080B1E50 keeps the map and its reference point at +0x10 / +0x14 and copies the point's spot here; 0 = the
    // spawn point of the map (the zone has no reference-point table yet: SetRevPos keeps the map only)
    std::uint32_t revive_map = 0;
    int revive_x = 0;
    int revive_y = 0;
    int revive_ref = 0;        // +0x14
    // KPlayerFaction at +0x59cc: the current faction, the first and last joined, how many times (docs §16.7); LoadFrom
    // 0x080C1A62 reads current / last / count from the record, the first stays -1
    KPlayerFaction faction;
    // KPlayer+0x7d30..+0x7dbc: the npcs its create-npc skills (style 4, 0x080E8770) made - three records of 36 bytes at
    // +0x7d38 on a free list (+0x7da4: nodes 2 and 1, so two at most) and a used list (+0x7db0, count +0x7db8); the
    // ctor 0x080BC1F0 sets +0x7d30 = +0x7d34 = 2 free.  The skill fills one {time = nValue[2] of the attribute, the
    // launcher, the npc's id and index, kind = Param1}; KPlayer::Clear 0x080B60A0 (0x080B68F0) posts a removal for
    // each used one and frees them all.  Nothing reads the time (they never expire) and a summon's death frees nothing:
    // a kind is spent until the character leaves (docs/LINUX-SERVER.md §16.5)
    struct KSummonRecord {
        bool used = false;
        int time = 0;     // +0
        int kind = 0;     // byte +0x18: Param1 of the skill
        EntityId npc;     // +0xc / +0x14 its id (+0x10 its index); empty while the spawn waits for the end of the tick
    };
    static constexpr int kSummonRecords = 3;
    static constexpr int kSummonFree = 2;
    std::array<KSummonRecord, kSummonRecords> summons{};
    int summon_free = kSummonFree;   // +0x7d34
    // 0x080E8829 / 0x080E8F5B: the used records of that kind
    [[nodiscard]] int summon_count(int kind) const noexcept
    {
        int n = 0;
        for (const KSummonRecord& r : summons) {
            if (r.used && r.kind == kind) ++n;
        }
        return n;
    }
    // the node the free list hands out (records 1 and 2: node 0 is never on it), nullptr when +0x7d34 == 0
    [[nodiscard]] KSummonRecord* free_summon() noexcept
    {
        if (summon_free <= 0) return nullptr;
        for (std::size_t i = 1; i < summons.size(); ++i) {
            if (!summons[i].used) return &summons[i];
        }
        return nullptr;
    }
    // 0x080AFEA0 with a loss (KNpc::OnDeath 0x08088E8D hands -loss): the experience down, never below 0, the level kept
    void lose_exp(std::int64_t loss) noexcept;
    // what the exp bonuses of equipment / states left here (expenhance_v 175 -> a random range
    // +0xc8..+0xcc, expenhance_p 176 -> +0xd0, add120skillexpenhance_p 206 -> +0xd4); cleared by
    // UpdataCurData like the binary does
    int exp_enhance_lo = 0;
    int exp_enhance_hi = 0;
    int exp_enhance_percent = 0;
    int exp_enhance_percent2 = 0;

    // KPlayer::LoadFrom (0x080C16D0), the attribute part, in the binary's order: the points,
    // SetNpcPhysicsDamage (base) / SetNpcAttackRating / SetNpcDefence, the experience and the
    // next level's, life max from the role data, stamina max from the level tables, mana max,
    // the resistances of the level, the fixed speeds, then KNpc::Init (UpdataCurData).
    void load_from(KNpc& npc, const pb::RoleData& role, const KPlayerSet& tables, const KItemList* items);
    // the inverse for PlayerSave
    void save_to(const KNpc& npc, pb::RoleData& role) const;

    // 0x080A7EC0: m_PhysicsDamage = {min = max = m_nCurStrength / 5 + 1}, the elemental damages 0
    void set_npc_physics_damage_base(KNpc& npc) const noexcept;
    // 0x080A7F90 / 0x080A7FC0: m_AttackRating = m_nDexterity x 4 - 28; m_Defend = m_nDexterity / 4
    void set_npc_attack_rating(KNpc& npc) const noexcept;
    void set_npc_defence(KNpc& npc) const noexcept;
    // 0x080AB7C0: the five base resistances from level_add (GetXResist(series, level, reborn))
    void set_npc_resist(KNpc& npc, const KPlayerSet& tables) const noexcept;
    // 0x080AF740: the current physics damage from the weapon (KItemList::GetWeaponDamage) plus
    // m_nCurStrength / 5 for a melee weapon, m_nCurDexterity / 5 for a ranged one; bare hands
    // are m_nCurStrength / 5 + 1
    void set_npc_physics_damage(KNpc& npc, const KItemList* items) const noexcept;
    // 0x080AF550: KNpc::ClearAttrib, current points = base, ReCalcStateEffect, ReCalcEquip
    void updata_cur_data(KNpc& npc, bool clear_state, const KPlayerSet& tables, const KItemList* items);
    // KPlayer::CalcExp (0x080A7C80): what a kill is worth by the level difference.  Within five
    // levels all of it; 6..15 apart x (25 - |d|) / 20; farther x 1/2; a monster 55..69 levels
    // above gives (-19 d - 1030) / 300, farther above all of it; a character of level 100 and up
    // gets 1 from anything below level 90.  Never less than 1.
    [[nodiscard]] static int calc_exp(int exp, int player_level, int npc_level) noexcept;
    // KPlayer::AddExp (0x080B00C0) + its core (0x080AFEA0): nothing when dead or at level 200,
    // CalcExp, the percent bonuses, the random range bonus (rand(n) = 0..n-1), then exp += it up
    // to the next level's need, a level up when reached (the leftover is lost, as in the old
    // game).  Returns the levels gained (0 or 1).
    int add_exp(KNpc& npc, int exp, int npc_level, const KPlayerSet& tables, const KItemList* items, int (*rand)(void*, int), void* rand_ctx);
    // 0x080AF800: one level up (true) or down (false): exp 0, +-5 attribute points, +-1 skill
    // point, the level tables, UpdataCurData, life / mana / stamina filled.  Returns false when
    // the level cannot move (1 or 200).
    bool level_up(KNpc& npc, bool up, const KPlayerSet& tables, const KItemList* items);
    // 0x080B0B60 and its siblings: spend attribute points (check = refuse more than owned);
    // returns false when refused
    bool add_base_strength(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items);
    bool add_base_dexterity(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items);
    bool add_base_vitality(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items);
    bool add_base_energy(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items);
    // 0x080B0B40 / 0x080B0AF0 and the vitality / energy versions: the current point moves
    // (equipment, skills) and what depends on it follows
    void change_cur_strength(KNpc& npc, int n, const KItemList* items) noexcept;
    void change_cur_dexterity(KNpc& npc, int n, const KItemList* items) noexcept;
    void change_cur_vitality(KNpc& npc, int n, const KPlayerSet& tables) noexcept;
    void change_cur_energy(KNpc& npc, int n, const KPlayerSet& tables) noexcept;
};

} // namespace jx::zone
