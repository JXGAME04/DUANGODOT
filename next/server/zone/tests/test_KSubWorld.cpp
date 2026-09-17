#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KSubWorld.h"

using jx::EntityId;
using jx::zone::Packet;
using jx::zone::Pos;
using jx::zone::KSubWorld;
using jx::zone::KSubWorldConfig;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
    }
};

KSubWorldConfig small_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 20;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    // These tests are about the mechanics around a cell boundary, not about how wide the view is,
    // so they pin the old one-cell square.  The default view is a rectangle derived from the
    // screen (see "the default view covers a whole client screen").
    c.view_cells = 1;
    // ... and about WHAT a client is told, not about when: every client looks around every tick and
    // gives an entity up the moment it leaves the view (no slack), so one tick settles everything.
    c.interest_period = 1;
    c.view_slack = 0;
    // ... and every move is told the moment it happens: the two rates of N3 have their own tests
    // (test_KInterest.cpp), here they would only add a wait of up to far_period ticks
    c.far_period = 1;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;   // 10 units per tick
    return c;
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at, std::uint32_t zone = 1)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_position()->set_zone_id(zone);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

template <class Msg>
Msg decode(const Packet& p)
{
    Msg m;
    REQUIRE(m.ParseFromString(p.payload));
    return m;
}

// packets of one message id addressed to sid
std::vector<Packet> to(const std::vector<Packet>& all, std::uint64_t sid, jx::pb::MsgId id)
{
    std::vector<Packet> out;
    for (const Packet& p : all) {
        if (p.msg_id == id && std::find(p.sids.begin(), p.sids.end(), sid) != p.sids.end()) out.push_back(p);
    }
    return out;
}

} // namespace

TEST_CASE("spawn sends the visible set to the newcomer and the newcomer to viewers", "[world]")
{
    Quiet q;
    KSubWorld w(small_world());
    EntityId ea, eb;
    Pos pa, pb;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{100, 100}), ea, pa) == jx::pb::RESULT_OK);
    CHECK(pa == Pos{100, 100});
    auto out = w.take_outbox();
    REQUIRE(out.size() == 1);
    auto spawn = decode<jx::pb::EntitySpawn>(out[0]);
    REQUIRE(spawn.entities_size() == 1);
    CHECK(spawn.entities(0).entity_id() == ea.value);
    CHECK(spawn.entities(0).name() == "A");
    CHECK(spawn.entities(0).entity_type() == jx::pb::ENTITY_PLAYER);
    CHECK(spawn.entities(0).move_speed() == 200);

    REQUIRE(w.spawn_player(2, role(22, "B", Pos{200, 200}), eb, pb) == jx::pb::RESULT_OK);
    out = w.take_outbox();
    // the newcomer is told at once; whoever was there notices it at their next look around
    const auto to_b = to(out, 2, jx::pb::G2C_ENTITY_SPAWN);
    CHECK(to(out, 1, jx::pb::G2C_ENTITY_SPAWN).empty());
    w.tick();
    out = w.take_outbox();
    const auto to_a = to(out, 1, jx::pb::G2C_ENTITY_SPAWN);
    REQUIRE(to_b.size() == 1);
    REQUIRE(to_a.size() == 1);
    CHECK(decode<jx::pb::EntitySpawn>(to_b[0]).entities_size() == 2);
    const auto a_sees = decode<jx::pb::EntitySpawn>(to_a[0]);
    REQUIRE(a_sees.entities_size() == 1);
    CHECK(a_sees.entities(0).entity_id() == eb.value);

    CHECK(w.player_count() == 2);
    CHECK(w.spawn_player(2, role(22, "B", Pos{200, 200}), eb, pb) == jx::pb::RESULT_WRONG_STATE);
}

