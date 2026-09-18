#pragma once

// KPlayerSet / KLevelAdd of the old core (Core/Src/KPlayerSet.h), the table half: what a level
// and an attribute point add, the experience of every level, the resistance per level, the
// stamina rules of stamina.ini and the frame counts of basevalue.ini.  Read from player.json
// (jxassets export-player, services/pkg/jxold/player) - the numbers of settings/npc/player of
// the JX2 server, loaded the way jx_linux_y loads them (docs/LINUX-SERVER.md §10.3).

#include <array>
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
    // the resist maxima a player starts with when the role data has none (KPlayer::LoadFrom: 0x4b)
    static constexpr int kDefaultResistMax = 75;

    // for tests: set the tables by hand
    void set_level_exp(int level, std::int64_t exp) noexcept;
    void set_level_add(int series, const KLevelAddRow& row) noexcept;
    void set_stamina(const KStaminaRule& r) noexcept { stamina_ = r; }

private:
    bool loaded_ = false;
    std::array<KLevelExpRow, kMaxLevel> level_exp_{};
    std::array<KLevelAddRow, kMaxSeries> level_add_{};
    KStaminaRule stamina_;
    KBaseValue base_value_;
};

} // namespace jx::zone
