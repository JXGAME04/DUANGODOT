// KNpcAI: the six AIMode behaviours of npcs.txt against the old code paths (KNpcAI.cpp).
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KSubWorld.h"

using jx::EntityId;
using jx::zone::KNpcTemplate;
using jx::zone::KNpcTemplateSet;
using jx::zone::KSubWorld;
using jx::zone::KSubWorldConfig;
using jx::zone::Packet;
using jx::zone::Pos;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
    }
};

template <class Msg>
Msg decode(const Packet& p)
{
    Msg m;
    REQUIRE(m.ParseFromString(p.payload));
    return m;
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    // a strong, sure-footed hero: attack rating 4 x 100 - 28 = 372 against the template-less
    // monster's defence 10 (KNpc::Init) reaches the 95 percent cap, life 500 (KPlayer::LoadFrom)
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

// Templates in the spirit of npcs.txt: 900 an active boar (AIMode 1, animal camp, melee skill 53
// with radius 75), 901 the passive twin (AIMode 4), 902 a justice-camp hunter, 903 a coward
// (AIMode 3 that always flees).  AIMaxTime 3 keeps the decisions quick.
std::shared_ptr<const KNpcTemplateSet> templates()
{
    KNpcTemplateSet set;
    KNpcTemplate t;
    t.id = 900;
    t.name = "boar";
    t.kind = jx::zone::kind_normal;
    t.camp = jx::zone::camp_animal;
    t.ai_mode = 1;
    const int p[10] = {0, 100, 0, 0, 0, 0, 0, 0, 0, 5};
    for (int i = 0; i < 10; ++i) t.ai_param[i] = p[i];
    t.ai_max_time = 3;
    t.vision_radius = 400;
    t.active_radius = 700;
    t.walk_speed = 6;
    t.attack_frame = 18;
    t.hurt_frame = 12;
    t.death_frame = 15;
    t.hit_recover = 0;
    t.life_param = 40;
    t.min_damage = 3;
    t.max_damage = 5;
    t.skills[1].id = 53;
    t.skills[1].level_a = 1;
    t.skills[1].known = true;
    t.skills[1].attack_radius = 75;
    t.skills[1].melee = true;
    set.add(t);
    t.id = 901;
    t.ai_mode = 4;
    set.add(t);
    t.id = 902;
    t.ai_mode = 1;
    t.camp = jx::zone::camp_justice;
    set.add(t);
    t.id = 903;
    t.ai_mode = 3;
    t.camp = jx::zone::camp_animal;
    const int coward[10] = {0, 101, 100, 0, 100, 0, 0, 0, 0, 5};   // life below 101 % -> always handled -> never skill 1 -> flee
    for (int i = 0; i < 10; ++i) t.ai_param[i] = coward[i];
    set.add(t);
    return std::make_shared<const KNpcTemplateSet>(std::move(set));
}

KSubWorldConfig ai_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 8192;
    c.height = 8192;
    c.cell_size = 512;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;
    c.templates = templates();
    return c;
}

struct Seen {
    bool moved = false;      // an EntityMove of the npc
    bool attacked = false;   // an ACTION_ATTACK of the npc
    bool hurt_hero = false;  // an EntityLife with a loss for the hero
};

Seen scan(KSubWorld& w, EntityId npc, EntityId hero)
{
    Seen s;
    for (const Packet& p : w.take_outbox()) {
        if (p.msg_id == jx::pb::G2C_ENTITY_MOVE && decode<jx::pb::EntityMove>(p).entity_id() == npc.value) s.moved = true;
        if (p.msg_id == jx::pb::G2C_ENTITY_ACTION) {
            const auto a = decode<jx::pb::EntityAction>(p);
            if (a.entity_id() == npc.value && a.action() == jx::pb::ACTION_ATTACK) s.attacked = true;
        }
        if (p.msg_id == jx::pb::G2C_ENTITY_LIFE) {
            const auto l = decode<jx::pb::EntityLife>(p);
            if (l.entity_id() == hero.value && l.delta() < 0) s.hurt_hero = true;
        }
    }
    return s;
}

} // namespace

TEST_CASE("relation table follows KNpcSet::GenOneRelation", "[ai][relation]")
{
    using namespace jx::zone;
    CHECK(g_GenOneRelation(kind_normal, kind_player, camp_animal, camp_begin) == relation_enemy);   // 新手和动物还是战斗关系
    CHECK(g_GenOneRelation(kind_normal, kind_player, camp_justice, camp_begin) == relation_ally);   // everybody helps a beginner
    CHECK(g_GenOneRelation(kind_dialoger, kind_player, camp_animal, camp_free) == relation_dialog);
    CHECK(g_GenOneRelation(kind_normal, kind_player, camp_event, camp_free) == relation_none);
    CHECK(g_GenOneRelation(kind_normal, kind_player, camp_justice, camp_free) == relation_enemy);
    CHECK(g_GenOneRelation(kind_normal, kind_normal, camp_animal, camp_animal) == relation_ally);
    CHECK(g_GenOneRelation(kind_player, kind_player, camp_justice, camp_justice) == relation_ally);
    CHECK(g_GenOneRelation(kind_player, kind_player, camp_free, camp_justice) == relation_enemy);
}