TEST_CASE("players far apart do not see each other and other zones fall back to the spawn point", "[world]")
{
    Quiet q;
    KSubWorld w(small_world());
    EntityId ea, eb;
    Pos pa, pb;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{100, 100}), ea, pa) == jx::pb::RESULT_OK);
    w.take_outbox();
    REQUIRE(w.spawn_player(2, role(22, "B", Pos{3000, 3000}), eb, pb) == jx::pb::RESULT_OK);
    auto out = w.take_outbox();
    CHECK(to(out, 1, jx::pb::G2C_ENTITY_SPAWN).empty());
    REQUIRE(to(out, 2, jx::pb::G2C_ENTITY_SPAWN).size() == 1);
    CHECK(decode<jx::pb::EntitySpawn>(to(out, 2, jx::pb::G2C_ENTITY_SPAWN)[0]).entities_size() == 1);

    EntityId ec;
    Pos pc;
    REQUIRE(w.spawn_player(3, role(33, "C", Pos{5, 5}, /*zone*/ 9), ec, pc) == jx::pb::RESULT_OK);
    CHECK(pc == Pos{2000, 2000});
    REQUIRE(w.spawn_player(4, role(44, "D", Pos{-50, 99999}), ec, pc) == jx::pb::RESULT_OK);
    CHECK(pc == Pos{0, 4095});   // clamped into the map
}

TEST_CASE("movement is deterministic, integer exact and arrives on the target", "[world]")
{
    Quiet q;
    KSubWorld w(small_world());
    EntityId ea;
    Pos pa;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{0, 0}), ea, pa) == jx::pb::RESULT_OK);
    w.take_outbox();

    REQUIRE(w.move_request(1, Pos{100, 0}, 7));
    auto out = w.take_outbox();
    REQUIRE(out.size() == 1);   // only A is around
    auto mv = decode<jx::pb::EntityMove>(out[0]);
    CHECK(mv.seq() == 7);
    CHECK(mv.target().x() == 100);
    CHECK(mv.pos().x() == 0);

    for (int i = 1; i <= 9; ++i) {
        w.tick();
        CHECK(w.find_player(1)->pos() == Pos{10 * i, 0});
        CHECK(w.take_outbox().empty());   // no cell change, still moving
    }
    w.tick();
    CHECK(w.find_player(1)->pos() == Pos{100, 0});
    CHECK_FALSE(w.find_player(1)->moving);
    out = w.take_outbox();
    REQUIRE(out.size() == 1);
    mv = decode<jx::pb::EntityMove>(out[0]);
    CHECK(mv.pos().x() == 100);
    CHECK(mv.target().x() == 100);
    CHECK(mv.tick() == 10);

    // 3-4-5 triangle: 500 units at 10 per tick = 50 ticks, straight line through (150,200)
    REQUIRE(w.move_request(1, Pos{400, 400}, 8));
    w.take_outbox();
    for (int i = 0; i < 25; ++i) w.tick();
    CHECK(w.find_player(1)->pos() == Pos{250, 200});
    for (int i = 0; i < 25; ++i) w.tick();
    CHECK(w.find_player(1)->pos() == Pos{400, 400});
    CHECK_FALSE(w.find_player(1)->moving);

    // same inputs on a fresh world give the same trajectory
    KSubWorld w2(small_world());
    REQUIRE(w2.spawn_player(1, role(11, "A", Pos{0, 0}), ea, pa) == jx::pb::RESULT_OK);
    REQUIRE(w2.move_request(1, Pos{100, 0}, 7));
    for (int i = 0; i < 10; ++i) w2.tick();
    REQUIRE(w2.move_request(1, Pos{400, 400}, 8));
    for (int i = 0; i < 50; ++i) w2.tick();
    CHECK(w2.find_player(1)->fx == w.find_player(1)->fx);
    CHECK(w2.find_player(1)->fy == w.find_player(1)->fy);
    CHECK(w2.tick_count() == w.tick_count());
}

