// The auto skills of the JX2 server (jx_linux_y: the lists 0x08189000 / 0x08188BB0, the on-cast map
// 0x0809AE60 / 0x080821C0) and the free spot of a knock back (0x08081B70 with the barrier kinds of
// 0x080E0A30) - docs/LINUX-SERVER.md §14.  Every number here is worked out by hand from the binary.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "jx/log.hpp"
#include "jx/client.pb.h"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSubWorld.h"

using namespace jx::zone;   // the MAGIC_ATTRIB ids
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

// a level script in the shape of the real ones: one immediate attribute per skill
constexpr const char* kLevelScript = R"lua(
function GetSkillLevelData(levelname, data, level)
    if levelname ~= "life_v" then return "" end
    if data == "a900" then return "-5,0,0" end
    if data == "a901" then return "-7,0,0" end
    if data == "a902" then return "-3,0,0" end
    if data == "a903" then return "-2,0,0" end
    return ""
end
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_autoskill_test";
    std::filesystem::create_directories(root / "script" / "skill");
    std::ofstream(root / "script" / "skill" / "auto.lua") << kLevelScript;
    return root.generic_string();
}

// four style 2 skills: 900 and 903 on oneself, 901 and 902 on an enemy, each one blow of life
std::shared_ptr<const KSkillTable> skill_table()
{
    KSkillTable t;
    auto add = [&](int id, bool self) {
        KSkillRow r = KSkillRow::from_cells({{"SkillId", std::to_string(id)}, {"SkillStyle", "2"}, {"TargetSelf", self ? "1" : "0"},
                                             {"TargetEnemy", self ? "0" : "1"}, {"IsPhysical", "1"}, {"Series", "-1"}, {"DoHurt", "0"},
                                             {"LvlSetScript", "\\script\\skill\\auto.lua"}, {"LvlSetting1", "life_v"}, {"LvlData1", "a" + std::to_string(id)}});
        r.row = id - 897;
        t.add(r);
    };
    add(900, true);
    add(901, false);
    add(902, false);
    add(903, true);
    // attribconstdata.ini [autoreplyskill] / [autoattackskill]: the skills whose blows do not wake
    // the lists again - the auto skills themselves, or a hit answers a hit without end (the
    // binary would run out of stack just the same)
    t.set_attrib_data(magic_autoreplyskill, {901, 902});
    t.set_attrib_data(magic_autoattackskill, {901, 902});
    return std::make_shared<const KSkillTable>(std::move(t));
}

KSubWorldConfig small_world(std::shared_ptr<const KMapData> map = nullptr)
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;
    c.map = std::move(map);
    c.map_npcs = false;
    c.skills = skill_table();
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    return c;
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
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

KMagicAttrib attrib(int type, int v0, int v1 = 0, int v2 = 0)
{
    KMagicAttrib m;
    m.type = type;
    m.value = {v0, v1, v2};
    return m;
}

struct Arena {
    Quiet quiet;
    KSubWorld w;
    EntityId hero;
    EntityId pig;
    KNpc* h = nullptr;
    KNpc* p = nullptr;
    explicit Arena(std::shared_ptr<const KMapData> map = nullptr) : w(small_world(std::move(map)))
    {
        Pos at;
        REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
        pig = w.spawn_npc("pig", Pos{2050, 2000}, 418, 0, KNpcKind::monster);
        h = w.mutable_entity(hero);
        p = w.mutable_entity(pig);
        REQUIRE(h != nullptr);
        REQUIRE(p != nullptr);
        p->base.life_max = 100;
        p->cur.life_max = p->cur.life_max_yan = 100;
        p->cur.life = 100;
        p->cur.defend = 0;
        p->camp = p->current_camp = camp_animal;
        p->cur.physics_resist_max = 100;
        h->cur.physics_damage.value = {10, 0, 10};
        w.take_outbox();
    }
    void ticks(int n)
    {
        for (int i = 0; i < n; ++i) w.tick();
    }
};

constexpr int key(int id, int level) { return (id << 8) | level; }

} // namespace

