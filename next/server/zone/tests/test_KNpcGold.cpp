// The gold (elite) monsters of the JX2 server (jx_linux_y: KNpcGoldTemplate::Init 0x0809CCC0, KNpcGold::BackData
// 0x0809D560, SetGoldTypeAndBackData 0x0809D8D0, RecoverBackData 0x0809E070, the revive roll 0x0808600D, the death
// frame end 0x08083720, KNpcSet::Add 0x0809FBD0 for a Region_S.dat placement, the maplist keys 0x080F1416) -
// docs/LINUX-SERVER.md §16.12.  Every number here is worked out by hand from the binary.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jx/log.hpp"
#include "jx/client.pb.h"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcGold.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSkillList.h"
#include "jx/zone/KSubWorld.h"

using namespace jx::zone;
using jx::EntityId;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::error;
        jx::log::init(o);
    }
};

// two rows: 1 "Kim" (the file's row 7: every stat doubled, the resists quartered, no skill), 2 "Lay bao vat" (the file's
// row 16: ten times the life, the aura 1103 at "0|1", the speeds +1)
constexpr const char* kGoldJson = R"({"rows": [
  {"name": "Kim", "exp": 100, "life": 200, "life_replenish": 20, "attack_rating": 200, "defense": 150, "min_damage": 100, "max_damage": 100,
   "treasure": 16, "walk_speed": 0, "run_speed": 0, "attack_speed": 0, "cast_speed": 0, "skill_id": 0, "skill_level": "",
   "fire_resist": 50, "fire_resist_max": 100, "cold_resist": 50, "cold_resist_max": 100, "light_resist": 50, "light_resist_max": 100,
   "poison_resist": 100, "poison_resist_max": 100, "physics_resist": 50, "physics_resist_max": 100,
   "ai_mode": 2, "ai_params": [80, 25, 15, 100, 25, 25, 25, 20, 50, 0], "ai_max_time": 18},
  {"name": "Lay bao vat", "exp": 100, "life": 1000, "life_replenish": 100, "attack_rating": 150, "defense": 200, "min_damage": 150, "max_damage": 150,
   "treasure": 4, "walk_speed": 1, "run_speed": 1, "attack_speed": 2, "cast_speed": 3, "skill_id": 1103, "skill_level": "0|1",
   "fire_resist": 75, "fire_resist_max": 95, "cold_resist": 75, "cold_resist_max": 95, "light_resist": 75, "light_resist_max": 95,
   "poison_resist": 75, "poison_resist_max": 95, "physics_resist": 75, "physics_resist_max": 95,
   "ai_mode": 2, "ai_params": [80, 25, 15, 100, 25, 25, 25, 20, 50, 0], "ai_max_time": 6}
], "client_rows": 17})";

// the level script of the skills: the buff 1102 puts +10 defence on for 60 frames
constexpr const char* kLevelScript = R"lua(
function GetSkillLevelData(levelname, data, level)
    if data == "buff" and levelname == "armordefense_v" then return "10,60,0" end
    return ""
end
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_gold_test";
    std::filesystem::create_directories(root / "script" / "skill");
    std::ofstream(root / "script" / "skill" / "gold.lua") << kLevelScript;
    return root.generic_string();
}

// 1102 a buff on oneself, 1103 an aura whose child is 1102 (the same shape as the command test's table)
std::shared_ptr<const KSkillTable> skill_table()
{
    KSkillTable t;
    int row = 2;
    auto add = [&](int id, std::vector<std::pair<std::string, std::string>> extra) {
        std::unordered_map<std::string, std::string> cells{{"SkillId", std::to_string(id)}, {"SkillStyle", "2"}, {"Series", "-1"}, {"DoHurt", "0"},
                                                           {"IsPhysical", "1"}, {"TargetEnemy", "1"}, {"AttackRadius", "100"}, {"EqtLimit", "-2"},
                                                           {"LvlSetScript", R"(\script\skill\gold.lua)"}, {"MaxLevel", "20"}, {"ReqLevel", "1"}, {"CharAnimId", "9"}};
        for (auto& [k, v] : extra) cells[k] = v;
        KSkillRow r = KSkillRow::from_cells(cells);
        r.row = row++;
        r.max_level = 20;
        t.add(r);
    };
    add(1, {{"WeaponSkill", "1"}});
    add(1102, {{"TargetEnemy", "0"}, {"TargetSelf", "1"}, {"PeaceCanUse", "1"}, {"IsPhysical", "0"}, {"LvlSetting1", "armordefense_v"}, {"LvlData1", "buff"}});
    add(1103, {{"IsAura", "1"}, {"ChildSkillId", "1102"}, {"StateSpecialId", "45"}, {"StatePriority", "2"}});
    return std::make_shared<const KSkillTable>(std::move(t));
}

