// Commands into a map instance and events out of it (MASTER SPEC 19, 30, 63).
//
// Nothing outside the owning worker touches world state.  The network thread decodes a packet
// and pushes a command; the owner applies it at the start of its next tick.  Whatever the world
// wants from the outside (an ack, a save, a transfer to another map) leaves as an event that
// the server thread carries out.
#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "jx/client.pb.h"
#include "jx/common.pb.h"
#include "jx/role.pb.h"
#include "jx/ids.hpp"
#include "jx/zone/KRegion.h"

namespace jx::zone {

// Why a player is being spawned; it decides what the session is told afterwards.
enum class KSpawnReason : std::uint8_t {
    enter_world = 0,   // gateway SessionOpen -> SessionOpenAck
    change_map,        // a trap script moved the player here -> G2C_CHANGE_MAP
};

struct KCmdSpawnPlayer {
    std::uint64_t sid = 0;
    std::uint64_t conn_id = 0;      // the gateway that owns the session
    pb::RoleData role;
    std::string account;            // the account's name (SessionOpen.account): Player+0x264, Lua GetAccount
    bool has_at = false;            // spawn at `at` instead of the saved / default position
    Pos at;
    KSpawnReason reason = KSpawnReason::enter_world;
};

struct KCmdRemovePlayer {
    std::uint64_t sid = 0;
    bool save = true;               // emit a final PlayerSave before removing
};

// A client message forwarded verbatim (C2G_MOVE, C2G_ATTACK, C2G_CHAT ...).
struct KCmdClientPacket {
    std::uint64_t sid = 0;
    std::uint32_t msg_id = 0;
    std::string payload;
};

// The periodic save asked for by the server.
struct KCmdSaveRequest {
    std::uint64_t sid = 0;
    bool final = false;
};

using KWorldCommand = std::variant<KCmdSpawnPlayer, KCmdRemovePlayer, KCmdClientPacket, KCmdSaveRequest>;

// ---- events -------------------------------------------------------------------------------

struct KEvSessionOpened {
    std::uint64_t sid = 0;
    std::uint64_t conn_id = 0;
    pb::Result result = pb::RESULT_OK;
    EntityId entity;
    Pos pos;
    std::uint32_t map_id = 0;
    std::uint32_t scene_w = 0;
    std::uint32_t scene_h = 0;
    KSpawnReason reason = KSpawnReason::enter_world;
};

struct KEvPlayerSave {
    std::uint64_t sid = 0;
    pb::RoleData role;
    bool final = false;
    std::uint64_t tick = 0;   // the map's tick the snapshot was taken at
};

// KNpc::ChangeWorld across map instances: the owner already removed the player and saved its
// state here; the server hands it to the target instance (SPEC 63).
struct KEvWorldChange {
    std::uint64_t sid = 0;
    std::uint64_t conn_id = 0;
    std::uint32_t map_id = 0;
    Pos pos;
    pb::RoleData role;
};

// a chat line for people beyond the map (WORLD / CITY / FACTION): the server sends it to every session of the zone
// that qualifies - the relay's broadcast class of the old game, done here
struct KEvChat {
    pb::ChatChannel channel = pb::CH_WORLD;
    int faction = -1;
    std::string payload;
};

using KWorldEvent = std::variant<KEvSessionOpened, KEvPlayerSave, KEvWorldChange, KEvChat>;

// std::visit helper: visit(cmd, [](const KCmdSpawnPlayer&){...}, [](const KCmdRemovePlayer&){...});
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace jx::zone
