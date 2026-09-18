// KNpc.cpp - the fight side of KNpc of the old core (Core/Src/KNpc.cpp), function by function
// from the JX2 server binary (jx_linux_y; every address in docs/LINUX-SERVER.md §12):
// ReceiveDamage, CalcDamage and its resists, AppendSkillEffect with the five element helpers,
// the states (SetStateSkillEffect / SetImmediatelySkillEffect / RemoveStateSkillEffect), the
// poison, the knock back, OnHurt and the per-frame ProcessState.  They are KSubWorld members
// because a blow needs the attacker, the target and the map's random numbers, and a death or
// a knock back moves what the world owns.  The arithmetic is the binary's: int, the truncating
// division of its compiler, a double where it used the FPU (fistp in the truncating mode).
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>

#include "jx/log.hpp"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMath.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KNpcAttribModify.h"

namespace jx::zone {

namespace {

// m_Kind of the old npc (+0x24): 1 = player, 2 = partner; the zone keeps NPCKIND in npc_kind
int old_kind(const KNpc& e) noexcept { return e.kind == KNpcKind::player ? kind_player : e.npc_kind; }
bool player_or_partner(const KNpc& e) noexcept
{
    const int k = old_kind(e);
    return k == kind_player || k == kind_partner;
}

constexpr int kMaxResistCap = 95;        // MAX_RESIST
constexpr int kHugeDamage = 20000000;    // 0x1312D00: above it the resist is applied to the hundredth first
constexpr int kBlockStateSkill = 963;    // the state of a blocked blow (0x0808B30E)
constexpr int kFreezeTimeReduceCap = 0;  // [0x830D234]: freezetimereduce_p counts up to it; 0 in this build (0x08062E43 sets it, nobody else writes it)
constexpr int kPoisonCapPlayer = 0;      // [0x830D248] / [0x830D24C]: the caps of the poison merge; 0 in this build (0x08062E2F)
constexpr int kPoisonCapNpc = 0;
constexpr int kNoCounterSkill = 203;     // a blow of this skill leaves the player's counter of 0x080AEBC0 alone (0x0808AB60)

// 0x08078910: a resist above its maximum counts less: max + (r - max) x (95 - max) / 400
int soften_resist(int r, int max) noexcept { return r <= max ? r : max + (r - max) * (kMaxResistCap - max) / 400; }

KMagicAttrib negated(const KMagicAttrib& m) noexcept
{
    KMagicAttrib n;
    n.type = m.type;
    n.value = {-m.value[0], -m.value[1], -m.value[2]};
    return n;
}

// 0x0807C910 -> 0x080A7C70: a player's current energy (m_nCurEngergy) goes into its magic damage
int player_energy(const KNpc& e) noexcept { return e.kind == KNpcKind::player ? e.player.cur_energy : 0; }

// 0x08099F90 KNpcAttribModify::MixPoisonDamage(pDes, pSrc).  pSrc is read as {interval, damage,
// time} - the layout of m_CurrentPoisonDamage seen from its type cell, which nothing ever sets,
// so a skill's own poison is never merged and a skill without one takes the npc's as it is.
void mix_poison_damage(KMagicAttrib& des, const KMagicAttrib& src) noexcept
{
    const int c2 = src.type, d2 = src.value[0], t2 = src.value[1];
    const int d1 = des.value[0], t1 = des.value[1], c1 = des.value[2];
    if (d1 == 0 || c1 == 0) {
        des = src;
        return;
    }
    if (d2 == 0 || c2 == 0) return;
    const int sum = c2 + c1;
    des.value[0] = (sum * d2 / c2 + sum * d1 / c1) / 2;
    des.value[1] = (t2 * d2 * c1 + t1 * d1 * c2) / (c1 * d2 + d1 * c2);
    des.value[2] = sum / 2;
}

// 0x0807C700: the physics slot from physicsenhance_p (65) or physicsdamage_v (59); a magic skill
// adds the npc's addphysicsmagic_v with magicdamage_p on top
void physics_effect(const KNpc& n, const KMagicAttrib& src, KMagicAttrib& des, int enhance, bool physical, int weapon_percent)
{
    const KNpcCurrentAttrib& c = n.cur;
    if (src.type == magic_physicsenhance_p) {
        const int min = c.add_physics_damage + c.physics_damage.value[0];
        const int max = c.add_physics_damage + c.physics_damage.value[2];
        des.type = magic_physicsdamage_v;
        des.value[0] = (src.value[0] + 100) * min / 100;
        des.value[2] = (src.value[0] + 100) * max / 100;
        if (enhance != 0) {
            des.value[0] += enhance * des.value[0] / 100;
            des.value[2] += enhance * des.value[2] / 100;
        }
        if (n.kind == KNpcKind::player) {   // 0x0807C847: the weapon's addphysicsdamage_p on the base numbers
            des.value[0] += min * weapon_percent / 100;
            des.value[2] += max * weapon_percent / 100;
        }
    } else if (src.type == magic_physicsdamage_v) {
        des.type = magic_physicsdamage_v;
        des.value[0] = src.value[0];
        des.value[2] = src.value[2];
        if (enhance != 0) {
            des.value[0] = enhance * src.value[0] / 100 + src.value[0];
            des.value[2] = src.value[2] * enhance / 100 + src.value[2];
        }
    }
    if (!physical && des.value[0] > 0) {   // 0x0807C745
        des.value[0] += (c.magic_damage_percent + 100) * c.physics_magic.value[0] / 100;
        des.value[2] += (c.magic_damage_percent + 100) * c.physics_magic.value[2] / 100;
    }
}

// 0x0807C950: the cold slot (60); a physical skill carries the npc's cold damage, a magic one its
// cold magic plus the player's energy
void cold_effect(const KNpc& n, const KMagicAttrib& src, KMagicAttrib& des, int enhance, bool physical, int energy)
{
    const KNpcCurrentAttrib& c = n.cur;
    if (src.type == magic_colddamage_v && src.value[0] > 0) {
        des.type = magic_colddamage_v;
        des.value[0] = src.value[0];
        des.value[1] = c.cold_enhance + src.value[1];
        des.value[2] = src.value[2];
        if (enhance != 0) {
            des.value[0] = enhance * src.value[0] / 100 + src.value[0];
            des.value[2] = enhance * src.value[2] / 100 + src.value[2];
        }
    }
    if (physical) {
        des.value[0] += c.cold_damage.value[0];
        if (des.value[0] != 0) des.value[1] = std::max(des.value[1], c.cold_enhance + c.cold_damage.value[1]);
        des.value[2] += c.cold_damage.value[2];
    } else if (src.value[0] > 0) {
        des.value[0] += c.cold_magic.value[0] + energy;
        des.value[2] += c.cold_magic.value[2] + energy;
    }
}

// 0x0807CD30: the fire slot (61)
void fire_effect(const KNpc& n, const KMagicAttrib& src, KMagicAttrib& des, int enhance, bool physical, int energy)
{
    const KNpcCurrentAttrib& c = n.cur;
    if (src.type == magic_firedamage_v && src.value[0] > 0) {
        des.type = magic_firedamage_v;
        des.value[0] = src.value[0];
        des.value[2] = src.value[2] + src.value[2] * c.fire_enhance / 100;
        if (enhance != 0) {
            des.value[0] = enhance * src.value[0] / 100 + src.value[0];
            des.value[2] = enhance * des.value[2] / 100 + des.value[2];
        }
    }
    if (physical) {
        des.value[0] += c.fire_damage.value[0];
        des.value[2] += c.fire_damage.value[2];
    } else if (src.value[0] != 0) {
        des.value[0] += c.fire_magic.value[0] + energy;
        des.value[2] += c.fire_magic.value[2] + energy;
    }
}

// 0x0807CA90: the lightning slot (62): the enhance closes the gap between min and max
void light_effect(const KNpc& n, const KMagicAttrib& src, KMagicAttrib& des, int enhance, bool physical, int energy)
{
    const KNpcCurrentAttrib& c = n.cur;
    if (src.type == magic_lightingdamage_v && src.value[0] > 0) {
        des.type = magic_lightingdamage_v;
        des.value[0] = src.value[0] + (src.value[2] - src.value[0]) * c.light_enhance / 100;
        des.value[2] = src.value[2];
        if (enhance != 0) {
            des.value[0] = enhance * des.value[0] / 100 + des.value[0];
            des.value[2] = enhance * src.value[2] / 100 + src.value[2];
        }
    }
    if (physical) {
        des.value[0] += c.light_damage.value[0];
        des.value[2] += c.light_damage.value[2];
    } else if (src.value[0] != 0) {
        des.value[0] += c.light_magic.value[0] + energy;
        des.value[2] += c.light_magic.value[2] + energy;
    }
}

// 0x0807CBD0: the poison slot (63): {damage, frames, frames between ticks}
void poison_effect(const KNpc& n, const KMagicAttrib& src, KMagicAttrib& des, int enhance, bool physical, int energy)
{
    const KNpcCurrentAttrib& c = n.cur;
    if (src.type == magic_poisondamage_v && src.value[0] > 0) {
        des.type = magic_poisondamage_v;
        des.value[0] = src.value[0];
        des.value[1] = src.value[1];
        des.value[2] = std::max(1, (100 - c.poison_enhance) * src.value[2] / 100);
        if (enhance > 0) des.value[0] = enhance * src.value[0] / 100 + src.value[0];
    }
    if (physical) {
        mix_poison_damage(des, c.poison_damage);
    } else if (src.value[0] > 0) {
        KMagicAttrib tmp;
        tmp.type = c.poison_magic.type;
        tmp.value[0] = c.poison_magic.value[0] + energy;
        tmp.value[1] = c.poison_magic.value[1] <= 0 ? 60 : c.poison_magic.value[1];
        tmp.value[2] = c.poison_magic.value[2] <= 0 ? 10 : c.poison_magic.value[2];
        mix_poison_damage(des, tmp);
    }
}

// 0x08079240: an icon into the six the client shows, kept sorted by priority toward the end
void add_state_icon(KNpc& e, int special, int priority) noexcept
{
    if (special <= 0 || e.state_flag == 2) return;
    int i = e.state_icon_first;
    if (i <= 5) {
        for (; i <= 5; ++i) {
            const KNpc::KStateIcon cell = e.state_icons[static_cast<std::size_t>(i)];
            if (cell.id != 0 && cell.priority > priority) break;
            if (i > 0) e.state_icons[static_cast<std::size_t>(i - 1)] = cell;
        }
    }
    if (i - 1 < 0) return;
    e.state_icons[static_cast<std::size_t>(i - 1)] = KNpc::KStateIcon{special, priority};
    if (e.state_icon_first > 0) --e.state_icon_first;
    e.state_flag = 1;
}

} // namespace

// ---- the resists --------------------------------------------------------------------------

int KSubWorld::calc_resist(const KNpc& t, const KNpc* a, int type, bool melee, bool is_return) noexcept
{
    const KNpcCurrentAttrib& c = t.cur;
    if (type >= damage_physics && type <= damage_poison) {   // 0x0807BB20
        int yin = 0, yan = 0, max = 0;
        switch (type) {
        case damage_physics: yin = c.physics_resist; yan = c.physics_resist_yan; max = c.physics_resist_max; break;
        case damage_fire: yin = c.fire_resist; yan = c.fire_resist_yan; max = c.fire_resist_max; break;
        case damage_cold: yin = c.cold_resist; yan = c.cold_resist_yan; max = c.cold_resist_max; break;
        case damage_light: yin = c.light_resist; yan = c.light_resist_yan; max = c.light_resist_max; break;
        default: yin = c.poison_resist; yan = c.poison_resist_yan; max = c.poison_resist_max; break;
        }
        int anti = 0, anti_yan = 0, anti_max = 0;
        if (a != nullptr) {
            const auto i = static_cast<std::size_t>(type);
            anti = a->cur.anti_resist[i][1];
            anti_yan = a->cur.anti_resist_yan[i][1];
            anti_max = a->cur.anti_resist_max[i][1];
        }
        const int r = std::max(yin - anti, yan - anti_yan);
        return std::min(kMaxResistCap, soften_resist(r, max - anti_max));
    }
    if (type == damage_return) {   // 0x0807BD28: returnres_p, plus the melee / ranged one unless this is the returned blow
        int r = c.return_res;
        if (!is_return) r += melee ? c.melee_return_res : c.range_return_res;
        return std::min(kMaxResistCap, r);
    }
    return 0;
}

void KSubWorld::add_five_resists(KNpc& e, int delta) noexcept
{
    e.cur.physics_resist += delta;
    e.cur.cold_resist += delta;
    e.cur.light_resist += delta;
    e.cur.poison_resist += delta;
    e.cur.fire_resist += delta;
}

void KSubWorld::modify_five_resist_max(KNpc& t, int enhance, int p)
{
    // 0x0807BAA0: x = (enhance - fiveelements_resist) x 100 / (level x 8 + 200); every maximum
    // moves by (x + 100) x p / 700
    const int x = (enhance - t.cur.five_elements_resist) * 100 / (static_cast<int>(t.level) * 8 + 200);
    const int d = (x + 100) * p / 700;
    t.cur.physics_resist_max += d;
    t.cur.cold_resist_max += d;
    t.cur.light_resist_max += d;
    t.cur.poison_resist_max += d;
    t.cur.fire_resist_max += d;
    log::trace("zone.fight", "resist max moved", {log::kv("entity", t.id), log::kv("delta", d)});
}

// ---- the attributes of a cast -----------------------------------------------------------------

int KSubWorld::weapon_enhance_percent(const KNpc& e)
{
    // KPlayer 0x080B0D50: a melee weapon (detail 0) -> the cell of its kind (particular), a
    // ranged one (detail 1) -> +0x145c, no weapon -> +0x1460
    const auto& a = e.cur.add_physics_damage_percent;
    const KItemList* items = e.sid != 0 ? items_of(e.sid) : nullptr;
    const int type = items != nullptr ? items->weapon_type() : -1;
    if (type == 0) {
        const int k = items->weapon_particular();
        return k >= 0 && k <= 8 ? a[static_cast<std::size_t>(k)] : 0;
    }
    if (type == 1) return a[7];
    return a[8];
}

void KSubWorld::append_skill_effect(const KNpc& n, bool physical, bool melee, const std::array<KMagicAttrib, kSkillAttribs>& src,
                                    std::array<KMagicAttrib, kSkillAttribs>& des, int enhance)
{
    (void)melee;   // bIsMelee is handed over and never read (0x0807CE70)
    const KNpcCurrentAttrib& c = n.cur;
    // slot 0: seriesdamage_p (75) plus the launcher's seriesenhance_p
    if (src[17].type == magic_seriesdamage_p) {
        des[0].type = magic_seriesdamage_p;
        des[0].value[0] = c.series_enhance + src[17].value[0];
    }
    // slot 1: the attack rating - always the launcher's, plus attackrating_v or attackrating_p of the base
    des[1].type = magic_attackrating_v;
    if (src[0].type == magic_attackrating_p) des[1].value[0] = n.base.attack_rating * src[0].value[0] / 100 + c.attack_rating;
    else if (src[0].type == magic_attackrating_v) des[1].value[0] = src[0].value[0] + c.attack_rating;
    else des[1].value[0] = c.attack_rating;
    // slot 2: ignoredefense_p (58)
    if (src[1].type == magic_ignoredefense_p) {
        des[2].type = magic_ignoredefense_p;
        des[2].value[0] = src[1].value[0];
    }
    // slots 3..7: the five kinds of damage
    const int energy = player_energy(n);
    physics_effect(n, src[2], des[3], enhance, physical, n.kind == KNpcKind::player ? weapon_enhance_percent(n) : 0);
    cold_effect(n, src[3], des[4], enhance, physical, energy);
    fire_effect(n, src[4], des[5], enhance, physical, energy);
    light_effect(n, src[5], des[6], enhance, physical, energy);
    poison_effect(n, src[6], des[7], enhance, physical, energy);
    // slot 8: magicdamage_v (64)
    if (src[7].type == magic_magicdamage_v) {
        des[8].type = magic_magicdamage_v;
        des[8].value[0] = src[7].value[0];
        des[8].value[2] = src[7].value[2];
        if (enhance != 0) {
            des[8].value[0] += enhance * src[7].value[0] / 100;
            des[8].value[2] = enhance * src[7].value[2] / 100 + src[7].value[2];
        }
    }
    // slots 9..11: the steals, physical skills only
    if (physical) {
        des[9].type = magic_steallife_p;
        des[9].value = {c.life_stolen + src[8].value[0], 0, 0};
        des[10].type = magic_stealmana_p;
        des[10].value = {c.mana_stolen + src[9].value[0], 0, 0};
        des[11].type = magic_stealstamina_p;
        des[11].value = {c.stamina_stolen + src[10].value[0], 0, 0};
    }
    // slot 12: knockback_p (69) with the frames and the distance of the skill
    des[12].type = magic_knockback_p;
    des[12].value = {c.knock_back + src[11].value[0], src[11].value[1], src[11].value[2]};
    // slot 13: deadlystrike_p (70), physical skills only
    if (physical) {
        des[13].type = magic_deadlystrike_p;
        des[13].value = {c.deadly_strike + src[12].value[0], 0, 0};
    }
    // slots 14 / 15: fatallystrike_p (71) and stun_p (72).  The binary crosses the two cells:
    // the fatally strike takes +0x1400 (where stun_p 72 adds up) and the stun takes +0x1418
    // (where fatallystrike_p 71 does) - kept as it is, the game was balanced with it.
    des[14].type = magic_fatallystrike_p;
    des[14].value = {c.stun + src[13].value[0], 0, 0};
    des[15].type = magic_stun_p;
    des[15].value = {c.fatally_strike + src[14].value[0], src[14].value[1], src[14].value[2]};
    // slots 16 / 17: addskillexp1 / 2 copied
    des[16].type = magic_addskillexp1;
    des[16].value = src[15].value;
    des[17].type = magic_addskillexp2;
    des[17].value = src[16].value;
}

// ---- the attributes on a npc ------------------------------------------------------------------

void KSubWorld::modify_attrib(KNpc& target, EntityId launcher, const KMagicAttrib& m, bool removing)
{
    (void)launcher;   // KNpc::ModifyAttrib(nLauncher, ...) hands it to the table; no entry the zone carries reads it
    KSkillListHost host = skill_host(target);
    const std::function<void(int)> set_hide_hook = [&](int v) { set_hide(target, v); };   // [hide] 200 -> KNpc::SetHide 0x0807FF80
    const KNpcAttribModifyContext ctx{&tables(), target.sid != 0 ? items_of(target.sid) : nullptr, removing, tick_, &host, &set_hide_hook};
    if (!KNpcAttribModify::modify(target, m, ctx)) {
        log::trace("zone.fight", "magic attribute not carried", {log::kv("entity", target.id), log::kv("attrib", m.type)});
    }
}

void KSubWorld::set_immediately_skill_effect(KNpc& target, EntityId launcher, const KMagicAttrib* attribs, int count)
{
    if (attribs == nullptr || count <= 0) return;
    for (int i = 0; i < count; ++i) modify_attrib(target, launcher, attribs[i], false);
}

const std::vector<int>* KSubWorld::attrib_data(int attrib_id) const noexcept
{
    const KSkillTable* table = skills_ ? skills_->table() : nullptr;
    return table != nullptr ? table->attrib_data(attrib_id) : nullptr;
}

bool KSubWorld::in_attrib_data(int attrib_id, int value, std::size_t from) const noexcept
{
    const std::vector<int>* d = attrib_data(attrib_id);
    if (d == nullptr) return false;
    for (std::size_t i = from; i < d->size(); ++i) {
        if ((*d)[i] == value) return true;
    }
    return false;
}

bool KSubWorld::apply_special_state(KNpc& target, int attrib_id)
{
    // [returnskill_p] / [ignoreskill_p] of attribconstdata.ini: Data0 = the state skill, Data1 =
    // its frames (0x080862F2 / 0x080864FB); a state skill the manager cannot give is a refusal
    const std::vector<int>* d = attrib_data(attrib_id);
    if (d == nullptr || d->empty()) return true;
    const int id = (*d)[0];
    if (id <= 0) return true;
    const KSkill* sk = skills_ ? skills_->get(id, 1) : nullptr;
    if (sk == nullptr) return false;
    const int time = d->size() > 1 ? (*d)[1] : 0;
    set_state_skill_effect(target, target.id, sk->row.id, 1, sk->state_attribs.data(), sk->state_attrib_count, time);
    return true;
}

void KSubWorld::refresh_state(KNpc& target, EntityId launcher, KStateNode& node, const KMagicAttrib* states, int count)
{
    // 0x08086768: the old values off (the negated ones, as a removal), the new ones on, and kept negated
    for (int i = 0; i < count; ++i) {
        auto& cell = node.states[static_cast<std::size_t>(i)];
        modify_attrib(target, launcher, cell, true);
        modify_attrib(target, launcher, states[i], false);
        cell = negated(states[i]);
    }
}

int KSubWorld::set_state_skill_effect(KNpc& t, EntityId launcher, int skill_id, int level, const KMagicAttrib* states, int n, int time,
                                      int param8, bool refresh, int param10, bool reflected, int param12)
{
    if (level <= 0 || skill_id <= 0) return -1;
    n = std::min(n, kMaxSkillState);
    KNpc* attacker = entities_.find(launcher);
    // 0x08086298: a status immunity may send the curse back to its sender
    if (t.cur.status_immunity != 0 && t.id != launcher && t.cur.return_skill > 0) {
        if (t.cur.return_skill > random(100)) {
            log::debug("zone.fight", "curse returned", {log::kv("entity", t.id), log::kv("launcher", launcher), log::kv("skill", skill_id), log::kv("percent", t.cur.return_skill)});
            if (!apply_special_state(t, magic_returnskill_p)) return 0;
            if (attacker != nullptr) set_state_skill_effect(*attacker, t.id, skill_id, level, states, n, time, param8, refresh, param10, true, 0);
        } else {
            log::trace("zone.fight", "curse not returned", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.return_skill)});
        }
    }
    // 0x08086410: ignorenegativestate_p against a skill aimed at enemies
    if (t.cur.ignore_negative_state > 0 && skill_id <= kMaxSkill && level <= kMaxSkillLevel - 1) {
        const KSkill* sk = skills_ ? skills_->get(skill_id, level) : nullptr;
        if (sk != nullptr && sk->row.target_enemy) {
            if (t.cur.ignore_negative_state > random(100)) {
                log::debug("zone.fight", "negative state ignored", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.ignore_negative_state)});
                return 0;
            }
            log::trace("zone.fight", "negative state taken", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.ignore_negative_state)});
        }
    }
    // 0x0808642B: a skill of [ignoreskill_p] may be ignored outright, with the state 724 on oneself
    if (in_attrib_data(magic_ignoreskill_p, skill_id, 2) && t.cur.ignore_skill > 0) {
        if (t.cur.ignore_skill > random(100)) {
            log::debug("zone.fight", "skill ignored", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.ignore_skill)});
            if (!apply_special_state(t, magic_ignoreskill_p)) return 0;
            return 0;
        }
        log::trace("zone.fight", "skill not ignored", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.ignore_skill)});
    }
    // 0x08086439: a skill of [returnskill_p] may be sent back, unless it already was
    if (!reflected && in_attrib_data(magic_returnskill_p, skill_id, 2) && t.cur.return_skill > 0) {
        if (t.cur.return_skill > random(100)) {
            log::debug("zone.fight", "curse returned", {log::kv("entity", t.id), log::kv("launcher", launcher), log::kv("skill", skill_id), log::kv("percent", t.cur.return_skill)});
            if (!apply_special_state(t, magic_returnskill_p)) return 0;
            if (attacker != nullptr) set_state_skill_effect(*attacker, t.id, skill_id, level, states, n, time, param8, refresh, param10, true, 0);
        } else {
            log::trace("zone.fight", "curse not returned", {log::kv("entity", t.id), log::kv("skill", skill_id), log::kv("percent", t.cur.return_skill)});
        }
    }
    t.people_id = launcher;   // +0x159c
    // (0x08086452: a player / partner is sent the packet 0x87 with the states - the client sync of B4)
    if (KStateNode* node = t.state_of(skill_id); node != nullptr) {   // 0x08086730
        const int prev = node->left_time;
        if (time == 0) {
            node->left_time = 0;
            return prev;
        }
        if (!refresh && param8 == 0) {   // 0x08086B24
            if (node->level == level) {
                node->left_time = time;
                return prev;
            }
            if (level <= node->level) return prev;
            if (n > 0) refresh_state(t, launcher, *node, states, n);
            node->level = level;
            return prev;
        }
        if (n > 0) refresh_state(t, launcher, *node, states, n);
        node->level = level;
        node->refresh = refresh;
        node->param = param10;
        node->extra = param12;
        if (refresh) node->left_time = time;
        return prev;
    }
    if (time == 0) return 0;   // 0x0808649B
    KStateNode node;
    node.skill_id = skill_id;
    node.level = level;
    node.left_time = time;
    node.refresh = refresh;
    node.param = param10;
    node.extra = param12;
    if (const KSkill* sk = skills_ ? skills_->get(skill_id, level) : nullptr; sk != nullptr) {
        node.special_id = sk->row.state_special_id;
        node.priority = sk->row.state_priority;
    }
    add_state_icon(t, node.special_id, node.priority);
    for (int i = 0; i < n; ++i) {
        modify_attrib(t, launcher, states[i], false);
        node.states[static_cast<std::size_t>(i)] = negated(states[i]);
    }
    t.state_skills.push_back(node);
    log::debug("zone.fight", "state added", {log::kv("entity", t.id), log::kv("launcher", launcher), log::kv("skill", skill_id), log::kv("level", level), log::kv("frames", time), log::kv("states", n)});
    return 0;
}

