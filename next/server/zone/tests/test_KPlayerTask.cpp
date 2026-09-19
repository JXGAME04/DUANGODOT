// The task values of the JX2 server (docs/LINUX-SERVER.md §21): KPlayerTask at Player+0x809c (GetSaveVal 0x080CB540 /
// SetSaveVal 0x080CB720, the temp array 0x080CB5A0 / 0x080CB5C0, the bits 0x080CB5E0 / 0x080CB910), the table of
// settings/task/player_task_def.txt (0x081C6E00), KPlayer::SetTaskValue 0x080A9190 and the 0xa7 / 0xb5 packets, the
// script api GetTask / SetTask / GetTaskTemp / SetTaskTemp / SyncTaskValue / SyncTaskValueMore / GetBitTask / SetBitTask,
// the enter-world sync 0x080B9CF0, the client's 0xaa packet 0x080DB070, the {id, value} pairs of the role data.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KPlayerTask.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KNpc;
using jx::zone::KPlayerTask;
using jx::zone::KScriptCache;
using jx::zone::KTaskDefRow;
using jx::zone::KTaskDefTable;

namespace {

constexpr const char* kTaskScript = R"(
function main(param)
    SetTask(5, 77)
    SetTask(5, 77)
    SetTask(1, 3)
    SetTaskTemp(2, 9)
    g_temp = GetTaskTemp(2)
    g_nil = GetTaskTemp(300)
    g_more = SyncTaskValueMore(1000, 1200, 1)
    g_bits_ok = SetBitTask(50, 4, 3, 5)
    g_bits = GetBitTask(50, 4, 3)
    g_val50 = GetTask(50)
    SyncTaskValue(5)
end
function GetV(id)
    return GetTask(id)
end
function SetV(id, v)
    SetTask(id, v)
end
function Fill(first, last)
    for id = first, last do
        SetTask(id, id)
    end
end
function More(first, last, only)
    return SyncTaskValueMore(first, last, only)
end
function MoreTwo(first, last)
    return SyncTaskValueMore(first, last)
end
function Temp()
    return g_temp
end
function NilTemp()
    if g_nil == nil then
        return 1
    end
    return 0
end
function MoreResult()
    return g_more
end
function Bits()
    return g_bits
end
function BitsOk()
    return g_bits_ok
end
function V50()
    return g_val50
end
function SetTempTop(a, b, c)
    SetTaskTemp(a, b, c)
end
function GetTempTop(a, b)
    return GetTaskTemp(a, b)
end
)";

// the rows: 5 alone (sync); 40..60 (sync); 1276..1277 (sync + client); a first of 0 (skipped); 100 (client only);
// 41 without a flag (nothing); 50 client only (its sync flag of the range overwritten); 40..45 again (the range of 40
// is kept as 40..60, the flags of 40..45 written again); 1000..1070 with the flags as numbers 0 (nothing)
constexpr const char* kTaskDefJson = R"({"source": "test", "rows": [
 {"first": 5, "last": 0, "name": "nam", "sync": true, "client": false},
 {"first": 40, "last": 60, "sync": true, "client": false},
 {"first": 1276, "last": 1277, "sync": true, "client": true},
 {"first": 0, "last": 9, "sync": true, "client": true},
 {"first": 100, "last": 0, "sync": false, "client": true},
 {"first": 41, "last": 41, "sync": false, "client": false},
 {"first": 50, "last": 50, "sync": false, "client": true},
 {"first": 40, "last": 45, "sync": true, "client": false},
 {"first": 1000, "last": 1070, "sync": 0, "client": 0}
]})";

std::string make_files()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_task_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "task.lua") << kTaskScript;
    std::ofstream(root / "task_def.json") << kTaskDefJson;
    return root.string();
}

jx::zone::KSubWorldConfig task_world(const std::string& root)
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
    auto t = KTaskDefTable::load((std::filesystem::path(root) / "task_def.json").string(), &error);
    REQUIRE(t.has_value());
    c.task_def = std::make_shared<const KTaskDefTable>(std::move(*t));
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
    r.set_level(10);
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

