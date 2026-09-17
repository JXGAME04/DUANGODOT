// The script api of the old server (Core/Src/ScriptFuns.cpp) for the traps: positions are in
// cells of 32 units like the old Lua* functions (SetPos(x, y) -> SetPos(x * 32, y * 32)).
#include "jx/zone/ScriptFuns.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include "jx/log.hpp"
#include "jx/zone/KNpc.h"
#include "jx/zone/KSubWorld.h"

namespace jx::zone {

KScriptContext& g_ScriptContext() noexcept
{
    static KScriptContext ctx;
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

const luaL_Reg kGameScriptFuns[] = {
    {"GetFightState", l_GetFightState}, {"SetFightState", l_SetFightState}, {"SetPos", l_SetPos},
    {"NewWorld", l_NewWorld},           {"GetPos", l_GetPos},               {"GetWorldPos", l_GetWorldPos},
    {"Msg2Player", l_Msg2Player},       {"AddStation", l_AddStation},       {"AddTermini", l_AddTermini},
    {"Say", l_Say},                     {"Talk", l_Talk},                   {nullptr, nullptr},
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