void KSubWorld::remove_state_skill_effect(KNpc& t, int skill_id, bool notify)
{
    (void)notify;   // 0x0807D3A1: the player / partner is sent the packet 0x87 without states (B4)
    if (skill_id <= 0) return;
    for (auto it = t.state_skills.begin(); it != t.state_skills.end(); ++it) {
        if (it->skill_id != skill_id) continue;
        for (const KMagicAttrib& m : it->states) {
            if (m.type != 0) modify_attrib(t, t.id, m, true);
        }
        t.state_skills.erase(it);
        t.state_flag = 2;
        log::debug("zone.fight", "state removed", {log::kv("entity", t.id), log::kv("skill", skill_id)});
        return;
    }
}

// KNpc::IsInvisibleTo(this, nNpcIdx) 0x08079200: while the "hidden" packet goes out (+0x19a4) the
// npc is invisible to itself only (the packet is for everybody else); hidden (+0x19a0 != 0) it is
// invisible to everybody but itself; otherwise to nobody.  The region walk 0x080E1B80 asks it for
// every player around before a packet goes out; the zone asks in look_around and in set_hide.
bool KSubWorld::invisible_to(const KNpc& e, EntityId viewer) const noexcept
{
    if (e.hide_syncing) return e.id == viewer;
    if (e.hide == 0) return false;
    return e.id != viewer;
}

