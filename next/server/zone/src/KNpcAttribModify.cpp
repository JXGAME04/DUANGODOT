#include "jx/zone/KNpcAttribModify.h"

#include <algorithm>
#include <cstdlib>

#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayerSet.h"

namespace jx::zone {

namespace {

// the percent of a base value the binary computes: v x base / 100 with the sign of C (idiv)
int percent_of(int base, int v) noexcept { return base * v / 100; }

// value / 5 the way the compiler emits it (0x66666667, sar 1): toward zero
int div5(int v) noexcept { return v / 5; }

// a KAttribPair keeps the running sum in both cells (anti_hitrecover 0x08096CD0 ...)
void add_pair(KAttribPair& p, int v) noexcept
{
    p[0] += v;
    p[1] = p[0];
}

// fastwalkrun_p (0x08098A50) and its yan twin (0x08095EF0): take the old bonus out of the two
// speeds (base x max(p, yan) / 100), move the percent, put the new bonus in
void fast_walk_run(KNpc& npc, int v, bool yan) noexcept
{
    KNpcCurrentAttrib& c = npc.cur;
    const int old = std::max(c.fast_walk_run, c.fast_walk_run_yan);
    c.walk_speed -= percent_of(npc.base.walk_speed, old);
    c.run_speed -= percent_of(npc.base.run_speed, old);
    if (yan) c.fast_walk_run_yan += v; else c.fast_walk_run += v;
    const int now = std::max(c.fast_walk_run, c.fast_walk_run_yan);
    c.walk_speed += percent_of(npc.base.walk_speed, now);
    c.run_speed += percent_of(npc.base.run_speed, now);
}

} // namespace

bool KNpcAttribModify::modify(KNpc& npc, const KMagicAttrib& m, const KNpcAttribModifyContext& ctx)
{
    KNpcCurrentAttrib& c = npc.cur;
    const KNpcAttrib& b = npc.base;
    const int v0 = m.value[0], v1 = m.value[1], v2 = m.value[2];
    switch (m.type) {
    // ---- defence / attack rating
    case magic_armordefense_v:            // 30 (0x08098000), the same entry serves adddefense_v 150
    case magic_adddefense_v: c.defend += v0; return true;
    case magic_attackrating_v:            // 56 (0x08098D30) = attackratingenhance_v 166
    case magic_attackratingenhance_v: c.attack_rating += v0; return true;
    case magic_attackrating_p:            // 57 (0x08098D70) = 167: percent of m_AttackRating
    case magic_attackratingenhance_p: c.attack_rating += percent_of(b.attack_rating, v0); return true;
    // ---- steals, strike chances
    case magic_steallife_p: case magic_steallifeenhance_p: c.life_stolen += v0; return true;          // 66 / 136 (0x080981E0)
    case magic_stealmana_p: case magic_stealmanaenhance_p: c.mana_stolen += v0; return true;          // 67 / 137
    case magic_stealstamina_p: case magic_stealstaminaenhance_p: c.stamina_stolen += v0; return true; // 68 / 138
    case magic_knockback_p: case magic_knockbackenhance_p: c.knock_back += v0; return true;           // 69 / 145 (0x080989A0)
    case magic_deadlystrike_p: case magic_deadlystrikeenhance_p: c.deadly_strike += v0; return true;  // 70 / 146 (0x08098BD0)
    case magic_fatallystrike_p: case magic_fatallystrikeenhance_p: c.fatally_strike += v0; return true;   // 71 / 152 (0x08097B90)
    case magic_stun_p: c.stun += v0; return true;                                                     // 72 (0x08097B10)
    case magic_fatallystrikeres_p: c.fatally_strike_res += v0; return true;                            // 173 (0x08097B50)
    // ---- life / mana / stamina
    case magic_lifemax_v: c.life_max += v0; return true;                                             // 85 (0x080988E0)
    case magic_lifemax_p: c.life_max += percent_of(b.life_max, v0); return true;                     // 86 (0x08098920)
    case magic_life_v: c.life += v0; return true;                                                    // 87 (0x08098840)
    case magic_lifereplenish_v: c.life_replenish += v0; return true;                                 // 88 (0x08098880)
    case magic_manamax_v: c.mana_max += v0; return true;                                             // 89 (0x08098740)
    case magic_manamax_p: c.mana_max += percent_of(b.mana_max, v0); return true;                     // 90 (0x08098780)
    case magic_mana_v: c.mana += v0; return true;                                                    // 91 (0x080986C0)
    case magic_manareplenish_v: c.mana_replenish += v0; return true;                                 // 92 (0x08098700)
    case magic_staminamax_v:                                                                          // 93 (0x080982A0)
        c.stamina_max += v0;
        c.set_stamina_sit_add(ctx.tables ? ctx.tables->stamina().sit_add : KStaminaRule{}.sit_add);
        return true;
    case magic_staminamax_p:                                                                          // 94 (0x08098310)
        c.stamina_max += percent_of(b.stamina_max, v0);
        c.set_stamina_sit_add(ctx.tables ? ctx.tables->stamina().sit_add : KStaminaRule{}.sit_add);
        return true;
    case magic_stamina_v: c.stamina += v0; return true;                                              // 95 (0x08098220)
    case magic_staminareplenish_v: c.stamina_gain += v0; return true;                                // 96 (0x08098260)
    // ---- the five points (players only: KPlayer::ChangeCurXXX, 0x0809A100 ..)
    case magic_strength_v:
        if (npc.kind != KNpcKind::player) return true;
        npc.player.change_cur_strength(npc, v0, ctx.items);
        return true;
    case magic_dexterity_v:
        if (npc.kind != KNpcKind::player) return true;
        npc.player.change_cur_dexterity(npc, v0, ctx.items);
        return true;
    case magic_vitality_v:
        if (npc.kind != KNpcKind::player || ctx.tables == nullptr) return true;
        npc.player.change_cur_vitality(npc, v0, *ctx.tables);
        return true;
    case magic_energy_v:
        if (npc.kind != KNpcKind::player || ctx.tables == nullptr) return true;
        npc.player.change_cur_energy(npc, v0, *ctx.tables);
        return true;
    case magic_expenhance_v:               // 175 (0x08099F40 -> 0x080A82A0): a random range on every gain
        if (npc.kind != KNpcKind::player) return true;
        npc.player.exp_enhance_lo += v0;
        npc.player.exp_enhance_hi += v2;
        return true;
    case magic_expenhance_p:               // 176 (-> 0x080A82C0)
        if (npc.kind != KNpcKind::player) return true;
        npc.player.exp_enhance_percent += v0;
        return true;
    case magic_add120skillexpenhance_p:    // 206 (-> 0x080A82E0)
        if (npc.kind != KNpcKind::player) return true;
        npc.player.exp_enhance_percent2 += v0;
        return true;
    case magic_lucky_v:                    // 135 (0x0809A180): KPlayer::m_nCurLucky (through 0x080B0Cxx)
        if (npc.kind != KNpcKind::player) return true;
        npc.player.cur_lucky += v0;
        return true;
    // ---- resistances (current, the yan twins, the maxima)
    case magic_poisonres_p: c.poison_resist += v0; return true;      // 101 (0x080984C0)
    case magic_fireres_p: c.fire_resist += v0; return true;          // 102 (0x08098040)
    case magic_lightingres_p: c.light_resist += v0; return true;     // 103 (0x08098800)
    case magic_physicsres_p: c.physics_resist += v0; return true;    // 104 (0x08098590)
    case magic_coldres_p: c.cold_resist += v0; return true;          // 105 (0x08098C10)
    case magic_allres_p:                                             // 114 (0x08098DF0)
        c.fire_resist += v0; c.cold_resist += v0; c.light_resist += v0; c.poison_resist += v0; c.physics_resist += v0;
        return true;
    case magic_poisonres_yan_p: c.poison_resist_yan += v0; return true;    // 228 (0x08095D70)
    case magic_lightingres_yan_p: c.light_resist_yan += v0; return true;   // 229
    case magic_fireres_yan_p: c.fire_resist_yan += v0; return true;        // 230
    case magic_physicsres_yan_p: c.physics_resist_yan += v0; return true;  // 231
    case magic_coldres_yan_p: c.cold_resist_yan += v0; return true;        // 232
    case magic_allres_yan_p:                                                // 241 (0x08096020)
        c.fire_resist_yan += v0; c.cold_resist_yan += v0; c.light_resist_yan += v0; c.poison_resist_yan += v0; c.physics_resist_yan += v0;
        return true;
    case magic_physicsresmax_p: c.physics_resist_max += v0; return true;   // 155 (0x08097DA0)
    case magic_coldresmax_p: c.cold_resist_max += v0; return true;         // 156
    case magic_fireresmax_p: c.fire_resist_max += v0; return true;         // 157
    case magic_lightingresmax_p: c.light_resist_max += v0; return true;    // 158
    case magic_poisonresmax_p: c.poison_resist_max += v0; return true;     // 159
    case magic_allresmax_p:                                                 // 160 (0x08097BD0)
        c.fire_resist_max += v0; c.cold_resist_max += v0; c.light_resist_max += v0; c.poison_resist_max += v0; c.physics_resist_max += v0;
        return true;
    // ---- time reductions, enhances
    case magic_freezetimereduce_p: c.freeze_time_reduce += v0; return true;   // 106 (0x080989E0)
    case magic_burntimereduce_p: return true;                                 // 107 (0x08095BD0): an empty entry
    case magic_poisontimereduce_p: c.poison_time_reduce += v0; return true;   // 108 (0x08098480)
    case magic_poisondamagereduce_v:                                          // 109 (0x08098500): eats the poison state's damage
        npc.poison_state.value -= v0;
        if (npc.poison_state.value <= 0) npc.poison_state.time = 0;
        return true;
    case magic_stuntimereduce_p: c.stun_time_reduce += v0; return true;       // 110 (0x08098120)
    case magic_fastwalkrun_p: fast_walk_run(npc, v0, false); return true;     // 111 (0x08098A50)
    case magic_fastwalkrun_yan_p: fast_walk_run(npc, v0, true); return true;  // 238 (0x08095EF0)
    case magic_visionradius_p: c.vision_radius += v0; return true;            // 112 (0x080980E0)
    case magic_fasthitrecover_v: c.hit_recover += v0; return true;            // 113 (0x08098B60)
    case magic_fasthitrecover_yan_v: c.hit_recover_yan += v0; return true;    // 245 (0x080960D0)
    case magic_attackspeed_v: c.attack_speed += v0; return true;              // 115 (0x08098CF0)
    case magic_castspeed_v: c.cast_speed += v0; return true;                  // 116 (0x08098CB0)
    case magic_attackspeed_yan_v: c.attack_speed_yan += v0; return true;      // 239
    case magic_castspeed_yan_v: c.cast_speed_yan += v0; return true;          // 240
    case magic_meleedamagereturn_v: c.melee_damage_return += v0; return true;             // 117 (0x08098600)
    case magic_meleedamagereturn_p: c.melee_damage_return_percent += v0; return true;     // 118 (0x08098640)
    case magic_rangedamagereturn_v: c.range_damage_return += v0; return true;             // 119 (0x08098440)
    case magic_rangedamagereturn_p: c.range_damage_return_percent += v0; return true;     // 120 (0x08098400)
    case magic_poisondamagereturn_v: c.poison_damage_return += v0; return true;           // 193 (0x0809A6F0)
    case magic_poisondamagereturn_p: c.poison_damage_return_percent += v0; return true;   // 194 (0x0809A5F0)
    // ---- damages
    case magic_addphysicsdamage_v: c.add_physics_damage += v0; return true;   // 121 (0x08098E50)
    case magic_addfiredamage_v:                                               // 122 (0x08098F60): min and max
        c.fire_damage.value[0] += v0; c.fire_damage.value[2] += v0;
        return true;
    case magic_addcolddamage_v: {                                             // 123 (0x08098FA0)
        c.cold_damage.value[0] += v0;
        c.cold_damage.value[2] += v0;
        if (c.cold_damage.value[0] > 0 && c.cold_damage.value[2] > 0) {
            const int freeze = std::min(0x36, div5(v0) * 4 + 10);   // the freeze time of the blow
            if (c.cold_damage.value[1] < freeze) c.cold_damage.value[1] = freeze;
        }
        return true;
    }
    case magic_addlightingdamage_v:                                           // 124 (0x08098F20)
        c.light_damage.value[0] += v0; c.light_damage.value[2] += v0;
        return true;
    case magic_addpoisondamage_v:                                             // 125 (0x08098E90): a poison of 60 frames every 10
        c.poison_damage.value[0] += v0;
        if (c.poison_damage.value[0] > 0) {
            c.poison_damage.value[1] = 0x3c;
            c.poison_damage.value[2] = 0xa;
        }
        return true;
    case magic_addphysicsdamage_p: {                                          // 126 (0x0809A7F0): percent by weapon kind |value[2]|
        // the jump table 0x082557E0 over kinds 0..10 and the map {0..5 -> 0..5, 10 -> 6} of
        // the cells +0x1440..+0x1458; 7 is the ranged cell +0x145c, 8 the bare hands +0x1460
        auto& a = c.add_physics_damage_percent;
        switch (std::abs(v2)) {
        case 0: case 1: case 2: case 3: case 4: case 5: a[static_cast<std::size_t>(std::abs(v2))] += v0; break;
        case 10: a[6] += v0; break;
        case 6:                                                               // 0x0809AE02: every kind, bare hands and ranged
            for (std::size_t i = 0; i < 7; ++i) a[i] += v0;
            a[8] += v0;
            a[7] += v0;
            break;
        case 7: a[7] += v0; break;                                            // 0x0809ACBC: the ranged weapons
        case 8: for (std::size_t i = 0; i < 7; ++i) a[i] += v0; break;       // 0x0809ACEB: every melee kind
        case 9: a[8] += v0; a[6] += v0; break;                                // 0x0809AD28: bare hands, then the map's kind 10
        default: break;                                                       // above 10: nothing (0x0809AA30)
        }
        return true;
    }
    case magic_addphysicsmagic_v: c.physics_magic.value[0] += v0; c.physics_magic.value[2] += v0; return true;   // 168 (0x08097A10)
    case magic_addcoldmagic_v: c.cold_magic.value[0] += v0; c.cold_magic.value[2] += v0; return true;            // 169 (0x08097AD0)
    case magic_addfiremagic_v: c.fire_magic.value[0] += v0; c.fire_magic.value[2] += v0; return true;            // 170 (0x08097A90)
    case magic_addlightingmagic_v: c.light_magic.value[0] += v0; c.light_magic.value[2] += v0; return true;      // 171 (0x08097A50)
    case magic_addpoisonmagic_v:                                                                                  // 172 (0x08097980)
        c.poison_magic.value[0] += v0;
        if (c.poison_magic.value[0] > 0) {
            c.poison_magic.value[1] = 0x3c;
            c.poison_magic.value[2] = 0xa;
        }
        return true;
    // ---- flags
    case magic_slowmissle_b: c.slow_missle = v0 > 0 ? 1 : 0; return true;        // 127 (0x080983C0)
    case magic_changecamp_b:                                                     // 128 (0x08098C50): not a player; 1..6 or back to m_Camp
        if (npc.kind == KNpcKind::player) return true;
        npc.current_camp = (v0 >= 1 && v0 <= 6) ? v0 : npc.camp;
        return true;
    case magic_damage2addmana_p: c.damage_to_mana_percent += v0; return true;     // 134 (0x08098550)
    case magic_manashield_p: c.mana_shield_percent += v0; return true;            // 149 (0x08098680)
    case magic_lifepotion_v:                                                      // 153 (0x08097E70): the potion state (KSubWorld applies it per tick)
        if (v1 > 0) {
            const int time = std::max(npc.life_state.time, v1);
            npc.life_state.value = (v0 * v1 + npc.life_state.value * npc.life_state.time) / time;
            npc.life_state.time = time;
        }
        return true;
    case magic_manapotion_v:                                                      // 154 (0x08097DE0)
        if (v1 > 0) {
            const int time = std::max(npc.mana_state.time, v1);
            npc.mana_state.value = (v0 * v1 + npc.mana_state.value * npc.mana_state.time) / time;
            npc.mana_state.time = time;
        }
        return true;
    case magic_coldenhance_p: c.cold_enhance += v0; return true;      // 161 (0x08097FC0)
    case magic_fireenhance_p: c.fire_enhance += v0; return true;      // 162 (0x08097F80)
    case magic_lightingenhance_p: c.light_enhance += v0; return true; // 163 (0x08097F40)
    case magic_poisonenhance_p: c.poison_enhance += v0; return true;  // 164 (0x08097F00)
    case magic_statusimmunity_b: c.status_immunity = v0 > 0 ? 1 : 0; return true;   // 174 (0x08097940)
    case magic_seriesres_p: c.series_res += v0; return true;          // 177 (0x08095C80)
    case magic_seriesenhance_p: c.series_enhance += v0; return true;  // 178 (0x08095C60)
    case magic_dynamicmagicshield_v: c.dynamic_magic_shield += v0; return true;    // 181 (0x08095CA0)
    case magic_nomovespeed: c.no_move_speed -= percent_of(c.no_move_speed_ex, v0); return true;   // 182 (0x08095CC0: sign - quotient)
    case magic_lifereplenish_p: c.life_replenish_percent += v0; return true;       // 190 (0x0809A2F0)
    case magic_ignoreskill_p: c.ignore_skill += v0; return true;                   // 191 (0x0809A4F0)
    case magic_returnskill_p: c.return_skill += v0; return true;                   // 192 (0x0809A3F0)
    case magic_randmove: c.rand_move += v0; return true;                           // 199 (0x080978D0; the stop of a random walk is the ai's)
    case magic_ignorenegativestate_p:                                              // 201 (0x080977D0)
        if (!ctx.removing) {
            npc.stun_state = KNpc::PotionState{};      // +0x1e8 = 0
            npc.freeze_state = KNpc::PotionState{};    // +0x1d8 = 0
        }
        c.ignore_negative_state += v0;
        return true;
    case magic_poison2decmana_p: c.poison_dec_mana_percent += v0; return true;     // 202 (0x08096ED0)
    case magic_staticmagicshield_v:                                                // 203 (0x08096E80): never below 0
        c.static_magic_shield = std::max(0, c.static_magic_shield + v0);
        return true;
    case magic_staticmagicshield_p:                                                // 204 (0x08096DC0): percent of the mana max in use
        c.static_magic_shield += percent_of(c.mana_max_v(), v0);
        return true;
    case magic_returnres_p: c.return_res += v0; return true;                       // 205 (0x08096D80)
    case magic_ignoredamage: c.ignore_damage = v0 > 0 ? 1 : 0; return true;        // 217 (0x08095D50)
    case magic_sorbdamage_p:                                                       // 218 (0x08096D10): 0..500
        c.sorb_damage = std::clamp(c.sorb_damage + v0, 0, 500);
        return true;
    case magic_sorbdamage_yan_p:                                                   // 237 (0x08095EB0): at most 500
        c.sorb_damage_yan = std::min(c.sorb_damage_yan + v0, 500);
        return true;
    case magic_anti_hitrecover: add_pair(c.anti_hit_recover, v0); return true;             // 219 (0x08096CD0)
    case magic_anti_stuntimereduce_p: add_pair(c.anti_stun_time_reduce, v0); return true;  // 220
    case magic_anti_poisonres_p: add_pair(c.anti_resist[4], v0); return true;              // 221 (+0x12e0)
    case magic_anti_fireres_p: add_pair(c.anti_resist[1], v0); return true;                // 222 (+0x12c8)
    case magic_anti_lightingres_p: add_pair(c.anti_resist[3], v0); return true;            // 223 (+0x12d8)
    case magic_anti_physicsres_p: add_pair(c.anti_resist[0], v0); return true;             // 224 (+0x12c0)
    case magic_anti_coldres_p: add_pair(c.anti_resist[2], v0); return true;                // 225 (+0x12d0)
    case magic_block_rate: c.block_rate += v0; return true;                                // 226 (0x08096C90)
    case magic_enhancehit_rate: c.enhance_hit_rate += v0; return true;                     // 227 (0x08096C50)
    case magic_lifemax_yan_v: c.life_max_yan += v0; return true;                           // 233 (0x08095E10)
    case magic_lifemax_yan_p: c.life_max_yan += percent_of(b.life_max, v0); return true;   // 234 (0x08095E30)
    case magic_manamax_yan_v: c.mana_max_yan += v0; return true;                           // 235
    case magic_manamax_yan_p: c.mana_max_yan += percent_of(b.mana_max, v0); return true;   // 236
    case magic_anti_maxres_p:                                                              // 242 (0x08096060): all five anti pairs
        for (auto& p : c.anti_resist) add_pair(p, v0);
        return true;
    case magic_skill_enhance: c.skill_enhance += v0; return true;                          // 243 (0x08096090)
    case magic_magicdamage_p: c.magic_damage_percent += v0; return true;                   // 244 (0x080960B0)
    case magic_five_elements_enhance_v: c.five_elements_enhance += v0; return true;        // 246 (0x080960F0)
    case magic_five_elements_resist_v: c.five_elements_resist += v0; return true;          // 247 (0x08096110)
    case magic_manareplenish_p: c.mana_replenish_percent += v0; return true;               // 248 (0x08096F10)
    case magic_add_damage_p: c.add_damage_percent += v0; return true;                      // 249 (0x08096130)
    case magic_forbit_attack: c.forbid_attack = v0 == 1; return true;                      // 250 (0x08096150): sete after cmp 1
    case magic_frozen_action: c.frozen_action = v0 == 1; return true;                      // 251
    case magic_forbit_takemedicine: c.forbid_medicine = v0 == 1; return true;              // 252
    case magic_invincibility: c.invincibility = v0 == 1; return true;                      // 253
    case magic_add_boss_damage: c.add_boss_damage = v0 > 0 ? v0 : 0; return true;          // 255 (0x08095C30): set, not added
    case magic_anti_poisontimereduce_p: add_pair(c.anti_poison_time_reduce, v0); return true;   // 258 (0x080969B0)
    case magic_do_hurt_p: add_pair(c.do_hurt, v0); return true;                            // 259 (0x08096970)
    case magic_anti_do_hurt_p: add_pair(c.anti_do_hurt, v0); return true;                  // 260
    case magic_do_stun_p: add_pair(c.do_stun, v0); return true;                            // 261
    case magic_anti_do_stun_p: add_pair(c.anti_do_stun, v0); return true;                  // 262
    case magic_anti_physicsres_yan_p: add_pair(c.anti_resist_yan[0], v0); return true;     // 263 (+0x12e8)
    case magic_anti_poisonres_yan_p: add_pair(c.anti_resist_yan[4], v0); return true;      // 264 (+0x1308)
    case magic_anti_coldres_yan_p: add_pair(c.anti_resist_yan[2], v0); return true;        // 265 (+0x12f8)
    case magic_anti_fireres_yan_p: add_pair(c.anti_resist_yan[1], v0); return true;        // 266 (+0x12f0)
    case magic_anti_lightingres_yan_p: add_pair(c.anti_resist_yan[3], v0); return true;    // 267 (+0x1300)
    case magic_anti_allres_yan_p:                                                          // 268 (0x08096710)
        for (auto& p : c.anti_resist_yan) add_pair(p, v0);
        return true;
    case magic_anti_sorbdamage_yan_p: add_pair(c.anti_sorb_damage_yan, v0); return true;   // 269
    case magic_anti_block_rate: add_pair(c.anti_block_rate, v0); return true;              // 270
    case magic_anti_enhancehit_rate: add_pair(c.anti_enhance_hit_rate, v0); return true;   // 271
    case magic_enhancehiteffect_rate: c.enhance_hit_effect_rate += v0; return true;        // 275 (0x080969F0)
    case magic_me2metaldamage_p: c.me_to_series_damage[0] += v0; return true;              // 276 (+0x15cc)
    case magic_metal2medamage_p: c.series_to_me_damage[0] += v0; return true;              // 277 (+0x15e0)
    case magic_me2wooddamage_p: c.me_to_series_damage[1] += v0; return true;               // 278
    case magic_wood2medamage_p: c.series_to_me_damage[1] += v0; return true;               // 279
    case magic_me2waterdamage_p: c.me_to_series_damage[2] += v0; return true;              // 280
    case magic_water2medamage_p: c.series_to_me_damage[2] += v0; return true;              // 281
    case magic_me2firedamage_p: c.me_to_series_damage[3] += v0; return true;               // 282
    case magic_fire2medamage_p: c.series_to_me_damage[3] += v0; return true;               // 283
    case magic_me2earthdamage_p: c.me_to_series_damage[4] += v0; return true;              // 284
    case magic_earth2medamage_p: c.series_to_me_damage[4] += v0; return true;              // 285
    case magic_meleedamagereturnmana_p: c.melee_damage_return_mana += v0; return true;     // 286 (0x080963D0)
    case magic_rangedamagereturnmana_p: c.range_damage_return_mana += v0; return true;     // 287
    case magic_clearallcd: c.clear_all_cd += v0; return true;                              // 291 (0x08096410)
    case magic_addblockrate: c.add_block_rate[0] += v0; c.add_block_rate[1] += v2; return true;   // 292 (0x08096430): value[0] and value[2]
    case magic_walkrunshadow: c.walk_run_shadow = v0 > 0 ? 1 : 0; return true;             // 293 (0x08096610)
    case magic_manatoskill_enhance:                                                        // 298 (0x08096450)
        if (v0 >= 0) {
            c.mana_to_skill_enhance = 1;
            c.mana_to_skill_enhance_ex = v0;
        }
        return true;
    case magic_melee_returnres_p: c.melee_return_res += v0; return true;                   // 299 (0x08096390)
    case magic_range_returnres_p: c.range_return_res += v0; return true;                   // 300 (0x080963B0)
    default:
        return false;
    }
}

bool KNpcAttribModify::handled(int id) noexcept
{
    KNpc probe;
    KMagicAttrib m;
    m.type = id;
    return modify(probe, m, KNpcAttribModifyContext{});
}

} // namespace jx::zone