std::vector<jx::pb::TaskValues> batches_of(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::TaskValues> out;
    for (const auto& p : all) {
        if (p.msg_id != jx::pb::G2C_TASK_VALUES || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
        jx::pb::TaskValues v;
        REQUIRE(v.ParseFromString(p.payload));
        out.push_back(v);
    }
    return out;
}

// A (sid 7) at the spawn with the test script and the table above; the outbox of the spawn is kept for the login test
struct TaskWorld {
    std::string root;
    jx::zone::KSubWorld w;
    jx::EntityId a;
    explicit TaskWorld(const jx::pb::RoleData& role = role_of(1, "A", 2000, 2000)) : root(make_files()), w(task_world(root))
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role, a, at) == jx::pb::RESULT_OK);
        w.tick();
        w.tick();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    bool run(const char* fn, int param = 0) { return w.execute_script(R"(\script\test\task.lua)", fn, A(), param); }
    double call(const char* fn, const std::vector<jx::zone::KLuaScript::Arg>& args = {})
    {
        // through the world so the script api has its player
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\task.lua)");
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
};

} // namespace

TEST_CASE("the constants of the task values follow the binary", "[task]")
{
    CHECK(jx::zone::kTaskValueCount == 0x1770);
    CHECK(jx::zone::kTaskTempCount == 256);
    CHECK(jx::zone::kTaskSyncMoreBatch == 80);
    CHECK(jx::zone::kTaskTraceId == 1);
    CHECK(jx::zone::kTaskWayPointBegin == 201);
    CHECK(jx::zone::kTaskWayPointCount == 3);
    CHECK(jx::zone::kTaskStationBegin == 210);
    CHECK(jx::zone::kTaskStationCount == 10);
    CHECK(jx::zone::kTaskDisabledTeam == 0x87);
    CHECK(jx::zone::kTaskTeamFlag == 0xb41);
    CHECK(jx::zone::kTaskLoginSyncFirst == 1000);
    CHECK(jx::zone::kTaskLoginSyncLast == 1070);
}

TEST_CASE("KPlayerTask: the saved map 0x080CB540 / 0x080CB720 and the temp array 0x080CB5A0 / 0x080CB5C0", "[task]")
{
    KPlayerTask t;
    CHECK(t.get_save_val(5) == 0);
    t.set_save_val(5, 77);
    CHECK(t.get_save_val(5) == 77);
    t.set_save_val(5, -4);
    CHECK(t.get_save_val(5) == -4);
    // a zero erases the node (0x080CB7B8)
    t.set_save_val(5, 0);
    CHECK(t.get_save_val(5) == 0);
    CHECK(t.saved.count(5) == 0);
    // the ids stop at 0x176f; a negative one is a big unsigned one
    t.set_save_val(0x176f, 1);
    CHECK(t.get_save_val(0x176f) == 1);
    t.set_save_val(0x1770, 1);
    CHECK(t.get_save_val(0x1770) == 0);
    CHECK(t.saved.count(0x1770) == 0);
    t.set_save_val(-1, 1);
    CHECK(t.get_save_val(-1) == 0);
    CHECK(t.saved.size() == 1);
    // the temp values: 0..0xff
    t.set_temp(0, 3);
    t.set_temp(255, 4);
    t.set_temp(256, 5);
    t.set_temp(-1, 6);
    CHECK(t.get_temp(0) == 3);
    CHECK(t.get_temp(255) == 4);
    CHECK(t.get_temp(256) == 0);
    CHECK(t.get_temp(-1) == 0);
    // Serialize 0x080CB6A0: in id order, no zeros
    t.set_save_val(9, 2);
    t.set_save_val(3, 1);
    const auto pairs = t.serialize();
    REQUIRE(pairs.size() == 3);
    CHECK(pairs[0] == std::pair<int, int>{3, 1});
    CHECK(pairs[1] == std::pair<int, int>{9, 2});
    CHECK(pairs[2] == std::pair<int, int>{0x176f, 1});
    // Release: everything gone
    t.release();
    CHECK(t.saved.empty());
    CHECK(t.get_temp(0) == 0);
}

