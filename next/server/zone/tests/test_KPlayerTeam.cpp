// The teams of the JX2 server (docs/LINUX-SERVER.md §17): KPlayerTeam (Player+0x5994), KTeam (g_Team 0x8BB86E0), the
// 0x53 sub-commands, the leadership table (level_lead_exp.txt), the experience share of AddExpTeam 0x080B03E0 and the
// damage records keyed by the captain (0x0809BC70 / 0x0809BDD0).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KPlayerTeam.h"
#include "jx/zone/KSubWorld.h"

using jx::zone::KNpc;
using jx::zone::KNpcKind;
using jx::zone::KPlayerSet;
using jx::zone::KTeam;

namespace {

KPlayerSet team_tables()
{
    KPlayerSet t;
    jx::zone::KLevelAddRow kim;
    kim.life_per_level = 4;
    kim.stamina_male_per_level = 9;
    kim.stamina_female_per_level = 8;
    kim.mana_per_level = 1;
    kim.life_per_vitality = 8;
    kim.mana_per_energy = 1;
    kim.stamina_male_base = 180;
    kim.stamina_female_base = 180;
    t.set_level_add(0, kim);
    for (int level = 1; level <= 200; ++level) t.set_level_exp(level, 1000000);
    // level_lead_exp.txt: the members a captain of leadership level L may have (2 at level 1, 3 at 2, ... - the real
    // table climbs slower; the numbers here make the limit visible)
    for (int level = 1; level <= 100; ++level) t.set_lead_exp(level, 100 * level, 1 + level);
    return t;
}

jx::zone::KSubWorldConfig team_world()
{
    jx::zone::KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.view_cells = 1;
    c.interest_period = 1;
    c.view_slack = 0;
    c.far_period = 1;
    c.spawn_point = jx::zone::Pos{2000, 2000};
    c.seed = 7;
    c.player_set = std::make_shared<KPlayerSet>(team_tables());
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
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
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(x);
    r.mutable_position()->mutable_pos()->set_y(y);
    return r;
}

std::vector<jx::zone::Packet> of(const std::vector<jx::zone::Packet>& all, std::uint64_t sid, jx::pb::MsgId id)
{
    std::vector<jx::zone::Packet> out;
    for (const auto& p : all) {
        if (p.msg_id == id && std::find(p.sids.begin(), p.sids.end(), sid) != p.sids.end()) out.push_back(p);
    }
    return out;
}

// the team events of one session, in order
std::vector<jx::pb::TeamEvent> events(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::TeamEvent> out;
    for (const auto& p : of(all, sid, jx::pb::G2C_TEAM_EVENT)) {
        jx::pb::TeamEvent ev;
        REQUIRE(ev.ParseFromString(p.payload));
        out.push_back(ev);
    }
    return out;
}

bool has_event(const std::vector<jx::pb::TeamEvent>& evs, jx::pb::TeamEventKind kind, int arg = -1)
{
    for (const auto& e : evs) {
        if (e.event() == kind && (arg < 0 || static_cast<int>(e.arg()) == arg)) return true;
    }
    return false;
}

jx::pb::TeamSelf last_self(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    auto v = of(all, sid, jx::pb::G2C_TEAM_SELF);
    REQUIRE(!v.empty());
    jx::pb::TeamSelf m;
    REQUIRE(m.ParseFromString(v.back().payload));
    return m;
}

// three players next to each other: A (sid 7), B (sid 8), C (sid 9)
struct TeamWorld {
    jx::zone::KSubWorld w;
    jx::EntityId a, b, c;
    TeamWorld() : w(team_world())
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
        REQUIRE(w.spawn_player(8, role_of(2, "B", 2040, 2000), b, at) == jx::pb::RESULT_OK);
        REQUIRE(w.spawn_player(9, role_of(3, "C", 2000, 2040), c, at) == jx::pb::RESULT_OK);
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    KNpc& B() { return *w.mutable_entity(b); }
    KNpc& C() { return *w.mutable_entity(c); }
    // the blow of Lua KillPlayer: attacker hits target dead in one (ReceiveDamage -> DoDeath)
    void kill(KNpc& target, KNpc& attacker)
    {
        std::array<jx::zone::KMagicAttrib, jx::zone::kSkillAttribs> dmg{};
        dmg[0] = jx::zone::KMagicAttrib{jx::zone::magic_seriesdamage_p, {100, 0, 0}};
        dmg[1] = jx::zone::KMagicAttrib{jx::zone::magic_attackrating_v, {50000, 0, 0}};
        dmg[2] = jx::zone::KMagicAttrib{jx::zone::magic_ignoredefense_p, {1, 0, 0}};
        dmg[3] = jx::zone::KMagicAttrib{0, {200000000, 0, 200000000}};
        w.receive_damage(target, attacker, 0, false, dmg.data(), false, 1, 0x1f, 0);
    }
    // B applies to A and A takes it (0x53 sub 4 then sub 5)
    void join_by_apply(std::uint64_t applicant_sid, jx::EntityId applicant, std::uint64_t captain_sid, jx::EntityId captain)
    {
        REQUIRE(w.team_request(applicant_sid, jx::pb::TEAM_APPLY_ADD, captain, 0));
        REQUIRE(w.team_request(captain_sid, jx::pb::TEAM_ACCEPT, applicant, 0));
    }
};

} // namespace

