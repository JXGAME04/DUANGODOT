// KSkills.cpp - KSkill::Cast of the old core (Core/Src/KSkills.cpp) the way the JX2 server casts
// (jx_linux_y; docs/LINUX-SERVER.md §12): Cast 0x080EA920 for a npc launcher, CastInitiativeSkill
// 0x080EAC90, CastPassivitySkill 0x080E8530, the StartEvent 0x080EAB90 and
// CreateMissleMagicAttribsData 0x080E9E90.  The missiles a style 0 / 14 skill fires are KMissle.cpp
// (CastMissles 0x080ECAC0, §13).
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <array>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpcAI.h"

namespace jx::zone {

int KSubWorld::skill_list_level(const KNpc& e, int skill_id) const noexcept
{
    // KSkillList::GetCurrentLevel(list, id, 1) 0x080E4440: the cell's current level with its
    // increments, 0 above 63 or without the skill (players and npcs alike)
    return e.skill_list.get_current_level(skill_id, true);
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
    const int enhance = launcher.skill_list.enhance[skill.row.id] + launcher.mana_skill_enhance + launcher.cur.skill_enhance;
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

// ---- the skill list of a npc (KSkillList.h; jx_linux_y KNpc+0x248, docs/LINUX-SERVER.md §15) ------

namespace {
// 0x0830CA14 + k x 4, k = 1..5: the character level a reborn character needs for the k-th skill
// level above the row's MaxLevel (KPlayer::AddSkillPoint 0x080BD8D0).  The table lives in .bss and
// is filled from the settings at start; its numbers were not found in the binary - 0 means no
// further requirement, as for a character that was never reborn.
constexpr std::array<int, 6> kRebornSkillLevelNeed{0, 0, 0, 0, 0, 0};
} // namespace

KSkillListHost KSubWorld::skill_host(KNpc& e)
{
    KSkillListHost h;
    h.skills = skills_.get();
    h.npc_level = static_cast<int>(e.level);
    const EntityId id = e.id;
    h.cast_passive = [this, id](const KSkill& sk) {
        // Cast(sk, idx, -1, idx, 0, 0, 1): the passive skill on the npc itself
        KNpc* n = entities_.find(id);
        if (n == nullptr) return;
        KCastParams p;
        p.target = id;
        p.extra = 1;
        skill_cast(sk, *n, p);
    };
    h.remove_state = [this, id](int skill_id) {
        if (KNpc* n = entities_.find(id)) remove_state_skill_effect(*n, skill_id, false);   // 0x0807D310(npc, id, 0)
    };
    return h;
}

void KSubWorld::load_skills(KNpc& e, const pb::RoleData& role)
{
    // KPlayer::LoadPlayerFightSkillList 0x080C0240: every record through KSkillList::Add(id,
    // level, exp, 0, 0, 0) - a reborn character's cells get its MaxLevel addon
    std::vector<KSkillSaved> saved;
    saved.reserve(static_cast<std::size_t>(role.skills_size()));
    for (const pb::RoleSkill& s : role.skills()) {
        saved.push_back(KSkillSaved{static_cast<int>(s.id()), static_cast<int>(s.level()), static_cast<int>(s.exp())});
    }
    KSkillListHost host = skill_host(e);
    const int addon = e.player.reborn != 0 ? e.player.skill_max_level_addons : 0;
    e.skill_list.deserialize(saved, addon, host);
    log::debug("zone.player", "skill list loaded", {log::kv("entity", e.id), log::kv("count", saved.size()), log::kv("held", e.skill_list.get_count())});
}

void KSubWorld::save_skills(const KNpc& e, pb::RoleData& out) const
{
    out.clear_skills();
    for (const KSkillSaved& s : e.skill_list.serialize()) {
        pb::RoleSkill* r = out.add_skills();
        r->set_id(static_cast<std::uint32_t>(s.id));
        r->set_level(static_cast<std::uint32_t>(std::max(0, s.level)));
        r->set_exp(static_cast<std::uint32_t>(std::max(0, s.exp)));
    }
}

void KSubWorld::send_skill_list(std::uint64_t sid)
{
    const KNpc* e = find_player(sid);
    if (e == nullptr) return;
    pb::SkillListSync out;
    out.set_forbid_all(e->skill_list.forbid_all != 0);
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = e->skill_list.skills[static_cast<std::size_t>(i)];
        if (c.id <= 0) continue;
        pb::SkillEntry* s = out.add_skills();
        s->set_skill_id(static_cast<std::uint32_t>(c.id));
        s->set_level(c.level);
        s->set_current_level(c.current_level);
        s->set_exp_percent(static_cast<std::uint32_t>(std::max(0, e->skill_list.exp_percent(i, skills_.get()))));
        s->set_max_level(c.max_level);
        s->set_req_level(c.req_level);
        s->set_forbidden(c.forbidden != 0);
        s->set_cool_down_left(c.next_cast_time > tick_ ? static_cast<std::uint32_t>(c.next_cast_time - tick_) : 0u);
        s->set_only_inc(c.only_inc);
    }
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_SKILL_LIST), out);
}

