// The teams of a world: KPlayerTeam / KTeam / KTeamSet of the old game as jx_linux_y runs them (docs/LINUX-SERVER.md §17).
// The client's 0x53 packet {sub 1..11, npc} lands in team_request; the answers are G2C_TEAM_EVENT (the 0x69 sub-commands
// and the 0x86 team messages) and G2C_TEAM_SELF (PLAYER_SEND_SELF_TEAM_INFO, sent to every member whenever the team changed).
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"

namespace jx::zone {

namespace {

// KPlayer::FindAroundPlayer / S2CSendTeamInfo: the players of the region and its eight neighbours - the cells of the zone
// are 512 units, so two cells around cover the old 3 x 3 regions
constexpr std::int64_t kAroundDist2 = 1536LL * 1536LL;
// AddExpTeam 0x080B0597 / 0x0809BE79 / 0x080CC620: "near" = within 0x100000 of squared distance (1024 units)
constexpr std::int64_t kTeamNearDist2 = 0x100000;

std::int64_t dist2(const KNpc& a, const KNpc& b) noexcept
{
    const std::int64_t dx = static_cast<std::int64_t>(a.pos().x) - b.pos().x;
    const std::int64_t dy = static_cast<std::int64_t>(a.pos().y) - b.pos().y;
    return dx * dx + dy * dy;
}

void fill_member(pb::TeamMember* m, const KNpc& e)
{
    m->set_entity_id(e.id.value);
    m->set_name(e.name);
    m->set_level(e.level);
}

}  // namespace

// ---- lookups -----------------------------------------------------------------------------------------------

KNpc* KSubWorld::team_player(std::uint64_t sid)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return nullptr;
    KNpc* e = entities_.find(pit->second);
    return e != nullptr && e->kind == KNpcKind::player && e->player.loaded ? e : nullptr;
}

const KTeam* KSubWorld::team_of(const KNpc& e) const noexcept
{
    if (e.kind != KNpcKind::player || !e.player.team.flag) return nullptr;
    const KTeam* t = teams_.get(e.player.team.id);
    return t != nullptr && !t->empty() ? t : nullptr;
}

KTeam* KSubWorld::team_of(const KNpc& e) noexcept
{
    if (e.kind != KNpcKind::player || !e.player.team.flag) return nullptr;
    KTeam* t = teams_.get(e.player.team.id);
    return t != nullptr && !t->empty() ? t : nullptr;
}

KNpc* KSubWorld::find_around_player(const KNpc& e, EntityId npc)
{
    KNpc* t = entities_.find(npc);
    if (t == nullptr || t->kind != KNpcKind::player || !t->player.loaded || t == &e) return nullptr;
    if (dist2(e, *t) > kAroundDist2) return nullptr;
    return t;
}

int KSubWorld::team_members_max(const KTeam& t) const noexcept
{
    const KNpc* c = find_player(t.captain);
    return tables().lead_members(c != nullptr ? c->player.lead_level : 1);   // 0x080CC960: Player[captain]+0x5970 -> the table
}

bool KSubWorld::team_full(const KTeam& t) const noexcept
{
    if (!t.lead_limit) return false;   // 0x080CC99F: +0x2c == 0 -> never full
    return t.count >= team_members_max(t);
}

int KSubWorld::team_near_count(const KNpc& e) const noexcept
{
    const KTeam* t = team_of(e);
    if (t == nullptr) return 0;
    int n = 0;
    for (const std::uint64_t sid : t->people()) {
        if (sid == e.sid) continue;
        const KNpc* m = find_player(sid);
        if (m == nullptr) continue;
        const std::int64_t d = dist2(e, *m);   // 0x0809E660: -1 when apart (another map), else the squared distance
        if (d >= 0 && d <= 0xfffff) ++n;       // 0x080CC671 / 0x080CC6B0
    }
    return n;
}

// the captain's npc id on every member: what KDamageRecord::Add 0x0809BC70 keys a team's hits by
void KSubWorld::team_sync_captain(KTeam& t)
{
    const KNpc* c = find_player(t.captain);
    const std::uint64_t npc = c != nullptr ? c->id.value : 0;
    for (const std::uint64_t sid : t.people()) {
        if (KNpc* m = team_player(sid)) m->player.team.captain_npc = npc;
    }
}

