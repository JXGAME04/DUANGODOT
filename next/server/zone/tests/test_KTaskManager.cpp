// The task system of the JX2 server (docs/LINUX-SERVER.md §22): the tables of settings/task (the loader 0x08170990,
// task_id.txt 0x08171390, task_type.txt 0x08172030), the two status bits of a task in the values from 2000 (0x0820E800 /
// 0x0820E720), the temp values serialized into 2200..2299 (0x0820DF90 / 0x0820E250, the key hash 0x0821DF00), StartTask /
// CloseTask / FirstTask / NextTask, the TASKSYS library of the scripts.
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
#include "jx/zone/KTaskManager.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KNpc;
using jx::zone::KPlayerTask;
using jx::zone::KScriptCache;
using jx::zone::KTaskManager;
using jx::zone::KTaskTable;
using jx::zone::KTaskType;
namespace ts = jx::zone::task_status;

namespace {

// task_id.txt as the exporter writes it: the description row first (id 0), then the tasks; the cells after the key
constexpr const char* kTablesJson = R"({"source": "test",
 "tasks": [
  {"id": 0, "name": "Ten nhiem vu", "event": 0, "type": "Loai", "cells": ["Ten nhiem vu", "ID su kien", "Loai", "co the huy", "Mieu ta"]},
  {"id": 101, "name": "Doi thoai_A", "event": 3, "type": "Doi thoai", "cells": ["Doi thoai_A", "3", "Doi thoai", "0", "Tang hoa"]},
  {"id": 102, "name": "Thu thap_B", "event": 3, "type": "Thu thap", "cells": ["Thu thap_B", "3", "Thu thap", "1", "Giao ¸"]},
  {"id": 103, "name": "Sat quai_C", "event": 1, "type": "Sat quai", "cells": ["Sat quai_C", "1", "Sat quai", "0", "Giet"]}
 ],
 "events": [{"key": "1", "cells": ["Phuong thuc test", "thu"]}, {"key": "3", "cells": ["Nhiem vu ban", "ngau nhien"]}],
 "types": [
  {"name": "Doi thoai",
   "condition": [{"key": "Doi thoai_A", "cells": ["dang cap lon", "5"]}, {"key": "Doi thoai_A", "cells": ["vat pham", "1,2,3"]}],
   "entity": [{"key": "Doi thoai_A", "cells": ["doi thoai", ""]}],
   "award": [],
   "talk": [{"key": "Doi thoai_A", "cells": ["<dec>xin chao"]}]},
  {"name": "Thu thap", "condition": [], "entity": [{"key": "Thu thap_B", "cells": ["thu thap"]}], "award": [], "talk": []}
 ]})";

constexpr const char* kTaskScript = R"(
function main(param)
    g_first = FirstTask()
    g_started = StartTask("Doi thoai_A")
    g_again = StartTask("Doi thoai_A")
    g_status0 = GetTaskStatus("Doi thoai_A")
    g_set = SetTaskStatus("Doi thoai_A", 3)
    g_status = GetTaskStatus("Doi thoai_A")
    g_tmp_nil = GetTmpValue("Doi thoai_A", "TalkNpc")
    g_tmp_set = SetTmpValue("Doi thoai_A", "TalkNpc", 7)
    g_tmp = GetTmpValue("Doi thoai_A", "TalkNpc")
    g_name = TaskName(102)
    g_no = TaskNo("Thu thap_B")
    g_rows, g_cols = TaskConditionMatrix("Doi thoai_A")
    g_cond = TaskCondition("Doi thoai_A", 2, 2)
    g_ent = TaskEntity("Doi thoai_A", 1, 1)
    g_talk = TaskTalk("Doi thoai_A", 1, 1)
    g_id_cell = TaskId(TaskNo("Thu thap_B"), 1, 5)
    g_ev = TaskEvent(3, 1, 1)
    g_evid = GetTaskEventID("Sat quai_C")
    g_evcount = GetEventTaskCount(3)
    g_evtask = GetEventTask(3, 1)
    g_world = SubWorldName(1)