// KNpc::SetHide(this, nHide) 0x0807FF80 (Lua SetHide / NpcSetHide, [hide] 200 of a state)
void KSubWorld::set_hide(KNpc& e, int value)
{
    if (e.hide == 0) {
        if (value != 0) {
            // 0x0807FFB4: the packet 0x4f {npc id} to the players around - with +0x19a4 set the npc is
            // invisible to itself only, so everybody else is told: they forget it (G2C_ENTITY_DESPAWN)
            e.hide_syncing = true;
            entity_gone(e, true);
            e.hide_syncing = false;
        }
    } else if (value == 0) {
        // 0x08080008: 0x0807FBB0(this, 0), the npc's full sync (the packet 0x4c) to the players around
        // - still hidden at that moment, only its own client qualifies; the others learn it again when
        // their client asks for it (0x0809F190, refused while hidden).  Here the viewers whose view
        // holds it look again at the next tick.
        wake_viewers_near(e);
    }
    if (e.hide != value) log::debug("zone.fight", "hidden", {log::kv("entity", e.id), log::kv("hide", value)});
    e.hide = value;
}

// 0x0807D4C0: a hidden npc breaks its hiding - the state skills named by [hide] of
// attribconstdata.ini come off, Data(Count-1) down to Data1 (Data0 is the transparency of the
// hidden client, 70, not a skill): 713, 1235, 1258, 1267.  KNpc::CastSkill 0x08088350 (after the
// cost), KNpc::DoDeath 0x080892E0 (after the death list), a mount 0x0807D520 and the Lua
// OpenProgressBar 0x081082D0 call it.
void KSubWorld::break_hide(KNpc& e)
{
    if (e.hide == 0) return;
    const std::vector<int>* d = attrib_data(magic_hide);
    if (d == nullptr || d->size() < 2) return;
    log::debug("zone.fight", "hide broken", {log::kv("entity", e.id), log::kv("hide", e.hide)});
    for (std::size_t i = d->size() - 1; i >= 1; --i) remove_state_skill_effect(e, (*d)[i], true);
}

