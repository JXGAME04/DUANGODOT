// The script functions of task_main.lua and its libraries that are neither dialog nor item (docs/LINUX-SERVER.md Â§25):
// the string buffer PushString 0x0812FDA0 / AppendString 0x0812FCD0 / ReplaceString 0x0812EB20 / PopString 0x080FFB00,
// WriteLog 0x081237D0, GetAccount 0x0810F6A0, AddOwnExp 0x081126C0 -> KPlayer 0x080AFEA0, AddRepute 0x08117290 /
// GetRepute 0x08117230, TaskTip 0x08122730 (the 0xb6 packet).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>
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
#include "jx/zone/KObjectBuffer.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KNpcTemplate.h"
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
function s1_bits()
    g_b1 = GetBit(5, 1)
    g_b2 = GetBit(5, 2)
    g_b3 = GetBit(5, 33)
    g_b4 = GetBit()
    g_s1 = SetBit(0, 3, 1)
    g_s2 = SetBit(7, 1, 0)
    g_s3 = SetBit(7, 40, 1)
    g_s4 = SetBit(-1, 1, 0)
    g_s5 = SetBit(0, 3, 2)
    g_by1 = GetByte(305419896, 1)
    g_by2 = GetByte(305419896, 4)
    g_by3 = GetByte(305419896, 5)
    g_sb1 = SetByte(305419896, 2, 171)
    g_sb2 = SetByte(305419896, 9, 1)
    g_sb3 = SetByte(305419896, 1, 256)
end
function s1_mission()
    g_m0 = GetMissionV(5)
    SetMissionV(5, 42)
    SetMissionV(0, 7)
    SetMissionV(100, 9)
    SetMissionV(99, 3.7)
    g_m1 = GetMissionV(5)
    g_m2 = GetMissionV(0)
    g_m3 = GetMissionV(100)
    g_m4 = GetMissionV(99)
    g_m5 = GetMissionV()
end
function s1_world(other)
    g_w1 = SubWorldIdx2ID()
    g_w2 = SubWorldID2Idx(g_w1)
    g_w3 = SubWorldID2Idx(9999)
    g_w4 = SubWorldIdx2ID(9999)
    g_w5 = SubWorldID2Idx()
    g_w6 = SubWorldID2Idx(other)
    g_w7 = SubWorldIdx2ID(other)
end
function s1_time()
    g_t1 = GetCurServerTime()
    g_d1 = GetLocalDate("%Y")
    g_d2 = GetLocalDate("%H:%M")
    g_d3 = GetLocalDate()
end
function s1_bad_date()
    return GetLocalDate("%q")
end
function s1_player()
    g_exp = GetExp()
    g_e0 = GetExtPoint(0)
    g_a1 = AddExtPoint(0, 10)
    g_a2 = AddExtPoint(0, -3)
    g_a3 = AddExtPoint(8, 5)
    g_a4 = AddExtPoint(0)
    g_e1 = GetExtPoint(0)
    g_p1 = PayExtPoint(0, 4)
    g_p2 = PayExtPoint(0, 100)
    g_e2 = GetExtPoint(0)
    g_g1 = AddExtPointForGS(1, 2)
    g_e3 = GetExtPoint(1)
    g_e4 = GetExtPoint(7)
    g_e5 = GetExtPoint(8)
    g_e6 = GetExtPoint()
    g_free = CalcFreeItemCellCount()
    g_sp1 = SearchPlayer("A")
    g_sp2 = SearchPlayer("nobody")
    g_sp3 = SearchPlayer("")
    g_spb = SearchPlayer("B")
    g_cp = CallPlayerFunction(g_sp1, "Twice", 21)
    g_cp2 = CallPlayerFunction(g_spb, "NameOf")
    g_cp3 = CallPlayerFunction(0, "Twice", 1)
    g_cp4 = CallPlayerFunction(g_sp1, "", 1)
    g_cp5 = CallPlayerFunction(g_sp1, "NoSuchFunction", 1)
    g_cp6 = CallPlayerFunction(g_spb, NameOf)
    g_cp7, g_cp8 = CallPlayerFunction(g_sp1, "Pair", 1, 2)
    g_me = GetName()
end
function s1_item(idx)
    g_in1 = GetItemName(idx)
    g_in2 = GetItemName(999999)
    g_in3 = GetItemName()
    g_ip1 = GetItemParam(idx, 1)
    g_ip2 = GetItemParam(idx, 6)
    g_ip3 = GetItemParam(idx, 7)
    g_ip4 = GetItemParam(idx)
    SetItemMagicLevel(idx, 6, 77)
    g_ip5 = GetItemParam(idx, 6)
end
function s2_npc()
    local map = SubWorldIdx2ID()
    g_n1 = AddNpc(900, 5, map, 2300, 2000, 1, "Heo Rung", 1)
    g_n2 = AddNpc("boar", 3, map, 2400, 2000)
    g_n3 = AddNpc(900, 1)
    g_n4 = AddNpc(900, 1, 999, 2300, 2000)
    g_n5 = AddNpc(12345, 1, map, 2300, 2000)
    g_n6 = AddNpc(900, -7, map, 2500, 2000, 0, "", 2)
    g_i1 = GetNpcSettingIdx(g_n1)
    SetNpcParam(g_n1, 3, 42)
    SetNpcParam(g_n1, 11, 1)
    SetNpcParam(g_n1, 0, 1)
    SetNpcParam(g_n1, 4)
    g_p1 = GetNpcParam(g_n1, 3)
    g_p2 = GetNpcParam(g_n1, 11)
    g_p3 = GetNpcParam(g_n1, 0)
    g_p4 = GetNpcParam(0, 1)
    g_p5 = GetNpcParam(g_n1)
    SetNpcScript(g_n1, "\\script\\test\\misc.lua", 7)
    SetNpcScript(g_n2)
    g_ss = AddSkillState(509, 1, 0, 180)
end
function s2_del(player)
    DelNpc(g_n1)
    DelNpc("boar")
    DelNpc(player)
    DelNpc()
