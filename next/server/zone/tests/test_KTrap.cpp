// Traps: KNpc::CheckTrap running the trap script of the cell (SetPos / SetFightState / NewWorld).
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"

using jx::EntityId;
using jx::zone::KMapData;
using jx::zone::KScriptCache;
using jx::zone::KSubWorld;
using jx::zone::KSubWorldConfig;
using jx::zone::Packet;
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

// the city gate script of the old maps (凤翔北门.lua) and a portal to another map
constexpr const char* kGate = R"lua(
function main(sel)
if ( GetFightState() == 0 ) then
	SetPos(20, 20)
	SetFightState(1)
else
	SetPos(5, 5)
	SetFightState(0)
end;
	AddStation(1)
end;
)lua";

constexpr const char* kPortal = R"lua(
function main(sel)
SetFightState(1);
NewWorld(3, 40, 41);
AddTermini(2)
end;
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_trap_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "gate.lua") << kGate;
    std::ofstream(root / "script" / "test" / "portal.lua") << kPortal;
    return root.generic_string();
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

KSubWorldConfig trap_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.cell_size = 512;
    c.default_speed = 200;
    KMapData m = KMapData::synthetic(64, 64);   // 2048 x 2048
    m.id = 1;
    m.set_trap(10, 10, 2, 0x1001, R"(\script\test\gate.lua)");     // cells (10,10) (11,10)
    m.set_trap(30, 30, 1, 0x1002, R"(\script\test\portal.lua)");
    m.set_trap(50, 50, 1, 0x1003, "");                              // no script known
    c.map = std::make_shared<const KMapData>(std::move(m));
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    return c;
}

} // namespace

TEST_CASE("KMapData keeps the trap runs of the region files", "[trap][map]")
{
    KMapData m = KMapData::synthetic(8, 8);
    m.set_trap(2, 3, 3, 77, R"(\script\x.lua)");
    CHECK(m.trap_at(Pos{2 * 32 + 5, 3 * 32 + 5}) == 77);
    CHECK(m.trap_at(Pos{4 * 32 + 31, 3 * 32}) == 77);
    CHECK(m.trap_at(Pos{5 * 32, 3 * 32}) == 0);
    CHECK(m.trap_at(Pos{2 * 32, 4 * 32}) == 0);
    CHECK(m.trap_script(77) == R"(\script\x.lua)");
    CHECK(m.trap_script(78).empty());
}

TEST_CASE("a map bundle with traps loads them into the cell grid", "[trap][map]")
{
    Quiet q;
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "jxnext_trap_bundle";
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "map.json") << R"({"id": 5, "name": "t", "cell_size": 32, "cells_x": 4, "cells_y": 4, "scene_w": 128, "scene_h": 128,
        "spawn": [16, 16], "npcs": [], "traps": [{"x": 1, "y": 2, "n": 2, "id": 9, "script": "\\script\\a.lua"}, {"x": 3, "y": 3, "n": 1, "id": 10, "script": ""}]})";
    std::ofstream(dir / "obstacle.bin", std::ios::binary) << std::string(16, '\0');
    std::string error;
    const auto m = KMapData::load(dir, &error);
    REQUIRE(m.has_value());
    CHECK(m->trap_at(Pos{1 * 32 + 1, 2 * 32 + 1}) == 9);
    CHECK(m->trap_at(Pos{2 * 32 + 1, 2 * 32 + 1}) == 9);
    CHECK(m->trap_at(Pos{3 * 32 + 1, 3 * 32 + 1}) == 10);
    CHECK(m->trap_at(Pos{0, 0}) == 0);
    CHECK(m->trap_script(9) == R"(\script\a.lua)");
    CHECK(m->trap_script(10).empty());
}

TEST_CASE("a gate trap runs its script: SetPos and the fight state toggle", "[trap][world]")
{
    Quiet q;
    KSubWorld w(trap_world());
    EntityId hero;
    Pos at;
    // spawned right on the trap cell (10,10): CheckTrap fires on the first tick
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{10 * 32 + 16, 10 * 32 + 16}), hero, at) == jx::pb::RESULT_OK);
    REQUIRE(at == Pos{10 * 32 + 16, 10 * 32 + 16});
    w.take_outbox();
    w.tick();
    const jx::zone::KNpc* h = w.find_player(7);
    REQUIRE(h != nullptr);
    CHECK(h->pos() == Pos{20 * 32, 20 * 32});   // SetPos(20, 20) -> cells * 32
    CHECK(h->fight_mode);
    CHECK(h->trap_script_id == 0x1001);
    bool moved = false;
    for (const Packet& p : w.take_outbox()) {
        if (p.msg_id == jx::pb::G2C_ENTITY_MOVE) moved = true;
    }
    CHECK(moved);   // the client is told about the jump

    // standing elsewhere clears m_TrapScriptID; back on the trap the other branch runs
    w.tick();
    CHECK(w.find_player(7)->trap_script_id == 0);
    REQUIRE(w.teleport(hero, Pos{11 * 32 + 3, 10 * 32 + 3}));
    w.tick();
    CHECK(w.find_player(7)->pos() == Pos{5 * 32, 5 * 32});
    CHECK_FALSE(w.find_player(7)->fight_mode);

    // a trap without a known script does nothing but is remembered (no re-fire every tick)
    REQUIRE(w.teleport(hero, Pos{50 * 32 + 1, 50 * 32 + 1}));
    w.tick();
    CHECK(w.find_player(7)->trap_script_id == 0x1003);
    CHECK(w.find_player(7)->pos() == Pos{50 * 32 + 1, 50 * 32 + 1});
}

