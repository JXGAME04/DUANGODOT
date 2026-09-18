// The character's numbers the way the JX2 server computes them (docs/LINUX-SERVER.md §10):
// KNpcAttribModify (one entry per magic id), KNpc::ClearAttrib, KPlayer::LoadFrom / LevelUp /
// AddBaseXXX / UpdataCurData with the level tables of KPlayerSet.  The expected values are the
// ones the binary's formulas give for the Linux server's own tables (level_add.txt, the
// newplayerini00 template: 35 / 25 / 25 / 15, life 204, mana 16).
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "jx/role.pb.h"
#include "jx/zone/KItem.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KNpcAttribModify.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerSet.h"

using jx::zone::KItem;
using jx::zone::KItemList;
using jx::zone::KLevelAddRow;
using jx::zone::KMagicAttrib;
using jx::zone::KNpc;
using jx::zone::KNpcAttribModify;
using jx::zone::KNpcAttribModifyContext;
using jx::zone::KNpcKind;
using jx::zone::KPlayerSet;

namespace {

// the metal (Kim) row of D:\ServerLinux\server1\settings\npc\player\level_add.txt and stamina.ini
KPlayerSet linux_tables()
{
    KPlayerSet t;
    KLevelAddRow kim;
    kim.life_per_level = 4;
    kim.stamina_male_per_level = 9;
    kim.stamina_female_per_level = 8;
    kim.mana_per_level = 1;
    kim.life_per_vitality = 8;
    kim.stamina_per_vitality = 0;
    kim.mana_per_energy = 1;
    kim.lead_exp_share = 25;
    kim.fire_res = -25;
    kim.cold_res = 0;
    kim.poison_res = 25;
    kim.lighting_res = 0;
    kim.physics_res = 0;
    kim.stamina_male_base = 180;
    kim.stamina_female_base = 180;
    t.set_level_add(0, kim);
    for (int level = 1; level <= 200; ++level) t.set_level_exp(level, level == 1 ? 100 : level == 2 ? 500 : 1100);
    jx::zone::KStaminaRule st;
    st.normal_add = 1;
    st.sit_add = 10;
    t.set_stamina(st);
    return t;
}

jx::pb::RoleData shaolin_role()
{
    jx::pb::RoleData r;
    r.set_player_id(1);
    r.set_name("Hero");
    r.set_level(1);
    r.set_series(0);
    r.set_sex(0);
    auto* s = r.mutable_stats();
    s->set_strength(35);
    s->set_dexterity(25);
    s->set_vitality(25);
    s->set_energy(15);
    s->set_hp_max(204);
    s->set_hp(204);
    s->set_mp_max(16);
    s->set_mp(16);
    return r;
}

KNpc player_npc()
{
    KNpc e;
    e.kind = KNpcKind::player;
    e.level = 1;
    e.series = 0;
    e.sex = 0;
    return e;
}

KMagicAttrib attrib(int type, int v0, int v1 = 0, int v2 = 0)
{
    KMagicAttrib m;
    m.type = type;
    m.value = {v0, v1, v2};
    return m;
}

} // namespace