// KPlayer::ServerPickUpItem 0x080B826C..0x080B83BB: in a team, a pile of money or an item that is not a task item may be
// taken when its owner is the captain or a member of one's team (KTeam 0x080CC210); a task item only by its owner
bool KSubWorld::team_may_take(const KNpc& e, const KNpc& object) const noexcept
{
    const KTeam* t = team_of(e);
    if (t == nullptr) return false;
    if (object.object.kind != KObjKind::money) {
        const auto it = ground_items_.find(object.id.value);
        if (it != ground_items_.end() && it->second.genre == KItemGenre::task) return false;
    }
    for (const std::uint64_t sid : t->people()) {
        const KNpc* m = find_player(sid);
        if (m != nullptr && m->player_id == object.object.belong) return true;
    }
    return false;
}

// ---- telling the clients -----------------------------------------------------------------------------------

void KSubWorld::team_event(std::uint64_t sid, pb::TeamEventKind kind, EntityId who, int arg)
{
    pb::TeamEvent ev;
    ev.set_event(kind);
    ev.set_arg(arg);
    if (who.value != 0) {
        ev.set_entity_id(who.value);
        if (const KNpc* w = entities_.find(who)) {
            ev.set_name(w->name);
            ev.set_level(w->level);
        }
    }
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_EVENT), ev);
}

void KSubWorld::team_event_all(const KTeam& t, pb::TeamEventKind kind, EntityId who, int arg)
{
    for (const std::uint64_t sid : t.people()) team_event(sid, kind, who, arg);
}

// KPlayer::SendSelfTeamInfo 0x080AA7F0: the captain first, then the members, the team state, the leadership; out of a team
// the s2c_teamleave with one's own id
void KSubWorld::team_send_self(const KNpc& e)
{
    if (e.kind != KNpcKind::player) return;
    pb::TeamSelf m;
    m.set_lead_level(static_cast<std::uint32_t>(std::max(1, e.player.lead_level)));
    m.set_lead_exp(static_cast<std::uint64_t>(std::max<std::int64_t>(0, e.player.lead_exp)));
    const KTeam* t = team_of(e);
    if (t == nullptr) {
        m.set_in_team(false);
        emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_SELF), m);
        return;
    }
    m.set_in_team(true);
    m.set_team_id(e.player.team.id);
    m.set_state(t->state);
    m.set_captain(t->captain == e.sid);
    m.set_members_max(team_members_max(*t));
    if (const KNpc* c = find_player(t->captain)) fill_member(m.mutable_leader(), *c);
    for (const std::uint64_t sid : t->members) {
        if (sid == 0) continue;
        if (const KNpc* p = find_player(sid)) fill_member(m.add_members(), *p);
    }
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_SELF), m);
}

void KSubWorld::team_send_self_all(const KTeam& t)
{
    for (const std::uint64_t sid : t.people()) {
        if (const KNpc* p = find_player(sid)) team_send_self(*p);
    }
}

// ---- KTeam -----------------------------------------------------------------------------------------------

// KTeam::SetTeamOpen 0x080CD960: refused while the captain trades (CheckTrading 0x080A7E90) or still loads (+0x5f88), the
// message 0x28 (MSG_TEAM_ERROR05) for a captain the system appointed (+0x59c4), nothing when full; else open, the 0x69
// {6, 1} to the captain and the menu-state icon around (0x76: the "looking for members" sign - not drawn by this client)
bool KSubWorld::team_set_open(KTeam& t)
{
    KNpc* c = team_player(t.captain);
    if (c == nullptr) return false;
    if (c->player.team.auto_captain) {
        team_event(c->sid, pb::TEAM_EV_MSG, EntityId{}, 0x28);
        return false;
    }
    if (team_full(t)) return false;
    if (trading(*c)) return false;   // CheckTrading 0x080A7E90
    t.state = 1;
    team_event(c->sid, pb::TEAM_EV_OPEN_CLOSE, EntityId{}, 1);
    if (c->player.menu.state != menu_state_team_open) set_menu_state(*c, menu_state_team_open, {}, EntityId{});   // 0x080CD9F0..
    return true;
}

