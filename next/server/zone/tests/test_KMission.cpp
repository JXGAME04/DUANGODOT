// The missions of a map (docs/LINUX-SERVER.md §33): KMission (2003 KMission.h, jx_linux_y 0x73a0 bytes each), the mission and
// timer-task tables, and the script api OpenMission 0x081332F0 / RunMission 0x08132E50 / CloseMission 0x081327E0 /
// JoinMission 0x08137E40 / AddMSPlayer 0x081366A0 / DelMSPlayer 0x081372A0 / GetMSPlayerCount 0x081351F0 /
// GetNextPlayer 0x08135760 / PIdx2MSDIdx 0x08136D90 / MSDIdx2PIdx 0x08137970 / Msg2MSAll 0x08134280 / Msg2MSGroup 0x08134C60 /
// StartMissionTimer 0x08138840 / StopMissionTimer 0x08134720 / GetMSRestTime 0x081361C0.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KMission.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KMission;
using jx::zone::KMissionTable;
using jx::zone::KNpc;
using jx::zone::KScriptCache;

namespace {

constexpr const char* kMissionScript = R"(
function InitMission(p) g_init = (g_init or 0) + 1 g_init_p = p end
function RunMission(p) g_run = (g_run or 0) + 1 end
function EndMission(p) g_end = (g_end or 0) + 1 end
function OnLeave(idx) g_leave = idx end
function JoinMission(idx, group) g_join_idx = idx g_join_group = group end
function Num(name) return _G[name] end
)";

constexpr const char* kTimerScript = R"(
function OnTimer(p) g_ticks = (g_ticks or 0) + 1 g_tick_p = p end
function Num(name) return _G[name] end
)";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_mission_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "mission.lua") << kMissionScript;
    std::ofstream(root / "script" / "test" / "timer.lua") << kTimerScript;
    return root.string();
}

jx::zone::KSubWorldConfig mission_world()
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
    auto tables = std::make_shared<jx::zone::KPlayerSet>();
    for (int level = 1; level <= 200; ++level) tables->set_level_exp(level, 5000);
    c.player_set = tables;
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    auto missions = std::make_shared<KMissionTable>();
    missions->missions[1] = R"(\script\test\mission.lua)";
    missions->missions[3] = "";                        // a row without a script
    missions->timers[1] = R"(\script\test\timer.lua)";
    c.missions = missions;
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

struct MissionWorld {
    jx::zone::KSubWorld w;
    jx::EntityId a, b;
    jx::zone::KLuaScript script;   // the driver: the api functions called from C++ with A as the player
    MissionWorld() : w(mission_world())
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
        REQUIRE(w.spawn_player(8, role_of(2, "B", 2100, 2000), b, at) == jx::pb::RESULT_OK);
        w.tick();
        w.take_outbox();
        REQUIRE(script.init(""));
        REQUIRE(script.do_string(
            "function NPIdx(m, i, g) local x = GetNextPlayer(m, i, g) return x end\n"
            "function NPPlayer(m, i, g) local x, y = GetNextPlayer(m, i, g) return y end\n"
            "function NPIdx1(m) local x = GetNextPlayer(m) return x end\n", "drive"));
        jx::zone::KScriptContext& ctx = jx::zone::g_ScriptContext();
        ctx.world = &w;
        ctx.player = w.mutable_entity(a);
        ctx.sid = 7;
        ctx.script_path.clear();
    }
    ~MissionWorld() { jx::zone::g_ScriptContext() = jx::zone::KScriptContext{}; }
    std::optional<double> call(const char* fn, const std::vector<jx::zone::KLuaScript::Arg>& args) { return script.call_number(fn, args); }
    std::optional<double> mission_num(const char* name)
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\mission.lua)");
        REQUIRE(s != nullptr);
        return s->call_number("Num", {std::string(name)});
    }
    std::optional<double> timer_num(const char* name)
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\timer.lua)");
        REQUIRE(s != nullptr);
        return s->call_number("Num", {std::string(name)});
    }
    int system_lines(std::uint64_t sid, const std::vector<jx::zone::Packet>& out)
    {
        int n = 0;
        for (const auto& p : out) {
            if (p.msg_id != jx::pb::G2C_CHAT_MSG || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
            jx::pb::ChatMsg m;
            REQUIRE(m.ParseFromString(p.payload));
            if (m.channel() == jx::pb::CH_SYSTEM) ++n;
        }
        return n;
    }
};

} // namespace