end
Lib = {}
function Lib:Ten(n) return n + 10 end
function Lib.Twice(n) return n * 2 end
function MyEntity() return SearchPlayer(GetName()) end
function OnTimer(idx) g_timer = idx end
function OnDeath(k) g_death = k end
function s3_kill() KillPlayer() end
function s3_a()
    local map = SubWorldIdx2ID()
    g_n1 = AddNpc(900, 1, map, 2300, 2000)
    g_n2 = AddNpc(900, 1, map, 2400, 2000)
    g_series = GetSeries()
    g_gt1 = GetGameTime()
    g_life = ST_GetTransLifeCount()
    SetLogoutRV(1)
    g_glb0 = GetGlbValue(8)
    SetGlbValue(8, 77)
    SetGlbValue(5001, 1)
    SetGlbValue(-1, 1)
    g_glb1 = GetGlbValue(8)
    g_glb2 = GetGlbValue(5001)
    g_glb3 = GetGlbValue()
    g_ms0 = GetMissionS(2)
    SetMissionS(2, "xin chao")
    SetMissionS(0, "khong")
    SetMissionS(101, "khong")
    g_ms1 = GetMissionS(2)
    g_ms2 = GetMissionS(101)
    g_ms3 = GetMissionS()
    SetMissionS(2, "")
    g_ms4 = GetMissionS(2)
    g_de1 = DynamicExecute("", "Twice", 4)
    g_de2 = DynamicExecute("\\script\\test\\misc.lua", "Lib:Ten", 5)
    g_de3 = DynamicExecute("\\script\\test\\misc.lua", "Lib.Twice", 6)
    g_de4 = DynamicExecute("\\script\\test\\nothing.lua", "Twice", 1)
    g_de5 = DynamicExecute("", "", 1)
    g_de6 = DynamicExecute("", "NoSuch", 1)
    g_np = GetNpcId(g_n1)
    g_np2 = GetNpcId()
    g_my = IsMyItem(1)
    SetPunish(0)
end
function s3_b()
    local other = SearchPlayer("B")
    g_dp1 = DynamicExecuteByPlayer(other, "", "MyEntity")
    g_dp2 = DynamicExecuteByPlayer(0, "", "MyEntity")
    g_dp3 = DynamicExecuteByPlayer(other, "", "")
    g_me2 = MyEntity()
    Msg2SubWorld("<color=green>Chuc mung")
    Msg2Map(SubWorldIdx2ID(), "toi ban do")
    Msg2Map(9999, "khong toi")
    DisabledUseTownP(1)
    g_tp1 = GetTask(135)
    DisabledUseTownP(0)
    g_tp2 = GetTask(135)
    SetDeathScript("\\script\\test\\misc.lua")
    SetRank(89)
    SetPunish(1)
    g_tm1 = SetNpcTimer(g_n2, 3)
    g_tm2 = SetNpcTimer(0, 3)
    SetNpcScript(g_n2, "\\script\\test\\misc.lua")
end
function Twice(n) return n * 2 end
function NameOf() return GetName() end
function Pair(a, b) return a + 10, b + 10 end
function Num(name) return _G[name] end
function Str(v)
    if v == nil then return "nil" end
    return tostring(v)
end
function s5_flags_off()
    ForbidEnmity(2)
    ForbitTrade(0)
end
function s5_flags()
    ForbidEnmity(1)
    ForbitTrade(1)
    SetProtectTime(300)
    g_ds1 = DisabledStall(1)
    g_bit1 = GetTask(135)
    g_ds2 = DisabledStall(0)
    g_bit2 = GetTask(135)
    g_ds3 = DisabledStall()
    g_tc1 = SetTmpCamp(3)
    g_tc2 = SetTmpCamp(-1)
    g_tc3 = SetTmpCamp()
    g_tc4 = SetTmpCamp(2, 999999)
    AddMagicPoint(5)
    AddMagicPoint(-100)
    AddStatData("boss_kill")
    AddStatData("boss_kill", 4)
    AddStatData("x", 1, 2)
    g_ct = GetCurrentTime()
    g_tm1 = Tm2Time(2026, 9, 19, 12, 30, 15)
    g_tm2 = Tm2Time(2026, 9, 19)
    g_tm3 = Tm2Time(2026)
    g_tm4 = Tm2Time()
    g_fs1 = FormatTime2String("%Y-%m-%d %H:%M:%S", g_tm1)
    g_fs2 = FormatTime2String("%Y", g_tm3)
    g_fs3 = FormatTime2String()
end
function s5_npc(map)
    g_ne1 = AddNpcEx(900, 7, 2, map, 2300, 2100, 1, "Heo Ex", 1)
    g_ne2 = AddNpcEx("boar", -3, 4, map, 2350, 2100)
    g_ne3 = AddNpcEx(900, 1, 0, map, 2300)
    g_ne4 = AddNpcEx(900, 1, 0, map + 1, 2300, 2100)
    g_ne5 = AddNpcEx(901, 1, 0, map, 2300, 2100)
    g_ne6 = AddNpcEx(900, 2, 1, map, 2400, 2100, 0, "", 2)
    g_ne7 = AddNpcEx(900, 2, 1, map, 2400, 2100, 1)
    SetTmpCamp(5, g_ne1)
    SetNpcCurCamp(g_ne1, 6)
    SetNpcCurCamp(g_ne2, 7)
    g_np1 = NpcIdx2PIdx(g_ne1)
    g_np2 = NpcIdx2PIdx(SearchPlayer("A"))
    g_np3 = NpcIdx2PIdx(999999)
    g_ns1 = AddNpcSkillState(g_ne1, 1, 1, 0, 100)
    g_ns2 = AddNpcSkillState(g_ne1, 1, 1, 0)
    g_ns3 = AddNpcSkillState(0, 1, 1, 0, 100)
    SetNpcDeathScript(g_ne1, "\\script\\test\\misc.lua", 3)
end
function s5_timers()
    g_t1 = AddTimer(2, "s5_OnTime", 11)
    g_t2 = AddTimer(3, "s5_OnTimeRepeat", 22)
    g_t3 = AddTimer(1, "s5_OnTimeTwo", 33)
    g_t4 = AddTimer(1, "", 1)
    g_t5 = AddTimer(1, "x")
    g_dt1 = DelTimer(g_t3)
    g_dt2 = DelTimer(g_t3)
    g_dt3 = DelTimer()
end
function s5_OnTime(param, id)
    g_ot = (g_ot or 0) + 1
    g_ot_param = param
    g_ot_id = id
end
function s5_OnTimeRepeat(param, id)
    g_rep = (g_rep or 0) + 1
    if g_rep == 1 then return 2, param + 1 end
    if g_rep == 2 then
        g_rep_param = param
        return 1
    end
    return 0
end
function s5_OnTimeTwo(param, id)
    g_two = 1