TEST_CASE("an active monster (AIMode 1) hunts the player in sight and strikes", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId boar = w.spawn_npc("boar", Pos{2200, 2000}, 900, 0, jx::zone::KNpcKind::monster);
    w.take_outbox();
    const jx::zone::KNpc* b = w.find_entity(boar);
    REQUIRE(b != nullptr);
    CHECK(b->ai_mode == 1);
    CHECK(b->speed == 6 * 18);            // WalkSpeed units per frame
    CHECK(b->ai_param[10] == 75 * 75);    // the biggest skill radius, squared
    CHECK(b->skills[1].level == 1);

    Seen total;
    for (int i = 0; i < 200 && !total.attacked; ++i) {
        w.tick();
        const Seen s = scan(w, boar, hero);
        total.moved |= s.moved;
        total.attacked |= s.attacked;
    }
    CHECK(total.moved);     // walked toward the hero (beyond the skill radius: FollowAttack)
    REQUIRE(total.attacked);
    CHECK(w.find_entity(boar)->people_id == hero);
    CHECK(w.find_entity(boar)->active_skill_id == 53);
    CHECK(w.find_entity(boar)->cur.attack_radius == 75);

    // the swing lands at 60 % of the frames: the hero loses life
    bool lost = false;
    for (int i = 0; i < 40 && !lost; ++i) {
        w.tick();
        lost = scan(w, boar, hero).hurt_hero;
    }
    CHECK(lost);
    CHECK(w.find_player(7)->life() < w.find_player(7)->life_max());
}

TEST_CASE("a passive monster (AIMode 4) ignores the player until it is hurt, then strikes back", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId boar = w.spawn_npc("boar", Pos{2100, 2000}, 901, 0, jx::zone::KNpcKind::monster);
    w.take_outbox();
    Seen total;
    for (int i = 0; i < 60; ++i) {
        w.tick();
        const Seen s = scan(w, boar, hero);
        total.moved |= s.moved;
        total.attacked |= s.attacked;
    }
    CHECK_FALSE(total.moved);
    CHECK_FALSE(total.attacked);
    CHECK_FALSE(w.find_entity(boar)->people_id.valid());

    // the hero hits it: ReceiveDamage sets m_nPeopleIdx and the boar answers
    REQUIRE(w.attack_request(7, boar, 1));
    bool answered = false;
    for (int i = 0; i < 200 && !answered; ++i) {
        w.tick();
        answered = scan(w, boar, hero).attacked;
        if (w.find_entity(boar)->people_id.valid()) CHECK(w.find_entity(boar)->people_id == hero);
    }
    CHECK(answered);
    CHECK(w.find_entity(boar)->people_id == hero);
}

TEST_CASE("a justice-camp monster lets a beginner be (relation ally)", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId guard = w.spawn_npc("guard", Pos{2150, 2000}, 902, 0, jx::zone::KNpcKind::monster);
    w.take_outbox();
    Seen total;
    for (int i = 0; i < 60; ++i) {
        w.tick();
        total.attacked |= scan(w, guard, hero).attacked;
    }
    CHECK_FALSE(total.attacked);
    CHECK_FALSE(w.find_entity(guard)->people_id.valid());
    CHECK(w.relation(*w.find_entity(guard), *w.find_player(7)) == jx::zone::relation_ally);
}

TEST_CASE("KeepActiveRange walks a pulled monster home and halves its radius meanwhile", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    const EntityId boar = w.spawn_npc("boar", Pos{3000, 3000}, 900, 0, jx::zone::KNpcKind::monster);
    REQUIRE(w.teleport(boar, Pos{3800, 3000}));   // 800 > ActiveRadius 700
    for (int i = 0; i < 3; ++i) w.tick();
    const jx::zone::KNpc* b = w.find_entity(boar);
    CHECK(b->moving);
    CHECK(b->destination() == Pos{3000, 3000});
    CHECK(b->cur.active_radius == 350);
    for (int i = 0; i < 400 && w.find_entity(boar)->moving; ++i) w.tick();
    CHECK(w.find_entity(boar)->pos() == Pos{3000, 3000});
    for (int i = 0; i < 3; ++i) w.tick();
    CHECK(w.find_entity(boar)->cur.active_radius == 700);   // back inside: the full radius again
}

TEST_CASE("a coward (AIMode 3, life below the threshold) flees straight away from the enemy", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId coward = w.spawn_npc("coward", Pos{2200, 2000}, 903, 0, jx::zone::KNpcKind::monster);
    for (int i = 0; i < 3; ++i) w.tick();
    const jx::zone::KNpc* c = w.find_entity(coward);
    CHECK(c->people_id == hero);
    CHECK(c->moving);
    CHECK(c->destination() == Pos{2400, 2000});   // x1 * 2 - x2
}

TEST_CASE("GetNearestNpc takes the enemy the old cell scan meets first", "[ai][world]")
{
    Quiet q;
    KSubWorld w(ai_world());
    EntityId a, b;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "A", Pos{2064, 2000}), a, at) == jx::pb::RESULT_OK);   // cells (+2, 0)
    REQUIRE(w.spawn_player(8, role(80, "B", Pos{2000, 2040}), b, at) == jx::pb::RESULT_OK);   // cells (0, +1): scanned first
    const EntityId boar = w.spawn_npc("boar", Pos{2000, 2000}, 900, 0, jx::zone::KNpcKind::monster);
    w.tick();
    CHECK(w.find_entity(boar)->people_id == b);
}
