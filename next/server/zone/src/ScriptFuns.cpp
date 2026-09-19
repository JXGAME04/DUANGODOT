// The script api of the old server (Core/Src/ScriptFuns.cpp) for the traps: positions are in
// cells of 32 units like the old Lua* functions (SetPos(x, y) -> SetPos(x * 32, y * 32)).
#include "jx/zone/ScriptFuns.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "jx/log.hpp"
#include "jx/zone/KItem.h"
#include "jx/zone/KPlayerDialog.h"
#include "jx/zone/KPlayerEvent.h"
#include "jx/zone/KPlayerTask.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSkillList.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KTabFile.h"
#include "jx/zone/KText.h"
#include "jx/zone/KTaskManager.h"

namespace jx::zone {

KScriptContext& g_ScriptContext() noexcept
{
    // MASTER SPEC 42: simulation workers run scripts at the same time, so the "who is this
    // script running for" context belongs to the thread, never to the process.
    static thread_local KScriptContext ctx;
    return ctx;
}

namespace {

// GetPlayerIndex(L) of the old code: the player the script runs for, or nothing.
KNpc* player_of(lua_State* L, const char* fn)
{
    KScriptContext& c = g_ScriptContext();
    if (c.world == nullptr || c.player == nullptr) {
        log::warn("lua", "script api called without a player", {log::kv("function", fn)});
        (void)L;
        return nullptr;
    }
    return c.player;
}

// GetFightState() -> 0 / 1
int l_GetFightState(lua_State* L)
{
    const KNpc* p = player_of(L, "GetFightState");
    lua_pushinteger(L, p != nullptr && p->fight_mode ? 1 : 0);
    return 1;
}

// SetFightState(n): KNpc::SetFightMode
int l_SetFightState(lua_State* L)
{
    if (KNpc* p = player_of(L, "SetFightState")) {
        p->fight_mode = luaL_checknumber(L, 1) != 0;
        log::debug("lua", "fight state", {log::kv("entity", p->id), log::kv("fight", p->fight_mode)});
    }
    return 0;
}

// SetPos(x, y): KNpc::SetPos(x * 32, y * 32)
int l_SetPos(lua_State* L)
{
    if (lua_gettop(L) != 2) return 0;
    if (KNpc* p = player_of(L, "SetPos")) {
        const auto x = static_cast<std::int32_t>(luaL_checknumber(L, 1));
        const auto y = static_cast<std::int32_t>(luaL_checknumber(L, 2));
        KSubWorld* w = g_ScriptContext().world;
        w->set_pos(p->id, w->to_local(Pos{x * 32, y * 32}));   // absolute Mps -> this map
    }
    return 0;
}

// NewWorld(map, x, y): KNpc::ChangeWorld(map, x * 32, y * 32) -> 1 done, 2 another server, 0 failed
int l_NewWorld(lua_State* L)
{
    int result = 0;
    if (lua_gettop(L) >= 3) {
        if (KNpc* p = player_of(L, "NewWorld")) {
            const auto map = static_cast<std::uint32_t>(luaL_checknumber(L, 1));
            const auto x = static_cast<std::int32_t>(luaL_checknumber(L, 2));
            const auto y = static_cast<std::int32_t>(luaL_checknumber(L, 3));
            result = g_ScriptContext().world->change_world_request(*p, map, Pos{x * 32, y * 32});
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

// GetPos() -> x, y (cells), subworld index
int l_GetPos(lua_State* L)
{
    const KNpc* p = player_of(L, "GetPos");
    if (p == nullptr) return 0;
    const Pos a = g_ScriptContext().world->to_absolute(p->pos());
    lua_pushinteger(L, a.x / 32);
    lua_pushinteger(L, a.y / 32);
    lua_pushinteger(L, 0);
    return 3;
}

// GetWorldPos() -> map id, x, y (cells)
int l_GetWorldPos(lua_State* L)
{
    const KNpc* p = player_of(L, "GetWorldPos");
    if (p == nullptr) return 0;
    const Pos a = g_ScriptContext().world->to_absolute(p->pos());
    lua_pushinteger(L, g_ScriptContext().world->map_id());
    lua_pushinteger(L, a.x / 32);
    lua_pushinteger(L, a.y / 32);
    return 3;
}

// Earn(n) (0x08118970): n > 0 -> KPlayer::Earn; the money log line "Lua_Earn" (0x081E8EA0), and a sum above 99 999
// also writes the script's call stack (10 levels) to the log.  Returns nothing.
int l_Earn(lua_State* L)
{
    if (KNpc* p = player_of(L, "Earn")) {
        const int n = static_cast<int>(luaL_checknumber(L, 1));
        if (n > 0 && g_ScriptContext().world->earn(g_ScriptContext().sid, n)) {
            log::info("zone.money", "script money", {log::kv("entity", p->id), log::kv("reason", "Lua_Earn"), log::kv("amount", n),
                                                     log::kv("money", g_ScriptContext().world->cash(g_ScriptContext().sid))});
        }
    }
    return 0;
}

// Pay(n) (0x08118A90) -> 1 paid (the log line "Lua_Pay"), 0 not (less in the bag); n <= 0 returns nothing
int l_Pay(lua_State* L)
{
    KNpc* p = player_of(L, "Pay");
    if (p == nullptr) return 0;
    const int n = static_cast<int>(luaL_checknumber(L, 1));
    if (n <= 0) return 0;
    const bool ok = g_ScriptContext().world->pay(g_ScriptContext().sid, n);
    if (ok) {
        log::info("zone.money", "script money", {log::kv("entity", p->id), log::kv("reason", "Lua_Pay"), log::kv("amount", -n),
                                                 log::kv("money", g_ScriptContext().world->cash(g_ScriptContext().sid))});
    }
    lua_pushnumber(L, ok ? 1 : 0);
    return 1;
}

// GetCash() (0x081116D0) -> the bag's money, Player+0x508c
int l_GetCash(lua_State* L)
{
    if (player_of(L, "GetCash") == nullptr) return 0;
    lua_pushnumber(L, g_ScriptContext().world->cash(g_ScriptContext().sid));
    return 1;
}

// Msg2Player(text): a line in the player's chat window
int l_Msg2Player(lua_State* L)
{
    const char* text = luaL_optstring(L, 1, "");
    if (player_of(L, "Msg2Player") != nullptr) g_ScriptContext().world->msg_to_player(g_ScriptContext().sid, text);
    return 0;
}

// AddStation / AddTermini: the travel stations of the player (KPlayer::AddWayPoint); not kept yet
int l_AddStation(lua_State* L)
{
    log::trace("lua", "AddStation ignored", {log::kv("station", luaL_optinteger(L, 1, 0))});
    return 0;
}

int l_AddTermini(lua_State* L)
{
    log::trace("lua", "AddTermini ignored", {log::kv("termini", luaL_optinteger(L, 1, 0))});
    return 0;
}

// Say(sentence, count, answer1, answer2, ... | {answers}) (LuaSelectUI; jx_linux_y 0x08123C90): the sentence a string or a
// number (a string-table id, m_bParam1 = 1); the count a number (else 0 answers); the answers as more strings or as a
// table at 3 (a string at 3 = the vararg form; neither with count > 0 = nothing); the count is clamped to the arguments
// and to 50; each answer "text/function" - the function runs when the client picks it (KSubWorld::dialog_say)
int l_Say(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "Say");
    if (p == nullptr || n < 1) return 0;
    int count = 0;
    if (n != 1 && lua_type(L, 2) == LUA_TNUMBER) count = static_cast<int>(lua_tonumber(L, 2));   // 0x08123CEA / 0x08123FA6
    std::string text;
    int text_id = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {   // 0x08124058
        text_id = static_cast<int>(lua_tonumber(L, 1));
    } else if (lua_isstring(L, 1)) {   // 0x08123D3D
        text = lua_tostring(L, 1);
    } else {
        return 0;
    }
    bool from_table = false;
    if (lua_isstring(L, 3)) {   // 0x08123D8B: the vararg form
        from_table = false;
    } else if (lua_type(L, 3) == LUA_TTABLE) {   // 0x08124133
        from_table = true;
    } else if (count > 0) {   // 0x08124146: answers promised, none given
        return 0;
    }
    if (!from_table && n != 1 && count >= n - 1) count = n - 2;   // 0x08123D9D / 0x08124158
    if (count < 0) count = 0;
    if (count > kDialogAnswers) count = kDialogAnswers;   // 0x08123DBC
    std::vector<std::string> answers;
    answers.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const char* a = nullptr;
        if (from_table) {   // 0x08123E38: t[i + 1]
            lua_rawgeti(L, 3, i + 1);
            a = lua_tostring(L, -1);
            answers.emplace_back(a != nullptr ? a : "");   // 0x08123F88: a missing answer is empty (and runs "main")
            lua_pop(L, 1);
        } else {
            a = lua_tostring(L, i + 3);
            answers.emplace_back(a != nullptr ? a : "");
        }
    }
    g_ScriptContext().world->dialog_say(*p, text, text_id, answers);
    return 0;
}

// Talk(count, callback, page1, page2, ...) (LuaTalkUI; jx_linux_y 0x08116930): fewer than three arguments = nothing;
// the count a number (else nothing), clamped to the pages given; the callback a string ("" = none): the function run
// when the last page is confirmed; a page a string, or a number printed with "%d" (KSubWorld::dialog_talk)
int l_Talk(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "Talk");
    if (p == nullptr || n <= 2) return 0;   // 0x0811694D
    if (lua_type(L, 1) != LUA_TNUMBER) return 0;   // 0x081169AD
    int count = static_cast<int>(lua_tonumber(L, 1));
    if (count >= n - 1) count = n - 2;   // 0x08116A01
    const char* cb = lua_tostring(L, 2);
    const std::string callback = cb != nullptr ? cb : "";
    if (lua_type(L, 3) != LUA_TNUMBER && !lua_isstring(L, 3)) return 0;   // 0x08116A46 / 0x08116A5B
    std::vector<std::string> pages;
    for (int i = 0; i < count; ++i) {
        const int at = 3 + i;
        if (lua_type(L, at) == LUA_TNUMBER) {   // 0x08116B76: "%d"
            pages.push_back(std::to_string(static_cast<long long>(lua_tonumber(L, at))));
        } else {
            const char* s = lua_tostring(L, at);
            if (s == nullptr) break;   // 0x08116ADE
            pages.emplace_back(s);
        }
    }
    g_ScriptContext().world->dialog_talk(*p, callback, pages);
    return 0;
}

// Describe(text | id, count, answer... | {answers}) (jx_linux_y 0x081242A0; no 2003 counterpart): Say's shape, shown
// by the client in its npc description window (ui 12).  Two arguments at least (0x081242B7); the count must be a number,
// else the binary prints "Describe(%s,%s) by %s" (0x08124377) and sends nothing; the sentence a string or a string-
// table id (0x081243B6 / 0x08124780); the answers as a table (0x08124443: t[i + 1]) or as more strings (0x08124828:
// count >= n - 1 -> n - 2); a count above 0 with no answer at all -> nothing (0x08124460); at most 50 (0x08124478)
int l_Describe(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "Describe");
    if (p == nullptr || n <= 1) return 0;
    if (lua_type(L, 2) != LUA_TNUMBER) {   // 0x08124315
        const char* a = lua_tostring(L, 1);
        const char* b = lua_tostring(L, 2);
        log::debug("lua", "describe without count", {log::kv("entity", p->id), log::kv("text", text::decode_mixed(a != nullptr ? a : "")),
                                                       log::kv("count", text::decode_mixed(b != nullptr ? b : ""))});
        return 0;
    }
    int count = static_cast<int>(lua_tonumber(L, 2));
    std::string text;
    int text_id = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {   // 0x08124780
        text_id = static_cast<int>(lua_tonumber(L, 1));
    } else if (lua_isstring(L, 1)) {   // 0x081243CA
        text = lua_tostring(L, 1);
    } else {
        return 0;
    }
    bool from_table = false;
    if (lua_type(L, 3) == LUA_TTABLE) {   // 0x08124446
        from_table = true;
    } else if (lua_isstring(L, 3)) {   // 0x0812445A -> 0x08124828
        if (count >= n - 1) count = n - 2;
    } else if (count > 0) {   // 0x08124460
        return 0;
    }
    if (count < 0) count = 0;
    if (count > kDialogAnswers) count = kDialogAnswers;   // 0x08124478
    std::vector<std::string> answers;
    answers.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const char* a = nullptr;
        if (from_table) {   // 0x08124518: t[i + 1]
            lua_rawgeti(L, 3, i + 1);
            a = lua_tostring(L, -1);
            answers.emplace_back(a != nullptr ? a : "");   // 0x081246F0: a missing answer is empty (and runs "main")
            lua_pop(L, 1);
        } else {
            a = lua_tostring(L, i + 3);   // 0x08124682
            answers.emplace_back(a != nullptr ? a : "");
        }
    }
    g_ScriptContext().world->dialog_describe(*p, text, text_id, answers);
    return 0;
}

// TaskTip(text) (0x08122730): a player 1..0x4af (0x0812275B), the text a string (0x0812277B), the 0xb6 packet (KSubWorld::
// task_tip).  The binary answers 1 without pushing anything (0x081227F2: the top value comes back); the one caller
// (task/random/task_head.lua) ignores it - nothing is returned here.
int l_TaskTip(lua_State* L)
{
    KNpc* p = player_of(L, "TaskTip");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const char* s = lua_tostring(L, 1);
    if (s == nullptr) return 0;
    g_ScriptContext().world->task_tip(*p, s);
    return 0;
}

// WriteLog(text) (0x081237D0): a string (0x081237F6) -> the script log 0x977ff60 (opened at 0x0805DF29 as
// Logs/KSG_ScriptLog<date>.txt): "%04d-%02d-%02d %02d:%02d:%02d\t" (0x0821D3A0), the text, "\r\n" (0x0821D6F0).  The
// zone's log (a line of the zone.script channel) is that file.
int l_WriteLog(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    const char* s = lua_tostring(L, 1);
    if (s == nullptr) return 0;
    const KScriptContext& c = g_ScriptContext();
    log::info("zone.script", "script log", {log::kv("text", text::decode_mixed(s)), log::kv("script", c.script_path)});
    return 0;
}

// GetAccount() -> the account's name (0x0810F6A0: strcpy of Player+0x264 when the player index is above 0, else "")
int l_GetAccount(lua_State* L)
{
    const KScriptContext& c = g_ScriptContext();
    const KNpc* p = c.world != nullptr ? c.player : nullptr;
    if (p == nullptr) {
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushlstring(L, p->player.account.data(), p->player.account.size());
    return 1;
}

// AddOwnExp(exp) (0x081126C0, LuaAddOwnExp of the 2003 source): a player above index 0 (0x081126E7), the amount as an
// int64 (0x08112709) - a negative one never reaches the core (0x08112717) - -> KPlayer 0x080AFEA0 (no CalcExp, no bonus)
int l_AddOwnExp(lua_State* L)
{
    KNpc* p = player_of(L, "AddOwnExp");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const double v = lua_tonumber(L, 1);
    if (v < 0) return 0;
    g_ScriptContext().world->give_player_exp_direct(*p, static_cast<std::int64_t>(v));
    return 0;
}

// AddRepute(n) (0x08117290, LuaModifyRepute of the 2003 source: TASKVALUE_REPUTE = 100): the task value 100 + n
// through KPlayer::SetTaskValue(100, v, 1) (0x0811732E) when the sum is not negative (0x08117315); the sprintf'd
// sentences of the binary (0x978a494 / 0x978a490) go to a local buffer nobody reads
int l_AddRepute(lua_State* L)
{
    KNpc* p = player_of(L, "AddRepute");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int delta = static_cast<int>(lua_tonumber(L, 1));
    const int value = p->player.task.get_save_val(kTaskRepute) + delta;
    if (value < 0) return 0;
    g_ScriptContext().world->task_set_value(*p, kTaskRepute, value, true);
    log::debug("zone.task", "repute changed", {log::kv("entity", p->id), log::kv("delta", delta), log::kv("value", value)});
    return 0;
}

// GetRepute() -> the task value 100 (0x08117230; 0 without a player, 0x08117242)
int l_GetRepute(lua_State* L)
{
    const KScriptContext& c = g_ScriptContext();
    const KNpc* p = c.world != nullptr ? c.player : nullptr;
    lua_pushinteger(L, p != nullptr ? p->player.task.get_save_val(kTaskRepute) : 0);
    return 1;
}

// The string buffer of the scripts (0x9780d54 / 0x9780d58 / 0x9780d5c of jx_linux_y: PushString 0x0812FDA0, AppendString
// 0x0812FCD0, ReplaceString 0x0812EB20, PopString 0x080FFB00; script/lib/basic.lua joins strings through it).  The old
// server has one; the zone runs scripts on its workers at the same time, so each thread has its own.
std::string& script_string_buffer() noexcept
{
    static thread_local std::string buffer;
    return buffer;
}

// PushString(s): the buffer emptied (0x0812FDDB) then s copied in (0x0812FE1F); a missing s empties it (0x0812FE70)
int l_PushString(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    std::string& b = script_string_buffer();
    b.clear();
    if (const char* s = lua_tostring(L, 1); s != nullptr) b = s;
    return 0;
}

// AppendString(s): s after what is there (0x0812FD4D; the buffer grows, 0x0812FD84); nothing for a missing or empty s
int l_AppendString(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    if (const char* s = lua_tostring(L, 1); s != nullptr) script_string_buffer() += s;
    return 0;
}

// ReplaceString(pattern, s): two strings (0x0812EB60 / 0x0812EB6E), a buffer with something in it (0x0812EB7B) and a
// pattern with a length (0x0812EB9B); the buffer is walked once, left to right: a match of the pattern (strncmp,
// 0x0812EBE4) puts s in the copy and skips the pattern, any other byte is copied (0x0812ECC8), the tail shorter than
// the pattern is copied whole (0x0812EC67); then the copy becomes the buffer (0x0812EC6C..0x0812ECAC)
int l_ReplaceString(lua_State* L)
{
    if (lua_gettop(L) < 2) return 0;
    const char* pattern = lua_tostring(L, 1);
    const char* with = lua_tostring(L, 2);
    std::string& b = script_string_buffer();
    if (pattern == nullptr || with == nullptr || b.empty() || pattern[0] == '\0') return 0;
    const std::string_view pat(pattern);
    std::string out;
    out.reserve(b.size());
    std::size_t at = 0;
    while (at < b.size()) {
        if (b.size() - at >= pat.size() && b.compare(at, pat.size(), pat) == 0) {
            out += with;
            at += pat.size();
        } else {
            out += b[at++];
        }
    }
    b.swap(out);
    return 0;
}

// PopString() -> the buffer, "" when empty (0x080FFB0B); the buffer stays as it is
int l_PopString(lua_State* L)
{
    const std::string& b = script_string_buffer();
    lua_pushlstring(L, b.data(), b.size());
    return 1;
}

// GiveItemUI(title, content | id, confirm, cancel [, param [, select [, notify]]]) (jx_linux_y 0x0812BBA0): three arguments at
// least (0x0812BBC0); the title a string (0x0812BC34), the content a string or a string-table id (0x0812BC4B / 0x0812BC5F,
// else nothing); the confirm function (the third) empty -> the answers cleared, nothing sent (0x0812BD0A); the cancel
// function (the fourth), the fifth a number into +0x52b8 (0x0812C0B0), the sixth a function into +0x60a0 (0x0812C131), the
// seventh a number into the packet's +8 (0x0812C180: the client reports the box's changes)
int l_GiveItemUI(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "GiveItemUI");
    if (p == nullptr || n <= 2) return 0;
    const char* title = lua_tostring(L, 1);
    std::string content;
    int text_id = 0;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        text_id = static_cast<int>(lua_tonumber(L, 2));
    } else if (lua_isstring(L, 2)) {
        content = lua_tostring(L, 2);
    } else {
        return 0;
    }
    const char* confirm = lua_tostring(L, 3);
    const char* cancel = lua_tostring(L, 4);
    const int param = n > 4 ? static_cast<int>(lua_tonumber(L, 5)) : 0;
    const char* select = n > 5 ? lua_tostring(L, 6) : nullptr;
    const bool notify = n > 6 && static_cast<int>(lua_tonumber(L, 7)) != 0;
    g_ScriptContext().world->dialog_give_item_ui(*p, title != nullptr ? title : "", content, text_id, confirm != nullptr ? confirm : "",
                                                 cancel != nullptr ? cancel : "", param, select != nullptr ? select : "", notify);
    return 0;
}

// GetGiveItemUnit(n) -> the item index in cell n of the last report (0x08114E60: a player above 0 and n in 1..24 -> Player+0x2b4[n - 1];
// else nothing)
int l_GetGiveItemUnit(lua_State* L)
{
    const KNpc* p = player_of(L, "GetGiveItemUnit");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int n = static_cast<int>(lua_tonumber(L, 1));
    if (n <= 0 || n > kGiveItemUnits) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(p->player.give.units[static_cast<std::size_t>(n - 1)]));
    return 1;
}

// GetGiveItemUnitWithPos(n) -> the item index and the cell code (y * 6 + x + 1) (0x08114D90: Player+0x2b4[n - 1], +0x314[n - 1])
int l_GetGiveItemUnitWithPos(lua_State* L)
{
    const KNpc* p = player_of(L, "GetGiveItemUnitWithPos");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int n = static_cast<int>(lua_tonumber(L, 1));
    if (n <= 0 || n > kGiveItemUnits) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(p->player.give.units[static_cast<std::size_t>(n - 1)]));
    lua_pushinteger(L, p->player.give.cells[static_cast<std::size_t>(n - 1)]);
    return 2;
}

