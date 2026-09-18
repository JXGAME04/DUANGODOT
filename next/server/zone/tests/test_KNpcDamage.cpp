// The blows and the states of KNpc as the JX2 server deals them (jx_linux_y: ReceiveDamage
// 0x0808A4A0, CalcDamage 0x08089C90, AppendSkillEffect 0x0807CE70, SetStateSkillEffect
// 0x08086260, the poison 0x0807BD60, ProcessState 0x0808B610 - docs/LINUX-SERVER.md §12).
// Every number here is worked out by hand from the binary's arithmetic.
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSubWorld.h"

using namespace jx::zone;   // the MAGIC_ATTRIB ids
using jx::EntityId;
using jx::zone::KMagicAttrib;
using jx::zone::KNpc;
using jx::zone::KNpcKind;
using jx::zone::KSkill;
using jx::zone::KSubWorld;
using jx::zone::KSubWorldConfig;
using jx::zone::Pos;

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

KSubWorldConfig small_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;
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

// a player and a monster, both standing still, with the monster's numbers made plain
struct Arena {
    Quiet quiet;
    KSubWorld w{small_world()};
    EntityId hero;
    EntityId pig;
    KNpc* h = nullptr;
    KNpc* p = nullptr;
    Arena()
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
        p->cur.mana_max = p->cur.mana_max_yan = 100;
        p->cur.mana = 100;
        p->cur.defend = 0;
        p->camp = p->current_camp = camp_animal;   // an enemy of a player (g_GenOneRelation)
        // resist maximums above every resist set here, so nothing is softened (0x08078910)
        p->cur.physics_resist_max = p->cur.fire_resist_max = p->cur.cold_resist_max = p->cur.light_resist_max = p->cur.poison_resist_max = 100;
        w.take_outbox();
    }
};

} // namespace

TEST_CASE("the resists of CalcDamage: the larger of yin and yan, softened above the maximum", "[fight]")
{
    Quiet q;
    KNpc t;
    t.cur.physics_resist = 50;
    t.cur.physics_resist_yan = 30;
    t.cur.physics_resist_max = 40;
    // 0x08078910: 40 + (50 - 40) x (95 - 40) / 400 = 41
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_physics, true, false) == 41);
    KNpc a;
    a.cur.anti_resist[0][1] = 20;   // physics: 30 against 30, under the maximum
    CHECK(KSubWorld::calc_resist(t, &a, jx::zone::damage_physics, true, false) == 30);
    a.cur.anti_resist_max[0][1] = 15;   // the maximum drops to 25: 25 + 5 x 70 / 400 = 25
    CHECK(KSubWorld::calc_resist(t, &a, jx::zone::damage_physics, true, false) == 25);
    t.cur.fire_resist = 200;   // a maximum of 0: 0 + 200 x 95 / 400 = 47
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_fire, true, false) == 47);
    t.cur.fire_resist_max = 100;
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_fire, true, false) == 95);   // capped
    t.cur.return_res = 10;
    t.cur.melee_return_res = 5;
    t.cur.range_return_res = 7;
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_return, true, false) == 15);
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_return, false, false) == 17);
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_return, true, true) == 10);   // the returned blow: returnres_p alone
    CHECK(KSubWorld::calc_resist(t, nullptr, jx::zone::damage_magic, true, false) == 0);
}