// template 960: a beast worth 3 drop rolls on its own table \t.ini
std::shared_ptr<const KNpcTemplateSet> templates()
{
    KNpcTemplateSet set;
    KNpcTemplate t;
    t.id = 960;
    t.name = "beast";
    t.kind = 0;
    t.camp = 5;
    t.life_param = 40;   // life 40 x level without a level script
    t.min_damage = 7;
    t.max_damage = 11;
    t.defense = 12;
    t.treasure = 3;
    t.walk_speed = 5;
    t.ai_mode = 4;
    t.ai_param[0] = 9;
    t.ai_max_time = 25;
    t.death_frame = 4;
    t.revive_frame = 6;
    t.drop_rate_file = R"(\t.ini)";
    set.add(t);
    set.add_drop_rate(R"(\t.ini)", KNpcDropRate{});
    set.add_drop_rate(R"(\n.ini)", KNpcDropRate{});
    set.add_drop_rate(R"(\g.ini)", KNpcDropRate{});
    return std::make_shared<const KNpcTemplateSet>(std::move(set));
}

std::shared_ptr<const KNpcGoldTemplateSet> gold_table()
{
    std::string error;
    auto g = KNpcGoldTemplateSet::parse(kGoldJson, &error);
    REQUIRE(g.has_value());
    return std::make_shared<const KNpcGoldTemplateSet>(std::move(*g));
}

// a 4096 x 4096 map with the placements asked for and the maplist keys of a gold map
KSubWorldConfig gold_world(std::vector<KNpcPlacement> npcs, int auto_golden, int golden_type)
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.cell_size = 512;
    c.default_speed = 200;
    c.seed = 7;
    c.skills = skill_table();
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    c.templates = templates();
    c.gold = gold_table();
    KMapData m = KMapData::synthetic(128, 128);
    m.id = 1;
    m.spawn = Pos{2000, 2000};
    m.npcs = std::move(npcs);
    m.settings.auto_golden_npc = auto_golden;
    m.settings.golden_type = golden_type;
    m.settings.golden_drop_rate = R"(\g.ini)";
    m.settings.normal_drop_rate = R"(\n.ini)";
    c.map = std::make_shared<const KMapData>(std::move(m));
    c.map_npcs = true;
    return c;
}

KNpcPlacement beast_at(Pos p, bool special = false)
{
    KNpcPlacement n;
    n.template_id = 960;
    n.name = "beast";
    n.pos = p;
    n.kind = 0;
    n.level = 5;
    n.camp = 5;
    n.series = 0;
    n.special = special;
    return n;
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_stats()->set_strength(100);
    r.mutable_stats()->set_dexterity(100);
    r.mutable_stats()->set_vitality(50);
    r.mutable_stats()->set_energy(10);
    r.mutable_stats()->set_hp_max(500);
    r.mutable_stats()->set_hp(500);
    r.mutable_stats()->set_mp_max(50);
    r.mutable_stats()->set_mp(50);
    r.set_fight_mode(true);
    r.set_faction(-1);
    r.set_faction_last(-1);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

template <typename Msg>
std::vector<Msg> packets(const std::vector<Packet>& all, jx::pb::MsgId id)
{
    std::vector<Msg> out;
    for (const Packet& pk : all) {
        if (pk.msg_id != static_cast<std::uint16_t>(id)) continue;
        Msg m;
        REQUIRE(m.ParseFromString(pk.payload));
        out.push_back(m);
    }
    return out;
}

// the placed npcs by their x (found through the hero's spawn packets: the ids are slot handles, not numbers)
std::map<int, EntityId> npcs_by_x(KSubWorld& w, std::uint64_t sid)
{
    for (int i = 0; i < 6; ++i) w.tick();
    std::map<int, EntityId> found;
    for (const Packet& pk : w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_SPAWN)) continue;
        if (std::find(pk.sids.begin(), pk.sids.end(), sid) == pk.sids.end()) continue;
        jx::pb::EntitySpawn sp;
        REQUIRE(sp.ParseFromString(pk.payload));
        for (const auto& e : sp.entities()) {
            if (e.entity_type() != jx::pb::ENTITY_PLAYER) found[e.pos().x()] = EntityId{e.entity_id()};
        }
    }
    return found;
}

