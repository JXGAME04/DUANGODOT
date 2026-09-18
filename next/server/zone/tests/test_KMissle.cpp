// The missiles as the JX2 server flies them (jx_linux_y: CastMissles 0x080ECAC0 and its
// generators, CreateMissle 0x080EA310, the frame 0x08076950, Activate 0x080760E0, OnFly
// 0x080758E0, CheckCollision 0x08075770, ProcessCollision 0x08075630, ProcessDamage 0x080753F0 -
// docs/LINUX-SERVER.md §13).  Every position and frame count here is worked out by hand from the
// binary's arithmetic: the 32-unit cells, the 1/1024 offsets, the steps of 10 units, the tables.
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KMissle.h"
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

// a template the way missles.txt has it: the columns 0x08074300 reads
KMissleTemplate tpl(int id, int move_kind, int speed, int life, int collide_range, int dmg_range, int dmg_interval, bool col_vanish,
                    int height = 10, bool auto_explode = false)
{
    std::unordered_map<std::string, std::string> cells = {
        {"MissleId", std::to_string(id)},       {"MissleName", "test"},
        {"MoveKind", std::to_string(move_kind)}, {"MissleHeight", std::to_string(height)},
        {"Speed", std::to_string(speed)},       {"LifeTime", std::to_string(life)},
        {"CollidRange", std::to_string(collide_range)}, {"DmgRange", std::to_string(dmg_range)},
        {"DmgInterval", std::to_string(dmg_interval)}, {"ColVanish", col_vanish ? "1" : "0"},
        {"AutoExplode", auto_explode ? "1" : "0"},
    };
    return KMissleTemplate::from_cells(cells);
}

// the templates of the basic attacks (rows 64 / 65 of the Linux missles.txt) and a few for the tests
std::shared_ptr<const KMissleTable> missle_table()
{
    KMissleTable t;
    t.add(tpl(64, 1, 20, 6, 1, 1, 6, false));    // 长兵物理攻击: MoveKind 1, height 10, lifetime 6, speed 20, interval 6
    t.add(tpl(65, 1, 16, 20, 1, 1, 0, true));    // 远程物理攻击: lifetime 20, speed 16, vanishes on the hit
    t.add(tpl(200, 0, 0, 5, 1, 2, 0, false));    // a still one that hits two cells around
    t.add(tpl(201, 7, 25, 40, 1, 1, 0, true));   // one that must reach its spot (MoveKind 7)
    t.add(tpl(202, 5, 20, 40, 1, 1, 0, true));   // one that follows its target
    t.add(tpl(203, 0, 0, 3, 1, 1, 0, false));    // a marker for the generator tests
    KMissleTemplate z = tpl(201, 7, 25, 40, 1, 1, 0, true);
    z.z_acceleration = 1024;
    t.add(z);
    return std::make_shared<const KMissleTable>(std::move(t));
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
    c.missles = missle_table();
    c.map = std::move(map);
    c.map_npcs = false;
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

// a style 0 skill that fires `child` missiles of template `child_id` with the launcher's own
// physics damage (no attack rating roll, no stagger: nothing random on the way)
KSkill missle_skill(int id, int form, int child_id, int child_num, int param1 = 0, int param2 = 0)
{
    KSkill s;
    s.row.id = id;
    s.row.row = 2;
    s.row.style = skill_style_missles;
    s.row.missles_form = form;
    s.row.child_skill_id = child_id;
    s.row.child_skill_num = child_num;
    s.row.child_skill_level = -1;
    s.row.base_skill = true;
    s.row.param1 = param1;
    s.row.param2 = param2;
    s.row.wait_time = 0;
    s.row.target_enemy = true;
    s.row.relation = skill_relation_enemy;
    s.row.is_physical = true;
    s.row.is_melee = true;
    s.row.use_attack_rate = false;
    s.row.series = -1;
    s.row.do_hurt = 0;
    s.level = 1;
    s.damage_attribs[2] = attrib(magic_physicsenhance_p, 0);   // the launcher's own damage
    return s;
}

// a player and a monster 50 units to its right, the monster's numbers made plain
struct Arena {
    Quiet quiet;
    KSubWorld w;
    EntityId hero;
    EntityId pig;
    KNpc* h = nullptr;
    KNpc* p = nullptr;
    explicit Arena(std::shared_ptr<const KMapData> map = nullptr, Pos pig_at = Pos{2050, 2000}) : w(small_world(std::move(map)))
    {
        Pos at;
        REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
        pig = w.spawn_npc("pig", pig_at, 418, 0, KNpcKind::monster);
        h = w.mutable_entity(hero);
        p = w.mutable_entity(pig);
        REQUIRE(h != nullptr);
        REQUIRE(p != nullptr);
        plain(*p);
        h->cur.physics_damage.value = {10, 0, 10};
        w.take_outbox();
    }
    // the entity table may move when another entity is spawned: the pointers taken again
    void refresh()
    {
        h = w.mutable_entity(hero);
        p = w.mutable_entity(pig);
        REQUIRE(h != nullptr);
        REQUIRE(p != nullptr);
    }
    void plain(KNpc& e)
    {
        e.base.life_max = 100;
        e.cur.life_max = e.cur.life_max_yan = 100;
        e.cur.life = 100;
        e.cur.mana_max = e.cur.mana_max_yan = 100;
        e.cur.mana = 100;
        e.cur.defend = 0;
        e.camp = e.current_camp = camp_animal;   // an enemy of a player (g_GenOneRelation)
        e.cur.physics_resist_max = e.cur.fire_resist_max = e.cur.cold_resist_max = e.cur.light_resist_max = e.cur.poison_resist_max = 100;
    }
    const KMissle* first_missle() const
    {
        for (const KMissle& m : w.missles()) {
            if (m.used()) return &m;
        }
        return nullptr;
    }
    std::vector<const KMissle*> live() const
    {
        std::vector<const KMissle*> r;
        for (const KMissle& m : w.missles()) {
            if (m.used()) r.push_back(&m);
        }
        return r;
    }
    void ticks(int n)
    {
        for (int i = 0; i < n; ++i) w.tick();
    }
};

} // namespace