TEST_CASE("the auto-skill attributes fill the lists the way 0x08189000 does", "[autoskill]")
{
    Arena a;
    KNpc& h = *a.h;
    auto& every = h.auto_skills[static_cast<std::size_t>(KAutoSkillList::every_frame)];
    auto& reply = h.auto_skills[static_cast<std::size_t>(KAutoSkillList::hit_reply)];
    auto& on_hit = h.auto_skills[static_cast<std::size_t>(KAutoSkillList::on_hit)];
    // autocastskill: |v0| = id << 8 | level, v2 = frames << 8 | percent, v1 == 1 = the npc's own skill
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, key(900, 1), 1, (6 << 8) | 100), false);
    REQUIRE(every.size() == 1);
    const KAutoSkillEntry* e = &every.at(key(900, 1));
    CHECK(e->rate == 100);
    CHECK(e->interval == 6);
    CHECK(e->own_skill);
    CHECK(e->at_target == 0);
    REQUIRE(e->next_tick.count(a.hero.value) == 1);
    CHECK(e->next_tick.at(a.hero.value) == 6);   // 0x08188A10: the frame now (0) + the interval
    // the same key again adds its percent; the negated values of a state that goes take it back
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, key(900, 1), 0, (6 << 8) | 50), false);
    CHECK(every.at(key(900, 1)).rate == 150);
    CHECK(every.at(key(900, 1)).own_skill);   // an entry keeps its own-skill flag
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, -key(900, 1), 0, -((6 << 8) | 150)), true);
    CHECK(every.empty());
    // autoreplyskill: the top byte of |v0| says "at the attacker"
    a.w.modify_attrib(h, a.hero, attrib(magic_autoreplyskill, (1 << 24) | key(901, 2), 0, 100), false);
    REQUIRE(reply.size() == 1);
    CHECK(reply.at(key(901, 2)).at_target == 1);
    CHECK(reply.at(key(901, 2)).rate == 100);
    CHECK(reply.at(key(901, 2)).interval == 0);
    CHECK_FALSE(reply.at(key(901, 2)).own_skill);
    // autoattackskill: never at the target, never an own skill
    a.w.modify_attrib(h, a.hero, attrib(magic_autoattackskill, key(902, 1), 1, (3 << 8) | 30), false);
    REQUIRE(on_hit.size() == 1);
    CHECK(on_hit.at(key(902, 1)).rate == 30);
    CHECK(on_hit.at(key(902, 1)).interval == 3);
    CHECK_FALSE(on_hit.at(key(902, 1)).own_skill);
    // oncastskill: [|v0|][|v2|] += v1, out at 0 or below, the outer node kept
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, 1, 100, 903), false);
    REQUIRE(h.on_cast_skills.count(1) == 1);
    CHECK(h.on_cast_skills.at(1).at(903) == 100);
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, 1, 20, 903), false);
    CHECK(h.on_cast_skills.at(1).at(903) == 120);
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, -1, -120, -903), true);
    CHECK(h.on_cast_skills.at(1).empty());
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, 2, -5, 903), false);   // nothing to take back: no node either
    CHECK(h.on_cast_skills.count(2) == 0);
    // ClearAttrib empties the every-frame list and the on-cast map, not the other lists
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, key(900, 1), 0, 100), false);
    h.clear_attrib(false, 0);
    CHECK(every.empty());
    CHECK(h.on_cast_skills.empty());
    CHECK(reply.size() == 1);
    CHECK(on_hit.size() == 1);
}

TEST_CASE("the every-frame list casts on the npc itself once the wait is over, then waits again", "[autoskill]")
{
    Arena a;
    KNpc& h = *a.h;
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, key(900, 1), 0, (5 << 8) | 100), false);   // 100 percent, 5 frames apart
    const int life = h.life();
    // the wait ends at frame 5 (0 + 5); the next cast at 10, the next at 15
    a.ticks(4);
    CHECK(h.life() == life);
    a.w.tick();
    CHECK(h.life() == life - 5);
    a.ticks(4);
    CHECK(h.life() == life - 5);
    a.w.tick();
    CHECK(h.life() == life - 10);
    CHECK(h.auto_skills[static_cast<std::size_t>(KAutoSkillList::every_frame)].at(key(900, 1)).next_tick.at(a.hero.value) == 15);
    // a percent of 0 never comes up; an unknown skill is skipped without a wait
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, -key(900, 1), 0, -100), true);
    a.w.modify_attrib(h, a.hero, attrib(magic_autocastskill, key(1999, 1), 0, 100), false);
    a.ticks(3);
    CHECK(h.life() == life - 10);
}

TEST_CASE("a blow wakes the hit-reply list of the target at the attacker and the on-hit list of the attacker at the target", "[autoskill]")
{
    Arena a;
    KNpc& h = *a.h;
    KNpc& p = *a.p;
    // the pig answers a hit with skill 901 on whoever hit it; the hero follows every hit with 902 on the victim
    a.w.modify_attrib(p, a.pig, attrib(magic_autoreplyskill, (1 << 24) | key(901, 1), 0, 100), false);
    a.w.modify_attrib(h, a.hero, attrib(magic_autoattackskill, key(902, 1), 0, 100), false);
    KSkill s = *KSkill::basic_attack(1);
    s.row.use_attack_rate = false;
    s.row.do_hurt = 0;
    KSubWorld::KCastParams cp;
    cp.target = a.pig;
    const int hero_life = h.life();
    REQUIRE(a.w.skill_cast(s, h, cp));
    a.ticks(8);   // the melee missile lands on frame 7
    CHECK(p.life() == 100 - 10 - 3);   // the blow, then 902 on the pig
    CHECK(h.life() == hero_life - 7);  // 901 on the hero
    // the waits are keyed by the npc the list was walked for: the pig's reply waits for the hero
    CHECK(p.auto_skills[static_cast<std::size_t>(KAutoSkillList::hit_reply)].at(key(901, 1)).next_tick.count(a.hero.value) == 0);
    CHECK(p.auto_skills[static_cast<std::size_t>(KAutoSkillList::hit_reply)].at(key(901, 1)).next_tick.count(a.pig.value) == 1);
    CHECK(h.auto_skills[static_cast<std::size_t>(KAutoSkillList::on_hit)].at(key(902, 1)).next_tick.count(a.pig.value) == 1);
}