end
function s6_ob()
    g_ob1 = OB_Create()
    g_ob2 = OB_Create()
    g_e1 = OB_IsEmpty(g_ob1)
    g_pi = OB_PushInt(g_ob1, -7)
    g_pd = OB_PushDouble(g_ob1, 2.5)
    g_pb = OB_PushByte(g_ob1, 200)
    g_ps = OB_PushString(g_ob1, "xin chao")
    g_e2 = OB_IsEmpty(g_ob1)
    g_cp = OB_Copy(g_ob2, g_ob1)
    g_ap = OB_Append(g_ob2, g_ob1)
    g_i1 = OB_PopInt(g_ob1)
    g_d1 = OB_PopDouble(g_ob1)
    g_b1 = OB_PopByte(g_ob1)
    g_s1 = OB_PopString(g_ob1)
    g_i2 = OB_PopInt(g_ob1)
    g_e3 = OB_IsEmpty(g_ob1)
    g_i3 = OB_PopInt(g_ob2)
    g_d3 = OB_PopDouble(g_ob2)
    g_b3 = OB_PopByte(g_ob2)
    g_s3 = OB_PopString(g_ob2)
    g_i4 = OB_PopInt(g_ob2)
    g_s4 = OB_PopString(g_ob2)
    OB_Clear(g_ob2)
    g_e4 = OB_IsEmpty(g_ob2)
    g_r1 = OB_Release(g_ob1)
    g_r2 = OB_Release(g_ob1)
    g_e5 = OB_IsEmpty(g_ob1)
    g_pi2 = OB_PushInt(g_ob1, 1)
    g_pi3 = OB_PushInt(g_ob2)
    g_cp2 = OB_Copy(g_ob2)
    g_e6 = OB_IsEmpty()
end
function s6_remote()
    local h = OB_Create()
    OB_PushInt(h, 41)
    OB_PushString(h, "tin")
    g_re1 = RemoteExecute("\\script\\test\\misc.lua", "s6_target", h)
    g_re2 = RemoteExecute("\\script\\test\\misc.lua", "s6_target", h, "s6_back", 9)
    g_re3 = RemoteExecute("", "s6_target", h)
    g_re4 = RemoteExecute("\\script\\test\\misc.lua", "s6_target")
    g_re5 = RemoteExecute("\\script\\test\\misc.lua", "s6_target", 0)
    g_re_left = OB_IsEmpty(h)
    OB_Release(h)
end
function s6_target(hin, hout)
    g_tg = (g_tg or 0) + 1
    g_tg_int = OB_PopInt(hin)
    g_tg_str = OB_PopString(hin)
    OB_PushInt(hout, (g_tg_int or 0) + 1)
end
function s6_back(param, hback)
    g_bk_param = param
    g_bk_int = OB_PopInt(hback)
    g_bk_left = OB_IsEmpty(hback)
end
function s6_misc()
    g_f1 = FileName2Id("\\script\\test\\misc.lua")
    g_f2 = FileName2Id("\\SCRIPT\\TEST\\MISC.LUA")
    g_f3 = FileName2Id("\\script\\x.lua")
    g_f4 = FileName2Id()
    g_f5 = FileName2Id("a", "b")
    SaveNow()
    WriteGoldLog("boss", 1, 2, 3, 4)
    WriteGoldLog()
    g_cs = CastSkill(1, 1)
end
function s6_trap(map, x, y)
    g_tr1 = AddMapTrap(map, x, y, "\\script\\test\\trap.lua")
    g_tr2 = AddMapTrap(map + 1, x, y, "\\script\\test\\trap.lua")
    g_tr3 = AddMapTrap(map, x, y)
    g_tr4 = AddMapTrap(map, x, y, 999)
    g_tr5 = AddMapTrap(map, x + 32, y, FileName2Id("\\script\\test\\trap.lua"))
    g_tr6 = AddMapTrap(map, -100000, y, "\\script\\test\\trap.lua")
end
function s6_chat(npc)
    g_nc1 = NpcChat(npc, "xin chao", 0)
    g_nc2 = NpcChat(npc, "lat nua", 1)
    g_nc3 = NpcChat(npc, "")
    g_nc4 = NpcChat(999999, "x")
    g_nc5 = NpcChat(npc)
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
    std::ofstream(root / "script" / "test" / "trap.lua") << "function main() g_trap = (g_trap or 0) + 1 end\nfunction Num(name) return _G[name] end\n";
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
    // one row of npcs.txt for AddNpc: 900 "boar", a plain monster
    jx::zone::KNpcTemplateSet set;
    jx::zone::KNpcTemplate t;
    t.id = 900;
    t.name = "boar";
    t.kind = jx::zone::kind_normal;
    t.camp = jx::zone::camp_animal;
    t.life_param = 40;
    set.add(t);
    c.templates = std::make_shared<const jx::zone::KNpcTemplateSet>(std::move(set));
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