TEST_CASE("the missile table reads the columns of missles.txt like 0x08074300", "[missle]")
{
    KMissleTemplate t = tpl(64, 1, 20, 6, 1, 1, 6, false);
    CHECK(t.id == 64);
    CHECK(t.height == 10 << 10);   // MissleHeight << 10
    CHECK(t.move_kind == 1);
    CHECK(t.life_time == 6);
    CHECK(t.speed == 20);
    CHECK(t.collide_range == 1);
    CHECK(t.damage_range == 1);
    CHECK(t.damage_interval == 6);
    CHECK_FALSE(t.collide_vanish);
    CHECK_FALSE(t.auto_explode);
    CHECK(t.name == "test");
    // an empty cell is 0 (KTabFile::GetInteger with the default 0)
    const KMissleTemplate e = KMissleTemplate::from_cells({{"MissleId", "7"}, {"MoveKind", ""}});
    CHECK(e.move_kind == 0);
    CHECK(e.height == 0);
    KMissleTable table;
    table.add(t);
    table.add(tpl(64, 0, 1, 2, 3, 4, 5, true));   // the last row of an id wins (0x0805D28F)
    REQUIRE(table.find(64) != nullptr);
    CHECK(table.find(64)->move_kind == 0);
    CHECK(table.find(65) == nullptr);
}

TEST_CASE("the start delay of the i-th missile follows MslsGenerate (0x080E8650)", "[missle]")
{
    Arena a;
    KSkill s = missle_skill(900, missles_form_line, 64, 4);
    s.row.wait_time = 5;
    s.row.missles_generate_data = 3;
    s.row.missles_generate = 0;
    CHECK(a.w.missle_start_life_time(s, 2) == 5);
    s.row.missles_generate = 1;
    CHECK(a.w.missle_start_life_time(s, 2) == 8);
    s.row.missles_generate = 2;
    CHECK(a.w.missle_start_life_time(s, 2) == 11);
    s.row.missles_generate = 5;   // |i - n/2| x data + wait
    CHECK(a.w.missle_start_life_time(s, 0) == 11);
    CHECK(a.w.missle_start_life_time(s, 2) == 5);
    CHECK(a.w.missle_start_life_time(s, 3) == 8);
    s.row.child_skill_num = 1;
    CHECK(a.w.missle_start_life_time(s, 0) == 5);
    s.row.missles_generate = 4;   // wait + g_Random(data)
    const int r = a.w.missle_start_life_time(s, 0);
    CHECK(r >= 5);
    CHECK(r < 8);
    s.row.missles_generate = 9;
    CHECK(a.w.missle_start_life_time(s, 2) == 5);
}

