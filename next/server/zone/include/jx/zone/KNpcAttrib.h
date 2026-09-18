#pragma once

// The two attribute blocks of KNpc of the old core (KNpc.h: the m_XXX members a npc is born
// with and the m_CurrentXXX members everything works with), laid out as the JX2 server keeps
// them (jx_linux_y, docs/LINUX-SERVER.md §10.1 - the offset of every member is noted so a
// reading of the binary maps straight onto a field).
//
// The base block comes from the npc template (KNpc::SetTemplate 0x08082E20), from the role data
// and the level tables for a player (KPlayer::LoadFrom 0x080C16D0) and from level-ups.  The
// current block is rebuilt from it by KNpc::ClearAttrib (0x0807EE60) and then changed by every
// KMagicAttrib applied - equipment, skills, states - through KNpcAttribModify.
//
// Several current values exist twice: the plain one and its "yan" twin (magic ids 228..245,
// lifemax_yan_v ...).  The value in use is the LARGER of the two (ProcessState clamps life at
// max(+0x1a14, +0x1a18), LevelUp fills life with the same max, OnHurt takes max(+0x1a44,
// +0x1a48)): the twin lets a second source of bonuses coexist without stacking on the first.

#include <algorithm>
#include <array>

#include "jx/zone/KMagicAttrib.h"