TEST_CASE("S1 script api: bits and bytes, mission values, subworld ids, the clock, ext points, the bag, SearchPlayer and CallPlayerFunction", "[scriptfuns][s1]")
{
    MiscWorld mw;
    jx::zone::KLuaScript* s = mw.w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto num = [&](const char* name) { return s->call_number("Num", {std::string(name)}); };

    // GetBit / SetBit (0x080FEBA0 / 0x080FEAC0), GetByte / SetByte (0x080FEA20 / 0x080FE950)
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s1_bits", mw.A(), 0));
    CHECK(mw.is("g_b1", "1"));
    CHECK(mw.is("g_b2", "0"));
    CHECK(mw.is("g_b3", "0"));
    CHECK(mw.is("g_b4", "0"));
    CHECK(mw.is("g_s1", "4"));
    CHECK(mw.is("g_s2", "6"));
    CHECK(mw.is("g_s3", "7"));
    CHECK(mw.is("g_s4", "4294967294"));   // -1 as the unsigned value with bit 1 cleared
    CHECK(mw.is("g_s5", "0"));            // `on` must be exactly 1 to set
    CHECK(mw.is("g_by1", "120"));         // 0x12345678: byte 1 = 0x78
    CHECK(mw.is("g_by2", "18"));          // byte 4 = 0x12
    CHECK(mw.is("g_by3", "0"));
    CHECK(mw.is("g_sb1", "305441656"));   // 0x1234ab78
    CHECK(mw.is("g_sb2", "305419896"));   // out of range: unchanged
    CHECK(mw.is("g_sb3", "305419776"));   // the low byte of 256 is 0

    // GetMissionV / SetMissionV (0x081072F0 / 0x08107390): 100 values of the map, 1..99 readable, 0..99 writable
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s1_mission", mw.A(), 0));
    CHECK(mw.is("g_m0", "0"));
    CHECK(mw.is("g_m1", "42"));
    CHECK(mw.is("g_m2", "0"));
    CHECK(mw.is("g_m3", "0"));
    CHECK(mw.is("g_m4", "3"));
    CHECK(mw.is("g_m5", "0"));
    CHECK(mw.w.mission_value(0) == 7);
    CHECK(mw.w.mission_value(99) == 3);
    CHECK(mw.w.mission_value(100) == 0);

    // SubWorldIdx2ID / SubWorldID2Idx (0x081077D0 / 0x08102580): the hosted maps of the zone
    mw.w.set_hosted_maps({mw.w.map_id(), 25});
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s1_world", mw.A(), 25));
    CHECK(num("g_w1") == static_cast<double>(mw.w.map_id()));
    CHECK(num("g_w2") == static_cast<double>(mw.w.map_id()));
    CHECK(mw.is("g_w3", "-1"));
    CHECK(mw.is("g_w4", "0"));
    CHECK(mw.is("g_w5", "-1"));
    CHECK(mw.is("g_w6", "25"));
    CHECK(mw.is("g_w7", "25"));

    // GetCurServerTime / GetLocalDate (0x08103800 / 0x0812A140)
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s1_time", mw.A(), 0));
    const auto now = static_cast<double>(std::time(nullptr));
    const std::optional<double> t1 = num("g_t1");
    REQUIRE(t1.has_value());
    CHECK(std::abs(*t1 - now) <= 5.0);
    std::time_t tt = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char year[8];
    std::strftime(year, sizeof year, "%Y", &tm);
    CHECK(mw.is("g_d1", year));
    CHECK(mw.is("g_d3", "nil"));   // no argument: nothing returned
    CHECK_FALSE(s->call_number("s1_bad_date", {}).has_value());   // "invalid `date' format" is an error

    // a second player for SearchPlayer / CallPlayerFunction
    jx::EntityId b;
    jx::zone::Pos at;
    REQUIRE(mw.w.spawn_player(8, role_of(2, "B", 2100, 2000), b, at) == jx::pb::RESULT_OK);
    mw.w.tick();
    mw.A().player.exp = 1234567;
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s1_player", mw.A(), 0));
    CHECK(mw.is("g_exp", "1234567"));
    CHECK(mw.is("g_e0", "0"));
    CHECK(mw.is("g_a1", "1"));
    CHECK(mw.is("g_a2", "0"));    // a negative amount is refused by the script function (0x0810FC48)
    CHECK(mw.is("g_a3", "0"));    // index 8 is outside 0..7
    CHECK(mw.is("g_a4", "0"));    // one argument only
    CHECK(mw.is("g_e1", "10"));
    CHECK(mw.is("g_p1", "1"));
    CHECK(mw.is("g_p2", "0"));    // not enough
    CHECK(mw.is("g_e2", "6"));
    CHECK(mw.is("g_g1", "1"));
    CHECK(mw.is("g_e3", "2"));
    CHECK(mw.is("g_e4", "0"));
    CHECK(mw.is("g_e5", "0"));
    CHECK(mw.is("g_e6", "0"));
    CHECK(mw.A().player.ext_point[0] == 6);
    CHECK(mw.A().player.ext_point[1] == 2);
    const jx::zone::KItemList* items = mw.w.items_of(7);
    REQUIRE(items != nullptr);
    CHECK(num("g_free") == static_cast<double>(items->room(jx::zone::room_equipment).free_cells()));
    CHECK(num("g_free") > 0.0);
    CHECK(num("g_sp1") == static_cast<double>(mw.a.value));
    CHECK(mw.is("g_sp2", "0"));
    CHECK(mw.is("g_sp3", "0"));
    CHECK(num("g_spb") == static_cast<double>(b.value));
    CHECK(mw.is("g_cp", "42"));
    CHECK(mw.is("g_cp2", "B"));    // GetName inside ran for B
    CHECK(mw.is("g_cp3", "nil"));  // no such player
    CHECK(mw.is("g_cp4", "nil"));  // an empty name
    CHECK(mw.is("g_cp5", "nil"));  // not a function
    CHECK(mw.is("g_cp6", "B"));    // a function value
    CHECK(mw.is("g_cp7", "11"));
    CHECK(mw.is("g_cp8", "12"));
    CHECK(mw.is("g_me", "A"));     // the script's own player is back

    // the ext points survive a save
    jx::pb::RoleData saved;
    REQUIRE(mw.w.role_snapshot(7, saved));
    REQUIRE(saved.stats().ext_point_size() == jx::zone::KPlayer::kExtPoints);
    CHECK(saved.stats().ext_point(0) == 6);
    CHECK(saved.stats().ext_point(1) == 2);

    // GetItemName / GetItemParam (0x081005D0 / 0x080FECC0) need an item: test_KItem.cpp ("S1 GetItemName / GetItemParam")
}