TEST_CASE("crossing a cell boundary spawns and despawns on both sides", "[world]")
{
    Quiet q;
    KSubWorld w(small_world());   // cell 512, view 1
    EntityId ea, eb;
    Pos pa, pb;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{500, 100}), ea, pa) == jx::pb::RESULT_OK);     // cell 0
    w.take_outbox();
    REQUIRE(w.spawn_player(2, role(22, "B", Pos{1100, 100}), eb, pb) == jx::pb::RESULT_OK);    // cell 2: invisible to A
    auto out = w.take_outbox();
    CHECK(to(out, 1, jx::pb::G2C_ENTITY_SPAWN).empty());

    REQUIRE(w.move_request(1, Pos{600, 100}, 1));   // into cell 1 after 10 ticks
    w.take_outbox();
    for (int i = 0; i < 2; ++i) w.tick();           // x = 520 -> crossed the boundary
    out = w.take_outbox();
    const auto a_sees = to(out, 1, jx::pb::G2C_ENTITY_SPAWN);
    const auto b_sees = to(out, 2, jx::pb::G2C_ENTITY_SPAWN);
    REQUIRE(a_sees.size() == 1);
    REQUIRE(b_sees.size() == 1);
    CHECK(decode<jx::pb::EntitySpawn>(a_sees[0]).entities(0).entity_id() == eb.value);
    CHECK(decode<jx::pb::EntitySpawn>(b_sees[0]).entities(0).entity_id() == ea.value);
    CHECK(decode<jx::pb::EntitySpawn>(b_sees[0]).entities(0).target().x() == 600);

    for (int i = 0; i < 8; ++i) w.tick();           // arrive at 600
    out = w.take_outbox();
    CHECK(to(out, 2, jx::pb::G2C_ENTITY_MOVE).size() == 1);   // B is told A stopped

    REQUIRE(w.move_request(1, Pos{400, 100}, 2));   // back to cell 0
    w.take_outbox();
    for (int i = 0; i < 10; ++i) w.tick();
    out = w.take_outbox();
    const auto a_lost = to(out, 1, jx::pb::G2C_ENTITY_DESPAWN);
    const auto b_lost = to(out, 2, jx::pb::G2C_ENTITY_DESPAWN);
    REQUIRE(a_lost.size() == 1);
    REQUIRE(b_lost.size() == 1);
    CHECK(decode<jx::pb::EntityDespawn>(a_lost[0]).entity_ids(0) == eb.value);
    CHECK(decode<jx::pb::EntityDespawn>(b_lost[0]).entity_ids(0) == ea.value);
}

TEST_CASE("remove, chat and role snapshot", "[world]")
{
    Quiet q;
    KSubWorld w(small_world());
    EntityId ea, eb;
    Pos pa, pb;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{100, 100}), ea, pa) == jx::pb::RESULT_OK);
    REQUIRE(w.spawn_player(2, role(22, "B", Pos{200, 200}), eb, pb) == jx::pb::RESULT_OK);
    w.tick();   // A notices B
    w.take_outbox();

    REQUIRE(w.chat(1, "hello"));
    auto out = w.take_outbox();
    REQUIRE(out.size() == 1);
    CHECK(out[0].sids.size() == 2);
    const auto chat = decode<jx::pb::ChatMsg>(out[0]);
    CHECK(chat.name() == "A");
    CHECK(chat.text() == "hello");
    CHECK_FALSE(w.chat(99, "nobody"));

    REQUIRE(w.move_request(1, Pos{150, 100}, 3));
    for (int i = 0; i < 5; ++i) w.tick();
    jx::pb::RoleData snap;
    REQUIRE(w.role_snapshot(1, snap));
    CHECK(snap.player_id() == 11);
    CHECK(snap.position().zone_id() == 1);
    CHECK(snap.position().pos().x() == 150);
    CHECK(snap.position().pos().y() == 100);
    CHECK(snap.name() == "A");

    w.take_outbox();
    REQUIRE(w.remove_player(1));
    out = w.take_outbox();
    const auto b_lost = to(out, 2, jx::pb::G2C_ENTITY_DESPAWN);
    REQUIRE(b_lost.size() == 1);
    CHECK(decode<jx::pb::EntityDespawn>(b_lost[0]).entity_ids(0) == ea.value);
    CHECK(w.player_count() == 1);
    CHECK(w.entity_count() == 1);
    CHECK_FALSE(w.remove_player(1));
    CHECK_FALSE(w.role_snapshot(1, snap));
    CHECK(w.session_ids() == std::vector<std::uint64_t>{2});
}

