// The script functions of task_main.lua and its libraries that are neither dialog nor item (docs/LINUX-SERVER.md §25):
// the string buffer PushString 0x0812FDA0 / AppendString 0x0812FCD0 / ReplaceString 0x0812EB20 / PopString 0x080FFB00,
// WriteLog 0x081237D0, GetAccount 0x0810F6A0, AddOwnExp 0x081126C0 -> KPlayer 0x080AFEA0, AddRepute 0x08117290 /
// GetRepute 0x08117230, TaskTip 0x08122730 (the 0xb6 packet).
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
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KPlayerTask.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"

using jx::zone::KNpc;
using jx::zone::KScriptCache;

namespace {

constexpr const char* kScript = R"(
function main(param)
    PushString("mot")
    AppendString("-")
    AppendString("hai-mot")
    g_pop1 = PopString()
    ReplaceString("mot", "1")
    g_pop2 = PopString()
    ReplaceString("", "x")
    ReplaceString("zzz", "x")
    g_pop2b = PopString()
    ReplaceString("1", "")
    g_pop3 = PopString()
    PushString("")
    g_pop4 = PopString()
    AppendString("abc")
    AppendString("")
    g_pop5 = PopString()
    ReplaceString("abcd", "x")
    g_pop6 = PopString()
    PushString("aaa")
    ReplaceString("aa", "b")
    g_pop7 = PopString()
    g_account = GetAccount()
    g_rep0 = GetRepute()
    AddRepute(5)
    g_rep1 = GetRepute()
    AddRepute(-9)
    g_rep2 = GetRepute()
    AddRepute(-2.9)
    g_rep3 = GetRepute()
    WriteLog("mot dong ghi")
    WriteLog()
    TaskTip("Ban nhan duoc mot nhiem vu ngau nhien rat dai qua sau muoi hai byte de bi cat bot o cuoi")
    TaskTip()
end
function exp_small()
    AddOwnExp(50)
end
function exp_up()
    AddOwnExp(60)
end
function exp_none()
    AddOwnExp(-5)
    AddOwnExp()
end
function Str(v)
    if v == nil then return "nil" end
    return tostring(v)
end
function Is(name, expected)
    if Str(_G[name]) == expected then return 1 end
    return 0
end
)";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_scriptfuns_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "misc.lua") << kScript;
    return root.string();
}

jx::zone::KSubWorldConfig misc_world()
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
    for (int level = 1; level <= 200; ++level) tables->set_level_exp(level, level == 10 ? 100 : level == 11 ? 1000 : 5000);
    c.player_set = tables;
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
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

struct MiscWorld {
    jx::zone::KSubWorld w;
    jx::EntityId a;
    MiscWorld() : w(misc_world())
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
        A().player.account = "dai04";
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    bool is(const char* name, const char* expected)
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\misc.lua)");
        REQUIRE(s != nullptr);
        return s->call_number("Is", {std::string(name), std::string(expected)}) == 1.0;
    }
};

std::vector<jx::pb::TaskTip> tips(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::TaskTip> out;
    for (const auto& p : all) {
        if (p.msg_id != jx::pb::G2C_TASK_TIP || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
        jx::pb::TaskTip m;
        REQUIRE(m.ParseFromString(p.payload));
        out.push_back(m);
    }
    return out;
}

} // namespace