// kills the npc with one blow of physics damage (receive_damage 0x0808A4A0 through the public api)
void slay(KSubWorld& w, KNpc& target, KNpc& hero)
{
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> d{};
    d[1] = KMagicAttrib{magic_attackrating_v, {100000, 0, 0}};
    d[3] = KMagicAttrib{magic_physicsdamage_v, {1000000, 0, 1000000}};
    REQUIRE(w.receive_damage(target, hero, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
}

} // namespace

TEST_CASE("KNpcGoldTemplateSet::parse: the rows of npc_gold.json in order, the defaults of the loader, the client's count", "[gold][table]")
{
    std::string error;
    auto g = KNpcGoldTemplateSet::parse(kGoldJson, &error);
    REQUIRE(g.has_value());
    CHECK(g->count() == 2);
    CHECK(g->client_rows == 17);
    const KNpcGoldTemplate* kim = g->row(0);
    REQUIRE(kim != nullptr);
    CHECK(kim->name == "Kim");
    CHECK(kim->life == 200);
    CHECK(kim->poison_resist == 100);
    CHECK(kim->ai_params[8] == 50);
    CHECK(kim->ai_max_time == 18);
    const KNpcGoldTemplate* bao = g->row(1);
    REQUIRE(bao != nullptr);
    CHECK(bao->skill_id == 1103);
    CHECK(bao->skill_level == "0|1");
    CHECK(g->row(2) == nullptr);
    CHECK(g->row(-1) == nullptr);
    // an empty row takes the loader's defaults (GetInteger 1 for the percents, 0 elsewhere, AiMaxTime 100; life 0 -> 1)
    auto d = KNpcGoldTemplateSet::parse(R"({"rows": [{"name": "x", "life": 0}]})", &error);
    REQUIRE(d.has_value());
    CHECK(d->row(0)->exp == 1);
    CHECK(d->row(0)->life == 1);
    CHECK(d->row(0)->min_damage == 1);
    CHECK(d->row(0)->treasure == 0);
    CHECK(d->row(0)->ai_max_time == 100);
    CHECK(d->client_rows == 0);
    CHECK_FALSE(KNpcGoldTemplateSet::parse("nonsense", &error).has_value());
}

TEST_CASE("KNpcGold: BackData keeps the numbers, SetGoldTypeAndBackData multiplies them (the resists in use twice), RecoverBackData puts them back", "[gold]")
{
    KNpc e;
    e.kind = KNpcKind::monster;
    e.base.experience = 300;
    e.cur.experience = 300;
    e.cur.life_max = 1000;
    e.cur.life_max_yan = 1200;   // the yan twin is the larger one: the backup keeps 1200
    e.cur.life = 500;
    e.cur.life_replenish = 10;
    e.cur.attack_rating = 250;
    e.cur.defend = 40;
    e.cur.treasure = 3;
    e.cur.walk_speed = 5;
    e.cur.run_speed = 10;
    e.cur.attack_speed = 20;
    e.cur.attack_speed_yan = 30;
    e.cur.cast_speed = 40;
    e.cur.cast_speed_yan = 50;
    e.cur.fire_resist = 60;
    e.cur.fire_resist_yan = 80;   // the backup takes max(60, 80) = 80; the change works on the plain 60
    e.cur.fire_resist_max = 75;
    e.cur.cold_resist = 40;
    e.cur.cold_resist_max = 75;
    e.cur.light_resist = -20;   // a negative resist: the signed division by 100 truncates toward zero
    e.cur.light_resist_max = 75;
    e.cur.poison_resist = 30;
    e.cur.poison_resist_max = 75;
    e.cur.physics_resist = 10;
    e.cur.physics_resist_max = 75;
    e.cur.physics_damage = KMagicAttrib{magic_physicsdamage_v, {7, 0, 11}};
    e.cur.cold_damage = KMagicAttrib{magic_colddamage_v, {3, 20, 5}};
    e.cur.fire_magic = KMagicAttrib{magic_addfiremagic_v, {9, 0, 13}};
    e.cur.poison_damage = KMagicAttrib{magic_poisondamage_v, {4, 90, 30}};   // a poison in use: time 90, interval 30
    e.cur.poison_magic = KMagicAttrib{};                                      // none: untouched
    e.ai_mode = 4;
    e.ai_param[0] = 9;
    e.ai_param[9] = 1;
    e.ai_param[10] = 12345;   // the reach cell beyond the ten params stays
    e.ai_max_time = 25;
    e.drop_rate_file = R"(\t.ini)";

    gold_back_data(e);
    CHECK(e.gold.is_gold);
    CHECK_FALSE(e.gold.is_golding);
    CHECK(e.gold.gold_type == 0);
    CHECK(e.gold.fire_resist == 80);
    CHECK(e.gold.light_resist == 0);   // max(-20, the yan twin 0): the backup of a negative resist is 0
    CHECK(e.gold.life_max == 1200);
    CHECK(e.gold.experience == 300);
    CHECK(e.gold.treasure == 3);
    CHECK(e.gold.damage[0].value[2] == 11);
    CHECK(e.gold.damage[4].value[1] == 90);
    CHECK(e.gold.ai_mode == 4);
    CHECK(e.gold.ai_params[0] == 9);
    CHECK(e.gold.ai_max_time == 25);
    CHECK(e.gold.drop_rate_file == R"(\t.ini)");

    const auto g = gold_table();
    const KNpcGoldTemplate& kim = *g->row(0);
    e.gold.is_golding = true;
    gold_apply(e, kim);
    // the resists in use: x 50 / 100 twice (0x0809D969 .. 0x0809D998); the maximums x 100 / 100 once
    CHECK(e.cur.fire_resist == 15);      // 60 -> 30 -> 15
    CHECK(e.cur.fire_resist_yan == 80);  // the yan twin is not touched
    CHECK(e.cur.fire_resist_max == 75);
    CHECK(e.cur.cold_resist == 10);
    CHECK(e.cur.light_resist == -5);     // -20 -> -10 -> -5
    CHECK(e.cur.poison_resist == 30);    // Kim keeps its poison resist (100 twice)
    CHECK(e.cur.physics_resist == 2);    // 10 -> 5 -> 2
    CHECK(e.base.experience == 300);     // Exp 100
    CHECK(e.cur.life_max == 2000);       // Life 200
    CHECK(e.cur.life_max_yan == 2400);
    CHECK(e.cur.life == 2400);           // refilled with the larger maximum
    CHECK(e.cur.life_replenish == 2);    // 10 x 20 / 100
    CHECK(e.cur.attack_rating == 500);
    CHECK(e.cur.defend == 60);           // 40 x 150 / 100
    CHECK(e.cur.treasure == 16);         // replaced, not multiplied
    CHECK(e.cur.physics_damage.value[0] == 7);
    CHECK(e.cur.physics_damage.value[2] == 11);
    CHECK(e.cur.cold_damage.value[1] == 20);   // the middle value of a block is left alone
    CHECK(e.cur.fire_magic.value[2] == 13);
    CHECK(e.cur.poison_damage.value[0] == 4);
    CHECK(e.cur.poison_damage.value[1] == 60);   // a poison in use gets time 60 / interval 10
    CHECK(e.cur.poison_damage.value[2] == 10);
    CHECK(e.cur.poison_magic.value[1] == 0);     // none: untouched
    CHECK(e.cur.walk_speed == 5);
    CHECK(e.cur.attack_speed == 20);
    CHECK(e.ai_mode == 2);
    CHECK(e.ai_param[0] == 80);
    CHECK(e.ai_param[8] == 50);
    CHECK(e.ai_param[10] == 12345);
    CHECK(e.ai_max_time == 18);

    // the second row on top (as the numbers stand now): x 1000 life, the speeds added
    gold_apply(e, *g->row(1));
    CHECK(e.cur.life_max == 20000);
    CHECK(e.cur.walk_speed == 6);
    CHECK(e.cur.run_speed == 11);
    CHECK(e.cur.attack_speed == 22);
    CHECK(e.cur.attack_speed_yan == 32);
    CHECK(e.cur.cast_speed == 43);
    CHECK(e.cur.cast_speed_yan == 53);
    CHECK(e.cur.physics_damage.value[0] == 10);   // 7 x 150 / 100
    CHECK(e.cur.physics_damage.value[2] == 16);   // 11 x 150 / 100
    CHECK(e.cur.poison_damage.value[0] == 6);     // 4 x 150 / 100 by MaxDamage
    CHECK(e.cur.treasure == 4);

    // RecoverBackData: everything back to the backup, the speeds minus the row's, the life at the backed-up maximum
    e.gold.gold_type = 1;
    REQUIRE(gold_recover(e, g->row(1)));
    CHECK_FALSE(e.gold.is_golding);
    CHECK(e.gold.is_gold);   // still a candidate
    CHECK(e.cur.fire_resist == 80);   // the backed-up max(plain, yan) lands on the plain value
    CHECK(e.cur.light_resist == 0);   // (the negative one comes back as the 0 that was backed up)
    CHECK(e.cur.fire_resist_max == 75);
    CHECK(e.base.experience == 300);
    CHECK(e.cur.life_max == 1200);
    CHECK(e.cur.life_max_yan == 1200);
    CHECK(e.cur.life == 1200);
    CHECK(e.cur.life_replenish == 10);
    CHECK(e.cur.attack_rating == 250);
    CHECK(e.cur.defend == 40);
    CHECK(e.cur.treasure == 3);
    CHECK(e.cur.physics_damage.value[2] == 11);
    CHECK(e.cur.poison_damage.value[1] == 90);
    CHECK(e.cur.walk_speed == 5);
    CHECK(e.cur.run_speed == 10);
    CHECK(e.cur.attack_speed == 20);
    CHECK(e.cur.attack_speed_yan == 30);
    CHECK(e.cur.cast_speed == 40);
    CHECK(e.cur.cast_speed_yan == 50);
    CHECK(e.ai_mode == 4);
    CHECK(e.ai_param[0] == 9);
    CHECK(e.ai_param[9] == 1);
    CHECK(e.ai_max_time == 25);
    CHECK(e.drop_rate_file == R"(\t.ini)");
    CHECK_FALSE(gold_recover(e, g->row(1)));   // nothing to recover twice

    // a type outside the table (0x0809E190): the resists, the drop table and the ai come back, the numbers do not
    e.gold.is_golding = true;
    e.cur.life_max = 777;
    e.cur.fire_resist = 1;
    e.ai_mode = 1;
    REQUIRE(gold_recover(e, nullptr));
    CHECK(e.cur.life_max == 777);
    CHECK(e.cur.fire_resist == 80);
    CHECK(e.ai_mode == 4);
}

TEST_CASE("npc_class 0x08079750: a player 0, a boss 3, a gold monster 2, anything else 1; GetGoldKind is the row + 1 while gold", "[gold]")
{
    KNpc e;
    e.kind = KNpcKind::monster;
    CHECK(npc_class(e) == 1);
    CHECK(e.gold.gold_kind() == 0);
    e.gold.is_gold = true;
    CHECK(e.gold.gold_kind() == 0);   // a candidate is not gold yet
    e.gold.is_golding = true;
    e.gold.gold_type = 12;
    CHECK(e.gold.gold_kind() == 13);
    CHECK(npc_class(e) == 2);
    e.boss_flag = 2;
    CHECK(npc_class(e) == 3);   // +0x181c wins over the gold state
    e.kind = KNpcKind::player;
    CHECK(npc_class(e) == 0);
}

TEST_CASE("a Region_S.dat placement on a map with AutoGoldenNpc: +0x181c stays 0, BackData at once, the map's NormalDropRate; SetGoldTypeAndBackData rolls, changes, tells the players around", "[gold][world]")
{
    Quiet quiet;
    KSubWorldConfig c = gold_world({beast_at(Pos{2100, 2000})}, 2000, 0);
    KSubWorld w(c);
    Pos at;
    EntityId hero;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    auto placed = npcs_by_x(w, 7);
    REQUIRE(placed.count(2100) == 1);
    const EntityId beast = placed[2100];
    KNpc* b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->boss_flag == 0);   // KNpcSet::Add 0x0809FBD0 never sets +0x181c
    CHECK(b->gold.is_gold);     // the map's AutoGoldenNpc 2000 -> BackData (0x0809FCFE)
    CHECK_FALSE(b->gold.is_golding);
    CHECK(b->drop_rate_file == R"(\n.ini)");        // the map's NormalDropRate (0x0809FD30)
    CHECK(b->gold.drop_rate_file == R"(\t.ini)");   // backed up BEFORE that: the template's table comes back with the first recover
    CHECK(b->skill_list.cell(5)->id == 0);           // no 0x08085250 for a placement
    const int life_before = b->life_max();
    const int ar_before = b->cur.attack_rating;
    const int exp_before = b->base.experience;
    CHECK(b->cur.treasure == 3);

    // rate 0: g_Random(1 000 000) >= 0 always - nothing happens
    CHECK_FALSE(w.set_gold_type(*b, 0, 2));
    CHECK_FALSE(b->gold.is_golding);
    // the kind asked for: 2 = the second row (1 .. count-1 pick a row; count or more go random / the map's GoldenType)
    // - here count is 2, so 2 goes random; 1 picks Kim
    REQUIRE(w.set_gold_type(*b, 1000000, 1));
    CHECK(b->gold.is_golding);
    CHECK(b->gold.gold_type == 0);
    CHECK(b->gold.gold_kind() == 1);
    CHECK(npc_class(*b) == 2);
    CHECK(b->life_max() == life_before * 2);
    CHECK(b->life() == b->life_max());
    CHECK(b->cur.attack_rating == ar_before * 2);
    CHECK(b->base.experience == exp_before);
    CHECK(b->cur.treasure == 16);
    CHECK(b->ai_mode == 2);
    CHECK(b->ai_max_time == 18);
    CHECK(b->skill_list.cell(5)->id == 0);   // Kim has no skill
    CHECK(b->aura_skill_id == 0);
    // the 0x9a packet {npc, kind} reached the hero (0x0809DF66 -> 0x0807A870)
    auto golds = packets<jx::pb::NpcGold>(w.take_outbox(), jx::pb::G2C_NPC_GOLD);
    REQUIRE(golds.size() == 1);
    CHECK(golds[0].entity_id() == beast.value);
    CHECK(golds[0].gold_type() == 1);
    // already gold: refused (0x0809D8EF)
    CHECK_FALSE(w.set_gold_type(*b, 1000000, 1));

    // the 0x4c sync of a newcomer carries the gold word (0x0807FCA5)
    EntityId other;
    REQUIRE(w.spawn_player(8, role(80, "Other", Pos{2050, 2000}), other, at) == jx::pb::RESULT_OK);
    for (int i = 0; i < 6; ++i) w.tick();
    bool seen = false;
    for (const Packet& pk : w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_SPAWN)) continue;
        if (std::find(pk.sids.begin(), pk.sids.end(), 8u) == pk.sids.end()) continue;
        jx::pb::EntitySpawn sp;
        REQUIRE(sp.ParseFromString(pk.payload));
        for (const auto& e : sp.entities()) {
            if (e.entity_id() == beast.value) {
                seen = true;
                CHECK(e.gold_type() == 1);
            }
        }
    }
    CHECK(seen);
}

