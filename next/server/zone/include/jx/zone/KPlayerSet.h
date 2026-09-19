#pragma once

// KPlayerSet / KLevelAdd of the old core (Core/Src/KPlayerSet.h), the table half: what a level
// and an attribute point add, the experience of every level, the resistance per level, the
// stamina rules of stamina.ini and the frame counts of basevalue.ini.  Read from player.json
// (jxassets export-player, services/pkg/jxold/player) - the numbers of settings/npc/player of
// the JX2 server, loaded the way jx_linux_y loads them (docs/LINUX-SERVER.md §10.3).

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace jx::zone {

inline constexpr int kMaxLevel = 200;      // KLevelAdd of the JX2 server keeps 200 levels (JX1: 150)
inline constexpr int kMaxSeries = 5;
inline constexpr int kMaxReborn = 7;

// one row of level_add.txt (a series)
struct KLevelAddRow {
    int life_per_level = 0;
    int stamina_male_per_level = 0;
    int stamina_female_per_level = 0;
    int mana_per_level = 0;
    int life_per_vitality = 0;
    int stamina_per_vitality = 0;
    int mana_per_energy = 0;
    int lead_exp_share = 0;
    int fire_res = 0;        // per level, x level / 100
    int cold_res = 0;
    int poison_res = 0;
    int lighting_res = 0;
    int physics_res = 0;
    int stamina_male_base = 0;
    int stamina_female_base = 0;
};

struct KLevelExpRow {
    std::int64_t exp = 0;                       // column 2 + 10 000 x column 3
    std::array<std::int64_t, kMaxReborn> reborn{};   // the 1..7 转 tables
};

// [stamina] of stamina.ini (KPlayerSet+0x14bc..)
struct KStaminaRule {
    int normal_add = 1;
    int exercise_run_sub = 6;
    int fight_run_sub = 6;
    int kill_run_sub = 6;
    int sit_add = 3;   // per mille of the maximum, every state tick while sitting
};

// [PK] of \settings\npc\PKRate.ini as KNpcSet::Init 0x080A08F6.. reads it (KNpcSet+0x1490..; the binary's defaults when a key
// is missing): the damage percent between players, the PK points a kill adds (KNpc::GetPKRelation 0x0807A350), the exp-percent
// gates of the PK state (0x080DBE00) and of the death penalty (0x080B9FA0)
struct KPKRate {
    int rate = 20;                    // +0x1490 [0x8badf50]
    int faction_pk_faction = 1;       // +0x1494 [0x8badf54]
    int killer_pk_faction = 1;        // +0x1498 [0x8badf58]
    int enmity_pk = 2;                // +0x149c [0x8badf5c]
    int be_killed = -1;               // +0x14a0 [0x8badf60]: added to the victim of a PK death
    int kill_partner_pk = 1;          // +0x14a4 [0x8badf64]: a killed companion (kind 2)
    int level_distance = 25;          // +0x14a8 [0x8badf68]
    int butcher_pk_exercise = 1;      // +0x14ac [0x8badf6c]: a normal-mode player killing a kill-mode one
    int not_sub_pk_exp_percent = -50; // +0x14b0 [0x8badf70]: below this percent of the level's exp no BeKilled is added
    int not_enmity_exp_percent = -50; // +0x14b4 [0x8badf74]
    int not_fight_exp_percent = -80;  // +0x14b8 [0x8badf78]: below it a PK state cannot be entered (0x080DBEF3) and a PK death drops the state (0x080BA0D2)
};

// one row of \settings\npc\player\PKPunish.txt (KPlayerSet+0x3710 + 24 x pk, 0x080C5B45..): the PK death penalty of a
// PK value 0..10 (KNpc::DeathPunish 0x080B9FA0)
struct KPKPunishRow {
    int exp_permille = 1;        // col 2: of the level's exp (levels up to 129)
    int money_permille = 1;      // col 3: of the money carried
    int item_permille = 1;       // col 4: chance per bag item to fall (0x08203BE0)
    int equip_percent = 1;       // col 5 (2004: the chance to lose a worn piece; not read by 0x080B9FA0)
    int col8 = -1;               // col 8 (default -1; unread so far)
    int durability_percent = 0;  // col 9: durability off every worn piece (0x08201D90)
};

struct KPKPunish {
    static constexpr int kRows = 11;
    std::array<KPKPunishRow, kRows> rows{};
    int normal_pk_time_long = 3240;   // row 2 col 7 [0x8bb2b38]: seconds a PK state must be held before the switch back is accepted
    [[nodiscard]] const KPKPunishRow& row(int pk) const noexcept { return rows[static_cast<std::size_t>(std::clamp(pk, 0, kRows - 1))]; }
};

// [Common] of basevalue.ini
struct KBaseValue {
    int hurt_frame = 12;
    int run_speed = 10;
    int walk_speed = 5;
    int attack_frame = 18;
    int cast_frame = 18;
};

class KPlayerSet {
public:
    // Loads player.json; false (with *error) when the file cannot be read.  Without a file the
    // set keeps its defaults: level 2 needs 100, nothing per level, 1..200 all the same.
    bool load(const std::string& file, std::string* error);
    [[nodiscard]] bool loaded() const noexcept { return loaded_; }

    // KLevelAdd::GetLevelExp(level, reborn) (0x080C3FF0): -1 outside 1..200 or reborn > 7
    [[nodiscard]] std::int64_t level_exp(int level, int reborn = 0) const noexcept;
    [[nodiscard]] const KLevelAddRow& level_add(int series) const noexcept;
    [[nodiscard]] int life_per_level(int series) const noexcept { return level_add(series).life_per_level; }
    [[nodiscard]] int stamina_per_level(int series, int sex) const noexcept
    {
        return sex == 0 ? level_add(series).stamina_male_per_level : level_add(series).stamina_female_per_level;
    }
    [[nodiscard]] int mana_per_level(int series) const noexcept { return level_add(series).mana_per_level; }
    [[nodiscard]] int life_per_vitality(int series) const noexcept { return level_add(series).life_per_vitality; }
    [[nodiscard]] int stamina_per_vitality(int series) const noexcept { return level_add(series).stamina_per_vitality; }
    [[nodiscard]] int mana_per_energy(int series) const noexcept { return level_add(series).mana_per_energy; }
    // GetStaminaBase (0x080C4120): (level - 1) x StaminaPerLevel(sex) + StaminaBase(sex); 0 out of range
    [[nodiscard]] int stamina_base(int series, int sex, int level) const noexcept;
    // GetFireResist.. (0x080C4220 ..): per level x n / 100 with n held at 120 above level 120 for a
    // NEGATIVE per-level value; a reborn character gets at least resist_floor
    [[nodiscard]] int fire_resist(int series, int level, bool reborn) const noexcept { return resist(level_add(series).fire_res, level, reborn); }
    [[nodiscard]] int cold_resist(int series, int level, bool reborn) const noexcept { return resist(level_add(series).cold_res, level, reborn); }
    [[nodiscard]] int poison_resist(int series, int level, bool reborn) const noexcept { return resist(level_add(series).poison_res, level, reborn); }
    [[nodiscard]] int light_resist(int series, int level, bool reborn) const noexcept { return resist(level_add(series).lighting_res, level, reborn); }
    [[nodiscard]] int physics_resist(int series, int level, bool reborn) const noexcept { return resist(level_add(series).physics_res, level, reborn); }
    [[nodiscard]] static int resist(int per_level, int level, bool reborn, int floor = 0) noexcept;

    [[nodiscard]] const KStaminaRule& stamina() const noexcept { return stamina_; }
    [[nodiscard]] const KBaseValue& base_value() const noexcept { return base_value_; }
    [[nodiscard]] const KPKRate& pk_rate() const noexcept { return pk_rate_; }
    [[nodiscard]] const KPKPunish& pk_punish() const noexcept { return pk_punish_; }
    // the resist maxima a player starts with when the role data has none (KPlayer::LoadFrom: 0x4b)
    static constexpr int kDefaultResistMax = 75;

    // for tests: set the tables by hand
    void set_level_exp(int level, std::int64_t exp) noexcept;
    void set_level_add(int series, const KLevelAddRow& row) noexcept;
    void set_stamina(const KStaminaRule& r) noexcept { stamina_ = r; }
    void set_pk_rate(const KPKRate& r) noexcept { pk_rate_ = r; }
    void set_pk_punish(const KPKPunish& p) noexcept { pk_punish_ = p; }

private:
    bool loaded_ = false;
    std::array<KLevelExpRow, kMaxLevel> level_exp_{};
    std::array<KLevelAddRow, kMaxSeries> level_add_{};
    KStaminaRule stamina_;
    KBaseValue base_value_;
    KPKRate pk_rate_;
    KPKPunish pk_punish_;
};

} // namespace jx::zone
