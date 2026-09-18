// KSkills.cpp - KSkill::Cast of the old core (Core/Src/KSkills.cpp) the way the JX2 server casts
// (jx_linux_y; docs/LINUX-SERVER.md §12): Cast 0x080EA920 for a npc launcher, CastInitiativeSkill
// 0x080EAC90, CastPassivitySkill 0x080E8530, the StartEvent 0x080EAB90 and
// CreateMissleMagicAttribsData 0x080E9E90.  The missiles a style 0 / 14 skill fires are KMissle.cpp
// (CastMissles 0x080ECAC0, §13).
#include "jx/zone/KSubWorld.h"

#include <algorithm>

#include "jx/log.hpp"
#include "jx/zone/KNpcAI.h"

namespace jx::zone {

int KSubWorld::skill_list_level(const KNpc& e, int skill_id) const noexcept
{
    // KSkillList::GetLevel(list, id, 1) 0x080E4440: the slot's current level, 0 above 63 or
    // without the skill.  A player's list comes with B3; a npc's slots are m_SkillList's 1..4.
    if (e.kind == KNpcKind::player) return 0;
    for (int i = 1; i < 5; ++i) {
        const KNpcSkillSlot& s = e.skills[i];
        if (s.id != skill_id) continue;
        return s.level > 63 ? 0 : s.level;
    }
    return 0;
}

bool KSubWorld::create_missle_magic_attribs_data(const KSkill& skill, KNpc& launcher, KMissleMagicAttribsList& out)
{
    // 0x080E9E90: the states and the immediate attributes as they are, the damage attributes
    // with the launcher's numbers (AppendSkillEffect) and its enhance for this skill: the
    // per-skill map +0x115c (an entry of 0 is made), the mana enhance +0x139c, skill_enhance
    if (skill.row.client_send) return false;
    KMissleMagicAttribsData node;
    node.skill_id = skill.row.id;
    node.level = skill.level;
    node.state_attribs = skill.state_attribs;
    node.state_count = skill.state_attrib_count;
    node.immediate_attribs = skill.immediate_attribs;
    node.immediate_count = skill.immediate_attrib_count;
    node.damage_count = skill.damage_attrib_count;
    const int enhance = launcher.skill_enhance[skill.row.id] + launcher.mana_skill_enhance + launcher.cur.skill_enhance;
    append_skill_effect(launcher, skill.row.is_physical, skill.row.is_melee, skill.damage_attribs, node.damage_attribs, enhance);
    KMagicAttrib* states = node.state_attribs.data();
    if (node.state_count > 0 && launcher.state_modifier.skill_id == skill.row.id) {   // 0x080792C0
        for (int i = 0; i < node.state_count; ++i) {
            if (states[i].type != launcher.state_modifier.attrib) continue;
            const int k = launcher.state_modifier.index;
            if (k >= 0 && k < 3) states[i].value[static_cast<std::size_t>(k)] += launcher.state_modifier.delta;
            break;
        }
    }
    out.push_back(node);
    // 0x080EA140: the appended skills (skill_appendskill) at the lower of their level and the
    // launcher's, followed down ChildSkillId to a BaseSkill, each with its own payload
    for (const auto& [id, level] : skill.append_skills) {
        int lvl = skill_list_level(launcher, id);
        if (lvl <= 0) continue;
        lvl = std::min(lvl, level);
        if (lvl <= 0 || id < 1 || id > kMaxSkill || lvl > kMaxSkillLevel - 1) continue;
        const KSkill* sk = skills_ ? skills_->get(id, lvl) : nullptr;
        while (sk != nullptr && !sk->row.base_skill) {
            const int child = sk->row.child_skill_id;
            sk = child >= 1 && child <= kMaxSkill ? skills_->get(child, lvl) : nullptr;
        }
        if (sk != nullptr) create_missle_magic_attribs_data(*sk, launcher, out);
    }
    return true;
}

bool KSubWorld::cast_initiative_skill(const KSkill& skill, KNpc& launcher, int param1, EntityId target, int wait_time, int extra,
                                      int time_override, bool refresh, int param10, int param12)
{
    (void)wait_time;   // nWaitTime is handed over and not read (0x080EAC90)
    // 0x080EAC9F: a cast at a spot is a cast on oneself (TargetSelf); a cast on a npc needs the
    // relation the skill allows
    KNpc* t = nullptr;
    if (param1 != -1) {
        if (!skill.row.target_self) return false;
        t = &launcher;
    } else {
        t = entities_.find(target);
        if (t == nullptr) return false;
        const int rel = relation(launcher, *t);
        bool ok = skill.row.target_self && (rel & relation_self) != 0;
        if (!ok) {
            if (skill.row.target_no_npc && t->kind != KNpcKind::player) return false;
            if (skill.row.target_enemy && (rel & relation_enemy) != 0) ok = true;
            else if (skill.row.target_ally && (rel & relation_ally) != 0) ok = true;
        }
        if (!ok) return false;
    }
    KMissleMagicAttribsList list;
    if (create_missle_magic_attribs_data(skill, launcher, list)) {
        const int before = t->cur.life;
        for (const KMissleMagicAttribsData& node : list) {
            if (receive_damage(*t, launcher, skill.row.series, skill.row.is_melee, node.damage_attribs.data(), skill.row.use_attack_rate, skill.row.do_hurt, skill.row.relation, node.skill_id) == 0) continue;
            t->people_id = launcher.id;
            if (time_override > 0) {   // 0x080EADB3: every state given the same time
                std::array<KMagicAttrib, kSkillAttribs> buf = node.state_attribs;
                for (int i = 0; i < node.state_count; ++i) buf[static_cast<std::size_t>(i)].value[1] = time_override;
                set_state_skill_effect(*t, launcher.id, node.skill_id, node.level, buf.data(), node.state_count, time_override, extra, refresh, param10, false, param12);
            } else {
                set_state_skill_effect(*t, launcher.id, node.skill_id, node.level, node.state_attribs.data(), node.state_count, node.state_attribs[0].value[1], extra, false, 0, false, param12);
            }
            if (node.immediate_count > 0) set_immediately_skill_effect(*t, launcher.id, node.immediate_attribs.data(), node.immediate_count);
        }
        sync_life(*t, before, launcher.id);
    }
    KCastParams p;
    p.target = t->id;
    skill_start_event(skill, launcher, p);   // 0x080EAEE4: (this, launcher, -1, p2)
    return true;
}

bool KSubWorld::cast_passivity_skill(const KSkill& skill, KNpc& launcher, int extra)
{
    // 0x080E8530: the skill's states on the launcher itself, for good (-1), after the state modifier
    if (skill.state_attrib_count <= 0) return true;
    std::array<KMagicAttrib, kSkillAttribs> buf = skill.state_attribs;
    if (launcher.state_modifier.skill_id == skill.row.id) {   // 0x080792C0
        for (int i = 0; i < skill.state_attrib_count; ++i) {
            if (buf[static_cast<std::size_t>(i)].type != launcher.state_modifier.attrib) continue;
            const int k = launcher.state_modifier.index;
            if (k >= 0 && k < 3) buf[static_cast<std::size_t>(i)].value[static_cast<std::size_t>(k)] += launcher.state_modifier.delta;
            break;
        }
    }
    set_state_skill_effect(launcher, launcher.id, skill.row.id, skill.level, buf.data(), skill.state_attrib_count, -1, extra);
    return true;
}

void KSubWorld::skill_start_event(const KSkill& skill, KNpc& launcher, const KCastParams& p)
{
    // 0x080EAB90: StartSkillId cast at EventSkillLevel (-1 = this level) with the same target,
    // then the launcher's on-cast map for this skill (0x080821C0)
    if (skill.row.start_event && skill.row.start_skill_id > 0) {
        int level = skill.row.event_skill_level;
        if (level == -1) level = skill.level;
        if (level > 0 && skill.row.start_skill_id <= kMaxSkill && level <= kMaxSkillLevel - 1) {
            if (const KSkill* sk = skills_ ? skills_->get(skill.row.start_skill_id, level) : nullptr; sk != nullptr) {
                KCastParams q = p;
                q.wait_time = 0;
                q.extra = 0;
                skill_cast(*sk, launcher, q);
            }
        }
    }
    cast_on_cast_skills(launcher, skill.row.id, skill.level, p);
}

void KSubWorld::cast_on_cast_skills(KNpc& launcher, int skill_id, int level, const KCastParams& p)
{
    // 0x080821C0(npc, skill, level, p1, p2): the entries under the skill in the npc's map (+0x18ec)
    // roll their percent; each skill that comes up (1..1999 at this level 1..63, of style 0, 1, 2
    // or 14) is cast with the same target or spot.  The casts may change the map: a copy is walked.
    const auto it = launcher.on_cast_skills.find(skill_id);
    if (it == launcher.on_cast_skills.end()) return;
    const std::map<int, int> entries = it->second;
    for (const auto& [id, rate] : entries) {
        if (!(rate > random(100))) continue;
        if (level <= 0 || id < 1 || id > 1999 || level > 63) continue;
        const KSkill* sk = skills_ ? skills_->get(id, level) : nullptr;
        if (sk == nullptr) continue;
        const int style = sk->row.style;
        if (style < 0 || style > skill_style_jx2_14 || ((1 << style) & 0x4007) == 0) continue;
        KCastParams q = p;
        q.wait_time = 0;
        q.extra = 0;
        skill_cast(*sk, launcher, q);
        // (the cast sync 0x85 to the clients: B4)
    }
}

bool KSubWorld::skill_cast(const KSkill& skill, KNpc& launcher, const KCastParams& p)
{
    // KSkill::Cast 0x080EA920 with a npc launcher (eLauncherType 0): a cast on a npc needs it on
    // this map (0x080EA956), a cast in a direction a direction (0x080EA9D4), a negative wait is 0
    // (logged), a style above 14 is nothing, then the jump table 0x0825843C on the style
    if (!p.at_pos && p.dir < 0 && entities_.find(p.target) == nullptr) return false;
    KCastParams q = p;
    if (q.wait_time < 0) {
        log::trace("zone.fight", "cast wait below zero", {log::kv("entity", launcher.id), log::kv("skill", skill.row.id)});
        q.wait_time = 0;
    }
    const int style = skill.row.style;
    if (style > skill_style_jx2_14) return true;
    switch (style) {
    case skill_style_missles:   // 0x080EAAF8: CastMissles
        cast_missles(skill, nullptr, launcher, q);
        return true;
    case skill_style_jx2_14:    // 0x080EAAD0: the instant missile
        cast_instant_missle(skill, launcher, q);
        return true;
    case skill_style_melee:   // 1: nothing on the server
        return true;
    case skill_style_initiative_npc_state:
        cast_initiative_skill(skill, launcher, q.at_pos ? 0 : -1, q.target, q.wait_time, q.extra);
        return true;
    case skill_style_passivity_npc_state:
        cast_passivity_skill(skill, launcher, q.extra);
        return true;
    case skill_style_create_npc:   // 0x080E8770: B2c
        log::trace("zone.fight", "create npc skill not carried", {log::kv("entity", launcher.id), log::kv("skill", skill.row.id)});
        return true;
    default:   // 5..13: nothing (0x080EA9EC)
        return true;
    }
}

const KSkill* KSubWorld::swing_skill(const KNpc& e)
{
    // A player's basic attack is skill 1 (melee) or 2 (a ranged weapon) - the client's choice
    // of the left click comes with B3; a npc swings its active skill at the level of its slot.
    int id = 1;
    int level = 1;
    if (e.kind == KNpcKind::player) {
        const KItemList* items = e.sid != 0 ? items_of(e.sid) : nullptr;
        if (items != nullptr && items->weapon_type() == 1) id = 2;
    } else if (e.active_skill_id > 0) {
        id = e.active_skill_id;
        level = std::max(1, skill_list_level(e, id));
    }
    if (skills_ != nullptr) {
        if (const KSkill* s = skills_->get(id, level); s != nullptr) return s;
    }
    if (const KSkill* s = KSkill::basic_attack(id); s != nullptr) return s;
    if (skills_ == nullptr) return KSkill::basic_attack(1);   // a monster's own skill without a table: a plain blow
    return nullptr;
}

void KSubWorld::on_skill(KNpc& e, KNpc& target)
{
    // KNpc::OnSkill: at 60 % of the swing the active skill is cast at the target (or oneself)
    const KSkill* skill = swing_skill(e);
    if (skill == nullptr) {
        log::trace("zone.fight", "no skill for the swing", {log::kv("entity", e.id), log::kv("skill", e.active_skill_id)});
        return;
    }
    KCastParams p;
    p.target = target.id;
    skill_cast(*skill, e, p);
}

} // namespace jx::zone