TEST_CASE("KMission keeps the player list by 1-based data index and the three periodic timers", "[mission]")
{
    KMission m;
    CHECK(m.player_count(0) == 0);
    CHECK(m.add_player(jx::EntityId{11}, 1, 2, 100) == 1);
    CHECK(m.add_player(jx::EntityId{12}, 2, 1, 100) == 2);
    CHECK(m.add_player(jx::EntityId{13}, 3, 2, 100) == 3);
    CHECK(m.add_player(jx::EntityId{14}, 0, 2, 100) == 0);   // no player id: refused (2003 AddPlayer)
    CHECK(m.player_count(0) == 3);
    CHECK(m.player_count(2) == 2);
    CHECK(m.player_count(1) == 1);
    CHECK(m.data_index(jx::EntityId{12}) == 2);
    CHECK(m.data_index(jx::EntityId{99}) == 0);
    CHECK(m.next_player(0, 0) == std::pair<int, jx::EntityId>{1, jx::EntityId{11}});
    CHECK(m.next_player(1, 2) == std::pair<int, jx::EntityId>{3, jx::EntityId{13}});
    CHECK(m.next_player(3, 0) == std::pair<int, jx::EntityId>{0, jx::EntityId{}});
    CHECK(m.remove_player(jx::EntityId{12}));
    CHECK_FALSE(m.remove_player(jx::EntityId{12}));
    CHECK(m.add_player(jx::EntityId{15}, 5, 1, 100) == 2);   // the freed slot again (FindFree: the lowest free)
    CHECK(m.entry(2)->player == jx::EntityId{15});
    CHECK(m.entry(9) == nullptr);
    // timers: three at most, periodic, SetTimer(0) closed
    CHECK(m.start_timer(1, 5, 100));
    CHECK(m.start_timer(2, 0, 100));   // closed at once: never fires
    CHECK(m.start_timer(3, 7, 100));
    CHECK_FALSE(m.start_timer(4, 1, 100));   // MAX_TIMER_PERMISSION 3
    CHECK(m.rest_time(1, 100) == 5);
    CHECK(m.rest_time(1, 103) == 2);
    CHECK(m.rest_time(2, 100) == 0);
    CHECK(m.rest_time(9, 100) == 0);
    CHECK(m.fire_timers(104).empty());
    CHECK(m.fire_timers(105) == std::vector<int>{1});
    CHECK(m.rest_time(1, 105) == 5);   // re-armed: fire = 105 + 5
    CHECK(m.fire_timers(107) == std::vector<int>{3});
    CHECK(m.fire_timers(110) == std::vector<int>{1});
    m.stop_timer(1);
    CHECK(m.rest_time(1, 110) == 0);
    CHECK(m.timer_count() == 2);
    CHECK(m.fire_timers(200) == std::vector<int>{3});
    m.clear();
    CHECK(m.player_count(0) == 0);
    CHECK(m.timer_count() == 0);
}

