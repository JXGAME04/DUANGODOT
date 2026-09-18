#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KSubWorld.h"

using jx::zone::KMapData;
using jx::zone::Pos;

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

// 20 x 20 cells, a vertical wall at x = 10 from y = 0..14 (gap at the bottom)
KMapData walled()
{
    KMapData m = KMapData::synthetic(20, 20);
    for (int y = 0; y < 15; ++y) m.set_blocked(10, y);
    return m;
}

} // namespace

TEST_CASE("walkability, nearest walkable and line of sight", "[map]")
{
    const KMapData m = walled();
    CHECK(m.walkable(Pos{16, 16}));
    CHECK_FALSE(m.walkable(Pos{10 * 32 + 5, 5}));
    CHECK_FALSE(m.walkable_cell(-1, 0));
    CHECK_FALSE(m.walkable_cell(20, 0));

    const Pos near = m.nearest_walkable(Pos{10 * 32 + 16, 16});   // inside the wall
    CHECK(m.walkable(near));
    CHECK((near.x == 9 * 32 + 16 || near.x == 11 * 32 + 16));
    CHECK(m.nearest_walkable(Pos{16, 16}) == Pos{16, 16});

    CHECK(m.line_of_sight(Pos{16, 16}, Pos{8 * 32 + 16, 16}));
    CHECK_FALSE(m.line_of_sight(Pos{16, 16}, Pos{15 * 32 + 16, 16}));   // crosses the wall
    CHECK(m.line_of_sight(Pos{16, 17 * 32}, Pos{15 * 32 + 16, 17 * 32}));  // below the wall
}

TEST_CASE("A* routes around the wall and smooths straight stretches", "[map]")
{
    const KMapData m = walled();
    const Pos from{2 * 32 + 16, 2 * 32 + 16};
    const Pos to{17 * 32 + 16, 2 * 32 + 16};
    const auto path = m.find_path(from, to);
    REQUIRE(!path.empty());
    CHECK(path.back() == to);
    CHECK(path.size() >= 2);            // must dip below the wall: at least two legs
    CHECK(path.size() <= 6);            // ... but smoothing keeps it short
    // every leg is walkable end to end
    Pos prev = from;
    for (const Pos& p : path) {
        CHECK(m.line_of_sight(prev, p));
        prev = p;
    }
    // the route passes below row 15 somewhere
    bool below = false;
    for (const Pos& p : path) below = below || p.y >= 15 * 32;
    CHECK(below);

    CHECK(m.find_path(from, Pos{5 * 32, 5 * 32}).size() == 1);   // direct line of sight: one leg
    CHECK(m.find_path(from, from).size() == 1);

    KMapData boxed = KMapData::synthetic(5, 5);
    for (int i = 0; i < 5; ++i) {
        boxed.set_blocked(2, i);
    }
    CHECK(boxed.find_path(Pos{16, 16}, Pos{4 * 32 + 16, 16}).empty());   // unreachable
    CHECK(boxed.find_path(Pos{16, 16}, Pos{2 * 32 + 16, 16}).empty());   // target blocked
}

TEST_CASE("world with a map follows waypoints and clamps to walkable cells", "[map][world]")
{
    Quiet q;
    jx::zone::KSubWorldConfig cfg;
    cfg.tick_hz = 20;
    cfg.default_speed = 320;   // 16 units per tick
    cfg.cell_size = 512;
    cfg.map = std::make_shared<const KMapData>(walled());
    cfg.map_npcs = false;
    jx::zone::KSubWorld w(cfg);
    CHECK(w.config().width == 20 * 32);
    // the synthetic spawn sits on the wall: the world moves it to the nearest walkable cell
    CHECK(w.config().spawn_point == cfg.map->nearest_walkable(cfg.map->spawn));
    CHECK(cfg.map->walkable(w.config().spawn_point));

    jx::pb::RoleData role;
    role.set_player_id(1);
    role.set_name("A");
    role.mutable_position()->set_zone_id(1);
    role.mutable_position()->mutable_pos()->set_x(2 * 32 + 16);
    role.mutable_position()->mutable_pos()->set_y(2 * 32 + 16);
    jx::EntityId id;
    Pos at;
    REQUIRE(w.spawn_player(1, role, id, at) == jx::pb::RESULT_OK);
    CHECK(at == Pos{2 * 32 + 16, 2 * 32 + 16});
    w.take_outbox();

    // a request into the wall lands on the nearest walkable cell, on the far side of the wall
    REQUIRE(w.move_request(1, Pos{17 * 32 + 16, 2 * 32 + 16}, 1));
    auto out = w.take_outbox();
    REQUIRE(out.size() == 1);
    jx::pb::EntityMove mv;
    REQUIRE(mv.ParseFromString(out[0].payload));
    CHECK(mv.path_size() >= 2);
    CHECK(mv.target().x() == 17 * 32 + 16);

    for (int i = 0; i < 400 && w.find_player(1)->moving; ++i) w.tick();
    CHECK_FALSE(w.find_player(1)->moving);
    CHECK(w.find_player(1)->pos() == Pos{17 * 32 + 16, 2 * 32 + 16});
    // arrival is announced with an empty path
    out = w.take_outbox();
    bool arrived = false;
    for (const auto& p : out) {
        if (p.msg_id != jx::pb::G2C_ENTITY_MOVE) continue;
        REQUIRE(mv.ParseFromString(p.payload));
        arrived = mv.path_size() == 0 && mv.pos().x() == 17 * 32 + 16;
    }
    CHECK(arrived);

    // the whole walk never crossed a blocked cell
    REQUIRE(w.move_request(1, Pos{2 * 32 + 16, 2 * 32 + 16}, 2));
    for (int i = 0; i < 400 && w.find_player(1)->moving; ++i) {
        w.tick();
        CHECK(cfg.map->walkable(w.find_player(1)->pos()));
    }
    CHECK(w.find_player(1)->pos() == Pos{2 * 32 + 16, 2 * 32 + 16});
}

TEST_CASE("map npcs are placed from the bundle", "[map][world]")
{
    Quiet q;
    KMapData m = KMapData::synthetic(10, 10);
    m.npcs.push_back(jx::zone::KNpcPlacement{7, "Lão Bản", Pos{100, 100}, 0, 1, ""});
    m.npcs.push_back(jx::zone::KNpcPlacement{8, "Thợ Rèn", Pos{200, 200}, 0, 1, ""});
    jx::zone::KSubWorldConfig cfg;
    cfg.map = std::make_shared<const KMapData>(std::move(m));
    jx::zone::KSubWorld w(cfg);
    CHECK(w.entity_count() == 2);
    CHECK(w.take_outbox().empty());
    jx::pb::RoleData role;
    role.set_player_id(1);
    role.set_name("A");
    jx::EntityId id;
    Pos at;
    REQUIRE(w.spawn_player(1, role, id, at) == jx::pb::RESULT_OK);
    const auto out = w.take_outbox();
    REQUIRE(out.size() == 4);   // the spawn, the item list, the character's numbers and the skill list
    REQUIRE(out[0].msg_id == jx::pb::G2C_ENTITY_SPAWN);
    jx::pb::EntitySpawn spawn;
    REQUIRE(spawn.ParseFromString(out[0].payload));
    CHECK(spawn.entities_size() == 3);
    bool named = false;
    for (const auto& e : spawn.entities()) named = named || e.name() == "Thợ Rèn";
    CHECK(named);
}