// KTeam::SetTeamClose 0x080CCA80: refused while the captain trades; state 0, the 0x69 {6, 0} to the captain
bool KSubWorld::team_set_close(KTeam& t)
{
    KNpc* c = team_player(t.captain);
    if (c == nullptr) return false;
    t.state = 0;
    team_event(c->sid, pb::TEAM_EV_OPEN_CLOSE, EntityId{}, 0);
    if (c->player.menu.state == menu_state_team_open) set_menu_state(*c, menu_state_normal, {}, EntityId{});   // 0x080CCAB0..
    return true;
}

// KTeam::AddMember 0x080CC9D0: a captain, the newcomer of the camp rule (CheckAddCondition: a newcomer of a camp other
// than begin (0) cannot join a captain of camp begin), not full, a free slot
bool KSubWorld::team_add_member(KTeam& t, KNpc& p)
{
    if (t.captain == 0) return false;
    const KNpc* c = find_player(t.captain);
    if (c == nullptr) return false;
    if (p.camp != 0 && c->camp == 0) return false;   // 0x080CCA18..0x080CCA36
    if (team_full(t)) return false;
    const int slot = t.find_free();
    if (slot < 0) return false;
    t.members[static_cast<std::size_t>(slot)] = p.sid;
    ++t.count;
    return true;
}

// 0x080CD480: the team closes, the first member of the captain's camp (0x080CC0D0) takes the lead - the old captain becomes a
// member, the new one is marked as appointed by the system (+0x59c4 = 1), the 0x69 {0xd, new, old} goes to everyone; a team without
// members is left instead (0x080CD5A8 -> LeaveTeam)
void KSubWorld::team_hand_over(KTeam& t)
{
    KNpc* old = team_player(t.captain);
    if (old == nullptr) return;
    team_set_close(t);
    if (t.count <= 0) {
        team_leave(*old);
        return;
    }
    int pick = -1;
    for (int i = 0; i < kTeamMembers; ++i) {
        const KNpc* m = find_player(t.members[static_cast<std::size_t>(i)]);
        if (m != nullptr && m->camp == old->camp) { pick = i; break; }
    }
    if (pick < 0) {
        for (int i = 0; i < kTeamMembers; ++i) {
            if (find_player(t.members[static_cast<std::size_t>(i)]) != nullptr) { pick = i; break; }
        }
    }
    if (pick < 0) {
        team_leave(*old);
        return;
    }
    KNpc* fresh = team_player(t.members[static_cast<std::size_t>(pick)]);
    if (fresh == nullptr) return;
    t.members[static_cast<std::size_t>(pick)] = old->sid;
    t.captain = fresh->sid;
    old->player.team.figure = 1;
    fresh->player.team.figure = 0;
    fresh->player.team.auto_captain = true;   // 0x080CD4FE
    team_sync_captain(t);
    log::info("zone.team", "captain handed over", {log::kv("team", old->player.team.id), log::kv("from", old->id), log::kv("to", fresh->id)});
    for (const std::uint64_t sid : t.people()) {
        pb::TeamEvent ev;
        ev.set_event(pb::TEAM_EV_CHANGE_CAPTAIN);
        ev.set_entity_id(fresh->id.value);
        ev.set_name(fresh->name);
        ev.set_level(fresh->level);
        ev.set_arg(sid == fresh->sid ? 1 : 0);
        emit({sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_EVENT), ev);
    }
    team_send_self_all(t);
}

