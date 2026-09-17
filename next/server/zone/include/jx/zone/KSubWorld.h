// The simulation.  World is pure logic: no sockets, no clock.  The server feeds it commands and
// tick() calls; World answers by appending Packets (already serialized protobuf) to its outbox.
// This keeps it unit-testable and, later, replayable from a recording.
#pragma once

#include <cstdint>
#include <memory>
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
#include "jx/zone/KRegion.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KMapData.h"

namespace jx::zone {

struct KSubWorldConfig {
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
    std::shared_ptr<const KMapData> map;  // optional: walkability + spawn + npcs override the fields above
    bool map_npcs = true;                // place the npcs listed in the map bundle
};

// One outgoing message for a set of sessions (fan-out happens at the gateway).
struct Packet {
    std::vector<std::uint64_t> sids;
    std::uint16_t msg_id = 0;
    std::string payload;
};

class KSubWorld {
public:
    explicit KSubWorld(KSubWorldConfig cfg);

    [[nodiscard]] const KSubWorldConfig& config() const noexcept { return cfg_; }

    pb::Result spawn_player(std::uint64_t sid, const pb::RoleData& role, EntityId& entity_out, Pos& pos_out);
    bool remove_player(std::uint64_t sid);
    bool move_request(std::uint64_t sid, Pos target, std::uint32_t seq);
    bool chat(std::uint64_t sid, std::string_view text);
    EntityId spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius = 0,
                       KNpcKind kind = KNpcKind::npc);

    void tick();
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_; }

    [[nodiscard]] std::size_t player_count() const noexcept { return players_.size(); }
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const KNpc* find_entity(EntityId id) const;
    [[nodiscard]] const KNpc* find_player(std::uint64_t sid) const;
    [[nodiscard]] std::vector<std::uint64_t> session_ids() const;
    // Copies the stored RoleData with the current position (what PlayerSave sends).
    bool role_snapshot(std::uint64_t sid, pb::RoleData& out) const;

    [[nodiscard]] Pos clamp(Pos p) const noexcept;
    [[nodiscard]] const KMapData* map() const noexcept { return cfg_.map.get(); }
    [[nodiscard]] std::uint32_t map_id() const noexcept { return cfg_.map ? static_cast<std::uint32_t>(cfg_.map->id) : 0u; }

    std::vector<Packet>& outbox() noexcept { return outbox_; }
    std::vector<Packet> take_outbox();

private:
    void emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    void viewers_of(Cell c, std::vector<std::uint64_t>& sids, EntityId exclude) const;
    void fill_info(const KNpc& e, pb::EntityInfo& out) const;
    void emit_move(const KNpc& e);
    void on_cell_change(KNpc& e, Cell from, Cell to);
    void wander(KNpc& e);

    KSubWorldConfig cfg_;
    KRegionGrid grid_;
    IdGenerator ids_;
    std::unordered_map<EntityId, KNpc, IdHash> entities_;
    std::unordered_map<std::uint64_t, EntityId> players_;      // sid -> entity
    std::unordered_map<std::uint64_t, pb::RoleData> roles_;    // sid -> persistent data
    std::vector<Packet> outbox_;
    std::uint64_t tick_ = 0;
    std::minstd_rand rng_;
    std::vector<EntityId> scratch_ids_, scratch_entered_, scratch_left_;
    std::vector<std::uint64_t> scratch_sids_;
};

} // namespace jx::zone