TEST_CASE("AppendSkillEffect lays the launcher's numbers into the damage slots", "[fight]")
{
    Arena a;
    KNpc& n = *a.p;
    n.base.attack_rating = 200;
    n.cur.attack_rating = 300;
    n.cur.physics_damage.value = {10, 0, 20};
    n.cur.add_physics_damage = 5;
    n.cur.cold_enhance = 7;
    n.cur.cold_damage.value = {0, 0, 0};
    n.cur.knock_back = 3;
    n.cur.deadly_strike = 4;
    n.cur.stun = 6;
    n.cur.fatally_strike = 8;
    n.cur.life_stolen = 2;
    n.cur.poison_damage = attrib(0, 5, 60, 10);   // addpoisondamage_v: the type cell stays 0
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> src{};
    src[0] = attrib(magic_attackrating_p, 50);
    src[2] = attrib(magic_physicsenhance_p, 100);
    src[3] = attrib(magic_colddamage_v, 10, 4, 12);
    src[11] = attrib(magic_knockback_p, 20, 3, 40);
    src[13] = attrib(magic_fatallystrike_p, 1);
    src[14] = attrib(magic_stun_p, 2, 30, 0);
    src[17] = attrib(magic_seriesdamage_p, 9);
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> des{};
    a.w.append_skill_effect(n, true, true, src, des, 0);
    CHECK(des[0].type == magic_seriesdamage_p);
    CHECK(des[0].value[0] == 9);
    CHECK(des[1].type == magic_attackrating_v);
    CHECK(des[1].value[0] == 200 * 50 / 100 + 300);
    CHECK(des[3].type == magic_physicsdamage_v);
    CHECK(des[3].value[0] == 30);   // (100 + 100) x (5 + 10) / 100
    CHECK(des[3].value[2] == 50);   // (100 + 100) x (5 + 20) / 100
    CHECK(des[4].type == magic_colddamage_v);
    CHECK(des[4].value == std::array<int, 3>{10, 11, 12});   // the frames: max(7 + 4, 7 + 0)
    CHECK(des[7].value == std::array<int, 3>{5, 60, 10});    // no poison of its own: the npc's, copied
    CHECK(des[7].type == 0);
    CHECK(des[9].type == magic_steallife_p);
    CHECK(des[9].value[0] == 2);
    CHECK(des[12].value == std::array<int, 3>{23, 3, 40});
    CHECK(des[13].type == magic_deadlystrike_p);
    CHECK(des[13].value[0] == 4);
    // the crossed cells of the binary: fatallystrike_p reads +0x1400 (stun), stun_p reads +0x1418 (fatally strike)
    CHECK(des[14].type == magic_fatallystrike_p);
    CHECK(des[14].value[0] == 6 + 1);
    CHECK(des[15].type == magic_stun_p);
    CHECK(des[15].value == std::array<int, 3>{8 + 2, 30, 0});
    CHECK(des[16].type == magic_addskillexp1);
    CHECK(des[17].type == magic_addskillexp2);

    // with an enhance of 10 percent
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> des2{};
    a.w.append_skill_effect(n, true, true, src, des2, 10);
    CHECK(des2[3].value[0] == 33);
    CHECK(des2[3].value[2] == 55);
    CHECK(des2[4].value[0] == 11);

    // a magic skill: no steals, no deadly strike, the physics magic on top, the energy into the cold
    n.cur.physics_magic.value = {4, 0, 8};
    n.cur.magic_damage_percent = 50;
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> des3{};
    a.w.append_skill_effect(n, false, false, src, des3, 0);
    CHECK(des3[3].value[0] == 30 + 150 * 4 / 100);
    CHECK(des3[3].value[2] == 50 + 150 * 8 / 100);
    CHECK(des3[9].type == 0);
    CHECK(des3[13].type == 0);
    CHECK(des3[4].value[0] == 10);   // cold magic 0, a monster has no energy
    CHECK(des3[7].type == 0);        // the poison magic's type cell is 0: nothing merged
}