void KSubWorld::tick_state_skills(KNpc& e)
{
    // 0x0808B8B6: -1 stays, a count runs down, 0 comes off (the negated values applied as a removal)
    bool changed = false;
    for (auto it = e.state_skills.begin(); it != e.state_skills.end();) {
        if (it->left_time == -1) {
            ++it;
            continue;
        }
        if (it->left_time != 0) {
            --it->left_time;
            ++it;
            continue;
        }
        for (const KMagicAttrib& m : it->states) {
            if (m.type != 0) modify_attrib(e, e.id, m, true);
        }
        log::debug("zone.fight", "state ran out", {log::kv("entity", e.id), log::kv("skill", it->skill_id)});
        it = e.state_skills.erase(it);
        changed = true;
    }
    if (changed) e.state_flag = 2;
}

// ---- the poison ------------------------------------------------------------------------------

void KSubWorld::set_poison(KNpc& t, EntityId launcher, int damage, int time, int interval)
{
    // 0x0807BD60: a fresh poison is taken as it is; one running is merged by damage-weighted
    // time, the damages and the intervals averaged, under the cap of the kind (0 here)
    if (time == 0 || damage == 0) return;
    KNpc::PotionState& p = t.poison_state;
    const int d_was = p.value, t_was = p.time, i_was = t.poison_interval;
    if (p.time == 0) {
        p.time = time;
        p.value = damage;
        t.poison_interval = interval;
    } else if (interval > 0 && i_was > 0 && damage > 0 && d_was > 0) {
        const double num = static_cast<double>(t_was) * d_was * interval + static_cast<double>(time) * damage * i_was;
        const double den = static_cast<double>(d_was) * interval + static_cast<double>(damage) * i_was;
        p.time = static_cast<int>(num / den);
        const int cap = old_kind(t) == kind_player ? kPoisonCapPlayer : kPoisonCapNpc;
        int d2 = damage;
        bool keep = false;
        if (cap > 0) {
            const int limit = cap * (2 * interval) - d_was * interval / i_was;
            if (d2 >= limit) {
                if (limit > 0) d2 = limit;
                else keep = true;
            }
        }
        if (!keep) {
            const int sum = i_was + interval;
            p.value = (d2 * sum / interval + sum * d_was / i_was) / 2;
            t.poison_interval = sum / 2;
        }
    }
    t.last_poison_id = launcher;
    log::debug("zone.fight", "poisoned", {log::kv("entity", t.id), log::kv("launcher", launcher), log::kv("damage", damage), log::kv("frames", time), log::kv("interval", interval),
                                        log::kv("was_damage", d_was), log::kv("was_frames", t_was), log::kv("was_interval", i_was),
                                        log::kv("now_damage", p.value), log::kv("now_frames", p.time), log::kv("now_interval", t.poison_interval)});
}

// ---- the blows -----------------------------------------------------------------------------