TEST_CASE("the string buffer, GetAccount, AddRepute / GetRepute, WriteLog and TaskTip of the scripts", "[scriptfuns][world]")
{
    MiscWorld mw;
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "main", mw.A(), 0));
    // PushString / AppendString / PopString: the buffer as one string (0x0812FDDB empties, 0x0812FD4D appends)
    CHECK(mw.is("g_pop1", "mot-hai-mot"));
    // ReplaceString: every occurrence, left to right (0x0812EBE4)
    CHECK(mw.is("g_pop2", "1-hai-1"));
    // an empty pattern (0x0812EB9B) or one that never matches leaves the buffer alone
    CHECK(mw.is("g_pop2b", "1-hai-1"));
    // an empty replacement removes the pattern (0x0812EBF6)
    CHECK(mw.is("g_pop3", "-hai-"));
    // PushString("") empties (0x0812FDDB then 0x0812FDEC), an empty AppendString adds nothing (0x0812FD0E)
    CHECK(mw.is("g_pop4", ""));
    CHECK(mw.is("g_pop5", "abc"));
    // a pattern longer than the buffer: the tail is copied as it is (0x0812EC67)
    CHECK(mw.is("g_pop6", "abc"));
    // no overlap: "aaa" with "aa" -> "b" gives "ba" (the match skips the pattern, 0x0812EC47)
    CHECK(mw.is("g_pop7", "ba"));
    // GetAccount: the name the gateway logged in (Player+0x264)
    CHECK(mw.is("g_account", "dai04"));
    // AddRepute / GetRepute on the task value 100: 0 + 5, then -9 refused (the sum would be negative, 0x08117315), then
    // -2.9 truncated to -2 (0x081172F4)
    CHECK(mw.is("g_rep0", "0"));
    CHECK(mw.is("g_rep1", "5"));
    CHECK(mw.is("g_rep2", "5"));
    CHECK(mw.is("g_rep3", "3"));
    CHECK(mw.A().player.task.get_save_val(jx::zone::kTaskRepute) == 3);
    // TaskTip: one packet for the string (the missing argument sends nothing, 0x0812274C), 0x3e bytes of text
    const auto t = tips(mw.w.take_outbox(), 7);
    REQUIRE(t.size() == 1);
    CHECK(t[0].text().size() == jx::zone::kTaskTipMax);
    CHECK(t[0].text() == "Ban nhan duoc mot nhiem vu ngau nhien rat dai qua sau muoi hai");   // 62 bytes: " byte de bi cat..." is gone
}

TEST_CASE("AddOwnExp 0x081126C0 -> 0x080AFEA0: the amount as given, up to the need, a level up at it, nothing when negative", "[scriptfuns][world]")
{
    MiscWorld mw;
    KNpc& A = mw.A();
    REQUIRE(A.level == 10);
    REQUIRE(A.player.exp == 0);
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "exp_small", A, 0));
    CHECK(A.player.exp == 50);
    CHECK(A.level == 10);
    CHECK(A.player.next_level_exp == 100);
    // 50 + 60 passes the need of 100: the exp is capped at the need (0x080AFF47) and LevelUp clears it (0x080AFFBD)
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "exp_up", A, 0));
    CHECK(A.level == 11);
    CHECK(A.player.exp == 0);
    CHECK(A.player.next_level_exp == 1000);
    // a negative amount never reaches the core (0x08112717); no argument does nothing (0x081126D9)
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "exp_none", A, 0));
    CHECK(A.player.exp == 0);
    CHECK(A.level == 11);
}

TEST_CASE("KPlayer::add_exp_direct 0x080AFEA0: the clamps of the core", "[scriptfuns]")
{
    jx::zone::KPlayerSet tables;
    for (int level = 1; level <= 200; ++level) tables.set_level_exp(level, 1000);
    KNpc n;
    n.kind = jx::zone::KNpcKind::player;
    n.level = 20;
    n.player.next_level_exp = 1000;
    // a gain below the need adds, at the need levels up
    CHECK(n.player.add_exp_direct(n, 999, tables, nullptr) == 0);
    CHECK(n.player.exp == 999);
    CHECK(n.player.add_exp_direct(n, 1, tables, nullptr) == 1);
    CHECK(n.level == 21);
    CHECK(n.player.exp == 0);
    // more than the room: capped at the need (the leftover is lost), one level
    CHECK(n.player.add_exp_direct(n, 5000, tables, nullptr) == 1);
    CHECK(n.level == 22);
    CHECK(n.player.exp == 0);
    // a zero moves nothing
    CHECK(n.player.add_exp_direct(n, 0, tables, nullptr) == 0);
    CHECK(n.player.exp == 0);
    // from a negative exp (the death loss) the gain climbs back; a loss beyond -need stops at -need (0x080B0030 / 0x080AFF6A)
    n.player.exp = -300;
    CHECK(n.player.add_exp_direct(n, 100, tables, nullptr) == 0);
    CHECK(n.player.exp == -200);
    CHECK(n.player.add_exp_direct(n, -5000, tables, nullptr) == 0);
    CHECK(n.player.exp == -1000);
    CHECK(n.player.add_exp_direct(n, 1500, tables, nullptr) == 0);
    CHECK(n.player.exp == 500);
    // level 200: a gain is nothing (0x080AFED6)
    n.level = 200;
    n.player.exp = 10;
    CHECK(n.player.add_exp_direct(n, 100, tables, nullptr) == 0);
    CHECK(n.player.exp == 10);
    CHECK(n.level == 200);
}