// SetUiGiveItemMsg(text) (0x0810B020) / SetUiGiveItemMoreConfirmMsg(text) (0x0810AF50): a string and a player 1..0x4af -> the
// 0xd8 / 0xdf packet (KSubWorld::give_item_msg)
int give_item_msg(lua_State* L, const char* fn, int kind)
{
    KNpc* p = player_of(L, fn);
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const char* s = lua_tostring(L, 1);
    if (s == nullptr) return 0;
    g_ScriptContext().world->give_item_msg(*p, kind, s);
    return 0;
}
int l_SetUiGiveItemMsg(lua_State* L) { return give_item_msg(L, "SetUiGiveItemMsg", 0); }
int l_SetUiGiveItemMoreConfirmMsg(lua_State* L) { return give_item_msg(L, "SetUiGiveItemMoreConfirmMsg", 1); }

// AddNote(text | id [, param]) (jx_linux_y 0x08124DC0; 680 uses in data/script): the player's index not negative, one
// argument at least (0x08124DEF); a number is a string-table id (0x08124E04), else a string (0x08124E1A, anything else ->
// nothing); two or more arguments -> the second is the number after the text (0x08124E7C); the 0x63 packet with the ui id 3
int l_AddNote(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "AddNote");
    if (p == nullptr || n <= 0) return 0;
    std::string text;
    int text_id = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        text_id = static_cast<int>(lua_tonumber(L, 1));
    } else if (lua_isstring(L, 1)) {
        text = lua_tostring(L, 1);
    } else {
        return 0;
    }
    const int param = n != 1 ? static_cast<int>(lua_tonumber(L, 2)) : 0;
    g_ScriptContext().world->dialog_add_note(*p, text, text_id, param);
    return 0;
}

// AskClientForNumber(fn, min, max, title) (jx_linux_y 0x08115CA0; 247 uses with AskClientForString): the player's index not
// negative, four arguments at least (0x08115CDC); fn (1) and title (4) strings, min (2) and max (3) numbers, else nothing
// (0x08115D16..0x08115D95); the 0xa3 packet with the kind 1; the number typed comes back through the 0x82 packet (kind 3)
// into fn(number)
int l_AskClientForNumber(lua_State* L)
{
    KNpc* p = player_of(L, "AskClientForNumber");
    if (p == nullptr) return 0;
    p->player.dialog.waiting = false;   // 0x08115CCC: before the argument checks
    if (lua_gettop(L) <= 3) return 0;
    if (!lua_isstring(L, 1) || !lua_isstring(L, 4) || lua_type(L, 2) != LUA_TNUMBER || lua_type(L, 3) != LUA_TNUMBER) return 0;
    const char* fn = lua_tostring(L, 1);
    const char* title = lua_tostring(L, 4);
    g_ScriptContext().world->dialog_ask_client(*p, 1, fn != nullptr ? fn : "", static_cast<int>(lua_tonumber(L, 2)), static_cast<int>(lua_tonumber(L, 3)),
                                               title != nullptr ? title : "", "");
    return 0;
}

// AskClientForString(default, fn, min, max, title) (0x08115E90): five arguments at least (0x08115ECC); default (1), fn (2) and
// title (5) strings, min (3) and max (4) numbers; the 0xa3 packet with the kind 0 and the default; the text typed comes back
// through the 0x82 packet (kind 2) into fn(text)
int l_AskClientForString(lua_State* L)
{
    KNpc* p = player_of(L, "AskClientForString");
    if (p == nullptr) return 0;
    p->player.dialog.waiting = false;   // 0x08115EBC
    if (lua_gettop(L) <= 4) return 0;
    if (!lua_isstring(L, 2) || !lua_isstring(L, 1) || !lua_isstring(L, 5) || lua_type(L, 3) != LUA_TNUMBER || lua_type(L, 4) != LUA_TNUMBER) return 0;
    const char* def = lua_tostring(L, 1);
    const char* fn = lua_tostring(L, 2);
    const char* title = lua_tostring(L, 5);
    g_ScriptContext().world->dialog_ask_client(*p, 0, fn != nullptr ? fn : "", static_cast<int>(lua_tonumber(L, 3)), static_cast<int>(lua_tonumber(L, 4)),
                                               title != nullptr ? title : "", def != nullptr ? def : "");
    return 0;
}

// AddItem(genre, detail, particular, level, series, luck [, magic1 [, magic2 .. magic6]]) -> 1 / 0
//
// LuaAddItem of the old ScriptFuns.cpp, and jx_linux_y 0x08120D30 -> 0x08120B30: fewer than six
// numbers is 0; the JX2 build puts the current table version (g_SubWorldSet+0x34), a zero seed and
// a zero in front and hands the nine to Lua_NewItem (0x0811F230) -> KItemSet::Add(genre, series,
// level, luck, detail, particular, magic levels...), the same order as the source.  The item
// goes to the first free spot of the bag; a full bag left it on the ground in the old server -
// that comes with the drops (M11 D), until then it is 0.  The magic prefix / suffix levels roll
// the attributes through Gen_MagicAttrib with luck 0 (Lua_NewItem passes no luck).
// Lua_NewItem 0x0811F230 -> KItemSet::Add 0x0806E110(set, genre, quality, series, level, luck, detail, particular, levels, version, seed, ctx,
// flag, 0, bNew 1): a version 0 / -1 is the current table set (the JX2 build refuses one above 4, 0x0811F3B9); a seed
// other than 0 is put into the random generator before the roll (0x0811F824, the old seed back after) so the same piece
// comes out again.  Only quality 0 (a plain roll) exists here; gold pieces have AddGoldItem.
std::optional<KItem> make_script_item(KSubWorld* w, std::uint32_t version, std::uint32_t seed, int quality, int genre, int detail, int particular,
                                      int level, int series, int luck, const KMagicLevels* levels)
{
    if (quality != 0) return std::nullopt;
    auto gen = w->item_generator(version);
    if (!gen) return std::nullopt;
    if (seed != 0) gen->seed(seed);
    switch (static_cast<KItemGenre>(genre)) {
    case KItemGenre::equip: return gen->equipment(detail, particular, series, level, levels, luck);
    case KItemGenre::medicine: return gen->medicine(detail, level);
    case KItemGenre::task: return gen->quest(detail, 1);
    case KItemGenre::town_portal: return gen->town_portal();
    case KItemGenre::magic_script: return gen->magic_script(detail, particular, level, series, 1);
    default: return std::nullopt;
    }
}

int l_AddItem(lua_State* L)
{
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "AddItem");
    if (p == nullptr || n < 6) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const auto genre = static_cast<int>(luaL_checknumber(L, 1));
    const auto detail = static_cast<int>(luaL_checknumber(L, 2));
    const auto particular = static_cast<int>(luaL_checknumber(L, 3));
    const auto level = static_cast<int>(luaL_checknumber(L, 4));
    const auto series = static_cast<int>(luaL_checknumber(L, 5));
    const auto luck = static_cast<int>(luaL_checknumber(L, 6));
    KMagicLevels levels{};
    bool with_magic = false;
    for (int i = 0; i < 6 && 7 + i <= n; ++i) {
        levels[static_cast<std::size_t>(i)] = static_cast<int>(luaL_optnumber(L, 7 + i, 0));
        with_magic = with_magic || levels[static_cast<std::size_t>(i)] != 0;
    }
    KSubWorld* w = g_ScriptContext().world;
    std::optional<KItem> item = make_script_item(w, w->item_version(), 0, 0, genre, detail, particular, level, series, luck, with_magic ? &levels : nullptr);
    if (!item) {
        log::warn("lua", "AddItem: no such item", {log::kv("genre", genre), log::kv("detail", detail), log::kv("particular", particular),
                                                    log::kv("level", level), log::kv("series", series), log::kv("luck", luck)});
        lua_pushinteger(L, 0);
        return 1;
    }
    const std::uint32_t id = w->give_item(g_ScriptContext().sid, std::move(*item));
    if (id == 0) log::info("lua", "AddItem: bag full", {log::kv("entity", p->id), log::kv("genre", genre), log::kv("detail", detail)});
    lua_pushinteger(L, id != 0 ? 1 : 0);
    return 1;
}

// AddItemEx([tag,] version, seed, quality, genre, detail, particular, level, series, luck [, m1 .. m6]) -> the item's index / 0
// (jx_linux_y 0x08120470): a leading string is dropped (lua_remove, 0x081205B5); fewer than nine numbers -> the sentence
// 0x978a464 printed and 0 (0x08120560); a player above 0; Lua_NewItem 0x0811F230 with the nine and the levels (the
// scripts' tab_vn_fy_ring: AddItemEx(4, 15788, 0, 0, 3, 0, 6, 0, 200, 6, 6, 6, 6, 6, 6) = version 4, seed 15788, a plain
// ring of level 6 with luck 200 and six levels of 6); KPlayer::AddItem 0x080B5180(player, idx, 1, 1, 0) into the bag -> the
// index (0x08120580), else the piece is freed (KItemSet::Remove 0x0806DB90) and 0 comes back
int l_AddItemEx(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TSTRING) lua_remove(L, 1);   // 0x08120598: the tag
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "AddItemEx");
    if (n <= 8) {   // 0x081204A8
        log::warn("lua", "AddItemEx: too few arguments", {log::kv("count", n)});
        lua_pushinteger(L, 0);
        return 1;
    }
    if (p == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const auto version = static_cast<std::int64_t>(lua_tonumber(L, 1));
    const auto seed = static_cast<std::uint32_t>(static_cast<std::int64_t>(lua_tonumber(L, 2)));
    const auto quality = static_cast<int>(lua_tonumber(L, 3));
    const auto genre = static_cast<int>(lua_tonumber(L, 4));
    const auto detail = static_cast<int>(lua_tonumber(L, 5));
    const auto particular = static_cast<int>(lua_tonumber(L, 6));
    const auto level = static_cast<int>(lua_tonumber(L, 7));
    const auto series = static_cast<int>(lua_tonumber(L, 8));
    const auto luck = static_cast<int>(lua_tonumber(L, 9));
    KMagicLevels levels{};
    bool with_magic = false;
    for (int i = 0; i < 6 && 10 + i <= n; ++i) {
        levels[static_cast<std::size_t>(i)] = static_cast<int>(luaL_optnumber(L, 10 + i, 0));
        with_magic = with_magic || levels[static_cast<std::size_t>(i)] != 0;
    }
    KSubWorld* w = g_ScriptContext().world;
    const std::uint32_t set_version = version <= 0 ? w->item_version() : static_cast<std::uint32_t>(version);
    std::optional<KItem> item = make_script_item(w, set_version, seed, quality, genre, detail, particular, level, series, luck, with_magic ? &levels : nullptr);
    if (!item) {
        log::warn("lua", "AddItem: no such item", {log::kv("function", "AddItemEx"), log::kv("version", set_version), log::kv("quality", quality),
                                                    log::kv("genre", genre), log::kv("detail", detail), log::kv("particular", particular),
                                                    log::kv("level", level), log::kv("series", series), log::kv("luck", luck)});
        lua_pushinteger(L, 0);
        return 1;
    }
    const std::uint32_t id = w->give_item(g_ScriptContext().sid, std::move(*item));
    if (id == 0) log::info("lua", "AddItem: bag full", {log::kv("entity", p->id), log::kv("genre", genre), log::kv("detail", detail)});
    lua_pushinteger(L, static_cast<lua_Integer>(id));
    return 1;
}

// the item of the player by its index (the ids of the player's list are the item indices the scripts pass around)
KItem* script_item(lua_State* L, const char* fn, int arg = 1)
{
    const KNpc* p = player_of(L, fn);
    if (p == nullptr || lua_gettop(L) < arg) return nullptr;
    const auto idx = static_cast<std::int64_t>(lua_tonumber(L, arg));
    if (idx <= 0 || idx > 0xffffffffLL) return nullptr;
    KItemList* list = g_ScriptContext().world->items_of(g_ScriptContext().sid);
    return list != nullptr ? list->find_mutable(static_cast<std::uint32_t>(idx)) : nullptr;
}

// GetItemProp(idx) -> genre, detail, particular, level, series, luck (0x080FF260: Item+0, +8, +0xc, +0x24, +0x28, +0x200 of the
// Item[] table 0x830d300; an index outside 1..max -> one 0)
int l_GetItemProp(lua_State* L)
{
    const KItem* it = script_item(L, "GetItemProp");
    if (it == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, static_cast<int>(it->genre));
    lua_pushinteger(L, it->detail);
    lua_pushinteger(L, it->particular);
    lua_pushinteger(L, it->level);
    lua_pushinteger(L, it->series);
    lua_pushinteger(L, it->luck);
    return 6;
}

// SyncItem(idx) (0x08114EF0): a player above 0 and an index above 0 -> KItemList::SyncItem 0x081FB9A0 (the item as it is now to
// the client: the 0x5f packet of 0x081F9430 with durability, +0x344 / +0x345, the bind time) - G2C_ITEM_ADD here
int l_SyncItem(lua_State* L)
{
    const KItem* it = script_item(L, "SyncItem");
    if (it == nullptr) return 0;
    g_ScriptContext().world->sync_item(g_ScriptContext().sid, it->id);
    return 0;
}

// RemoveItemByIndex(idx) -> 1 / 0 (0x08114F80): KItemList::Remove 0x082006B0 (a worn piece comes off first), then
// KItemSet::Remove 0x0806DB90 with the reason 0xd frees it; the list's answer comes back
int l_RemoveItemByIndex(lua_State* L)
{
    const KItem* it = script_item(L, "RemoveItemByIndex");
    if (it == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const std::uint32_t id = it->id;
    const bool ok = g_ScriptContext().world->take_item(g_ScriptContext().sid, id);
    log::debug("lua", "item removed by script", {log::kv("entity", g_ScriptContext().player->id), log::kv("item", id), log::kv("ok", ok)});
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// GetItemStackCount(idx) -> the stack (0x080FD250): exactly one argument and an index in range, else -1; a stackable piece
// (Item+0x14) whose count (+0x308) is above 0 and at most its maximum (+0x30c, 1 when none) answers the count, anything
// else 1
int l_GetItemStackCount(lua_State* L)
{
    if (lua_gettop(L) != 1) {
        lua_pushinteger(L, -1);
        return 1;
    }
    const KItem* it = script_item(L, "GetItemStackCount");
    if (it == nullptr) {
        lua_pushinteger(L, -1);
        return 1;
    }
    const int max = it->max_stack() > 0 ? it->max_stack() : 1;
    lua_pushinteger(L, it->max_stack() > 0 && it->count > 0 && it->count <= max ? it->count : 1);
    return 1;
}

// GetGlodEqIndex(idx) -> the gold row + 1 (0x080FEF90 -> 0x080FEEB0(L, 1): Item+4 == 1 (a gold piece) -> Item+0x80 + 1), else 0
int l_GetGlodEqIndex(lua_State* L)
{
    const KItem* it = script_item(L, "GetGlodEqIndex");
    lua_pushinteger(L, it != nullptr && it->ex_type == 1 ? it->gen_param + 1 : 0);
    return 1;
}

// SetItemMagicLevel(idx, slot, level) (0x080FD020): exactly three arguments; the index not negative and within the table,
// the slot 1..6 (0x080FD10C); Item+0x1e4 + slot * 4 = level - nothing is synced (the scripts call SyncItem)
int l_SetItemMagicLevel(lua_State* L)
{
    if (lua_gettop(L) != 3) return 0;
    KItem* it = script_item(L, "SetItemMagicLevel");
    const int slot = static_cast<int>(lua_tonumber(L, 2));
    const int level = static_cast<int>(lua_tonumber(L, 3));
    if (it == nullptr || slot < 1 || slot > 6) return 0;
    it->magic_level[static_cast<std::size_t>(slot - 1)] = level;
    return 0;
}

// ITEM_GetItemRandSeed(idx) -> the seed (0x081548E0: exactly one argument and an index in range, else -1; Item+0x1e0 as an
// unsigned number)
int l_ITEM_GetItemRandSeed(lua_State* L)
{
    if (lua_gettop(L) != 1) {
        lua_pushinteger(L, -1);
        return 1;
    }
    const KItem* it = script_item(L, "ITEM_GetItemRandSeed");
    if (it == nullptr) {
        lua_pushinteger(L, -1);
        return 1;
    }
    lua_pushinteger(L, static_cast<lua_Integer>(it->rand_seed));
    return 1;
}

// AddGoldItem(luck, id) -> 1 / 0 (jx_linux_y 0x0811F210; the scripts also write
// AddGoldItem(where, luck, id) - the leading string is skipped): Gen_GoldEquip row `id`
int l_AddGoldItem(lua_State* L)
{
    int first = 1;
    if (lua_type(L, 1) == LUA_TSTRING) first = 2;
    KNpc* p = player_of(L, "AddGoldItem");
    if (p == nullptr || lua_gettop(L) < first + 1) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const auto luck = static_cast<int>(luaL_checknumber(L, first));
    const auto row = static_cast<int>(luaL_checknumber(L, first + 1));
    KSubWorld* w = g_ScriptContext().world;
    auto gen = w->item_generator(w->item_version());
    std::optional<KItem> item = gen ? gen->gold(luck, row) : std::nullopt;
    if (!item) {
        log::warn("lua", "AddGoldItem: no such row", {log::kv("row", row), log::kv("luck", luck)});
        lua_pushinteger(L, 0);
        return 1;
    }
    const std::uint32_t id = w->give_item(g_ScriptContext().sid, std::move(*item));
    lua_pushinteger(L, id != 0 ? 1 : 0);
    return 1;
}

// The JX2 script api names a quest item by its DetailType or by its 名称 in
// \settings\item\questkey.txt (jx_linux_y DelItem 0x0811D5D0: a string argument is looked up
// with KTabFile::GetInteger(row-by-name, "DetailType")).  -1 when the name is unknown.
int quest_detail_arg(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TSTRING) {
        KSubWorld* w = g_ScriptContext().world;
        auto gen = w->item_generator(w->item_version());
        return gen ? gen->set().quest_detail_of(lua_tostring(L, idx)) : -1;
    }
    return static_cast<int>(luaL_optnumber(L, idx, -1));
}

// The rooms the JX2 KItemList walks: every item the player has (bag, repository, quick slots,
// worn); the Ex forms look at the bag only (pos_equiproom = 3 in the entry list)
bool in_bag(const KItemPlace& p) { return p.room == room_equipment; }

// HaveItem(detail | name) -> 1 / 0: a quest item of that detail anywhere (0x0811D250 -> 0x081F9FA0)
int l_HaveItem(lua_State* L)
{
    const KNpc* p = player_of(L, "HaveItem");
    const int detail = quest_detail_arg(L, 1);
    bool have = false;
    if (p != nullptr && detail >= 0) {
        if (const KItemList* list = g_ScriptContext().world->items_of(g_ScriptContext().sid)) {
            list->each([&](const KItem& it, const KItemPlace&) {
                if (it.genre == KItemGenre::task && it.detail == detail) have = true;
            });
        }
    }
    lua_pushinteger(L, have ? 1 : 0);
    return 1;
}

// GetItemCount(detail | name) -> how many quest items of that detail the player holds, each
// stack one (0x0811D6E0 -> 0x081FA010); GetItemCountEx counts the bag only (0x081FC550)
int item_count(lua_State* L, const char* fn, bool bag_only)
{
    const KNpc* p = player_of(L, fn);
    const int detail = quest_detail_arg(L, 1);
    int n = 0;
    if (p != nullptr && detail >= 0) {
        if (const KItemList* list = g_ScriptContext().world->items_of(g_ScriptContext().sid)) {
            list->each([&](const KItem& it, const KItemPlace& place) {
                if (it.genre == KItemGenre::task && it.detail == detail && (!bag_only || in_bag(place))) ++n;
            });
        }
    }
    lua_pushinteger(L, n);
    return 1;
}

int l_GetItemCount(lua_State* L) { return item_count(L, "GetItemCount", false); }
int l_GetItemCountEx(lua_State* L) { return item_count(L, "GetItemCountEx", true); }

// DelItem(detail | name): the first quest item of that detail goes, stack and all
// (0x0811D5D0 -> KItemList 0x08204560); DelItemEx takes it from the bag only (0x08204350)
int del_item(lua_State* L, const char* fn, bool bag_only)
{
    const KNpc* p = player_of(L, fn);
    const int detail = quest_detail_arg(L, 1);
    if (p == nullptr || detail < 0) return 0;
    KSubWorld* w = g_ScriptContext().world;
    std::uint32_t victim = 0;
    if (const KItemList* list = w->items_of(g_ScriptContext().sid)) {
        list->each([&](const KItem& it, const KItemPlace& place) {
            if (victim == 0 && it.genre == KItemGenre::task && it.detail == detail && (!bag_only || in_bag(place))) victim = it.id;
        });
    }
    if (victim != 0) {
        w->take_item(g_ScriptContext().sid, victim);
        log::debug("lua", "quest item taken", {log::kv("entity", p->id), log::kv("detail", detail), log::kv("item", victim)});
    } else {
        log::debug("lua", "quest item to take not found", {log::kv("entity", p->id), log::kv("detail", detail)});
    }
    return 0;
}

int l_DelItem(lua_State* L) { return del_item(L, "DelItem", false); }
int l_DelItemEx(lua_State* L) { return del_item(L, "DelItemEx", true); }

// HaveCommonItem(genre, detail, particular) -> 1 / 0, -1 = any detail / particular (0x0811D140 -> 0x081FA080)
int l_HaveCommonItem(lua_State* L)
{
    const KNpc* p = player_of(L, "HaveCommonItem");
    const auto genre = static_cast<int>(luaL_optnumber(L, 1, -1));
    const auto detail = static_cast<int>(luaL_optnumber(L, 2, -1));
    const auto particular = static_cast<int>(luaL_optnumber(L, 3, -1));
    bool have = false;
    if (p != nullptr) {
        if (const KItemList* list = g_ScriptContext().world->items_of(g_ScriptContext().sid)) {
            list->each([&](const KItem& it, const KItemPlace&) {
                if (static_cast<int>(it.genre) == genre && (detail == -1 || it.detail == detail) && (particular == -1 || it.particular == particular)) have = true;
            });
        }
    }
    lua_pushinteger(L, have ? 1 : 0);
    return 1;
}

// DelCommonItem(genre, detail, particular): the first such item goes (0x0811D4C0 -> 0x08204470)
int l_DelCommonItem(lua_State* L)
{
    const KNpc* p = player_of(L, "DelCommonItem");
    const auto genre = static_cast<int>(luaL_optnumber(L, 1, -1));
    const auto detail = static_cast<int>(luaL_optnumber(L, 2, -1));
    const auto particular = static_cast<int>(luaL_optnumber(L, 3, -1));
    if (p == nullptr) return 0;
    KSubWorld* w = g_ScriptContext().world;
    std::uint32_t victim = 0;
    if (const KItemList* list = w->items_of(g_ScriptContext().sid)) {
        list->each([&](const KItem& it, const KItemPlace&) {
            if (victim == 0 && static_cast<int>(it.genre) == genre && (detail == -1 || it.detail == detail) && (particular == -1 || it.particular == particular)) victim = it.id;
        });
    }
    if (victim != 0) w->take_item(g_ScriptContext().sid, victim);
    return 0;
}

// GetTotalItemCount() -> every item the player holds (the JX2 one counts the item set)
int l_GetTotalItemCount(lua_State* L)
{
    const KNpc* p = player_of(L, "GetTotalItemCount");
    const KItemList* list = p ? g_ScriptContext().world->items_of(g_ScriptContext().sid) : nullptr;
    lua_pushinteger(L, list ? static_cast<lua_Integer>(list->size()) : 0);
    return 1;
}

// AddStackItem([tag,] count, genre, detail, particular, level, series, luck [, magic1..6]) -> item
// id / 0 (jx_linux_y 0x0811FA10): AddItem with a stack of `count` when the piece stacks and the
// count is within its maximum (Item+0x14 stackable, +0x30c max, +0x308 the stack); a leading
// string is a tag the scripts write and is skipped
int l_AddStackItem(lua_State* L)
{
    int first = 1;
    if (lua_type(L, 1) == LUA_TSTRING) first = 2;
    const int n = lua_gettop(L);
    KNpc* p = player_of(L, "AddStackItem");
    if (p == nullptr || n < first + 6) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const auto count = static_cast<int>(luaL_checknumber(L, first));
    const auto genre = static_cast<int>(luaL_checknumber(L, first + 1));
    const auto detail = static_cast<int>(luaL_checknumber(L, first + 2));
    const auto particular = static_cast<int>(luaL_checknumber(L, first + 3));
    const auto level = static_cast<int>(luaL_checknumber(L, first + 4));
    const auto series = static_cast<int>(luaL_checknumber(L, first + 5));
    const auto luck = static_cast<int>(luaL_checknumber(L, first + 6));
    KMagicLevels levels{};
    bool with_magic = false;
    for (int i = 0; i < 6 && first + 7 + i <= n; ++i) {
        levels[static_cast<std::size_t>(i)] = static_cast<int>(luaL_optnumber(L, first + 7 + i, 0));
        with_magic = with_magic || levels[static_cast<std::size_t>(i)] != 0;
    }
    KSubWorld* w = g_ScriptContext().world;
    auto gen = w->item_generator(w->item_version());
    std::optional<KItem> item;
    if (gen) {
        switch (static_cast<KItemGenre>(genre)) {
        case KItemGenre::equip: item = gen->equipment(detail, particular, series, level, with_magic ? &levels : nullptr, luck); break;
        case KItemGenre::medicine: item = gen->medicine(detail, level); break;
        case KItemGenre::task: item = gen->quest(detail, 1); break;
        case KItemGenre::town_portal: item = gen->town_portal(); break;
        case KItemGenre::magic_script: item = gen->magic_script(detail, particular, level, series, 1); break;
        default: break;
        }
    }
    if (!item) {
        log::warn("lua", "AddStackItem: no such item", {log::kv("genre", genre), log::kv("detail", detail), log::kv("particular", particular),
                                                         log::kv("level", level)});
        lua_pushinteger(L, 0);
        return 1;
    }
    if (item->tpl != nullptr && (item->tpl->stackable || item->tpl->max_stack > 0) && count > 0) {
        const int max_stack = item->tpl->max_stack > 0 ? item->tpl->max_stack : 1;
        if (count <= max_stack) item->count = std::min(count, 0xFFFF);
    }
    const std::uint32_t id = w->give_item(g_ScriptContext().sid, std::move(*item));
    lua_pushinteger(L, id);
    return 1;
}

// ---- the skill list (KSkillList.h; the JX2 script api of jx_linux_y, docs/LINUX-SERVER.md §15) ----

// the skill of argument `idx`: a number, or the name of a row of Skills.txt (the old code:
// KTabFile::GetInteger(szRowName, "SkillId")); 0 when neither
int skill_id_arg(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TNUMBER) return static_cast<int>(lua_tonumber(L, idx));
    const char* name = lua_tostring(L, idx);
    KSubWorld* w = g_ScriptContext().world;
    if (name == nullptr || w == nullptr || w->skills() == nullptr || w->skills()->table() == nullptr) return 0;
    return w->skills()->table()->id_of(name);
}