TEST_CASE("melee attack: swing, hit at 60 percent, death animation, corpse gone, revive", "[world][combat]")
{
    Quiet q;
    KSubWorldConfig cfg = small_world();
    cfg.tick_hz = 18;
    KSubWorld w(cfg);
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    const EntityId pig = w.spawn_npc("pig", Pos{2050, 2000}, 418, 0, jx::zone::KNpcKind::monster);
    w.take_outbox();
    auto to_hero = [](const std::vector<Packet>& out, jx::pb::MsgId id) {
        std::vector<Packet> r;
        for (const Packet& p : out) {
            if (p.msg_id == id && std::find(p.sids.begin(), p.sids.end(), 7) != p.sids.end()) r.push_back(p);
        }
        return r;
    };

    REQUIRE(w.attack_request(7, pig, 1));
    auto acts = to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_ACTION);
    REQUIRE(acts.size() == 1);
    const auto swing = decode<jx::pb::EntityAction>(acts[0]);
    CHECK(swing.entity_id() == hero.value);
    CHECK(swing.action() == jx::pb::ACTION_ATTACK);
    CHECK(swing.target() == pig.value);
    CHECK(swing.frames() == 18);   // BaseValue AttackFrame 18, attack speed 0
    CHECK(swing.dir() == 47);      // the pig is to the right (g_GetDirIndex)

    // nothing lands before frame 60% (18 * 60 / 100 = 10)
    for (int i = 0; i < 9; ++i) w.tick();
    CHECK(to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_LIFE).empty());
    w.tick();
    auto lifes = to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_LIFE);
    REQUIRE(lifes.size() == 1);
    const auto l = decode<jx::pb::EntityLife>(lifes[0]);
    CHECK(l.entity_id() == pig.value);
    CHECK(l.delta() < 0);
    CHECK(l.life() < l.life_max());
    CHECK(l.life_max() == 30);   // no template table: the test monster's default life

    // the zone keeps swinging until the pig dies
    bool died = false;
    for (int guard = 0; guard < 2000 && !died; ++guard) {
        w.tick();
        for (const Packet& p : to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_ACTION)) {
            if (decode<jx::pb::EntityAction>(p).action() == jx::pb::ACTION_DEATH) died = true;
        }
    }
    REQUIRE(died);
    const jx::zone::KNpc* corpse = w.find_entity(pig);
    REQUIRE(corpse != nullptr);
    CHECK(corpse->life == 0);
    CHECK(corpse->doing == jx::zone::KDoing::death);

    // after DeathFrame (15) ticks the corpse leaves sight; after ReviveFrame (2400) it is back with full life
    bool gone = false;
    for (int i = 0; i < 15 && !gone; ++i) {
        w.tick();
        gone = !to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_DESPAWN).empty();
    }
    CHECK(gone);
    CHECK(w.find_entity(hero)->attack_target.value == 0);   // the swing ended, the dead target was dropped
    CHECK(w.find_entity(pig)->doing == jx::zone::KDoing::revive);
    bool back = false;
    for (int i = 0; i < 2400 && !back; ++i) {
        w.tick();
        for (const Packet& p : to_hero(w.take_outbox(), jx::pb::G2C_ENTITY_SPAWN)) {
            const auto sp = decode<jx::pb::EntitySpawn>(p);
            for (const auto& e : sp.entities()) {
                if (e.entity_id() == pig.value) {
                    back = true;
                    CHECK(e.life() == e.life_max());
                    CHECK(e.doing() == jx::pb::ACTION_STAND);
                }
            }
        }
    }
    CHECK(back);
    // walking cancels an attack; townsfolk cannot be attacked
    REQUIRE(w.attack_request(7, pig, 2));
    REQUIRE(w.move_request(7, Pos{2100, 2100}, 3));
    CHECK(w.find_entity(hero)->attack_target.value == 0);
    CHECK(w.find_entity(hero)->doing == jx::zone::KDoing::stand);
    const EntityId smith = w.spawn_npc("smith", Pos{2010, 2000}, 199, 0, jx::zone::KNpcKind::npc);
    CHECK_FALSE(w.attack_request(7, smith, 4));
}