TEST_CASE("the death frames end with RecoverBackData (0x08083720), the revive rolls AutoGoldenNpc (0x0808600D) and swaps the drop table for the GoldenDropRate", "[gold][world]")
{
    Quiet quiet;
    // AutoGoldenNpc 1 000 000: every revive is gold; GoldenType 2: always the second row (the aura 1103)
    KSubWorldConfig c = gold_world({beast_at(Pos{2100, 2000})}, 1000000, 2);
    KSubWorld w(c);
    Pos at;
    EntityId hero;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    auto placed = npcs_by_x(w, 7);
    REQUIRE(placed.count(2100) == 1);
    const EntityId beast = placed[2100];
    KNpc* b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    const int life_before = b->life_max();
    CHECK(b->drop_rate_file == R"(\n.ini)");
    KNpc* h = w.mutable_entity(hero);
    REQUIRE(h != nullptr);

    // first death: nothing to recover, the revive makes it gold (row 2: x 10 life, the aura 1103 at level "0|1" = 5, +1 walk)
    slay(w, *b, *h);
    CHECK(b->doing == KDoing::death);
    for (int i = 0; i < 4; ++i) w.tick();   // DeathFrame 4 -> do_revive
    b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->doing == KDoing::revive);
    CHECK_FALSE(b->gold.is_golding);
    for (int i = 0; i < 6; ++i) w.tick();   // ReviveFrame 6 -> revive
    b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->doing == KDoing::stand);
    CHECK(b->gold.is_golding);
    CHECK(b->gold.gold_type == 1);   // the map's GoldenType 2 - 1
    CHECK(b->life_max() == life_before * 10);
    CHECK(b->life() == b->life_max());
    CHECK(b->skill_list.cell(5)->id == 1103);
    CHECK(b->skill_list.cell(5)->current_level == 5);   // "0|1" -> floor(0 + 1 x 5) without the level script
    CHECK(b->aura_skill_id == 1103);                    // SetAura 0x08087290
    CHECK(b->drop_rate_file == R"(\g.ini)");            // the map's GoldenDropRate (0x08086073)
    CHECK(b->cur.walk_speed == 6);
    CHECK(b->speed == 6 * 18);
    // the revived npc has no watchers yet (0x0807A870 finds nobody in its region either, it was just re-added): the
    // hero learns the kind from the 0x4c sync when it comes into view again
    std::vector<Packet> out = w.take_outbox();
    CHECK(packets<jx::pb::NpcGold>(out, jx::pb::G2C_NPC_GOLD).empty());
    bool resynced = false;
    for (int i = 0; i < 12; ++i) {
        for (const Packet& pk : out) {
            if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_SPAWN)) continue;
            jx::pb::EntitySpawn sp;
            REQUIRE(sp.ParseFromString(pk.payload));
            for (const auto& e : sp.entities()) {
                if (e.entity_id() == beast.value) {
                    resynced = true;
                    CHECK(e.gold_type() == 2);
                    CHECK(e.life_max() == static_cast<std::uint32_t>(life_before * 10));
                }
            }
        }
        w.tick();
        out = w.take_outbox();
    }
    CHECK(resynced);
    b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->state_of(1102) != nullptr);   // the aura's child every tenth frame (0x0808BAF6 through +0x244)

    // second death: RecoverBackData at the end of the death frames - the numbers, cell 5, the aura and the drop table
    // (the TEMPLATE's: the backup was taken before the map's NormalDropRate replaced it)
    h = w.mutable_entity(hero);
    REQUIRE(h != nullptr);
    slay(w, *b, *h);
    for (int i = 0; i < 4; ++i) w.tick();
    b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->doing == KDoing::revive);
    CHECK_FALSE(b->gold.is_golding);
    CHECK(b->gold.is_gold);
    CHECK(b->life_max() == life_before);
    CHECK(b->skill_list.cell(5)->id == 0);
    CHECK(b->aura_skill_id == 0);
    CHECK(b->cur.walk_speed == 5);
    CHECK(b->drop_rate_file == R"(\t.ini)");
    CHECK(b->cur.treasure == 3);
    // and the next revive rolls again
    for (int i = 0; i < 6; ++i) w.tick();
    b = w.mutable_entity(beast);
    REQUIRE(b != nullptr);
    CHECK(b->gold.is_golding);
    CHECK(b->life_max() == life_before * 10);
}