end
function Walk()
    local n = 0
    local name = FirstTask()
    g_walk = ""
    while name ~= nil do
        n = n + 1
        g_walk = g_walk .. name .. ";"
        name = NextTask()
    end
    return n
end
function Start(name)
    return StartTask(name)
end
function Close(name)
    return CloseTask(name)
end
function Status(name)
    local s = GetTaskStatus(name)
    if s == nil then return -1 end
    return s
end
function SetStatus(name, s)
    return SetTaskStatus(name, s)
end
function Tmp(name, key)
    local v = GetTmpValue(name, key)
    if v == nil then return -1 end
    return v
end
function SetTmp(name, key, v)
    return SetTmpValue(name, key, v)
end
function Select(id)
    SelectTaskStart(id)
end
function Str(v)
    if v == nil then return "nil" end
    return tostring(v)
end
function Get(name)
    return _G[name]
end
function GetStr(name)
    return Str(_G[name])
end
function Is(name, expected)
    if Str(_G[name]) == expected then return 1 end
    return 0
end
)";

constexpr const char* kTaskFunction = R"(
function OnMenuTaskStart(taskid)
    g_menu = taskid
end
function MenuSeen()
    if g_menu == nil then return -1 end
    return g_menu
end
)";

std::string make_files()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_tasksys_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::filesystem::create_directories(root / "script" / "task" / "system");
    std::ofstream(root / "script" / "test" / "task.lua") << kTaskScript;
    std::ofstream(root / "script" / "task" / "system" / "task_function.lua") << kTaskFunction;
    std::ofstream(root / "task_tables.json") << kTablesJson;
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
    auto t = KTaskManager::load((std::filesystem::path(root) / "task_tables.json").string(), &error);
    REQUIRE(t.has_value());
    c.tasks = std::make_shared<const KTaskManager>(std::move(*t));
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

struct TaskWorld {
    std::string root;
    jx::zone::KSubWorld w;
    jx::EntityId a;
    TaskWorld() : root(make_files()), w(task_world(root))
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
        w.tick();
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    bool run(const char* fn, int param = 0) { return w.execute_script(R"(\script\test\task.lua)", fn, A(), param); }
    double call(const char* fn, const std::vector<jx::zone::KLuaScript::Arg>& args = {})
    {
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
    // a global of the script against a text, compared inside Lua (nil -> "nil")
    bool is(const char* name, const char* expected)
    {
        return call("Is", {std::string(name), std::string(expected)}) == 1.0;
    }
};

} // namespace

TEST_CASE("the constants of the task system follow the binary", "[tasksys]")
{
    CHECK(ts::kStatusValueFirst == 2000);
    CHECK(ts::kTempCountId == 2200);
    CHECK(ts::kTempFirst == 2201);
    CHECK(ts::kTempLast == 2299);
    CHECK(ts::kTempSlots == 98);
}

TEST_CASE("the key hash 0x0821DF00 of the temp value names", "[tasksys]")
{
    CHECK(ts::key_hash("TalkNpc") == 0xfa7e2d03u);
    CHECK(ts::key_hash("KillNpc") == 0xc947adf0u);
    CHECK(ts::key_hash("Collect") == 0xa26cb6dcu);
    CHECK(ts::key_hash("ItemNpc") == 0x3fe53c11u);
    CHECK(ts::key_hash("a") == 0xedcbaff7u);
    CHECK(ts::key_hash("") == 0x12345678u);
}