TEST_CASE("S2 script api: AddNpc / DelNpc / SetNpcScript / GetNpcParam / SetNpcParam and AddSkillState without a skill table", "[scriptfuns][s2]")
{
    MiscWorld mw;
    jx::zone::KLuaScript* s = mw.w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto num = [&](const char* name) { return s->call_number("Num", {std::string(name)}); };
    auto entity = [&](const char* name) -> const KNpc* {
        const std::optional<double> v = num(name);
        return v.has_value() && *v > 0.0 ? mw.w.find_entity(jx::EntityId{static_cast<std::uint64_t>(*v)}) : nullptr;
    };
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s2_npc", mw.A(), 0));
    // AddNpc(900, 5, map, x, y, 1, "Heo Rung", 1): the boar at level 5, remove_on_death, named, a boss of kind 3
    const KNpc* n1 = entity("g_n1");
    REQUIRE(n1 != nullptr);
    CHECK(n1->kind == jx::zone::KNpcKind::monster);
    CHECK(n1->template_id == 900);
    CHECK(n1->level == 5);
    CHECK(n1->name == "Heo Rung");
    CHECK(n1->remove_on_death);
    CHECK(n1->boss_flag == 3);
    CHECK(n1->series <= 4);   // rand() % 5
    CHECK(mw.w.to_absolute(n1->pos()).x == 2300);
    // by name, five arguments: the template's name, nothing more set
    const KNpc* n2 = entity("g_n2");
    REQUIRE(n2 != nullptr);
    CHECK(n2->template_id == 900);
    CHECK(n2->name == "boar");
    CHECK(n2->level == 3);
    CHECK_FALSE(n2->remove_on_death);
    CHECK(n2->boss_flag == 0);
    CHECK(mw.is("g_n3", "nil"));   // fewer than five arguments: nothing
    CHECK(mw.is("g_n4", "0"));     // another subworld than this map
    CHECK(mw.is("g_n5", "0"));     // no such template
    // a negative level is 1; boss 2 = a gold npc (BackData ran; no gold table here, so not golding)
    const KNpc* n6 = entity("g_n6");
    REQUIRE(n6 != nullptr);
    CHECK(n6->level == 1);
    CHECK(n6->gold.is_gold);
    CHECK(mw.is("g_i1", "900"));
    // the ten script numbers: n 1..10 only
    CHECK(mw.is("g_p1", "42"));
    CHECK(mw.is("g_p2", "0"));
    CHECK(mw.is("g_p3", "0"));
    CHECK(mw.is("g_p4", "0"));
    CHECK(mw.is("g_p5", "0"));
    CHECK(n1->script_param[2] == 42);
    // SetNpcScript: the path (and the number main() gets); one argument does nothing
    CHECK(n1->script == R"(\script\test\misc.lua)");
    CHECK(n1->script_main_param == 7);
    CHECK(n2->script.empty());
    // AddSkillState without a skill table: -1
    CHECK(mw.is("g_ss", "-1"));
    // DelNpc: by index, by name; a player stays; the npcs leave at the next frame
    const jx::EntityId id1 = n1->id, id2 = n2->id, id6 = n6->id;
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s2_del", mw.A(), static_cast<int>(mw.a.value)));
    CHECK(mw.w.find_entity(id1) != nullptr);   // still there inside the frame
    mw.w.tick();
    CHECK(mw.w.find_entity(id1) == nullptr);
    CHECK(mw.w.find_entity(id2) == nullptr);
    CHECK(mw.w.find_entity(id6) != nullptr);
    CHECK(mw.w.find_entity(mw.a) != nullptr);
}

TEST_CASE("S3 script api: getters, global values, mission strings, DynamicExecute, system lines, town portal bit, death script, rank, npc timer", "[scriptfuns][s3]")
{
    MiscWorld mw;
    jx::zone::KLuaScript* s = mw.w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto num = [&](const char* name) { return s->call_number("Num", {std::string(name)}); };
    mw.A().player.reborn = 2;
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s3_a", mw.A(), 0));
    CHECK(num("g_series") == static_cast<double>(mw.A().series));
    const std::uint64_t frames = mw.w.tick_count() - mw.A().player.login_tick;
    CHECK(num("g_gt1") == static_cast<double>(frames * 20 / 18));
    CHECK(mw.is("g_life", "2"));
    CHECK(mw.A().player.logout_revive);
    CHECK(mw.is("g_glb0", "0"));
    CHECK(mw.is("g_glb1", "77"));
    CHECK(mw.is("g_glb2", "0"));
    CHECK(mw.is("g_glb3", "0"));
    CHECK(mw.is("g_ms0", ""));
    CHECK(mw.is("g_ms1", "xin chao"));
    CHECK(mw.is("g_ms2", ""));
    CHECK(mw.is("g_ms3", ""));
    CHECK(mw.is("g_ms4", ""));
    CHECK(mw.w.mission_string(2).empty());
    CHECK(mw.is("g_de1", "8"));     // the running script
    CHECK(mw.is("g_de2", "15"));    // a:b - the method form
    CHECK(mw.is("g_de3", "12"));    // a.b
    CHECK(mw.is("g_de4", "nil"));   // no such script
    CHECK(mw.is("g_de5", "nil"));   // no function name
    CHECK(mw.is("g_de6", "nil"));   // no such function
    const std::optional<double> n1 = num("g_n1");
    REQUIRE(n1.has_value());
    CHECK(num("g_np") == n1);
    CHECK(mw.is("g_np2", "nil"));
    CHECK(mw.is("g_my", "0"));
    CHECK(mw.A().pk_punish_state == 3);
    // a second player for DynamicExecuteByPlayer and the lines
    jx::EntityId b;
    jx::zone::Pos at;
    REQUIRE(mw.w.spawn_player(8, role_of(2, "B", 2100, 2000), b, at) == jx::pb::RESULT_OK);
    mw.w.tick();
    mw.w.take_outbox();
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s3_b", mw.A(), 0));
    CHECK(num("g_dp1") == static_cast<double>(b.value));   // MyEntity ran as B
    CHECK(mw.is("g_dp2", "nil"));
    CHECK(mw.is("g_dp3", "nil"));
    CHECK(num("g_me2") == static_cast<double>(mw.a.value));   // A is back
    int lines_a = 0, lines_b = 0;
    for (const auto& p : mw.w.take_outbox()) {
        if (p.msg_id != jx::pb::G2C_CHAT_MSG) continue;
        jx::pb::ChatMsg m;
        REQUIRE(m.ParseFromString(p.payload));
        if (m.channel() != jx::pb::CH_SYSTEM) continue;
        if (std::find(p.sids.begin(), p.sids.end(), 7) != p.sids.end()) ++lines_a;
        if (std::find(p.sids.begin(), p.sids.end(), 8) != p.sids.end()) ++lines_b;
    }
    CHECK(lines_a == 2);   // Msg2SubWorld + Msg2Map of this map; the other map's line went nowhere
    CHECK(lines_b == 2);
    CHECK(num("g_tp1") == 1048576.0);   // bit 0x100000 of task value 135 (GetTask pushes a float)
    CHECK(num("g_tp2") == 0.0);
    CHECK(mw.A().player.death_script == R"(\script\test\misc.lua)");
    CHECK(mw.A().rank == 89);
    CHECK(mw.A().pk_punish_state == 0);
    CHECK(mw.is("g_tm1", "1"));
    CHECK(mw.is("g_tm2", "nil"));
    const std::optional<double> n2 = num("g_n2");
    REQUIRE(n2.has_value());
    const KNpc* npc2 = mw.w.find_entity(jx::EntityId{static_cast<std::uint64_t>(*n2)});
    REQUIRE(npc2 != nullptr);
    CHECK(npc2->timer_frame == mw.w.tick_count() + 3);
    // the timer: three frames later OnTimer(npc) of the npc's script ran and the timer is gone
    mw.w.tick();
    mw.w.tick();
    CHECK(mw.is("g_timer", "nil"));
    mw.w.tick();
    CHECK(num("g_timer") == n2);
    CHECK(npc2->timer_frame == 0);
    // the death script: KillPlayer (the player's own blow) -> the death frames -> the corpse -> OnDeath(the last attacker)
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s3_kill", mw.A(), 0));
    CHECK(mw.A().doing == jx::zone::KDoing::death);
    for (int i = 0; i < 40 && mw.is("g_death", "nil"); ++i) mw.w.tick();
    CHECK(num("g_death") == static_cast<double>(mw.a.value));
}

