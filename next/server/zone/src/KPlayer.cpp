#include "jx/zone/KPlayer.h"

#include <algorithm>
#include <cstdlib>

#include "jx/role.pb.h"
#include "jx/zone/KItem.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KNpcAttribModify.h"
#include "jx/zone/KPlayerSet.h"

namespace jx::zone {

namespace {

// KItem::ApplyBaseAttrib (jx_linux_y 0x080669B0): the seven base attributes, every one that is
// set, through KNpc::ModifyAttrib
void apply_base_attribs(const KItem& item, KNpc& npc, const KNpcAttribModifyContext& ctx)
{
    for (const KMagicAttrib& a : item.base) {
        if (!a.empty()) KNpcAttribModify::modify(npc, a, ctx);
    }
}

// KItem::ApplyMagicAttrib (0x08066890): the six magic attributes; the even slots (prefixes) are
// always applied, an odd slot (a suffix) only while `active` is left - one per awake suffix
void apply_magic_attribs(const KItem& item, KNpc& npc, int active, const KNpcAttribModifyContext& ctx)
{
    for (std::size_t i = 0; i < item.magic.size(); ++i) {
        const KMagicAttrib& a = item.magic[i];
        if (a.empty()) continue;
        if (i % 2 == 1) {
            if (active <= 0) continue;
            --active;
        }
        KNpcAttribModify::modify(npc, a, ctx);
    }
}

} // namespace

void KPlayer::load_from(KNpc& npc, const pb::RoleData& role, const KPlayerSet& tables, const KItemList* items)
{
    // a record without numbers (a test's, or one from before the tables) gets the placeholders
    // persist.MigrateRole / NewRole give: 100 life, 50 mana, 100 stamina, ten of each point
    pb::RoleStats s = role.stats();
    if (s.hp_max() <= 0) {
        s.set_hp_max(100);
        s.set_mp_max(50);
        if (s.strength() == 0 && s.dexterity() == 0 && s.vitality() == 0 && s.energy() == 0) {
            s.set_strength(10);
            s.set_dexterity(10);
            s.set_vitality(10);
            s.set_energy(10);
        }
    }
    // KPlayer::LoadFrom 0x080C16D0: the resistance maxima of the role data, 0 = the default 75
    npc.base.fire_resist_max = KPlayerSet::kDefaultResistMax;
    npc.base.cold_resist_max = KPlayerSet::kDefaultResistMax;
    npc.base.poison_resist_max = KPlayerSet::kDefaultResistMax;
    npc.base.light_resist_max = KPlayerSet::kDefaultResistMax;
    npc.base.physics_resist_max = KPlayerSet::kDefaultResistMax;
    reborn = static_cast<int>(std::min<std::uint32_t>(s.reborn(), static_cast<std::uint32_t>(kMaxReborn)));
    // the five points: base and current alike (+0x5930.. / +0x5948..)
    strength = cur_strength = s.strength();
    dexterity = cur_dexterity = s.dexterity();
    vitality = cur_vitality = s.vitality();
    energy = cur_energy = s.energy();
    lucky = cur_lucky = s.lucky();
    attribute_point = s.attribute_point();
    skill_point = s.skill_point();
    set_npc_physics_damage_base(npc);
    set_npc_attack_rating(npc);
    set_npc_defence(npc);
    exp = static_cast<std::int64_t>(role.exp());
    next_level_exp = tables.level_exp(static_cast<int>(npc.level), reborn);
    revive_map = role.revive_map();
    revive_x = role.revive_pos().x();
    revive_y = role.revive_pos().y();
    revive_ref = static_cast<int>(role.revive_ref());
    // m_LifeMax straight from the role data (TRoleData+0xeb), the stamina from the tables, the
    // mana from the role data; a player has no natural life / mana replenish of its own
    npc.base.life_max = std::max(1, s.hp_max());
    npc.base.stamina_max = tables.stamina_base(static_cast<int>(npc.series), static_cast<int>(npc.sex), static_cast<int>(npc.level));
    npc.base.life_replenish = 0;
    npc.base.mana_replenish = 0;
    npc.base.mana_max = std::max(0, s.mp_max());
    npc.base.stamina_gain = tables.stamina().normal_add;
    set_npc_resist(npc, tables);
    // 0x080A7FF0: the fixed player speeds and radii
    npc.base.walk_speed = 5;
    npc.base.run_speed = 10;
    npc.base.attack_speed = 0;
    npc.base.cast_speed = 0;
    npc.base.vision_radius = 120;
    npc.base.hit_recover = 0;
    loaded = true;
    // KNpc::Init (0x08082680) then the current life / mana / stamina of the role data
    // 0x080C1F83: SetHorse(0), then the equip pass of the worn pieces mounts the horse (0x081FE78E: 1 when the horse
    // table knows it - every horse here)
    npc.horse = items != nullptr && items->equipped(itempart_horse) != 0 ? 1 : 0;
    updata_cur_data(npc, true, tables, items);
    npc.cur.life = s.hp() > 0 ? std::min(s.hp(), npc.life_max()) : npc.life_max();
    npc.cur.mana = s.mp() > 0 ? std::min(s.mp(), npc.mana_max()) : npc.mana_max();
    npc.cur.stamina = s.stamina() > 0 ? std::min(s.stamina(), npc.cur.stamina_max) : npc.cur.stamina_max;
}

void KPlayer::save_to(const KNpc& npc, pb::RoleData& role) const
{
    pb::RoleStats* s = role.mutable_stats();
    s->set_hp(npc.cur.life);
    s->set_hp_max(npc.base.life_max);
    s->set_mp(npc.cur.mana);
    s->set_mp_max(npc.base.mana_max);
    s->set_stamina(npc.cur.stamina);
    s->set_stamina_max(npc.base.stamina_max);
    s->set_strength(strength);
    s->set_dexterity(dexterity);
    s->set_vitality(vitality);
    s->set_energy(energy);
    s->set_lucky(lucky);
    s->set_attribute_point(attribute_point);
    s->set_skill_point(skill_point);
    s->set_reborn(static_cast<std::uint32_t>(std::max(0, reborn)));
    role.set_level(npc.level);
    role.set_exp(static_cast<std::uint64_t>(std::max<std::int64_t>(0, exp)));
    role.set_revive_map(revive_map);
    role.mutable_revive_pos()->set_x(revive_x);
    role.mutable_revive_pos()->set_y(revive_y);
    role.set_revive_ref(static_cast<std::uint32_t>(std::max(0, revive_ref)));
}

void KPlayer::lose_exp(std::int64_t loss) noexcept
{
    if (loss <= 0) return;
    exp = std::max<std::int64_t>(0, exp - loss);
}

void KPlayer::set_npc_physics_damage_base(KNpc& npc) const noexcept
{
    // 0x080A7EC0: min = max = m_nCurStrength / 5 + 1, the middle value 0, the elements cleared
    const int d = cur_strength / 5 + 1;
    npc.cur.physics_damage.value = {d, 0, d};
    npc.cur.fire_damage = KMagicAttrib{};
    npc.cur.cold_damage = KMagicAttrib{};
    npc.cur.light_damage = KMagicAttrib{};
    npc.cur.poison_damage = KMagicAttrib{};
}

void KPlayer::set_npc_attack_rating(KNpc& npc) const noexcept
{
    npc.base.attack_rating = dexterity * 4 - 28;   // 0x080A7F90
}

void KPlayer::set_npc_defence(KNpc& npc) const noexcept
{
    npc.base.defend = dexterity / 4;   // 0x080A7FC0 (sar 2 of a non-negative point)
}

void KPlayer::set_npc_resist(KNpc& npc, const KPlayerSet& tables) const noexcept
{
    // 0x080AB7C0: GetXResist(series, level, reborn != 0) into the base block
    const int series = static_cast<int>(npc.series), level = static_cast<int>(npc.level);
    const bool rb = reborn != 0;
    npc.base.fire_resist = tables.fire_resist(series, level, rb);
    npc.base.cold_resist = tables.cold_resist(series, level, rb);
    npc.base.poison_resist = tables.poison_resist(series, level, rb);
    npc.base.light_resist = tables.light_resist(series, level, rb);
    npc.base.physics_resist = tables.physics_resist(series, level, rb);
}

void KPlayer::set_npc_physics_damage(KNpc& npc, const KItemList* items) const noexcept
{
    // 0x080AF740: GetWeaponDamage, then the strength for a melee weapon (detail 0), the
    // dexterity for a ranged one (detail 1); anything else (bare hands: -1) as it is
    std::pair<int, int> d = items ? items->weapon_damage(cur_strength) : std::pair<int, int>{cur_strength / 5 + 1, cur_strength / 5 + 1};
    const int type = items ? items->weapon_type() : -1;
    if (type == equip_meleeweapon) {
        d.first += cur_strength / 5;
        d.second += cur_strength / 5;
    } else if (type == equip_rangeweapon) {
        d.first += cur_dexterity / 5;
        d.second += cur_dexterity / 5;
    }
    npc.cur.physics_damage.value[0] = d.first;    // KNpc::SetPhysicsDamage 0x08078E20
    npc.cur.physics_damage.value[2] = d.second;
}

int KPlayer::calc_exp(int exp, int player_level, int npc_level) noexcept
{
    if (player_level > 99) return npc_level <= 89 ? 1 : std::max(1, exp);
    const int d = player_level - npc_level;
    int out;
    if (d < -54) {
        out = d < -69 ? exp : exp * (-19 * d - 1030) / 300;
    } else {
        const int a = std::abs(d);
        if (a <= 5) out = exp;
        else if (a <= 15) out = exp * (25 - a) / 20;
        else out = exp / 2;
    }
    return std::max(1, out);
}

int KPlayer::add_exp(KNpc& npc, int add, int npc_level, const KPlayerSet& tables, const KItemList* items, int (*rand)(void*, int), void* rand_ctx)
{
    // 0x080B00C0
    if (add <= 0 || !npc.alive()) return 0;
    add = calc_exp(add, static_cast<int>(npc.level), npc_level);
    add = add > 5'000'000 ? add / 100 * (100 + exp_enhance_percent2) : (100 + exp_enhance_percent2) * add / 100;
    // 0x080AF640: x (100 + p) / 100 (through /100 first above 999 999) plus lo + rand(hi - lo)
    std::int64_t gain = add > 999'999 ? static_cast<std::int64_t>(add / 100) * (100 + exp_enhance_percent)
                                      : static_cast<std::int64_t>(add) * (100 + exp_enhance_percent) / 100;
    if (exp_enhance_hi > exp_enhance_lo && rand) gain += exp_enhance_lo + rand(rand_ctx, exp_enhance_hi - exp_enhance_lo);
    else gain += exp_enhance_lo;
    // 0x080AFEA0: a character of level 200 gains nothing; otherwise up to the next level's need
    if (gain <= 0) return 0;
    if (npc.level > static_cast<std::uint32_t>(kMaxLevel - 1)) return 0;
    if (next_level_exp <= 0) next_level_exp = tables.level_exp(static_cast<int>(npc.level), reborn);
    exp = std::min<std::int64_t>(exp + gain, next_level_exp);
    if (next_level_exp > 0 && exp >= next_level_exp) return level_up(npc, true, tables, items) ? 1 : 0;
    return 0;
}

void KPlayer::updata_cur_data(KNpc& npc, bool clear_state, const KPlayerSet& tables, const KItemList* items)
{
    // 0x080AF550
    npc.clear_attrib(clear_state, tables.stamina().sit_add);
    cur_strength = strength;
    cur_dexterity = dexterity;
    cur_vitality = vitality;
    cur_energy = energy;
    cur_lucky = lucky;
    exp_enhance_lo = exp_enhance_hi = exp_enhance_percent = exp_enhance_percent2 = 0;   // Player+0xc8..+0xd4
    // KNpc::ReCalcStateEffect (0x0807D270): the states are not in the zone yet
    // KPlayer::ReCalcEquip (0x080AF3E0): every worn piece, base attributes then magic with the
    // awake suffixes (KItemList::GetEquipEnhance)
    if (items != nullptr) {
        const KNpcAttribModifyContext ctx{&tables, items, false};
        for (int part = 0; part < itempart_num; ++part) {
            const KItem* piece = items->find(items->equipped(part));
            if (piece == nullptr) continue;
            if (part == itempart_horse && npc.horse == 0) continue;   // 0x080AF4EA: a horse not ridden gives nothing
            apply_base_attribs(*piece, npc, ctx);
            apply_magic_attribs(*piece, npc, items->equip_enhance(part, static_cast<int>(npc.series)), ctx);
        }
    }
    // the physics damage from the weapon and the current points (LoadFrom / LevelUp /
    // AddBaseXXX all call SetNpcPhysicsDamage right after)
    set_npc_physics_damage(npc, items);
}

bool KPlayer::level_up(KNpc& npc, bool up, const KPlayerSet& tables, const KItemList* items)
{
    // 0x080AF800
    exp = 0;
    int sign;
    if (up) {
        if (npc.level > static_cast<std::uint32_t>(kMaxLevel - 1)) return false;
        ++npc.level;
        attribute_point += 5;
        skill_point += 1;
        sign = 1;
    } else {
        if (npc.level <= 1) return false;
        --npc.level;
        attribute_point -= 5;
        skill_point -= 1;
        sign = -1;
    }
    next_level_exp = tables.level_exp(static_cast<int>(npc.level), reborn);
    const int series = static_cast<int>(npc.series), sex = static_cast<int>(npc.sex);
    // LevelAddBaseLifeMax / StaminaMax / ManaMax (0x080AF250 / 0x080AF1A0 / 0x080AF120)
    npc.base.life_max += tables.life_per_level(series) * sign;
    npc.cur.life_max = npc.cur.life_max_yan = npc.base.life_max;
    npc.base.stamina_max += tables.stamina_per_level(series, sex) * sign;
    npc.cur.stamina_max = npc.base.stamina_max;
    npc.cur.set_stamina_sit_add(tables.stamina().sit_add);
    npc.base.mana_max += tables.mana_per_level(series) * sign;
    npc.cur.mana_max = npc.cur.mana_max_yan = npc.base.mana_max;
    set_npc_resist(npc, tables);
    npc.cur.fire_resist = npc.base.fire_resist;
    npc.cur.cold_resist = npc.base.cold_resist;
    npc.cur.poison_resist = npc.base.poison_resist;
    npc.cur.light_resist = npc.base.light_resist;
    npc.cur.physics_resist = npc.base.physics_resist;
    npc.cur.fire_resist_max = npc.base.fire_resist_max;
    npc.cur.cold_resist_max = npc.base.cold_resist_max;
    npc.cur.poison_resist_max = npc.base.poison_resist_max;
    npc.cur.light_resist_max = npc.base.light_resist_max;
    npc.cur.physics_resist_max = npc.base.physics_resist_max;
    const int camp = npc.current_camp;
    updata_cur_data(npc, true, tables, items);
    npc.current_camp = camp;
    // life, stamina and mana filled to the maxima in use
    npc.cur.life = npc.life_max();
    npc.cur.stamina = npc.cur.stamina_max;
    npc.cur.mana = npc.mana_max();
    return true;
}

bool KPlayer::add_base_strength(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items)
{
    if (check && n > attribute_point) return false;
    if (strength + n < 0 || cur_strength + n < 0) return false;
    strength += n;
    cur_strength += n;
    attribute_point -= n;
    updata_cur_data(npc, false, tables, items);
    return true;
}

bool KPlayer::add_base_dexterity(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items)
{
    // 0x080B0B60: the base and the current point, the attack rating and the defence of the base
    // block, then UpdataCurData(0) and SetNpcPhysicsDamage
    if (check && n > attribute_point) return false;
    if (dexterity + n < 0 || cur_dexterity + n < 0) return false;
    dexterity += n;
    cur_dexterity += n;
    attribute_point -= n;
    set_npc_attack_rating(npc);
    set_npc_defence(npc);
    updata_cur_data(npc, false, tables, items);
    return true;
}

bool KPlayer::add_base_vitality(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items)
{
    if (check && n > attribute_point) return false;
    if (vitality + n < 0 || cur_vitality + n < 0) return false;
    vitality += n;
    cur_vitality += n;
    attribute_point -= n;
    // AddBaseLifeMax / AddBaseStaminaMax: LifePerVitality / StaminaPerVitality of the series
    const int series = static_cast<int>(npc.series);
    npc.base.life_max += tables.life_per_vitality(series) * n;
    npc.base.stamina_max += tables.stamina_per_vitality(series) * n;
    updata_cur_data(npc, false, tables, items);
    return true;
}

bool KPlayer::add_base_energy(KNpc& npc, int n, bool check, const KPlayerSet& tables, const KItemList* items)
{
    if (check && n > attribute_point) return false;
    if (energy + n < 0 || cur_energy + n < 0) return false;
    energy += n;
    cur_energy += n;
    attribute_point -= n;
    npc.base.mana_max += tables.mana_per_energy(static_cast<int>(npc.series)) * n;
    updata_cur_data(npc, false, tables, items);
    return true;
}

void KPlayer::change_cur_strength(KNpc& npc, int n, const KItemList* items) noexcept
{
    cur_strength += n;                      // 0x080B0B40
    set_npc_physics_damage(npc, items);
}

void KPlayer::change_cur_dexterity(KNpc& npc, int n, const KItemList* items) noexcept
{
    cur_dexterity += n;                     // 0x080B0AF0
    npc.cur.attack_rating += n * 4;
    npc.cur.defend += n / 4;                // toward zero (the cmovs / sar of the binary)
    set_npc_physics_damage(npc, items);
}

void KPlayer::change_cur_vitality(KNpc& npc, int n, const KPlayerSet& tables) noexcept
{
    cur_vitality += n;                      // 0x080B0E60
    const int series = static_cast<int>(npc.series);
    const int life = tables.life_per_vitality(series) * n;
    npc.cur.life_max += life;               // KNpc::AddCurLifeMax 0x08078C30: both twins
    npc.cur.life_max_yan += life;
    npc.cur.stamina_max += tables.stamina_per_vitality(series) * n;   // 0x08078CA0
    npc.cur.set_stamina_sit_add(tables.stamina().sit_add);
}

void KPlayer::change_cur_energy(KNpc& npc, int n, const KPlayerSet& tables) noexcept
{
    cur_energy += n;                        // 0x080B0DF0
    const int mana = tables.mana_per_energy(static_cast<int>(npc.series)) * n;
    npc.cur.mana_max += mana;               // KNpc::AddCurManaMax 0x08078D10: both twins
    npc.cur.mana_max_yan += mana;
}

} // namespace jx::zone