TEST_CASE("CreateMissle takes the template, the skill and the missle_* attributes of the level", "[missle]")
{
    Arena a;
    KSkill s = missle_skill(901, missles_form_line, 64, 1);
    s.row.wait_time = 5;
    s.row.collide_event = true;
    s.row.fly_event = true;
    s.row.fly_event_time = 3;
    s.row.do_hurt = 80;
    s.missle_attribs[0] = attrib(magic_missle_speed_v, 30);
    s.missle_attribs[1] = attrib(magic_missle_range, 2, 1, 3);       // CollidRange, IsRangeDmg, DmgRange
    s.missle_attribs[2] = attrib(magic_missle_hitcount, 4);
    s.missle_attribs[3] = attrib(magic_missle_ablility, 0, 1, 0);    // ColVanish from nValue[1]
    s.missle_attribs[4] = attrib(magic_missle_zspeed, 7, 2, 0);      // Zspeed, Zacc
    s.missle_attribs[5] = attrib(magic_missle_height_v, 3000);       // the raw +0x1c; +0x74 keeps the template's
    s.missle_attrib_count = 6;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    const KMissle* m = a.first_missle();
    REQUIRE(m != nullptr);
    CHECK(m->missle_id == 64);
    CHECK(m->move_kind == 1);
    CHECK(m->speed == 30);
    CHECK(m->collide_range == 2);
    CHECK(m->range_damage);
    CHECK(m->damage_range == 3);
    CHECK(m->rest_hit_count == 4);
    CHECK(m->collide_vanish);
    CHECK(m->height_speed == 7);
    CHECK(m->z_acceleration == 2);
    CHECK(m->height == 3000);
    CHECK(m->map_z == 10);
    CHECK(m->damage_interval == 6);
    CHECK(m->collide_event);
    CHECK(m->fly_event);
    CHECK(m->fly_event_time == 3);
    CHECK(m->do_hurt == 80);
    CHECK(m->is_melee);
    CHECK(m->relation == skill_relation_enemy);
    CHECK(m->skill_id == 901);
    CHECK(m->level == 1);
    CHECK(m->launcher == a.hero);
    CHECK(m->follow == a.pig);
    CHECK(m->parent_missle == 0);
    CHECK(m->start_life_time == 5);
    CHECK(m->life_time == 11);      // LifeTime 6 + the delay
    CHECK(m->status == missle_status_wait);
    CHECK(m->dir == 48);            // the pig is straight to the right (g_GetDirIndex of the JX2 server)
    CHECK(m->x_factor == 1024);
    CHECK(m->y_factor == 0);
    CHECK(m->ref == Pos{2000, 2000});
    CHECK(KSubWorld::missle_pos(*m) == Pos{2000, 2000});
    CHECK(m->map_x == 62);
    CHECK(m->x_offset == 16 << 10);
    REQUIRE(m->attribs != nullptr);
    CHECK(m->attribs->size() == 1);
    // slowmissle_b halves the speed of a missile that is not melee
    KSkill r = missle_skill(902, missles_form_line, 65, 1);
    r.row.is_melee = false;
    a.h->cur.slow_missle = 1;
    REQUIRE(a.w.skill_cast(r, *a.h, p));
    CHECK(a.live().size() == 2);
    CHECK(a.live()[1]->speed == 8);
    // a ClientSend 2 skill fires nothing on the server, 1 fires missiles without a payload
    r.row.client_send = 2;
    REQUIRE(a.w.skill_cast(r, *a.h, p));
    CHECK(a.live().size() == 2);
    r.row.client_send = 1;
    REQUIRE(a.w.skill_cast(r, *a.h, p));
    REQUIRE(a.live().size() == 3);
    CHECK(a.live()[2]->attribs == nullptr);
    CHECK(a.live()[2]->client_send == 1);
}

TEST_CASE("the melee attack is a missile: 20 units a frame in steps of 10, the blow when its cell holds the target", "[missle]")
{
    Arena a;
    KSkill s = *KSkill::basic_attack(1);   // row 2 of Skills.txt: template 64, WaitTime 5
    s.row.use_attack_rate = false;         // no hit roll: the blow is certain
    s.row.do_hurt = 0;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    REQUIRE(a.w.missle_count() == 1);
    // frames 0..4 wait (WaitTime 5); frame 5 flies to 2020, frame 6 to 2040
    a.ticks(7);
    const KMissle* m = a.first_missle();
    REQUIRE(m != nullptr);
    CHECK(m->status == missle_status_fly);
    CHECK(KSubWorld::missle_pos(*m) == Pos{2040, 2000});
    CHECK(a.p->life() == 100);
    // frame 7: the first step crosses into the pig's cell (2048..2079), the second step finds it
    // there before moving: 10 physics through ProcessDamage, the missile stays put this frame
    a.w.tick();
    CHECK(a.p->life() == 90);
    REQUIRE(a.first_missle() != nullptr);
    CHECK(KSubWorld::missle_pos(*a.first_missle()) == Pos{2050, 2000});
    CHECK(a.first_missle()->last_map_x == 64);
    // DmgInterval 6 keeps the next blows off while it passes; the life (6 + 5) ends at frame 11
    a.ticks(3);
    CHECK(a.p->life() == 90);
    CHECK(KSubWorld::missle_pos(*a.first_missle()) == Pos{2110, 2000});
    a.w.tick();
    CHECK(a.w.missle_count() == 0);
    CHECK(a.p->life() == 90);
}