TEST_CASE("KNpcAttribModify: every entry does what its ProcessFunc does", "[player][attrib]")
{
    const KPlayerSet t = linux_tables();
    KNpcAttribModifyContext ctx{&t, nullptr, false};
    KNpc e;
    e.base.life_max = 200;
    e.base.mana_max = 50;
    e.base.stamina_max = 180;
    e.base.attack_rating = 72;
    e.base.walk_speed = 5;
    e.base.run_speed = 10;
    e.clear_attrib(true, 10);
    CHECK(e.cur.life_max == 200);
    CHECK(e.cur.life_max_yan == 200);
    CHECK(e.cur.stamina_sit_add == 1);   // 180 x 10 / 1000 = 1
    CHECK(e.cur.add_damage_percent == 100);
    CHECK(e.cur.life_replenish_percent == 100);

    // _v adds, _p is a percent of the BASE value (0x08098920: v x m_LifeMax / 100)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_lifemax_v, 30), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_lifemax_p, 10), ctx));
    CHECK(e.cur.life_max == 250);
    CHECK(e.cur.life_max_yan == 200);
    CHECK(e.life_max() == 250);   // the larger twin is the one in use
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_lifemax_yan_v, 100), ctx));
    CHECK(e.life_max() == 300);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_manamax_p, 50), ctx));
    CHECK(e.cur.mana_max == 75);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_attackrating_p, 50), ctx));
    CHECK(e.cur.attack_rating == 72 + 36);
    // stamina max recomputes the sit regeneration (0x080982A0)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_staminamax_v, 320), ctx));
    CHECK(e.cur.stamina_max == 500);
    CHECK(e.cur.stamina_sit_add == 5);
    // cold damage: min and max, and a freeze time of min(54, 4 x (v / 5) + 10) (0x08098FA0)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addcolddamage_v, 23), ctx));
    CHECK(e.cur.cold_damage.value[0] == 23);
    CHECK(e.cur.cold_damage.value[2] == 23);
    CHECK(e.cur.cold_damage.value[1] == 4 * 4 + 10);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addcolddamage_v, 100), ctx));
    CHECK(e.cur.cold_damage.value[1] == 54);
    // poison: 60 frames every 10 (0x08098E90)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addpoisondamage_v, 7), ctx));
    CHECK(e.cur.poison_damage.value[0] == 7);
    CHECK(e.cur.poison_damage.value[1] == 60);
    CHECK(e.cur.poison_damage.value[2] == 10);
    // sorbdamage is held at 0..500 (0x08096D10)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_sorbdamage_p, 600), ctx));
    CHECK(e.cur.sorb_damage == 500);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_sorbdamage_p, -900), ctx));
    CHECK(e.cur.sorb_damage == 0);
    // an anti pair keeps the sum in both cells (0x08096CD0)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_anti_hitrecover, 7), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_anti_hitrecover, 5), ctx));
    CHECK(e.cur.anti_hit_recover[0] == 12);
    CHECK(e.cur.anti_hit_recover[1] == 12);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_anti_maxres_p, 3), ctx));
    CHECK(e.cur.anti_resist[4][1] == 3);
    // the flags: set when the value is exactly 1 (sete after cmp 1)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_forbit_takemedicine, 1), ctx));
    CHECK(e.cur.forbid_medicine);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_forbit_takemedicine, 2), ctx));
    CHECK_FALSE(e.cur.forbid_medicine);
    // add_boss_damage is set, never added; ignoredamage is a boolean of v > 0
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_add_boss_damage, 40), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_add_boss_damage, 30), ctx));
    CHECK(e.cur.add_boss_damage == 30);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_add_boss_damage, -1), ctx));
    CHECK(e.cur.add_boss_damage == 0);
    // addphysicsdamage_p goes by weapon kind: 0..5 straight, 10 -> slot 6, others ignored
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addphysicsdamage_p, 15, 0, 10), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addphysicsdamage_p, 5, 0, -2), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_addphysicsdamage_p, 9, 0, 7), ctx));
    CHECK(e.cur.add_physics_damage_percent[6] == 15);
    CHECK(e.cur.add_physics_damage_percent[2] == 5);
    // fastwalkrun_p: base x max(p, yan) / 100 on both speeds (0x08098A50)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_fastwalkrun_p, 40), ctx));
    CHECK(e.cur.walk_speed == 5 + 2);
    CHECK(e.cur.run_speed == 10 + 4);
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_fastwalkrun_yan_p, 100), ctx));
    CHECK(e.cur.run_speed == 20);   // the yan 100 % replaces the 40 %, nothing stacks
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_fastwalkrun_p, -40), ctx));
    CHECK(e.cur.run_speed == 20);
    // the potion state: value = (x1 t1 + x2 t2) / max(t1, t2) (0x08097E70)
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_lifepotion_v, 10, 100), ctx));
    CHECK(KNpcAttribModify::modify(e, attrib(jx::zone::magic_lifepotion_v, 20, 50), ctx));
    CHECK(e.life_state.time == 100);
    CHECK(e.life_state.value == (10 * 100 + 20 * 50) / 100);
    // unknown ids are reported, not applied
    CHECK_FALSE(KNpcAttribModify::modify(e, attrib(jx::zone::magic_metalskill_v, 1), ctx));
    CHECK_FALSE(KNpcAttribModify::modify(e, attrib(999, 1), ctx));
    CHECK(KNpcAttribModify::handled(jx::zone::magic_lifemax_v));
    CHECK_FALSE(KNpcAttribModify::handled(jx::zone::magic_execscript));
}