TEST_CASE("a map without AutoGoldenNpc: only a bSpecialNpc placement is a candidate, and it is gold on EVERY revive (2 000 000); a boss never rolls", "[gold][world]")
{
    Quiet quiet;
    KSubWorldConfig c = gold_world({beast_at(Pos{2100, 2000}, true), beast_at(Pos{2300, 2000}, false)}, 0, 1);
    KSubWorld w(c);
    Pos at;
    EntityId hero;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    auto placed = npcs_by_x(w, 7);
    REQUIRE(placed.count(2100) == 1);
    REQUIRE(placed.count(2300) == 1);
    const EntityId special = placed[2100];
    const EntityId plain = placed[2300];
    KNpc* s = w.mutable_entity(special);
    KNpc* p = w.mutable_entity(plain);
    REQUIRE(s != nullptr);
    REQUIRE(p != nullptr);
    CHECK(s->gold.is_gold);        // 0x0809FCA2: bSpecialNpc -> BackData whatever the map says
    CHECK_FALSE(p->gold.is_gold);  // 0x0809FF1E: IsGold = 0
    KNpc* h = w.mutable_entity(hero);
    REQUIRE(h != nullptr);
    s = w.mutable_entity(special);
    p = w.mutable_entity(plain);
    slay(w, *s, *h);
    slay(w, *p, *h);
    for (int i = 0; i < 10; ++i) w.tick();
    s = w.mutable_entity(special);
    p = w.mutable_entity(plain);
    REQUIRE(s != nullptr);
    REQUIRE(p != nullptr);
    CHECK(s->doing == KDoing::stand);
    CHECK(s->gold.is_golding);          // 0x080861AE: a map without the key hands 2 000 000 - always
    CHECK(s->gold.gold_type == 0);      // GoldenType 1
    CHECK(s->drop_rate_file == R"(\g.ini)");
    CHECK_FALSE(p->gold.is_golding);    // never backed up: SetGoldTypeAndBackData returns at once (0x0809D8E2)
    CHECK(p->drop_rate_file == R"(\n.ini)");

    // a boss (+0x181c != 0) is backed up by nobody and the revive skips the roll (0x08086088)
    const EntityId boss = w.spawn_npc("boss", Pos{2500, 2000}, 960, 0, KNpcKind::monster, 5, 0, 2);
    KNpc* bo = w.mutable_entity(boss);
    REQUIRE(bo != nullptr);
    gold_back_data(*bo);   // even as a candidate
    h = w.mutable_entity(hero);
    slay(w, *bo, *h);
    for (int i = 0; i < 10; ++i) w.tick();
    bo = w.mutable_entity(boss);
    REQUIRE(bo != nullptr);
    CHECK(bo->doing == KDoing::stand);
    CHECK_FALSE(bo->gold.is_golding);
    CHECK(npc_class(*bo) == 3);
}