TEST_CASE("S5 script api: player flags, tmp / current camps, skill points, stat counters, the clock helpers, AddNpcEx, AddNpcSkillState, NpcIdx2PIdx and the AddTimer timers", "[scriptfuns][s5]")
{
    MiscWorld mw;
    jx::zone::KLuaScript* s = mw.w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto num = [&](const char* name) { return s->call_number("Num", {std::string(name)}); };
    auto entity = [&](const char* name) -> const KNpc* {
        const std::optional<double> v = num(name);
        return v.has_value() && *v > 0.0 ? mw.w.find_entity(jx::EntityId{static_cast<std::uint64_t>(*v)}) : nullptr;
    };
    // the flags: ForbidEnmity wants exactly 1, ForbitTrade anything but 0
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s5_flags_off", mw.A(), 0));
    CHECK_FALSE(mw.A().player.forbid_enmity);
    CHECK_FALSE(mw.A().player.forbid_trade);
    mw.w.take_outbox();
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s5_flags", mw.A(), 0));
    CHECK(mw.A().player.forbid_enmity);
    CHECK(mw.A().player.forbid_trade);
    CHECK(mw.A().player.protect_time == 300);
    CHECK(mw.is("g_ds1", "1"));
    CHECK(mw.is("g_ds2", "1"));
    CHECK(mw.is("g_ds3", "0"));   // no argument
    CHECK((static_cast<std::uint32_t>(*num("g_bit1")) & 0x800u) != 0);   // the stall bit of task value 135
    CHECK((static_cast<std::uint32_t>(*num("g_bit2")) & 0x800u) == 0);
    CHECK(mw.is("g_tc1", "1"));
    CHECK(mw.is("g_tc2", "0"));   // a negative camp
    CHECK(mw.is("g_tc3", "0"));   // no argument
    CHECK(mw.is("g_tc4", "0"));   // no such npc
    CHECK(mw.A().tmp_camp == 3);
    CHECK(mw.A().player.skill_point == 0);   // 0 + 5 - 100 -> 0
    const auto out = mw.w.take_outbox();
    int camps = 0, skill_lines = 0;
    for (const auto& p : out) {
        if (std::find(p.sids.begin(), p.sids.end(), 7) == p.sids.end()) continue;
        if (p.msg_id == jx::pb::G2C_ENTITY_CAMP) {
            jx::pb::EntityCamp m;
            REQUIRE(m.ParseFromString(p.payload));
            if (m.tmp_camp() == 3) ++camps;
        } else if (p.msg_id == jx::pb::G2C_SKILL_LEVEL) {
            jx::pb::SkillLevelSync m;
            REQUIRE(m.ParseFromString(p.payload));
            CHECK(m.skill_id() == 0);   // the 0x5e packet {0, -1, points}
            CHECK(m.level() == -1);
            ++skill_lines;
        }
    }
    CHECK(camps == 1);         // SetTmpCamp on a player: the line to itself only
    CHECK(skill_lines == 2);   // one per AddMagicPoint
    CHECK(mw.w.stat_data("boss_kill") == 5);
    CHECK(mw.w.stat_data("x") == 0);   // three arguments: nothing
    // the clock
    CHECK(std::fabs(*num("g_ct") - static_cast<double>(std::time(nullptr))) <= 5.0);
    std::tm tm{};
    tm.tm_year = 126;
    tm.tm_mon = 8;
    tm.tm_mday = 19;
    tm.tm_hour = 12;
    tm.tm_min = 30;
    tm.tm_sec = 15;
    CHECK(num("g_tm1") == static_cast<double>(std::mktime(&tm)));
    tm = std::tm{};
    tm.tm_year = 126;
    tm.tm_mon = 8;
    tm.tm_mday = 19;
    CHECK(num("g_tm2") == static_cast<double>(std::mktime(&tm)));
    tm = std::tm{};
    tm.tm_year = 126;
    tm.tm_mday = 1;   // a missing month is 0, a missing day 1
    CHECK(num("g_tm3") == static_cast<double>(std::mktime(&tm)));
    CHECK(mw.is("g_tm4", "nil"));
    {
        // strftime of localtime(t): the machine's zone and daylight rule decide the hour, so the expectation is computed the same way
        auto t1 = static_cast<std::time_t>(*num("g_tm1"));
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &t1);
#else
        localtime_r(&t1, &local);
#endif
        char expected[64];
        REQUIRE(std::strftime(expected, sizeof expected, "%Y-%m-%d %H:%M:%S", &local) > 0);
        CHECK(mw.is("g_fs1", expected));
    }
    CHECK(mw.is("g_fs2", "2026"));
    CHECK(mw.is("g_fs3", ""));
    // AddNpcEx: series and subworld explicit, the extras from the seventh argument
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s5_npc", mw.A(), static_cast<int>(mw.w.map_id())));
    const KNpc* n1 = entity("g_ne1");
    REQUIRE(n1 != nullptr);
    CHECK(n1->template_id == 900);
    CHECK(n1->level == 7);
    CHECK(n1->series == 2);
    CHECK(n1->name == "Heo Ex");
    CHECK(n1->remove_on_death);
    CHECK(n1->boss_flag == 3);
    CHECK(mw.w.to_absolute(n1->pos()).x == 2300);
    CHECK(n1->tmp_camp == 5);
    CHECK(n1->current_camp == 6);
    CHECK(n1->script == R"(\script\test\misc.lua)");   // SetNpcDeathScript is SetNpcScript (0x08101500 twice in the table)
    CHECK(n1->script_main_param == 3);
    const KNpc* n2 = entity("g_ne2");
    REQUIRE(n2 != nullptr);
    CHECK(n2->level == 1);     // a negative level
    CHECK(n2->series == 4);
    CHECK(n2->name == "boar");
    CHECK_FALSE(n2->remove_on_death);
    CHECK(n2->boss_flag == 0);
    CHECK(n2->current_camp != 7);   // above 6: refused
    CHECK(mw.is("g_ne3", "nil"));   // five arguments
    CHECK(mw.is("g_ne4", "0"));     // another subworld
    CHECK(mw.is("g_ne5", "0"));     // no such template
    const KNpc* n6 = entity("g_ne6");
    REQUIRE(n6 != nullptr);
    CHECK(n6->gold.is_gold);
    CHECK_FALSE(n6->remove_on_death);
    const KNpc* n7 = entity("g_ne7");
    REQUIRE(n7 != nullptr);
    CHECK(n7->remove_on_death);
    CHECK(n7->boss_flag == 0);
    CHECK(mw.is("g_np1", "0"));   // a monster is no player
    CHECK(num("g_np2") == static_cast<double>(mw.a.value));
    CHECK(mw.is("g_np3", "0"));
    CHECK(mw.is("g_ns1", "-1"));   // no skill table
    CHECK(mw.is("g_ns2", "-1"));   // four arguments
    CHECK(mw.is("g_ns3", "-1"));   // npc 0
    // AddTimer: fn(param, id) of this script when due; the results re-arm it
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s5_timers", mw.A(), 0));
    CHECK(num("g_t1") > 0.0);
    CHECK(num("g_t2") > 0.0);
    CHECK(num("g_t3") > 0.0);
    CHECK(mw.is("g_t4", "0"));    // an empty function name
    CHECK(mw.is("g_t5", "0"));    // two arguments
    CHECK(mw.is("g_dt1", "1"));
    CHECK(mw.is("g_dt2", "0"));   // gone already
    CHECK(mw.is("g_dt3", "0"));   // no argument
    CHECK(mw.w.script_timer_count() == 2);
    mw.w.tick();   // frame 1: the one-frame timer was deleted
    CHECK(mw.is("g_two", "nil"));
    CHECK(mw.is("g_ot", "nil"));
    mw.w.tick();   // frame 2: s5_OnTime(11, id) - nothing back: done
    CHECK(mw.is("g_ot", "1"));
    CHECK(mw.is("g_ot_param", "11"));
    CHECK(num("g_ot_id") == num("g_t1"));
    CHECK(mw.w.script_timer_count() == 1);
    mw.w.tick();   // frame 3: s5_OnTimeRepeat(22) -> 2, 23: again in two frames with 23
    CHECK(mw.is("g_rep", "1"));
    mw.w.tick();   // 4
    CHECK(mw.is("g_rep", "1"));
    mw.w.tick();   // 5: (23) -> 1: once more next frame
    CHECK(mw.is("g_rep", "2"));
    CHECK(mw.is("g_rep_param", "23"));
    mw.w.tick();   // 6: -> 0: done
    CHECK(mw.is("g_rep", "3"));
    CHECK(mw.w.script_timer_count() == 0);
    for (int i = 0; i < 4; ++i) mw.w.tick();
    CHECK(mw.is("g_rep", "3"));
    CHECK(mw.is("g_ot", "1"));
}

