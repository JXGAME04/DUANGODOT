#include "jx/zone/KNpcGold.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "jx/zone/KNpc.h"

namespace jx::zone {

namespace {

// x * percent / 100 the way the binary does it (imul, then the 0x51eb851f magic division: a signed / 100)
int percent(int x, int p) noexcept
{
    return static_cast<int>((static_cast<std::int64_t>(x) * p) / 100);
}

KNpcGoldTemplate row_of(const nlohmann::json& r)
{
    KNpcGoldTemplate t;
    t.name = r.value("name", "");
    t.exp = r.value("exp", 1);
    t.life = r.value("life", 1);
    t.life_replenish = r.value("life_replenish", 1);
    t.attack_rating = r.value("attack_rating", 1);
    t.defense = r.value("defense", 1);
    t.min_damage = r.value("min_damage", 1);
    t.max_damage = r.value("max_damage", 1);
    t.treasure = r.value("treasure", 0);
    t.walk_speed = r.value("walk_speed", 0);
    t.run_speed = r.value("run_speed", 0);
    t.attack_speed = r.value("attack_speed", 0);
    t.cast_speed = r.value("cast_speed", 0);
    t.skill_id = r.value("skill_id", 0);
    t.skill_level = r.value("skill_level", "");
    t.fire_resist = r.value("fire_resist", 0);
    t.fire_resist_max = r.value("fire_resist_max", 0);
    t.cold_resist = r.value("cold_resist", 0);
    t.cold_resist_max = r.value("cold_resist_max", 0);
    t.light_resist = r.value("light_resist", 0);
    t.light_resist_max = r.value("light_resist_max", 0);
    t.poison_resist = r.value("poison_resist", 0);
    t.poison_resist_max = r.value("poison_resist_max", 0);
    t.physics_resist = r.value("physics_resist", 0);
    t.physics_resist_max = r.value("physics_resist_max", 0);
    t.ai_mode = r.value("ai_mode", 0);
    if (r.contains("ai_params") && r["ai_params"].is_array()) {
        std::size_t i = 0;
        for (const auto& v : r["ai_params"]) {
            if (i >= t.ai_params.size()) break;
            t.ai_params[i++] = v.get<int>();
        }
    }
    t.ai_max_time = r.value("ai_max_time", 100);
    if (t.life <= 0) t.life = 1;   // 0x0809D432: "GoldTemplate:%d settings is error! life <= 0"
    return t;
}

} // namespace

std::optional<KNpcGoldTemplateSet> KNpcGoldTemplateSet::load(const std::filesystem::path& file, std::string* error)
{
    std::ifstream in(file);
    if (!in) {
        if (error) *error = "cannot open " + file.string();
        return std::nullopt;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return parse(ss.str(), error);
}

std::optional<KNpcGoldTemplateSet> KNpcGoldTemplateSet::parse(const std::string& json, std::string* error)
{
    KNpcGoldTemplateSet set;
    try {
        const nlohmann::json j = nlohmann::json::parse(json);
        const nlohmann::json rows = j.value("rows", nlohmann::json::array());
        for (const auto& r : rows) {
            if (set.rows.size() >= 30) break;   // the arrays of both loaders
            set.rows.push_back(row_of(r));
        }
        set.client_rows = j.value("client_rows", 0);
    } catch (const std::exception& e) {
        if (error) *error = std::string("npc_gold.json: ") + e.what();
        return std::nullopt;
    }
    return set;
}

void gold_back_data(KNpc& e)
{
    KNpcGold& g = e.gold;
    g.is_gold = true;
    g.is_golding = false;
    g.gold_type = 0;
    g.fire_resist = e.cur.fire_resist_v();   // 0x0809D597: max(+0x19ec, +0x19f0)
    g.fire_resist_max = e.cur.fire_resist_max;
    g.cold_resist = e.cur.cold_resist_v();
    g.cold_resist_max = e.cur.cold_resist_max;
    g.light_resist = e.cur.light_resist_v();
    g.light_resist_max = e.cur.light_resist_max;
    g.poison_resist = e.cur.poison_resist_v();
    g.poison_resist_max = e.cur.poison_resist_max;
    g.physics_resist = e.cur.physics_resist_v();
    g.physics_resist_max = e.cur.physics_resist_max;
    g.damage = {e.cur.physics_damage, e.cur.fire_damage, e.cur.cold_damage, e.cur.light_damage, e.cur.poison_damage,
                e.cur.physics_magic, e.cur.cold_magic, e.cur.light_magic, e.cur.fire_magic, e.cur.poison_magic};
    g.experience = e.base.experience;   // +0x15a8: the base experience is what the gold change multiplies
    g.life_max = e.cur.life_max_v();    // 0x0809D815: max(+0x1a14, +0x1a18)
    g.life_replenish = e.cur.life_replenish;
    g.attack_rating = e.cur.attack_rating;
    g.defend = e.cur.defend;
    g.treasure = e.cur.treasure;
    g.ai_mode = e.ai_mode;
    for (std::size_t i = 0; i < g.ai_params.size(); ++i) g.ai_params[i] = e.ai_param[i];
    g.drop_rate_file = e.drop_rate_file;
    g.ai_max_time = static_cast<int>(e.ai_max_time & 0xff);   // the byte +0x14c0
}

void gold_apply(KNpc& e, const KNpcGoldTemplate& t)
{
    KNpcCurrentAttrib& c = e.cur;
    // 0x0809D95A .. 0x0809DB2E: the resists in use through the row's percent twice, their maximums once
    c.fire_resist = percent(percent(c.fire_resist, t.fire_resist), t.fire_resist);
    c.fire_resist_max = percent(c.fire_resist_max, t.fire_resist_max);
    c.cold_resist = percent(percent(c.cold_resist, t.cold_resist), t.cold_resist);
    c.cold_resist_max = percent(c.cold_resist_max, t.cold_resist_max);
    c.light_resist = percent(percent(c.light_resist, t.light_resist), t.light_resist);
    c.light_resist_max = percent(c.light_resist_max, t.light_resist_max);
    c.poison_resist = percent(percent(c.poison_resist, t.poison_resist), t.poison_resist);
    c.poison_resist_max = percent(c.poison_resist_max, t.poison_resist_max);
    c.physics_resist = percent(percent(c.physics_resist, t.physics_resist), t.physics_resist);
    c.physics_resist_max = percent(c.physics_resist_max, t.physics_resist_max);
    // 0x0809DB4E ..: the experience, both life maximums, the replenish, the attack rating, the defence
    e.base.experience = percent(e.base.experience, t.exp);
    c.life_max_yan = percent(c.life_max_yan, t.life);
    c.life_max = percent(c.life_max, t.life);
    c.life_replenish = percent(c.life_replenish, t.life_replenish);
    c.attack_rating = percent(c.attack_rating, t.attack_rating);
    c.defend = percent(c.defend, t.defense);
    // the damage blocks: min by MinDamage, max by MaxDamage (0x0809DC01 .. 0x0809DDF9)
    const auto scale = [&](KMagicAttrib& a) {
        a.value[0] = percent(a.value[0], t.min_damage);
        a.value[2] = percent(a.value[2], t.max_damage);
    };
    scale(c.physics_damage);
    scale(c.physics_magic);
    scale(c.cold_damage);
    scale(c.cold_magic);
    scale(c.light_damage);
    scale(c.light_magic);
    scale(c.fire_damage);
    scale(c.fire_magic);
    // a poison in use: its time 60, its interval 10 and the damage by MaxDamage (0x0809DDF3 .. 0x0809DE60)
    if (c.poison_damage.value[0] > 0) {
        c.poison_damage.value[1] = 60;
        c.poison_damage.value[2] = 10;
        c.poison_damage.value[0] = percent(c.poison_damage.value[0], t.max_damage);
    }
    if (c.poison_magic.value[0] > 0) {
        c.poison_magic.value[1] = 60;
        c.poison_magic.value[2] = 10;
        c.poison_magic.value[0] = percent(c.poison_magic.value[0], t.max_damage);
    }
    // 0x0809DE66 ..: the treasure replaced, the speeds added, the life refilled, the ai replaced
    c.treasure = t.treasure;
    c.walk_speed += t.walk_speed;
    c.run_speed += t.run_speed;
    c.attack_speed_yan += t.attack_speed;
    c.attack_speed += t.attack_speed;
    c.cast_speed_yan += t.cast_speed;
    c.cast_speed += t.cast_speed;
    c.life = c.life_max_v();
    e.ai_mode = t.ai_mode;
    for (std::size_t i = 0; i < t.ai_params.size(); ++i) e.ai_param[i] = t.ai_params[i];
    e.ai_max_time = static_cast<std::uint32_t>(t.ai_max_time & 0xff);   // the byte +0x14c0
}

bool gold_recover(KNpc& e, const KNpcGoldTemplate* t)
{
    KNpcGold& g = e.gold;
    if (!g.is_gold || !g.is_golding) return false;
    g.is_golding = false;
    KNpcCurrentAttrib& c = e.cur;
    c.fire_resist = g.fire_resist;
    c.fire_resist_max = g.fire_resist_max;
    c.cold_resist = g.cold_resist;
    c.cold_resist_max = g.cold_resist_max;
    c.light_resist = g.light_resist;
    c.light_resist_max = g.light_resist_max;
    c.poison_resist = g.poison_resist;
    c.poison_resist_max = g.poison_resist_max;
    c.physics_resist = g.physics_resist;
    c.physics_resist_max = g.physics_resist_max;
    // (0x0809E105 .. 0x0809E128: cell 5 taken out and the aura cleared - the world does that)
    e.drop_rate_file = g.drop_rate_file;
    e.ai_mode = g.ai_mode;
    e.ai_max_time = static_cast<std::uint32_t>(g.ai_max_time & 0xff);
    for (std::size_t i = 0; i < g.ai_params.size(); ++i) e.ai_param[i] = g.ai_params[i];
    if (t == nullptr) return true;   // 0x0809E190: a type outside the table leaves the numbers as they are
    e.base.experience = g.experience;
    c.life_max = g.life_max;
    c.life_max_yan = g.life_max;
    c.life_replenish = g.life_replenish;
    c.attack_rating = g.attack_rating;
    c.defend = g.defend;
    c.physics_damage = g.damage[0];
    c.fire_damage = g.damage[1];
    c.cold_damage = g.damage[2];
    c.light_damage = g.damage[3];
    c.poison_damage = g.damage[4];
    c.physics_magic = g.damage[5];
    c.cold_magic = g.damage[6];
    c.light_magic = g.damage[7];
    c.fire_magic = g.damage[8];
    c.poison_magic = g.damage[9];
    c.treasure = g.treasure;
    c.walk_speed -= t->walk_speed;
    c.run_speed -= t->run_speed;
    c.attack_speed_yan -= t->attack_speed;
    c.attack_speed -= t->attack_speed;
    c.cast_speed_yan -= t->cast_speed;
    c.cast_speed -= t->cast_speed;
    c.life = g.life_max;   // 0x0809E40E
    return true;
}

int npc_class(const KNpc& e) noexcept
{
    if (e.kind == KNpcKind::player) return 0;
    if (e.boss_flag != 0) return 3;
    return e.gold.gold_kind() >= 1 ? 2 : 1;
}

} // namespace jx::zone