// KTeam::DeleteMember 0x080CD5E0: the captain going ends the team (enumMSG_ID_TEAM_DISMISS + s2c_teamleave to everyone, each
// camp restored) unless the leadership limit is off and members remain (0x080CD730: then 0x080CD480 hands the lead over); a
// member going is told to all, its camp restored, the slot freed
void KSubWorld::team_delete_member(KTeam& t, KNpc& p)
{
    const int id = p.player.team.id;
    if (t.captain == p.sid) {
        if (!t.lead_limit && t.count > 0) {
            team_hand_over(t);
            if (t.captain == p.sid) return;   // nobody to take it
            // now a member: fall through as a member leaving
        } else {
            for (const std::uint64_t sid : t.people()) {
                KNpc* m = team_player(sid);
                if (m == nullptr) continue;
                team_event(sid, pb::TEAM_EV_DISMISS);
                team_event(sid, pb::TEAM_EV_LEAVE, m->id);
                m->player.team.release();
                set_current_camp(*m, m->camp);   // RestoreCurrentCamp 0x0807B8F0
                team_send_self(*m);
            }
            log::info("zone.team", "team dismissed", {log::kv("team", id), log::kv("captain", p.id)});
            t.release();
            return;
        }
    }
    const int slot = t.find_member(p.sid);
    if (slot < 0) return;
    team_event_all(t, pb::TEAM_EV_LEAVE, p.id);
    t.members[static_cast<std::size_t>(slot)] = 0;
    --t.count;
    p.player.team.release();
    set_current_camp(p, p.camp);
    team_send_self(p);
    team_send_self_all(t);
    log::info("zone.team", "member left", {log::kv("team", id), log::kv("entity", p.id), log::kv("count", t.count)});
}

// ---- the requests ------------------------------------------------------------------------------------------

bool KSubWorld::team_request(std::uint64_t sid, int cmd, EntityId target, int flag)
{
    KNpc* me = team_player(sid);
    if (me == nullptr) return false;
    switch (cmd) {
    case pb::TEAM_INFO: team_info(*me, target); return true;
    case pb::TEAM_CREATE: return team_create(*me);
    case pb::TEAM_OPEN_CLOSE: return team_set_state(*me, flag != 0);
    case pb::TEAM_APPLY_ADD: team_apply_add(*me, target); return true;
    case pb::TEAM_ACCEPT: return team_accept(*me, target);
    case pb::TEAM_LEAVE: team_leave(*me); return true;
    case pb::TEAM_KICK: team_kick(*me, target); return true;
    case pb::TEAM_CHANGE_CAPTAIN: team_change_captain(*me, target); return true;
    case pb::TEAM_DISMISS: team_dismiss(*me); return true;
    case pb::TEAM_INVITE: team_invite(*me, target); return true;
    case pb::TEAM_REPLY_INVITE: team_reply_invite(*me, target, flag != 0); return true;
    default: return false;
    }
}

// KPlayerTeam::CreateTeam 0x080CE3C0: a npc of camp 6 cannot; can_team off or the task value 0x87 bit 0x400 (Lua
// DisabledTeam) -> the 0x69 {5, 4}; in a team already -> {5, 0}; no free team -> {5, 2}; else the captain of a fresh closed
// team (figure 0), the camp restored (0x0807B8F0 unless Player+0xd8 / +0xdc), the 0x69 {4, team id, camp}, the team opened
// (SetTeamOpen) and the auto-captain flag cleared (0x080CE54C)
bool KSubWorld::team_create(KNpc& e)
{
    if (e.camp == 6) return false;   // 0x080CE3F9
    if (!e.player.team.can_team || e.player.team.lua_disabled) {
        team_event(e.sid, pb::TEAM_EV_CREATE_FAIL, EntityId{}, 4);
        return false;
    }
    if (e.player.team.flag) {
        team_event(e.sid, pb::TEAM_EV_CREATE_FAIL, EntityId{}, 0);
        return false;
    }
    const int id = teams_.create(e.sid);
    if (id < 0) {
        team_event(e.sid, pb::TEAM_EV_CREATE_FAIL, EntityId{}, 2);
        return false;
    }
    e.player.team.flag = true;
    e.player.team.id = id;
    e.player.team.figure = 0;
    e.player.team.apply_captain = 0;
    e.player.team.captain_npc = e.id.value;
    set_current_camp(e, e.camp);   // RestoreCurrentCamp
    team_event(e.sid, pb::TEAM_EV_CREATE_OK, e.id, e.current_camp);
    log::info("zone.team", "team created", {log::kv("team", id), log::kv("captain", e.id)});
    if (KTeam* t = teams_.get(id)) team_set_open(*t);   // 0x080CE542
    e.player.team.auto_captain = false;   // 0x080CE54C
    team_send_self(e);
    return true;
}