// SetSkillLevel(id | name [, level]): KSkillList::Add(id, level, 0, 0, 0, 0), then the 0x5e packet
int l_SetSkillLevel(lua_State* L)
{
    KNpc* p = player_of(L, "SetSkillLevel");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = skill_id_arg(L, 1);
    if (id <= 0) return 0;
    const int level = lua_gettop(L) >= 2 ? static_cast<int>(lua_tonumber(L, 2)) : 0;
    if (level > 1 && id > 1999) return 0;
    KSubWorld* w = g_ScriptContext().world;
    KSkillListHost host = w->skill_host(*p);
    const int addon = p->player.reborn != 0 ? p->player.skill_max_level_addons : 0;
    if (p->skill_list.add(id, level, 0, 0, addon, host) == 0) return 0;
    log::debug("lua", "skill level set", {log::kv("entity", p->id), log::kv("skill", id), log::kv("level", level)});
    w->send_skill_level(p->sid, id, p->skill_list.get_level(id), 0);
    return 0;
}

// HaveMagic(id | name) -> the level held, -1 when not held (0x0811C930)
int l_HaveMagic(lua_State* L)
{
    KNpc* p = player_of(L, "HaveMagic");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = skill_id_arg(L, 1);
    if (id <= 0) return 0;
    if (id > 1999 || p->skill_list.find_same(id) == 0) {
        lua_pushnumber(L, -1);
        return 1;
    }
    lua_pushnumber(L, p->skill_list.get_level(id));
    return 1;
}

// DelMagic(id | name): KSkillList::Remove, then the 0x5e packet with a level of -1 (0x0811C7E0)
int l_DelMagic(lua_State* L)
{
    KNpc* p = player_of(L, "DelMagic");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = skill_id_arg(L, 1);
    if (id <= 0) return 0;
    KSubWorld* w = g_ScriptContext().world;
    KSkillListHost host = w->skill_host(*p);
    p->skill_list.remove(id, host);
    log::debug("lua", "skill removed", {log::kv("entity", p->id), log::kv("skill", id)});
    w->send_skill_level(p->sid, id, -1, 0);
    return 0;
}

// GetCurrentMagicLevel(id | name [, withInc = 1]) -> KSkillList::GetCurrentLevel (0x0811C3A0)
int l_GetCurrentMagicLevel(lua_State* L)
{
    KNpc* p = player_of(L, "GetCurrentMagicLevel");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = skill_id_arg(L, 1);
    if (id <= 0) return 0;
    const int with_inc = lua_gettop(L) >= 2 ? static_cast<int>(lua_tonumber(L, 2)) : 1;
    lua_pushnumber(L, p->skill_list.get_current_level(id, with_inc != 0));
    return 1;
}

// GetSkillMaxLevel(id) -> MaxLevel of the row (0 without one; -1 without an argument) (0x080FDAD0)
int l_GetSkillMaxLevel(lua_State* L)
{
    if (lua_gettop(L) < 1) {
        lua_pushnumber(L, -1);
        return 1;
    }
    const int id = static_cast<int>(lua_tonumber(L, 1));
    KSubWorld* w = g_ScriptContext().world;
    int max_level = 0;
    if (id >= 1 && id <= 2000 && w != nullptr && w->skills() != nullptr && w->skills()->table() != nullptr) max_level = w->skills()->table()->max_level(id);
    lua_pushnumber(L, max_level);
    return 1;
}

// GetSkillExp(id) -> the experience held; -1 for a bad id / no row / not an exp skill (0x0812AB40)
int l_GetSkillExp(lua_State* L)
{
    KNpc* p = player_of(L, "GetSkillExp");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = static_cast<int>(lua_tonumber(L, 1));
    KSubWorld* w = g_ScriptContext().world;
    const KSkill* sk = id >= 1 && id <= 1999 && w->skills() != nullptr ? w->skills()->get(id, 1) : nullptr;
    if (sk == nullptr || !sk->row.is_exp_skill) {
        lua_pushnumber(L, -1);
        return 1;
    }
    const int idx = p->skill_list.find_same(id);
    lua_pushnumber(L, idx != 0 ? p->skill_list.cell(idx)->exp : 0);
    return 1;
}

// GetSkillNextExp(id) -> skill_skillexp_v of the instance at the current level without the
// increments; 0 when none / not an exp skill (0x0812AC50)
int l_GetSkillNextExp(lua_State* L)
{
    KNpc* p = player_of(L, "GetSkillNextExp");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = static_cast<int>(lua_tonumber(L, 1));
    const int level = p->skill_list.get_current_level(id, false);
    KSubWorld* w = g_ScriptContext().world;
    if (id < 1 || id > 1999 || level <= 0 || level > 63 || w->skills() == nullptr) return 0;
    const KSkill* sk = w->skills()->get(id, level);
    if (sk == nullptr || !sk->row.is_exp_skill) return 0;
    lua_pushnumber(L, sk->skill_exp);
    return 1;
}

// AddSkillExp(id, exp [, levelUp [, percent]]) -> 1 / 0 (KSkillList::AddSkillExp), -1 for bad
// arguments: addskillexp1 {id, exp, levelUp == 0}, the fourth argument = the percent mode (0x08129DC0)
int l_AddSkillExp(lua_State* L)
{
    const int argc = lua_gettop(L);
    if (argc <= 1) {
        lua_pushnumber(L, -1);
        return 1;
    }
    KNpc* p = player_of(L, "AddSkillExp");
    if (p == nullptr) {
        lua_pushnumber(L, -1);
        return 1;
    }
    const int id = static_cast<int>(lua_tonumber(L, 1));
    KSubWorld* w = g_ScriptContext().world;
    const KSkill* sk = id >= 1 && id <= 1999 && w->skills() != nullptr ? w->skills()->get(id, 1) : nullptr;
    if (sk == nullptr || !sk->row.is_exp_skill) {
        lua_pushnumber(L, -1);
        return 1;
    }
    int no_level_up = 1;
    int percent = 0;
    if (argc >= 3) no_level_up = static_cast<int>(lua_tonumber(L, 3)) == 0 ? 1 : 0;
    if (argc >= 4) percent = static_cast<int>(lua_tonumber(L, 4));
    KMagicAttrib x;
    x.type = magic_addskillexp1;
    x.value = {id, static_cast<int>(lua_tonumber(L, 2)), no_level_up};
    lua_pushnumber(L, w->give_skill_exp(*p, x, percent != 0) ? 1 : 0);
    return 1;
}

// RollbackSkill() -> the levels taken back (the points a script gives back), then the 0x5e packet (0x0811C640)
int l_RollbackSkill(lua_State* L)
{
    KNpc* p = player_of(L, "RollbackSkill");
    if (p == nullptr) return 0;
    KSubWorld* w = g_ScriptContext().world;
    KSkillListHost host = w->skill_host(*p);
    const int sum = p->skill_list.rollback(host);
    log::debug("lua", "skills rolled back", {log::kv("entity", p->id), log::kv("level", sum)});
    w->send_skill_level(p->sid, 0, 0, 0);
    lua_pushnumber(L, sum);
    return 1;
}

// ForbitSkill(n): every skill locked (n ~= 0) or freed (0x08121620)
int l_ForbitSkill(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitSkill");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    g_ScriptContext().world->forbit_skill(*p, static_cast<int>(lua_tonumber(L, 1)) != 0);
    return 0;
}

// SetAForbitSkill(id, n): one skill locked / freed (0x08121580)
int l_SetAForbitSkill(lua_State* L)
{
    KNpc* p = player_of(L, "SetAForbitSkill");
    if (p == nullptr || lua_gettop(L) < 2) return 0;
    g_ScriptContext().world->set_a_forbit_skill(*p, static_cast<int>(lua_tonumber(L, 1)), static_cast<int>(lua_tonumber(L, 2)));
    return 0;
}

// ForbitAura(n): Player+0x375 = n ~= 0; forbidding also clears the aura (0x08111560 -> KNpc::SetAura(npc, 0))
int l_ForbitAura(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitAura");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const bool forbid = static_cast<int>(lua_tonumber(L, 1)) != 0;
    p->player.forbid_aura = forbid;
    if (forbid) g_ScriptContext().world->set_aura(*p, 0);
    return 0;
}

// ForbitTalk(n) (0x0810CB70): Player+0x38c = (n ~= 0) - a channel line of the player is dropped (0x081E387A)
int l_ForbitTalk(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitTalk");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.forbid_talk = static_cast<int>(lua_tonumber(L, 1)) != 0;
    return 0;
}

// SetChatFlag(n) (0x08111460): bit 0 of Player+0x394 - set, the cost check refuses every channel (0x080502DD)
int l_SetChatFlag(lua_State* L)
{
    KNpc* p = player_of(L, "SetChatFlag");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.chat_flag = static_cast<int>(lua_tonumber(L, 1)) != 0;
    return 0;
}

// ForbitStamina(n): Player+0x86b4 = (n ~= 0) (0x0810CCC0) - no stamina gain while set (ProcessState 0x0808BD53)
int l_ForbitStamina(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitStamina");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.forbid_stamina = static_cast<int>(lua_tonumber(L, 1)) != 0 ? 1 : 0;
    return 0;
}

// RestoreLife() (0x08112480): the life back to max(+0x1a14, +0x1a18); RestoreMana() (0x08112430): the mana back to
// max(+0x1a1c, +0x1a20).  The numbers reach the client with the next attribute sync.
int l_RestoreLife(lua_State* L)
{
    if (KNpc* p = player_of(L, "RestoreLife")) {
        p->cur.life = p->life_max();
        g_ScriptContext().world->send_player_attrib(g_ScriptContext().sid);
    }
    return 0;
}

int l_RestoreMana(lua_State* L)
{
    if (KNpc* p = player_of(L, "RestoreMana")) {
        p->cur.mana = p->mana_max();
        g_ScriptContext().world->send_player_attrib(g_ScriptContext().sid);
    }
    return 0;
}

// GetLife(kind) (0x081124D0) / GetMana(kind) (0x08112270): kind 0 -> the current value (+0x118c / +0x11a0), 1 or 2 ->
// the base maximum (m_LifeMax +0x15ac / m_ManaMax +0x15b4); anything else raises a Lua error (0x08232E70)
int life_or_mana(lua_State* L, const char* fn, bool mana)
{
    const KNpc* p = player_of(L, fn);
    if (p == nullptr) return 0;
    const int kind = static_cast<int>(luaL_checknumber(L, 1));
    if (kind == 1 || kind == 2) {
        lua_pushnumber(L, mana ? p->base.mana_max : p->base.life_max);
    } else if (kind == 0) {
        lua_pushnumber(L, mana ? p->mana() : p->life());
    } else {
        return luaL_error(L, "%s: bad kind %d", fn, kind);
    }
    return 1;
}

int l_GetLife(lua_State* L) { return life_or_mana(L, "GetLife", false); }
int l_GetMana(lua_State* L) { return life_or_mana(L, "GetMana", true); }

// GetPK() -> the PK value (0x081103C0: KPlayerPK::GetPKValue +0x2c)
int l_GetPK(lua_State* L)
{
    const KNpc* p = player_of(L, "GetPK");
    lua_pushinteger(L, p ? p->player.pk.value : 0);
    return 1;
}

// SetPK(value) (0x08110420: KPlayerPK::SetPKValue - clamped to 0..10)
int l_SetPK(lua_State* L)
{
    KNpc* p = player_of(L, "SetPK");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    g_ScriptContext().world->pk_set_value(*p, static_cast<int>(lua_tonumber(L, 1)));
    return 0;
}

// SetPKFlag(state) (0x0810F610: KPlayerPK::SetPKState(state, force = 1))
int l_SetPKFlag(lua_State* L)
{
    KNpc* p = player_of(L, "SetPKFlag");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    g_ScriptContext().world->pk_set_state(*p, static_cast<int>(lua_tonumber(L, 1)), true);
    return 0;
}

// ForbidChangePK(n): Player+0x5a58 = (n == 1) (0x0810F590); IsForbidChangePK() -> it (0x0810F540)
int l_ForbidChangePK(lua_State* L)
{
    KNpc* p = player_of(L, "ForbidChangePK");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.pk.locked = static_cast<int>(lua_tonumber(L, 1)) == 1;
    return 0;
}

int l_IsForbidChangePK(lua_State* L)
{
    const KNpc* p = player_of(L, "IsForbidChangePK");
    lua_pushinteger(L, p && p->player.pk.locked ? 1 : 0);
    return 1;
}

// ---- the team (docs/LINUX-SERVER.md §17) ----

// IsCaptain() -> 1 when in a team as its captain (0x08115690: +0x5994 && +0x599c == 0)
int l_IsCaptain(lua_State* L)
{
    const KNpc* p = player_of(L, "IsCaptain");
    lua_pushinteger(L, p != nullptr && p->player.team.captain() ? 1 : 0);
    return 1;
}

// GetTeam() -> the team id, nil out of a team (0x08115630)
int l_GetTeam(lua_State* L)
{
    const KNpc* p = player_of(L, "GetTeam");
    if (p == nullptr || !p->player.team.flag) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, p->player.team.id);
    return 1;
}

// GetTeamSize([team]) -> the members + the captain of that team, of one's own team without an argument; 0 out of a team
// (0x08115480: g_Team[id]+0x28 + 1)
int l_GetTeamSize(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    int id = -1;
    if (lua_gettop(L) >= 1) {
        id = static_cast<int>(lua_tonumber(L, 1));
        if (id < 0) {
            lua_pushinteger(L, 0);
            return 1;
        }
    } else {
        const KNpc* p = player_of(L, "GetTeamSize");
        if (p == nullptr || !p->player.team.flag) {
            lua_pushinteger(L, 0);
            return 1;
        }
        id = p->player.team.id;
    }
    const KTeam* t = w != nullptr ? w->teams().get(id) : nullptr;
    lua_pushinteger(L, t != nullptr && !t->empty() ? t->count + 1 : 0);
    return 1;
}

// GetTeamMember(n) -> the npc id of the captain (n == 1) or of the (n - 1)-th member of one's team; -1 when none
// (0x08115530: the player indices of g_Team - the zone hands out entity ids)
int l_GetTeamMember(lua_State* L)
{
    const KNpc* p = player_of(L, "GetTeamMember");
    KSubWorld* w = g_ScriptContext().world;
    const int n = lua_gettop(L) >= 1 ? static_cast<int>(lua_tonumber(L, 1)) : 0;
    const KTeam* t = p != nullptr && w != nullptr ? w->team_of(*p) : nullptr;
    if (t == nullptr || n <= 0) {
        lua_pushinteger(L, -1);
        return 1;
    }
    const KNpc* who = nullptr;
    if (n == 1) {
        who = w->find_player(t->captain);
    } else {
        int seen = 0;
        for (const std::uint64_t sid : t->members) {
            if (sid == 0) continue;
            if (++seen == n - 1) {
                who = w->find_player(sid);
                break;
            }
        }
    }
    lua_pushinteger(L, who != nullptr ? static_cast<lua_Integer>(who->id.value) : -1);
    return 1;
}

// LeaveTeam() (0x08121060 -> KPlayer::LeaveTeam 0x080B7C60)
int l_LeaveTeam(lua_State* L)
{
    if (KNpc* p = player_of(L, "LeaveTeam")) g_ScriptContext().world->team_leave(*p);
    return 0;
}

// SetCreateTeam(n): KPlayerTeam::SetCanTeamFlag(n ~= 0, leave = 1) (0x08120FC0 -> 0x080CC580): can_team = n; n == 0 also
// leaves the team
int l_SetCreateTeam(lua_State* L)
{
    KNpc* p = player_of(L, "SetCreateTeam");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const bool can = static_cast<int>(lua_tonumber(L, 1)) != 0;
    p->player.team.can_team = can;
    if (!can) g_ScriptContext().world->team_leave(*p);
    return 0;
}

