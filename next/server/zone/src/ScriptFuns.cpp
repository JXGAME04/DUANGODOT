// The script api of the old server (Core/Src/ScriptFuns.cpp) for the traps: positions are in
// cells of 32 units like the old Lua* functions (SetPos(x, y) -> SetPos(x * 32, y * 32)).
#include "jx/zone/ScriptFuns.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "jx/log.hpp"
#include "jx/zone/KItem.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSkillList.h"
#include "jx/zone/KSubWorld.h"

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

// Say / Talk: the dialog window (LuaSelectUI / LuaTalkUI); nothing to show yet, logged for later
int l_Say(lua_State* L)
{
    log::info("lua", "Say (dialog not implemented)", {log::kv("text", luaL_optstring(L, 1, ""))});
    return 0;
}

int l_Talk(lua_State* L)
{
    log::info("lua", "Talk (dialog not implemented)", {log::kv("pages", lua_gettop(L))});
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

// ForbitStamina(n): Player+0x86b4 = (n ~= 0) (0x0810CCC0) - no stamina gain while set (ProcessState 0x0808BD53)
int l_ForbitStamina(lua_State* L)
{
    KNpc* p = player_of(L, "ForbitStamina");
    if (p == nullptr || lua_gettop(L) < 1) return 0;
    p->player.forbid_stamina = static_cast<int>(lua_tonumber(L, 1)) != 0 ? 1 : 0;
    return 0;
}

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
