#include "jx/zone/KNpcTemplate.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"
#include "jx/zone/KScriptCache.h"

namespace jx::zone {

std::optional<KNpcTemplateSet> KNpcTemplateSet::load(const std::string& file, std::string* error)
{
    auto fail = [&](std::string why) -> std::optional<KNpcTemplateSet> {
        if (error) *error = std::move(why);
        return std::nullopt;
    };
    std::ifstream in(file);
    if (!in) return fail("cannot open " + file);
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        return fail(std::string("npcs.json: ") + e.what());
    }
    KNpcTemplateSet set;
    try {
        // a named copy: items() keeps a reference to it (a temporary would die under the loop)
        const nlohmann::json templates = j.value("templates", nlohmann::json::object());
        for (const auto& [key, t] : templates.items()) {
            KNpcTemplate tpl;
            tpl.id = static_cast<std::uint32_t>(std::stoul(key));
            tpl.name = t.value("name", "");
            tpl.kind = t.value("kind", 0);
            tpl.camp = t.value("camp", 4);
            tpl.series = t.value("series", 0);
            tpl.stature = t.value("stature", 0);
            tpl.stand_frame = t.value("stand_frame", 15u);
            tpl.stand_frame1 = t.value("stand_frame1", 15u);
            tpl.walk_frame = t.value("walk_frame", 15u);
            tpl.run_frame = t.value("run_frame", 15u);
            tpl.attack_frame = t.value("attack_frame", 20u);
            tpl.cast_frame = t.value("cast_frame", 20u);
            tpl.hurt_frame = t.value("hurt_frame", 10u);
            tpl.death_frame = t.value("death_frame", 12u);
            tpl.hit_recover = t.value("hit_recover", 0u);
            tpl.revive_frame = t.value("revive_frame", 2400u);
            tpl.life_param = t.value("life_param", 1u);
            tpl.min_damage = t.value("min_damage", 1u);
            tpl.max_damage = t.value("max_damage", 3u);
            tpl.defense = t.value("defense", 0u);
            tpl.walk_speed = t.value("walk_speed", 5);
            tpl.run_speed = t.value("run_speed", 10);
            tpl.ai_mode = t.value("ai_mode", 0);
            if (const auto ap = t.find("ai_param"); ap != t.end() && ap->is_array()) {
                int i = 0;
                for (const auto& v : *ap) {
                    if (i >= 10) break;
                    tpl.ai_param[i++] = v.is_number() ? v.get<int>() : 0;
                }
            }
            tpl.ai_max_time = t.value("ai_max_time", 25u);
            tpl.vision_radius = t.value("vision_radius", 40);
            tpl.active_radius = t.value("active_radius", 30);
            if (const auto sk = t.find("skills"); sk != t.end() && sk->is_array()) {
                int slot = 1;
                for (const auto& s : *sk) {
                    if (slot >= 5) break;
                    KNpcTemplateSkill& d = tpl.skills[slot++];
                    if (!s.is_object()) continue;
                    d.id = s.value("id", 0);
                    d.level_a = s.value("level_a", 0.0);
                    d.level_b = s.value("level_b", 0.0);
                    d.known = s.value("known", false);
                    d.attack_radius = s.value("attack_radius", 0);
                    d.melee = s.value("melee", false);
                    d.target_self = s.value("target_self", false);
                    d.style = s.value("style", 0);
                }
            }
            tpl.level_script = t.value("level_script", "");
            if (const auto c = t.find("cells"); c != t.end() && c->is_object()) {
                for (const auto& [col, v] : c->items()) {
                    if (v.is_string()) tpl.cells[col] = v.get<std::string>();
                }
            }
            set.templates_[tpl.id] = std::move(tpl);
        }
    } catch (const std::exception& e) {
        return fail(std::string("npcs.json fields: ") + e.what());
    }
    log::info("npc", "npc templates loaded", {log::kv("file", file), log::kv("count", set.templates_.size())});
    return set;
}

const KNpcTemplate* KNpcTemplateSet::find(std::uint32_t id) const
{
    const auto it = templates_.find(id);
    return it == templates_.end() ? nullptr : &it->second;
}