TEST_CASE("GetBits 0x080CB5E0 / SetBits 0x080CB910: the bits of a value", "[task]")
{
    CHECK(KPlayerTask::bit_mask(4, 3) == 0x70u);
    CHECK(KPlayerTask::bit_mask(0, 32) == 0xffffffffu);
    CHECK(KPlayerTask::bit_mask(31, 1) == 0x80000000u);
    KPlayerTask t;
    // a value that is not there reads 0 and takes the bits alone
    CHECK(t.get_bits(50, 4, 3) == 0);
    CHECK(t.set_bits(50, 4, 3, 5));
    CHECK(t.get_save_val(50) == 0x50);
    CHECK(t.get_bits(50, 4, 3) == 5u);
    // the other bits are kept; the value is cut to `count` bits
    CHECK(t.set_bits(50, 0, 2, 7));
    CHECK(t.get_save_val(50) == 0x53);
    CHECK(t.get_bits(50, 0, 8) == 0x53u);
    // the top bit reads as an unsigned number
    CHECK(t.set_bits(50, 31, 1, 1));
    CHECK(t.get_bits(50, 31, 1) == 1u);
    CHECK(t.get_bits(50, 0, 32) == 0x80000053u);
    // the checks: count > 0, start >= 0, start + count <= 32, the id within the table
    CHECK_FALSE(t.set_bits(50, 30, 3, 1));
    CHECK_FALSE(t.set_bits(50, 0, 0, 1));
    CHECK_FALSE(t.set_bits(50, -1, 2, 1));
    CHECK_FALSE(t.set_bits(0x1770, 0, 1, 1));
    CHECK(t.get_bits(50, 30, 3) == 0);
    CHECK(t.get_bits(50, 0, 0) == 0);
    CHECK(t.get_save_val(50) == static_cast<int>(0x80000053u));
    // clearing all the bits erases the value like a SetSaveVal(0)
    CHECK(t.set_bits(50, 0, 32, 0));
    CHECK(t.saved.count(50) == 0);
}

TEST_CASE("ClearRange 0x080CBC40: first..first+count-1, cut at the table's end", "[task]")
{
    KPlayerTask t;
    for (int id = 40; id <= 60; ++id) t.set_save_val(id, id);
    t.set_save_val(0x176f, 9);
    t.clear_range(45, 10);
    CHECK(t.get_save_val(44) == 44);
    CHECK(t.get_save_val(45) == 0);
    CHECK(t.get_save_val(54) == 0);
    CHECK(t.get_save_val(55) == 55);
    t.clear_range(0x1760, 1000);
    CHECK(t.get_save_val(0x176f) == 0);
    t.clear_range(0x1770, 5);   // outside: nothing
    t.clear_range(60, 0);       // an empty range: nothing
    CHECK(t.get_save_val(60) == 60);
    CHECK(t.saved.size() == 11);
}

TEST_CASE("the table of player_task_def.txt (0x081C6E00): the ranges of the SYNC_FLAG rows and the flags of every id", "[task]")
{
    const std::string root = make_files();
    std::string error;
    auto t = KTaskDefTable::load((std::filesystem::path(root) / "task_def.json").string(), &error);
    REQUIRE(t.has_value());
    CHECK(t->source == "test");
    // the ranges keyed by their first id: 5, 40 (the later 40..45 row does not replace 40..60), 1276
    REQUIRE(t->sync_ranges().size() == 3);
    CHECK(t->sync_ranges().at(5).last == 5);
    CHECK(t->sync_ranges().at(40).last == 60);
    CHECK(t->sync_ranges().at(1276).last == 1277);
    // the flags: 5 sync; 40..60 sync but 50 (client only, the last row of it); 41 kept (a row without flags leaves nothing);
    // 100 client; 1276..1277 both; a first of 0 skipped; numbers 0 are no flags
    CHECK(t->size() == 1 + 21 + 1 + 2);
    CHECK(t->synced(5));
    CHECK_FALSE(t->client_may_set(5));
    CHECK(t->synced(40));
    CHECK(t->synced(41));
    CHECK(t->synced(60));
    CHECK_FALSE(t->synced(61));
    CHECK_FALSE(t->synced(50));
    CHECK(t->client_may_set(50));
    CHECK(t->client_may_set(100));
    CHECK_FALSE(t->synced(100));
    CHECK(t->flags(1276) == 3u);
    CHECK(t->flags(1277) == 3u);
    CHECK(t->flags(0) == 0u);
    CHECK(t->flags(9) == 0u);
    CHECK(t->flags(1000) == 0u);
    // add() by hand: a last below the first keeps the range and marks no id
    KTaskDefTable u;
    u.add(KTaskDefRow{9, 3, true, false, ""});
    CHECK(u.sync_ranges().size() == 1);
    CHECK(u.size() == 0);
    u.add(KTaskDefRow{0x1760, 0x2000, true, true, ""});   // ids past the table are left out
    CHECK(u.size() == 16);
    CHECK(u.sync_ranges().at(0x1760).last == 0x2000);
    // a missing file, a file without rows
    CHECK_FALSE(KTaskDefTable::load((std::filesystem::path(root) / "nothing.json").string(), &error).has_value());
    CHECK(error.find("cannot open") == 0);
    std::ofstream(std::filesystem::path(root) / "empty.json") << "{\"source\": \"x\"}";
    CHECK_FALSE(KTaskDefTable::load((std::filesystem::path(root) / "empty.json").string(), &error).has_value());
    CHECK(error == "no rows");
}

