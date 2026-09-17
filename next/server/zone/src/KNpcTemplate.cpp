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
            tpl.series = t.value("series", 0);
            tpl.stature = t.value("stature", 0);
            tpl.stand_frame = t.value("stand_frame", 15u);
            tpl.stand_frame1 = t.value("stand_frame1", 15u);
            tpl.walk_frame = t.value("walk_frame", 15u);
            tpl.run_frame = t.value("run_frame", 15u);
            tpl.attack_frame = t.value("attack_frame", 20u);
            tpl.hurt_frame = t.value("hurt_frame", 10u);
            tpl.death_frame = t.value("death_frame", 12u);
            tpl.hit_recover = t.value("hit_recover", 0u);
            tpl.revive_frame = t.value("revive_frame", 2400u);
            tpl.life_param = t.value("life_param", 1u);
            tpl.min_damage = t.value("min_damage", 1u);
            tpl.max_damage = t.value("max_damage", 3u);
            tpl.defense = t.value("defense", 0u);
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