TEST_CASE("the fight state of a gate trap is saved with the character and restored", "[trap][world][role]")
{
    Quiet q;
    KSubWorld w(trap_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{10 * 32 + 16, 10 * 32 + 16}), hero, at) == jx::pb::RESULT_OK);
    w.tick();   // the gate trap switches the fight state on
    REQUIRE(w.find_player(7)->fight_mode);
    jx::pb::RoleData saved;
    REQUIRE(w.role_snapshot(7, saved));
    CHECK(saved.fight_mode());
    CHECK(saved.position().map_id() == 1);

    // logging in again with that record starts in the same stance
    KSubWorld w2(trap_world());
    EntityId again;
    Pos at2;
    saved.mutable_position()->mutable_pos()->set_x(40 * 32);
    saved.mutable_position()->mutable_pos()->set_y(40 * 32);
    REQUIRE(w2.spawn_player(8, saved, again, at2) == jx::pb::RESULT_OK);
    CHECK(w2.find_player(8)->fight_mode);
}

TEST_CASE("script coordinates are absolute Mps: the map origin is subtracted", "[trap][world]")
{
    Quiet q;
    KSubWorldConfig c = trap_world();
    KMapData m = KMapData::synthetic(64, 64);
    m.id = 1;
    m.origin = Pos{3 * 512, 2 * 1024};   // region_left 3, region_top 2
    m.set_trap(10, 10, 1, 0x1001, R"(\script\test\gate.lua)");
    c.map = std::make_shared<const KMapData>(std::move(m));
    KSubWorld w(c);
    CHECK(w.to_local(Pos{1536 + 5, 2048 + 7}) == Pos{5, 7});
    CHECK(w.to_absolute(Pos{5, 7}) == Pos{1541, 2055});
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{10 * 32 + 16, 10 * 32 + 16}), hero, at) == jx::pb::RESULT_OK);
    w.tick();
    // SetPos(20, 20) = absolute (640, 640) -> local (640 - 1536, 640 - 2048) is off the map: clamped to the corner
    CHECK(w.find_player(7)->pos() == Pos{0, 0});
}

TEST_CASE("a portal trap asks for another map (NewWorld) instead of moving on its own", "[trap][world]")
{
    Quiet q;
    KSubWorld w(trap_world());
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{30 * 32 + 8, 30 * 32 + 8}), hero, at) == jx::pb::RESULT_OK);
    w.tick();
    const auto changes = w.take_world_changes();
    REQUIRE(changes.size() == 1);
    CHECK(changes[0].sid == 7);
    CHECK(changes[0].map_id == 3);
    CHECK(changes[0].pos == Pos{40 * 32, 41 * 32});
    CHECK(w.find_player(7)->fight_mode);
    CHECK(w.find_player(7)->pos() == Pos{30 * 32 + 8, 30 * 32 + 8});   // the server moves it, not the world
    CHECK(w.take_world_changes().empty());
    // NewWorld to the map we are in is a plain SetPos (KNpc::ChangeWorld: same subworld)
    jx::zone::KNpc* self = nullptr;
    (void)self;
}