// KPlayer::SetTeamState 0x080B1CF0: not while trading; a captain only (else the own info again); 0 closes, else opens (a
// trade menu state dropped first - no trade in the zone)
bool KSubWorld::team_set_state(KNpc& e, bool open)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain()) {
        team_send_self(e);
        return false;
    }
    const bool ok = open ? team_set_open(*t) : team_set_close(*t);
    if (ok) team_send_self_all(*t);
    return ok;
}

// KPlayer::S2CSendAddTeamInfo 0x080B8000: can_team (else the 0x86 message 0x12), not of camp 6, the captain around
// (0x080E0D80 by npc id in the regions around), the camp rule (a captain of camp begin takes no one of another camp), then
// m_nApplyCaptainID = that npc and the 0x69 {7, my npc} (s2c_teamgetapply) to the captain
void KSubWorld::team_apply_add(KNpc& e, EntityId target)
{
    if (!e.player.team.can_team) {
        team_event(e.sid, pb::TEAM_EV_MSG, EntityId{}, 0x12);   // 0x080B81A0
        return;
    }
    if (e.camp == 6) return;   // 0x080B804B
    KNpc* c = find_around_player(e, target);
    if (c == nullptr) return;
    if (c->camp == 0 && e.camp != 0) return;   // 0x080B80AB..0x080B80C8
    e.player.team.apply_captain = c->id.value;
    team_event(c->sid, pb::TEAM_EV_APPLY, e.id);
    log::debug("zone.team", "team apply", {log::kv("entity", e.id), log::kv("captain", c->id)});
}

// the tail shared by AddTeamMember 0x080B75B0 and GetInviteReply 0x080CCBA0 (0x080B7730.. / 0x080CCC84..): AddMember, the
// team closed when it is full (the limit or the seven slots), the newcomer's KPlayerTeam set (flag, member, id), its current
// camp = the captain's camp (SetCurrentCamp), s2c_teamaddmember to the others, the whole team to the newcomer, TEAM_SELF_ADD
void KSubWorld::team_join(KTeam& t, int id, KNpc& newcomer, KNpc& captain)
{
    if (t.count >= kTeamMembers || team_full(t)) team_set_close(t);   // 0x080B7745 / 0x080CCC9A
    newcomer.player.team.release();
    newcomer.player.team.flag = true;
    newcomer.player.team.figure = 1;
    newcomer.player.team.id = id;
    newcomer.player.team.captain_npc = captain.id.value;
    set_current_camp(newcomer, captain.camp);   // 0x080B77B3..: Npc[new].SetCurrentCamp(Npc[captain].m_Camp)
    for (const std::uint64_t sid : t.people()) {
        if (sid == newcomer.sid) continue;
        team_event(sid, pb::TEAM_EV_ADD_MEMBER, newcomer.id);
    }
    team_event(newcomer.sid, pb::TEAM_EV_SELF_ADD, captain.id);
    team_send_self_all(t);
    log::info("zone.team", "member joined", {log::kv("team", id), log::kv("entity", newcomer.id), log::kv("count", t.count)});
}

// KPlayer::AddTeamMember 0x080B75B0: a captain of an open team with room (< 7 and under the leadership limit; a captain the
// system appointed -> the message 0x24); the applicant around, can_team (else 0x13), not of camp 6, not in a team, having
// applied to this captain
bool KSubWorld::team_accept(KNpc& e, EntityId target)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain() || t->state == 0 || t->count > 6 || t->count >= team_members_max(*t)) {
        team_send_self(e);   // 0x080B75CF: SendSelfTeamInfo
        return false;
    }
    if (e.player.team.auto_captain) {
        team_event(e.sid, pb::TEAM_EV_MSG, EntityId{}, 0x24);   // 0x080B7645.. MSG_TEAM_ERROR01
        return false;
    }
    KNpc* p = find_around_player(e, target);
    if (p == nullptr) return false;
    if (!p->player.team.can_team) {
        team_event(e.sid, pb::TEAM_EV_MSG, p->id, 0x13);   // 0x080B7A18: MSG_TEAM_TARGET_CANNOT_ADD_TEAM
        return false;
    }
    if (p->camp == 6) return false;                              // 0x080B76DE
    if (p->player.team.flag) return false;                       // 0x080B76EE
    if (p->player.team.apply_captain != e.id.value) return false;   // 0x080B7702
    if (!team_add_member(*t, *p)) return false;
    team_join(*t, e.player.team.id, *p, e);
    return true;
}