namespace jx::zone {

// A {value, extra} pair the anti_* attributes keep (two dwords each, 0x08078F10 clears them).
using KAttribPair = std::array<int, 2>;

// The m_XXX block: KNpc+0x15a8 .. +0x1640.  Defaults are those of KNpc::Init (0x0807DBD0:
// life / mana / stamina / attack rating 100, defence 10) and of a player (0x080A7FF0: walk 5,
// run 10, vision 120).
struct KNpcAttrib {
    int experience = 0;            // m_Experience        +0x15a8  (template x ExpRate / 100)
    int life_max = 100;            // m_LifeMax           +0x15ac
    int life_replenish = 0;        // m_LifeReplenish     +0x15b0
    int mana_max = 100;            // m_ManaMax           +0x15b4
    int mana_replenish = 0;        // m_ManaReplenish     +0x15b8
    int stamina_max = 100;         // m_StaminaMax        +0x15bc
    int stamina_gain = 0;          // m_StaminaGain       +0x15c0  (a player: stamina.ini NormalAdd)
    int attack_rating = 100;       // m_AttackRating      +0x15c4
    int defend = 10;               // m_Defend            +0x15c8
    int fire_resist = 0;           // m_FireResist        +0x15f4
    int cold_resist = 0;           // m_ColdResist        +0x15f8
    int poison_resist = 0;         // m_PoisonResist      +0x15fc
    int light_resist = 0;          // m_LightResist       +0x1600
    int physics_resist = 0;        // m_PhysicsResist     +0x1604
    int fire_resist_max = 0;       // m_FireResistMax     +0x1608  (a player: 75 unless the role data says)
    int cold_resist_max = 0;       // +0x160c
    int poison_resist_max = 0;     // +0x1610
    int light_resist_max = 0;      // +0x1614
    int physics_resist_max = 0;    // +0x1618
    int walk_speed = 5;            // m_WalkSpeed         +0x161c
    int run_speed = 10;            // m_RunSpeed          +0x1620
    int attack_radius = 0;         // m_AttackRadius      +0x1624
    int attack_speed = 0;          // m_AttackSpeed       +0x1628
    int cast_speed = 0;            // m_CastSpeed         +0x162c
    int vision_radius = 0;         // m_VisionRadius      +0x1630
    int active_radius = 0;         // m_ActiveRadius      +0x1638
    int hit_recover = 0;           // m_HitRecover        +0x163c
    int treasure = 0;              // m_Treasure          +0x1640
};

// The m_CurrentXXX block.  Field order follows the binary's offsets so the map in §10.1 reads
// top to bottom.  ClearAttrib() sets it from a KNpcAttrib.
struct KNpcCurrentAttrib {
    int add_boss_damage = 0;                 // add_boss_damage 255          +0x84
    int experience = 0;                      // m_CurrentExperience          +0x1188
    int life = 0;                            // m_CurrentLife                +0x118c
    int life_replenish = 0;                  // m_CurrentLifeReplenish       +0x1190
    int life_replenish_percent = 100;        // lifereplenish_p 190          +0x1194
    int mana_replenish_percent = 100;        // manareplenish_p 248          +0x119c
    int mana = 0;                            // m_CurrentMana                +0x11a0
    int mana_replenish = 0;                  // m_CurrentManaReplenish       +0x11a4
    int stamina = 0;                         // m_CurrentStamina             +0x11a8
    int stamina_max = 0;                     // m_CurrentStaminaMax          +0x11ac
    int stamina_sit_add = 1;                 // max(1, stamina_max x SitAdd / 1000)  +0x11b0
    int stamina_gain = 0;                    // m_CurrentStaminaGain         +0x11b4
    KMagicAttrib physics_damage;             // m_PhysicsDamage {type, min, -, max}  +0x11b8
    KMagicAttrib fire_damage;                // m_CurrentFireDamage          +0x11c8
    KMagicAttrib cold_damage;                // m_CurrentColdDamage          +0x11d8
    KMagicAttrib light_damage;               // m_CurrentLightDamage         +0x11e8
    KMagicAttrib poison_damage;              // m_CurrentPoisonDamage        +0x11f8
    KMagicAttrib physics_magic;              // addphysicsmagic_v 168        +0x1208
    KMagicAttrib cold_magic;                 // addcoldmagic_v 169           +0x1218
    KMagicAttrib light_magic;                // addlightingmagic_v 171       +0x1228
    KMagicAttrib fire_magic;                 // addfiremagic_v 170           +0x1238
    KMagicAttrib poison_magic;               // addpoisonmagic_v 172         +0x1248
    int attack_rating = 0;                   // m_CurrentAttackRating        +0x1258
    int defend = 0;                          // m_CurrentDefend              +0x125c
    int return_res = 0;                      // returnres_p 205              +0x1260
    int melee_return_res = 0;                // melee_returnres_p 299        +0x1264
    int range_return_res = 0;                // range_returnres_p 300        +0x1268
    int fire_resist_max = 0;                 // m_CurrentFireResistMax       +0x126c
    int cold_resist_max = 0;                 // +0x1270
    int poison_resist_max = 0;               // +0x1274
    int light_resist_max = 0;                // +0x1278
    int physics_resist_max = 0;              // +0x127c
    int skill_enhance = 0;                   // skill_enhance 243            +0x1280
    int magic_damage_percent = 0;            // magicdamage_p 244            +0x1284
    int walk_speed = 0;                      // m_CurrentWalkSpeed           +0x1288
    int run_speed = 0;                       // m_CurrentRunSpeed            +0x128c
    int no_move_speed = 0;                   // nomovespeed 182              +0x1294
    int no_move_speed_ex = 0;                // +0x1298
    int attack_radius = 0;                   // m_CurrentAttackRadius        +0x129c
    int vision_radius = 0;                   // m_CurrentVisionRadius        +0x12a4
    int active_radius = 0;                   // m_CurrentActiveRadius        +0x12ac
    KAttribPair anti_hit_recover{};          // anti_hitrecover 219          +0x12b0
    KAttribPair anti_stun_time_reduce{};     // anti_stuntimereduce_p 220    +0x12b8
    std::array<KAttribPair, 5> anti_resist{};      // physics, fire, cold, lighting, poison (224, 222, 225, 223, 221)  +0x12c0
    std::array<KAttribPair, 5> anti_resist_yan{};  // the same for the yan resists (263, 266, 265, 267, 264)  +0x12e8
    KAttribPair anti_sorb_damage_yan{};      // 269                          +0x1310
    KAttribPair anti_block_rate{};           // 270                          +0x1318
    KAttribPair anti_enhance_hit_rate{};     // 271                          +0x1320
    KAttribPair anti_do_stun{};              // 262                          +0x1328
    KAttribPair do_stun{};                   // 261                          +0x1330
    KAttribPair anti_do_hurt{};              // 260                          +0x1338
    KAttribPair do_hurt{};                   // 259                          +0x1340
    KAttribPair anti_poison_time_reduce{};   // 258                          +0x1348
    // +0x1350: the anti pairs against the resist MAXIMUMS, in the order of anti_resist (cleared
    // with the anti_* block; read by the resist of CalcDamage 0x0807BB20, no writer found)
    std::array<KAttribPair, 5> anti_resist_max{};
    int treasure = 0;                        // m_CurrentTreasure            +0x1378
    int melee_damage_return_mana = 0;        // 286                          +0x137c
    int range_damage_return_mana = 0;        // 287                          +0x1380
    int clear_all_cd = 0;                    // 291                          +0x1384
    KAttribPair add_block_rate{};            // 292                          +0x1388
    int walk_run_shadow = 0;                 // 293                          +0x1394
    int mana_to_skill_enhance = 0;           // 298                          +0x1398
    int mana_to_skill_enhance_ex = 0;        // +0x13a0
    int melee_damage_return_percent = 0;     // meleedamagereturn_p 118      +0x13a4
    int melee_damage_return = 0;             // meleedamagereturn_v 117      +0x13a8
    int range_damage_return_percent = 0;     // 120                          +0x13ac
    int range_damage_return = 0;             // 119                          +0x13b0
    int poison_damage_return_percent = 0;    // 194                          +0x13b4
    int poison_damage_return = 0;            // 193                          +0x13b8
    int slow_missle = 0;                     // slowmissle_b 127             +0x13bc
    int status_immunity = 0;                 // statusimmunity_b 174         +0x13c0
    int damage_to_mana_percent = 0;          // damage2addmana_p 134         +0x13cc
    int poison_dec_mana_percent = 0;         // poison2decmana_p 202         +0x13d0
    int mana_shield_percent = 0;             // manashield_p 149             +0x13d4
    int life_stolen = 0;                     // steallife_p 66 / 136         +0x13dc
    int mana_stolen = 0;                     // stealmana_p 67 / 137         +0x13e0
    int stamina_stolen = 0;                  // stealstamina_p 68 / 138      +0x13e4
    int series_res = 0;                      // seriesres_p 177              +0x13e8
    int series_enhance = 0;                  // seriesenhance_p 178          +0x13ec
    int five_elements_enhance = 0;           // 246                          +0x13f0
    int five_elements_resist = 0;            // 247                          +0x13f4
    int knock_back = 0;                      // knockback_p 69 / 145         +0x13f8
    int deadly_strike = 0;                   // deadlystrike_p 70 / 146      +0x13fc
    int stun = 0;                            // stun_p 72                    +0x1400
    int fatally_strike_res = 0;              // fatallystrikeres_p 173       +0x1404
    int block_rate = 0;                      // block_rate 226               +0x1408
    int enhance_hit_rate = 0;                // enhancehit_rate 227          +0x140c
    int enhance_hit_effect_rate = 0;         // enhancehiteffect_rate 275    +0x1410
    int add_damage_percent = 100;            // add_damage_p 249             +0x1414
    int fatally_strike = 0;                  // fatallystrike_p 71 / 152     +0x1418
    int freeze_time_reduce = 0;              // freezetimereduce_p 106       +0x1420
    int poison_time_reduce = 0;              // poisontimereduce_p 108       +0x1424
    int stun_time_reduce = 0;                // stuntimereduce_p 110         +0x1428
    int fire_enhance = 0;                    // fireenhance_p 162            +0x142c
    int cold_enhance = 0;                    // coldenhance_p 161            +0x1430
    int poison_enhance = 0;                  // poisonenhance_p 164          +0x1434
    int light_enhance = 0;                   // lightingenhance_p 163        +0x1438
    int add_physics_damage = 0;              // addphysicsdamage_v 121       +0x143c
    // addphysicsdamage_p 126 by weapon kind: [0..6] +0x1440..+0x1458 (kinds 0..5 and 10 -> 6),
    // [7] +0x145c ranged weapons, [8] +0x1460 bare hands (0x0809A7F0; KPlayer 0x080B0D50 picks
    // the slot of the weapon worn)
    std::array<int, 9> add_physics_damage_percent{};
    int dynamic_magic_shield = 0;            // dynamicmagicshield_v 181     +0x1464
    int static_magic_shield = 0;             // staticmagicshield_v/_p 203/204  +0x1468
    int ignore_skill = 0;                    // ignoreskill_p 191            +0x146c
    int return_skill = 0;                    // returnskill_p 192            +0x1470
    int ignore_negative_state = 0;           // ignorenegativestate_p 201    +0x1474
    bool forbid_attack = false;              // forbit_attack 250 (value == 1)   +0x1478
    bool frozen_action = false;              // frozen_action 251            +0x1479
    bool forbid_medicine = false;            // forbit_takemedicine 252      +0x147a
    bool invincibility = false;              // invincibility 253            +0x147b
    int rand_move = 0;                       // randmove 199                 +0x14c4
    std::array<int, 5> me_to_series_damage{};  // me2{metal,wood,water,fire,earth}damage_p 276..284  +0x15cc
    std::array<int, 5> series_to_me_damage{};  // {..}2medamage_p 277..285   +0x15e0
    int ignore_damage = 0;                   // ignoredamage 217             +0x18e0
    int fire_resist = 0;                     // m_CurrentFireResist          +0x19ec
    int fire_resist_yan = 0;                 // fireres_yan_p 230            +0x19f0
    int cold_resist = 0;                     // +0x19f4
    int cold_resist_yan = 0;                 // 232                          +0x19f8
    int poison_resist = 0;                   // +0x19fc
    int poison_resist_yan = 0;               // 228                          +0x1a00
    int light_resist = 0;                    // +0x1a04
    int light_resist_yan = 0;                // 229                          +0x1a08
    int physics_resist = 0;                  // +0x1a0c
    int physics_resist_yan = 0;              // 231                          +0x1a10
    int life_max = 0;                        // m_CurrentLifeMax             +0x1a14
    int life_max_yan = 0;                    // lifemax_yan_v/_p 233/234     +0x1a18
    int mana_max = 0;                        // m_CurrentManaMax             +0x1a1c
    int mana_max_yan = 0;                    // 235/236                      +0x1a20
    int sorb_damage = 0;                     // sorbdamage_p 218             +0x1a24
    int sorb_damage_yan = 0;                 // 237                          +0x1a28
    int fast_walk_run = 0;                   // fastwalkrun_p 111 (percent)  +0x1a2c
    int fast_walk_run_yan = 0;               // 238                          +0x1a30
    int attack_speed = 0;                    // m_CurrentAttackSpeed         +0x1a34
    int attack_speed_yan = 0;                // 239                          +0x1a38
    int cast_speed = 0;                      // m_CurrentCastSpeed           +0x1a3c
    int cast_speed_yan = 0;                  // 240                          +0x1a40
    int hit_recover = 0;                     // m_CurrentHitRecover          +0x1a44
    int hit_recover_yan = 0;                 // 245                          +0x1a48