TEST_CASE("the gold word of the 0x4c sync: a boss carries the table's count + 1 (0x0807FCC7)", "[gold][world]")
{
    Quiet quiet;
    KSubWorldConfig c = gold_world({}, 0, 0);
    KSubWorld w(c);
    const EntityId boss = w.spawn_npc("boss", Pos{2100, 2000}, 960, 0, KNpcKind::monster, 5, 0, 1);
    const EntityId plain = w.spawn_npc("plain", Pos{2150, 2000}, 960, 0, KNpcKind::monster, 5, 0, 0);
    Pos at;
    EntityId hero;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    for (int i = 0; i < 6; ++i) w.tick();
    int boss_word = -1, plain_word = -1, hero_word = -1;
    for (const Packet& pk : w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_SPAWN)) continue;
        jx::pb::EntitySpawn sp;
        REQUIRE(sp.ParseFromString(pk.payload));
        for (const auto& e : sp.entities()) {
            if (e.entity_id() == boss.value) boss_word = static_cast<int>(e.gold_type());
            if (e.entity_id() == plain.value) plain_word = static_cast<int>(e.gold_type());
            if (e.entity_id() == hero.value) hero_word = static_cast<int>(e.gold_type());
        }
    }
    CHECK(boss_word == 3);    // 2 rows + 1
    CHECK(plain_word == 0);
    CHECK(hero_word == 0);
}
