// The events a script registers on a player (docs/LINUX-SERVER.md §23): KPlayerEvent at Player+0x8064 (Add 0x081560C0 /
// Remove 0x08156050 / Clear 0x08155F60), the table of event_killnpc.txt (0x08156760), the kills that count them
// (0x08083720 -> 0x08155F80 -> 0x08156330: the match, the count, the script when the total is reached, the task value
// mirror), the script api AddPlayerEvent / RemovePlayerEvent / RemoveAllPlayerEvent / GetNpcName / GetNpcPos /
// NpcName2Replace / NpcDialog / GetLastDiagNpc / GetNpcSettingIdx / GetLevel / GetName / GetSex.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerEvent.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KKillEventRow;
using jx::zone::KKillEventTable;
using jx::zone::KMagicAttrib;
using jx::zone::KNpc;
using jx::zone::KNpcKind;
using jx::zone::KPlayerEvent;
using jx::zone::KScriptCache;

namespace {

constexpr const char* kKillEventsJson = R"({"source": "test", "rows": [
 {"id": 1, "script": "\\script\\test\\kill.lua", "function": "OnKill", "task_id": 300, "only_once": true, "total": 2, "map": -1, "npc_template": 7, "power": -1, "level": -1},
 {"id": 2, "script": "\\script\\test\\kill.lua", "function": "OnKillAny", "task_id": 301, "only_once": false, "total": 1, "map": -1, "npc_template": -1, "power": 1, "level": -1},
 {"id": 3, "script": "\\script\\test\\missing.lua", "function": "OnKill", "task_id": 302, "only_once": false, "total": 1, "map": -1, "npc_template": -1, "power": -1, "level": -1},
 {"id": 4, "script": "\\script\\test\\kill.lua", "function": "OnKill", "task_id": 9999, "only_once": false, "total": 1, "map": -1, "npc_template": -1, "power": -1, "level": -1},
 {"id": 5, "script": "\\script\\test\\kill.lua", "function": "OnKill", "task_id": 305, "only_once": false, "total": 1, "map": 77, "npc_template": -1, "power": -1, "level": -1}
]})";

constexpr const char* kKillScript = R"(
g_fires = 0
g_any = 0
function OnKill(player, task, event, map, template, count, power, level)
    g_hit = {player, task, event, map, template, count, power, level}
    g_fires = g_fires + 1
end
function OnKillAny(player, task, event, map, template, count, power, level)
    g_any = g_any + 1
end
function Fires()
    return g_fires
end
function Any()
    return g_any
end
function Hit(i)
    return g_hit[i]
end
)";

constexpr const char* kApiScript = R"(
function main(param)
    g_add = AddPlayerEvent(1)
    g_add2 = AddPlayerEvent(2)
    g_dup = AddPlayerEvent(1)
    g_level = GetLevel()
    g_name = GetName()
    g_sex = GetSex()
    g_last = GetLastDiagNpc()
    g_replace = NpcName2Replace("Banh Tieu de")
end
function Remove(id)
    return RemovePlayerEvent(id)
end
function RemoveAll()
    return RemoveAllPlayerEvent()
end
function NpcName(id)
    local n = GetNpcName(id)
    if n == nil then return "nil" end
    return n
end
function NpcX(id)
    local x, y, w = GetNpcPos(id)
    return x
end
function NpcW(id)
    local x, y, w = GetNpcPos(id)
    return w
end
function NpcIdx(id)
    return GetNpcSettingIdx(id)
end
function Dialog()
    NpcDialog()
    return 1
end
function Str(v)
    if v == nil then return "nil" end
    return tostring(v)
end
function Is(name, expected)
    if Str(_G[name]) == expected then return 1 end
    return 0
end
function Get(name)
    return _G[name]
end
)";

constexpr const char* kNpcScript = R"(
g_main = 0
function main(param)
    g_main = g_main + 1
end
function Mains()
    return g_main
end
)";

std::string make_files()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_event_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "kill.lua") << kKillScript;
    std::ofstream(root / "script" / "test" / "api.lua") << kApiScript;
    std::ofstream(root / "script" / "test" / "npc.lua") << kNpcScript;
    std::ofstream(root / "kill_events.json") << kKillEventsJson;
    return root.string();
}

jx::zone::KSubWorldConfig event_world(const std::string& root)
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
    c.scripts = std::make_shared<KScriptCache>(root);
    std::string error;
    auto t = KKillEventTable::load((std::filesystem::path(root) / "kill_events.json").string(), &error);
    REQUIRE(t.has_value());
    c.kill_events = std::make_shared<const KKillEventTable>(std::move(*t));
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
    r.set_level(10);
    r.set_sex(1);
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