    // the values in use: the larger of a pair
    [[nodiscard]] int life_max_v() const noexcept { return std::max(life_max, life_max_yan); }
    [[nodiscard]] int mana_max_v() const noexcept { return std::max(mana_max, mana_max_yan); }
    [[nodiscard]] int fire_resist_v() const noexcept { return std::max(fire_resist, fire_resist_yan); }
    [[nodiscard]] int cold_resist_v() const noexcept { return std::max(cold_resist, cold_resist_yan); }
    [[nodiscard]] int poison_resist_v() const noexcept { return std::max(poison_resist, poison_resist_yan); }
    [[nodiscard]] int light_resist_v() const noexcept { return std::max(light_resist, light_resist_yan); }
    [[nodiscard]] int physics_resist_v() const noexcept { return std::max(physics_resist, physics_resist_yan); }
    [[nodiscard]] int attack_speed_v() const noexcept { return std::max(attack_speed, attack_speed_yan); }
    [[nodiscard]] int cast_speed_v() const noexcept { return std::max(cast_speed, cast_speed_yan); }
    [[nodiscard]] int hit_recover_v() const noexcept { return std::max(hit_recover, hit_recover_yan); }
    [[nodiscard]] int sorb_damage_v() const noexcept { return std::max(sorb_damage, sorb_damage_yan); }
    [[nodiscard]] int fast_walk_run_v() const noexcept { return std::max(fast_walk_run, fast_walk_run_yan); }
    [[nodiscard]] int min_damage() const noexcept { return physics_damage.value[0]; }
    [[nodiscard]] int max_damage() const noexcept { return physics_damage.value[2]; }