TEST_CASE("the ranged attack vanishes on its first hit (ColVanish) and finds the target at once every step", "[missle]")
{
    Arena a;
    KSkill s = *KSkill::basic_attack(2);   // template 65: speed 16, lifetime 20, ColVanish
    s.row.use_attack_rate = false;
    s.row.do_hurt = 0;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    // frame 5: 2010 then 2016 (an offset of exactly 32 x 1024 stays in cell 62); 6: 2026, 2032;
    // 7: 2042, 2048; 8: the step to 2058 crosses into cell 64, the rest of the frame finds the pig
    a.ticks(8);
    REQUIRE(a.first_missle() != nullptr);
    CHECK(KSubWorld::missle_pos(*a.first_missle()) == Pos{2048, 2000});
    CHECK(a.first_missle()->map_x == 63);
    CHECK(a.first_missle()->x_offset == 32 << 10);
    a.w.tick();
    CHECK(a.p->life() == 90);
    CHECK(a.w.missle_count() == 0);
}

TEST_CASE("CastCircle puts the missiles on a ring, CastWall across the direction, CastZone on a grid", "[missle]")
{
    Arena a;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    SECTION("the ring (form 3): 64 / n apart from the direction, Param2 out, at the launcher without Param1")
    {
        KSkill s = missle_skill(910, missles_form_circle, 203, 3, 0, 64);
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        auto live = a.live();
        REQUIRE(live.size() == 3);
        CHECK(KSubWorld::missle_pos(*live[0]) == Pos{2064, 2000});   // dir 48: (1024, 0) x 64 >> 10
        CHECK(live[0]->dir == 48);
        CHECK(KSubWorld::missle_pos(*live[1]) == Pos{1969, 2056});   // dir 69 -> 5: (-482, 903)
        CHECK(live[1]->dir == 5);
        CHECK(KSubWorld::missle_pos(*live[2]) == Pos{1964, 1946});   // dir 90 -> 26: (-568, -851)
        CHECK(live[2]->dir == 26);
        // Param1: the ring around the target instead
        KSkill t = missle_skill(911, missles_form_circle, 203, 1, 1, 64);
        REQUIRE(a.w.skill_cast(t, *a.h, p));
        live = a.live();
        REQUIRE(live.size() == 4);
        CHECK(KSubWorld::missle_pos(*live[3]) == Pos{2114, 2000});
    }
    SECTION("the wall (form 0): n missiles Param1 apart across dir + 16, at the target, flying along it")
    {
        KSkill s = missle_skill(912, missles_form_wall, 203, 2, 32, 0);
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        auto live = a.live();
        REQUIRE(live.size() == 2);
        // dir 48 + 16 = 64 -> 0 (down): the offsets -32 and 0 along (0, 1024)
        CHECK(KSubWorld::missle_pos(*live[0]) == Pos{2050, 1968});
        CHECK(KSubWorld::missle_pos(*live[1]) == Pos{2050, 2000});
        CHECK(live[0]->dir == 0);
        CHECK(live[1]->dir == 0);
        // Param2's low word turns them dir - 16 (wrapping +48); its high word puts the wall at the launcher
        KSkill t = missle_skill(913, missles_form_wall, 203, 1, 32, 1 | (1 << 16));
        REQUIRE(a.w.skill_cast(t, *a.h, p));
        live = a.live();
        REQUIRE(live.size() == 3);
        CHECK(KSubWorld::missle_pos(*live[2]) == Pos{2000, 1984});   // offset -16 x (0, 1024) at the launcher
        CHECK(live[2]->dir == 48);                                    // 0 - 16 < 0 -> 0 + 48
    }
    SECTION("the grid (form 6): n x n cells of 32 centred on the target; Param1 == 1 keeps the circle")
    {
        KSkill s = missle_skill(914, missles_form_at_target, 203, 3, 0, 0);
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        auto live = a.live();
        REQUIRE(live.size() == 9);
        CHECK(KSubWorld::missle_pos(*live[0]) == Pos{2002, 1952});   // 2050 - 48, 2000 - 48
        CHECK(KSubWorld::missle_pos(*live[1]) == Pos{2034, 1952});
        CHECK(KSubWorld::missle_pos(*live[3]) == Pos{2002, 1984});
        CHECK(KSubWorld::missle_pos(*live[8]) == Pos{2066, 2016});
        CHECK(live[4]->follow == a.pig);
        // n = 4 with Param1 == 1: (j - 2)^2 + (i - 2)^2 <= 4 keeps 11 of 16 cells
        Arena b;
        KSkill t = missle_skill(915, missles_form_at_target, 203, 4, 1, 0);
        REQUIRE(b.w.skill_cast(t, *b.h, p));
        CHECK(b.live().size() == 11);
        // form 7: the same grid at the launcher, aimed at nobody
        Arena c;
        KSkill u = missle_skill(916, missles_form_at_firer, 203, 1, 0, 0);
        REQUIRE(c.w.skill_cast(u, *c.h, p));
        REQUIRE(c.live().size() == 1);
        CHECK(KSubWorld::missle_pos(*c.live()[0]) == Pos{2000, 2000});
        CHECK_FALSE(c.live()[0]->follow.valid());
        // form 4: nothing at all
        Arena d;
        KSkill v = missle_skill(917, missles_form_random, 203, 3, 0, 0);
        REQUIRE(d.w.skill_cast(v, *d.h, p));
        CHECK(d.live().empty());
    }
}

