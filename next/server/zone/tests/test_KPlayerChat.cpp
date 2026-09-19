// The chat channels of the JX2 server (docs/LINUX-SERVER.md §19): the checks of 0x081E3710 (ForbitTalk, the 0x95
// byte limit, the team of a 'T' line), the cost of 0x080502A0 over chatcost.ini, and who hears what: the watchers
// ('S'), the team ('T'), one player by name (the 2003 someone chat), the zone (WORLD / CITY / FACTION broadcasts).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerChat.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KSubWorld.h"

using jx::zone::KNpc;
using jx::zone::KChatCostRow;
using jx::zone::KChatCostTable;

namespace {

KChatCostTable real_costs()
{
    // \settings\npc\player\chatcost.ini of the reference server
    KChatCostTable t;
    t.rows[1] = KChatCostRow{0, 10, 0, 0};
    t.rows[2] = KChatCostRow{20, 0, 20, 0};
    t.rows[3] = KChatCostRow{0, 0, 10, 0};
    t.rows[4] = KChatCostRow{30, 0, 80, 0};
    return t;
}

jx::zone::KSubWorldConfig chat_world()
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
    c.player_set = std::make_shared<jx::zone::KPlayerSet>();
    c.chat_cost = std::make_shared<const KChatCostTable>(real_costs());
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y, int level)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
    r.set_level(static_cast<std::uint32_t>(level));
    auto* s = r.mutable_stats();
    s->set_strength(35);
    s->set_dexterity(25);
    s->set_vitality(25);
    s->set_energy(15);
    s->set_hp_max(204);
    s->set_hp(204);
    s->set_mp_max(100);
    s->set_mp(100);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(x);
    r.mutable_position()->mutable_pos()->set_y(y);
    return r;
}

std::vector<jx::pb::ChatMsg> lines(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::ChatMsg> out;
    for (const auto& p : all) {
        if (p.msg_id != jx::pb::G2C_CHAT_MSG || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
        jx::pb::ChatMsg m;
        REQUIRE(m.ParseFromString(p.payload));
        out.push_back(m);
    }
    return out;
}

// A (sid 7) and B (sid 8) next to each other, C (sid 9) far away on the same map
struct ChatWorld {
    jx::zone::KSubWorld w;
    jx::EntityId a, b, c;
    ChatWorld() : w(chat_world())
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000, 35), a, at) == jx::pb::RESULT_OK);
        REQUIRE(w.spawn_player(8, role_of(2, "B", 2040, 2000, 5), b, at) == jx::pb::RESULT_OK);
        REQUIRE(w.spawn_player(9, role_of(3, "C", 200, 200, 25), c, at) == jx::pb::RESULT_OK);
        w.tick();
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    KNpc& B() { return *w.mutable_entity(b); }
    KNpc& C() { return *w.mutable_entity(c); }
};

} // namespace

TEST_CASE("the cost type of a channel follows the relay's tables", "[chat]")
{
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_NEARBY) == 0);
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_TEAM) == 0);
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_CITY) == 2);
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_FACTION) == 3);
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_WORLD) == 4);
    CHECK(jx::zone::chat_cost_type(jx::pb::CH_WHISPER) == 0);
    CHECK(jx::zone::kChatSentenceMax == 0x95);
}

TEST_CASE("a nearby line reaches the watchers; ForbitTalk and the 0x95 byte limit drop it", "[chat][world]")
{
    ChatWorld cw;
    REQUIRE(cw.w.chat(7, "xin chao", jx::pb::CH_NEARBY));
    {
        auto all = cw.w.take_outbox();
        const auto a = lines(all, 7);
        const auto b = lines(all, 8);
        REQUIRE(a.size() == 1);
        REQUIRE(b.size() == 1);
        CHECK(a[0].channel() == jx::pb::CH_NEARBY);
        CHECK(a[0].name() == "A");
        CHECK(a[0].text() == "xin chao");
        CHECK(lines(all, 9).empty());   // C is out of sight
    }
    // 0x081E387A: ForbitTalk
    cw.A().player.forbid_talk = true;
    CHECK_FALSE(cw.w.chat(7, "im lang", jx::pb::CH_NEARBY));
    CHECK(cw.w.take_outbox().empty());
    cw.A().player.forbid_talk = false;
    // 0x081E38AC: at most 0x95 bytes
    CHECK(cw.w.chat(7, std::string(0x95, 'a'), jx::pb::CH_NEARBY));
    CHECK_FALSE(cw.w.chat(7, std::string(0x96, 'a'), jx::pb::CH_NEARBY));
    CHECK(lines(cw.w.take_outbox(), 7).size() == 1);
    // the server's own channel and the tong are not for a client
    CHECK_FALSE(cw.w.chat(7, "x", jx::pb::CH_SYSTEM));
    CHECK_FALSE(cw.w.chat(7, "x", jx::pb::CH_TONG));
}