TEST_CASE("wandering npcs move and are announced to viewers", "[world]")
{
    Quiet q;
    auto cfg = small_world();
    cfg.seed = 42;
    KSubWorld w(cfg);
    EntityId ea;
    Pos pa;
    REQUIRE(w.spawn_player(1, role(11, "A", Pos{1000, 1000}), ea, pa) == jx::pb::RESULT_OK);
    w.take_outbox();
    const EntityId npc = w.spawn_npc("npc1", Pos{1010, 1010}, 1000, 100);
    CHECK(w.take_outbox().empty());   // spawning sends nothing by itself ...
    w.tick();                         // ... the client finds the npc when it looks around
    auto out = w.take_outbox();
    REQUIRE(to(out, 1, jx::pb::G2C_ENTITY_SPAWN).size() == 1);
    CHECK(decode<jx::pb::EntitySpawn>(out[0]).entities(0).entity_type() == jx::pb::ENTITY_NPC);

    std::size_t moves = 0;
    for (int i = 0; i < 400; ++i) {
        w.tick();
        moves += to(w.take_outbox(), 1, jx::pb::G2C_ENTITY_MOVE).size();
    }
    CHECK(moves > 0);
    const Pos p = w.find_entity(npc)->pos();
    CHECK(std::abs(p.x - 1010) <= 100);
    CHECK(std::abs(p.y - 1010) <= 100);
}

// MASTER SPEC 44 / 45: a npc nobody can see does not think.  This is what lets a map hold
// thousands of creatures without paying for all of them every tick.
TEST_CASE("npcs far from every player sleep, and wake up when one comes near", "[world][sleep]")
{
    Quiet q;
    KSubWorld w(small_world());
    // one crowd near the spawn point, one far away in the corner
    std::vector<EntityId> near_ids, far_ids;
    for (int i = 0; i < 20; ++i) {
        near_ids.push_back(w.spawn_npc("near" + std::to_string(i), Pos{2000 + i * 8, 2000}, 0, 100, jx::zone::KNpcKind::monster));
        far_ids.push_back(w.spawn_npc("far" + std::to_string(i), Pos{100 + i * 8, 3900}, 0, 100, jx::zone::KNpcKind::monster));
    }
    w.tick();
    CHECK(w.awake_entities() == 0);   // no player in the map at all: nobody thinks

    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(1, role(11, "Hero", Pos{2000, 2000}), hero, at) == jx::pb::RESULT_OK);
    w.tick();
    const std::size_t awake_near = w.awake_entities();
    CHECK(awake_near >= 21);                       // the player and the crowd around it
    CHECK(awake_near < 41);                        // but not the far crowd
    CHECK(w.entity_count() == 41);

    // walking over to the far corner wakes that crowd and lets the first one fall asleep
    REQUIRE(w.teleport(hero, Pos{150, 3900}));
    w.tick();
    w.tick();
    const std::size_t awake_far = w.awake_entities();
    CHECK(awake_far >= 2);
    CHECK(awake_far < 41);
}

