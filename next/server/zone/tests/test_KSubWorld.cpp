#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
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
    const auto to_b = to(out, 2, jx::pb::G2C_ENTITY_SPAWN);
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