TEST_CASE("a team line reaches the team wherever it stands, none without a team", "[chat][world]")
{
    ChatWorld cw;
    // 0x081E3A4B: no team -> dropped before anything is paid
    CHECK_FALSE(cw.w.chat(7, "doi oi", jx::pb::CH_TEAM));
    CHECK(cw.w.take_outbox().empty());
    // A leads; C comes over to join (AddTeamMember wants the applicant around: FindAroundPlayer), then walks far away
    REQUIRE(cw.w.team_request(7, jx::pb::TEAM_CREATE, jx::EntityId{}, 0));
    REQUIRE(cw.w.teleport(cw.c, jx::zone::Pos{2000, 2040}));
    cw.w.tick();
    REQUIRE(cw.w.team_request(9, jx::pb::TEAM_APPLY_ADD, cw.a, 0));
    REQUIRE(cw.w.team_request(7, jx::pb::TEAM_ACCEPT, cw.c, 0));
    REQUIRE(cw.w.teleport(cw.c, jx::zone::Pos{200, 200}));
    cw.w.tick();
    cw.w.tick();
    cw.w.take_outbox();
    REQUIRE(cw.w.chat(7, "doi oi", jx::pb::CH_TEAM));
    auto all = cw.w.take_outbox();
    CHECK(lines(all, 7).size() == 1);
    CHECK(lines(all, 9).size() == 1);
    CHECK(lines(all, 8).empty());   // B stands next to A but is not of the team
    CHECK(lines(all, 9)[0].channel() == jx::pb::CH_TEAM);
}

TEST_CASE("a whisper reaches the one named and the speaker; an unknown name is refused", "[chat][world]")
{
    ChatWorld cw;
    REQUIRE(cw.w.chat(7, "chi minh C nghe", jx::pb::CH_WHISPER, "C"));
    auto all = cw.w.take_outbox();
    CHECK(lines(all, 9).size() == 1);
    CHECK(lines(all, 7).size() == 1);
    CHECK(lines(all, 8).empty());
    CHECK(lines(all, 9)[0].channel() == jx::pb::CH_WHISPER);
    CHECK_FALSE(cw.w.chat(7, "ai do", jx::pb::CH_WHISPER, "Khong co"));
    CHECK(cw.w.take_outbox().empty());
}

TEST_CASE("0x080502A0: the cost of the city / faction / world channels", "[chat][world]")
{
    ChatWorld cw;
    KNpc& A = cw.A();
    KNpc& B = cw.B();
    // the world channel (type 4): level 30 and 80 % of the mana maximum
    CHECK_FALSE(cw.w.chat(8, "the gioi", jx::pb::CH_WORLD));   // B is level 5
    CHECK(cw.w.take_chat_broadcasts().empty());
    const int mana_max = A.mana_max();
    REQUIRE(mana_max > 0);
    A.cur.mana = mana_max * 80 / 100 - 1;
    CHECK_FALSE(cw.w.chat(7, "the gioi", jx::pb::CH_WORLD));   // not enough mana
    A.cur.mana = mana_max;
    REQUIRE(cw.w.chat(7, "the gioi", jx::pb::CH_WORLD));
    CHECK(A.cur.mana == mana_max - mana_max * 80 / 100);
    {
        auto bs = cw.w.take_chat_broadcasts();
        REQUIRE(bs.size() == 1);
        CHECK(bs[0].channel == jx::pb::CH_WORLD);
        CHECK(bs[0].faction == -1);
        jx::pb::ChatMsg m;
        REQUIRE(m.ParseFromString(bs[0].payload));
        CHECK(m.text() == "the gioi");
        CHECK(m.channel() == jx::pb::CH_WORLD);
    }
    CHECK(lines(cw.w.take_outbox(), 7).empty());   // nothing goes out of the map itself
    // the city channel (type 2): level 20 and 20 % of the mana maximum - C is level 25
    KNpc& C = cw.C();
    const int c_max = C.mana_max();
    C.cur.mana = c_max;
    REQUIRE(cw.w.chat(9, "thanh", jx::pb::CH_CITY));
    CHECK(C.cur.mana == c_max - c_max * 20 / 100);
    CHECK(cw.w.take_chat_broadcasts().size() == 1);
    // the faction channel (type 3): 10 % of the mana, the speaker's faction goes with the line
    A.cur.mana = mana_max;
    A.player.faction.current = 3;
    REQUIRE(cw.w.chat(7, "mon phai", jx::pb::CH_FACTION));
    CHECK(A.cur.mana == mana_max - mana_max * 10 / 100);
    {
        auto bs = cw.w.take_chat_broadcasts();
        REQUIRE(bs.size() == 1);
        CHECK(bs[0].channel == jx::pb::CH_FACTION);
        CHECK(bs[0].faction == 3);
    }
    // SetChatFlag (Player+0x394 bit 0): every channel refused, even the free ones
    B.player.chat_flag = true;
    CHECK_FALSE(cw.w.chat(8, "x", jx::pb::CH_NEARBY));
    B.player.chat_flag = false;
    CHECK(cw.w.chat(8, "x", jx::pb::CH_NEARBY));
    cw.w.take_outbox();
}
