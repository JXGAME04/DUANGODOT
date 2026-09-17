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
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcTemplate.h"

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
    bool spawn_from_config = false;      // keep spawn_point even when a map bundle has its own
    std::shared_ptr<const KNpcTemplateSet> templates;   // npcs.txt numbers (frames, life, damage, ai); optional
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
    bool attack_request(std::uint64_t sid, EntityId target, std::uint32_t seq);
    bool chat(std::uint64_t sid, std::string_view text);

    static constexpr std::uint32_t kAttackEffectPercent = 60;   // ATTACKACTION_EFFECT_PERCENT (KNpc.cpp)
    static constexpr std::uint32_t kMinHurtPercent = 50;        // MIN_HURT_PERCENT (KNpc::DoHurt)
    static constexpr std::int32_t kMeleeReach = 96;             // scene units, until weapons carry their range
    EntityId spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius = 0,
                       KNpcKind kind = KNpcKind::npc);
    // Puts an entity elsewhere at once (KNpc::SetPos of the old core; traps and tests use it).
    bool teleport(EntityId id, Pos p);
    // Overrides the AIMode of a npc (SetNpcAIMode of the old script api); 0 switches the ai off.
    bool set_ai_mode(EntityId id, int mode);

    void tick();
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_; }

    [[nodiscard]] std::size_t player_count() const noexcept { return players_.size(); }
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const KNpc* find_entity(EntityId id) const;
    [[nodiscard]] const KNpc* find_player(std::uint64_t sid) const;
    [[nodiscard]] std::vector<std::uint64_t> session_ids() const;
    // Copies the stored RoleData with the current position (what PlayerSave sends).
    bool role_snapshot(std::uint64_t sid, pb::RoleData& out) const;
    // KNpcSet::GetRelation (server side): NPC_RELATION bits between two entities.
    [[nodiscard]] int relation(const KNpc& a, const KNpc& b) const noexcept;

    [[nodiscard]] Pos clamp(Pos p) const noexcept;
    [[nodiscard]] const KMapData* map() const noexcept { return cfg_.map.get(); }
    [[nodiscard]] std::uint32_t map_id() const noexcept { return cfg_.map ? static_cast<std::uint32_t>(cfg_.map->id) : 0u; }

    std::vector<Packet>& outbox() noexcept { return outbox_; }
    std::vector<Packet> take_outbox();

private:
    friend class KNpcAI;   // like the old KNpcAI, which is a friend of KNpc

    void emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    void viewers_of(Cell c, std::vector<std::uint64_t>& sids, EntityId exclude) const;
    void fill_info(const KNpc& e, pb::EntityInfo& out) const;
    void emit_move(const KNpc& e);
    void on_cell_change(KNpc& e, Cell from, Cell to);
    void wander(KNpc& e);
    // combat (KNpc::DoAttack / OnSpecial1 / DoHurt / DoDeath / DoRevive of the old core)
    void apply_template(KNpc& e) const;
    void update_action(KNpc& e);
    [[nodiscard]] int reach_of(const KNpc& e) const noexcept;
    [[nodiscard]] bool in_reach(const KNpc& a, const KNpc& b) const noexcept;
    void start_attack(KNpc& e, KNpc& target);
    void begin_action(KNpc& e, KNpc& target, std::uint32_t frames);
    void approach(KNpc& e, const KNpc& target);
    void hit(KNpc& attacker, KNpc& target);
    void heal(KNpc& e);
    void do_hurt(KNpc& e, EntityId source);
    void do_death(KNpc& e, EntityId killer);
    void do_revive(KNpc& e);
    void revive(KNpc& e);
    void emit_action(const KNpc& e, pb::Action action, EntityId target);
    void emit_life(const KNpc& e, std::int32_t delta, EntityId source);
    void broadcast(const KNpc& e, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    // what KNpcAI issues as SendCommand(do_walk / do_stand / do_skill) on the old server
    KNpc* find_mutable(EntityId id);
    bool rand_percent(int percent);   // g_RandPercent
    int random(int n);                // g_Random: 0 .. n-1
    void walk_to(KNpc& e, Pos dest);
    void do_stand(KNpc& e);
    void cast_skill(KNpc& e, KNpc& target);

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
