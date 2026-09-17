// The simulation.  World is pure logic: no sockets, no clock.  The server feeds it commands and
// tick() calls; World answers by appending Packets (already serialized protobuf) to its outbox.
// This keeps it unit-testable and, later, replayable from a recording.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <google/protobuf/message_lite.h>

#include "jx/client.pb.h"
#include "jx/common.pb.h"
#include "jx/role.pb.h"
#include "jx/ids.hpp"
#include "jx/zone/aoi.hpp"
#include "jx/zone/entity.hpp"

namespace jx::zone {

struct WorldConfig {
    std::uint32_t zone_id = 1;
    std::string name = "Test Field";
    std::uint32_t tick_hz = 20;
    std::int32_t width = 8192;
    std::int32_t height = 8192;
    Pos spawn_point{4096, 4096};
    std::int32_t cell_size = 512;
    std::int32_t view_cells = 1;
    std::uint32_t default_speed = 200;   // units per second
    std::uint32_t max_players = 2000;
    std::uint32_t seed = 1;              // npc wander rng
};

// One outgoing message for a set of sessions (fan-out happens at the gateway).
struct Packet {
    std::vector<std::uint64_t> sids;
    std::uint16_t msg_id = 0;
    std::string payload;
};

class World {
public:
    explicit World(WorldConfig cfg);

    [[nodiscard]] const WorldConfig& config() const noexcept { return cfg_; }

    pb::Result spawn_player(std::uint64_t sid, const pb::RoleData& role, EntityId& entity_out, Pos& pos_out);
    bool remove_player(std::uint64_t sid);
    bool move_request(std::uint64_t sid, Pos target, std::uint32_t seq);
    bool chat(std::uint64_t sid, std::string_view text);
    EntityId spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius = 0,
                       EntityKind kind = EntityKind::npc);

    void tick();
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_; }

    [[nodiscard]] std::size_t player_count() const noexcept { return players_.size(); }
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const Entity* find_entity(EntityId id) const;
    [[nodiscard]] const Entity* find_player(std::uint64_t sid) const;
    [[nodiscard]] std::vector<std::uint64_t> session_ids() const;
    // Copies the stored RoleData with the current position (what PlayerSave sends).
    bool role_snapshot(std::uint64_t sid, pb::RoleData& out) const;

    [[nodiscard]] Pos clamp(Pos p) const noexcept;

    std::vector<Packet>& outbox() noexcept { return outbox_; }
    std::vector<Packet> take_outbox();

private:
    void emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    void viewers_of(Cell c, std::vector<std::uint64_t>& sids, EntityId exclude) const;
    void fill_info(const Entity& e, pb::EntityInfo& out) const;
    void emit_move(const Entity& e);
    void on_cell_change(Entity& e, Cell from, Cell to);
    void wander(Entity& e);

    WorldConfig cfg_;
    AoiGrid grid_;
    IdGenerator ids_;
    std::unordered_map<EntityId, Entity, IdHash> entities_;
    std::unordered_map<std::uint64_t, EntityId> players_;      // sid -> entity
    std::unordered_map<std::uint64_t, pb::RoleData> roles_;    // sid -> persistent data
    std::vector<Packet> outbox_;
    std::uint64_t tick_ = 0;
    std::minstd_rand rng_;
    std::vector<EntityId> scratch_ids_, scratch_entered_, scratch_left_;
    std::vector<std::uint64_t> scratch_sids_;
};

} // namespace jx::zone