TEST_CASE("the default view covers a whole client screen", "[world][aoi]")
{
    // The renderer draws a scene point at (x, y/2), so a screen W x H pixels shows W units of x
    // and 2H units of y.  Anything the player can see must have been sent: a view smaller than the
    // screen means an entity appears out of nothing at the edge.
    Quiet q;
    KSubWorldConfig c;
    c.width = 20000;
    c.height = 20000;
    c.spawn_point = Pos{10000, 10000};
    KSubWorld w(c);

    struct Screen {
        const char* name;
        std::int32_t half_x;
        std::int32_t half_y;
    };
    // VLTK 2.0 runs 1024x768; this project's Godot client runs 1280x720.
    const Screen screens[] = {{"VLTK 2.0 1024x768", 512, 768}, {"Godot 1280x720", 640, 720}};

    EntityId me, other;
    Pos pme, pother;
    REQUIRE(w.spawn_player(1, role(11, "A", c.spawn_point), me, pme) == jx::pb::RESULT_OK);

    for (const Screen& s : screens) {
        INFO(s.name);
        for (const Pos corner : {Pos{s.half_x, s.half_y}, Pos{-s.half_x, s.half_y},
                                 Pos{s.half_x, -s.half_y}, Pos{-s.half_x, -s.half_y}}) {
            const Pos at{c.spawn_point.x + corner.x, c.spawn_point.y + corner.y};
            REQUIRE(w.spawn_player(2, role(22, "B", at), other, pother) == jx::pb::RESULT_OK);
            auto out = w.take_outbox();
            for (std::uint32_t i = 0; i <= c.interest_period; ++i) {   // A's next routine look
                w.tick();
                for (Packet& pk : w.take_outbox()) out.push_back(std::move(pk));
            }
            // A must be told about B, and B about A: the contract is what the client receives.
            INFO("corner " << corner.x << "," << corner.y);
            CHECK_FALSE(to(out, 1, jx::pb::G2C_ENTITY_SPAWN).empty());
            CHECK_FALSE(to(out, 2, jx::pb::G2C_ENTITY_SPAWN).empty());
            REQUIRE(w.remove_player(2));
            w.take_outbox();
        }
    }
}

TEST_CASE("nearby chat reaches exactly the clients that see the speaker", "[world][aoi]")
{
    // The old rule cut every packet to the 100 nearest sessions (MAX_BROADCAST_COUNT in
    // Core/Src/KRegion.h).  The limit now sits on what each client KNOWS, so who hears a line of
    // chat is not "the first 8" but precisely the clients that were sent the speaker's spawn.
    // test_KInterest.cpp holds the traffic bound and the no-ghost guarantees.
    Quiet q;
    KSubWorldConfig c;
    c.width = 20000;
    c.height = 20000;
    c.spawn_point = Pos{10000, 10000};
    c.max_viewers = 8;
    KSubWorld w(c);

    EntityId id, speaker;
    Pos p;
    for (std::uint64_t sid = 1; sid <= 40; ++sid) {
        const Pos at{c.spawn_point.x + static_cast<std::int32_t>(sid), c.spawn_point.y};
        REQUIRE(w.spawn_player(sid, role(10 + sid, "P" + std::to_string(sid), at), id, p) == jx::pb::RESULT_OK);
        if (sid == 1) speaker = id;
    }
    for (int i = 0; i < 20; ++i) w.tick();
    w.take_outbox();

    std::vector<std::uint64_t> expected;
    for (std::uint64_t sid = 1; sid <= 40; ++sid) {
        const jx::zone::KViewer* v = w.viewer(sid);
        REQUIRE(v != nullptr);
        CHECK(v->known_players <= c.max_viewers);
        if (std::find(v->known.begin(), v->known.end(), speaker) != v->known.end()) expected.push_back(sid);
    }
    REQUIRE(w.chat(1, "xin chao"));
    const auto out = w.take_outbox();
    REQUIRE(out.size() == 1);
    CHECK(out[0].sids == expected);
    CHECK(expected.size() > 1);                      // himself and his neighbours
    CHECK(expected.size() < 40);                     // not the whole crowd
    CHECK(w.viewers_capped() > 0);
}