TEST_CASE("KTeamSet::CreateTeam 0x080CC290 and the KTeam helpers", "[team]")
{
    jx::zone::KTeamSet set;
    const int id = set.create(7);
    REQUIRE(id == 0);
    KTeam* t = set.get(id);
    REQUIRE(t != nullptr);
    CHECK(t->captain == 7);
    CHECK(t->state == 0);
    CHECK(t->count == 0);
    CHECK(t->lead_limit);
    CHECK(t->find_free() == 0);
    CHECK(t->find_member(8) == -1);
    CHECK(t->check_in(7));
    CHECK_FALSE(t->check_in(8));
    t->members[0] = 8;
    t->count = 1;
    CHECK(t->find_member(8) == 0);
    CHECK(t->check_in(8));
    CHECK(t->people() == std::vector<std::uint64_t>{7, 8});
    CHECK(set.create(9) == 1);
    t->release();
    CHECK(t->empty());
    CHECK(set.create(10) == 0);   // the freed row is reused
    CHECK(set.get(-1) == nullptr);
    CHECK(set.get(5) == nullptr);
    jx::zone::KPlayerTeam pt;
    pt.invite_list[0] = 8;
    CHECK(pt.invited(8));
    CHECK_FALSE(pt.invited(9));
    pt.flag = true;
    pt.id = 0;
    CHECK(pt.captain());
    pt.figure = 1;
    CHECK_FALSE(pt.captain());
    pt.release();
    CHECK_FALSE(pt.flag);
    CHECK_FALSE(pt.invited(8));
}

TEST_CASE("the leadership table: lead_members 0x080C4560 outside 1..100 is 1", "[team]")
{
    const KPlayerSet t = team_tables();
    CHECK(t.lead_members(1) == 2);
    CHECK(t.lead_members(5) == 6);
    CHECK(t.lead_members(0) == 1);
    CHECK(t.lead_members(101) == 1);
    CHECK(t.lead_level_exp(3) == 300);
    CHECK(t.lead_level_exp(0) == 0);
}