// KPlayer::LeaveTeam 0x080B7C60: nothing out of a team; a captain of an open team closes it first; a member leaving is told
// to the captain and the members (enumMSG_ID_TEAM_LEAVE); then KTeam::DeleteMember
void KSubWorld::team_leave(KNpc& e)
{
    KTeam* t = team_of(e);
    if (t == nullptr) {
        if (e.player.team.flag) e.player.team.release();   // a stale flag (the team is gone)
        return;
    }
    if (e.player.team.captain() && t->state != 0) team_set_close(*t);
    team_delete_member(*t, e);
}

// KPlayer::TeamKickOne 0x080B9880: a captain (one the system appointed -> the message 0x25); the npc must be a member;
// DeleteMember, then enumMSG_ID_TEAM_KICK_One with the name to the kicked one, the captain and the members
void KSubWorld::team_kick(KNpc& e, EntityId target)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain()) return;
    if (e.player.team.auto_captain) {
        team_event(e.sid, pb::TEAM_EV_MSG, EntityId{}, 0x25);   // 0x080B9A78: MSG_TEAM_ERROR02
        return;
    }
    KNpc* p = entities_.find(target);
    if (p == nullptr || p->kind != KNpcKind::player) return;
    if (t->find_member(p->sid) < 0) return;
    const std::vector<std::uint64_t> people = t->people();
    team_delete_member(*t, *p);
    for (const std::uint64_t sid : people) team_event(sid, pb::TEAM_EV_KICK, p->id);
    log::info("zone.team", "member kicked", {log::kv("team", e.player.team.id), log::kv("entity", p->id)});
}

// KPlayer::TeamChangeCaptain 0x080B9400: a captain (one the system appointed -> the message 0x26); the npc a member; a
// member of camp begin cannot lead a captain of another camp (the message 7: FAIL1 + FAIL3); the new leader's leadership
// must hold the members (the message 6: FAIL1 + FAIL2); an open team closes for the swap and opens again; the two swap
// places, the figures follow, the new captain's auto flag clears (0x080B9579), its camp restored, every member's current
// camp = the new captain's camp, s2c_teamchangecaptain to all
void KSubWorld::team_change_captain(KNpc& e, EntityId target)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain()) return;
    if (e.player.team.auto_captain) {
        team_event(e.sid, pb::TEAM_EV_MSG, EntityId{}, 0x26);   // 0x080B9750: MSG_TEAM_ERROR03
        return;
    }
    KNpc* p = entities_.find(target);
    if (p == nullptr || p->kind != KNpcKind::player) return;
    const int slot = t->find_member(p->sid);
    if (slot < 0) return;
    if (p->camp == 0 && e.camp != 0) {
        team_event(e.sid, pb::TEAM_EV_MSG, p->id, 7);   // MSG_TEAM_CHANGE_CAPTAIN_FAIL1 + FAIL3
        return;
    }
    if (t->count > tables().lead_members(p->player.lead_level)) {
        team_event(e.sid, pb::TEAM_EV_MSG, p->id, 6);   // MSG_TEAM_CHANGE_CAPTAIN_FAIL1 + FAIL2
        return;
    }
    const bool was_open = t->state != 0;
    if (was_open) team_set_close(*t);
    t->members[static_cast<std::size_t>(slot)] = e.sid;
    t->captain = p->sid;
    e.player.team.figure = 1;
    p->player.team.figure = 0;
    p->player.team.auto_captain = false;   // 0x080B9579
    set_current_camp(*p, p->camp);   // RestoreCurrentCamp
    for (const std::uint64_t sid : t->members) {
        if (KNpc* m = team_player(sid)) set_current_camp(*m, p->camp);
    }
    team_sync_captain(*t);
    for (const std::uint64_t sid : t->people()) {
        pb::TeamEvent ev;
        ev.set_event(pb::TEAM_EV_CHANGE_CAPTAIN);
        ev.set_entity_id(p->id.value);
        ev.set_name(p->name);
        ev.set_level(p->level);
        ev.set_arg(sid == p->sid ? 1 : 0);
        emit({sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_EVENT), ev);
    }
    log::info("zone.team", "captain changed", {log::kv("team", e.player.team.id), log::kv("from", e.id), log::kv("to", p->id)});
    if (was_open) team_set_open(*t);
    team_send_self_all(*t);
}