TEST_CASE("CastSpread aims the fan at the target: the unit vector turned by the fan angle", "[missle]")
{
    Arena a;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    KSkill s = missle_skill(920, missles_form_spread, 203, 3, 4, 64);   // Param1 4 apart, Param2 64 out
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    auto live = a.live();
    REQUIRE(live.size() == 3);
    // the unit vector to the pig is (1024, 0); half = 1: the angle index (1 x 4 + 48) = 52 turns it
    // to (946, -391), then 48 leaves it, then 44 gives (946, 391)
    CHECK(KSubWorld::missle_pos(*live[0]) == Pos{2059, 1975});
    CHECK(live[0]->dir == 44);
    CHECK(live[0]->x_factor == 946);
    CHECK(live[0]->y_factor == -391);
    CHECK(KSubWorld::missle_pos(*live[1]) == Pos{2064, 2000});
    CHECK(live[1]->dir == 48);
    CHECK(live[1]->x_factor == 1024);
    CHECK(KSubWorld::missle_pos(*live[2]) == Pos{2059, 2024});
    CHECK(live[2]->dir == 52);
    CHECK(live[2]->y_factor == 391);
    // without a target (a cast in a direction) the fan spreads around that direction
    Arena b;
    KSubWorld::KCastParams q;
    q.dir = 16;   // left
    REQUIRE(b.w.skill_cast(s, *b.h, q));
    live = b.live();
    REQUIRE(live.size() == 3);
    CHECK(live[0]->dir == 12);
    CHECK(live[1]->dir == 16);
    CHECK(live[2]->dir == 20);
    CHECK(KSubWorld::missle_pos(*live[1]) == Pos{1936, 2000});   // 2000 + (64 x -1024 >> 10)
    CHECK_FALSE(live[1]->follow.valid());
}

TEST_CASE("a MoveKind 7 missile climbs and lands on its spot in the frames it should be there", "[missle]")
{
    Arena a;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    KSkill s = missle_skill(930, missles_form_line, 201, 1);   // template 201: MoveKind 7, speed 25, Zacc 1024
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    const KMissle* m = a.first_missle();
    REQUIRE(m != nullptr);
    CHECK(m->must_be_hit);
    CHECK(m->x_factor == 1024);   // the unit vector to the pig (50 units away)
    CHECK(m->y_factor == 0);
    CHECK(m->height_speed == 512);   // ((50 / 25) - 1) x 1024 / 2
    CHECK(m->arrive_from == 2);      // the delay 0 + 50 / 25
    CHECK(m->arrive_to == 5);        // + 32 / 25 + 2
    // frames 0 and 1 fly 25 units each; frame 2 is inside the window and the exact spot holds the pig
    a.ticks(2);
    CHECK(KSubWorld::missle_pos(*a.first_missle()) == Pos{2050, 2000});
    CHECK(a.p->life() == 100);
    a.w.tick();
    CHECK(a.p->life() == 90);
    CHECK(a.w.missle_count() == 0);   // ColVanish
}