int KSubWorld::calc_damage(KNpc& t, KNpc& a, int min, int max, int type, bool melee, const int* dynamic_shield, int* dealt, int do_hurt, bool is_return)
{
    if (!t.alive()) return 0;                 // m_Doing 0xa / 0x15
    if (!grid_.contains(t.id)) return 0;      // m_RegionIndex < 0
    const int sum = min + max;
    if (sum <= 0) return 1;
    int dmg;
    const int range = max - min;
    if (range < 0) dmg = max + random(-range);
    else dmg = min + random(range);
    if (dmg <= 0) return 1;
    KNpcCurrentAttrib& c = t.cur;
    if (c.ignore_damage != 0) return 0;
    const int res = calc_resist(t, &a, type, melee, is_return);
    // 0x08089D5D: the static magic shield takes the blow first; when it breaks, its states go
    if (c.static_magic_shield > 0) {
        if (dmg < c.static_magic_shield) {
            c.static_magic_shield -= dmg;
            log::trace("zone.fight", "static shield", {log::kv("entity", t.id), log::kv("used", dmg), log::kv("rest", c.static_magic_shield)});
            return 1;
        }
        log::trace("zone.fight", "static shield", {log::kv("entity", t.id), log::kv("used", c.static_magic_shield), log::kv("rest", 0)});
        dmg -= c.static_magic_shield;
        c.static_magic_shield = 0;
        if (const std::vector<int>* ids = attrib_data(magic_staticmagicshield_v); ids != nullptr) {
            for (std::size_t i = ids->size(); i-- > 0;) remove_state_skill_effect(t, (*ids)[i], true);
        }
    }
    // 0x0808A070: the dynamic shield eats its share of the average blow
    if (dynamic_shield != nullptr && c.dynamic_magic_shield > 0 && *dynamic_shield > 0) {
        dmg -= (sum / 2) * c.dynamic_magic_shield / *dynamic_shield;
        if (dmg <= 0) dmg = 1;
    }
    if (dmg > kHugeDamage) dmg = dmg / 100 * (100 - res);
    else dmg = dmg * (100 - res) / 100;
    if (dmg == 0) return 1;
    // 0x08089E20: what the target returns (a blow of type 6 on the attacker, no further return)
    if (type != damage_return) {
        int ret = 0;
        if (!is_return) {
            ret = melee ? c.melee_damage_return_percent * dmg / 100 + c.melee_damage_return : c.range_damage_return_percent * dmg / 100 + c.range_damage_return;
        } else if (type == damage_poison && (c.poison_damage_return != 0 || c.poison_damage_return_percent != 0)) {
            ret = dmg * c.poison_damage_return_percent / 100 + c.poison_damage_return;
            log::trace("zone.fight", "poison returned", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("damage", ret)});
        }
        calc_damage(a, t, ret, ret, damage_return, melee, nullptr, nullptr, 0, is_return);
    }
    // 0x08089E31: between players (and partners) the PK rate [0x8BADF50]
    if (player_or_partner(t) && player_or_partner(a)) dmg = dmg * cfg_.pk_damage_percent / 100;
    // (0x08089E44: a player's damage counter +0x86ac / +0x86b0 - B3)
    t.last_damage_id = a.id;   // +0x1598
    if (type == damage_poison && c.poison_dec_mana_percent != 0) {   // 0x0808A0C8: Posion2Mana
        const int m = dmg * c.poison_dec_mana_percent / 100;
        c.mana -= m;
        if (c.mana > c.mana_max_v()) c.mana = c.mana_max_v();
        if (c.mana < 0) c.mana = 0;
        log::trace("zone.fight", "poison to mana", {log::kv("entity", t.id), log::kv("damage", dmg), log::kv("percent", c.poison_dec_mana_percent), log::kv("mana", m)});
    }
    if (dmg > 0) {   // 0x08089E6A: sorbdamage_p (per mille), less the attacker's anti
        const int sorb = c.sorb_damage_v() - a.cur.anti_sorb_damage_yan[1];
        dmg -= dmg * sorb / 1000;
    }
    if (c.mana_shield_percent > 0) {   // 0x08089EC3: the mana shield pays its share - only when the mana suffices
        const int m = dmg * c.mana_shield_percent / 100;
        c.mana -= m;
        if (c.mana < 0) c.mana = 0;
        else dmg -= m;
    }
    {   // 0x08078A10: me2Xdamage_p of the attacker against X2medamage_p of the target
        const int ts = static_cast<int>(t.series), as = static_cast<int>(a.series);
        const int mine = ts >= 0 && ts < 5 ? a.cur.me_to_series_damage[static_cast<std::size_t>(ts)] : 0;
        const int theirs = as >= 0 && as < 5 ? t.cur.series_to_me_damage[static_cast<std::size_t>(as)] : 0;
        dmg += static_cast<int>(static_cast<double>(dmg) * static_cast<double>(mine - theirs) / 100.0);
    }
    {   // 0x08089F0C: the attacker's mana moves by damage x Xdamagereturnmana_p / -100
        const int p = melee ? c.melee_damage_return_mana : c.range_damage_return_mana;
        const double m = static_cast<double>(a.cur.mana) + static_cast<double>(dmg) * (static_cast<double>(p) / -100.0);
        a.cur.mana = static_cast<int>(m);
        if (a.cur.mana < 0) a.cur.mana = 0;
    }
    // 0x08089F65: the damage record of a monster, for the experience (a partner's owner - B3)
    if (!player_or_partner(t) && a.kind == KNpcKind::player) t.add_damage_record(a.id, std::min(dmg, c.life));
    if (dealt != nullptr) *dealt = dmg;
    const int life_was = c.life;
    c.life -= dmg;
    if (dmg > 0) {
        c.mana += c.damage_to_mana_percent * dmg / 100;   // 0x08089FA7: damage2addmana_p
        if (c.mana > c.mana_max_v()) c.mana = c.mana_max_v();
        if (c.mana < 0) c.mana = 0;
        const int quarter = t.life_max() / 4;
        if (c.life < quarter && c.life > 0 && life_was >= quarter) trigger_auto_skills(t, KAutoSkillList::life_quarter, t.id, a.id);
        do_hurt_chance(t, do_hurt, a);
    }
    if (c.life <= 0) {   // 0x0808A44C -> 0x08089920: the death (a player's is decided by KPlayer 0x080B2790 - B3)
        if (t.kind != KNpcKind::player) c.life = 0;   // 0x08089939
        do_death(t, a.id);
        return 0;
    }
    if (c.life > t.life_max()) c.life = t.life_max();
    return 1;
}