TEST_CASE("the two status bits of a task: value 2000 + ordinal / 16, bits 31 - 2k and 30 - 2k", "[tasksys]")
{
    CHECK(ts::value_id(0) == 2000);
    CHECK(ts::value_id(15) == 2000);
    CHECK(ts::value_id(16) == 2001);
    CHECK(ts::value_id(107) == 2006);
    CHECK(ts::hi_bit(0) == 0x80000000u);
    CHECK(ts::lo_bit(0) == 0x40000000u);
    CHECK(ts::hi_bit(1) == 0x20000000u);
    CHECK(ts::lo_bit(15) == 1u);
    CHECK(ts::hi_bit(16) == 0x80000000u);
    KPlayerTask t;
    CHECK(ts::status_of(t, 5) == 0);
    int v = ts::status_value(0, 5, 3);
    CHECK(v == static_cast<int>(0x00300000u));   // bits 21 and 20
    t.set_save_val(2000, v);
    CHECK(ts::status_of(t, 5) == 3);
    CHECK(ts::status_of(t, 4) == 0);
    CHECK(ts::status_of(t, 6) == 0);
    v = ts::status_value(v, 5, 2);
    CHECK(v == static_cast<int>(0x00200000u));
    t.set_save_val(2000, v);
    CHECK(ts::status_of(t, 5) == 2);
    v = ts::status_value(v, 5, 1);
    CHECK(v == static_cast<int>(0x00100000u));
    v = ts::status_value(v, 5, 0);
    CHECK(v == 0);
    // another task in the same value keeps its bits; the seventeenth task lives in the next value
    v = ts::status_value(ts::status_value(0, 0, 1), 15, 2);
    CHECK(v == static_cast<int>(0x40000002u));
    t.set_save_val(2001, ts::status_value(0, 16, 3));
    CHECK(ts::status_of(t, 16) == 3);
    CHECK(ts::status_of(t, 0) == 0);
}

TEST_CASE("the temp values: decode 0x0820DF90 / encode 0x0820E250 in the task values 2200..2299", "[tasksys]")
{
    KPlayerTask t;
    ts::KTaskTemp temp;
    CHECK(temp.decode(t));
    CHECK(temp.groups.empty());
    CHECK(temp.slots() == 0);
    // two groups: 101 with two pairs, 102 empty; the encode: 101, 2, k1, v1, k2, v2, 102, 0, then the count, then zeros
    temp.groups[102];
    temp.groups[101][ts::key_hash("TalkNpc")] = 7;
    temp.groups[101][ts::key_hash("Collect")] = 1;
    CHECK(temp.slots() == 2 + 4 + 2);
    const auto writes = temp.encode();
    REQUIRE(writes.size() >= 9);
    CHECK(writes[0] == std::pair<int, int>{2201, 101});
    CHECK(writes[1] == std::pair<int, int>{2202, 2});
    CHECK(writes[2] == std::pair<int, int>{2203, static_cast<int>(ts::key_hash("Collect"))});   // the smaller key first
    CHECK(writes[3] == std::pair<int, int>{2204, 1});
    CHECK(writes[4] == std::pair<int, int>{2205, static_cast<int>(ts::key_hash("TalkNpc"))});
    CHECK(writes[5] == std::pair<int, int>{2206, 7});
    CHECK(writes[6] == std::pair<int, int>{2207, 102});
    CHECK(writes[7] == std::pair<int, int>{2208, 0});
    CHECK(writes[8] == std::pair<int, int>{2200, 2});
    CHECK(writes[9] == std::pair<int, int>{2209, 0});
    CHECK(writes.back() == std::pair<int, int>{2299, 0});
    CHECK(writes.size() == 9 + (2299 - 2209 + 1));
    for (const auto& [id, value] : writes) t.set_save_val(id, value);
    ts::KTaskTemp back;
    CHECK(back.decode(t));
    REQUIRE(back.groups.size() == 2);
    CHECK(back.groups.at(101).at(ts::key_hash("TalkNpc")) == 7);
    CHECK(back.groups.at(101).at(ts::key_hash("Collect")) == 1);
    CHECK(back.groups.at(102).empty());
    // a group that runs past the values (0x0820E00C): what was read before it stays, false comes back
    t.set_save_val(2200, 3);
    t.set_save_val(2207, 103);
    t.set_save_val(2208, 60);
    ts::KTaskTemp bad;
    CHECK_FALSE(bad.decode(t));
    CHECK(bad.groups.size() == 1);
    CHECK(bad.groups.count(101) == 1);
}

