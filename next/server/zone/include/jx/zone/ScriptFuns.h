// ScriptFuns.cpp of the old core (GameScriptFuns[]): the script api a KLuaScript sees, as far
// as the trap / portal scripts need it.  A script runs "for" a player (SCRIPT_PLAYERINDEX of the
// old engine): KSubWorld::execute_script sets the context before the call.
#pragma once

#include <cstdint>

#include "jx/ids.hpp"

struct lua_State;

namespace jx::zone {

class KSubWorld;
struct KNpc;

struct KScriptContext {
    KSubWorld* world = nullptr;
    KNpc* player = nullptr;   // the player the script runs for (nullptr = none)
    std::uint64_t sid = 0;
};

// The context of the script being executed (one at a time: the zone is single threaded).
KScriptContext& g_ScriptContext() noexcept;

// KLuaScript::RegisterFunctions(GameScriptFuns, ...): installs the api into a state.
void RegisterGameScriptFuns(lua_State* L);

} // namespace jx::zone