TEST_CASE("a following missile aims again after eight frames and jumps onto its target's spot", "[missle]")
{
    Arena a(nullptr, Pos{2050, 2200});
    KSubWorld::KCastParams p;
    p.target = a.pig;
    KSkill s = missle_skill(931, missles_form_line, 202, 1);   // template 202: MoveKind 5, speed 20
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    const KMissle* m = a.first_missle();
    REQUIRE(m != nullptr);
    CHECK(m->dir == 62);          // 50 right, 200 down: nsin 994 lands between 1004 and 979
    CHECK(m->x_factor == 199);
    CHECK(m->y_factor == 1004);
    // eight frames along (199, 1004) x 20; the ninth aims again from (2031, 2156)
    a.ticks(9);
    m = a.first_missle();
    REQUIRE(m != nullptr);
    CHECK(m->des == Pos{2050, 2200});
    CHECK(m->x_factor == 413);    // (19 << 10) / 47
    CHECK(m->y_factor == 958);    // (44 << 10) / 47
    CHECK(m->dir == 60);
    CHECK(m->param2 == 2);        // 47 / 20 + 1, one frame counted down
    CHECK(m->param1 == 0);        // the counter starts again
    CHECK(a.p->life() == 100);
    // two more frames, then the jump onto the spot; the frame after finds the pig in its cell
    a.ticks(2);
    CHECK(KSubWorld::missle_pos(*a.first_missle()) == Pos{2050, 2200});
    CHECK(a.p->life() == 100);
    a.w.tick();
    CHECK(a.p->life() == 90);
    CHECK(a.w.missle_count() == 0);
}

TEST_CASE("the miss rate, the hit count and the blows within DmgRange", "[missle]")
{
    Arena a;
    const EntityId pig2 = a.w.spawn_npc("pig2", Pos{2070, 2000}, 418, 0, KNpcKind::monster);
    a.refresh();
    KNpc* p2 = a.w.mutable_entity(pig2);
    REQUIRE(p2 != nullptr);
    a.plain(*p2);
    KSubWorld::KCastParams p;
    p.target = a.pig;
    SECTION("a still missile at the target hits everybody within DmgRange cells of its cell")
    {
        KSkill s = missle_skill(940, missles_form_at_target, 200, 1);   // template 200: MoveKind 0, DmgRange 2
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        a.w.tick();
        CHECK(a.p->life() == 90);
        CHECK(p2->life() == 90);
        CHECK(a.w.missle_count() == 1);   // no interval: it goes on hitting every frame of its life
        a.w.tick();
        CHECK(a.p->life() == 80);
    }
    SECTION("missle_hitcount ends the missile after so many blows, the walk with it")
    {
        KSkill s = missle_skill(941, missles_form_at_target, 200, 1);
        s.missle_attribs[0] = attrib(magic_missle_hitcount, 1);
        s.missle_attrib_count = 1;
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        a.w.tick();
        CHECK(a.p->life() + p2->life() == 190);
        CHECK(a.w.missle_count() == 0);
    }
    SECTION("MissRate 100 misses every blow")
    {
        KSkill s = missle_skill(942, missles_form_at_target, 200, 1);
        s.missle_attribs[0] = attrib(magic_missle_missrate, 100);
        s.missle_attrib_count = 1;
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        a.ticks(3);
        CHECK(a.p->life() == 100);
        CHECK(p2->life() == 100);
    }
    SECTION("the state modifier of the skill takes its delta off the miss rate")
    {
        KSkill s = missle_skill(943, missles_form_at_target, 200, 1);
        s.missle_attribs[0] = attrib(magic_missle_missrate, 100);
        s.missle_attrib_count = 1;
        a.h->state_modifier.skill_id = 943;
        a.h->state_modifier.attrib = magic_missle_missrate;
        a.h->state_modifier.delta = 100;
        REQUIRE(a.w.skill_cast(s, *a.h, p));
        CHECK(a.first_missle()->miss_rate == 0);
        a.w.tick();
        CHECK(a.p->life() == 90);
    }
}