TEST_CASE("the tables of settings/task (0x08170990 / 0x08171390 / 0x08172030) as KTaskManager", "[tasksys]")
{
    const std::string root = make_files();
    std::string error;
    auto m = KTaskManager::load((std::filesystem::path(root) / "task_tables.json").string(), &error);
    REQUIRE(m.has_value());
    CHECK(m->source == "test");
    CHECK(m->task_count() == 4);
    CHECK(m->type_count() == 2);
    CHECK(m->event_count() == 2);
    // the ordinal is the row from 0: the description row is 0, the first task 1
    CHECK(m->ordinal_of("Ten nhiem vu") == 0);
    CHECK(m->ordinal_of("Doi thoai_A") == 1);
    CHECK(m->ordinal_of("Sat quai_C") == 3);
    CHECK_FALSE(m->ordinal_of("nobody").has_value());
    CHECK(m->id_of("Thu thap_B") == 102);
    CHECK(std::string(m->name_of(103)) == "Sat quai_C");
    CHECK(m->name_of(999) == nullptr);
    CHECK(m->event_of("Sat quai_C") == 1);
    CHECK(m->event_task_count(3) == 2);
    CHECK(m->event_task_count(9) == 0);
    REQUIRE(m->event_task(3, 0) != nullptr);
    CHECK(*m->event_task(3, 0) == "Doi thoai_A");
    CHECK(*m->event_task(3, 1) == "Thu thap_B");
    CHECK(m->event_task(3, 2) == nullptr);
    CHECK(m->event_task(3, -1) == nullptr);
    // the matrices: rows x cols without the key column, cells from 0 here (the Lua glue takes 1 off)
    const auto* cond = m->condition("Doi thoai_A");
    REQUIRE(cond != nullptr);
    CHECK(cond->row_count() == 2);
    CHECK(cond->cols == 2);
    CHECK(*cond->cell(1, 1) == "1,2,3");
    CHECK(*cond->cell(0, 0) == "dang cap lon");
    CHECK(cond->cell(2, 0) == nullptr);
    CHECK(cond->cell(0, 2) == nullptr);
    CHECK(cond->cell(-1, 0) == nullptr);
    CHECK(m->condition("Thu thap_B") == nullptr);   // the kind has no condition rows
    CHECK(m->entity("Thu thap_B") != nullptr);
    CHECK(m->award("Doi thoai_A") == nullptr);
    CHECK(*m->talk("Doi thoai_A")->cell(0, 0) == "<dec>xin chao");
    CHECK(m->condition("nobody") == nullptr);
    // the id and event tables: the Latin-1 rune of the JSON is the byte of the file again
    const auto* id = m->id_matrix("102");
    REQUIRE(id != nullptr);
    CHECK(id->cols == 5);
    CHECK(*id->cell(0, 3) == "1");
    CHECK(*id->cell(0, 4) == "Giao \xb8");
    CHECK(m->id_matrix("Thu thap_B") == nullptr);
    CHECK(*m->event_matrix("3")->cell(0, 0) == "Nhiem vu ban");
    // a table with two rows of one key keeps their order; a missing file, a file without tasks
    KTaskTable t;
    t.add("k", {"a"});
    t.add("k", {"b", "c"});
    CHECK(t.find("k")->row_count() == 2);
    CHECK(t.find("k")->cols == 2);
    CHECK(t.find("k")->cell(0, 1) == nullptr);   // the short row has no second cell
    CHECK_FALSE(KTaskManager::load((std::filesystem::path(root) / "nothing.json").string(), &error).has_value());
    std::ofstream(std::filesystem::path(root) / "empty.json") << "{\"source\": \"x\"}";
    CHECK_FALSE(KTaskManager::load((std::filesystem::path(root) / "empty.json").string(), &error).has_value());
    CHECK(error == "no tasks");
}