// DisabledTeam(n) -> 1: the task value 0x87 bit 0x400 set (n ~= 0) or cleared (0x08126590; the packet 0xa4 of the task
// value is not in the zone); IsDisabledTeam() -> that bit (0x0812EF50)
int l_DisabledTeam(lua_State* L)
{
    KNpc* p = player_of(L, "DisabledTeam");
    if (p == nullptr || lua_gettop(L) < 1) {
        lua_pushinteger(L, 0);
        return 1;
    }
    p->player.team.lua_disabled = static_cast<int>(lua_tonumber(L, 1)) != 0;
    lua_pushinteger(L, 1);
    return 1;
}

int l_IsDisabledTeam(lua_State* L)
{
    const KNpc* p = player_of(L, "IsDisabledTeam");
    lua_pushinteger(L, p != nullptr && p->player.team.lua_disabled ? 1 : 0);
    return 1;
}

// ChangeTeamFeature(team, feature, value) (0x08103630): feature 1 = the leadership limit of g_Team[team] (+0x2c: 0 makes
// CheckFull never full); other features are not in the binary's switch
int l_ChangeTeamFeature(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    if (w == nullptr || lua_gettop(L) < 3) return 0;
    const int id = static_cast<int>(lua_tonumber(L, 1));
    const int feature = static_cast<int>(lua_tonumber(L, 2));
    const int value = static_cast<int>(lua_tonumber(L, 3));
    KTeam* t = w->mutable_team(id);
    if (t == nullptr || t->empty()) return 0;
    if (feature == 1) t->lead_limit = value != 0;
    return 0;
}

// Msg2Team(text): the text to the captain and every member of one's team as a system message (0x081152C0 -> 0x081C9220(1, ...))
int l_Msg2Team(lua_State* L)
{
    const KNpc* p = player_of(L, "Msg2Team");
    KSubWorld* w = g_ScriptContext().world;
    if (p == nullptr || w == nullptr || lua_gettop(L) < 1) return 0;
    const char* text = lua_tostring(L, 1);
    if (text == nullptr) return 0;
    const KTeam* t = w->team_of(*p);
    if (t == nullptr) return 0;
    for (const std::uint64_t sid : t->people()) w->msg_to_player(sid, text);
    return 0;
}

// SetPkReduceState(seconds, value, weaken, enhance) (0x08109570): Player+0x5a8c / +0x5a84 / +0x5a88 = (weaken << 8) | enhance;
// GetPkReduceState() -> seconds, value, weaken, enhance (0x081094D0)
int l_SetPkReduceState(lua_State* L)
{
    KNpc* p = player_of(L, "SetPkReduceState");
    if (p == nullptr || lua_gettop(L) < 4) return 0;
    const int seconds = static_cast<int>(lua_tonumber(L, 1));
    if (seconds <= 0) return 0;   // 0x08109616
    p->player.pk.reduce_seconds = seconds;
    p->player.pk.reduce_value = static_cast<int>(lua_tonumber(L, 2));
    p->player.pk.punish_weaken = static_cast<int>(lua_tonumber(L, 3));
    p->player.pk.punish_enhance = static_cast<int>(lua_tonumber(L, 4));
    return 0;
}

int l_GetPkReduceState(lua_State* L)
{
    const KNpc* p = player_of(L, "GetPkReduceState");
    lua_pushinteger(L, p ? p->player.pk.reduce_seconds : 0);
    lua_pushinteger(L, p ? p->player.pk.reduce_value : 0);
    lua_pushinteger(L, p ? p->player.pk.punish_weaken : 0);
    lua_pushinteger(L, p ? p->player.pk.punish_enhance : 0);
    return 4;
}

// SetDeathPunish_PK10(n): Player+0x384 = n (0x08110680) - the arena death of 0x08089750 (not in the zone)
int l_SetDeathPunish_PK10(lua_State* L)
{
    KNpc* p = player_of(L, "SetDeathPunish_PK10");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.pk10_death_punish = static_cast<int>(lua_tonumber(L, 1));
    return 0;
}

// ForbitSyncAura(n): Player+0x388 = 0 when n ~= 0, 1 otherwise (0x0810CC10) - whether an aura tick is shown to others
int l_ForbitSyncAura(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitSyncAura");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.sync_aura = static_cast<int>(lua_tonumber(L, 1)) == 0;
    return 0;
}

// SetSkillMaxLevelAddons(n): Player+0x8600, at most 99 (0x08108B70); GetSkillMaxLevelAddons() reads it
int l_SetSkillMaxLevelAddons(lua_State* L)
{
    KNpc* p = player_of(L, "SetSkillMaxLevelAddons");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int n = static_cast<int>(lua_tonumber(L, 1));
    if (n > 99) return 0;
    p->player.skill_max_level_addons = n;
    return 0;
}

int l_GetSkillMaxLevelAddons(lua_State* L)
{
    KNpc* p = player_of(L, "GetSkillMaxLevelAddons");
    if (p == nullptr) return 0;
    lua_pushnumber(L, p->player.skill_max_level_addons);
    return 1;
}

// GetSkillCount([all]) -> KSkillList::GetCount(all) (0x0811C510); GetTotalSkill() -> the levels held (0x0811C5D0)
int l_GetSkillCount(lua_State* L)
{
    KNpc* p = player_of(L, "GetSkillCount");
    if (p == nullptr) {
        lua_pushnumber(L, 0);
        return 1;
    }
    const bool all = lua_gettop(L) >= 1 && static_cast<int>(lua_tonumber(L, 1)) != 0;
    lua_pushnumber(L, p->skill_list.get_count(all, g_ScriptContext().world->skills()));
    return 1;
}

int l_GetTotalSkill(lua_State* L)
{
    KNpc* p = player_of(L, "GetTotalSkill");
    lua_pushnumber(L, p == nullptr ? 0 : p->skill_list.get_total_level(g_ScriptContext().world->skills()));
    return 1;
}

// IsExpSkill(id) -> true / false (0x0812CF30)
int l_IsExpSkill(lua_State* L)
{
    if (lua_gettop(L) < 1 || player_of(L, "IsExpSkill") == nullptr) return 0;
    const int id = static_cast<int>(lua_tonumber(L, 1));
    KSubWorld* w = g_ScriptContext().world;
    const KSkill* sk = id >= 1 && id <= 1999 && w->skills() != nullptr ? w->skills()->get(id, 1) : nullptr;
    if (sk == nullptr) return 0;
    lua_pushboolean(L, sk->row.is_exp_skill ? 1 : 0);
    return 1;
}

// UpdateSkill(): the JX2 server does nothing here (0x08114A10); the zone resends the list so a
// client follows what a script changed
int l_UpdateSkill(lua_State* L)
{
    if (KNpc* p = player_of(L, "UpdateSkill")) g_ScriptContext().world->send_skill_list(p->sid);
    return 0;
}

// SetHide(n): KNpc::SetHide 0x0807FF80 on the player's npc (0x0810AD80; NpcSetHide(idx, n)
// 0x08101C10 waits for the npc handle of the script api)
int l_SetHide(lua_State* L)
{
    KNpc* p = player_of(L, "SetHide");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    g_ScriptContext().world->set_hide(*p, static_cast<int>(lua_tonumber(L, 1)));
    return 0;
}

// AbradeEquipments(mode): KItemList 0x08201940 on the player's worn pieces - 0 an attack, 1 a hit, 2 a
// step (0x08107AA0: the first argument, above 2 nothing)
int l_AbradeEquipments(lua_State* L)
{
    KNpc* p = player_of(L, "AbradeEquipments");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int mode = static_cast<int>(lua_tonumber(L, 1));
    if (mode < 0 || mode > 2) return 0;
    g_ScriptContext().world->abrade_equipments(*p, mode);
    return 0;
}

// SetTempRevPos(map, x, y): KPlayer+0x20 / +0x28 / +0x2c (0x08110790) - where KPlayer::Revive(0) puts the
// character; cells like NewWorld (SetTempRevPos(map) alone keeps the map)
int l_SetTempRevPos(lua_State* L)
{
    KNpc* p = player_of(L, "SetTempRevPos");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.revive_map = static_cast<std::uint32_t>(lua_tonumber(L, 1));
    if (lua_gettop(L) >= 3) {
        p->player.revive_x = static_cast<int>(lua_tonumber(L, 2)) * 32;
        p->player.revive_y = static_cast<int>(lua_tonumber(L, 3)) * 32;
    }
    return 0;
}

// SetRevPos(map, ref): KPlayer 0x080B1E50 - the map and its reference point (KSubWorldSet 0x080F6D20 finds the
// spot in revivepos.ini: +0x18 / +0x1c and +0x20 / +0x28 / +0x2c take it; a point it cannot find is logged as
// revive_error and leaves the spot unset - the revive then goes to the map's spawn point here)
int l_SetRevPos(lua_State* L)
{
    KNpc* p = player_of(L, "SetRevPos");
    if (p == nullptr || lua_gettop(L) < 2) return 0;
    const int map = static_cast<int>(lua_tonumber(L, 1));
    if (map < 0) return 0;
    p->player.revive_map = static_cast<std::uint32_t>(map);
    p->player.revive_ref = static_cast<int>(lua_tonumber(L, 2));
    if (const auto at = g_ScriptContext().world->revive_point(p->player.revive_map, p->player.revive_ref)) {
        p->player.revive_x = at->x;
        p->player.revive_y = at->y;
    } else {
        p->player.revive_x = 0;
        p->player.revive_y = 0;
        log::warn("lua", "revive point not found", {log::kv("map", map), log::kv("ref", p->player.revive_ref)});
    }
    return 0;
}

// KillPlayer(): 0x08117BC0 - the character takes a hit from itself that nothing softens: twenty damage
// cells, [0] seriesdamage_p 100, [1] attackrating_v 50000, [2] ignoredefense_p 1, [3] (the physics slot,
// type left 0) 200 000 000 .. 200 000 000, through KNpc::ReceiveDamage(self, series 0, not melee, no AR
// check, do_hurt 1, relation 0x1f, skill 0); the death of 16.4 follows (KillNpc / KillNpcWithIdx
// 0x08117E00 / 0x081181D0 -> 0x08117CE0 wait for the npc handle of the script api)
int l_KillPlayer(lua_State* L)
{
    KNpc* p = player_of(L, "KillPlayer");
    if (p == nullptr) return 0;
    std::array<KMagicAttrib, kSkillAttribs> dmg{};
    dmg[0] = KMagicAttrib{magic_seriesdamage_p, {100, 0, 0}};
    dmg[1] = KMagicAttrib{magic_attackrating_v, {50000, 0, 0}};
    dmg[2] = KMagicAttrib{magic_ignoredefense_p, {1, 0, 0}};
    dmg[3] = KMagicAttrib{0, {200000000, 0, 200000000}};
    g_ScriptContext().world->receive_damage(*p, *p, 0, false, dmg.data(), false, 1, 0x1f, 0);
    return 0;
}

// GetRideState(): 0x08111730 - KNpc+0x199c of the player's npc (1 while riding)
int l_GetRideState(lua_State* L)
{
    KNpc* p = player_of(L, "GetRideState");
    if (p == nullptr) return 0;
    lua_pushnumber(L, p->horse);
    return 1;
}

// AddExp(exp[, npcLevel]): 0x0811A140 - at most two numbers, then KPlayer::AddExp 0x080B00C0(player, exp, npcLevel) on the
// player's npc (the rule of a kill: the level difference weighs the gain); nothing without a player
int l_AddExp(lua_State* L)
{
    KNpc* p = player_of(L, "AddExp");
    if (p == nullptr || lua_gettop(L) < 1 || lua_gettop(L) > 2) return 0;
    const int exp = static_cast<int>(lua_tonumber(L, 1));
    const int npc_level = lua_gettop(L) >= 2 ? static_cast<int>(lua_tonumber(L, 2)) : 0;
    g_ScriptContext().world->give_player_exp(*p, exp, npc_level);
    return 0;
}

// ---- factions (docs/LINUX-SERVER.md §16.7; KFaction.h) ----

// SetFaction(name) -> 1 / 0: 0x0811A540 - no player -> 0; an empty name -> KPlayer::ClearFaction 0x080AEDE0 (1);
// else KPlayer::SetFaction 0x080AEEC0
int l_SetFaction(lua_State* L)
{
    KNpc* p = player_of(L, "SetFaction");
    if (p == nullptr) {
        lua_pushnumber(L, 0);
        return 1;
    }
    const char* name = lua_tostring(L, 1);
    KSubWorld* w = g_ScriptContext().world;
    if (name == nullptr || *name == '\0') {
        w->clear_faction(*p);
        lua_pushnumber(L, 1);
        return 1;
    }
    lua_pushnumber(L, w->set_faction(*p, name) ? 1 : 0);
    return 1;
}

// GetFaction() -> the code name of the current faction (0x0811A5D0 -> 0x080ABB70 -> 0x080C2680): "" without a
// player or a faction, the [Name] Old string (G_FACTION_OLD) for a character that left
int l_GetFaction(lua_State* L)
{
    KNpc* p = player_of(L, "GetFaction");
    if (p == nullptr) {
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushstring(L, p->player.faction.name(g_ScriptContext().world->faction_table()).c_str());
    return 1;
}

// GetFactionNumber() -> the current faction's index, -1 without one (0x08113C60 reads +0x59cc)
int l_GetFactionNumber(lua_State* L)
{
    KNpc* p = player_of(L, "GetFactionNumber");
    lua_pushnumber(L, p == nullptr ? -1 : p->player.faction.current);
    return 1;
}

// GetLastAddFaction() -> the code name of the faction last joined (0x0811A4C0 -> 0x080ABB50 -> 0x080C27B0)
int l_GetLastAddFaction(lua_State* L)
{
    KNpc* p = player_of(L, "GetLastAddFaction");
    if (p == nullptr) {
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushstring(L, p->player.faction.last_name(g_ScriptContext().world->faction_table()).c_str());
    return 1;
}

// GetLastFactionNumber() -> the index of the faction last joined (+0x59d4); -1 with any argument or no player (0x0810E6D0)
int l_GetLastFactionNumber(lua_State* L)
{
    KNpc* p = player_of(L, "GetLastFactionNumber");
    if (lua_gettop(L) != 0 || p == nullptr) {
        lua_pushnumber(L, -1);
        return 1;
    }
    lua_pushnumber(L, p->player.faction.last);
    return 1;
}

// SetLastFactionNumber(n): +0x59d4 = n (0x0810E4B0); nothing without an argument or a player
int l_SetLastFactionNumber(lua_State* L)
{
    KNpc* p = player_of(L, "SetLastFactionNumber");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.faction.last = static_cast<int>(lua_tonumber(L, 1));
    return 0;
}

// ClearFactionRecord(): the whole record back to the ctor's -1 / -1 / -1 / 0 (0x0811A480 -> 0x080C25C0); no packet
int l_ClearFactionRecord(lua_State* L)
{
    KNpc* p = player_of(L, "ClearFactionRecord");
    if (p == nullptr) return 0;
    p->player.faction.reset();
    return 0;
}

// SetCamp(n): n >= 0 -> KNpc::SetCamp 0x0807B7B0 on the player's npc (0x0811B2E0); SetCurCamp(n) -> SetCurrentCamp
// 0x0807B850 (0x0811B1D0); GetCamp() / GetCurCamp() -> +0x21c / +0x220, nil without a player (0x08114650 / 0x081146C0)
int l_SetCamp(lua_State* L)
{
    const int camp = static_cast<int>(lua_tonumber(L, 1));
    KNpc* p = camp >= 0 ? player_of(L, "SetCamp") : nullptr;
    if (p != nullptr) g_ScriptContext().world->set_camp(*p, camp);
    return 0;
}

int l_SetCurCamp(lua_State* L)
{
    const int camp = static_cast<int>(lua_tonumber(L, 1));
    KNpc* p = camp >= 0 ? player_of(L, "SetCurCamp") : nullptr;
    if (p != nullptr) g_ScriptContext().world->set_current_camp(*p, camp);
    return 0;
}

int l_GetCamp(lua_State* L)
{
    KNpc* p = player_of(L, "GetCamp");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, p->camp);
    return 1;
}

int l_GetCurCamp(lua_State* L)
{
    KNpc* p = player_of(L, "GetCurCamp");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, p->current_camp);
    return 1;
}

// AddMagic(id | name [, level = 0]): 0x0812C430 - at least one argument and a player; an unknown name (SkillId of
// the row) -> nothing; a level above 1 needs a skill id 1..1999 and a level <= 63 whose instance exists (0x0812C620);
// then KSkillList::Add(id, level, 0, 0, 0, 0) and the 0x5e packet {id, the level held, the skill points left}
int l_AddMagic(lua_State* L)
{
    KNpc* p = player_of(L, "AddMagic");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    const int id = skill_id_arg(L, 1);
    if (id <= 0) return 0;
    int level = lua_gettop(L) >= 2 ? static_cast<int>(lua_tonumber(L, 2)) : 0;
    KSubWorld* w = g_ScriptContext().world;
    if (level > 1) {
        if (id > 0x7cf || level > 0x3f) return 0;
        if (w->skills() == nullptr || w->skills()->get(id, level) == nullptr) return 0;
    }
    KSkillListHost host = w->skill_host(*p);
    if (p->skill_list.add(id, level, 0, 0, 0, host) == 0) return 0;
    log::debug("lua", "skill added", {log::kv("entity", p->id), log::kv("skill", id), log::kv("level", level)});
    w->send_skill_level(p->sid, id, p->skill_list.get_level(id), 0);
    return 0;
}

// ---- the task values (KPlayerTask at Player+0x809c; docs/LINUX-SERVER.md §21) ----

// the integer of a Lua number the way the binary's fistp makes one (toward zero); out of range -> INT_MIN like the fpu
int task_int(lua_State* L, int idx)
{
    const lua_Number n = lua_tonumber(L, idx);
    if (!(n > -2147483648.0 && n < 2147483648.0)) return INT_MIN;
    return static_cast<int>(n);
}

// GetTask(id) (0x08116890): the saved value of the id (GetTaskValue 0x080CB540) - nil without a player (0x08116910)
int l_GetTask(lua_State* L)
{
    const KNpc* p = player_of(L, "GetTask");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, p->player.task.get_save_val(task_int(L, 1)));
    return 1;
}

// SetTask(id, value) (0x08116780): KPlayer::SetTaskValue(id, value, sync = 1); id 1 is traced (0x08116812)
int l_SetTask(lua_State* L)
{
    KNpc* p = player_of(L, "SetTask");
    if (p == nullptr) return 0;
    const int id = task_int(L, 1);
    const int value = task_int(L, 2);
    g_ScriptContext().world->task_set_value(*p, id, value, true);
    if (id == kTaskTraceId) {
        log::info("zone.task", "trace task value", {log::kv("entity", p->id), log::kv("id", id), log::kv("value", value), log::kv("name", p->name)});
    }
    return 0;
}

// GetTaskTemp(id) (0x08123A20): the temp value (GetClearVal 0x080CB5A0) of the last argument (Lua_GetTopIndex of 2003) -
// nil above 0xff (0x08123A5F) or without a player; a negative id reads 0
int l_GetTaskTemp(lua_State* L)
{
    const int top = lua_gettop(L);
    const int id = top >= 1 ? task_int(L, top) : 0;
    const KNpc* p = id > 0xff ? nullptr : player_of(L, "GetTaskTemp");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, p->player.task.get_temp(id));
    return 1;
}

// SetTaskTemp(id, value) (0x08123950): SetClearVal 0x080CB5C0 with the last two arguments as the id and the value; ids 0..0xff
int l_SetTaskTemp(lua_State* L)
{
    const int top = lua_gettop(L);
    const int id = top >= 2 ? task_int(L, top - 1) : 0;
    const int value = top >= 1 ? task_int(L, top) : 0;
    KNpc* p = player_of(L, "SetTaskTemp");
    if (p == nullptr) return 0;
    p->player.task.set_temp(id, value);
    return 0;
}

// SyncTaskValue(id) (0x0810E350): the 0xa7 packet of the id now (0x080A8CC0), whatever its flags; nothing without an argument
int l_SyncTaskValue(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    KNpc* p = player_of(L, "SyncTaskValue");
    if (p == nullptr) return 0;
    g_ScriptContext().world->task_send_value(*p, task_int(L, 1));
    return 0;
}

// SyncTaskValueMore(first, last [, only_non_zero]) (0x0810E240): the 0xb5 packets of the range (0x080A9550) -> 1, or 0
// for a bad range; nothing with fewer than two arguments (0x0810E257)
int l_SyncTaskValueMore(lua_State* L)
{
    const int top = lua_gettop(L);
    if (top <= 1) return 0;
    KNpc* p = player_of(L, "SyncTaskValueMore");
    if (p == nullptr) {
        lua_pushnumber(L, 0);
        return 1;
    }
    const int first = task_int(L, 1);
    const int last = task_int(L, 2);
    const int only = top == 2 ? 0 : task_int(L, 3);
    lua_pushnumber(L, g_ScriptContext().world->task_sync_more(*p, first, last, only != 0) ? 1 : 0);
    return 1;
}

// GetBitTask(id, start, count) (0x081090A0): `count` bits from `start` of the value (GetBits 0x080CB5E0) as an unsigned
// number; nothing with fewer than three arguments (0x081090D8) or without a player
int l_GetBitTask(lua_State* L)
{
    const KNpc* p = player_of(L, "GetBitTask");
    if (p == nullptr || lua_gettop(L) <= 2) return 0;
    lua_pushnumber(L, static_cast<lua_Number>(p->player.task.get_bits(task_int(L, 1), task_int(L, 2), task_int(L, 3))));
    return 1;
}