TEST_CASE("CreateTeam 0x080CE3C0: the captain of a fresh open team; the refusals of camp 6, can_team, twice", "[team][world]")
{
    TeamWorld tw;
    // camp 6 cannot (0x080CE3F9) - silently
    tw.A().camp = 6;
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    CHECK_FALSE(tw.A().player.team.flag);
    tw.A().camp = 4;
    // can_team off -> the 0x69 {5, 4}
    tw.A().player.team.can_team = false;
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    CHECK(has_event(events(tw.w.take_outbox(), 7), jx::pb::TEAM_EV_CREATE_FAIL, 4));
    tw.A().player.team.can_team = true;
    // Lua DisabledTeam -> the same
    tw.A().player.team.lua_disabled = true;
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    CHECK(has_event(events(tw.w.take_outbox(), 7), jx::pb::TEAM_EV_CREATE_FAIL, 4));
    tw.A().player.team.lua_disabled = false;
    // created: captain, figure 0, the team open (SetTeamOpen at 0x080CE542), the own info sent
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    const KNpc& A = tw.A();
    CHECK(A.player.team.flag);
    CHECK(A.player.team.captain());
    CHECK(A.player.team.id == 0);
    CHECK_FALSE(A.player.team.auto_captain);
    CHECK(A.player.team.captain_npc == tw.a.value);
    const KTeam* t = tw.w.team_of(A);
    REQUIRE(t != nullptr);
    CHECK(t->state == 1);
    CHECK(t->captain == 7);
    CHECK(t->count == 0);
    CHECK(tw.w.team_members_max(*t) == 2);   // lead level 1
    {
        auto all = tw.w.take_outbox();
        auto evs = events(all, 7);
        CHECK(has_event(evs, jx::pb::TEAM_EV_CREATE_OK));
        CHECK(has_event(evs, jx::pb::TEAM_EV_OPEN_CLOSE, 1));
        const jx::pb::TeamSelf self = last_self(all, 7);
        CHECK(self.in_team());
        CHECK(self.captain());
        CHECK(self.state() == 1);
        CHECK(self.members_max() == 2);
        CHECK(self.leader().entity_id() == tw.a.value);
        CHECK(self.members_size() == 0);
    }
    // twice -> {5, 0}
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    CHECK(has_event(events(tw.w.take_outbox(), 7), jx::pb::TEAM_EV_CREATE_FAIL, 0));
}

TEST_CASE("apply and accept (0x080B8000 / 0x080B75B0): the applicant must have applied to this captain, the room of the leadership level, the camp sync", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.w.take_outbox();
    // taking someone who did not apply: nothing
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_ACCEPT, tw.b, 0));
    // a member cannot accept (the own info goes back instead, 0x080B75CF)
    CHECK_FALSE(tw.w.team_request(8, jx::pb::TEAM_ACCEPT, tw.c, 0));
    CHECK(!of(tw.w.take_outbox(), 8, jx::pb::G2C_TEAM_SELF).empty());
    // B applies: the captain hears the 0x69 {7, npc}
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_APPLY_ADD, tw.a, 0));
    CHECK(tw.B().player.team.apply_captain == tw.a.value);
    {
        auto evs = events(tw.w.take_outbox(), 7);
        REQUIRE(has_event(evs, jx::pb::TEAM_EV_APPLY));
        CHECK(evs.back().entity_id() == tw.b.value);
        CHECK(evs.back().name() == "B");
    }
    // accepted: member, figure 1, the same team id, the captain's camp as the current camp, both told
    tw.A().camp = 5;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_ACCEPT, tw.b, 0));
    CHECK(tw.B().player.team.flag);
    CHECK(tw.B().player.team.figure == 1);
    CHECK(tw.B().player.team.id == 0);
    CHECK(tw.B().player.team.captain_npc == tw.a.value);
    CHECK(tw.B().current_camp == 5);
    const KTeam* t = tw.w.team_of(tw.A());
    REQUIRE(t != nullptr);
    CHECK(t->count == 1);
    CHECK(t->find_member(8) == 0);
    {
        auto all = tw.w.take_outbox();
        CHECK(has_event(events(all, 7), jx::pb::TEAM_EV_ADD_MEMBER));
        CHECK(has_event(events(all, 8), jx::pb::TEAM_EV_SELF_ADD));
        const jx::pb::TeamSelf sb = last_self(all, 8);
        CHECK(sb.in_team());
        CHECK_FALSE(sb.captain());
        CHECK(sb.leader().entity_id() == tw.a.value);
        REQUIRE(sb.members_size() == 1);
        CHECK(sb.members(0).entity_id() == tw.b.value);
    }
    // the leadership limit: level 1 holds 2 members - C fits, and the team closes when it is full (0x080B7745)
    tw.join_by_apply(9, tw.c, 7, tw.a);
    CHECK(tw.w.team_of(tw.A())->count == 2);
    CHECK(tw.w.team_of(tw.A())->state == 0);
    CHECK(tw.w.team_full(*tw.w.team_of(tw.A())));
    tw.w.take_outbox();
    // a fourth cannot come: the team is closed and full (0x080B75CF: the own info only)
    jx::EntityId d;
    jx::zone::Pos at;
    REQUIRE(tw.w.spawn_player(10, role_of(4, "D", 2040, 2040), d, at) == jx::pb::RESULT_OK);
    tw.w.tick();
    REQUIRE(tw.w.team_request(10, jx::pb::TEAM_APPLY_ADD, tw.a, 0));
    CHECK_FALSE(tw.w.team_request(7, jx::pb::TEAM_ACCEPT, d, 0));
    CHECK_FALSE(tw.w.mutable_entity(d)->player.team.flag);
    // the leadership level grows: the limit follows (CalcCaptainPower 0x080CC960 reads it live)
    tw.A().player.lead_level = 3;   // 4 members
    CHECK_FALSE(tw.w.team_full(*tw.w.team_of(tw.A())));
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_OPEN_CLOSE, jx::EntityId{}, 1));
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_ACCEPT, d, 0));
    CHECK(tw.w.team_of(tw.A())->count == 3);
    // the camp rule of AddMember 0x080CCA18: a captain of camp begin (0) takes no one of another camp
    tw.w.team_request(7, jx::pb::TEAM_KICK, d, 0);
    tw.A().camp = 0;
    tw.w.mutable_entity(d)->camp = 4;
    tw.w.mutable_entity(d)->player.team.apply_captain = 0;
    tw.w.team_request(10, jx::pb::TEAM_APPLY_ADD, tw.a, 0);
    CHECK(tw.w.mutable_entity(d)->player.team.apply_captain == 0);
}