TEST_CASE("KPlayer::LoadFrom turns the Shaolin template into the binary's numbers", "[player]")
{
    const KPlayerSet t = linux_tables();
    KNpc e = player_npc();
    e.player.load_from(e, shaolin_role(), t, nullptr);
    CHECK(e.player.strength == 35);
    CHECK(e.player.cur_dexterity == 25);
    CHECK(e.base.attack_rating == 25 * 4 - 28);     // SetNpcAttackRating
    CHECK(e.cur.attack_rating == 72);
    CHECK(e.base.defend == 25 / 4);                 // SetNpcDefence
    CHECK(e.cur.physics_damage.value[0] == 35 / 5 + 1);   // bare hands
    CHECK(e.cur.physics_damage.value[2] == 8);
    CHECK(e.base.life_max == 204);                  // TRoleData's m_LifeMax as it is
    CHECK(e.life_max() == 204);
    CHECK(e.life() == 204);
    CHECK(e.base.mana_max == 16);
    CHECK(e.base.stamina_max == 180);               // GetStaminaBase(metal, male, 1)
    CHECK(e.cur.stamina == 180);
    CHECK(e.cur.stamina_sit_add == 1);
    CHECK(e.cur.stamina_gain == 1);                 // NormalAdd
    CHECK(e.base.fire_resist == -25 * 1 / 100);     // the level's resist: -25 x 1 / 100 = 0 at level 1
    CHECK(e.base.fire_resist_max == 75);
    CHECK(e.base.vision_radius == 120);
    CHECK(e.player.next_level_exp == 100);
    CHECK(e.player.loaded);

    // at level 40 the metal character has -10 fire and +10 poison
    e.level = 40;
    e.player.load_from(e, shaolin_role(), t, nullptr);
    CHECK(e.cur.fire_resist == -10);
    CHECK(e.cur.poison_resist == 10);
    CHECK(e.base.stamina_max == 180 + 39 * 9);

    // the record written back carries the base numbers, not the equipment's
    jx::pb::RoleData out;
    e.player.save_to(e, out);
    CHECK(out.stats().hp_max() == 204);
    CHECK(out.stats().strength() == 35);
    CHECK(out.level() == 40);
}

TEST_CASE("KPlayer::LevelUp and the attribute points follow level_add.txt", "[player]")
{
    const KPlayerSet t = linux_tables();
    KNpc e = player_npc();
    e.player.load_from(e, shaolin_role(), t, nullptr);
    e.cur.life = 10;
    REQUIRE(e.player.level_up(e, true, t, nullptr));
    CHECK(e.level == 2);
    CHECK(e.player.attribute_point == 5);
    CHECK(e.player.skill_point == 1);
    CHECK(e.player.exp == 0);
    CHECK(e.player.next_level_exp == 500);
    CHECK(e.base.life_max == 204 + 4);
    CHECK(e.base.mana_max == 16 + 1);
    CHECK(e.base.stamina_max == 180 + 9);
    CHECK(e.life() == 208);          // filled to the maximum
    CHECK(e.cur.stamina == 189);
    // a level down takes it all back
    REQUIRE(e.player.level_up(e, false, t, nullptr));
    CHECK(e.level == 1);
    CHECK(e.player.attribute_point == 0);
    CHECK(e.base.life_max == 204);
    CHECK_FALSE(e.player.level_up(e, false, t, nullptr));   // level 1 stays

    // five points: vitality gives LifePerVitality (8) life each, energy ManaPerEnergy (1) mana,
    // dexterity 4 attack rating and a quarter of a defence point
    e.player.attribute_point = 10;
    REQUIRE(e.player.add_base_vitality(e, 3, true, t, nullptr));
    CHECK(e.base.life_max == 204 + 24);
    CHECK(e.life_max() == 228);
    CHECK(e.player.attribute_point == 7);
    REQUIRE(e.player.add_base_energy(e, 2, true, t, nullptr));
    CHECK(e.base.mana_max == 18);
    REQUIRE(e.player.add_base_dexterity(e, 4, true, t, nullptr));
    CHECK(e.base.attack_rating == 29 * 4 - 28);
    CHECK(e.base.defend == 29 / 4);
    REQUIRE(e.player.add_base_strength(e, 1, true, t, nullptr));
    CHECK(e.cur.physics_damage.value[0] == 36 / 5 + 1);
    CHECK(e.player.attribute_point == 0);
    CHECK_FALSE(e.player.add_base_strength(e, 1, true, t, nullptr));   // no points left
    CHECK(e.player.strength == 36);
    REQUIRE(e.player.add_base_strength(e, 4, false, t, nullptr));     // unchecked (a script) goes negative
    CHECK(e.player.attribute_point == -4);
}