TEST_CASE("S6 script api: the object buffers, RemoteExecute, FileName2Id, SaveNow, WriteGoldLog, AddMapTrap, NpcChat, CastSkill and IL", "[scriptfuns][s6]")
{
    MiscWorld mw;
    jx::zone::KLuaScript* s = mw.w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto num = [&](const char* name) { return s->call_number("Num", {std::string(name)}); };
    auto entity = [&](const char* name) -> const KNpc* {
        const std::optional<double> v = num(name);
        return v.has_value() && *v > 0.0 ? mw.w.find_entity(jx::EntityId{static_cast<std::uint64_t>(*v)}) : nullptr;
    };
    // IL is IncludeLib
    CHECK(s->has_function("IL"));
    CHECK(s->has_function("IncludeLib"));
    // the object buffers
    const std::size_t buffers_before = jx::zone::g_ObjectBuffers().count();
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s6_ob", mw.A(), 0));
    CHECK(num("g_ob1") > 0.0);
    CHECK(num("g_ob2") > 0.0);
    CHECK(num("g_ob2") != num("g_ob1"));
    CHECK(mw.is("g_e1", "1"));    // fresh: empty
    CHECK(mw.is("g_pi", "1"));
    CHECK(mw.is("g_pd", "1"));
    CHECK(mw.is("g_pb", "1"));
    CHECK(mw.is("g_ps", "1"));
    CHECK(mw.is("g_e2", "0"));
    CHECK(mw.is("g_cp", "1"));
    CHECK(mw.is("g_ap", "1"));
    CHECK(mw.is("g_i1", "-7"));
    CHECK(mw.is("g_d1", "2.5"));
    CHECK(mw.is("g_b1", "200"));
    CHECK(mw.is("g_s1", "xin chao"));
    CHECK(mw.is("g_i2", "nil"));   // read out: nil
    CHECK(mw.is("g_e3", "1"));
    CHECK(mw.is("g_i3", "-7"));    // the copy, then the appended copy
    CHECK(mw.is("g_d3", "2.5"));
    CHECK(mw.is("g_b3", "200"));
    CHECK(mw.is("g_s3", "xin chao"));
    CHECK(mw.is("g_i4", "-7"));
    CHECK(mw.is("g_s4", "nil"));   // a double's bytes are no string length that fits
    CHECK(mw.is("g_e4", "1"));     // cleared
    CHECK(mw.is("g_r1", "1"));
    CHECK(mw.is("g_r2", "0"));     // gone already
    CHECK(mw.is("g_e5", "1"));     // unknown: empty
    CHECK(mw.is("g_pi2", "0"));    // unknown: refused
    CHECK(mw.is("g_pi3", "0"));    // one argument
    CHECK(mw.is("g_cp2", "nil"));  // one argument: nothing
    CHECK(mw.is("g_e6", "1"));
    CHECK(jx::zone::g_ObjectBuffers().count() == buffers_before + 1);   // g_ob2 still there
    // RemoteExecute: fn(in, out) at once, the callback with the out bytes
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s6_remote", mw.A(), 0));
    CHECK(mw.is("g_re1", "1"));
    CHECK(mw.is("g_re2", "1"));
    CHECK(mw.is("g_re3", "0"));    // an empty script path
    CHECK(mw.is("g_re4", "0"));    // two arguments
    CHECK(mw.is("g_re5", "1"));    // buffer 0: fn(empty, out)
    CHECK(mw.is("g_tg", "3"));
    CHECK(mw.is("g_tg_int", "nil"));   // the last call had no bytes
    CHECK(mw.is("g_tg_str", "nil"));
    CHECK(mw.is("g_bk_param", "9"));
    CHECK(mw.is("g_bk_int", "42"));    // 41 + 1 pushed by the target
    CHECK(mw.is("g_bk_left", "1"));
    CHECK(mw.is("g_re_left", "0"));    // the caller's buffer keeps its bytes (a copy went out)
    CHECK(jx::zone::g_ObjectBuffers().count() == buffers_before + 1);   // every in / out / back buffer released
    // FileName2Id, SaveNow, WriteGoldLog, CastSkill without a skill table
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s6_misc", mw.A(), 0));
    CHECK(num("g_f1") >= 1.0);
    CHECK(num("g_f2") == num("g_f1"));            // the same file, another spelling
    CHECK(num("g_f3") == *num("g_f1") + 1.0);     // the next slot
    CHECK(mw.is("g_f4", "0"));
    CHECK(mw.is("g_f5", "0"));
    CHECK(mw.A().player.save_now);
    CHECK(mw.w.take_save_requests().empty());     // gathered by the frame
    mw.w.tick();
    CHECK_FALSE(mw.A().player.save_now);
    CHECK(mw.w.take_save_requests() == std::vector<std::uint64_t>{7});
    CHECK(mw.w.take_save_requests().empty());
    CHECK(mw.is("g_cs", "nil"));
    // NpcChat: to the watchers at once, or after seconds * 18 frames while the npc lives
    REQUIRE(mw.w.execute_script(R"(\script\test\misc.lua)", "s2_npc", mw.A(), 0));
    const KNpc* n1 = entity("g_n1");
    REQUIRE(n1 != nullptr);
    mw.w.tick();
    mw.w.take_outbox();
    REQUIRE(mw.w.execute_script_args(R"(\script\test\misc.lua)", "s6_chat", mw.A(), {static_cast<double>(n1->id.value)}));
    CHECK(mw.is("g_nc1", "0"));       // the last argument comes back (nothing pushed)
    CHECK(mw.is("g_nc2", "1"));
    CHECK(mw.is("g_nc3", "nil"));     // an empty text: nothing
    CHECK(mw.is("g_nc4", "nil"));     // no such npc
    CHECK(mw.is("g_nc5", "nil"));     // one argument
    auto chats = [&](const std::vector<jx::zone::Packet>& out) {
        std::vector<std::string> lines;
        for (const auto& p : out) {
            if (p.msg_id != jx::pb::G2C_NPC_CHAT || std::find(p.sids.begin(), p.sids.end(), 7) == p.sids.end()) continue;
            jx::pb::NpcChat m;
            REQUIRE(m.ParseFromString(p.payload));
            CHECK(m.entity_id() == n1->id.value);
            lines.push_back(m.text());
        }
        return lines;
    };
    CHECK(chats(mw.w.take_outbox()) == std::vector<std::string>{"xin chao"});
    CHECK(mw.w.pending_npc_chats() == 1);
    for (int i = 0; i < 17; ++i) mw.w.tick();
    CHECK(chats(mw.w.take_outbox()).empty());
    mw.w.tick();
    CHECK(chats(mw.w.take_outbox()) == std::vector<std::string>{"lat nua"});
    CHECK(mw.w.pending_npc_chats() == 0);
}