TEST_CASE("invite and reply (0x080CE150 / 0x080CCBA0): the invite ring, a refusal, joining from another team", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    REQUIRE(tw.w.team_request(9, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));   // C leads its own team (id 1)
    tw.w.take_outbox();
    // a member cannot invite; a captain invites B: B hears the 0x69 {0xc, captain}
    CHECK(tw.C().player.team.id == 1);
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_INVITE, tw.b, 0));
    CHECK(tw.A().player.team.invited(8));
    CHECK(tw.A().player.team.list_pos == 1);
    {
        auto evs = events(tw.w.take_outbox(), 8);
        REQUIRE(has_event(evs, jx::pb::TEAM_EV_INVITE));
        CHECK(evs.back().entity_id() == tw.a.value);
    }
    // a reply from someone never invited: nothing
    REQUIRE(tw.w.team_request(9, jx::pb::TEAM_REPLY_INVITE, tw.a, 1));
    CHECK(tw.w.team_of(tw.A())->count == 0);
    // B refuses: the captain hears the name (enumMSG_ID_TEAM_REFUSE_INVITE)
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_REPLY_INVITE, tw.a, 0));
    CHECK(has_event(events(tw.w.take_outbox(), 7), jx::pb::TEAM_EV_REFUSE));
    CHECK_FALSE(tw.B().player.team.flag);
    // B accepts: in
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_REPLY_INVITE, tw.a, 1));
    CHECK(tw.B().player.team.flag);
    CHECK(tw.B().player.team.id == 0);
    CHECK(tw.w.team_of(tw.A())->count == 1);
    // C, the captain of team 1, is invited and accepts: it leaves its own team first (0x080CCC6A) - team 1 is released
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_INVITE, tw.c, 0));
    REQUIRE(tw.w.team_request(9, jx::pb::TEAM_REPLY_INVITE, tw.a, 1));
    CHECK(tw.C().player.team.id == 0);
    CHECK(tw.C().player.team.figure == 1);
    CHECK(tw.w.teams().get(1)->empty());
    CHECK(tw.w.team_of(tw.A())->count == 2);
    // the target's can_team off: the captain is told (0x13) and nothing else happens
    jx::EntityId d;
    jx::zone::Pos at;
    REQUIRE(tw.w.spawn_player(10, role_of(4, "D", 2040, 2040), d, at) == jx::pb::RESULT_OK);
    tw.w.tick();
    tw.A().player.lead_level = 5;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_OPEN_CLOSE, jx::EntityId{}, 1));
    tw.w.mutable_entity(d)->player.team.can_team = false;
    tw.w.take_outbox();
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_INVITE, d, 0));
    CHECK(has_event(events(tw.w.take_outbox(), 7), jx::pb::TEAM_EV_MSG, 0x13));
    CHECK_FALSE(tw.A().player.team.invited(10));
    // the ring holds seven: the eighth invitation overwrites the first slot
    tw.w.mutable_entity(d)->player.team.can_team = true;
    for (int i = 0; i < 6; ++i) REQUIRE(tw.w.team_request(7, jx::pb::TEAM_INVITE, d, 0));
    CHECK(tw.A().player.team.list_pos == (2 + 6) % jx::zone::kTeamMembers);
}

