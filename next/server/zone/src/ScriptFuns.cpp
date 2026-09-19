// The script api of the old server (Core/Src/ScriptFuns.cpp) for the traps: positions are in
// cells of 32 units like the old Lua* functions (SetPos(x, y) -> SetPos(x * 32, y * 32)).
#include "jx/zone/ScriptFuns.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <array>
#include <climits>
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

// AddItem(genre, detail, particular, level, series, luck [, magic1 [, magic2 .. magic6]]) -> 1 / 0
//
// LuaAddItem of the old ScriptFuns.cpp, and jx_linux_y 0x08120D30 -> 0x08120B30: fewer than six
// numbers is 0; the JX2 build puts the current table version (g_SubWorldSet+0x34), a zero seed and
// a zero in front and hands the nine to Lua_NewItem (0x0811F230) -> KItemSet::Add(genre, series,
// level, luck, detail, particular, magic levels...), the same order as the source.  The item
// goes to the first free spot of the bag; a full bag left it on the ground in the old server -
// that comes with the drops (M11 D), until then it is 0.  The magic prefix / suffix levels roll
// the attributes through Gen_MagicAttrib with luck 0 (Lua_NewItem passes no luck).
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
    if (npc != nullptr && !npc->script.empty()) w->execute_script(npc->script, "main", *p, 0);
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