// SetBitTask(id, start, count, value) (0x08108F10): SetBits 0x080CB910 -> 1 / 0; id 1 is traced (0x08109032); nothing with
// fewer than four arguments (0x08108F50) or without a player.  SetBits writes the map itself: the client is not told
int l_SetBitTask(lua_State* L)
{
    KNpc* p = player_of(L, "SetBitTask");
    if (p == nullptr || lua_gettop(L) <= 3) return 0;
    const int id = task_int(L, 1);
    const int value = task_int(L, 4);
    const bool ok = p->player.task.set_bits(id, task_int(L, 2), task_int(L, 3), value);
    lua_pushnumber(L, ok ? 1 : 0);
    if (id == kTaskTraceId) {
        log::info("zone.task", "trace task value", {log::kv("entity", p->id), log::kv("id", id), log::kv("value", value), log::kv("name", p->name)});
    }
    return 1;
}

// ---- the TASKSYS library (KTaskManager, docs/LINUX-SERVER.md §22) ----

// the text of an argument the way the old Lua handed it to lua_tostring: a number is written as an integer ("102",
// what TaskNo returned), a string as is, anything else nothing
std::optional<std::string> task_text(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TNUMBER) {
        const lua_Number n = lua_tonumber(L, idx);
        if (!(n > -9.0e18 && n < 9.0e18)) return std::nullopt;
        return std::to_string(static_cast<long long>(n));
    }
    if (lua_type(L, idx) == LUA_TSTRING) return std::string(lua_tostring(L, idx));
    return std::nullopt;
}

const KTaskManager* task_tables(lua_State* L, const char* fn)
{
    KScriptContext& c = g_ScriptContext();
    if (c.world == nullptr) {
        log::warn("lua", "script api called without a player", {log::kv("function", fn)});
        (void)L;
        return nullptr;
    }
    return c.world->config().tasks.get();
}

// TaskName(id) (0x08174CB0): the name of the task with that id (0x08170060), nil when none; one argument exactly
int l_TaskName(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const KTaskManager* m = task_tables(L, "TaskName");
    const char* name = m == nullptr ? nullptr : m->name_of(task_int(L, 1));
    if (name == nullptr) lua_pushnil(L);
    else lua_pushstring(L, name);
    return 1;
}

// TaskNo(name) (0x08174740): the id of the task with that name (0x08170440), nil when none
int l_TaskNo(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const KTaskManager* m = task_tables(L, "TaskNo");
    const auto name = task_text(L, 1);
    const auto id = m != nullptr && name ? m->id_of(*name) : std::nullopt;
    if (!id) lua_pushnil(L);
    else lua_pushinteger(L, *id);
    return 1;
}

// GetTaskStatus(name) (0x08175090): the two status bits of the task, nil when the name or the player is unknown
int l_GetTaskStatus(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;   // 0x081750D4: not a string -> nothing
    const KNpc* p = player_of(L, "GetTaskStatus");
    const auto status = p == nullptr ? std::nullopt : g_ScriptContext().world->task_status(*p, *name);
    if (!status) lua_pushnil(L);
    else lua_pushinteger(L, *status);
    return 1;
}