TEST_CASE("leave, kick, change captain, dismiss (0x080B7C60 / 0x080B9880 / 0x080B9400 / 0x080B7DE0)", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.A().player.lead_level = 5;
    tw.join_by_apply(8, tw.b, 7, tw.a);
    tw.join_by_apply(9, tw.c, 7, tw.a);
    tw.w.take_outbox();
    // a member leaves: told to all, its camp restored, the slot freed
    tw.B().camp = 2;
    tw.B().current_camp = 4;
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_LEAVE, jx::EntityId{}, 0));
    CHECK_FALSE(tw.B().player.team.flag);
    CHECK(tw.B().current_camp == 2);
    CHECK(tw.w.team_of(tw.A())->count == 1);
    CHECK(tw.w.team_of(tw.A())->find_member(8) == -1);
    {
        auto all = tw.w.take_outbox();
        CHECK(has_event(events(all, 7), jx::pb::TEAM_EV_LEAVE));
        CHECK(has_event(events(all, 9), jx::pb::TEAM_EV_LEAVE));
        CHECK_FALSE(last_self(all, 8).in_team());
    }
    // a captain of camp begin (0 - what a fresh role carries) takes no one of another camp (0x080B80AB / 0x080CCA18)
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_APPLY_ADD, tw.a, 0));
    CHECK(tw.B().player.team.apply_captain == 0);
    tw.A().camp = 2;
    // kick: a member only, by the captain only
    tw.join_by_apply(8, tw.b, 7, tw.a);
    tw.w.take_outbox();
    tw.w.team_request(9, jx::pb::TEAM_KICK, tw.b, 0);
    CHECK(tw.B().player.team.flag);
    tw.w.team_request(7, jx::pb::TEAM_KICK, tw.b, 0);
    CHECK_FALSE(tw.B().player.team.flag);
    CHECK(tw.w.team_of(tw.A())->count == 1);
    {
        auto all = tw.w.take_outbox();
        CHECK(has_event(events(all, 8), jx::pb::TEAM_EV_KICK));
        CHECK(has_event(events(all, 9), jx::pb::TEAM_EV_KICK));
    }
    // change captain: C leads, A becomes a member; the figures, the current camps, the event
    tw.join_by_apply(8, tw.b, 7, tw.a);
    tw.w.take_outbox();
    tw.C().player.lead_level = 5;
    tw.C().camp = 3;
    tw.w.team_request(7, jx::pb::TEAM_CHANGE_CAPTAIN, tw.c, 0);
    const KTeam* t = tw.w.team_of(tw.A());
    REQUIRE(t != nullptr);
    CHECK(t->captain == 9);
    CHECK(t->find_member(7) >= 0);
    CHECK(tw.C().player.team.captain());
    CHECK(tw.A().player.team.figure == 1);
    CHECK(tw.A().current_camp == 3);
    CHECK(tw.B().current_camp == 3);
    CHECK(tw.A().player.team.captain_npc == tw.c.value);
    {
        auto all = tw.w.take_outbox();
        auto ev9 = events(all, 9);
        REQUIRE(has_event(ev9, jx::pb::TEAM_EV_CHANGE_CAPTAIN, 1));
        CHECK(has_event(events(all, 7), jx::pb::TEAM_EV_CHANGE_CAPTAIN, 0));
        CHECK(last_self(all, 9).captain());
        CHECK_FALSE(last_self(all, 7).captain());
    }
    // a new captain whose leadership cannot hold the members: the message 6 (FAIL1 + FAIL2)
    tw.B().player.lead_level = 1;   // 2 members, the team has 2: fine - so make it 3 members first
    jx::EntityId d;
    jx::zone::Pos at;
    REQUIRE(tw.w.spawn_player(10, role_of(4, "D", 2040, 2040), d, at) == jx::pb::RESULT_OK);
    tw.w.tick();
    tw.join_by_apply(10, d, 9, tw.c);
    CHECK(tw.w.team_of(tw.C())->count == 3);
    tw.w.take_outbox();
    tw.w.team_request(9, jx::pb::TEAM_CHANGE_CAPTAIN, tw.b, 0);
    CHECK(tw.w.team_of(tw.C())->captain == 9);
    CHECK(has_event(events(tw.w.take_outbox(), 9), jx::pb::TEAM_EV_MSG, 6));
    // dismiss: everyone out, the camps restored, the row free
    tw.w.team_request(7, jx::pb::TEAM_DISMISS, jx::EntityId{}, 0);   // a member cannot
    CHECK(tw.C().player.team.flag);
    tw.w.take_outbox();
    tw.w.team_request(9, jx::pb::TEAM_DISMISS, jx::EntityId{}, 0);
    CHECK_FALSE(tw.A().player.team.flag);
    CHECK_FALSE(tw.B().player.team.flag);
    CHECK_FALSE(tw.C().player.team.flag);
    CHECK(tw.A().current_camp == tw.A().camp);
    CHECK(tw.w.teams().get(0)->empty());
    {
        auto all = tw.w.take_outbox();
        for (const std::uint64_t sid : {7u, 8u, 9u, 10u}) {
            CHECK(has_event(events(all, sid), jx::pb::TEAM_EV_DISMISS));
            CHECK_FALSE(last_self(all, sid).in_team());
        }
    }
}

