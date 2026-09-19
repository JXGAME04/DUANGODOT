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
    if (const auto it = j.find("pk_rate"); it != j.end() && it->is_object()) {
        pk_rate_.rate = geti(*it, "rate", 20);
        pk_rate_.faction_pk_faction = geti(*it, "faction_pk_faction", 1);
        pk_rate_.killer_pk_faction = geti(*it, "killer_pk_faction", 1);
        pk_rate_.enmity_pk = geti(*it, "enmity_pk", 2);
        pk_rate_.be_killed = geti(*it, "be_killed", -1);
        pk_rate_.kill_partner_pk = geti(*it, "kill_partner_pk", 1);
        pk_rate_.level_distance = geti(*it, "level_distance", 25);
        pk_rate_.butcher_pk_exercise = geti(*it, "butcher_pk_exercise", 1);
        pk_rate_.not_sub_pk_exp_percent = geti(*it, "not_sub_pk_exp_percent", -50);
        pk_rate_.not_enmity_exp_percent = geti(*it, "not_enmity_exp_percent", -50);
        pk_rate_.not_fight_exp_percent = geti(*it, "not_fight_exp_percent", -80);
    }
    if (const auto it = j.find("pk_punish"); it != j.end() && it->is_object()) {
        pk_punish_.normal_pk_time_long = geti(*it, "normal_pk_time_long", 3240);
        if (const auto rows = it->find("rows"); rows != it->end() && rows->is_array()) {
            for (std::size_t k = 0; k < pk_punish_.rows.size() && k < rows->size(); ++k) {
                const auto& r = (*rows)[k];
                if (!r.is_object()) continue;
                KPKPunishRow& row = pk_punish_.rows[k];
                row.exp_permille = geti(r, "exp_permille", 1);
                row.money_permille = geti(r, "money_permille", 1);
                row.item_permille = geti(r, "item_permille", 1);
                row.equip_percent = geti(r, "equip_percent", 1);
                row.col8 = geti(r, "col8", -1);
                row.durability_percent = geti(r, "durability_percent", 0);
            }
        }
    }
    if (const auto it = j.find("lead_exp"); it != j.end() && it->is_array()) {
        for (std::size_t k = 0; k < lead_exp_.size() && k < it->size(); ++k) {
            const auto& r = (*it)[k];
            if (!r.is_object()) continue;
            lead_exp_[k].exp = r.value("exp", std::int64_t{0});
            lead_exp_[k].members = geti(r, "members", 1);
        }
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

int KPlayerSet::lead_members(int level) const noexcept
{
    if (level < 1 || level > kMaxLeadLevel) return 1;   // 0x080C4560: eax = 1 outside the table
    const int m = lead_exp_[static_cast<std::size_t>(level - 1)].members;
    return m > 0 ? m : 1;
}

std::int64_t KPlayerSet::lead_level_exp(int level) const noexcept
{
    if (level < 1 || level > kMaxLeadLevel) return 0;
    return lead_exp_[static_cast<std::size_t>(level - 1)].exp;
}

void KPlayerSet::set_lead_exp(int level, std::int64_t exp, int members) noexcept
{
    if (level < 1 || level > kMaxLeadLevel) return;
    lead_exp_[static_cast<std::size_t>(level - 1)] = KLeadExpRow{exp, members};
}

} // namespace jx::zone