TEST_CASE("an obstacle under the missile ends it (TestBarrier), the map's edge as well", "[missle]")
{
    auto map = std::make_shared<KMapData>(KMapData::synthetic(128, 128));
    map->set_blocked(63, 62, 1);   // Obstacle_Normal on the way (2016..2047)
    Arena a(map);
    KSkill s = *KSkill::basic_attack(1);
    s.row.use_attack_rate = false;
    s.row.do_hurt = 0;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    a.ticks(6);   // frame 5 moves it to 2020, inside the blocked cell
    REQUIRE(a.first_missle() != nullptr);
    CHECK(a.first_missle()->map_x == 63);
    a.w.tick();   // frame 6: the first step finds the barrier
    CHECK(a.w.missle_count() == 0);
    CHECK(a.p->life() == 100);
    // Obstacle_Jump (3) as well; 2 (a low one) lets it through
    map->set_blocked(63, 62, 2);
    Arena b(map);
    REQUIRE(b.w.skill_cast(s, *b.h, p));
    b.ticks(8);
    CHECK(b.p->life() == 90);
    // fired toward the edge of the map it flies off it and is gone
    Arena c(map);
    KSubWorld::KCastParams q;
    q.dir = 16;   // left, 2000 units of room at 20 a frame: the life ends first
    KSkill t = missle_skill(950, missles_form_line, 64, 1);
    REQUIRE(c.w.skill_cast(t, *c.h, q));
    c.ticks(7);
    CHECK(c.w.missle_count() == 0);
}

TEST_CASE("the collide event casts CollidSkillId from the missile's spot, the child keeps the target and the parent", "[missle]")
{
    Quiet quiet;
    // the event skill lives in the map's skill table (the events look it up by id and level)
    KSkillTable table;
    KSkillRow child = KSkillRow::from_cells({{"SkillId", "961"}, {"SkillStyle", "0"}, {"MisslesForm", "6"}, {"ChildSkillId", "203"},
                                             {"ChildSkillNum", "1"}, {"ChildSkillLevel", "-1"}, {"BaseSkill", "1"}, {"TargetAlly", "1"},
                                             {"IsPhysical", "1"}, {"Series", "-1"}, {"DoHurt", "0"}});
    child.row = 2;
    table.add(child);
    // the casting skill itself (the events look it up by id and level: 0x08076CE0)
    KSkillRow parent = KSkillRow::from_cells({{"SkillId", "960"}, {"SkillStyle", "0"}, {"MisslesForm", "1"}, {"ChildSkillId", "65"},
                                              {"ChildSkillNum", "1"}, {"ChildSkillLevel", "-1"}, {"BaseSkill", "1"}, {"TargetEnemy", "1"},
                                              {"IsPhysical", "1"}, {"IsMelee", "1"}, {"Series", "-1"}, {"DoHurt", "0"}, {"IsUseAR", "0"},
                                              {"CollideEvent", "1"}, {"CollidSkillId", "961"}, {"VanishedEvent", "1"}, {"VanishedSkillId", "961"},
                                              {"EventSkillLevel", "-1"}, {"ByMissle", "1"}, {"LvlSetScript", "\\script\\skill\\none.lua"},
                                              {"LvlSetting1", "physicsenhance_p"}, {"LvlData1", "x"}});
    parent.row = 3;
    table.add(parent);
    KSubWorldConfig cfg = small_world();
    cfg.skills = std::make_shared<const KSkillTable>(std::move(table));
    KSubWorld w(cfg);
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId pig = w.spawn_npc("pig", Pos{2050, 2000}, 418, 0, KNpcKind::monster);
    KNpc* h = w.mutable_entity(hero);
    KNpc* p = w.mutable_entity(pig);
    REQUIRE(h != nullptr);
    REQUIRE(p != nullptr);
    p->cur.defend = 0;
    p->camp = p->current_camp = camp_animal;
    p->cur.physics_resist_max = 100;
    h->cur.physics_damage.value = {10, 0, 10};
    REQUIRE(w.skills() != nullptr);
    const KSkill* s = w.skills()->get(960, 1);   // template 65: ColVanish on the hit; the level numbers are 0 without a script
    REQUIRE(s != nullptr);
    CHECK(s->row.collide_event);
    CHECK(s->row.is_physical);
    CHECK(s->row.relation == skill_relation_enemy);
    CHECK(s->damage_attrib_count == 1);   // physicsenhance_p at 0: the launcher's own damage
    CHECK(s->damage_attribs[2].type == magic_physicsenhance_p);
    KSubWorld::KCastParams cp;
    cp.target = pig;
    REQUIRE(w.skill_cast(*s, *h, cp));
    const int life = p->life();
    for (int i = 0; i < 4; ++i) w.tick();   // 16 a frame: 2016, 2032, 2048; the fourth frame steps to 2058 and finds the pig
    CHECK(p->life() == life - 10);
    // the parent vanished (and its vanished event fired the same skill again); two children stand
    // where it hit, aimed at the pig, marked with the parent's index
    std::vector<const KMissle*> live;
    for (const KMissle& m : w.missles()) {
        if (m.used()) live.push_back(&m);
    }
    REQUIRE(live.size() == 2);
    for (const KMissle* m : live) {
        CHECK(m->skill_id == 961);
        CHECK(m->parent_missle == 1);
        CHECK(m->follow == pig);
        CHECK(KSubWorld::missle_pos(*m) == Pos{2058, 2000});
        CHECK(m->launcher == hero);
    }
}