// KPlayer::TeamDismiss 0x080B7DE0: a captain; enumMSG_ID_TEAM_DISMISS + s2c_teamleave to everyone, every KPlayerTeam cleared,
// every camp restored, the team released
void KSubWorld::team_dismiss(KNpc& e)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain()) return;
    const int id = e.player.team.id;
    for (const std::uint64_t sid : t->people()) {
        KNpc* m = team_player(sid);
        if (m == nullptr) continue;
        team_event(sid, pb::TEAM_EV_DISMISS);
        team_event(sid, pb::TEAM_EV_LEAVE, m->id);
        m->player.team.release();
        set_current_camp(*m, m->camp);
        team_send_self(*m);
    }
    t->release();
    log::info("zone.team", "team dismissed", {log::kv("team", id), log::kv("captain", e.id)});
}

// KPlayerTeam::InviteAdd 0x080CE150: a captain (one the system appointed -> the message 0x27); a closed team opens first
// (SetTeamOpen must succeed); the target around, not of camp 6, can_team and not Lua-disabled (else the message 0x13 to the
// captain); the target into the invite ring (m_nInviteList[m_nListPos++ % 7]); the 0x69 {0xc, captain npc, name} to it
void KSubWorld::team_invite(KNpc& e, EntityId target)
{
    KTeam* t = team_of(e);
    if (t == nullptr || !e.player.team.captain()) return;
    if (e.player.team.auto_captain) {
        team_event(e.sid, pb::TEAM_EV_MSG, EntityId{}, 0x27);   // 0x080CE270: MSG_TEAM_ERROR04
        return;
    }
    if (t->state == 0 && !team_set_open(*t)) return;   // 0x080CE1A7..0x080CE1BA
    KNpc* p = find_around_player(e, target);
    if (p == nullptr) return;
    if (p->camp == 6) return;   // 0x080CE20D
    if (!p->player.team.can_team || p->player.team.lua_disabled) {
        team_event(e.sid, pb::TEAM_EV_MSG, p->id, 0x13);   // 0x080CE225: MSG_TEAM_TARGET_CANNOT_ADD_TEAM
        return;
    }
    KPlayerTeam& my = e.player.team;
    my.invite_list[static_cast<std::size_t>(my.list_pos)] = p->sid;
    my.list_pos = (my.list_pos + 1) % kTeamMembers;
    team_event(p->sid, pb::TEAM_EV_INVITE, e.id);
    log::debug("zone.team", "team invite", {log::kv("captain", e.id), log::kv("entity", p->id)});
}