TEST_CASE("SetTaskValue 0x080A9190 and the script api: GetTask / SetTask / GetTaskTemp / SetTaskTemp / SyncTaskValue / GetBitTask / SetBitTask", "[task][world]")
{
    TaskWorld tw;
    tw.w.take_outbox();
    REQUIRE(tw.run("main"));
    KPlayerTask& t = tw.A().player.task;
    CHECK(t.get_save_val(5) == 77);
    CHECK(t.get_save_val(1) == 3);
    CHECK(t.get_temp(2) == 9);
    CHECK(tw.call("Temp") == 9.0);
    CHECK(tw.call("NilTemp") == 1.0);       // GetTaskTemp above 0xff -> nil (0x08123A5F)
    CHECK(tw.call("MoreResult") == 1.0);    // the range is good, every value 0 -> nothing sent, still 1
    CHECK(tw.call("BitsOk") == 1.0);
    CHECK(tw.call("Bits") == 5.0);
    CHECK(tw.call("V50") == 80.0);          // 5 << 4
    // the packets: SetTask(5, 77) once (the second one changes nothing), SetTask(1, 3) is not a synced id, SetBitTask
    // never syncs, SyncTaskValue(5) sends it again
    const auto out = tw.w.take_outbox();
    const auto values = values_of(out, 7);
    REQUIRE(values.size() == 2);
    CHECK(values[0].id() == 5);
    CHECK(values[0].value() == 77);
    CHECK(values[1].id() == 5);
    CHECK(values[1].value() == 77);
    CHECK(batches_of(out, 7).empty());
    // GetTask / SetTask outside the table: 0 and nothing
    CHECK(tw.call("GetV", {9999.0}) == 0.0);
    tw.call("SetV", {9999.0, 1.0});
    tw.call("SetV", {-1.0, 5.0});
    CHECK(t.saved.count(9999) == 0);
    CHECK(t.saved.size() == 3);
    // the last two arguments are the id and the value of SetTaskTemp (Lua_GetTopIndex of 2003), the last one of GetTaskTemp
    tw.call("SetTempTop", {1.0, 2.0, 3.0});
    CHECK(t.get_temp(2) == 3);
    CHECK(tw.call("GetTempTop", {9.0, 2.0}) == 3.0);
    // 41 kept its sync flag, 50 lost it to the client-only row
    tw.call("SetV", {41.0, 1.0});
    tw.call("SetV", {50.0, 1.0});
    const auto again = values_of(tw.w.take_outbox(), 7);
    REQUIRE(again.size() == 1);
    CHECK(again[0].id() == 41);
    CHECK(again[0].value() == 1);
    // a script without a player: GetTask is nil, the others do nothing
    jx::zone::KLuaScript* s = tw.w.config().scripts->get(R"(\script\test\task.lua)");
    REQUIRE(s != nullptr);
    jx::zone::KScriptContext& ctx = jx::zone::g_ScriptContext();
    const jx::zone::KScriptContext saved = ctx;
    ctx = jx::zone::KScriptContext{};
    CHECK_FALSE(s->call_number("GetV", {5.0}).has_value());
    ctx = saved;
}