    // KNpc::ClearAttrib (0x0807EE60) as far as this block goes: every current value from the base
    // block, the two replenish percents and add_damage_percent at 100, everything else 0.  Life,
    // mana, stamina, the experience, the treasure and the physics damage keep what they were:
    // the binary leaves them alone here (m_PhysicsDamage is rewritten by
    // KPlayer::SetNpcPhysicsDamage right after; a monster's comes with the template).
    void clear(const KNpcAttrib& base, int sit_add_per_mille) noexcept
    {
        const int stamina_now = stamina;
        const int life_now = life, mana_now = mana;
        const KMagicAttrib damage = physics_damage;
        const int exp = experience, treasure_now = treasure, ignore = ignore_damage;
        *this = KNpcCurrentAttrib{};
        experience = exp;
        treasure = treasure_now;      // +0x1378 is set by KNpc::Init, not by ClearAttrib
        ignore_damage = ignore;       // +0x18e0 likewise
        life = life_now;
        mana = mana_now;
        stamina = stamina_now;
        physics_damage = damage;
        stamina_max = base.stamina_max;
        life_max = life_max_yan = base.life_max;
        mana_max = mana_max_yan = base.mana_max;
        set_stamina_sit_add(sit_add_per_mille);
        attack_rating = base.attack_rating;
        attack_speed = attack_speed_yan = base.attack_speed;
        cast_speed = cast_speed_yan = base.cast_speed;
        cold_resist = base.cold_resist;
        cold_resist_max = base.cold_resist_max;
        defend = base.defend;
        fire_resist = base.fire_resist;
        fire_resist_max = base.fire_resist_max;
        hit_recover = hit_recover_yan = base.hit_recover;
        attack_radius = base.attack_radius;
        life_replenish = base.life_replenish;
        light_resist = base.light_resist;
        light_resist_max = base.light_resist_max;
        mana_replenish = base.mana_replenish;
        physics_resist = base.physics_resist;
        physics_resist_max = base.physics_resist_max;
        poison_resist = base.poison_resist;
        poison_resist_max = base.poison_resist_max;
        run_speed = base.run_speed;
        stamina_gain = base.stamina_gain;
        vision_radius = base.vision_radius;
        active_radius = base.active_radius;
        walk_speed = base.walk_speed;
    }

    // +0x11b0 = max(1, m_CurrentStaminaMax x SitAdd / 1000) - after every change of the maximum
    void set_stamina_sit_add(int sit_add_per_mille) noexcept
    {
        stamina_sit_add = std::max(1, stamina_max * sit_add_per_mille / 1000);
    }
};

} // namespace jx::zone