std::vector<jx::pb::TaskValue> values_of(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::TaskValue> out;
    for (const auto& p : all) {
        if (p.msg_id != jx::pb::G2C_TASK_VALUE || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
        jx::pb::TaskValue v;
        REQUIRE(v.ParseFromString(p.payload));
        out.push_back(v);
    }
    return out;
}

KMagicAttrib attrib(int type, int v0, int v1 = 0, int v2 = 0)
{
    KMagicAttrib a;
    a.type = type;
    a.value = {v0, v1, v2};
    return a;
}

// A (sid 7) at the spawn; a wolf of template 7 and a hen of template 9 next to it, both level 5
struct EventWorld {
    std::string root;
    jx::zone::KSubWorld w;
    jx::EntityId a, wolf, hen;
    explicit EventWorld(const jx::pb::RoleData& role = role_of(1, "A", 2000, 2000)) : root(make_files()), w(event_world(root))
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role, a, at) == jx::pb::RESULT_OK);
        wolf = w.spawn_npc("Soi", jx::zone::Pos{2100, 2000}, 7, 0, KNpcKind::monster, 5, 0, 0);
        hen = w.spawn_npc("Ga", jx::zone::Pos{2000, 2100}, 9, 0, KNpcKind::monster, 5, 0, 0);
        w.mutable_entity(wolf)->script = R"(\script\test\npc.lua)";
        w.tick();
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    bool run(const char* fn, int param = 0) { return w.execute_script(R"(\script\test\api.lua)", fn, A(), param); }
    double call(const char* fn, const std::vector<jx::zone::KLuaScript::Arg>& args = {})
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\api.lua)");
        REQUIRE(s != nullptr);
        jx::zone::KScriptContext& ctx = jx::zone::g_ScriptContext();
        const jx::zone::KScriptContext saved = ctx;
        ctx.world = &w;
        ctx.player = &A();
        ctx.sid = 7;
        const double r = s->call_number(fn, args).value_or(-999.0);
        ctx = saved;
        return r;
    }
    bool is(const char* name, const char* expected) { return call("Is", {std::string(name), std::string(expected)}) == 1.0; }
    double kill_script(const char* fn, const std::vector<jx::zone::KLuaScript::Arg>& args = {})
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\kill.lua)");
        REQUIRE(s != nullptr);
        return s->call_number(fn, args).value_or(-999.0);
    }
    // the last hit of A on the npc, then the fire of the end of its death frames
    void kill(jx::EntityId npc)
    {
        KNpc* n = w.mutable_entity(npc);
        REQUIRE(n != nullptr);
        n->last_damage_id = a;
        w.fire_kill_events(*n);
    }
};

} // namespace

TEST_CASE("KPlayerEvent: sixty-three pairs at most, a known id stays, remove and clear", "[event]")
{
    KPlayerEvent ev;
    CHECK(ev.add(5));
    CHECK(ev.add(5, 9));   // already there: 1, the count untouched
    REQUIRE(ev.entries.size() == 1);
    CHECK(ev.entries[0].count == 0);
    CHECK(ev.find(5) != nullptr);
    CHECK(ev.find(6) == nullptr);
    for (int i = 100; i < 162; ++i) CHECK(ev.add(i));
    CHECK(ev.entries.size() == 63);
    CHECK(ev.add(999));            // the sixty-fourth goes in: the check is on the count before the add (> 0x3f)
    CHECK(ev.entries.size() == 64);
    CHECK_FALSE(ev.add(1000));     // sixty-four held: refused
    CHECK(ev.remove(5));
    CHECK_FALSE(ev.remove(5));
    CHECK(ev.entries.size() == 63);
    CHECK(ev.entries[0].id == 100);
    ev.clear();
    CHECK(ev.entries.empty());
    // the words
    CHECK(ev.add(0x10001, 0x20003));
    CHECK(ev.entries[0].id == 1);
    CHECK(ev.entries[0].count == 3);
}

