// The script api of the old server (Core/Src/ScriptFuns.cpp) for the traps: positions are in
// cells of 32 units like the old Lua* functions (SetPos(x, y) -> SetPos(x * 32, y * 32)).
#include "jx/zone/ScriptFuns.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <algorithm>
#include <optional>
#include <utility>

#include "jx/log.hpp"
#include "jx/zone/KItem.h"
#include "jx/zone/KNpc.h"
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

const luaL_Reg kGameScriptFuns[] = {
    {"GetFightState", l_GetFightState}, {"SetFightState", l_SetFightState}, {"SetPos", l_SetPos},
    {"NewWorld", l_NewWorld},           {"GetPos", l_GetPos},               {"GetWorldPos", l_GetWorldPos},
    {"Msg2Player", l_Msg2Player},       {"AddStation", l_AddStation},       {"AddTermini", l_AddTermini},
    {"Say", l_Say},                     {"Talk", l_Talk},                   {"AddItem", l_AddItem},
    {"AddGoldItem", l_AddGoldItem},     {"AddStackItem", l_AddStackItem},   {"HaveItem", l_HaveItem},
    {"GetItemCount", l_GetItemCount},   {"GetItemCountEx", l_GetItemCountEx}, {"DelItem", l_DelItem},
    {"DelItemEx", l_DelItemEx},         {"HaveCommonItem", l_HaveCommonItem}, {"DelCommonItem", l_DelCommonItem},
    {"GetTotalItemCount", l_GetTotalItemCount}, {nullptr, nullptr},
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