TEST_CASE("a style 14 skill lands its child's area blow at the target at once", "[missle]")
{
    Arena a;
    const EntityId pig2 = a.w.spawn_npc("pig2", Pos{2070, 2000}, 418, 0, KNpcKind::monster);
    a.refresh();
    KNpc* p2 = a.w.mutable_entity(pig2);
    REQUIRE(p2 != nullptr);
    a.plain(*p2);
    KSkill s = missle_skill(970, missles_form_line, 200, 1);   // template 200: DmgRange 2
    s.row.style = skill_style_jx2_14;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    CHECK(a.p->life() == 90);
    CHECK(p2->life() == 90);
    CHECK(a.w.missle_count() == 1);   // gone after the frame
    a.w.tick();
    CHECK(a.w.missle_count() == 0);
    CHECK(a.p->life() == 90);
}

TEST_CASE("a missile whose launcher left the map or changed camp is dropped", "[missle]")
{
    Arena a;
    KSkill s = *KSkill::basic_attack(1);
    KSubWorld::KCastParams p;
    p.target = a.pig;
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    a.h->current_camp = camp_evil;
    a.w.tick();
    CHECK(a.w.missle_count() == 0);
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    a.h->current_camp = a.h->camp;
    REQUIRE(a.w.remove_player(7));
    a.w.tick();
    CHECK(a.w.missle_count() == 0);
}

TEST_CASE("the clients hear a missile: born, the first flying frame and every sixth, gone (G2C_MISSLE)", "[missle]")
{
    // the 2.0 client re-runs CastMissles itself; here the zone tells the watchers of the launcher (docs/CLIENT-2.0.md §11)
    Arena a;
    a.h->watchers = {7};   // the hero's own client sees the hero
    KSkill s = *KSkill::basic_attack(1);   // template 64: WaitTime 5, life 6, speed 20
    s.row.use_attack_rate = false;
    s.row.do_hurt = 0;
    KSubWorld::KCastParams p;
    p.target = a.pig;
    a.w.take_outbox();
    REQUIRE(a.w.skill_cast(s, *a.h, p));
    const auto packets = [](std::vector<Packet> all) {
        std::vector<jx::pb::MissleSync> out;
        for (const Packet& pk : all) {
            if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_MISSLE)) continue;
            jx::pb::MissleSync m;
            REQUIRE(m.ParseFromString(pk.payload));
            out.push_back(m);
        }
        return out;
    };
    auto born = packets(a.w.take_outbox());
    REQUIRE(born.size() == 1);
    CHECK(born[0].status() == 0);
    CHECK(born[0].missle_id() == 64);
    CHECK(born[0].skill_id() == 1);
    CHECK((born[0].x() == 2000 && born[0].y() == 2000));
    CHECK(born[0].start_life_time() == 5);
    CHECK(born[0].life_time() == 11);
    CHECK(born[0].speed() == 20);
    CHECK_FALSE(born[0].removed());
    // frames 0..4 wait: nothing; frame 5 flies (the first flying frame at 2020); frame 11 would be the sixth after
    a.ticks(7);
    auto fly = packets(a.w.take_outbox());
    REQUIRE(fly.size() == 1);
    CHECK(fly[0].status() == 1);
    CHECK(fly[0].x() == 2020);
    CHECK(fly[0].current_life() == 5);
    CHECK(fly[0].index() == born[0].index());
    // the life ends at frame 11 (frames 7..11): the removal is told once
    a.ticks(5);
    auto gone = packets(a.w.take_outbox());
    REQUIRE(!gone.empty());
    CHECK(gone.back().removed());
    CHECK(gone.back().status() == 2);
    CHECK(a.w.missle_count() == 0);
    // the blow at frame 7 (2050, the pig's cell) is told as a collision at the missile's spot
    bool hit = false;
    for (const auto& g : gone) {
        if (g.collided()) {
            hit = true;
            CHECK((g.x() == 2050 && !g.removed()));
        }
    }
    CHECK(hit);
}