// SetTaskStatus(name, status) (0x08174B70): the bits set -> 1, else 0
int l_SetTaskStatus(lua_State* L)
{
    if (lua_gettop(L) != 2) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    KNpc* p = player_of(L, "SetTaskStatus");
    const bool ok = p != nullptr && g_ScriptContext().world->task_set_status(*p, *name, task_int(L, 2));
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// StartTask(name) (0x081748E0): a group for the task among the temp values (0x0820E4E0); 1 when the name is known
// whatever happened to the group (0x081749B2), 0 otherwise
int l_StartTask(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    KNpc* p = player_of(L, "StartTask");
    const KTaskManager* m = task_tables(L, "StartTask");
    if (p == nullptr || m == nullptr || !m->id_of(*name)) {
        lua_pushinteger(L, 0);
        return 1;
    }
    g_ScriptContext().world->task_start(*p, *name);
    lua_pushinteger(L, 1);
    return 1;
}

// CloseTask(name) (0x081747D0): the group and its temp values dropped (0x0820E430) -> 1, 0 when there was none
int l_CloseTask(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    KNpc* p = player_of(L, "CloseTask");
    const bool ok = p != nullptr && g_ScriptContext().world->task_close(*p, *name);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// GetTmpValue(name, key) (0x08174F40): the temp value of the task under the key (0x0820DF10), nil when none
int l_GetTmpValue(lua_State* L)
{
    if (lua_gettop(L) != 2) return 0;
    const auto name = task_text(L, 1);
    const auto key = task_text(L, 2);
    if (!name || !key) return 0;
    const KNpc* p = player_of(L, "GetTmpValue");
    const auto v = p == nullptr ? std::nullopt : g_ScriptContext().world->task_temp(*p, *name, *key);
    if (!v) lua_pushnil(L);
    else lua_pushinteger(L, *v);
    return 1;
}

// SetTmpValue(name, key, value) (0x081749F0): the temp value set (0x0820E5C0, a group when none) -> 1, 0 when the name is unknown
int l_SetTmpValue(lua_State* L)
{
    if (lua_gettop(L) != 3) return 0;
    const auto name = task_text(L, 1);
    const auto key = task_text(L, 2);
    if (!name || !key) return 0;
    KNpc* p = player_of(L, "SetTmpValue");
    const bool ok = p != nullptr && g_ScriptContext().world->task_set_temp(*p, *name, *key, task_int(L, 3));
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// FirstTask() (0x08174E30): the name of the first task the player has a group for, nil when none
int l_FirstTask(lua_State* L)
{
    KNpc* p = player_of(L, "FirstTask");
    if (p == nullptr) return 0;
    const char* name = g_ScriptContext().world->task_first(*p);
    if (name == nullptr) lua_pushnil(L);
    else lua_pushstring(L, name);
    return 1;
}

// NextTask() (0x08174D40): the next one, nil at the end (the list starts over with FirstTask)
int l_NextTask(lua_State* L)
{
    KNpc* p = player_of(L, "NextTask");
    if (p == nullptr) return 0;
    const char* name = g_ScriptContext().world->task_next(*p);
    if (name == nullptr) lua_pushnil(L);
    else lua_pushstring(L, name);
    return 1;
}

// TaskXxx(name, row, col) (0x081756D0 / 0x08175520 / 0x08175370 / 0x081751C0 / 0x08175A30 / 0x08175880): the cell of the
// task's matrix in that table, rows and columns from 1, nil outside; TaskXxxMatrix(name): rows, cols of it
int task_cell(lua_State* L, const char* fn, const KTaskMatrix* (KTaskManager::*table)(std::string_view) const)
{
    if (lua_gettop(L) != 3) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    const int row = task_int(L, 2);
    const int col = task_int(L, 3);
    if (row <= 0 || col <= 0) return 0;   // 0x0817576E / 0x08175778
    const KTaskManager* m = task_tables(L, fn);
    const KTaskMatrix* mat = m == nullptr ? nullptr : (m->*table)(*name);
    const std::string* cell = mat == nullptr ? nullptr : mat->cell(row - 1, col - 1);
    if (cell == nullptr) lua_pushnil(L);
    else lua_pushlstring(L, cell->data(), cell->size());
    return 1;
}

int task_matrix(lua_State* L, const char* fn, const KTaskMatrix* (KTaskManager::*table)(std::string_view) const)
{
    if (lua_gettop(L) != 1) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    const KTaskManager* m = task_tables(L, fn);
    const KTaskMatrix* mat = m == nullptr ? nullptr : (m->*table)(*name);
    if (mat == nullptr) return 0;   // 0x08175830
    lua_pushinteger(L, mat->row_count());
    lua_pushinteger(L, mat->cols);
    return 2;
}

int l_TaskCondition(lua_State* L) { return task_cell(L, "TaskCondition", &KTaskManager::condition); }
int l_TaskConditionMatrix(lua_State* L) { return task_matrix(L, "TaskConditionMatrix", &KTaskManager::condition); }
int l_TaskEntity(lua_State* L) { return task_cell(L, "TaskEntity", &KTaskManager::entity); }
int l_TaskEntityMatrix(lua_State* L) { return task_matrix(L, "TaskEntityMatrix", &KTaskManager::entity); }
int l_TaskAward(lua_State* L) { return task_cell(L, "TaskAward", &KTaskManager::award); }
int l_TaskAwardMatrix(lua_State* L) { return task_matrix(L, "TaskAwardMatrix", &KTaskManager::award); }
int l_TaskTalk(lua_State* L) { return task_cell(L, "TaskTalk", &KTaskManager::talk); }
int l_TaskTalkMatrix(lua_State* L) { return task_matrix(L, "TaskTalkMatrix", &KTaskManager::talk); }
int l_TaskId(lua_State* L) { return task_cell(L, "TaskId", &KTaskManager::id_matrix); }
int l_TaskIdMatrix(lua_State* L) { return task_matrix(L, "TaskIdMatrix", &KTaskManager::id_matrix); }
int l_TaskEvent(lua_State* L) { return task_cell(L, "TaskEvent", &KTaskManager::event_matrix); }
int l_TaskEventMatrix(lua_State* L) { return task_matrix(L, "TaskEventMatrix", &KTaskManager::event_matrix); }

// GetTaskEventID(name) (0x08174230): the EventID column of the task (0x08170830), nil when the name is unknown
int l_GetTaskEventID(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const auto name = task_text(L, 1);
    if (!name) return 0;
    const KTaskManager* m = task_tables(L, "GetTaskEventID");
    const auto ev = m == nullptr ? std::nullopt : m->event_of(*name);
    if (!ev) lua_pushnil(L);
    else lua_pushinteger(L, *ev);
    return 1;
}

// GetEventTaskCount(event) (0x08174430): how many tasks name the event (0x081701A0)
int l_GetEventTaskCount(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    const KTaskManager* m = task_tables(L, "GetEventTaskCount");
    lua_pushinteger(L, m == nullptr ? 0 : m->event_task_count(task_int(L, 1)));
    return 1;
}

// GetEventTask(event, index) (0x081742C0): the name of the index-th task of the event, from 0 (0x08170200), nil outside
int l_GetEventTask(lua_State* L)
{
    if (lua_gettop(L) != 2) return 0;
    const KTaskManager* m = task_tables(L, "GetEventTask");
    const std::string* name = m == nullptr ? nullptr : m->event_task(task_int(L, 1), task_int(L, 2));
    if (name == nullptr) lua_pushnil(L);
    else lua_pushlstring(L, name->data(), name->size());
    return 1;
}

// SubWorldName(index) (0x08174390): the name of the subworld - the zone has one map; its id as text stands in for a
// name until the map names of the old server are exported (the talk tables compare it with TalkNpcMap)
int l_SubWorldName(lua_State* L)
{
    if (lua_gettop(L) != 1) return 0;
    KScriptContext& c = g_ScriptContext();
    if (c.world == nullptr || task_int(L, 1) < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, std::to_string(c.world->map_id()).c_str());
    return 1;
}

// SelectTaskStart / SelectTaskFinish / SelectTaskAward(id) (0x08174690 / 0x081745E0 / 0x08174530): the menu functions of
// task_function.lua (OnMenuTaskStart / OnMenuTaskFinish / OnMenuTaskAward) with the id, for the player
int task_select(lua_State* L, const char* fn, const char* menu)
{
    if (lua_gettop(L) != 1) return 0;
    KNpc* p = player_of(L, fn);
    if (p == nullptr) return 0;
    g_ScriptContext().world->task_select(*p, menu, task_int(L, 1));
    return 0;
}

int l_SelectTaskStart(lua_State* L) { return task_select(L, "SelectTaskStart", "OnMenuTaskStart"); }
int l_SelectTaskFinish(lua_State* L) { return task_select(L, "SelectTaskFinish", "OnMenuTaskFinish"); }
int l_SelectTaskAward(lua_State* L) { return task_select(L, "SelectTaskAward", "OnMenuTaskAward"); }

// ---- the player events and the npc helpers of the task scripts (KPlayerEvent.h, docs/LINUX-SERVER.md §23) ----

// the entity behind a "npc index" argument (the old index was a small number; the zone's ids are 64-bit)
EntityId entity_arg(lua_State* L, int idx)
{
    const lua_Number n = lua_tonumber(L, idx);
    if (!(n >= 0.0 && n < 18446744073709551616.0)) return EntityId{};
    return EntityId{static_cast<std::uint64_t>(n)};
}

// AddPlayerEvent(id) (0x0810C510): the event on the player's list (0x081560C0) -> 1; 0 when the list is full or no player
int l_AddPlayerEvent(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    KNpc* p = player_of(L, "AddPlayerEvent");
    const bool ok = p != nullptr && g_ScriptContext().world->player_event_add(*p, task_int(L, 1) & 0xffff);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// RemovePlayerEvent(id) (0x0810C440): the event off the list (0x08156050) -> 1; 0 when it was not there or no player
int l_RemovePlayerEvent(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    KNpc* p = player_of(L, "RemovePlayerEvent");
    const bool ok = p != nullptr && g_ScriptContext().world->player_event_remove(*p, task_int(L, 1) & 0xffff);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// RemoveAllPlayerEvent() (0x0810C3E0): the list cleared (0x08155F60) -> 1
int l_RemoveAllPlayerEvent(lua_State* L)
{
    KNpc* p = player_of(L, "RemoveAllPlayerEvent");
    if (p == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    p->player.events.clear();
    lua_pushinteger(L, 1);
    return 1;
}

// GetNpcName(index) (0x08100040): the name of the npc (+0x1505), nil when the index names none.  (The zone's names are
// UTF-8: a script comparing them with the TCVN3 bytes of a table sees no match yet)
int l_GetNpcName(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    const KNpc* n = c.world == nullptr ? nullptr : c.world->find_entity(entity_arg(L, 1));
    if (n == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushlstring(L, n->name.data(), n->name.size());
    return 1;
}

// GetNpcPos(index) (0x081293F0): x, y of the npc (cells, like GetPos) and its subworld index; one argument; a
// npc the index does not name gives a single 0
int l_GetNpcPos(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    const KNpc* n = lua_gettop(L) == 1 && c.world != nullptr ? c.world->find_entity(entity_arg(L, 1)) : nullptr;
    if (n == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    const Pos a = c.world->to_absolute(n->pos());
    lua_pushinteger(L, a.x / 32);
    lua_pushinteger(L, a.y / 32);
    lua_pushinteger(L, 0);
    return 3;
}

// NpcName2Replace(name) (0x081006D0): the name through the replacement table 0x080A0420 (the Taiwanese names of the
// old data); the zone has no such table - the name comes back as it is
int l_NpcName2Replace(lua_State* L)
{
    if (lua_type(L, 1) != LUA_TSTRING) return 0;
    lua_pushvalue(L, 1);
    return 1;
}

// NpcDialog() (0x081744B0): the npc the player talked to last (Player+0xc) runs its script's main for the player again
int l_NpcDialog(lua_State* L)
{
    KNpc* p = player_of(L, "NpcDialog");
    if (p == nullptr) return 0;
    KSubWorld* w = g_ScriptContext().world;
    const KNpc* npc = w->find_entity(p->player.dialog.npc);
    if (npc != nullptr && !npc->script.empty()) w->execute_script(npc->script, "main", *p, npc->script_main_param);   // main(npc+0x158c)
    return 0;
}

// GetLastDiagNpc() (0x0810C5E0): the index of the npc the player talked to last (Player+0xc), 0 when none or gone
int l_GetLastDiagNpc(lua_State* L)
{
    const KNpc* p = player_of(L, "GetLastDiagNpc");
    if (p == nullptr) return 0;
    const KNpc* npc = g_ScriptContext().world->find_entity(p->player.dialog.npc);
    lua_pushinteger(L, npc == nullptr ? 0 : static_cast<lua_Integer>(p->player.dialog.npc.value));   // an integer: tostring gives "0" like the old Lua
    return 1;
}

// GetNpcSettingIdx(index) (0x080FDE50): the template id of the npc (+0x1530); 0 when the index names none
int l_GetNpcSettingIdx(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    const KNpc* n = lua_gettop(L) == 1 && c.world != nullptr ? c.world->find_entity(entity_arg(L, 1)) : nullptr;
    lua_pushinteger(L, n == nullptr ? 0 : static_cast<lua_Integer>(n->template_id));
    return 1;
}

// GetLevel() (0x081111E0): the level of the player's npc (+0x20)
int l_GetLevel(lua_State* L)
{
    const KNpc* p = player_of(L, "GetLevel");
    if (p == nullptr) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(p->level));
    return 1;
}

// GetName() (0x08111E70): the player's name; nil without a player
int l_GetName(lua_State* L)
{
    const KNpc* p = player_of(L, "GetName");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushlstring(L, p->name.data(), p->name.size());
    return 1;
}

// GetSex() (0x08112020): the sex of the player's npc (+0x152c)
int l_GetSex(lua_State* L)
{
    const KNpc* p = player_of(L, "GetSex");
    if (p == nullptr) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(p->sex));
    return 1;
}

// ---- the TabFile_* library (KTabFile.h, docs/LINUX-SERVER.md §24) ----

// the cache of the process: an Include of a script may call TabFile_Load before any world or player is on the context
KTabFileCache* tab_files(lua_State* L, const char* fn)
{
    (void)L;
    (void)fn;
    return &g_TabFiles();
}

// TabFile_Load(file, key [, writable]) (0x0814AEF0): the table of the game path under the key (0x0814D1A0 on the read cache,
// 0x0814CDE0 on the writable one with a third argument) -> 1 / 0; two arguments at least
int l_TabFile_Load(lua_State* L)
{
    const int top = lua_gettop(L);
    if (top <= 1) return 0;
    const char* file = lua_tostring(L, 1);
    const char* key = lua_tostring(L, 2);
    KTabFileCache* cache = tab_files(L, "TabFile_Load");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    const int ok = cache != nullptr && file != nullptr && key != nullptr ? cache->load(file, key, top >= 3) : 0;
    if (ok == 0) log::debug("lua", "tab file not loaded", {log::kv("file", text::decode_mixed(file ? file : "")), log::kv("key", text::decode_mixed(key ? key : ""))});
    lua_pushinteger(L, ok);
    return 1;
}

// TabFile_UnLoad(key) (0x0814B040) -> 1 when it was there
int l_TabFile_UnLoad(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    const char* key = lua_tostring(L, 1);
    KTabFileCache* cache = tab_files(L, "TabFile_UnLoad");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    lua_pushinteger(L, cache != nullptr && key != nullptr && cache->unload(key) ? 1 : 0);
    return 1;
}

// TabFile_GetRowCount(key) (0x0814A690): GetHeight, 0 without the table; TabFile_GetColCount(key) (0x0814A5E0): GetWidth
int l_TabFile_GetRowCount(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    const char* key = lua_tostring(L, 1);
    KTabFileCache* cache = tab_files(L, "TabFile_GetRowCount");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    const KTabFile* t = cache != nullptr && key != nullptr ? cache->find(key) : nullptr;
    lua_pushinteger(L, t == nullptr ? 0 : t->height());
    return 1;
}

int l_TabFile_GetColCount(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    const char* key = lua_tostring(L, 1);
    KTabFileCache* cache = tab_files(L, "TabFile_GetColCount");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    const KTabFile* t = cache != nullptr && key != nullptr ? cache->find(key) : nullptr;
    lua_pushinteger(L, t == nullptr ? 0 : t->width());
    return 1;
}

// the column of a TabFile argument: a number, or the header name (FindColumn); -1 when neither names one
int tab_column(lua_State* L, int idx, const KTabFile& t)
{
    if (lua_type(L, idx) == LUA_TSTRING) return t.find_column(lua_tostring(L, idx));
    return task_int(L, idx);
}

// TabFile_GetCell(key, row, column) (0x0814A740): GetString(row, column, "", buf, 0x400) - the cell, or "" (a column by its
// header name too); three arguments at least; "" without the table (0x0814A8A0)
int l_TabFile_GetCell(lua_State* L)
{
    if (lua_gettop(L) <= 2) return 0;
    const char* key = lua_tostring(L, 1);
    KTabFileCache* cache = tab_files(L, "TabFile_GetCell");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    const KTabFile* t = cache != nullptr && key != nullptr ? cache->find(key) : nullptr;
    std::string cell;
    if (t != nullptr) t->get_string(task_int(L, 2), tab_column(L, 3, *t), cell, 0x400);
    lua_pushlstring(L, cell.data(), cell.size());
    return 1;
}

// TabFile_Search(key, column, value) (0x0814A960): the first row whose cell in the column equals the value (0x08227C90 /
// 0x08227BE0), -1 when none or no table
int l_TabFile_Search(lua_State* L)
{
    if (lua_gettop(L) <= 2) return 0;
    const char* key = lua_tostring(L, 1);
    const char* value = lua_tostring(L, 3);
    KTabFileCache* cache = tab_files(L, "TabFile_Search");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    const KTabFile* t = cache != nullptr && key != nullptr ? cache->find(key) : nullptr;
    lua_pushinteger(L, t == nullptr || value == nullptr ? -1 : t->find_row(tab_column(L, 2, *t), value));
    return 1;
}

// TabFile_SetCell(key, row, column, value) (0x0814A420): the cell of the table in memory -> 1 / 0; four arguments at least
int l_TabFile_SetCell(lua_State* L)
{
    if (lua_gettop(L) <= 3) return 0;
    const char* key = lua_tostring(L, 1);
    const char* value = lua_tostring(L, 4);
    KTabFileCache* cache = tab_files(L, "TabFile_SetCell");
    const std::lock_guard<std::mutex> guard(g_TabFilesLock());
    KTabFile* t = cache != nullptr && key != nullptr ? cache->find(key) : nullptr;
    const bool ok = t != nullptr && value != nullptr && t->set_string(task_int(L, 2), tab_column(L, 3, *t), value);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// TabFile_Save(key) (0x0814A3A0): the old server wrote the table back into its file; the zone keeps the old data as it
// found it -> 0 always
int l_TabFile_Save(lua_State* L)
{
    if (lua_gettop(L) < 1) return 0;
    const char* key = lua_tostring(L, 1);
    log::warn("lua", "tab file save refused", {log::kv("key", text::decode_mixed(key ? key : ""))});
    lua_pushinteger(L, 0);
    return 1;
}

// ---- S1 (docs/SCRIPT-API.md, docs/LINUX-SERVER.md §30): the script api the Linux scripts call most that needs no new packet

// the wall clock the scripts see: time(0) plus the map's offset (jx_linux_y adds [0x9789ee4] / [0x9789ee8] once the clock
// flag [0x9789ee0] is set, 0 before it)
std::int64_t script_now()
{
    const KSubWorld* w = g_ScriptContext().world;
    return static_cast<std::int64_t>(std::time(nullptr)) + (w != nullptr ? w->config().time_offset : 0);
}

bool local_time(std::time_t t, std::tm& out)
{
#ifdef _WIN32
    return localtime_s(&out, &t) == 0;
#else
    return localtime_r(&t, &out) != nullptr;
#endif
}

// the conversions strftime knows (C99; glibc prints an unknown one as it is, the Windows runtime aborts on it - so an
// unknown one is refused like an empty result is, "invalid `date' format")
bool strftime_format_ok(const char* f)
{
    static const char* const kConv = "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%";
    for (const char* p = f; *p != '\0'; ++p) {
        if (*p != '%') continue;
        ++p;
        if (*p == 'E' || *p == 'O') ++p;
        if (*p == '\0' || std::strchr(kConv, *p) == nullptr) return false;
    }
    return true;
}

// GetLocalDate(format) (0x0812A140): nothing without an argument; localtime(now + offset) - and when that is daylight saving
// time (tm_isdst == 1) an hour earlier with tm_isdst cleared (0x0812A260: the game keeps standard time); strftime into 0x100
// bytes; an empty result raises "invalid `date' format".  Returns the string.
int l_GetLocalDate(lua_State* L)
{
    if (lua_gettop(L) <= 0) return 0;
    const char* fmt = lua_tostring(L, 1);
    if (fmt == nullptr) fmt = "";
    auto t = static_cast<std::time_t>(script_now());
    std::tm tm{};
    if (local_time(t, tm) && tm.tm_isdst == 1) {
        t -= 3600;
        local_time(t, tm);
        tm.tm_isdst = 0;
    }
    char buf[0x100];
    const std::size_t n = strftime_format_ok(fmt) ? std::strftime(buf, sizeof buf, fmt, &tm) : 0;
    if (n == 0) return luaL_error(L, "invalid `date' format");
    lua_pushlstring(L, buf, n);
    return 1;
}

// GetCurServerTime() (0x08103800): time(0) + the offset once the clock is set (0 before; the zone's clock is always set)
int l_GetCurServerTime(lua_State* L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(script_now()));
    return 1;
}

// SubWorldID2Idx(id) (0x08102580): -1 without an argument; else KSubWorldSet::GetSubWorldIdx 0x080F68A0 - the first hosted
// map whose id (SubWorld+0xc) is it, -1 when none.  This zone's index of a map IS its id (GetWorldPos / NewWorld speak in
// map ids), so a hosted map answers its own id.
int l_SubWorldID2Idx(lua_State* L)
{
    lua_Integer idx = -1;
    if (lua_gettop(L) > 0) {
        const auto id = static_cast<std::int64_t>(lua_tonumber(L, 1));
        const KSubWorld* w = g_ScriptContext().world;
        if (id >= 0 && w != nullptr && w->hosts_map(static_cast<std::uint32_t>(id))) idx = id;
    }
    lua_pushinteger(L, idx);
    return 1;
}

// SubWorldIdx2ID([idx]) (0x081077D0): without an argument the script's own map (the "SubWorld" global of the old states,
// 0x08106A40); an index below the count answers its id, anything else 0
int l_SubWorldIdx2ID(lua_State* L)
{
    const KSubWorld* w = g_ScriptContext().world;
    lua_Integer id = 0;
    if (lua_gettop(L) <= 0) {
        if (w != nullptr) id = w->map_id();
    } else {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        if (idx >= 0 && w != nullptr && w->hosts_map(static_cast<std::uint32_t>(idx))) id = idx;
    }
    lua_pushinteger(L, id);
    return 1;
}

std::uint32_t arg_u32(lua_State* L, int i)
{
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(lua_tonumber(L, i)));   // fistp then the low dword
}

// GetBit(value, bit) (0x080FEBA0): bit 1..32 -> (value >> (bit - 1)) & 1, else 0; the arguments are read as numbers whatever
// they are (a missing one is 0)
int l_GetBit(lua_State* L)
{
    const std::uint32_t value = arg_u32(L, 1);
    const auto bit = static_cast<std::int64_t>(lua_tonumber(L, 2));
    lua_pushinteger(L, bit >= 1 && bit <= 32 ? static_cast<lua_Integer>((value >> (bit - 1)) & 1u) : 0);
    return 1;
}

// SetBit(value, bit, on) (0x080FEAC0): bit 1..32 -> the bit set when `on` is exactly 1, cleared otherwise (0x080FEB5A: the
// mask 0xfffffffe rolled); outside 1..32 the value as it is.  The result is the unsigned 32-bit value (0x080FEB72).
int l_SetBit(lua_State* L)
{
    std::uint32_t value = arg_u32(L, 1);
    const auto bit = static_cast<std::int64_t>(lua_tonumber(L, 2));
    const auto on = static_cast<std::int64_t>(lua_tonumber(L, 3));
    if (bit >= 1 && bit <= 32) {
        const std::uint32_t mask = 1u << (bit - 1);
        value = on == 1 ? (value | mask) : (value & ~mask);
    }
    lua_pushinteger(L, static_cast<lua_Integer>(value));
    return 1;
}

// GetByte(value, n) (0x080FEA20): byte n = 1..4 of the 32-bit value (0x080FEA7D: (n - 1) * 8), else 0
int l_GetByte(lua_State* L)
{
    const std::uint32_t value = arg_u32(L, 1);
    const auto n = static_cast<std::int64_t>(lua_tonumber(L, 2));
    lua_pushinteger(L, n >= 1 && n <= 4 ? static_cast<lua_Integer>((value >> ((n - 1) * 8)) & 0xffu) : 0);
    return 1;
}

// SetByte(value, n, byte) (0x080FE950): byte n = 1..4 of the value replaced by the low byte of `byte` (0x080FE9E8); outside
// 1..4 the value as it is (unsigned 32-bit)
int l_SetByte(lua_State* L)
{
    std::uint32_t value = arg_u32(L, 1);
    const auto n = static_cast<std::int64_t>(lua_tonumber(L, 2));
    const std::uint32_t b = arg_u32(L, 3) & 0xffu;
    if (n >= 1 && n <= 4) {
        const int shift = static_cast<int>((n - 1) * 8);
        value = (value & ~(0xffu << shift)) | (b << shift);
    }
    lua_pushinteger(L, static_cast<lua_Integer>(value));
    return 1;
}

// GetMissionV(idx) (0x081072F0): the script's map (the SubWorld global, 0x08106A40) and idx 1..99 -> the map's mission value
// (SubWorld+0x484b8 + idx * 4), else 0
int l_GetMissionV(lua_State* L)
{
    const KSubWorld* w = g_ScriptContext().world;
    lua_Integer v = 0;
    if (lua_gettop(L) > 0 && w != nullptr) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        if (idx >= 1 && idx <= 99) v = w->mission_value(static_cast<int>(idx));
    }
    lua_pushinteger(L, v);
    return 1;
}

// SetMissionV(idx, value) (0x08107390): idx 0..99 -> the map's mission value = int(value); returns nothing
int l_SetMissionV(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    if (lua_gettop(L) > 1 && w != nullptr) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        const auto value = static_cast<std::int64_t>(lua_tonumber(L, 2));
        if (idx >= 0 && idx <= 99) w->set_mission_value(static_cast<int>(idx), static_cast<int>(value));
    }
    return 0;
}

// GetItemName(idx) (0x081005D0): the item of that index (Item[] 0x830D300, 1..count-1; an id of the player's list here) ->
// its name (Item+0x2c), else nil
int l_GetItemName(lua_State* L)
{
    if (lua_gettop(L) > 0) {
        if (const KItem* it = script_item(L, "GetItemName")) {
            lua_pushstring(L, it->name().c_str());
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

// GetItemParam(idx, n) (0x080FECC0): two arguments, a live item and n 1..6 -> Item+0x1e0 + n * 4 (the six numbers
// SetItemMagicLevel writes: KItem::magic_level), else 0
int l_GetItemParam(lua_State* L)
{
    lua_Integer v = 0;
    if (lua_gettop(L) > 1) {
        const KItem* it = script_item(L, "GetItemParam");
        const auto n = static_cast<std::int64_t>(lua_tonumber(L, 2));
        if (it != nullptr && n >= 1 && n <= 6) v = it->magic_level[static_cast<std::size_t>(n - 1)];
    }
    lua_pushinteger(L, v);
    return 1;
}

// GetExp() (0x08117860): the player's m_nExp (Player+0x595c, 64-bit); nil without a player (0x081178A0)
int l_GetExp(lua_State* L)
{
    const KNpc* p = player_of(L, "GetExp");
    if (p == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, static_cast<lua_Integer>(p->player.exp));
    return 1;
}

// GetExtPoint(n) (0x0810FA90 -> KPlayer 0x080A8080): n 0..7 -> Player+0x44 + n * 4; 0 otherwise, without a player or an argument
int l_GetExtPoint(lua_State* L)
{
    lua_Integer v = 0;
    if (lua_gettop(L) > 0) {
        if (const KNpc* p = player_of(L, "GetExtPoint")) {
            const auto n = static_cast<std::int64_t>(lua_tonumber(L, 1));
            if (n >= 0 && n < KPlayer::kExtPoints) v = p->player.ext_point[static_cast<std::size_t>(n)];
        }
    }
    lua_pushinteger(L, v);
    return 1;
}

// AddExtPoint(n, value) (0x0810FBE0) / AddExtPointForGS (0x0810FB20): two arguments, a player, value >= 0 (0x0810FC48) ->
// KPlayer::AddExtPoint 0x080AB090(n, value, for_gs) -> 1 when n is 0..7 (the point added, the KSG line logged for GS), else 0
int ext_point_add(lua_State* L, const char* fn, bool for_gs)
{
    lua_Integer ok = 0;
    if (lua_gettop(L) > 1) {
        if (KNpc* p = player_of(L, fn)) {
            const auto n = static_cast<std::int64_t>(lua_tonumber(L, 1));
            const auto value = static_cast<std::int64_t>(lua_tonumber(L, 2));
            if (value >= 0 && p->player.add_ext_point(static_cast<int>(n), static_cast<int>(value))) {
                ok = 1;
                log::info("zone.player", "ext point", {log::kv("entity", p->id), log::kv("index", n), log::kv("value", value),
                                                       log::kv("gs", for_gs), log::kv("left", p->player.ext_point[static_cast<std::size_t>(n)])});
            }
        }
    }
    lua_pushinteger(L, ok);
    return 1;
}

int l_AddExtPoint(lua_State* L) { return ext_point_add(L, "AddExtPoint", false); }
int l_AddExtPointForGS(lua_State* L) { return ext_point_add(L, "AddExtPointForGS", true); }

// PayExtPoint(n, value) (0x0810FCA0): two arguments, a player, value >= 0 -> KPlayer::PayExtPoint 0x080AB100: n 0..7 and enough
// points -> paid (logged), 1; else 0
int l_PayExtPoint(lua_State* L)
{
    lua_Integer ok = 0;
    if (lua_gettop(L) > 1) {
        if (KNpc* p = player_of(L, "PayExtPoint")) {
            const auto n = static_cast<std::int64_t>(lua_tonumber(L, 1));
            const auto value = static_cast<std::int64_t>(lua_tonumber(L, 2));
            if (value >= 0 && p->player.pay_ext_point(static_cast<int>(n), static_cast<int>(value))) {
                ok = 1;
                log::info("zone.player", "ext point", {log::kv("entity", p->id), log::kv("index", n), log::kv("value", -value),
                                                       log::kv("gs", false), log::kv("left", p->player.ext_point[static_cast<std::size_t>(n)])});
            }
        }
    }
    lua_pushinteger(L, ok);
    return 1;
}

// CalcFreeItemCellCount() (0x0810CA20): a player -> KItemList::CalcFreeCellCount 0x081F8A90 of Player+0x5088 (the bag: the
// cells holding nothing); 0 without a player
int l_CalcFreeItemCellCount(lua_State* L)
{
    lua_Integer n = 0;
    if (const KNpc* p = player_of(L, "CalcFreeItemCellCount")) {
        if (const KItemList* items = g_ScriptContext().world->items_of(p->sid)) n = items->room(room_equipment).free_cells();
    }
    lua_pushinteger(L, n);
    return 1;
}

// SearchPlayer(name) (0x081020A0 -> KPlayerSet::SearchPlayer 0x080C6010): the player of that exact name -> its index (the
// entity id here), 0 for nobody, an empty name or no argument
int l_SearchPlayer(lua_State* L)
{
    lua_Integer idx = 0;
    if (lua_gettop(L) > 0) {
        const char* name = lua_tostring(L, 1);
        const KSubWorld* w = g_ScriptContext().world;
        if (name != nullptr && w != nullptr) {
            if (const KNpc* p = w->find_player_by_name(name)) idx = static_cast<lua_Integer>(p->id.value);
        }
    }
    lua_pushinteger(L, idx);
    return 1;
}

// CallPlayerFunction(player, fn, ...) (0x08129580): at least two arguments; `player` 1..0x4af (an entity id of a player here);
// `fn` a function value or the name of a global of the running script (an empty name does nothing, 0x08129633); the script's
// current player becomes `player` for the call (SetPlayerIndex 0x080FBF10 before, the old one back after, 0x081296EE), the
// remaining arguments are handed over (0x08221ED0 / 0x08221AF0 with top - 2 of them) and whatever the function returns is
// returned.  Nothing when the player or the function is missing, or the call fails.
int l_CallPlayerFunction(lua_State* L)
{
    const int top = lua_gettop(L);
    if (top <= 1) return 0;
    KSubWorld* w = g_ScriptContext().world;
    if (w == nullptr) return 0;
    const auto id = static_cast<std::int64_t>(lua_tonumber(L, 1));
    KNpc* target = id > 0 ? w->mutable_entity(EntityId{static_cast<std::uint64_t>(id)}) : nullptr;
    if (target == nullptr || target->kind != KNpcKind::player) return 0;
    const char* name = "";
    if (lua_type(L, 2) == LUA_TFUNCTION) {
        lua_pushvalue(L, 2);
    } else {
        name = lua_isstring(L, 2) ? lua_tostring(L, 2) : nullptr;
        if (name == nullptr || *name == '\0') return 0;
        lua_getglobal(L, name);
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            log::warn("lua", "call player function failed", {log::kv("function", name), log::kv("error", "not a function")});
            return 0;
        }
    }
    for (int i = 3; i <= top; ++i) lua_pushvalue(L, i);
    KScriptContext& ctx = g_ScriptContext();
    KNpc* const saved_player = ctx.player;
    const std::uint64_t saved_sid = ctx.sid;
    ctx.player = target;
    ctx.sid = target->sid;
    const int rc = lua_pcall(L, top - 2, LUA_MULTRET, 0);
    ctx.player = saved_player;
    ctx.sid = saved_sid;
    if (rc != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log::warn("lua", "call player function failed", {log::kv("function", name), log::kv("error", err != nullptr ? err : "?")});
        lua_pop(L, 1);
        return 0;
    }
    return lua_gettop(L) - top;   // the function's results sit above the arguments
}

// ---- S2 (docs/SCRIPT-API.md, docs/LINUX-SERVER.md §31): the item, npc and state functions the scripts call next

// AddEventItem(detail | name) (0x0811DF90): one argument and a player; a number is the DetailType of the quest item, a string
// its name in questkey.txt (KTabFile::GetInteger row-by-name "DetailType", 0x0811E017); KItemSet::Add 0x0806E110(genre 4,
// quality 0, series 0, level 0, luck 0, detail, particular 0, no levels, the current version) and KPlayer::AddItem(idx, 1, 1, 0)
// into the bag -> the item's index (0x0811E190); a failed add frees the piece (0x0806DB90) and 0 comes back
int l_AddEventItem(lua_State* L)
{
    lua_Integer id = 0;
    if (lua_gettop(L) > 0) {
        if (KNpc* p = player_of(L, "AddEventItem")) {
            KSubWorld* w = g_ScriptContext().world;
            const int detail = quest_detail_arg(L, 1);
            std::optional<KItem> item = detail >= 0 ? make_script_item(w, w->item_version(), 0, 0, static_cast<int>(KItemGenre::task), detail, 0, 0, 0, 0, nullptr)
                                                    : std::nullopt;
            if (!item) {
                log::warn("lua", "AddItem: no such item", {log::kv("function", "AddEventItem"), log::kv("detail", detail)});
            } else {
                id = w->give_item(p->sid, std::move(*item));
                if (id == 0) log::info("lua", "AddItem: bag full", {log::kv("entity", p->id), log::kv("genre", 4), log::kv("detail", detail)});
            }
        }
    }
    lua_pushinteger(L, id);
    return 1;
}

// AddQualityItem([tag,] quality, genre, detail, particular, level, series, luck, m1 .. m6) (0x081206A0): a leading string is
// dropped (0x08120810); fewer than seven numbers -> the sentence 0x978a464 and 0; a player; the LAST argument is dropped
// (lua_settop(-2), 0x081206F3), the current table version and a zero seed go in front and Lua_NewItem 0x0811F230 rolls the
// piece; KPlayer::AddItem(idx, 1, 1, 0) -> the index (0x081207F8), else freed and 0.  Only quality 0 rolls here: a gold (1)
// or platina (2, Gen 0x0806B6C0) piece waits for the quality table of KItemSet::Add (docs/HANDOVER.md S3) and gives 0.
int l_AddQualityItem(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TSTRING) lua_remove(L, 1);
    const int n = lua_gettop(L);
    if (n <= 6) {
        log::warn("lua", "AddQualityItem: too few arguments", {log::kv("count", n)});
        lua_pushinteger(L, 0);
        return 1;
    }
    KNpc* p = player_of(L, "AddQualityItem");
    if (p == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_settop(L, n - 1);   // the last argument is dropped before the roll
    const auto quality = static_cast<int>(lua_tonumber(L, 1));
    const auto genre = static_cast<int>(lua_tonumber(L, 2));
    const auto detail = static_cast<int>(lua_tonumber(L, 3));
    const auto particular = static_cast<int>(lua_tonumber(L, 4));
    const auto level = static_cast<int>(lua_tonumber(L, 5));
    const auto series = static_cast<int>(lua_tonumber(L, 6));
    const auto luck = static_cast<int>(lua_tonumber(L, 7));
    KMagicLevels levels{};
    bool with_magic = false;
    for (int i = 0; i < 6 && 8 + i <= n - 1; ++i) {
        levels[static_cast<std::size_t>(i)] = static_cast<int>(luaL_optnumber(L, 8 + i, 0));
        with_magic = with_magic || levels[static_cast<std::size_t>(i)] != 0;
    }
    KSubWorld* w = g_ScriptContext().world;
    std::optional<KItem> item = make_script_item(w, w->item_version(), 0, quality, genre, detail, particular, level, series, luck, with_magic ? &levels : nullptr);
    if (!item) {
        log::warn("lua", "AddItem: no such item", {log::kv("function", "AddQualityItem"), log::kv("quality", quality), log::kv("genre", genre),
                                                    log::kv("detail", detail), log::kv("particular", particular), log::kv("level", level)});
        lua_pushinteger(L, 0);
        return 1;
    }
    const std::uint32_t id = w->give_item(p->sid, std::move(*item));
    if (id == 0) log::info("lua", "AddItem: bag full", {log::kv("entity", p->id), log::kv("genre", genre), log::kv("detail", detail)});
    lua_pushinteger(L, id);
    return 1;
}

// the bag walk of CalcEquiproomItemCount / ConsumeEquiproomItem (KItemList 0x081FA770 / 0x08202410): the entries of the list
// in order, kept when in room 3 - the bag (+0x98 of the entry) - and of `genre` (Item+0), with the detail, particular and
// level (Item+8 / +0xc / +0x24) equal unless the argument is negative; a stackable piece (Item+0x14) counts its stack
// (Item+0x308, when 1..max(1, Item+0x30c)), anything else one
struct KBagMatch {
    std::uint32_t id;
    int units;
};

std::vector<KBagMatch> bag_matches(const KItemList& items, int genre, int detail, int particular, int level, int room)
{
    std::vector<KBagMatch> out;
    items.each([&](const KItem& it, const KItemPlace& place) {
        if ((room >= 0 && place.room != room) || static_cast<int>(it.genre) != genre) return;
        if (detail >= 0 && it.detail != detail) return;
        if (particular >= 0 && it.particular != particular) return;
        if (level >= 0 && it.level != level) return;
        int units = 1;
        if (it.tpl != nullptr && it.tpl->stackable) {
            const int max = std::max(1, it.max_stack());
            if (it.count >= 1 && it.count <= max) units = it.count;
        }
        out.push_back({it.id, units});
    });
    return out;
}

// the taking of KItemList 0x08202410 (ConsumeEquiproomItem / ConsumeItem): the matching pieces in order - a whole piece while
// `left` covers its units, the last stack shrunk by what is left (0x08200D30); 1 when everything asked was taken, -1 when the
// pieces ran out first (what was taken stays taken)
int consume_matches(KSubWorld& w, std::uint64_t sid, KItemList& items, int genre, int detail, int particular, int level, int room, int left)
{
    for (const KBagMatch& m : bag_matches(items, genre, detail, particular, level, room)) {
        if (left >= m.units) {
            w.take_item(sid, m.id);
            left -= m.units;
            if (left == 0) return 1;
        } else {
            if (KItem* it = items.find_mutable(m.id)) {
                it->count = m.units - left;
                w.sync_item(sid, m.id);
            }
            return 1;
        }
    }
    return -1;
}

// CalcEquiproomItemCount(genre, detail, particular, level) (0x0810D580): exactly four arguments and a player, else 0;
// KItemList 0x081FA770(list, genre, detail, particular, level, room 3, count stacks 1) - the units in the bag
int l_CalcEquiproomItemCount(lua_State* L)
{
    lua_Integer n = 0;
    if (lua_gettop(L) == 4) {
        if (const KNpc* p = player_of(L, "CalcEquiproomItemCount")) {
            if (const KItemList* items = g_ScriptContext().world->items_of(p->sid)) {
                for (const KBagMatch& m : bag_matches(*items, static_cast<int>(lua_tonumber(L, 1)), static_cast<int>(lua_tonumber(L, 2)),
                                                       static_cast<int>(lua_tonumber(L, 3)), static_cast<int>(lua_tonumber(L, 4)), room_equipment)) {
                    n += m.units;
                }
            }
        }
    }
    lua_pushinteger(L, n);
    return 1;
}

// ConsumeEquiproomItem(genre, detail, particular, level, count) (0x0810CEF0): exactly five arguments and a player, else -1;
// KItemList 0x08202410 walks the matching pieces of the bag in order: while `count` still covers a piece's units the whole
// piece goes (KItemList::Remove + KItemSet::Remove, 0x082025A8) and `count` drops by them - 0 left ends with 1; a piece
// with more units than what is left keeps units - left (0x08200D30) and 1 comes back; running out of pieces first leaves
// the taken ones taken and answers 0, -1 here (0x0810D02B)
int l_ConsumeEquiproomItem(lua_State* L)
{
    lua_Integer result = -1;
    if (lua_gettop(L) == 5) {
        if (const KNpc* p = player_of(L, "ConsumeEquiproomItem")) {
            KSubWorld* w = g_ScriptContext().world;
            KItemList* items = w->items_of(p->sid);
            int left = static_cast<int>(lua_tonumber(L, 5));
            if (items != nullptr) {
                result = consume_matches(*w, p->sid, *items, static_cast<int>(lua_tonumber(L, 1)), static_cast<int>(lua_tonumber(L, 2)),
                                         static_cast<int>(lua_tonumber(L, 3)), static_cast<int>(lua_tonumber(L, 4)), room_equipment, left);
            }
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

// GetNpcParam(npc, n) (0x081C0A00): two arguments, a live npc index (1..count) and n 1..10 -> Npc+0x19ac + n * 4 (the ten
// script numbers of a npc), else 0
int l_GetNpcParam(lua_State* L)
{
    lua_Integer v = 0;
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) > 1 && c.world != nullptr) {
        const KNpc* e = c.world->find_entity(entity_arg(L, 1));
        const auto n = static_cast<std::int64_t>(lua_tonumber(L, 2));
        if (e != nullptr && n >= 1 && n <= 10) v = e->script_param[static_cast<std::size_t>(n - 1)];
    }
    lua_pushinteger(L, v);
    return 1;
}

// SetNpcParam(npc, n, value) (0x081C0260): three arguments, a live npc and n 1..10 -> the number is stored (0x081C0374);
// nothing returned
int l_SetNpcParam(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) > 2 && c.world != nullptr) {
        KNpc* e = c.world->mutable_entity(entity_arg(L, 1));
        const auto n = static_cast<std::int64_t>(lua_tonumber(L, 2));
        if (e != nullptr && n >= 1 && n <= 10) {
            e->script_param[static_cast<std::size_t>(n - 1)] = static_cast<int>(static_cast<std::int64_t>(lua_tonumber(L, 3)));
        }
    }
    return 0;
}

// SetNpcScript(npc, script[, param]) (0x08101500): two arguments, a live npc and a string -> the script's game path into
// Npc+0x1538 (0x4f bytes, 0x08101594) and its id into +0x1588 (0x0821DE70); a third number goes to +0x158c (0x08101609),
// what DialogNpc hands to main().  Nothing returned.
int l_SetNpcScript(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) > 1 && c.world != nullptr) {
        KNpc* e = c.world->mutable_entity(entity_arg(L, 1));
        const char* path = lua_tostring(L, 2);
        if (e != nullptr && path != nullptr) {
            e->script = std::string(path).substr(0, 0x4f);
            if (lua_gettop(L) > 2 && lua_type(L, 3) == LUA_TNUMBER) e->script_main_param = static_cast<int>(static_cast<std::int64_t>(lua_tonumber(L, 3)));
            log::debug("lua", "npc script set", {log::kv("npc", e->id), log::kv("script", e->script), log::kv("param", e->script_main_param)});
        }
    }
    return 0;
}

// DelNpc(npc | name) (0x08107460): one argument; a number is the npc's index, a string the name of a npc on the script's map
// (0x080EFEE0: the first non-player with that name); a player (kind 1), a partner (npc kind 2) or a kind 6 npc is left
// alone (0x08107504..), so is one outside a region (+0x1184 < 0); the rest goes at the map's next frame (the 0x3e9 node,
// 0x08107536).  Nothing returned.
int l_DelNpc(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) <= 0 || c.world == nullptr) return 0;
    const KNpc* e = nullptr;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        e = c.world->find_entity(entity_arg(L, 1));
    } else {
        const char* name = lua_tostring(L, 1);
        if (name != nullptr && *name != '\0') e = c.world->find_npc_by_name(name);
    }
    if (e == nullptr || e->kind == KNpcKind::player || e->kind == KNpcKind::drop || e->npc_kind == 2 || e->npc_kind == 6) return 0;
    c.world->queue_remove_npc(e->id);
    return 0;
}