namespace {

// KTabFile::GetFloat: atof of the cell, the default for an empty cell.
double cell_float(const KNpcTemplate& t, const std::string& column, double def)
{
    const std::string s = t.cell(column);
    if (s.empty()) return def;
    return std::strtod(s.c_str(), nullptr);
}

std::uint32_t to_u32(double v) noexcept
{
    return v <= 0 ? 0u : static_cast<std::uint32_t>(v);   // the old int members truncate
}

// GetNpcLevelDataFromScript(pScript, nSeries, name, nLevel, p1, p2, p3): GetNpcKeyData
int key_data(KLuaScript& s, int series, int level, const char* name, double p1, double p2, double p3)
{
    const auto v = s.call_number("GetNpcKeyData", {static_cast<double>(series), static_cast<double>(level), std::string(name), p1, p2, p3});
    return v ? static_cast<int>(*v) : 0;
}

// GetNpcLevelDataFromScript(pScript, nSeries, name, nLevel, szParam): GetNpcLevelData; an empty
// cell is 0 without a call.
int level_data_str(KLuaScript& s, int series, int level, const char* name, const std::string& cell)
{
    if (cell.empty()) return 0;
    const auto v = s.call_number("GetNpcLevelData", {static_cast<double>(series), static_cast<double>(level), std::string(name), cell});
    return v ? static_cast<int>(*v) : 0;
}

// nParam * GetNpcKeyData(...) / 100.0 for one stat group (ExpParam, ExpParam1..3 ...)
std::uint32_t percent_of(KLuaScript& s, const KNpcTemplate& t, int series, int level, const char* group, const char* name)
{
    const std::string g(group);
    const double param = cell_float(t, g, 1);
    const double p1 = cell_float(t, g + "1", 0);
    const double p2 = cell_float(t, g + "2", 0);
    const double p3 = cell_float(t, g + "3", 0);
    return to_u32(param * key_data(s, series, level, name, p1, p2, p3) / 100.0);
}

} // namespace

KNpcLevelData KNpcTemplateSet::level_data(const KNpcTemplate& t, int level, int series, KScriptCache* scripts)
{
    KNpcLevelData d;
    level = std::max(1, level);
    // placeholders: what the zone simulated with before the level scripts
    d.life_max = std::max(10u, std::max(1u, t.life_param) * static_cast<std::uint32_t>(level));
    d.min_damage = std::max(1u, t.min_damage);
    d.max_damage = std::max(d.min_damage, t.max_damage);
    d.attack_rating = 100;
    d.defend = t.defense;
    for (int slot = 1; slot < 5; ++slot) {
        if (t.skills[slot].id > 0) {
            d.skill_level[slot] = static_cast<int>(std::floor(t.skills[slot].level_a + t.skills[slot].level_b * level));
        }
    }
    if (scripts == nullptr) return d;
    KLuaScript* s = t.level_script.empty() ? nullptr : scripts->get(t.level_script);
    if (s == nullptr) s = scripts->get(KScriptCache::kNpcLevelScript);   // g_pNpcLevelScript
    if (s == nullptr) return d;

    // KNpcTemplate::InitNpcLevelData, in its order
    for (int slot = 1; slot < 5; ++slot) {
        const std::string name = "Level" + std::to_string(slot);
        const std::string cell = t.cell(name);
        if (t.skills[slot].id > 0 && !cell.empty()) {
            d.skill_level[slot] = level_data_str(*s, series, level, name.c_str(), cell);
        } else {
            d.skill_level[slot] = 0;
        }
    }
    d.exp = percent_of(*s, t, series, level, "ExpParam", "Exp");
    d.life_max = percent_of(*s, t, series, level, "LifeParam", "Life");
    if (d.life_max == 0) d.life_max = 100;
    d.life_replenish = level_data_str(*s, series, level, "LifeReplenish", t.cell("LifeReplenish"));
    d.attack_rating = percent_of(*s, t, series, level, "ARParam", "AR");   // "AR", not "AttackRating": most scripts fall through to the quadratic
    if (d.attack_rating == 0) d.attack_rating = 100;
    d.defend = percent_of(*s, t, series, level, "DefenseParam", "Defense");
    d.min_damage = percent_of(*s, t, series, level, "MinDamageParam", "MinDamage");
    d.max_damage = percent_of(*s, t, series, level, "MaxDamageParam", "MaxDamage");
    d.fire_resist = level_data_str(*s, series, level, "FireResist", t.cell("FireResist"));
    d.cold_resist = level_data_str(*s, series, level, "ColdResist", t.cell("ColdResist"));
    d.light_resist = level_data_str(*s, series, level, "LightResist", t.cell("LightResist"));
    d.poison_resist = level_data_str(*s, series, level, "PoisonResist", t.cell("PoisonResist"));
    d.physics_resist = level_data_str(*s, series, level, "PhysicsResist", t.cell("PhysicsResist"));
    d.from_script = true;
    return d;
}

} // namespace jx::zone