TEST_CASE("the worn weapon and armour reach the npc through UpdataCurData", "[player][item]")
{
    const KPlayerSet t = linux_tables();
    static jx::zone::KItemTemplate sword_row, armour_row;
    sword_row.genre = jx::zone::KItemGenre::equip;
    sword_row.detail = jx::zone::equip_meleeweapon;
    sword_row.width = 1;
    sword_row.height = 3;
    armour_row.genre = jx::zone::KItemGenre::equip;
    armour_row.detail = jx::zone::equip_armor;
    armour_row.width = 2;
    armour_row.height = 3;

    KItem sword;
    sword.genre = jx::zone::KItemGenre::equip;
    sword.detail = jx::zone::equip_meleeweapon;
    sword.series = 0;
    sword.tpl = &sword_row;
    sword.base[0] = attrib(jx::zone::magic_weapondamagemin_v, 5);
    sword.base[1] = attrib(jx::zone::magic_weapondamagemax_v, 9);
    sword.magic[0] = attrib(jx::zone::magic_weapondamageenhance_p, 100);   // a prefix: always on
    sword.magic[1] = attrib(jx::zone::magic_lifemax_v, 50);                 // a suffix: needs an awake slot
    KItem armour;
    armour.genre = jx::zone::KItemGenre::equip;
    armour.detail = jx::zone::equip_armor;
    armour.series = 2;   // water feeds wood, not metal: nothing wakes up on a metal character
    armour.tpl = &armour_row;
    armour.base[0] = attrib(jx::zone::magic_armordefense_v, 12);
    armour.magic[0] = attrib(jx::zone::magic_strength_v, 10);

    KItemList list;
    const std::uint32_t sid = list.add(sword, jx::zone::room_equipment);
    const std::uint32_t aid = list.add(armour, jx::zone::room_equipment);
    REQUIRE(sid != 0);
    REQUIRE(aid != 0);
    REQUIRE(list.wear(sid, jx::zone::itempart_weapon));
    REQUIRE(list.wear(aid, jx::zone::itempart_body));

    KNpc e = player_npc();
    e.player.load_from(e, shaolin_role(), t, &list);
    // the weapon: (5, 9) x (100 + 100) / 100 = (10, 18), plus strength 45 / 5 = 9 (the armour's
    // +10 strength counts: it is applied before SetNpcPhysicsDamage runs)
    CHECK(e.player.cur_strength == 45);
    CHECK(e.player.strength == 35);
    CHECK(e.cur.physics_damage.value[0] == 10 + 9);
    CHECK(e.cur.physics_damage.value[2] == 18 + 9);
    CHECK(e.cur.defend == 6 + 12);
    CHECK(e.cur.life_max == 204);   // the sword's suffix stays asleep (metal sword on a metal character: no feeding)
    // taking the armour off and recalculating: the strength and the defence go with it
    REQUIRE(list.unequip(jx::zone::itempart_body));
    e.player.updata_cur_data(e, false, t, &list);
    CHECK(e.player.cur_strength == 35);
    CHECK(e.cur.physics_damage.value[0] == 10 + 7);
    CHECK(e.cur.defend == 6);
}

TEST_CASE("KPlayerSet reads player.json and follows the level rules", "[player][tables]")
{
    const std::filesystem::path p = std::filesystem::temp_directory_path() / "jx_player_test.json";
    {
        std::ofstream out(p, std::ios::binary);
        out << R"({"level_exp": [{"exp": 100, "reborn": [1, 2, 3, 4, 5, 6, 7]}, {"exp": 500, "reborn": [10, 20, 30, 40, 50, 60, 70]}],
                   "level_add": [{"life_per_level": 4, "stamina_male_per_level": 9, "stamina_female_per_level": 8, "mana_per_level": 1,
                                  "life_per_vitality": 8, "mana_per_energy": 1, "fire_res": -25, "poison_res": 25,
                                  "stamina_male_base": 180, "stamina_female_base": 170}],
                   "stamina": {"normal_add": 1, "sit_add": 10, "kill_run_sub": 18},
                   "basevalue": {"hurt_frame": 12, "attack_frame": 18, "cast_frame": 18}})";
    }
    KPlayerSet t;
    std::string err;
    REQUIRE(t.load(p.string(), &err));
    CHECK(t.loaded());
    CHECK(t.level_exp(1) == 100);
    CHECK(t.level_exp(2, 3) == 30);
    CHECK(t.level_exp(0) == -1);
    CHECK(t.level_exp(2, 8) == -1);
    CHECK(t.level_exp(3) == 0);   // rows the file did not carry
    CHECK(t.life_per_vitality(0) == 8);
    CHECK(t.life_per_vitality(4) == 0);
    CHECK(t.stamina_base(0, 1, 11) == 170 + 10 * 8);
    CHECK(t.stamina_base(5, 0, 1) == 0);
    CHECK(t.fire_resist(0, 160, false) == -30);   // a negative per-level value is held at 120
    CHECK(t.poison_resist(0, 160, false) == 40);  // a positive one is not
    CHECK(t.fire_resist(0, 40, true) == 0);       // the reborn floor
    CHECK(t.stamina().sit_add == 10);
    CHECK(t.stamina().exercise_run_sub == 6);     // the default of a missing key
    CHECK(t.base_value().attack_frame == 18);
    CHECK(t.base_value().walk_speed == 5);
    std::filesystem::remove(p);
    KPlayerSet none;
    CHECK_FALSE(none.load("no-such-file.json", &err));
    CHECK_FALSE(err.empty());
}