TEST_CASE("the on-cast map casts its skills with the one cast, at its level and target", "[autoskill]")
{
    Arena a;
    KNpc& h = *a.h;
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, 900, 100, 903), false);
    a.w.modify_attrib(h, a.hero, attrib(magic_oncastskill, 900, 100, 901), false);   // 901 wants an enemy: refused on oneself
    REQUIRE(a.w.skills() != nullptr);
    const KSkill* s = a.w.skills()->get(900, 1);
    REQUIRE(s != nullptr);
    const int life = h.life();
    KSubWorld::KCastParams p;
    p.target = a.hero;
    REQUIRE(a.w.skill_cast(*s, h, p));
    CHECK(h.life() == life - 5 - 2);
}

TEST_CASE("the knock back walks toward its spot in steps of the step length and stops before a barrier", "[autoskill][knockback]")
{
    auto map = std::make_shared<KMapData>(KMapData::synthetic(128, 128));
    SECTION("a free way: 8 steps of 12 of the 100 asked")
    {
        Arena a(map);
        CHECK(a.p->cur.step_length == 12);
        a.ticks(4);   // the hero's next look (every fourth tick): it learns the pig, spawned after its first look
        a.w.take_outbox();
        a.w.knock_back(*a.p, *a.h, 5, 100);
        CHECK(a.p->doing == KDoing::knock_back);
        CHECK(a.p->knock_dest == Pos{2146, 2000});   // away from the hero (dir 16 -> right): 2050 + (8 x 12288 >> 10)
        // SendSyncAction(0x18, x, y, frames, 0) 0x0807A970 - the 0x56 packet {0x56, id, doing, x, y, frames, 0} to the
        // players around (0x0807A870, 22 bytes): ACTION_KNOCK_BACK with the spot, the frames, who pushed, the facing
        int told = 0;
        for (const Packet& pk : a.w.take_outbox()) {
            if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
            jx::pb::EntityAction act;
            REQUIRE(act.ParseFromString(pk.payload));
            if (act.entity_id() != a.pig.value || act.action() != jx::pb::ACTION_KNOCK_BACK) continue;
            ++told;
            CHECK(act.aim().x() == 2146);
            CHECK(act.aim().y() == 2000);
            CHECK(act.frames() == 5);
            CHECK(act.target() == a.hero.value);
            CHECK(act.dir() == a.p->dir);
            CHECK(act.pos().x() == 2050);
        }
        CHECK(told == 1);
        a.ticks(5);
        CHECK(a.p->doing == KDoing::stand);
        CHECK(a.p->pos() == Pos{2146, 2000});
    }
    SECTION("Obstacle_Normal on the sixth step: the last good step is the spot")
    {
        map->set_blocked(66, 62, 1);   // 2112..2143
        Arena a(map);
        a.w.knock_back(*a.p, *a.h, 5, 100);
        CHECK(a.p->knock_dest == Pos{2110, 2000});
        map->set_blocked(66, 62, 0);
    }
    SECTION("Obstacle_Jump is flown over and does not count as a good step")
    {
        map->set_blocked(66, 62, 3);
        Arena a(map);
        a.w.knock_back(*a.p, *a.h, 5, 100);
        CHECK(a.p->knock_dest == Pos{2146, 2000});   // steps 6 and 7 (2122, 2134) fly, step 8 (2146) is good
        map->set_blocked(66, 62, 0);
    }
    SECTION("a diagonal cell passes on one side of its line (0x080E0A30)")
    {
        map->set_blocked(66, 62, 0x31);   // shape 3 over kind 1: passes where ox < oy
        Arena a(map);
        CHECK(a.w.barrier_kind(Pos{2112 + 5, 1984 + 20}) == 0);
        CHECK(a.w.barrier_kind(Pos{2112 + 20, 1984 + 5}) == 1);
        CHECK(a.w.barrier_kind(Pos{-1, 5}) == -1);
        map->set_blocked(66, 62, 0x52);   // shape 5 over kind 2: passes where ox + oy <= 31
        CHECK(a.w.barrier_kind(Pos{2112 + 10, 1984 + 21}) == 0);
        CHECK(a.w.barrier_kind(Pos{2112 + 10, 1984 + 22}) == 2);
        map->set_blocked(66, 62, 0);
    }
    SECTION("no step length, no way: nothing happens")
    {
        Arena a(map);
        a.p->cur.step_length = 0;
        a.w.knock_back(*a.p, *a.h, 5, 100);
        CHECK(a.p->doing == KDoing::stand);
    }
}