TEST_CASE("S6 AddMapTrap: a trap cell a script adds runs its main() when a player steps on it", "[scriptfuns][s6][trap]")
{
    jx::zone::KSubWorldConfig cfg = misc_world();
    cfg.map = std::make_shared<jx::zone::KMapData>(jx::zone::KMapData::synthetic(128, 128));   // 32-cell grid: 4096 x 4096
    jx::zone::KSubWorld w(cfg);
    jx::log::Options o;
    o.console = false;
    o.default_level = jx::log::Level::warn;
    jx::log::init(o);
    jx::EntityId a;
    jx::zone::Pos at;
    REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
    w.tick();
    jx::zone::KLuaScript* trap = w.config().scripts->get(R"(\script\test\trap.lua)");
    REQUIRE(trap != nullptr);
    // the cell (70, 62): local (2256, 2000), given in world units
    const jx::zone::Pos cell = w.to_absolute(jx::zone::Pos{70 * 32 + 16, 62 * 32 + 16});
    REQUIRE(w.execute_script_args(R"(\script\test\misc.lua)", "s6_trap", *w.mutable_entity(a),
                                  {static_cast<double>(w.map_id()), static_cast<double>(cell.x), static_cast<double>(cell.y)}));
    jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\misc.lua)");
    REQUIRE(s != nullptr);
    auto is = [&](const char* name, const char* expected) { return s->call_number("Is", {std::string(name), std::string(expected)}) == 1.0; };
    CHECK(is("g_tr1", "1"));
    CHECK(is("g_tr2", "0"));     // another subworld
    CHECK(is("g_tr3", "nil"));   // three arguments
    CHECK(is("g_tr4", "0"));     // a script id nobody asked for
    CHECK(is("g_tr5", "1"));     // by the id FileName2Id gave
    CHECK(is("g_tr6", "0"));     // outside the map
    CHECK(w.script_trap_count() == 2);
    CHECK_FALSE(trap->call_number("Num", {std::string("g_trap")}).has_value());
    REQUIRE(w.set_pos(a, jx::zone::Pos{70 * 32 + 16, 62 * 32 + 16}));
    w.tick();
    CHECK(trap->call_number("Num", {std::string("g_trap")}) == 1.0);
    w.tick();
    CHECK(trap->call_number("Num", {std::string("g_trap")}) == 1.0);   // standing there: once
    REQUIRE(w.set_pos(a, jx::zone::Pos{2000, 2000}));
    w.tick();
    REQUIRE(w.set_pos(a, jx::zone::Pos{71 * 32 + 16, 62 * 32 + 16}));   // the second cell
    w.tick();
    CHECK(trap->call_number("Num", {std::string("g_trap")}) == 2.0);
}