TEST_CASE("the captain's death hands the lead over (0x080CD480): the appointed captain is limited until it creates a team again", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.A().player.lead_level = 5;
    tw.join_by_apply(8, tw.b, 7, tw.a);
    tw.join_by_apply(9, tw.c, 7, tw.a);
    tw.w.take_outbox();
    tw.kill(tw.A(), tw.A());
    REQUIRE(tw.A().doing == jx::zone::KDoing::death);
    const KTeam* t = tw.w.team_of(tw.B());
    REQUIRE(t != nullptr);
    CHECK(t->captain == 8);   // the first member of the captain's camp
    CHECK(t->state == 0);     // closed first
    CHECK(tw.B().player.team.captain());
    CHECK(tw.B().player.team.auto_captain);
    CHECK(tw.A().player.team.figure == 1);
    CHECK(tw.C().player.team.captain_npc == tw.b.value);
    {
        auto all = tw.w.take_outbox();
        CHECK(has_event(events(all, 8), jx::pb::TEAM_EV_CHANGE_CAPTAIN, 1));
        CHECK(has_event(events(all, 9), jx::pb::TEAM_EV_CHANGE_CAPTAIN, 0));
    }
    // the appointed captain: no opening (0x28), no taking (0x24), no kicking (0x25), no appointing (0x26), no inviting (0x27)
    CHECK_FALSE(tw.w.team_request(8, jx::pb::TEAM_OPEN_CLOSE, jx::EntityId{}, 1));
    CHECK(has_event(events(tw.w.take_outbox(), 8), jx::pb::TEAM_EV_MSG, 0x28));
    tw.w.team_request(8, jx::pb::TEAM_KICK, tw.c, 0);
    CHECK(tw.C().player.team.flag);
    CHECK(has_event(events(tw.w.take_outbox(), 8), jx::pb::TEAM_EV_MSG, 0x25));
    tw.w.team_request(8, jx::pb::TEAM_CHANGE_CAPTAIN, tw.c, 0);
    CHECK(tw.w.team_of(tw.B())->captain == 8);
    CHECK(has_event(events(tw.w.take_outbox(), 8), jx::pb::TEAM_EV_MSG, 0x26));
    tw.w.team_request(8, jx::pb::TEAM_INVITE, tw.c, 0);
    CHECK(has_event(events(tw.w.take_outbox(), 8), jx::pb::TEAM_EV_MSG, 0x27));
    // it may still dismiss
    tw.w.team_request(8, jx::pb::TEAM_DISMISS, jx::EntityId{}, 0);
    CHECK_FALSE(tw.B().player.team.flag);
    CHECK_FALSE(tw.A().player.team.flag);
    // a fresh team of its own clears the mark
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    CHECK_FALSE(tw.B().player.team.auto_captain);
    tw.A().doing = jx::zone::KDoing::stand;   // (the test moves on without a revive)
    // a captain leaving with the leadership limit on dismisses the team (DeleteMember 0x080CD5E0); with it off the lead is
    // handed over instead (0x080CD730)
    tw.join_by_apply(9, tw.c, 8, tw.b);
    tw.w.mutable_team(tw.B().player.team.id)->lead_limit = false;
    REQUIRE(tw.w.team_request(8, jx::pb::TEAM_LEAVE, jx::EntityId{}, 0));
    CHECK(tw.C().player.team.captain());
    CHECK(tw.C().player.team.auto_captain);
    CHECK_FALSE(tw.B().player.team.flag);
    CHECK(tw.w.team_of(tw.C())->count == 0);
    // leaving the world leaves the team (0x080C55D7)
    tw.w.team_request(9, jx::pb::TEAM_DISMISS, jx::EntityId{}, 0);
    REQUIRE(tw.w.team_request(9, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));   // (a fresh team: the appointed one cannot open)
    tw.join_by_apply(8, tw.b, 9, tw.c);
    CHECK(tw.w.team_of(tw.C())->count == 1);
    REQUIRE(tw.w.remove_player(8));
    CHECK(tw.w.team_of(tw.C())->count == 0);
}

