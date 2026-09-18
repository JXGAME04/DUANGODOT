#pragma once

// KPlayer of the old core - the part that turns a character's points, level and equipment into
// the numbers its KNpc fights with, ported from the JX2 server binary function by function
// (jx_linux_y, docs/LINUX-SERVER.md §10.2 / §10.4; offsets of KPlayer noted).  The zone keeps
// it inside the player's KNpc (Player[Npc.m_nPlayerIdx] of the old code, without the index).

#include <cstdint>

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
    bool loaded = false;       // LoadFrom ran (a player's npc; false for every other npc)
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