TEST_CASE("the TASKSYS library on a player: StartTask / CloseTask / FirstTask / NextTask / GetTaskStatus / SetTaskStatus / GetTmpValue / SetTmpValue / the matrices", "[tasksys][world]")
{
    TaskWorld tw;
    REQUIRE(tw.run("main"));
    CHECK(tw.is("g_first", "nil"));        // no task yet
    CHECK(tw.is("g_started", "1"));
    CHECK(tw.is("g_again", "1"));          // StartTask pushes 1 whatever happened (0x081749B2); the group is not doubled
    CHECK(tw.is("g_status0", "0"));
    CHECK(tw.is("g_set", "1"));
    CHECK(tw.is("g_status", "3"));
    CHECK(tw.is("g_tmp_nil", "nil"));
    CHECK(tw.is("g_tmp_set", "1"));
    CHECK(tw.is("g_tmp", "7"));
    CHECK(tw.is("g_name", "Thu thap_B"));
    CHECK(tw.is("g_no", "102"));
    CHECK(tw.is("g_rows", "2"));
    CHECK(tw.is("g_cols", "2"));
    CHECK(tw.is("g_cond", "1,2,3"));
    CHECK(tw.is("g_ent", "doi thoai"));
    CHECK(tw.is("g_talk", "<dec>xin chao"));
    CHECK(tw.is("g_id_cell", "Giao \xb8"));
    CHECK(tw.is("g_ev", "Nhiem vu ban"));
    CHECK(tw.is("g_evid", "1"));
    CHECK(tw.is("g_evcount", "2"));
    CHECK(tw.is("g_evtask", "Thu thap_B"));   // the index counts from 0 (0x08170255)
    CHECK(tw.is("g_world", std::to_string(tw.w.map_id()).c_str()));
    // the values behind it: the status bits of ordinal 1 in value 2000, the temp structure from 2200
    const KPlayerTask& t = tw.A().player.task;
    CHECK(t.get_save_val(2000) == static_cast<int>(0x30000000u));
    CHECK(t.get_save_val(2200) == 1);
    CHECK(t.get_save_val(2201) == 101);
    CHECK(t.get_save_val(2202) == 1);
    CHECK(t.get_save_val(2203) == static_cast<int>(ts::key_hash("TalkNpc")));
    CHECK(t.get_save_val(2204) == 7);
    // every changed value went to the client as a 0xa7 (0x0820E1E0), none of them a SYNC_FLAG id
    const auto values = values_of(tw.w.take_outbox(), 7);
    CHECK(values.size() == 6);   // the start (2201, 2200; 2202 stays 0), the status (2000), the temp (2202, 2203, 2204; 2200 and 2201 unchanged)
    // a second task: FirstTask / NextTask walk the groups in id order, the list ends with nil and starts over
    CHECK(tw.call("Start", {std::string("Sat quai_C")}) == 1.0);
    CHECK(tw.call("Walk") == 2.0);
    CHECK(tw.is("g_walk", "Doi thoai_A;Sat quai_C;"));
    CHECK(tw.call("Walk") == 2.0);
    // an unknown task: StartTask 0, CloseTask 0, the status nil, SetTaskStatus 0, the temp nil / 0
    CHECK(tw.call("Start", {std::string("nobody")}) == 0.0);
    CHECK(tw.call("Close", {std::string("nobody")}) == 0.0);
    CHECK(tw.call("Status", {std::string("nobody")}) == -1.0);
    CHECK(tw.call("SetStatus", {std::string("nobody"), 1.0}) == 0.0);
    CHECK(tw.call("Tmp", {std::string("nobody"), std::string("TalkNpc")}) == -1.0);
    CHECK(tw.call("SetTmp", {std::string("nobody"), std::string("TalkNpc"), 1.0}) == 0.0);
    // CloseTask drops the group and its temp values; the status bits stay (task_head.lua clears them itself)
    CHECK(tw.call("Close", {std::string("Doi thoai_A")}) == 1.0);
    CHECK(tw.call("Close", {std::string("Doi thoai_A")}) == 0.0);
    CHECK(tw.call("Tmp", {std::string("Doi thoai_A"), std::string("TalkNpc")}) == -1.0);
    CHECK(tw.call("Status", {std::string("Doi thoai_A")}) == 3.0);
    CHECK(tw.call("Walk") == 1.0);
    CHECK(tw.is("g_walk", "Sat quai_C;"));
    CHECK(t.get_save_val(2201) == 103);
    CHECK(t.get_save_val(2203) == 0);
    // SetTmpValue on a task without a group makes one (0x0820DE40 creates); the value is a word
    CHECK(tw.call("SetTmp", {std::string("Thu thap_B"), std::string("KillNpc"), 70000.0}) == 1.0);
    CHECK(tw.call("Tmp", {std::string("Thu thap_B"), std::string("KillNpc")}) == static_cast<double>(70000 & 0xffff));
    CHECK(tw.call("Walk") == 2.0);
    // the status of the description row (ordinal 0) and of ordinal 3 share value 2000 with ordinal 1
    CHECK(tw.call("SetStatus", {std::string("Sat quai_C"), 2.0}) == 1.0);
    CHECK(tw.call("Status", {std::string("Sat quai_C")}) == 2.0);
    CHECK(tw.call("Status", {std::string("Doi thoai_A")}) == 3.0);
    CHECK(t.get_save_val(2000) == static_cast<int>(0x30000000u | 0x02000000u));
    // SelectTaskStart(id): OnMenuTaskStart of task_function.lua with the id
    tw.call("Select", {102.0});
    jx::zone::KLuaScript* tf = tw.w.config().scripts->get(R"(\script\task\system\task_function.lua)");
    REQUIRE(tf != nullptr);
    CHECK(tf->call_number("MenuSeen", {}) == 102.0);
}