// The 3D maps' areas (map.json "areas", the reference's scn_area_list + MarkArea polygons): the safe flag of the
// highest-priority area under the character switches the fight mode as the gate traps do (KSubWorld::check_area).
TEST_CASE("KMapData areas: even-odd polygon test, the highest priority wins, the bundle field", "[trap][map][area]")
{
    KMapData m = KMapData::synthetic(64, 64);
    // a diamond of priority 2 (safe) inside a rectangle of priority 1 (fight)
    m.add_area(1, "safe", true, 2, {Pos{500, 200}, Pos{800, 500}, Pos{500, 800}, Pos{200, 500}});
    m.add_area(2, "fight", false, 1, {Pos{0, 0}, Pos{2000, 0}, Pos{2000, 2000}, Pos{0, 2000}});
    CHECK(m.area_at(Pos{500, 500}) == 1);      // the middle of the diamond
    CHECK(m.area_at(Pos{260, 500}) == 1);      // near the left tip (x = 200 at y = 500)
    CHECK(m.area_at(Pos{230, 250}) == 2);      // outside the diamond's slanted edge, inside the rectangle
    CHECK(m.area_at(Pos{1500, 1500}) == 2);
    CHECK(m.area_at(Pos{2001, 5}) == 0);       // out of every area
    REQUIRE(m.area(1) != nullptr);
    CHECK(m.area(1)->safe);
    CHECK(m.area(1)->min_x == 200);
    CHECK(m.area(1)->max_y == 800);
    CHECK(m.area(3) == nullptr);
    // the order of the list does not matter: the higher priority still wins where they overlap
    KMapData m2 = KMapData::synthetic(64, 64);
    m2.add_area(2, "fight", false, 1, {Pos{0, 0}, Pos{2000, 0}, Pos{2000, 2000}, Pos{0, 2000}});
    m2.add_area(1, "safe", true, 2, {Pos{500, 200}, Pos{800, 500}, Pos{500, 800}, Pos{200, 500}});
    CHECK(m2.area_at(Pos{500, 500}) == 1);
    // a polygon of fewer than three points is not an area
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "jxnext_area_bundle";
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "map.json") << R"({"id": 6, "name": "a", "cell_size": 32, "cells_x": 4, "cells_y": 4, "scene_w": 128, "scene_h": 128,
        "spawn": [16, 16], "npcs": [], "areas": [{"id": 1, "name": "safe", "safe": true, "priority": 2, "poly": [[0, 0], [64, 0], [64, 64], [0, 64]]},
        {"id": 2, "name": "fight", "priority": 1, "poly": [[0, 0], [128, 0], [128, 128], [0, 128]]}, {"id": 3, "name": "bad", "poly": [[1, 1], [2, 2]]}]})";
    std::ofstream(dir / "obstacle.bin", std::ios::binary) << std::string(16, '\0');
    std::string error;
    const auto b = KMapData::load(dir, &error);
    REQUIRE(b.has_value());
    REQUIRE(b->areas.size() == 2);
    CHECK(b->area_at(Pos{10, 10}) == 1);
    CHECK(b->area_at(Pos{100, 100}) == 2);
    CHECK_FALSE(b->area(2)->safe);
}

TEST_CASE("walking out of the safe area switches the fight mode on, back in switches it off", "[trap][world][area]")
{
    Quiet q;
    KSubWorldConfig c = trap_world();
    KMapData m = KMapData::synthetic(64, 64);
    m.id = 1;
    m.add_area(1, "safe", true, 2, {Pos{0, 0}, Pos{640, 0}, Pos{640, 640}, Pos{0, 640}});
    m.add_area(2, "fight", false, 1, {Pos{0, 0}, Pos{1500, 0}, Pos{1500, 1500}, Pos{0, 1500}});
    c.map = std::make_shared<const KMapData>(std::move(m));
    KSubWorld w(c);
    EntityId hero;
    Pos at;
    jx::pb::RoleData r = role(70, "Hero", Pos{300, 300});
    r.set_fight_mode(true);   // the saved stance: the village area takes it off on the first tick
    REQUIRE(w.spawn_player(7, r, hero, at) == jx::pb::RESULT_OK);
    w.tick();
    const jx::zone::KNpc* h = w.find_player(7);
    REQUIRE(h != nullptr);
    CHECK(h->area_id == 1);
    CHECK_FALSE(h->fight_mode);
    // a GM SetFightState holds while the character stays in the area
    w.mutable_entity(hero)->fight_mode = true;
    w.tick();
    CHECK(w.find_player(7)->fight_mode);
    // out into the wild: fight mode on
    REQUIRE(w.teleport(hero, Pos{1000, 1000}));
    w.tick();
    CHECK(w.find_player(7)->area_id == 2);
    CHECK(w.find_player(7)->fight_mode);
    // back into the village: off again
    REQUIRE(w.teleport(hero, Pos{100, 100}));
    w.tick();
    CHECK(w.find_player(7)->area_id == 1);
    CHECK_FALSE(w.find_player(7)->fight_mode);
    // beyond every area the stance stays as it was
    w.mutable_entity(hero)->fight_mode = true;
    REQUIRE(w.teleport(hero, Pos{1800, 1800}));
    w.tick();
    CHECK(w.find_player(7)->area_id == 0);
    CHECK(w.find_player(7)->fight_mode);
}