TEST_CASE("AddExpTeam 0x080B03E0: the killer's exp x (100 + n) / 100, the mates' exp x min(60, level x sqrt(n) x 100 / sum) / 100; alone or apart the plain AddExp", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.A().player.lead_level = 5;
    tw.join_by_apply(8, tw.b, 7, tw.a);
    tw.join_by_apply(9, tw.c, 7, tw.a);
    tw.A().level = 10;
    tw.B().level = 10;
    tw.C().level = 20;
    tw.A().player.exp = 0;
    tw.B().player.exp = 0;
    tw.C().player.exp = 0;
    // n = 3, sum = 40, k = int(sqrt(3) x 100) = 173: the killer A gets 1000 x 103 / 100 = 1030; B gets min(60, 10 x 173 / 40 = 43) % = 430;
    // C gets min(60, 20 x 173 / 40 = 86) = 60 % = 600 (AddExp keeps the whole of it: the npc level equals theirs, within 5)
    tw.w.add_exp_team(tw.A(), 1000, 15, tw.a);
    CHECK(tw.A().player.exp == 1030);
    CHECK(tw.B().player.exp == 430);
    CHECK(tw.C().player.exp == 600);
    // a mate far away (beyond 1024 units) is not counted: n = 2, sum = 20, k = 141: A 1020, B min(60, 10 x 141 / 20 = 70) -> 60 % = 600
    tw.C().set_pos(jx::zone::Pos{3500, 3500});
    tw.A().player.exp = tw.B().player.exp = tw.C().player.exp = 0;
    tw.w.add_exp_team(tw.A(), 1000, 15, tw.a);
    CHECK(tw.A().player.exp == 1020);
    CHECK(tw.B().player.exp == 600);
    CHECK(tw.C().player.exp == 0);
    // nobody near: the plain AddExp of the anchor (n == 1)
    tw.B().set_pos(jx::zone::Pos{3400, 3400});
    tw.A().player.exp = tw.B().player.exp = 0;
    tw.w.add_exp_team(tw.A(), 1000, 15, tw.a);
    CHECK(tw.A().player.exp == 1000);
    CHECK(tw.B().player.exp == 0);
    // out of a team: the plain AddExp
    tw.A().player.exp = 0;
    tw.w.team_request(7, jx::pb::TEAM_DISMISS, jx::EntityId{}, 0);
    tw.w.add_exp_team(tw.A(), 500, 15, tw.a);
    CHECK(tw.A().player.exp == 500);
}