TEST_CASE("CalcDamage: resist, shields, sorb, the death at zero", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    KNpc& h = *a.h;
    t.cur.physics_resist = 20;
    int dealt = 0;
    CHECK(a.w.calc_damage(t, h, 50, 50, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(dealt == 40);
    CHECK(t.life() == 60);
    CHECK(t.last_damage_id == a.hero);
    CHECK(t.damage_records[0].player == a.hero);
    CHECK(t.damage_records[0].damage == 40);
    // the static shield takes a smaller blow whole, a larger one breaks it
    t.cur.static_magic_shield = 30;
    CHECK(a.w.calc_damage(t, h, 20, 20, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(t.cur.static_magic_shield == 10);
    CHECK(t.life() == 60);
    CHECK(a.w.calc_damage(t, h, 20, 20, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(t.cur.static_magic_shield == 0);
    CHECK(dealt == 10 * 80 / 100);
    CHECK(t.life() == 52);
    // the mana shield pays its half only when the mana suffices (0x08089EE9)
    t.cur.physics_resist = 0;
    t.cur.mana_shield_percent = 50;
    t.cur.mana = 100;
    CHECK(a.w.calc_damage(t, h, 40, 40, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(dealt == 20);
    CHECK(t.cur.mana == 80);
    CHECK(t.life() == 32);
    t.cur.mana = 5;   // 5 - 10 < 0: the mana goes to 0 and the blow stays whole (0x08089EE9)
    CHECK(a.w.calc_damage(t, h, 20, 20, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(dealt == 20);
    CHECK(t.cur.mana == 0);
    CHECK(t.life() == 12);
    t.cur.mana_shield_percent = 0;
    // sorbdamage_p is per mille
    t.cur.sorb_damage = 100;
    CHECK(a.w.calc_damage(t, h, 10, 10, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(dealt == 9);
    CHECK(t.life() == 3);
    // ignoredamage refuses, an empty blow does nothing
    t.cur.ignore_damage = 1;
    CHECK(a.w.calc_damage(t, h, 10, 10, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 0);
    t.cur.ignore_damage = 0;
    CHECK(a.w.calc_damage(t, h, 0, 0, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(t.life() == 3);
    // exactly zero life kills (JX2: test / jle, unlike the JX1 source)
    t.cur.sorb_damage = 0;
    CHECK(a.w.calc_damage(t, h, 3, 3, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 0);
    CHECK(t.life() == 0);
    CHECK(t.doing == jx::zone::KDoing::death);
}

TEST_CASE("CalcDamage: the damage returned, the mana of the attacker, damage2addmana", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    KNpc& h = *a.h;
    const int hero_life = h.life();
    t.cur.melee_damage_return_percent = 50;
    t.cur.melee_damage_return = 3;
    t.cur.damage_to_mana_percent = 10;
    t.cur.mana = 0;
    int dealt = 0;
    CHECK(a.w.calc_damage(t, h, 40, 40, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(dealt == 40);
    CHECK(h.life() == hero_life - (40 * 50 / 100 + 3));   // the return: a type 6 blow, no resist here
    CHECK(t.cur.mana == 4);                                 // 10 percent of the blow back as mana
    // meleedamagereturnmana_p on the target costs the attacker mana
    h.cur.mana = 50;
    t.cur.melee_damage_return_mana = 20;
    t.cur.melee_damage_return_percent = 0;
    t.cur.melee_damage_return = 0;
    CHECK(a.w.calc_damage(t, h, 10, 10, jx::zone::damage_physics, true, nullptr, &dealt, 0, false) == 1);
    CHECK(h.cur.mana == 48);   // 50 + 10 x (20 / -100)
}

TEST_CASE("ReceiveDamage: the slots in order, the steals, the fatally strike, the stun, the freeze", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    KNpc& h = *a.h;
    h.cur.life = 100;
    h.cur.life_max = h.cur.life_max_yan = 200;
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> d{};
    d[1] = attrib(magic_attackrating_v, 1000);
    d[3] = attrib(magic_physicsdamage_v, 30, 0, 30);
    d[4] = attrib(magic_colddamage_v, 0, 20, 0);          // no cold damage, a freeze of 20 frames
    d[9] = attrib(magic_steallife_p, 50);                  // half the physics dealt
    d[14] = attrib(magic_fatallystrike_p, 100);            // always: a quarter of the life left
    d[15] = attrib(magic_stun_p, 100, 40, 0);              // always: 40 frames
    CHECK(a.w.receive_damage(t, h, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    CHECK(t.people_id == a.hero);
    // 100 - 30 = 70, then the fatally strike: 70 / 4 = 17 -> 53
    CHECK(t.life() == 53);
    CHECK(h.life() == 100 + 15);
    CHECK(t.freeze_state.time == 20);
    CHECK(t.stun_state.time == 40);
    // the refusals
    t.damage_lock = 1;
    CHECK(a.w.receive_damage(t, h, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 0);
    t.damage_lock = 0;
    t.cur.invincibility = true;
    CHECK(a.w.receive_damage(t, h, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 0);
    t.cur.invincibility = false;
    CHECK(t.life() == 53);
    // a block (no state skill 963 without a table: the blow is refused all the same)
    t.cur.block_rate = 100;
    CHECK(a.w.receive_damage(t, h, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 0);
    CHECK(t.life() == 53);
    t.cur.block_rate = 0;
    // add_damage_p of the attacker scales the numbers
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> plain{};
    plain[1] = attrib(magic_attackrating_v, 1000);
    plain[3] = attrib(magic_physicsdamage_v, 10, 0, 10);
    h.cur.add_damage_percent = 150;
    CHECK(a.w.receive_damage(t, h, -1, true, plain.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    CHECK(t.life() == 53 - 15);
}

TEST_CASE("ReceiveDamage: the five elements move the resists for the blow", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    KNpc& h = *a.h;
    t.series = 0;   // metal; wood (1) is what metal beats: g_IsConquer(0, 1) through 0x0830ED2C
    h.series = 3;
    t.cur.physics_resist = 30;
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> d{};
    d[0] = attrib(magic_seriesdamage_p, 25);
    d[1] = attrib(magic_attackrating_v, 1000);
    d[3] = attrib(magic_physicsdamage_v, 100, 0, 100);
    // a metal skill on a wood target: the target loses 25 resist for the blow (30 -> 5 -> 95 percent through)
    t.series = 1;
    CHECK(a.w.receive_damage(t, h, 0, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    CHECK(t.life() == 100 - 95);
    CHECK(t.cur.physics_resist == 30);   // put back
    CHECK(t.cur.fire_resist == 0);
    // a wood skill on a metal target: the target gains 25 (55 percent through)
    t.cur.life = 100;
    t.series = 0;
    CHECK(a.w.receive_damage(t, h, 1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    CHECK(t.life() == 100 - 45);
    CHECK(t.cur.physics_resist == 30);
}

TEST_CASE("SetStateSkillEffect: added, refreshed by the rules, run out, taken off negated", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    const int max_was = t.life_max();
    std::array<KMagicAttrib, 2> states{attrib(magic_lifemax_v, 50, 5, 0), attrib(magic_manamax_v, 20, 5, 0)};
    CHECK(a.w.set_state_skill_effect(t, a.hero, 700, 3, states.data(), 2, 5) == 0);
    CHECK(t.life_max() == max_was + 50);
    CHECK(t.mana_max() == 120);
    REQUIRE(t.state_skills.size() == 1);
    CHECK(t.state_skills[0].skill_id == 700);
    CHECK(t.state_skills[0].level == 3);
    CHECK(t.state_skills[0].left_time == 5);
    CHECK(t.state_skills[0].states[0].value[0] == -50);
    CHECK(t.people_id == a.hero);
    // the same level again: the time is set anew, the numbers are not applied twice
    CHECK(a.w.set_state_skill_effect(t, a.hero, 700, 3, states.data(), 2, 9) == 5);
    CHECK(t.state_skills[0].left_time == 9);
    CHECK(t.life_max() == max_was + 50);
    // a lower level changes nothing, a higher one refreshes the numbers and the level
    CHECK(a.w.set_state_skill_effect(t, a.hero, 700, 2, states.data(), 2, 20) == 9);
    CHECK(t.state_skills[0].level == 3);
    CHECK(t.state_skills[0].left_time == 9);
    std::array<KMagicAttrib, 2> stronger{attrib(magic_lifemax_v, 80, 5, 0), attrib(magic_manamax_v, 20, 5, 0)};
    CHECK(a.w.set_state_skill_effect(t, a.hero, 700, 4, stronger.data(), 2, 20) == 9);
    CHECK(t.state_skills[0].level == 4);
    CHECK(t.state_skills[0].left_time == 9);   // the time stays unless `refresh`
    CHECK(t.life_max() == max_was + 80);
    // a time of 0 ends it at the next frame
    CHECK(a.w.set_state_skill_effect(t, a.hero, 700, 4, stronger.data(), 2, 0) == 9);
    CHECK(t.state_skills[0].left_time == 0);
    a.w.process_frame_state(t, false);
    CHECK(t.state_skills.empty());
    CHECK(t.life_max() == max_was);
    CHECK(t.mana_max() == 100);
    CHECK(t.state_flag == 2);
    // a fresh one runs out by itself: 3 frames, off at the fourth
    CHECK(a.w.set_state_skill_effect(t, a.hero, 701, 1, states.data(), 1, 3) == 0);
    for (int i = 0; i < 3; ++i) a.w.process_frame_state(t, false);
    CHECK(t.state_skills.size() == 1);
    CHECK(t.state_skills[0].left_time == 0);
    a.w.process_frame_state(t, false);
    CHECK(t.state_skills.empty());
    CHECK(t.life_max() == max_was);
    // -1 stays until removed
    CHECK(a.w.set_state_skill_effect(t, a.hero, 702, 1, states.data(), 1, -1) == 0);
    for (int i = 0; i < 50; ++i) a.w.process_frame_state(t, false);
    CHECK(t.state_skills.size() == 1);
    a.w.remove_state_skill_effect(t, 702, false);
    CHECK(t.state_skills.empty());
    CHECK(t.life_max() == max_was);
    CHECK(a.w.set_state_skill_effect(t, a.hero, 0, 1, states.data(), 1, 5) == -1);
}

TEST_CASE("the poison: set, merged, ticking every interval while the sender lives", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    a.w.set_poison(t, a.hero, 5, 30, 10);
    CHECK(t.poison_state.value == 5);
    CHECK(t.poison_state.time == 30);
    CHECK(t.poison_interval == 10);
    CHECK(t.last_poison_id == a.hero);
    // 0x0807BDF8: merged by damage-weighted time, the damage and the interval averaged
    a.w.set_poison(t, a.hero, 7, 20, 10);
    CHECK(t.poison_state.time == (30 * 5 * 10 + 20 * 7 * 10) / (5 * 10 + 7 * 10));
    CHECK(t.poison_state.value == (7 * 20 / 10 + 20 * 5 / 10) / 2);
    CHECK(t.poison_interval == 10);
    // ticks at 20, 10 and 0 of a 30-frame poison of 4 a tick
    t.poison_state = KNpc::PotionState{4, 30};
    t.poison_interval = 10;
    const int life = t.life();
    for (int i = 0; i < 30; ++i) a.w.process_frame_state(t, false);
    CHECK(t.life() == life - 12);
    CHECK(t.poison_state.time == 0);
    // a sender gone (or dead) ends it
    t.poison_state = KNpc::PotionState{4, 30};
    a.h->cur.life = 0;
    a.w.process_frame_state(t, false);
    CHECK(t.poison_state.time == 0);
    CHECK(t.life() == life - 12);
}

TEST_CASE("the freeze takes every other frame, the stun every frame", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    t.freeze_state.time = 4;
    // 4 -> 3 (odd: skipped), 3 -> 2 (not), 2 -> 1 (skipped), 1 -> 0 (not)
    CHECK(a.w.process_frame_state(t, false));
    CHECK_FALSE(a.w.process_frame_state(t, false));
    CHECK(a.w.process_frame_state(t, false));
    CHECK_FALSE(a.w.process_frame_state(t, false));
    CHECK_FALSE(a.w.process_frame_state(t, false));
    t.stun_state.time = 2;
    CHECK(a.w.process_frame_state(t, false));
    CHECK(a.w.process_frame_state(t, false));
    CHECK_FALSE(a.w.process_frame_state(t, false));
}

TEST_CASE("once a second: manatoskill_enhance follows the mana left", "[fight]")
{
    Arena a;
    KNpc& t = *a.p;
    t.cur.mana_to_skill_enhance = 1;
    t.cur.mana_to_skill_enhance_ex = 40;
    t.cur.mana = 50;
    a.w.per_second_attribs(t);
    CHECK(t.mana_skill_enhance == 20);
    t.cur.mana = 0;
    a.w.per_second_attribs(t);
    CHECK(t.mana_skill_enhance == 0);
    CHECK(t.cur.mana_to_skill_enhance_ex == 0);   // 0x0808C058: the percent goes with it
}

TEST_CASE("CastInitiativeSkill lands the blow and the state on an enemy, the payload carries the launcher", "[fight]")
{
    Arena a;
    KNpc& h = *a.h;
    KNpc& t = *a.p;
    KSkill s;
    s.row.id = 500;
    s.row.style = jx::zone::skill_style_initiative_npc_state;
    s.row.target_enemy = true;
    s.row.relation = jx::zone::skill_relation_enemy;
    s.row.is_physical = true;
    s.row.is_melee = true;
    s.row.use_attack_rate = false;
    s.row.series = -1;
    s.level = 2;
    s.damage_attribs[2] = attrib(magic_physicsenhance_p, 0);   // the launcher's own damage
    s.damage_attribs[3] = attrib(magic_colddamage_v, 5, 0, 5);
    s.state_attribs[0] = attrib(magic_lifemax_v, -20, 6, 0);
    s.state_attrib_count = 1;
    s.immediate_attribs[0] = attrib(magic_life_v, -3, 0, 0);
    s.immediate_attrib_count = 1;
    h.cur.physics_damage.value = {10, 0, 10};
    h.skill_enhance[500] = 100;   // the per-skill enhance map: the physics doubles
    const int life = t.life();
    KSubWorld::KCastParams p;
    p.target = a.pig;
    CHECK(a.w.skill_cast(s, h, p));
    CHECK(t.life() == life - 20 - 10 - 3);   // physics x 2, the cold x 2 (the enhance), the immediate life_v
    CHECK(t.cur.life_max == 80);           // lifemax_v -20 (the yan twin keeps the maximum in use at 100)
    REQUIRE(t.state_skills.size() == 1);
    CHECK(t.state_skills[0].skill_id == 500);
    CHECK(t.state_skills[0].level == 2);
    CHECK(t.state_skills[0].left_time == 6);
    CHECK(t.people_id == a.hero);
    // a self cast needs TargetSelf; an ally cast on an enemy is refused (Cast itself still says 1)
    p.target = a.hero;
    CHECK_FALSE(a.w.cast_initiative_skill(s, h, -1, a.hero, 0, 0));
    CHECK(a.w.skill_cast(s, h, p));
    s.row.target_enemy = false;
    s.row.target_ally = true;
    p.target = a.pig;
    CHECK(a.w.skill_cast(s, h, p));   // Cast itself says 1; CastInitiativeSkill did nothing
    CHECK(t.life() == life - 33);
    // a passive skill puts its states on the launcher for good
    KSkill pas;
    pas.row.id = 501;
    pas.row.style = jx::zone::skill_style_passivity_npc_state;
    pas.level = 1;
    pas.state_attribs[0] = attrib(magic_lifemax_v, 30, 1, 0);
    pas.state_attrib_count = 1;
    const int hero_max = h.life_max();
    CHECK(a.w.skill_cast(pas, h, p));
    CHECK(h.life_max() == hero_max + 30);
    REQUIRE(h.state_skills.size() == 1);
    CHECK(h.state_skills[0].left_time == -1);
    // the basic attack without a table: skill 1 with its numbers at 0 - the plain physics blow
    const KSkill* basic = KSkill::basic_attack(1);
    REQUIRE(basic != nullptr);
    CHECK(basic->row.attack_radius == 100);
    CHECK(basic->row.do_hurt == 80);
    CHECK(basic->row.use_attack_rate);
    CHECK(basic->damage_attribs[2].type == magic_physicsenhance_p);
    CHECK(basic->damage_attribs[0].type == magic_attackrating_p);
    CHECK(KSkill::basic_attack(2)->row.attack_radius == 320);
    CHECK(KSkill::basic_attack(3) == nullptr);
}