TEST_CASE("SyncTaskValueMore 0x080A9550: eighty pairs a packet, the zeros left out on request, a bad range -> 0", "[task][world]")
{
    TaskWorld tw;
    tw.w.take_outbox();
    tw.call("Fill", {1000.0, 1100.0});
    CHECK(values_of(tw.w.take_outbox(), 7).empty());   // none of them is a synced id
    CHECK(tw.call("More", {1000.0, 1100.0, 1.0}) == 1.0);
    auto batches = batches_of(tw.w.take_outbox(), 7);
    REQUIRE(batches.size() == 2);
    CHECK(batches[0].values_size() == 80);
    CHECK(batches[1].values_size() == 21);
    CHECK(batches[0].values(0).id() == 1000);
    CHECK(batches[0].values(0).value() == 1000);
    CHECK(batches[1].values(20).id() == 1100);
    // with the zeros: 1000..1200 = 201 pairs in three packets
    CHECK(tw.call("More", {1000.0, 1200.0, 0.0}) == 1.0);
    batches = batches_of(tw.w.take_outbox(), 7);
    REQUIRE(batches.size() == 3);
    CHECK(batches[2].values_size() == 41);
    CHECK(batches[2].values(40).id() == 1200);
    CHECK(batches[2].values(40).value() == 0);
    // two arguments: the zeros stay in
    CHECK(tw.call("MoreTwo", {1101.0, 1102.0}) == 1.0);
    batches = batches_of(tw.w.take_outbox(), 7);
    REQUIRE(batches.size() == 1);
    CHECK(batches[0].values_size() == 2);
    // a bad range: first > last, an end past the table, a negative one
    CHECK(tw.call("More", {5.0, 3.0, 1.0}) == 0.0);
    CHECK(tw.call("More", {0.0, 6000.0, 1.0}) == 0.0);
    CHECK(tw.call("More", {-1.0, 3.0, 1.0}) == 0.0);
    CHECK(batches_of(tw.w.take_outbox(), 7).empty());
    // the direct call with only the zeros left out and nothing to send still returns true
    CHECK(tw.w.task_sync_more(tw.A(), 2000, 2100, true));
    CHECK(batches_of(tw.w.take_outbox(), 7).empty());
}

TEST_CASE("the role data (LoadPlayerTaskList 0x080C0050 / SavePlayerTaskList 0x080BF1C0) and the enter-world sync 0x080B9CF0", "[task][world]")
{
    jx::pb::RoleData snapshot;
    {
        TaskWorld tw;
        tw.call("Fill", {1000.0, 1100.0});
        tw.call("SetV", {5.0, 77.0});
        tw.call("SetV", {1276.0, -2.0});
        tw.A().player.task.set_temp(3, 9);   // the temp values are not saved
        REQUIRE(tw.w.role_snapshot(7, snapshot));
    }
    REQUIRE(snapshot.task_values_size() == 101 + 2);
    CHECK(snapshot.task_values(0).id() == 5);
    CHECK(snapshot.task_values(0).value() == 77);
    CHECK(snapshot.task_values(1).id() == 1000);
    CHECK(snapshot.task_values(102).id() == 1276);
    CHECK(snapshot.task_values(102).value() == -2);
    // an id past the table in the record is dropped at the load
    auto* bad = snapshot.add_task_values();
    bad->set_id(9999);
    bad->set_value(1);
    TaskWorld tw2(snapshot);
    const KPlayerTask& t = tw2.A().player.task;
    CHECK(t.get_save_val(5) == 77);
    CHECK(t.get_save_val(1050) == 1050);
    CHECK(t.get_save_val(1276) == -2);
    CHECK(t.saved.size() == 103);
    CHECK(t.get_temp(3) == 0);
    // the login: every id of the ranges 5, 40..60, 1276..1277 as a 0xa7 (24 of them), then 1000..1070 without the zeros
    const auto out = tw2.w.take_outbox();
    const auto values = values_of(out, 7);
    REQUIRE(values.size() == 24);
    CHECK(values[0].id() == 5);
    CHECK(values[0].value() == 77);
    CHECK(values[1].id() == 40);
    CHECK(values[1].value() == 0);
    CHECK(values[21].id() == 60);
    CHECK(values[22].id() == 1276);
    CHECK(values[22].value() == -2);
    CHECK(values[23].id() == 1277);
    const auto batches = batches_of(out, 7);
    REQUIRE(batches.size() == 1);
    CHECK(batches[0].values_size() == 71);
    CHECK(batches[0].values(0).id() == 1000);
    CHECK(batches[0].values(70).id() == 1070);
}

TEST_CASE("the client's 0xaa packet 0x080DB070: a CLIENT_FLAG id is set without a sync back, any other refused", "[task][world]")
{
    TaskWorld tw;
    tw.w.take_outbox();
    CHECK(tw.w.task_value_request(7, 100, 9));
    CHECK(tw.A().player.task.get_save_val(100) == 9);
    CHECK(tw.w.task_value_request(7, 1276, 4));   // sync + client: set, but bSync = 0 -> no 0xa7
    CHECK(tw.A().player.task.get_save_val(1276) == 4);
    CHECK_FALSE(tw.w.task_value_request(7, 5, 1));
    CHECK(tw.A().player.task.get_save_val(5) == 0);
    CHECK_FALSE(tw.w.task_value_request(7, 41, 1));
    CHECK_FALSE(tw.w.task_value_request(99, 100, 1));
    CHECK(values_of(tw.w.take_outbox(), 7).empty());
}