TEST_CASE("the mission api: open / run / players / lines / timers / leave / join / close", "[mission][world][lua]")
{
    MissionWorld mw;
    // OpenMission(1): the mission made and InitMission of its script run with no player (0x0813377E); again: nothing
    CHECK_FALSE(mw.call("OpenMission", {1.0}).has_value());
    REQUIRE(mw.w.find_mission(1) != nullptr);
    CHECK(mw.mission_num("g_init") == 1.0);
    CHECK(mw.mission_num("g_init_p") == 0.0);
    mw.call("OpenMission", {1.0});
    CHECK(mw.mission_num("g_init") == 1.0);
    // a row without a script still opens (2003 LuaInitMission: Add, then the script only if named); an unknown id too
    mw.call("OpenMission", {3.0});
    CHECK(mw.w.find_mission(3) != nullptr);
    mw.call("OpenMission", {9.0});
    CHECK(mw.w.find_mission(9) != nullptr);
    mw.call("OpenMission", {-1.0});
    CHECK(mw.w.find_mission(-1) == nullptr);
    // RunMission(1) -> RunMission of the script
    mw.call("RunMission", {1.0});
    CHECK(mw.mission_num("g_run") == 1.0);
    mw.call("RunMission", {2.0});   // no such mission
    CHECK(mw.mission_num("g_run") == 1.0);
    // AddMSPlayer(id, group) for the script's player, (id, player, group) for another
    mw.call("AddMSPlayer", {1.0, 2.0});
    CHECK(mw.call("GetMSPlayerCount", {1.0}) == 1.0);
    CHECK(mw.call("GetMSPlayerCount", {1.0, 2.0}) == 1.0);
    CHECK(mw.call("GetMSPlayerCount", {1.0, 1.0}) == 0.0);
    mw.call("AddMSPlayer", {1.0, static_cast<double>(mw.b.value), 1.0});
    CHECK(mw.call("GetMSPlayerCount", {1.0, 0.0}) == 2.0);
    CHECK(mw.call("GetMSPlayerCount", {2.0}) == 0.0);   // no such mission
    // GetNextPlayer(id, idx, group) -> the next data index and its player
    CHECK(mw.call("NPIdx", {1.0, 0.0, 0.0}) == 1.0);
    CHECK(mw.call("NPPlayer", {1.0, 0.0, 0.0}) == static_cast<double>(mw.a.value));
    CHECK(mw.call("NPIdx", {1.0, 1.0, 0.0}) == 2.0);
    CHECK(mw.call("NPPlayer", {1.0, 1.0, 0.0}) == static_cast<double>(mw.b.value));
    CHECK(mw.call("NPIdx", {1.0, 2.0, 0.0}) == 0.0);
    CHECK(mw.call("NPPlayer", {1.0, 2.0, 0.0}) == 0.0);
    CHECK(mw.call("NPIdx", {1.0, 0.0, 1.0}) == 2.0);    // group 1: B only
    CHECK(mw.call("NPIdx1", {1.0}) == 0.0);             // one argument: (0, 0) (0x08135760: top < 2)
    CHECK(mw.call("NPIdx", {1.0}) == 1.0);              // three, the last two nil: idx 0, group 0 (lua_tonumber of nil)
    CHECK(mw.call("PIdx2MSDIdx", {1.0, static_cast<double>(mw.b.value)}) == 2.0);
    CHECK(mw.call("PIdx2MSDIdx", {1.0, 999.0}) == 0.0);
    CHECK(mw.call("MSDIdx2PIdx", {1.0, 2.0}) == static_cast<double>(mw.b.value));
    CHECK(mw.call("MSDIdx2PIdx", {1.0, 7.0}) == 0.0);
    // Msg2MSAll / Msg2MSGroup: a system line to each player of the mission / of the group
    mw.w.take_outbox();
    mw.call("Msg2MSAll", {1.0, std::string("xin chao")});
    mw.call("Msg2MSGroup", {1.0, std::string("nhom mot"), 1.0});
    mw.call("Msg2MSAll", {2.0, std::string("khong ai")});
    const auto out = mw.w.take_outbox();
    CHECK(mw.system_lines(7, out) == 1);
    CHECK(mw.system_lines(8, out) == 2);
    // StartMissionTimer(id, timer, frames): OnTimer of \settings\timertask.txt's script every `frames`, GetMSRestTime counts down
    CHECK_FALSE(mw.call("StartMissionTimer", {1.0, 1.0}).has_value());   // two arguments: nothing
    mw.call("StartMissionTimer", {1.0, 1.0, 3.0});
    CHECK(mw.call("GetMSRestTime", {1.0, 1.0}) == 3.0);
    mw.w.tick();
    mw.w.tick();
    CHECK(mw.call("GetMSRestTime", {1.0, 1.0}) == 1.0);
    CHECK_FALSE(mw.timer_num("g_ticks").has_value());
    mw.w.tick();
    CHECK(mw.timer_num("g_ticks") == 1.0);
    CHECK(mw.timer_num("g_tick_p") == 0.0);
    CHECK(mw.call("GetMSRestTime", {1.0, 1.0}) == 3.0);   // periodic
    mw.w.tick();
    mw.w.tick();
    mw.w.tick();
    CHECK(mw.timer_num("g_ticks") == 2.0);
    mw.call("StopMissionTimer", {1.0, 1.0});
    CHECK(mw.call("GetMSRestTime", {1.0, 1.0}) == 0.0);
    for (int i = 0; i < 4; ++i) mw.w.tick();
    CHECK(mw.timer_num("g_ticks") == 2.0);
    mw.call("StartMissionTimer", {1.0, 5.0, 2.0});   // a timer id without a script: fires nothing
    mw.call("StartMissionTimer", {1.0, 1.0, 0.0});   // interval 0: closed
    for (int i = 0; i < 3; ++i) mw.w.tick();
    CHECK(mw.timer_num("g_ticks") == 2.0);
    CHECK(mw.call("GetMSRestTime", {1.0, 1.0}) == 0.0);
    // DelMSPlayer(id, player, group) / (id, group): the entry gone and OnLeave(player) of the script
    mw.call("DelMSPlayer", {1.0, static_cast<double>(mw.b.value), 1.0});
    CHECK(mw.mission_num("g_leave") == static_cast<double>(mw.b.value));
    CHECK(mw.call("GetMSPlayerCount", {1.0}) == 1.0);
    mw.call("DelMSPlayer", {1.0, 2.0});
    CHECK(mw.mission_num("g_leave") == static_cast<double>(mw.a.value));
    CHECK(mw.call("GetMSPlayerCount", {1.0}) == 0.0);
    mw.call("DelMSPlayer", {1.0, 2.0});   // not in: nothing
    CHECK(mw.mission_num("g_leave") == static_cast<double>(mw.a.value));
    // JoinMission(id, group): JoinMission(player, group) of the script for the script's player (0x08138266)
    mw.call("JoinMission", {1.0, 3.0});
    CHECK(mw.mission_num("g_join_idx") == static_cast<double>(mw.a.value));
    CHECK(mw.mission_num("g_join_group") == 3.0);
    // CloseMission(id): EndMission of the script, the mission gone
    mw.call("CloseMission", {1.0});
    CHECK(mw.mission_num("g_end") == 1.0);
    CHECK(mw.w.find_mission(1) == nullptr);
    CHECK(mw.call("GetMSPlayerCount", {1.0}) == 0.0);
    mw.call("RunMission", {1.0});
    CHECK(mw.mission_num("g_run") == 1.0);
    mw.call("CloseMission", {3.0});
    CHECK(mw.w.find_mission(3) == nullptr);
}