int KSubWorld::receive_damage(KNpc& t, KNpc& a, int series, bool melee, const KMagicAttrib* dmg, bool use_ar, int do_hurt, int relation, int skill_id)
{
    if (!t.alive()) return 0;                // m_Doing 0x15 / 0xa
    if (t.damage_lock != 0) return 0;        // +0x1694
    if (dmg == nullptr) return 0;
    if (t.cur.invincibility) return 0;       // +0x147b
    int add_damage = a.cur.add_damage_percent;
    if (t.boss_flag && t.kind != KNpcKind::player) add_damage += a.cur.add_boss_damage;   // 0x08079750 == 3
    int mul = 100;
    if ((relation & 0xc) == relation_enemy) {   // 0x0808B2B8: a block, then an enhanced hit
        const int block = t.crowd_block_rate + t.cur.block_rate - a.cur.anti_block_rate[1];
        if (block > 0 && block > random(100)) {
            log::debug("zone.fight", "blocked", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("percent", block)});
            const KSkill* sk = skills_ ? skills_->get(kBlockStateSkill, 1) : nullptr;
            if (sk == nullptr) return 0;
            set_state_skill_effect(t, t.id, sk->row.id, 1, sk->state_attribs.data(), sk->state_attrib_count, 1);
            return 0;
        }
        const int enh = a.cur.enhance_hit_rate - t.cur.anti_enhance_hit_rate[1];
        if (enh > 0 && enh > random(100)) {
            mul = a.cur.enhance_hit_effect_rate + 200;
            log::trace("zone.fight", "enhanced hit", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("percent", enh), log::kv("effect", a.cur.enhance_hit_effect_rate)});
        }
    }
    t.people_id = a.id;   // +0x159c
    // 0x0808A566: the sums over the five damage slots
    int dyn_sum = 0, min_sum = 0, max_sum = 0;
    for (int slot = damage_slot_physics; slot <= damage_slot_poison; ++slot) {
        const KMagicAttrib& d = dmg[slot];
        if (slot != damage_slot_poison && t.cur.dynamic_magic_shield > 0) dyn_sum += std::max(0, (d.value[0] + d.value[2]) / 2);
        min_sum += d.value[0];
        max_sum += d.type == magic_poisondamage_v ? d.value[0] : d.value[2];
    }
    // 0x0808AC20: the five elements through the conquer table 0x0830ED2C (g_IsConquer): a skill
    // whose element beats the target's takes seriesdamage_p off its five resists for this blow
    // (and moves its maximums); a target whose element beats the skill's gets it added
    int adjust = 0;
    if (series >= 0 && series <= 4) {
        const int ts = static_cast<int>(t.series);
        if (g_IsConquer(series, ts)) {
            int p = dmg[0].value[0] - t.cur.series_res;
            if (player_or_partner(a) && player_or_partner(t)) {
                const int d = static_cast<int>(t.level) - 10 - static_cast<int>(a.level);
                if (d > 0) p -= d * dmg[0].value[2];
            }
            if (p > 0) adjust = -p;
        } else if (g_IsConquer(ts, series)) {
            adjust = dmg[0].value[0];
        }
    }
    add_five_resists(t, adjust);
    if (adjust < 0) modify_five_resist_max(t, a.cur.five_elements_enhance, adjust);
    const auto restore_resists = [&] {
        add_five_resists(t, -adjust);
        if (adjust < 0) modify_five_resist_max(t, a.cur.five_elements_enhance, -adjust);
    };
    if (use_ar && !check_hit_target(dmg[1].value[0], t.cur.defend, dmg[2].value[0])) {   // 0x0808ABB0
        restore_resists();
        log::trace("zone.fight", "dodged", {log::kv("attacker", a.id), log::kv("target", t.id), log::kv("ar", dmg[1].value[0]), log::kv("defend", t.cur.defend)});
        return 0;
    }
    const double f = static_cast<double>(add_damage) * mul / 10000.0;   // 0x0808A5F8: add_damage_p x nMul / 10000.0f
    const auto scaled = [&](int v) { return static_cast<int>(static_cast<double>(v) * f); };
    const int life_before = t.cur.life;
    const int a_life_before = a.cur.life;
    int dealt_sum = 0, magic_dealt = 0, out = 0;
    // slot 3: the physics blow, doubled by a deadly strike (slot 13)
    int result;
    {
        const KMagicAttrib& p = dmg[damage_slot_physics];
        const int deadly = dmg[damage_slot_deadly_strike].value[0];
        if (deadly != 0 && deadly > random(100)) result = calc_damage(t, a, scaled(2 * p.value[0]), scaled(2 * p.value[2]), damage_physics, melee, &dyn_sum, &out, do_hurt, false);
        else result = calc_damage(t, a, scaled(p.value[0]), scaled(p.value[2]), damage_physics, melee, &dyn_sum, &out, do_hurt, false);
    }
    const int physics_dealt = life_before - std::max(t.cur.life, 0);
    dealt_sum += out;
    out = 0;
    if (result != 0) {   // slot 4: cold
        const KMagicAttrib& p = dmg[damage_slot_cold];
        result = calc_damage(t, a, scaled(p.value[0]), scaled(p.value[2]), damage_cold, melee, &dyn_sum, &out, do_hurt, false);
        dealt_sum += out;
        out = 0;
    }
    if (t.freeze_state.time <= 0) {   // 0x0808B1E0: the freeze of the cold slot, when not frozen yet
        const int v1 = dmg[damage_slot_cold].value[1];
        if (v1 > 0) {
            const int reduce = std::min(t.cur.freeze_time_reduce, kFreezeTimeReduceCap);
            t.freeze_state.time = (100 - reduce) * v1 / 100;
            if (t.cur.ignore_negative_state > 0) {
                if (t.cur.ignore_negative_state > random(100)) {
                    log::trace("zone.fight", "freeze ignored", {log::kv("entity", t.id), log::kv("percent", t.cur.ignore_negative_state)});
                    t.freeze_state.time = 0;
                } else {
                    log::trace("zone.fight", "frozen", {log::kv("entity", t.id), log::kv("frames", t.freeze_state.time)});
                }
            } else {
                log::trace("zone.fight", "frozen", {log::kv("entity", t.id), log::kv("frames", t.freeze_state.time)});
            }
        }
    }
    if (result != 0) {   // slot 5: fire
        const KMagicAttrib& p = dmg[damage_slot_fire];
        result = calc_damage(t, a, scaled(p.value[0]), scaled(p.value[2]), damage_fire, melee, &dyn_sum, &out, do_hurt, false);
        dealt_sum += out;
        out = 0;
    }
    if (result != 0) {   // slot 6: lightning
        const KMagicAttrib& p = dmg[damage_slot_light];
        result = calc_damage(t, a, scaled(p.value[0]), scaled(p.value[2]), damage_light, melee, &dyn_sum, &out, do_hurt, false);
        dealt_sum += out;
        out = 0;
    }
    if (result != 0) {   // slot 7: the poison - one blow now (no dynamic shield), the state after
        const KMagicAttrib& p = dmg[damage_slot_poison];
        const int pv = scaled(p.value[0]);
        result = calc_damage(t, a, pv, pv, damage_poison, melee, nullptr, &out, do_hurt, false);
        dealt_sum += out;
        out = 0;
        if (result != 0 && pv != 0) {   // 0x0808AF61
            const int reduce = t.cur.poison_time_reduce - a.cur.anti_poison_time_reduce[1];
            const int time = reduce > 74 ? p.value[1] / 4 : (100 - reduce) * p.value[1] / 100;
            int pd = pv;
            if (adjust != 0) pd = pd * (100 - adjust) / 100;
            set_poison(t, a.id, pd, time, p.value[2]);
        }
    }
    if (result != 0) {   // slot 8: magic (type 5: no resist)
        const KMagicAttrib& p = dmg[damage_slot_magic];
        calc_damage(t, a, scaled(p.value[0]), scaled(p.value[2]), damage_magic, melee, &dyn_sum, &out, do_hurt, false);
        magic_dealt = out;
        out = 0;
    }
    // slots 9..11: the steals of the physics dealt (0x0808A71C), only capped above
    if (const int v = dmg[damage_slot_steal_life].value[0]; v != 0) {
        a.cur.life += physics_dealt * v / 100;
        if (a.cur.life > a.life_max()) a.cur.life = a.life_max();
    }
    if (const int v = dmg[damage_slot_steal_mana].value[0]; v != 0) {
        a.cur.mana += physics_dealt * v / 100;
        if (a.cur.mana > a.mana_max()) a.cur.mana = a.mana_max();
    }
    if (const int v = dmg[damage_slot_steal_stamina].value[0]; v != 0) {
        a.cur.stamina += physics_dealt * v / 100;
        if (a.cur.stamina > a.cur.stamina_max) a.cur.stamina = a.cur.stamina_max;
    }
    {   // slot 12: the knock back (0x0808A830)
        const KMagicAttrib& k = dmg[damage_slot_knock_back];
        if (k.value[0] > 0) {
            if (k.value[0] > random(100)) {
                log::trace("zone.fight", "knock back", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("percent", k.value[0]), log::kv("frames", k.value[1]), log::kv("distance", k.value[2])});
                knock_back(t, a, k.value[1], k.value[2]);
            } else {
                log::trace("zone.fight", "knock back missed", {log::kv("entity", t.id), log::kv("percent", k.value[0])});
            }
        }
    }
    {   // slot 14: the fatally strike - a quarter of the life, less its resist, outside CalcDamage (0x0808B068)
        const int v0 = dmg[damage_slot_fatally_strike].value[0];
        if (v0 != 0 && v0 > random(100)) {
            const int life = t.cur.life;
            const int c = std::max(0, 100 - t.cur.fatally_strike_res);
            const int fd = static_cast<int>(static_cast<double>(life / 4) * static_cast<double>(c) / 100.0);
            if (t.kind != KNpcKind::player && a.kind == KNpcKind::player) t.add_damage_record(a.id, fd);
            t.cur.life = life - fd;
            const int quarter = t.life_max() / 4;
            if (t.cur.life < quarter && t.cur.life > 0 && life >= quarter) trigger_auto_skills(t, KAutoSkillList::life_quarter, t.id, a.id);
            log::trace("zone.fight", "fatally strike", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("damage", fd)});
        }
    }
    {   // slot 15: the stun (0x0808A8A8)
        const KMagicAttrib& s = dmg[damage_slot_stun];
        if (s.value[0] > 0) {
            int rate = s.value[0];
            if (t.cur.ignore_negative_state > 0) rate = (100 - t.cur.ignore_negative_state) * rate / 100;
            rate += a.cur.do_stun[1] - t.cur.anti_do_stun[1];
            if (rate > random(100) && t.stun_state.time <= 0) {
                const int r = t.cur.stun_time_reduce - a.cur.anti_stun_time_reduce[1];
                if (r > 74) t.stun_state.time = mul * s.value[1] / 400;
                else t.stun_state.time = mul * s.value[1] / 100 * (100 - r) / 100;
                log::trace("zone.fight", "stunned", {log::kv("entity", t.id), log::kv("attacker", a.id), log::kv("percent", rate), log::kv("frames", t.stun_state.time)});
            }
        }
    }
    // 0x0808A9D8: a player's equipment wears (Abrade) - B3; an untouched target may still stagger
    if (t.cur.life == life_before) {
        do_hurt_chance(t, do_hurt, a);
    } else if (t.cur.life < life_before && t.kind == KNpcKind::player && skill_id != kNoCounterSkill) {
        // 0x080AEBC0(Player, 10): B3
    }
    // 0x0808AA38, slots 16 / 17 (addskillexp1 / 2): six blows in ten give the skill its experience
    // (KSkillList::AddSkillExp 0x080E5D90) - to the attacker when a player or a partner, or to the
    // target when nValue[2] bit 1 says so and the target is a player (or the attacker a partner)
    for (int slot = damage_slot_add_skill_exp1; slot <= damage_slot_add_skill_exp2; ++slot) {
        const KMagicAttrib& x = dmg[slot];
        if (x.value[0] <= 0 || random(100) > 59) continue;
        KNpc* to = nullptr;
        if ((x.value[2] & 2) != 0) {
            if (t.kind == KNpcKind::player || old_kind(a) == kind_partner) to = &t;
        } else if (player_or_partner(a)) {
            to = &a;
        }
        if (to != nullptr) give_skill_exp(*to, x, false);
    }
    restore_resists();
    if ((relation & 0xc) == relation_enemy) {   // 0x0808B190: the auto skills, unless the skill is excluded
        if (!in_attrib_data(magic_autoreplyskill, skill_id, 0)) trigger_auto_skills(t, KAutoSkillList::hit_reply, t.id, a.id);
        if (!in_attrib_data(magic_autoattackskill, skill_id, 0)) trigger_auto_skills(a, KAutoSkillList::on_hit, t.id, a.id);
    }
    log::debug("zone.fight", "hit", {log::kv("attacker", a.id), log::kv("target", t.id), log::kv("skill", skill_id), log::kv("min", min_sum), log::kv("max", max_sum),
                                    log::kv("damage", magic_dealt + dealt_sum), log::kv("life", t.cur.life)});
    if (a.cur.life != a_life_before) sync_life(a, a_life_before, t.id);
    return 1;
}