void KSubWorld::send_skill_level(std::uint64_t sid, int skill_id, int level, int exp_percent, std::uint32_t seq, bool level_up)
{
    const KNpc* e = find_player(sid);
    if (e == nullptr) return;
    pb::SkillLevelSync out;
    out.set_skill_id(static_cast<std::uint32_t>(std::max(0, skill_id)));
    out.set_level(level);
    out.set_skill_point(static_cast<std::uint32_t>(std::max(0, e->player.skill_point)));
    out.set_exp_percent(static_cast<std::uint32_t>(std::max(0, exp_percent)));
    out.set_seq(seq);
    out.set_level_up(level_up);
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_SKILL_LEVEL), out);
}

bool KSubWorld::give_skill_exp(KNpc& e, const KMagicAttrib& x, bool percent_mode)
{
    // 0x080E5D90: the list's part, then what a player is told
    KSkillListHost host = skill_host(e);
    const KSkillList::ExpResult r = e.skill_list.add_skill_exp(x, percent_mode, host);
    if (!r.handled) return false;
    const KNpcSkill* c = e.skill_list.cell(r.idx);
    log::trace("zone.fight", "skill exp", {log::kv("entity", e.id), log::kv("skill", x.value[0]), log::kv("exp", x.value[1]),
                                           log::kv("level", c->level), log::kv("percent", percent_mode)});
    if (e.kind != KNpcKind::player || !e.player.loaded) return false;   // 0x080E5F8D: nobody to tell (a partner's master - B3b)
    if (r.level_reached) {   // 0x080E62EE: the skill's script hears OnLevelUp(level + 1)
        const KSkill* sk = skills_ ? skills_->get(x.value[0], r.old_level) : nullptr;
        if (sk != nullptr && !sk->row.level_up_script.empty()) execute_script(sk->row.level_up_script, "OnLevelUp", e, c->level + 1);
    }
    const int pct = e.skill_list.exp_percent(r.idx, skills_.get());
    const bool leveled = c->level != r.old_level;
    if (!leveled && ((pct ^ r.old_percent) & 0xfff8) == 0) return false;   // 0x080E62A2: the bar moved by less than 8/1024
    send_skill_level(e.sid, x.value[0], c->level, pct, 0, leveled);
    if (leveled) log::info("zone.player", "skill level up", {log::kv("entity", e.id), log::kv("skill", x.value[0]), log::kv("level", c->level)});
    return leveled;
}

void KSubWorld::set_skill_cool_time(KNpc& e, int skill_id, int level)
{
    // 0x080847B0: TimePerCast of the level (on a horse the other column), less the state
    // modifier when it is on this skill's skill_mintimepercast_v (+0x19e4 on foot)
    int mod = 0;
    if (e.state_modifier.skill_id == skill_id && e.state_modifier.attrib == magic_skill_mintimepercast_v) mod = e.state_modifier.delta;
    if (skill_id < 1 || skill_id > 1999 || level < 1 || level > 63) return;
    const KSkill* sk = skills_ ? skills_->get(skill_id, level) : nullptr;
    if (sk == nullptr) return;
    e.skill_list.set_next_cast_time(skill_id, tick_, sk->row.time_per_cast - mod);
}

void KSubWorld::forbit_skill(KNpc& e, bool forbid)
{
    e.skill_list.set_forbid_all(forbid ? 1 : 0);   // 0x080B2950
    log::debug("zone.player", "skills locked", {log::kv("entity", e.id), log::kv("skill", 0), log::kv("forbid", forbid)});
    if (e.kind != KNpcKind::player || e.sid == 0) return;
    pb::SkillForbidSync out;
    out.set_skill_id(0);
    out.set_forbid(forbid);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_SKILL_FORBID), out);
}