TEST_CASE("the kill-event table of event_killnpc.txt and the npc power 0x08079750", "[event]")
{
    const std::string root = make_files();
    std::string error;
    auto t = KKillEventTable::load((std::filesystem::path(root) / "kill_events.json").string(), &error);
    REQUIRE(t.has_value());
    CHECK(t->source == "test");
    CHECK(t->size() == 4);   // the row with a task value past 0x176f is dropped
    CHECK(t->find(4) == nullptr);
    const KKillEventRow* r = t->find(1);
    REQUIRE(r != nullptr);
    CHECK(r->script == R"(\script\test\kill.lua)");
    CHECK(r->function == "OnKill");
    CHECK(r->task_id == 300);
    CHECK(r->only_once);
    CHECK(r->total == 2);
    CHECK(r->npc_template == 7);
    CHECK(r->matches(1, 7, 1, 5));
    CHECK_FALSE(r->matches(1, 8, 1, 5));
    CHECK(r->matches(1, -1, 1, 5));   // a kill that says -1 matches too (0x081563BE)
    const KKillEventRow* any = t->find(2);
    REQUIRE(any != nullptr);
    CHECK(any->matches(0, 0, 1, 0));
    CHECK_FALSE(any->matches(0, 0, 2, 0));
    CHECK(t->find(5)->matches(77, 1, 1, 1));
    CHECK_FALSE(t->find(5)->matches(78, 1, 1, 1));
    CHECK(jx::zone::npc_power_of(true, true, true) == 0);
    CHECK(jx::zone::npc_power_of(false, true, true) == 3);
    CHECK(jx::zone::npc_power_of(false, false, true) == 2);
    CHECK(jx::zone::npc_power_of(false, false, false) == 1);
    CHECK_FALSE(KKillEventTable::load((std::filesystem::path(root) / "nothing.json").string(), &error).has_value());
}

TEST_CASE("the script api: AddPlayerEvent / RemovePlayerEvent / RemoveAllPlayerEvent, GetLevel / GetName / GetSex, the npc helpers", "[event][world]")
{
    EventWorld ew;
    REQUIRE(ew.run("main"));
    CHECK(ew.is("g_add", "1"));
    CHECK(ew.is("g_add2", "1"));
    CHECK(ew.is("g_dup", "1"));
    CHECK(ew.A().player.events.entries.size() == 2);
    CHECK(ew.is("g_level", "10"));
    CHECK(ew.is("g_name", "A"));
    CHECK(ew.is("g_sex", "1"));
    CHECK(ew.is("g_last", "0"));
    CHECK(ew.is("g_replace", "Banh Tieu de"));
    CHECK(ew.call("Remove", {3.0}) == 0.0);
    CHECK(ew.call("Remove", {2.0}) == 1.0);
    CHECK(ew.A().player.events.entries.size() == 1);
    CHECK(ew.call("RemoveAll") == 1.0);
    CHECK(ew.A().player.events.entries.empty());
    // the npc helpers take the entity id of the npc
    const double wolf = static_cast<double>(ew.wolf.value);
    CHECK(ew.call("NpcName", {wolf}) == -999.0);   // a string comes back: call_number sees no number
    CHECK(ew.call("Is", {std::string("g_none"), std::string("nil")}) == 1.0);
    CHECK(ew.call("NpcIdx", {wolf}) == 7.0);
    CHECK(ew.call("NpcIdx", {99999.0}) == 0.0);
    CHECK(ew.call("NpcX", {wolf}) == static_cast<double>(ew.w.to_absolute(jx::zone::Pos{2100, 2000}).x / 32));
    CHECK(ew.call("NpcW", {wolf}) == 0.0);
    // NpcDialog(): the npc last talked to runs its main again; none -> nothing
    CHECK(ew.call("Dialog") == 1.0);
    jx::zone::KLuaScript* npc = ew.w.config().scripts->get(R"(\script\test\npc.lua)");
    REQUIRE(npc != nullptr);
    CHECK(npc->call_number("Mains", {}) == 0.0);
    ew.A().player.dialog.npc = ew.wolf;
    CHECK(ew.call("Dialog") == 1.0);
    CHECK(npc->call_number("Mains", {}) == 1.0);
    CHECK(ew.is("g_last", "0"));
    REQUIRE(ew.run("main"));
    CHECK(ew.call("Get", {std::string("g_last")}) == static_cast<double>(ew.wolf.value));
}