TEST_CASE("the room of the temp structure: 2 per group + 2 per pair, StartTask allows 98 (0x0820E538), the read wants two slots spare (0x0820E00C)", "[tasksys][world]")
{
    TaskWorld tw;
    // 44 groups = 88 slots
    for (int i = 0; i < 44; ++i) tw.A().player.task.set_save_val(2201 + 2 * i, 10000 + i);
    tw.A().player.task.set_save_val(2200, 44);
    ts::KTaskTemp temp;
    REQUIRE(temp.decode(tw.A().player.task));
    CHECK(temp.slots() == 88);
    CHECK(tw.w.task_start(tw.A(), "Doi thoai_A"));   // 90
    for (int i = 0; i < 3; ++i) CHECK(tw.w.task_set_temp(tw.A(), "Doi thoai_A", "k" + std::to_string(i), i));   // 96
    REQUIRE(temp.decode(tw.A().player.task));
    CHECK(temp.slots() == 96);
    CHECK(temp.groups.size() == 45);
    // 96 is not over 98: another group goes in and fills the structure to the last slot
    CHECK(tw.w.task_start(tw.A(), "Thu thap_B"));
    CHECK(tw.A().player.task.get_save_val(2200) == 46);
    CHECK(tw.A().player.task.get_save_val(2298) == 0);   // the last group (id 102 sorts after 101, before 10000): its n
    // the binary's reader wants two slots after the last group: a full structure comes back one group short - like the old server
    CHECK_FALSE(temp.decode(tw.A().player.task));
    CHECK(temp.groups.size() == 45);
    CHECK(temp.slots() == 96);
}
