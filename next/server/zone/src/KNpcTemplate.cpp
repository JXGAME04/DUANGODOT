#include "jx/zone/KNpcTemplate.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"

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

} // namespace jx::zone