void KSubWorld::set_a_forbit_skill(KNpc& e, int skill_id, int forbid)
{
    e.skill_list.set_forbid(skill_id, forbid);   // 0x080AE9E0
    log::debug("zone.player", "skills locked", {log::kv("entity", e.id), log::kv("skill", skill_id), log::kv("forbid", forbid != 0)});
    if (e.kind != KNpcKind::player || e.sid == 0) return;
    pb::SkillForbidSync out;
    out.set_skill_id(static_cast<std::uint32_t>(std::max(0, skill_id)));
    out.set_forbid(forbid != 0);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_SKILL_FORBID), out);
}

bool KSubWorld::add_skill_point_request(std::uint64_t sid, int skill_id, int points, std::uint32_t seq)
{
    // KPlayer::AddSkillPoint 0x080BD460, check by check in its order; `answer` = the old server
    // sent its 0x5e packet on that refusal (with the level unchanged and no experience)
    const auto pit = players_.find(sid);
    KNpc* e = pit == players_.end() ? nullptr : entities_.find(pit->second);
    if (e == nullptr || !e->player.loaded) return false;
    KPlayer& pl = e->player;
    KSkillList& list = e->skill_list;
    const int idx = list.find_same(skill_id);
    const int level = idx != 0 ? list.cell(idx)->level : 0;
    auto refuse = [&](const char* why, bool answer) {
        log::debug("zone.player", "skill points refused", {log::kv("sid", sid), log::kv("skill", skill_id), log::kv("points", points),
                                                           log::kv("reason", why), log::kv("left", pl.skill_point)});
        if (answer) send_skill_level(sid, skill_id, level, 0, seq);
        return false;
    };
    if (idx == 0) return refuse("not held", false);
    if (skill_id < 1 || skill_id > 1999) return refuse("bad id", false);
    if (list.cell(idx)->only_inc) return refuse("increments only", false);
    const KSkill* sk1 = skills_ ? skills_->get(skill_id, 1) : nullptr;
    if (sk1 == nullptr) return refuse("no row", false);
    const bool exp_skill = sk1->row.is_exp_skill;   // IsExpSkill == 1 skips the point check
    if (!exp_skill && pl.skill_point < points) return refuse("not enough points", false);
    const KSkill* sk = skills_->get(skill_id, level != 0 ? level : 1);
    if (sk == nullptr) return refuse("no instance", false);
    if (!sk->row.level_up_script.empty()) {   // 0x080BD5B3: the skill's own script decides (main(points))
        execute_script(sk->row.level_up_script, "main", *e, points);
        const int now = list.cell(idx)->level;
        send_skill_level(sid, skill_id, now, list.exp_percent(idx, skills_.get()), seq);
        log::debug("zone.player", "skill points spent", {log::kv("sid", sid), log::kv("skill", skill_id), log::kv("points", points),
                                                         log::kv("level", now), log::kv("script", sk->row.level_up_script)});
        return true;
    }
    const int style = sk->row.style;
    if (style == skill_style_thief) return refuse("thief skill", false);
    if (style > skill_style_thief && style != skill_style_jx2_14) return refuse("style", true);
    if (style > skill_style_passivity_npc_state && style < skill_style_thief) return refuse("style", true);
    if (exp_skill) return refuse("exp skill", false);   // 0x080BD977: they grow by use
    const KSkillRow* info = skills_->table() != nullptr ? skills_->table()->info(skill_id) : nullptr;
    const int max_level = info != nullptr ? info->max_level : 0;
    if (max_level == 1) return refuse("single level", false);   // 0x080BD72C
    const int new_level = points + level;
    const int addon = pl.reborn != 0 ? pl.skill_max_level_addons : 0;
    if (new_level > max_level + addon) return refuse("max level", true);   // 0x080BD7A5
    const int char_level = static_cast<int>(e->level);
    if (new_level > char_level + 1 - sk->row.req_level) return refuse("character level", true);   // 0x080BD8F9
    if (pl.reborn != 0) {   // 0x080BD90E: the reborn table for the levels above MaxLevel
        const int k = new_level - max_level;
        if (k >= 1 && k <= 5 && char_level < kRebornSkillLevelNeed[static_cast<std::size_t>(k)]) return refuse("reborn level", false);
    }
    KSkillListHost host = skill_host(*e);
    if (list.increase_level(idx, points, host) == 0) return refuse("no level", false);
    pl.skill_point -= points;
    send_skill_level(sid, skill_id, level + points, 0, seq);
    log::debug("zone.player", "skill points spent", {log::kv("sid", sid), log::kv("skill", skill_id), log::kv("points", points),
                                                     log::kv("level", level + points), log::kv("left", pl.skill_point)});
    return true;
}

} // namespace jx::zone