// KPlayerTeam::GetInviteReply 0x080CCBA0 on the captain's side: the reply comes from the invited one (the 0x53 sub 11 with the
// captain's npc): the captain must lead an open team and have invited the replier; a refusal tells the captain the name
// (enumMSG_ID_TEAM_REFUSE_INVITE); room needed (< 7 and under the limit); a replier in another team leaves it first
// (LeaveTeam); AddMember and the common join tail
void KSubWorld::team_reply_invite(KNpc& e, EntityId captain, bool ok)
{
    KNpc* c = entities_.find(captain);
    if (c == nullptr || c->kind != KNpcKind::player || !c->player.loaded) return;
    KTeam* t = team_of(*c);
    if (t == nullptr || !c->player.team.captain() || t->state == 0) return;
    if (!c->player.team.invited(e.sid)) return;
    if (!ok) {
        team_event(c->sid, pb::TEAM_EV_REFUSE, e.id);
        return;
    }
    if (t->count > 6 || t->count >= team_members_max(*t)) return;   // 0x080CCC37..0x080CCC4B
    if (e.player.team.flag) team_leave(e);                          // 0x080CCC6A
    t = team_of(*c);
    if (t == nullptr) return;
    if (!team_add_member(*t, e)) return;
    team_join(*t, c->player.team.id, e, *c);
}

// KPlayer::S2CSendTeamInfo 0x080B1AD0: one's own id -> the own info; else the npc among the players around, in a team whose
// captain is there and which is open -> s2c_teaminfo {captain, members}; anything else -> s2c_teamapplyinfofalse
void KSubWorld::team_info(KNpc& e, EntityId target)
{
    if (target == e.id) {
        team_send_self(e);
        return;
    }
    const KNpc* p = find_around_player(e, target);
    const KTeam* t = p != nullptr ? team_of(*p) : nullptr;
    if (t == nullptr || t->state == 0) {
        team_event(e.sid, pb::TEAM_EV_INFO_FALSE);
        return;
    }
    pb::TeamEvent ev;
    ev.set_event(pb::TEAM_EV_INFO);
    ev.set_entity_id(p->id.value);
    if (const KNpc* c = find_player(t->captain)) fill_member(ev.mutable_leader(), *c);
    for (const std::uint64_t sid : t->members) {
        if (sid == 0) continue;
        if (const KNpc* m = find_player(sid)) fill_member(ev.add_members(), *m);
    }
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TEAM_EVENT), ev);
}

// ---- the experience of a team -----------------------------------------------------------------------------

// KPlayer::AddExpTeam 0x080B03E0(anchor, exp, npc level, killer): the team mates of the anchor within 1024 units on its map
// (the captain first, then the members) are counted (n) and their levels summed; no team, none near, n above 8, a level sum
// under the anchor's level or n == 1 -> the plain AddExp of the anchor; else k = int(sqrt(n) x 100.0) and every one near gets
// exp x min(60, level_i x k / sum) / 100 (at least 1) - the killer gets exp x (100 + n) / 100 instead (at least 1)
void KSubWorld::add_exp_team(KNpc& anchor, int exp, int npc_level, EntityId killer)
{
    if (exp < 0) return;
    const KTeam* t = team_of(anchor);
    if (t == nullptr) {
        give_player_exp(anchor, exp, npc_level);
        return;
    }
    std::vector<KNpc*> near;
    std::int64_t sum = 0;
    for (const std::uint64_t sid : t->people()) {
        KNpc* m = team_player(sid);
        if (m == nullptr) continue;
        if (m != &anchor && dist2(anchor, *m) > kTeamNearDist2) continue;
        near.push_back(m);
        sum += m->level;
    }
    const int n = static_cast<int>(near.size());
    if (n <= 0 || sum <= 0 || n > 8 || sum < static_cast<std::int64_t>(anchor.level) || n == 1) {
        give_player_exp(anchor, exp, npc_level);   // 0x080B05E0 / 0x080B05F6
        return;
    }
    const int k = static_cast<int>(std::sqrt(static_cast<double>(n)) * 100.0);   // 0x080B063A..0x080B0667 (float 0x0825528C = 100.0)
    for (KNpc* m : near) {
        int share;
        if (m->id == killer) {
            share = static_cast<int>(static_cast<std::int64_t>(exp) * (100 + n) / 100);   // 0x080B0741
        } else {
            const std::int64_t pct = std::min<std::int64_t>(60, static_cast<std::int64_t>(m->level) * k / sum);   // 0x080B0700
            share = static_cast<int>(pct * exp / 100);
        }
        if (share <= 0) share = 1;
        give_player_exp(*m, share, npc_level);
    }
}

}  // namespace jx::zone