TEST_CASE("the damage records of a team are its captain's (0x0809BC70) and a kill pays the team (0x0809BDD0); team_near_count 0x080CC620", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.A().player.lead_level = 5;
    tw.join_by_apply(8, tw.b, 7, tw.a);
    CHECK(tw.w.team_near_count(tw.A()) == 1);
    CHECK(tw.w.team_near_count(tw.B()) == 1);
    CHECK(tw.w.team_near_count(tw.C()) == 0);
    CHECK(KNpc::damage_record_key(tw.B()) == tw.a);
    CHECK(KNpc::damage_record_key(tw.A()) == tw.a);
    CHECK(KNpc::damage_record_key(tw.C()) == tw.c);
    // a pig B kills alone: the record is A's (the captain), the anchor A (near) and AddExpTeam pays both - B as the killer
    const jx::EntityId pig = tw.w.spawn_npc("pig", jx::zone::Pos{2040, 2040}, 418, 0, KNpcKind::monster);
    KNpc* e = tw.w.mutable_entity(pig);
    REQUIRE(e != nullptr);
    e->cur.experience = 1000;
    e->level = 1;
    tw.A().player.exp = tw.B().player.exp = 0;
    tw.kill(*e, tw.B());   // the record of the blow is A's (the captain), whole: min(damage, life) = the life
    REQUIRE(e->doing == jx::zone::KDoing::death);
    // n = 2, sum = 2, k = 141: the killer B gets 1000 x life / life x 102 / 100 = 1020, A min(60, 1 x 141 / 2 = 70) -> 60 % = 600
    CHECK(tw.B().player.exp == 1020);
    CHECK(tw.A().player.exp == 600);
    // the captain far away: the nearest member is the anchor; a record of a team nobody of which is near pays nothing
    tw.A().set_pos(jx::zone::Pos{3500, 3500});
    const jx::EntityId pig2 = tw.w.spawn_npc("pig", jx::zone::Pos{2040, 2040}, 418, 0, KNpcKind::monster);
    KNpc* e2 = tw.w.mutable_entity(pig2);
    REQUIRE(e2 != nullptr);
    e2->cur.experience = 1000;
    e2->level = 1;
    tw.A().player.exp = tw.B().player.exp = 0;
    tw.kill(*e2, tw.B());
    REQUIRE(e2->doing == jx::zone::KDoing::death);
    CHECK(tw.B().player.exp == 1000);   // the anchor B alone near: the plain AddExp
    CHECK(tw.A().player.exp == 0);
}

TEST_CASE("a team mate may pick up a drop kept for the captain, but not a task item (ServerPickUpItem 0x080B826C)", "[team][world]")
{
    TeamWorld tw;
    REQUIRE(tw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    tw.join_by_apply(8, tw.b, 7, tw.a);
    KNpc obj;
    obj.kind = KNpcKind::drop;
    obj.object.kind = jx::zone::KObjKind::money;
    obj.object.belong = 1;   // A's player id
    CHECK(tw.w.team_may_take(tw.B(), obj));
    CHECK_FALSE(tw.w.team_may_take(tw.C(), obj));
    obj.object.belong = 3;   // C is in no team
    CHECK_FALSE(tw.w.team_may_take(tw.B(), obj));
}