bool KSubWorld::check_hit_target(int ar, int df, int ignore)
{
    // 0x0807ED60: no rating -> miss; a negative defence -> 95; otherwise AR x 100 / (AR + the
    // defence left after ignoredefense_p), 5..95, and 50 when both are nothing
    if (ar < 0) return false;
    int percent = kMaxHitPercent;
    if (df >= 0) {
        percent = 50;
        const int sum = ar + df * (100 - ignore) / 100;
        if (sum != 0) {
            const int p = ar * 100 / sum;
            percent = p > kMaxHitPercent ? kMaxHitPercent : (p > 4 ? p : kMinHitPercent);
        }
    }
    return percent > random(100);
}

void KSubWorld::do_hurt_chance(KNpc& t, int do_hurt, const KNpc& a)
{
    // 0x0807F9D0: DoHurt of the skill plus the attacker's dohurt, less the target's anti
    if (do_hurt <= 0) return;
    const int rate = do_hurt + a.cur.do_hurt[1] - t.cur.anti_do_hurt[1];
    if (rate > random(100)) this->do_hurt(t, a.cur.anti_hit_recover[1], a.id);
}

void KSubWorld::do_hurt(KNpc& e, int anti_hit_recover, EntityId source)
{
    // KNpc::OnHurt 0x0807F780: half the blows stagger; hit recover (less the attacker's anti)
    // above 99 never, and it shortens the stagger; ignorenegativestate_p may wave it off
    if (!grid_.contains(e.id)) return;
    if (e.doing == KDoing::hurt || e.doing == KDoing::death) return;   // m_Doing 9 / 10
    const int hr = e.cur.hit_recover_v();
    if (hr - anti_hit_recover > 99) return;
    if (random(100) <= 49) return;
    if (e.cur.ignore_negative_state > 0 && e.cur.ignore_negative_state > random(100)) {
        log::trace("zone.fight", "stagger ignored", {log::kv("entity", e.id), log::kv("percent", e.cur.ignore_negative_state)});
        return;
    }
    // (0x0807F85C: a running npc loses its run bonus +0x14b0 - the zone has no run state yet)
    e.doing = KDoing::hurt;
    e.frame_cur = 0;
    e.frame_total = static_cast<std::uint32_t>(std::max(1, (anti_hit_recover - hr + 100) * static_cast<int>(e.hurt_frame) / 100));
    e.attack_target = EntityId{};
    if (e.moving) e.set_pos(e.pos());   // the hit interrupts walking
    emit_action(e, pb::ACTION_HURT, source);
}

void KSubWorld::knock_back(KNpc& t, const KNpc& launcher, int frames, int distance)
{
    // KNpc::KnockBack 0x08087940: `distance` away from the launcher along the line between the
    // two (the launcher's own direction when they stand on the same spot), over `frames` frames
    if (!grid_.contains(t.id) || t.doing == KDoing::death || t.doing == KDoing::knock_back) return;
    const Pos p1 = t.pos();
    const Pos p2 = launcher.pos();
    int dir = static_cast<int>(t.dir);
    if (p2.x != p1.x || p2.y != p1.y) {
        dir = g_GetDirIndex(p1.x, p1.y, p2.x, p2.y);   // the same table search as 0x08087BC6.. (KMath.h)
        if (dir < 0) dir = -1;
    }
    t.dir = dir < 0 ? 0u : static_cast<std::uint32_t>(dir);
    Pos dest{p1.x - ((g_DirCos(dir) * distance) >> 10), p1.y - ((g_DirSin(dir) * distance) >> 10)};
    // 0x08087AD5: the way that is free, flying over jump barriers and npcs; none -> no knock back
    int way = distance;
    if (!knock_back_free_spot(t, dest, way, true)) return;
    t.knock_from = p1;
    t.knock_dest = dest;
    t.doing = KDoing::knock_back;
    t.frame_cur = 0;
    t.frame_total = static_cast<std::uint32_t>(std::max(1, frames));
    t.attack_target = EntityId{};
    if (t.moving) t.set_pos(t.pos());
    emit_action(t, pb::ACTION_HURT, launcher.id);   // shown as a stagger until the client knows the knock back (B4)
}