TEST_CASE("a kill counts the first matching event, mirrors the count into the task value, fires the script at the total", "[event][world]")
{
    EventWorld ew;
    ew.A().player.events.add(3);   // a script that is not there: skipped
    ew.A().player.events.add(1);   // template 7, twice, once
    ew.A().player.events.add(2);   // any npc of power 1, every kill
    ew.A().player.events.add(5);   // another map: never
    // the hen (template 9): event 1 does not match, event 2 does - total 1: the script, the count starts over at 0
    ew.kill(ew.hen);
    CHECK(ew.kill_script("Any") == 1.0);
    CHECK(ew.kill_script("Fires") == 0.0);
    CHECK(ew.A().player.events.find(2)->count == 0);
    CHECK(ew.A().player.task.get_save_val(301) == 0);
    auto values = values_of(ew.w.take_outbox(), 7);
    REQUIRE(values.size() == 1);   // the mirror is told even when the value did not change (0x0815650F)
    CHECK(values[0].id() == 301);
    CHECK(values[0].value() == 0);
    // the wolf (template 7): event 1 comes first in the list order (3, 1, 2, 5): the first kill counts 1 and ends the walk
    ew.kill(ew.wolf);
    CHECK(ew.kill_script("Fires") == 0.0);
    CHECK(ew.kill_script("Any") == 1.0);
    CHECK(ew.A().player.events.find(1)->count == 1);
    CHECK(ew.A().player.task.get_save_val(300) == 1);
    values = values_of(ew.w.take_outbox(), 7);
    REQUIRE(values.size() == 1);
    CHECK(values[0].id() == 300);
    CHECK(values[0].value() == 1);
    // the second wolf: the total 2 is reached - OnKill(player, task, event, map, template, count, power, level), then the
    // event is gone (only once) and the task value holds the count reached
    ew.kill(ew.wolf);
    CHECK(ew.kill_script("Fires") == 1.0);
    CHECK(ew.kill_script("Hit", {1.0}) == 7.0);
    CHECK(ew.kill_script("Hit", {2.0}) == 300.0);
    CHECK(ew.kill_script("Hit", {3.0}) == 1.0);
    CHECK(ew.kill_script("Hit", {4.0}) == static_cast<double>(ew.w.map_id()));
    CHECK(ew.kill_script("Hit", {5.0}) == 7.0);
    CHECK(ew.kill_script("Hit", {6.0}) == 2.0);
    CHECK(ew.kill_script("Hit", {7.0}) == 1.0);
    CHECK(ew.kill_script("Hit", {8.0}) == 5.0);
    CHECK(ew.A().player.events.find(1) == nullptr);
    CHECK(ew.A().player.task.get_save_val(300) == 2);
    CHECK(ew.A().player.events.entries.size() == 3);
    // a third wolf: event 1 is gone, event 2 takes it
    ew.kill(ew.wolf);
    CHECK(ew.kill_script("Any") == 2.0);
    CHECK(ew.kill_script("Fires") == 1.0);
    // a player's death fires nothing; a npc nobody hit fires nothing
    ew.w.fire_kill_events(ew.A());
    KNpc* hen = ew.w.mutable_entity(ew.hen);
    hen->last_damage_id = jx::EntityId{};
    ew.w.fire_kill_events(*hen);
    CHECK(ew.kill_script("Any") == 2.0);
}

TEST_CASE("the events live in the role data and the kill of a fight fires them at the end of the death frames", "[event][world]")
{
    jx::pb::RoleData snapshot;
    {
        EventWorld ew;
        ew.A().player.events.add(1);
        ew.A().player.events.add(2, 4);
        REQUIRE(ew.w.role_snapshot(7, snapshot));
    }
    REQUIRE(snapshot.events_size() == 2);
    CHECK(snapshot.events(0).id() == 1);
    CHECK(snapshot.events(1).id() == 2);
    CHECK(snapshot.events(1).count() == 4);
    EventWorld ew(snapshot);
    REQUIRE(ew.A().player.events.entries.size() == 2);
    CHECK(ew.A().player.events.find(2)->count == 4);
    // a real kill: A hits the wolf for more than its life; the death frames run out, the event counts (0x08083720)
    KNpc* wolf = ew.w.mutable_entity(ew.wolf);
    REQUIRE(wolf != nullptr);
    wolf->cur.life = 10;
    std::array<KMagicAttrib, jx::zone::kSkillAttribs> d{};
    d[1] = attrib(jx::zone::magic_attackrating_v, 100000);
    d[3] = attrib(jx::zone::magic_physicsdamage_v, 5000, 0, 5000);
    REQUIRE(ew.w.receive_damage(*wolf, ew.A(), -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    for (int i = 0; i < 200 && ew.A().player.events.find(1) != nullptr && ew.A().player.events.find(1)->count == 0; ++i) ew.w.tick();
    REQUIRE(ew.A().player.events.find(1) != nullptr);
    CHECK(ew.A().player.events.find(1)->count == 1);
    CHECK(ew.A().player.task.get_save_val(300) == 1);
}