// AddNpc(template | name, level, subworld, x, y[, remove_on_death[, name[, boss[, p9]]]]) (0x0811BB10): fewer than five
// arguments -> nothing.  The template by number, or by its name in npcs.txt (KTabFile::FindRow - 2, 0x0811BB98); the level a
// word, below 0 -> 1 (0x0811BBD5); the series rand() % 5 (0x0811BC89); a ninth number is KNpcSet::Add's last argument (-1
// without); KNpcSet::Add 0x0809FB10(series, template << 16 | level, x, y, subworld, p9) at the absolute spot (world units:
// DlgNpcManager:AddNpc(.., 49344, 102720)).  Five arguments -> the index.  More: +0x18e4 = 0 (a kind 3 npc also enters the
// name table 0x830c7a0 - not kept here), the sixth sets +0x1824 (remove_on_death), a non-empty seventh is the name
// (KNpc::SetName 0x081395C0, 0x20 bytes), an eighth of 1 = 0x08085250 (init_template_skills) and +0x181c = 3 when the sixth
// was set else 2, of 2 = a gold npc (BackData 0x0809D560 + SetGoldTypeAndBackData 0x0809D8D0 rate 1 000 000).  Returns the
// index (the entity id here); a failed Add gives what Add gave - 0 here, also for a subworld this zone does not run.
int l_AddNpc(lua_State* L)
{
    const int top = lua_gettop(L);
    if (top <= 4) return 0;
    KSubWorld* w = g_ScriptContext().world;
    if (w == nullptr) return 0;
    const KNpcTemplateSet* templates = w->config().templates.get();
    const KNpcTemplate* tpl = nullptr;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        if (templates != nullptr) tpl = templates->find(static_cast<std::uint32_t>(static_cast<std::int64_t>(lua_tonumber(L, 1))));
    } else if (lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        if (templates != nullptr && name != nullptr) tpl = templates->find_by_name(name);
    } else {
        return 0;   // 0x0811BB73: neither a number nor a string
    }
    const auto level_arg = static_cast<std::int64_t>(lua_tonumber(L, 2));
    const int level = level_arg < 0 ? 1 : static_cast<int>(static_cast<std::uint16_t>(level_arg));
    const auto map = static_cast<std::int64_t>(lua_tonumber(L, 3));
    const Pos at{static_cast<std::int32_t>(lua_tonumber(L, 4)), static_cast<std::int32_t>(lua_tonumber(L, 5))};
    if (tpl == nullptr || map != static_cast<std::int64_t>(w->map_id())) {
        log::warn("lua", "AddNpc failed", {log::kv("template", tpl != nullptr ? static_cast<int>(tpl->id) : -1), log::kv("map", map),
                                           log::kv("this_map", w->map_id())});
        lua_pushinteger(L, 0);
        return 1;
    }
    const int series = std::rand() % 5;
    const bool remove_on_death = top >= 6 && static_cast<std::int64_t>(lua_tonumber(L, 6)) != 0;
    const KNpcKind kind = tpl->kind == 0 ? KNpcKind::monster : KNpcKind::npc;
    KScriptContext& ctx = g_ScriptContext();
    const EntityId self = ctx.player != nullptr ? ctx.player->id : EntityId{};
    const EntityId id = w->spawn_npc(tpl->name, w->to_local(at), tpl->id, 0, kind, static_cast<std::uint32_t>(level), series, 0);
    // the entity table may have grown (its entities sit in one vector): the script's player is found again so the
    // functions after this one do not write through a moved pointer
    if (self.value != 0) ctx.player = w->mutable_entity(self);
    KNpc* e = w->mutable_entity(id);
    if (e == nullptr) {
        lua_pushinteger(L, 0);
        return 1;
    }
    if (top >= 6) {
        e->remove_on_death = remove_on_death;
        if (top >= 7) {
            const char* name = lua_tostring(L, 7);
            if (name != nullptr && *name != '\0') e->name = std::string(name).substr(0, 0x20);
        }
        if (top >= 8) {
            const auto boss = static_cast<std::int64_t>(lua_tonumber(L, 8));
            if (boss == 1) {
                w->init_template_skills(*e);
                e->boss_flag = remove_on_death ? 3 : 2;
            } else if (boss == 2) {
                w->make_gold_npc(*e);
            }
        }
    }
    log::info("lua", "npc added", {log::kv("npc", e->id), log::kv("template", tpl->id), log::kv("level", level), log::kv("series", series),
                                   log::kv("name", e->name)});
    lua_pushinteger(L, static_cast<lua_Integer>(id.value));
    return 1;
}

// AddSkillState(skill, level, mode, time[, extra]) (0x08126240): at least four arguments and a player; 0x08125D70(npc, skill,
// level, mode, time, extra): time < 0 (0x08125DA3) or mode > 2 (0x08125DB9) -> -1; the skill at that level (g_SkillManager
// 0x08076CE0) must be of style 2 or 3 - a state skill - else -1; mode 0 -> KNpc::SetStateSkillEffect 0x08086260(self as the
// launcher, skill, level, its state attributes +0x540, their count +0x680, time, 0, 0, 0, 0, extra); mode 1 -> a copy of the
// attributes with every value[1] set to `time` (0x08125EF5: their own duration) then the same; mode 2 -> `time` is a date
// (YYYYMMDDhh.., 0x08125EFE..) turned into seconds from now - not built here (-1, logged).  Returns what SetStateSkillEffect
// returned.
int l_AddSkillState(lua_State* L)
{
    lua_Integer result = -1;
    if (lua_gettop(L) > 3) {
        if (KNpc* p = player_of(L, "AddSkillState")) {
            KSubWorld* w = g_ScriptContext().world;
            const auto skill_id = static_cast<int>(lua_tonumber(L, 1));
            const auto level = static_cast<int>(lua_tonumber(L, 2));
            const auto mode = static_cast<std::int64_t>(lua_tonumber(L, 3));
            const auto time = static_cast<std::int64_t>(lua_tonumber(L, 4));
            const auto extra = static_cast<int>(luaL_optnumber(L, 5, 0));
            const KSkill* skill = w->skills() != nullptr && mode >= 0 && mode <= 2 && time >= 0 ? w->skills()->get(skill_id, level) : nullptr;
            if (skill != nullptr && (skill->row.style == skill_style_initiative_npc_state || skill->row.style == skill_style_passivity_npc_state)) {
                if (mode == 2) {
                    log::warn("lua", "AddSkillState: date mode not built", {log::kv("skill", skill_id), log::kv("time", time)});
                } else if (mode == 1) {
                    std::array<KMagicAttrib, kSkillAttribs> states = skill->state_attribs;
                    for (int i = 0; i < skill->state_attrib_count && i < static_cast<int>(states.size()); ++i) {
                        states[static_cast<std::size_t>(i)].value[1] = static_cast<int>(time);
                    }
                    result = w->set_state_skill_effect(*p, p->id, skill_id, level, states.data(), skill->state_attrib_count, static_cast<int>(time), 0, false, 0,
                                                       false, extra);
                } else {
                    result = w->set_state_skill_effect(*p, p->id, skill_id, level, skill->state_attribs.data(), skill->state_attrib_count, static_cast<int>(time),
                                                       0, false, 0, false, extra);
                }
            }
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

// ---- S3 (docs/SCRIPT-API.md, docs/LINUX-SERVER.md §32): the player / world getters, global values, mission strings,
// DynamicExecute, the system lines, timers and flags the scripts call next

// GetSeries() (0x08111F30): the series (Npc+0x28) of the player's npc; nothing without a player
int l_GetSeries(lua_State* L)
{
    const KNpc* p = player_of(L, "GetSeries");
    if (p == nullptr) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(p->series));
    return 1;
}

// GetGameTime() (0x0810F3A0): Player+0x8c - the frames the character has been online - times 20 / 18 (0x0810F3C4..: an
// 18 Hz count read in 20 Hz units); 0 without a player
int l_GetGameTime(lua_State* L)
{
    lua_Integer t = 0;
    if (const KNpc* p = player_of(L, "GetGameTime")) {
        const std::uint64_t now = g_ScriptContext().world->tick_count();
        const std::uint64_t frames = now >= p->player.login_tick ? now - p->player.login_tick : 0;
        t = static_cast<lua_Integer>(frames * 20 / 18);
    }
    lua_pushinteger(L, t);
    return 1;
}

// ST_GetTransLifeCount() (0x081C1100): the reborn count (Player+0x86b8, a byte); nothing without a player (1..0x4af)
int l_ST_GetTransLifeCount(lua_State* L)
{
    const KNpc* p = player_of(L, "ST_GetTransLifeCount");
    if (p == nullptr) return 0;
    lua_pushinteger(L, p->player.reborn);
    return 1;
}

// SetLogoutRV(flag) (0x08110500): a player -> Player+0x40 = (flag != 0); nothing returned
int l_SetLogoutRV(lua_State* L)
{
    if (KNpc* p = player_of(L, "SetLogoutRV")) p->player.logout_revive = static_cast<std::int64_t>(lua_tonumber(L, 1)) != 0;
    return 0;
}

// the 5001 global numbers of the process (0x8bb37dc.., GetGlbValue 0x080FE270 / SetGlbValue 0x080FE2F0): in memory only,
// one array for every map of the zone
constexpr int kGlbValues = 5001;
std::array<std::atomic<int>, kGlbValues>& glb_values()
{
    static std::array<std::atomic<int>, kGlbValues> values{};
    return values;
}

// GetGlbValue(idx) (0x080FE270): an argument and idx <= 5000 -> the number (a negative idx read before the array in the
// binary; 0 here), else 0
int l_GetGlbValue(lua_State* L)
{
    lua_Integer v = 0;
    if (lua_gettop(L) > 0) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        if (idx >= 0 && idx < kGlbValues) v = glb_values()[static_cast<std::size_t>(idx)].load(std::memory_order_relaxed);
    }
    lua_pushinteger(L, v);
    return 1;
}

// SetGlbValue(idx, value) (0x080FE2F0): two arguments and idx <= 5000 -> stored; nothing returned
int l_SetGlbValue(lua_State* L)
{
    if (lua_gettop(L) > 1) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        if (idx >= 0 && idx < kGlbValues) {
            glb_values()[static_cast<std::size_t>(idx)].store(static_cast<int>(static_cast<std::int64_t>(lua_tonumber(L, 2))), std::memory_order_relaxed);
        }
    }
    return 0;
}

// GetMissionS(idx) (0x08107160): the script's map and idx 1..100 -> the map's mission string (SubWorld+0x48648 + (idx - 1) * 0xc8);
// "" otherwise (0x081071CB)
int l_GetMissionS(lua_State* L)
{
    const KSubWorld* w = g_ScriptContext().world;
    const char* s = "";
    if (lua_gettop(L) > 0 && w != nullptr) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        if (idx >= 1 && idx <= KSubWorld::kMissionStrings) s = w->mission_string(static_cast<int>(idx)).c_str();
    }
    lua_pushstring(L, s);
    return 1;
}

// SetMissionS(idx, text) (0x08107220): the script's map, two arguments, idx 1..100 and a string -> the mission string, cut to
// 0xc7 bytes ("" clears it, 0x081072B4); nothing returned
int l_SetMissionS(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    if (lua_gettop(L) > 1 && w != nullptr) {
        const auto idx = static_cast<std::int64_t>(lua_tonumber(L, 1));
        const char* s = lua_tostring(L, 2);
        if (idx >= 1 && idx <= KSubWorld::kMissionStrings && s != nullptr) w->set_mission_string(static_cast<int>(idx), s);
    }
    return 0;
}

// a number back to the script: a whole one as a Lua integer (like every other function here: tostring says "8", not "8.0")
void push_lua_number(lua_State* L, double v)
{
    if (v == std::floor(v) && std::fabs(v) < 9007199254740992.0) {
        lua_pushinteger(L, static_cast<lua_Integer>(v));
    } else {
        lua_pushnumber(L, v);
    }
}

// a script by its game path, or the running one for "" (DynamicExecute 0x081301E0 / 0x0813011F: the script of this state)
KLuaScript* script_of(KSubWorld* w, const char* path, std::string& game_path)
{
    if (w == nullptr || !w->config().scripts) return nullptr;
    if (path == nullptr || *path == '\0') {
        game_path = g_ScriptContext().script_path;
        return game_path.empty() ? nullptr : w->config().scripts->get(game_path);
    }
    game_path = path;
    return w->config().scripts->get(game_path);
}

// the arguments from `first` on, as a call into another state takes them (KLuaScript::Arg: numbers and strings; anything
// else goes over as 0)
std::vector<KLuaScript::Arg> args_from(lua_State* L, int first)
{
    std::vector<KLuaScript::Arg> out;
    const int top = lua_gettop(L);
    for (int i = first; i <= top; ++i) {
        if (lua_type(L, i) == LUA_TSTRING) {
            out.emplace_back(std::string(lua_tostring(L, i)));
        } else {
            out.emplace_back(static_cast<double>(lua_tonumber(L, i)));
        }
    }
    return out;
}

// the call of DynamicExecute / DynamicExecuteByPlayer: fn(args) of `script` with `player` as its player (SetPlayerIndex /
// the PlayerIndex global of the old states), the context as execute_script sets it.  One number comes back - the old call
// returned every value of the function; a string or a table does not cross the states here.
std::optional<double> dynamic_call(KSubWorld& w, KLuaScript& script, const std::string& game_path, const char* fn, KNpc* player,
                                   const std::vector<KLuaScript::Arg>& args)
{
    KScriptContext& ctx = g_ScriptContext();
    const KScriptContext saved = ctx;
    ctx.world = &w;
    ctx.player = player;
    ctx.sid = player != nullptr ? player->sid : 0;
    ctx.script_path = game_path;
    const std::optional<double> r = script.call_number(fn, args);
    ctx = saved;
    return r;
}

// DynamicExecute(script, fn, ...) (0x081300B0): two arguments at least, both strings and `fn` not empty; `script` names a
// script by its game path (0x08222A90 -> the cache 0x830b260), "" the running one (0x08220780); fn(...) runs there with the
// rest of the arguments (0x08221ED0) and what it returns comes back.  Nothing when the script or the function is missing.
int l_DynamicExecute(lua_State* L)
{
    if (lua_gettop(L) <= 1 || !lua_isstring(L, 1) || !lua_isstring(L, 2)) return 0;
    const char* fn = lua_tostring(L, 2);
    if (fn == nullptr || *fn == '\0') return 0;
    KSubWorld* w = g_ScriptContext().world;
    std::string game_path;
    KLuaScript* script = script_of(w, lua_tostring(L, 1), game_path);
    if (script == nullptr) {
        log::warn("lua", "dynamic execute: no script", {log::kv("script", lua_tostring(L, 1)), log::kv("function", fn)});
        return 0;
    }
    const std::optional<double> r = dynamic_call(*w, *script, game_path, fn, g_ScriptContext().player, args_from(L, 3));
    if (!r) return 0;
    push_lua_number(L, *r);
    return 1;
}