bool KSubWorld::knock_back_free_spot(const KNpc& e, Pos& to, int& distance, bool fly) const
{
    // 0x08081B70(npc, &x, &y, &distance, fly): from the npc's spot toward (x, y) in steps of its
    // step length (1..32, else no way), at most min(way, distance) / step of them.  A step onto a
    // cell of the region's scripted-cell map (0x080E0990; the zone has none) is where the way
    // ends; else the barrier under the step decides (0x080F0530): none - the step is good; 1 or 2
    // - the way ends at the last good step; 3 or 4 (a jump barrier, a npc standing there) - the
    // same unless `fly`, which flies over without making the step a good one; anything else (off
    // the map too) - no way.
    const int step = e.cur.step_length;
    if (step < 1 || step > 32) return false;
    if (distance <= 0) return false;
    const Pos from = e.pos();
    const std::int64_t dx = to.x - from.x;
    const std::int64_t dy = to.y - from.y;
    const int len = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
    if (len == 0) return false;
    const auto step_x = static_cast<int>(((dx * step) << 10) / len);
    const auto step_y = static_cast<int>(((dy * step) << 10) / len);
    const int n = std::min(len, distance) / step;
    auto result = [&](int good) {
        distance = good * step;
        to = Pos{from.x + ((good * step_x) >> 10), from.y + ((good * step_y) >> 10)};
        return true;
    };
    if (n <= 0) return result(0);
    std::int64_t ax = static_cast<std::int64_t>(from.x) << 10;
    std::int64_t ay = static_cast<std::int64_t>(from.y) << 10;
    int good = 0;
    for (int i = 1; i <= n; ++i) {
        ax += step_x;
        ay += step_y;
        const Pos at{static_cast<std::int32_t>(ax >> 10), static_cast<std::int32_t>(ay >> 10)};
        const int kind = barrier_kind(at);
        if (kind < 0 || kind > 4) return false;   // (byte)-1 and anything past the table 0x08254A48
        switch (kind) {
        case 0: good = i; break;                    // 0x08081DA8
        case 1:
        case 2: return result(good);                // 0x08081D62
        default:                                    // 3, 4: 0x08081D58
            if (!fly) return result(good);
            break;
        }
    }
    return result(good);
}

void KSubWorld::trigger_auto_skills(KNpc& owner, KAutoSkillList which, EntityId self_key, EntityId other)
{
    // 0x08188BB0(list, index, target): every entry whose wait for `index` is over rolls its
    // percent, then its skill (a style 0..4 or 14 one; an own skill only when the skill list
    // allows it) is cast on `index` - or on `target` when the entry says so - and the wait for
    // `index` starts again.  The casts may change the list: the keys are taken first.
    if (!self_key.valid()) return;
    auto& list = owner.auto_skills[static_cast<std::size_t>(which)];
    if (list.empty()) return;
    std::vector<int> keys;
    keys.reserve(list.size());
    for (const auto& [key, entry] : list) keys.push_back(key);
    for (const int key : keys) {
        const auto it = list.find(key);
        if (it == list.end()) continue;
        const KAutoSkillEntry& e = it->second;
        if (const auto w = e.next_tick.find(self_key.value); w != e.next_tick.end() && tick_ < w->second) continue;
        if (!(e.rate > random(100))) {
            log::trace("zone.fight", "auto skill failed", {log::kv("entity", owner.id), log::kv("skill", key >> 8), log::kv("level", key & 0xff), log::kv("percent", e.rate)});
            continue;
        }
        const int id = key >> 8;
        const int level = key & 0xff;
        log::trace("zone.fight", "auto skill cast", {log::kv("entity", owner.id), log::kv("skill", id), log::kv("level", level), log::kv("percent", e.rate)});
        const bool own = e.own_skill;
        if (own) {
            // 0x080E4540 KSkillList::CanCast(list, id, frame, level): held with a current level,
            // not forbidden, past its cool down and at the level the cell asks for
            if (!owner.skill_list.can_cast(id, tick_, static_cast<int>(owner.level))) continue;
        }
        if (id < 1 || id > 1999 || level < 1 || level > 63) continue;
        const KSkill* sk = skills_ ? skills_->get(id, level) : nullptr;
        if (sk == nullptr) continue;
        if (sk->row.style != skill_style_jx2_14 && sk->row.style > skill_style_create_npc) continue;
        KCastParams p;
        p.target = e.at_target == 1 ? other : self_key;
        if (!skill_cast(*sk, owner, p)) continue;
        if (own) set_skill_cool_time(owner, id, level);   // 0x080847B0: an own skill's cool down starts (the cast sync 0x85 - B4)
        if (const auto again = list.find(key); again != list.end()) {
            again->second.next_tick[self_key.value] = tick_ + static_cast<std::uint64_t>(again->second.interval);
        }
    }
}

void KSubWorld::sync_life(const KNpc& e, int life_before, EntityId source)
{
    const int now = std::max(0, e.life());
    const int was = std::max(0, life_before);
    if (now != was) emit_life(e, now - was, source);
}

// ---- every frame ------------------------------------------------------------------------------

int KSubWorld::count_npcs_within(const KNpc& e, int radius) const
{
    // 0x0807A0E0: the npcs of the region and its eight neighbours within `radius` (the exact
    // distance of KNpcSet 0x0809F370), the npc itself left out by its distance of 0
    int n = 0;
    const Pos p = e.pos();
    grid_.for_each_within(p, radius, [&](EntityId id) {
        if (id == e.id) return;
        const KNpc* o = entities_.find(id);
        if (o == nullptr || o->kind == KNpcKind::drop) return;
        const std::int64_t dx = o->pos().x - p.x;
        const std::int64_t dy = o->pos().y - p.y;
        const auto d = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
        if (d > 0 && d <= radius) ++n;
    });
    return n;
}

void KSubWorld::per_second_attribs(KNpc& e)
{
    // 0x0808C078: a player's block rate from the crowd around it (addblockrate)
    if (e.kind == KNpcKind::player) {
        const KAttribPair& ab = e.cur.add_block_rate;
        if (ab[0] > 0 && ab[1] > 0) e.crowd_block_rate = std::min(25, count_npcs_within(e, 256) / ab[0] * ab[1]);
    }
    // 0x0808BFFC: manatoskill_enhance x the mana left; off (and its percent with it) otherwise
    if (e.cur.mana_to_skill_enhance != 0) {
        const int max = e.cur.mana_max_v();
        const int mana = e.cur.mana;
        if (max > 0 && mana > 0) {
            e.mana_skill_enhance = e.cur.mana_to_skill_enhance_ex * (mana * 100 / max) / 100;
            return;
        }
    }
    e.mana_skill_enhance = 0;
    e.cur.mana_to_skill_enhance_ex = 0;
}

bool KSubWorld::process_frame_state(KNpc& e, bool regen)
{
    // KNpc::ProcessState 0x0808B610 while m_ProcessState (alive)
    if (!grid_.contains(e.id)) return false;
    if (regen) process_state(e);
    // 0x0808B75E: the poison ticks every `interval` frames while its sender lives
    if (e.poison_state.time > 0) {
        KNpc* sender = e.last_poison_id ? entities_.find(e.last_poison_id) : nullptr;
        if (sender == nullptr || sender->cur.life <= 0) {
            e.poison_state.time = 0;
        } else {
            const int time = --e.poison_state.time;
            bool tick = false;
            if (e.poison_interval == 0) {
                e.poison_interval = 1;
                tick = true;
            } else {
                tick = time % e.poison_interval == 0;
            }
            if (tick) {
                const int before = e.cur.life;
                calc_damage(e, *sender, e.poison_state.value, e.poison_state.value, damage_poison, false, nullptr, nullptr, 0, true);
                sync_life(e, before, sender->id);
            }
        }
    }
    // 0x0808B780: a freeze takes every other frame, a stun every frame
    bool skip = false;
    if (e.freeze_state.time > 0) {
        --e.freeze_state.time;
        skip = (e.freeze_state.time & 1) != 0;
    }
    if (e.stun_state.time > 0) {
        --e.stun_state.time;
        skip = true;
    }
    // 0x0808B7BC: the life and mana potions, a dose every GAME_UPDATE_TIME frames
    if (e.life_state.time > 0) {
        --e.life_state.time;
        if (e.life_state.time % static_cast<int>(kGameUpdateTime) == 0) {
            const int before = e.cur.life;
            e.cur.life += e.life_state.value * e.cur.life_replenish_percent / 100;
            if (e.cur.life > e.life_max()) e.cur.life = e.life_max();
            sync_life(e, before, EntityId{});
        }
        if (e.life_state.time <= 0) e.life_state = {};
    }
    if (e.mana_state.time > 0) {
        --e.mana_state.time;
        if (e.mana_state.time % static_cast<int>(kGameUpdateTime) == 0) {
            e.cur.mana += e.mana_state.value * e.cur.mana_replenish_percent / 100;
            if (e.cur.mana > e.mana_max()) e.cur.mana = e.mana_max();
        }
        if (e.mana_state.time <= 0) e.mana_state = {};
    }
    // (0x0808B8A8: the timed change +0x14cc / +0x14d0 / +0x14d4 - not met yet)
    tick_state_skills(e);
    return skip;
}

} // namespace jx::zone
