#include "jx/zone/KPlayerSet.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>

namespace jx::zone {

using nlohmann::json;

namespace {
int geti(const json& j, const char* key, int def = 0)
{
    const auto it = j.find(key);
    return it != j.end() && it->is_number() ? it->get<int>() : def;
}
} // namespace

bool KPlayerSet::load(const std::string& file, std::string* error)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + file;
        return false;
    }
    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        if (error) *error = file + ": " + e.what();
        return false;
    }
    if (const auto it = j.find("level_exp"); it != j.end() && it->is_array()) {
        int i = 0;
        for (const auto& row : *it) {
            if (i >= kMaxLevel) break;
            auto& r = level_exp_[static_cast<std::size_t>(i)];
            r.exp = row.value("exp", std::int64_t{0});
            if (const auto rb = row.find("reborn"); rb != row.end() && rb->is_array()) {
                int k = 0;
                for (const auto& v : *rb) {
                    if (k >= kMaxReborn) break;
                    r.reborn[static_cast<std::size_t>(k++)] = v.is_number() ? v.get<std::int64_t>() : 0;
                }
            }
            ++i;
        }
    }
    if (const auto it = j.find("level_add"); it != j.end() && it->is_array()) {
        int i = 0;
        for (const auto& row : *it) {
            if (i >= kMaxSeries) break;
            KLevelAddRow& a = level_add_[static_cast<std::size_t>(i++)];
            a.life_per_level = geti(row, "life_per_level");
            a.stamina_male_per_level = geti(row, "stamina_male_per_level");
            a.stamina_female_per_level = geti(row, "stamina_female_per_level");
            a.mana_per_level = geti(row, "mana_per_level");
            a.life_per_vitality = geti(row, "life_per_vitality");
            a.stamina_per_vitality = geti(row, "stamina_per_vitality");
            a.mana_per_energy = geti(row, "mana_per_energy");
            a.lead_exp_share = geti(row, "lead_exp_share");
            a.fire_res = geti(row, "fire_res");
            a.cold_res = geti(row, "cold_res");
            a.poison_res = geti(row, "poison_res");
            a.lighting_res = geti(row, "lighting_res");
            a.physics_res = geti(row, "physics_res");
            a.stamina_male_base = geti(row, "stamina_male_base");
            a.stamina_female_base = geti(row, "stamina_female_base");
        }
    }
    if (const auto it = j.find("stamina"); it != j.end() && it->is_object()) {
        stamina_.normal_add = geti(*it, "normal_add", 1);
        stamina_.exercise_run_sub = geti(*it, "exercise_run_sub", 6);
        stamina_.fight_run_sub = geti(*it, "fight_run_sub", 6);
        stamina_.kill_run_sub = geti(*it, "kill_run_sub", 6);
        stamina_.sit_add = geti(*it, "sit_add", 3);
    }
    if (const auto it = j.find("basevalue"); it != j.end() && it->is_object()) {
        base_value_.hurt_frame = geti(*it, "hurt_frame", 12);
        base_value_.run_speed = geti(*it, "run_speed", 10);
        base_value_.walk_speed = geti(*it, "walk_speed", 5);
        base_value_.attack_frame = geti(*it, "attack_frame", 18);
        base_value_.cast_frame = geti(*it, "cast_frame", 18);
    }
    loaded_ = true;
    return true;
}

std::int64_t KPlayerSet::level_exp(int level, int reborn) const noexcept
{
    if (level < 1 || level > kMaxLevel || reborn < 0 || reborn > kMaxReborn) return -1;
    const auto& r = level_exp_[static_cast<std::size_t>(level - 1)];
    return reborn == 0 ? r.exp : r.reborn[static_cast<std::size_t>(reborn - 1)];
}

const KLevelAddRow& KPlayerSet::level_add(int series) const noexcept
{
    static const KLevelAddRow none;
    if (series < 0 || series >= kMaxSeries) return none;
    return level_add_[static_cast<std::size_t>(series)];
}

int KPlayerSet::stamina_base(int series, int sex, int level) const noexcept
{
    if (series < 0 || series >= kMaxSeries || level < 1 || level > kMaxLevel) return 0;
    const KLevelAddRow& a = level_add_[static_cast<std::size_t>(series)];
    return sex == 0 ? (level - 1) * a.stamina_male_per_level + a.stamina_male_base
                    : (level - 1) * a.stamina_female_per_level + a.stamina_female_base;
}

int KPlayerSet::resist(int per_level, int level, bool reborn, int floor) noexcept
{
    if (level < 1 || level > kMaxLevel) return 0;
    const int n = (level > 120 && per_level < 0) ? 120 : level;
    const int res = per_level * n / 100;
    return reborn ? std::max(res, floor) : res;
}

void KPlayerSet::set_level_exp(int level, std::int64_t exp) noexcept
{
    if (level < 1 || level > kMaxLevel) return;
    auto& r = level_exp_[static_cast<std::size_t>(level - 1)];
    r.exp = exp;
    r.reborn.fill(exp);
}

void KPlayerSet::set_level_add(int series, const KLevelAddRow& row) noexcept
{
    if (series < 0 || series >= kMaxSeries) return;
    level_add_[static_cast<std::size_t>(series)] = row;
}

} // namespace jx::zone