// DynamicExecuteByPlayer(player, script, fn, ...) (0x0812FE80): three arguments at least; `player` 1..0x4af (an entity id of a
// player here), `script` / `fn` as DynamicExecute; the script's PlayerIndex becomes `player` for the call (0x0812FFD7) and
// the old one comes back after.  Returns what the function returned.
int l_DynamicExecuteByPlayer(lua_State* L)
{
    if (lua_gettop(L) <= 2) return 0;
    KSubWorld* w = g_ScriptContext().world;
    if (w == nullptr) return 0;
    const auto id = static_cast<std::int64_t>(lua_tonumber(L, 1));
    KNpc* target = id > 0 ? w->mutable_entity(EntityId{static_cast<std::uint64_t>(id)}) : nullptr;
    const char* fn = lua_isstring(L, 3) ? lua_tostring(L, 3) : nullptr;
    if (target == nullptr || target->kind != KNpcKind::player || !lua_isstring(L, 2) || fn == nullptr || *fn == '\0') return 0;
    std::string game_path;
    KLuaScript* script = script_of(w, lua_tostring(L, 2), game_path);
    if (script == nullptr) {
        log::warn("lua", "dynamic execute: no script", {log::kv("script", lua_tostring(L, 2)), log::kv("function", fn)});
        return 0;
    }
    const std::optional<double> r = dynamic_call(*w, *script, game_path, fn, target, args_from(L, 4));
    if (!r) return 0;
    push_lua_number(L, *r);
    return 1;
}

// Msg2SubWorld(text) (0x08105170): one string -> 0x081C9220(kind 0, 0, [0x978a650], text, len): the line to EVERY player of
// the process as a system message (0x080C3EE0 / 0x080C3F10 walk the player set); this map's players here - a zone with
// several maps reaches only the script's map.  Nothing returned.
int l_Msg2SubWorld(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    if (lua_gettop(L) > 0 && w != nullptr) {
        const char* text = lua_tostring(L, 1);
        if (text != nullptr) w->msg_to_all(text);
    }
    return 0;
}

// Msg2Map(map, text) (0x08105080): two arguments; KSubWorldSet::GetSubWorldIdx(map) = -1 -> nothing; every player of that map
// (0x080EF5C0 / 0x080EF640) gets the line (kind 1 of 0x081C9220).  This zone reaches the script's map only.  Nothing returned.
int l_Msg2Map(lua_State* L)
{
    KSubWorld* w = g_ScriptContext().world;
    if (lua_gettop(L) > 1 && w != nullptr) {
        const auto map = static_cast<std::int64_t>(lua_tonumber(L, 1));
        const char* text = lua_tostring(L, 2);
        if (text == nullptr) return 0;
        if (map == static_cast<std::int64_t>(w->map_id())) {
            w->msg_to_all(text);
        } else {
            log::debug("lua", "Msg2Map: other map", {log::kv("map", map), log::kv("this_map", w->map_id())});
        }
    }
    return 0;
}

// IsMyItem(item) (0x0811B4D0): an argument, a live item index -> KItemList 0x081FA550(Player+0x3fc, item): 1 when the piece
// is in the player's list, else 0
int l_IsMyItem(lua_State* L)
{
    lua_Integer mine = 0;
    if (lua_gettop(L) > 0) {
        if (const KNpc* p = player_of(L, "IsMyItem")) {
            const auto id = static_cast<std::int64_t>(lua_tonumber(L, 1));
            const KItemList* items = g_ScriptContext().world->items_of(p->sid);
            if (id > 0 && items != nullptr && items->find(static_cast<std::uint32_t>(id)) != nullptr) mine = 1;
        }
    }
    lua_pushinteger(L, mine);
    return 1;
}

// GetNpcId(npc) (0x080FD920): exactly one argument, a live npc index -> Npc+0 (its running id, pushed as a 64-bit number);
// nothing otherwise.  The entity id is that here.
int l_GetNpcId(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) != 1 || c.world == nullptr) return 0;
    const KNpc* e = c.world->find_entity(entity_arg(L, 1));
    if (e == nullptr) return 0;
    lua_pushinteger(L, static_cast<lua_Integer>(e->id.value));
    return 1;
}

// the room argument of CalcItemCount / ConsumeItem (the rooms of the old KItemList: 3 the bag, 0 the pieces worn, 1 the quick
// slots; -1 every room, what the scripts pass) -> this zone's room, -2 for one it has not
int room_arg(std::int64_t room)
{
    switch (room) {
    case -1: return -1;
    case 3: return room_equipment;
    case 0: return room_body;
    case 1: return room_immediacy;
    default: return -2;
    }
}

// CalcItemCount(genre, detail, particular, level, room) (0x0810D840): exactly five arguments and a player, else 0; KItemList
// 0x081FA770 with the room as given (-1 = every room) counting stacks
int l_CalcItemCount(lua_State* L)
{
    lua_Integer n = 0;
    if (lua_gettop(L) == 5) {
        if (const KNpc* p = player_of(L, "CalcItemCount")) {
            const int room = room_arg(static_cast<std::int64_t>(lua_tonumber(L, 5)));
            const KItemList* items = g_ScriptContext().world->items_of(p->sid);
            if (items != nullptr && room != -2) {
                for (const KBagMatch& m : bag_matches(*items, static_cast<int>(lua_tonumber(L, 1)), static_cast<int>(lua_tonumber(L, 2)),
                                                       static_cast<int>(lua_tonumber(L, 3)), static_cast<int>(lua_tonumber(L, 4)), room)) {
                    n += m.units;
                }
            }
        }
    }
    lua_pushinteger(L, n);
    return 1;
}

// ConsumeItem(genre, detail, particular, level, count, room) (0x0810D6C0): exactly six arguments and a player, else -1;
// KItemList 0x08202410 with the room as given (-1 = every room): 1 when everything was taken, -1 when the pieces ran out
int l_ConsumeItem(lua_State* L)
{
    lua_Integer result = -1;
    if (lua_gettop(L) == 6) {
        if (const KNpc* p = player_of(L, "ConsumeItem")) {
            KSubWorld* w = g_ScriptContext().world;
            KItemList* items = w->items_of(p->sid);
            const int room = room_arg(static_cast<std::int64_t>(lua_tonumber(L, 6)));
            if (items != nullptr && room != -2) {
                result = consume_matches(*w, p->sid, *items, static_cast<int>(lua_tonumber(L, 1)), static_cast<int>(lua_tonumber(L, 2)),
                                         static_cast<int>(lua_tonumber(L, 3)), static_cast<int>(lua_tonumber(L, 4)), room,
                                         static_cast<int>(lua_tonumber(L, 5)));
            }
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

// SetPunish(flag) (0x0810F470): an argument and a player -> Npc+0x1818 (m_nCurPKPunishState) = 0 when flag != 0 (the death
// penalties apply, 0x0810F4F8), 3 when 0 (a PK-battle death: none, 0x0810F52B); nothing returned
int l_SetPunish(lua_State* L)
{
    if (lua_gettop(L) > 0) {
        if (KNpc* p = player_of(L, "SetPunish")) p->pk_punish_state = static_cast<std::int64_t>(lua_tonumber(L, 1)) != 0 ? 0 : 3;
    }
    return 0;
}

// DisabledUseTownP(flag) (0x08130A80): an argument and a player -> bit 0x100000 of task value 0x87 (135) set (flag != 0) or
// cleared (KPlayerTask::GetSaveVal 0x080CB540, SetTask 0x080A9190 with the sync) and 1 comes back (0x08130B0B); 0 without
// (0x08130B68).  The town portal itself does not read the bit yet (docs §32).
int l_DisabledUseTownP(lua_State* L)
{
    lua_Integer done = 0;
    if (lua_gettop(L) > 0) {
        if (KNpc* p = player_of(L, "DisabledUseTownP")) {
            const auto v = static_cast<std::uint32_t>(p->player.task.get_save_val(0x87));
            const bool on = static_cast<std::int64_t>(lua_tonumber(L, 1)) != 0;
            g_ScriptContext().world->task_set_value(*p, 0x87, static_cast<int>(on ? (v | 0x100000u) : (v & ~0x100000u)), true);
            done = 1;
        }
    }
    lua_pushinteger(L, done);
    return 1;
}

// SetDeathScript(script) (0x08110700): a player; a non-empty string -> Player+0x5f98 = the script (0x08110768), "" or nothing
// -> 0.  OnDeath(the last attacker) of that script runs at the end of the death frames (0x080839A8).  Nothing returned.
int l_SetDeathScript(lua_State* L)
{
    if (KNpc* p = player_of(L, "SetDeathScript")) {
        const char* s = lua_gettop(L) > 0 ? lua_tostring(L, 1) : nullptr;
        p->player.death_script = s != nullptr ? s : "";
    }
    return 0;
}

// SetRank(rank) (0x081109E0): a player -> Npc+0x30 = the low byte of the number, then 0x08079500: the 0xa5 packet {rank, npc id}
// to the player's own client (KPlayer::Send) - not sent yet.  Nothing returned.
int l_SetRank(lua_State* L)
{
    if (KNpc* p = player_of(L, "SetRank")) {
        p->rank = static_cast<int>(static_cast<std::uint8_t>(static_cast<std::int64_t>(lua_tonumber(L, 1))));
        log::debug("lua", "rank set", {log::kv("entity", p->id), log::kv("rank", p->rank)});
    }
    return 0;
}

// SetNpcTimer(npc, frames) (0x080FC180): two arguments and a live npc, else nothing (0x080FC24A); frames > 0 -> Npc+0x19a8 =
// the map's frame counter [0x9777f00] + frames, frames <= 0 -> cleared (0x080FC260); 1 comes back either way (0x080FC21E).
// KNpc::Activate 0x0808BF81 runs the npc script's OnTimer(npc) once the frame is reached.
int l_SetNpcTimer(lua_State* L)
{
    KScriptContext& c = g_ScriptContext();
    if (lua_gettop(L) <= 1 || c.world == nullptr) return 0;
    KNpc* e = c.world->mutable_entity(entity_arg(L, 1));
    if (e == nullptr) return 0;
    const auto frames = static_cast<std::int64_t>(lua_tonumber(L, 2));
    e->timer_frame = frames > 0 ? c.world->tick_count() + static_cast<std::uint64_t>(frames) : 0;
    lua_pushinteger(L, 1);
    return 1;
}

const luaL_Reg kGameScriptFuns[] = {
    {"GetFightState", l_GetFightState}, {"SetFightState", l_SetFightState}, {"SetPos", l_SetPos},
    {"NewWorld", l_NewWorld},           {"GetPos", l_GetPos},               {"GetWorldPos", l_GetWorldPos},
    {"Msg2Player", l_Msg2Player},       {"AddStation", l_AddStation},       {"AddTermini", l_AddTermini},
    {"Say", l_Say},                     {"Talk", l_Talk},                   {"AddItem", l_AddItem},
    {"AddGoldItem", l_AddGoldItem},     {"AddStackItem", l_AddStackItem},   {"HaveItem", l_HaveItem},
    {"GetItemCount", l_GetItemCount},   {"GetItemCountEx", l_GetItemCountEx}, {"DelItem", l_DelItem},
    {"DelItemEx", l_DelItemEx},         {"HaveCommonItem", l_HaveCommonItem}, {"DelCommonItem", l_DelCommonItem},
    {"GetTotalItemCount", l_GetTotalItemCount},
    {"Earn", l_Earn},                   {"Pay", l_Pay},                     {"GetCash", l_GetCash},
    {"SetSkillLevel", l_SetSkillLevel},   {"HaveMagic", l_HaveMagic},           {"DelMagic", l_DelMagic},
    {"GetCurrentMagicLevel", l_GetCurrentMagicLevel}, {"GetSkillMaxLevel", l_GetSkillMaxLevel}, {"GetSkillExp", l_GetSkillExp},
    {"GetSkillNextExp", l_GetSkillNextExp}, {"AddSkillExp", l_AddSkillExp},     {"RollbackSkill", l_RollbackSkill},
    {"ForbitSkill", l_ForbitSkill},       {"SetAForbitSkill", l_SetAForbitSkill}, {"SetSkillMaxLevelAddons", l_SetSkillMaxLevelAddons},
    {"ForbitAura", l_ForbitAura},         {"ForbitSyncAura", l_ForbitSyncAura},   {"ForbitStamina", l_ForbitStamina},
    {"ForbitTalk", l_ForbitTalk},         {"SetChatFlag", l_SetChatFlag},
    {"RestoreLife", l_RestoreLife},       {"RestoreMana", l_RestoreMana},         {"GetLife", l_GetLife},
    {"GetMana", l_GetMana},
    {"GetPK", l_GetPK},                   {"SetPK", l_SetPK},                     {"SetPKFlag", l_SetPKFlag},
    {"ForbidChangePK", l_ForbidChangePK}, {"IsForbidChangePK", l_IsForbidChangePK},
    {"SetPkReduceState", l_SetPkReduceState}, {"GetPkReduceState", l_GetPkReduceState}, {"SetDeathPunish_PK10", l_SetDeathPunish_PK10},
    {"IsCaptain", l_IsCaptain},           {"GetTeam", l_GetTeam},                 {"GetTeamSize", l_GetTeamSize},
    {"GetTeamMember", l_GetTeamMember},   {"LeaveTeam", l_LeaveTeam},             {"SetCreateTeam", l_SetCreateTeam},
    {"DisabledTeam", l_DisabledTeam},     {"IsDisabledTeam", l_IsDisabledTeam},   {"ChangeTeamFeature", l_ChangeTeamFeature},
    {"Msg2Team", l_Msg2Team},
    {"GetSkillMaxLevelAddons", l_GetSkillMaxLevelAddons}, {"GetSkillCount", l_GetSkillCount}, {"GetTotalSkill", l_GetTotalSkill},
    {"IsExpSkill", l_IsExpSkill},         {"UpdateSkill", l_UpdateSkill},       {"SetHide", l_SetHide},
    {"AbradeEquipments", l_AbradeEquipments}, {"SetTempRevPos", l_SetTempRevPos}, {"SetRevPos", l_SetRevPos},
    {"KillPlayer", l_KillPlayer},         {"GetRideState", l_GetRideState},   {"AddExp", l_AddExp},
    {"SetFaction", l_SetFaction},         {"GetFaction", l_GetFaction},       {"GetFactionNumber", l_GetFactionNumber},
    {"GetLastAddFaction", l_GetLastAddFaction}, {"GetLastFactionNumber", l_GetLastFactionNumber}, {"SetLastFactionNumber", l_SetLastFactionNumber},
    {"ClearFactionRecord", l_ClearFactionRecord}, {"SetCamp", l_SetCamp},        {"SetCurCamp", l_SetCurCamp},
    {"GetCamp", l_GetCamp},               {"GetCurCamp", l_GetCurCamp},       {"AddMagic", l_AddMagic},
    {"GetTask", l_GetTask},               {"SetTask", l_SetTask},             {"GetTaskTemp", l_GetTaskTemp},
    {"SetTaskTemp", l_SetTaskTemp},       {"SyncTaskValue", l_SyncTaskValue}, {"SyncTaskValueMore", l_SyncTaskValueMore},
    {"GetBitTask", l_GetBitTask},         {"SetBitTask", l_SetBitTask},
    {"TaskName", l_TaskName},             {"TaskNo", l_TaskNo},               {"GetTaskStatus", l_GetTaskStatus},
    {"SetTaskStatus", l_SetTaskStatus},   {"StartTask", l_StartTask},         {"CloseTask", l_CloseTask},
    {"GetTmpValue", l_GetTmpValue},       {"SetTmpValue", l_SetTmpValue},     {"FirstTask", l_FirstTask},
    {"NextTask", l_NextTask},             {"TaskCondition", l_TaskCondition}, {"TaskConditionMatrix", l_TaskConditionMatrix},
    {"TaskEntity", l_TaskEntity},         {"TaskEntityMatrix", l_TaskEntityMatrix}, {"TaskAward", l_TaskAward},
    {"TaskAwardMatrix", l_TaskAwardMatrix}, {"TaskTalk", l_TaskTalk},         {"TaskTalkMatrix", l_TaskTalkMatrix},
    {"TaskId", l_TaskId},                 {"TaskIdMatrix", l_TaskIdMatrix},   {"TaskEvent", l_TaskEvent},
    {"TaskEventMatrix", l_TaskEventMatrix}, {"GetTaskEventID", l_GetTaskEventID}, {"GetEventTaskCount", l_GetEventTaskCount},
    {"GetEventTask", l_GetEventTask},     {"SubWorldName", l_SubWorldName},   {"SelectTaskStart", l_SelectTaskStart},
    {"SelectTaskFinish", l_SelectTaskFinish}, {"SelectTaskAward", l_SelectTaskAward},
    {"AddPlayerEvent", l_AddPlayerEvent}, {"RemovePlayerEvent", l_RemovePlayerEvent}, {"RemoveAllPlayerEvent", l_RemoveAllPlayerEvent},
    {"GetNpcName", l_GetNpcName},         {"GetNpcPos", l_GetNpcPos},         {"NpcName2Replace", l_NpcName2Replace},
    {"NpcDialog", l_NpcDialog},           {"GetLastDiagNpc", l_GetLastDiagNpc}, {"GetNpcSettingIdx", l_GetNpcSettingIdx},
    {"GetLevel", l_GetLevel},             {"GetName", l_GetName},             {"GetSex", l_GetSex},
    {"TabFile_Load", l_TabFile_Load},     {"TabFile_UnLoad", l_TabFile_UnLoad}, {"TabFile_GetRowCount", l_TabFile_GetRowCount},
    {"TabFile_GetColCount", l_TabFile_GetColCount}, {"TabFile_GetCell", l_TabFile_GetCell}, {"TabFile_Search", l_TabFile_Search},
    {"TabFile_SetCell", l_TabFile_SetCell}, {"TabFile_Save", l_TabFile_Save},
    {"Describe", l_Describe},             {"TaskTip", l_TaskTip},             {"WriteLog", l_WriteLog},
    {"GetAccount", l_GetAccount},         {"AddOwnExp", l_AddOwnExp},         {"AddRepute", l_AddRepute},
    {"GetRepute", l_GetRepute},           {"PushString", l_PushString},       {"AppendString", l_AppendString},
    {"ReplaceString", l_ReplaceString},   {"PopString", l_PopString},
    {"AddItemEx", l_AddItemEx},           {"GetItemProp", l_GetItemProp},     {"SyncItem", l_SyncItem},
    {"RemoveItemByIndex", l_RemoveItemByIndex}, {"GetItemStackCount", l_GetItemStackCount}, {"GetGlodEqIndex", l_GetGlodEqIndex},
    {"SetItemMagicLevel", l_SetItemMagicLevel}, {"ITEM_GetItemRandSeed", l_ITEM_GetItemRandSeed},
    {"GiveItemUI", l_GiveItemUI},         {"GetGiveItemUnit", l_GetGiveItemUnit}, {"GetGiveItemUnitWithPos", l_GetGiveItemUnitWithPos},
    {"SetUiGiveItemMsg", l_SetUiGiveItemMsg}, {"SetUiGiveItemMoreConfirmMsg", l_SetUiGiveItemMoreConfirmMsg},
    {"AddNote", l_AddNote},               {"AskClientForNumber", l_AskClientForNumber}, {"AskClientForString", l_AskClientForString},
    {"GetLocalDate", l_GetLocalDate},     {"GetCurServerTime", l_GetCurServerTime}, {"SubWorldID2Idx", l_SubWorldID2Idx},
    {"SubWorldIdx2ID", l_SubWorldIdx2ID}, {"GetBit", l_GetBit},               {"SetBit", l_SetBit},
    {"GetByte", l_GetByte},               {"SetByte", l_SetByte},             {"GetMissionV", l_GetMissionV},
    {"SetMissionV", l_SetMissionV},       {"GetItemName", l_GetItemName},     {"GetItemParam", l_GetItemParam},
    {"GetExp", l_GetExp},                 {"GetExtPoint", l_GetExtPoint},     {"AddExtPoint", l_AddExtPoint},
    {"AddExtPointForGS", l_AddExtPointForGS}, {"PayExtPoint", l_PayExtPoint},  {"CalcFreeItemCellCount", l_CalcFreeItemCellCount},
    {"SearchPlayer", l_SearchPlayer},     {"CallPlayerFunction", l_CallPlayerFunction},
    {"AddEventItem", l_AddEventItem},     {"AddQualityItem", l_AddQualityItem}, {"CalcEquiproomItemCount", l_CalcEquiproomItemCount},
    {"ConsumeEquiproomItem", l_ConsumeEquiproomItem}, {"GetNpcParam", l_GetNpcParam}, {"SetNpcParam", l_SetNpcParam},
    {"SetNpcScript", l_SetNpcScript},     {"DelNpc", l_DelNpc},               {"AddNpc", l_AddNpc},
    {"AddSkillState", l_AddSkillState},
    {"GetSeries", l_GetSeries},           {"GetGameTime", l_GetGameTime},     {"ST_GetTransLifeCount", l_ST_GetTransLifeCount},
    {"SetLogoutRV", l_SetLogoutRV},       {"GetGlbValue", l_GetGlbValue},     {"SetGlbValue", l_SetGlbValue},
    {"GetMissionS", l_GetMissionS},       {"SetMissionS", l_SetMissionS},     {"DynamicExecute", l_DynamicExecute},
    {"DynamicExecuteByPlayer", l_DynamicExecuteByPlayer}, {"Msg2SubWorld", l_Msg2SubWorld}, {"Msg2Map", l_Msg2Map},
    {"IsMyItem", l_IsMyItem},             {"GetNpcId", l_GetNpcId},           {"CalcItemCount", l_CalcItemCount},
    {"ConsumeItem", l_ConsumeItem},       {"SetPunish", l_SetPunish},         {"DisabledUseTownP", l_DisabledUseTownP},
    {"SetDeathScript", l_SetDeathScript}, {"SetRank", l_SetRank},             {"SetNpcTimer", l_SetNpcTimer},
    {nullptr, nullptr},
};

} // namespace

void RegisterGameScriptFuns(lua_State* L)
{
    for (const luaL_Reg* f = kGameScriptFuns; f->name != nullptr; ++f) {
        lua_pushcfunction(L, f->func);
        lua_setglobal(L, f->name);
    }
}

} // namespace jx::zone
