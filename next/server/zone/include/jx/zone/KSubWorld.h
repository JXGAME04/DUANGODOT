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
#include <unordered_set>
#include <vector>

#include <google/protobuf/message_lite.h>

#include "jx/client.pb.h"
#include "jx/common.pb.h"
#include "jx/role.pb.h"
#include "jx/core/FixedTick.h"
#include "jx/entity/EntityTable.h"
#include "jx/ids.hpp"
#include "jx/zone/KRegion.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KPathFinder.h"
#include "jx/zone/KScriptCache.h"

namespace jx::zone {

struct KSubWorldConfig {
    std::uint32_t zone_id = 1;
    std::string name = "Test Field";
    std::uint32_t tick_hz = 20;
    std::int32_t width = 8192;
    std::int32_t height = 8192;
    Pos spawn_point{4096, 4096};
    std::int32_t cell_size = 256;
    // What one client must be told about, in scene units.  The renderer draws a scene point at
    // (x, y/2), so a screen W x H pixels shows W units of x and 2H units of y: 1280 x 1536 covers
    // both the VLTK 2.0 client (1024 x 768) and this project's Godot client (1280 x 720) with room
    // to spare.  The grid turns these into cell counts, rounding up so the view is never smaller
    // than the screen - an entity the player can see must always have been sent.
    std::int32_t view_width = 1280;
    std::int32_t view_height = 1536;
    // At most this many sessions receive one world packet, nearest first.  A crowd otherwise makes
    // the traffic grow with the square of the number of players: measured, 1846 players in one spot
    // produced 1 058 242 packets a second.  The old server had the same rule, MAX_BROADCAST_COUNT
    // in Core/Src/KRegion.h, and its number was 100.  0 = no limit.
    std::int32_t max_viewers = 100;
    std::int32_t view_cells = 0;   // derived from view_width/view_height; set only by tests
    std::uint32_t default_speed = 200;   // units per second
    std::uint32_t max_players = 2000;
    std::uint32_t seed = 1;              // npc wander rng
    std::shared_ptr<const KMapData> map;  // optional: walkability + spawn + npcs override the fields above
    bool map_npcs = true;                // place the npcs listed in the map bundle
    bool spawn_from_config = false;      // keep spawn_point even when a map bundle has its own
    std::shared_ptr<const KNpcTemplateSet> templates;   // npcs.txt numbers (frames, life, damage, ai); optional
    std::shared_ptr<KScriptCache> scripts;              // the old server folder with script\ (level scripts); optional
};

// A move to another map a trap script asked for (KNpc::ChangeWorld); KGameServer carries it out.
struct KWorldChange {
    std::uint64_t sid = 0;
    std::uint32_t map_id = 0;
    Pos pos;
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

    // at: where to put the player instead of the saved / spawn position (a map change)
    pb::Result spawn_player(std::uint64_t sid, const pb::RoleData& role, EntityId& entity_out, Pos& pos_out, const Pos* at = nullptr);
    bool remove_player(std::uint64_t sid);
    bool move_request(std::uint64_t sid, Pos target, std::uint32_t seq);
    bool attack_request(std::uint64_t sid, EntityId target, std::uint32_t seq);
    bool chat(std::uint64_t sid, std::string_view text);

    static constexpr std::uint32_t kAttackEffectPercent = 60;   // ATTACKACTION_EFFECT_PERCENT (KNpc.cpp)
    static constexpr std::uint32_t kMinHurtPercent = 50;        // MIN_HURT_PERCENT (KNpc::DoHurt)
    static constexpr std::int32_t kMeleeReach = 96;             // scene units, until weapons carry their range
    static constexpr int kMaxResist = 95;                       // MAX_RESIST (GameDataDef.h)
    static constexpr int kMaxHitPercent = 95;                   // MAX_HIT_PERCENT
    static constexpr int kMinHitPercent = 5;                    // MIN_HIT_PERCENT
    static constexpr std::uint64_t kGameUpdateTime = 10;        // GAME_UPDATE_TIME: frames between two KNpc::ProcessState
    // How long a map has to stay empty and quiet before its tick stops doing anything at all.
    // Two seconds at 18 Hz: long enough that walking out and back in never sees a frozen map.
    static constexpr std::uint64_t kDormantAfterTicks = 36;
    EntityId spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius = 0,
                       KNpcKind kind = KNpcKind::npc);
    // Puts an entity elsewhere at once (KNpc::SetPos of the old core; traps and tests use it).
    bool teleport(EntityId id, Pos p);
    // Overrides the AIMode of a npc (SetNpcAIMode of the old script api); 0 switches the ai off.
    bool set_ai_mode(EntityId id, int mode);
    // KNpc::SetPos: a jump within this map (the SetPos of the scripts), then DoStand.
    bool set_pos(EntityId id, Pos p);
    // KNpc::ChangeWorld from a script: the same map is a SetPos (1); another map is queued for
    // KGameServer (1); 0 = failed.  Only players change worlds.
    int change_world_request(KNpc& player, std::uint32_t map_id, Pos pos);
    std::vector<KWorldChange> take_world_changes();
    // Msg2Player: one line in the player's chat window.
    void msg_to_player(std::uint64_t sid, std::string_view text);
    // KPlayer::ExecuteScript: runs fn(param) of the script (a game path) for the player.
    bool execute_script(const std::string& game_path, const char* fn, KNpc& player, int param = 0);

    void tick();
    [[nodiscard]] std::uint64_t tick_count() const noexcept { return tick_; }
    // How many entities actually thought during the last tick (MASTER SPEC 44, 45): the rest were
    // asleep because no player was near.  The difference is the CPU a crowded map gets back.
    [[nodiscard]] std::size_t awake_entities() const noexcept { return awake_entities_; }
    // A map nobody is on and where nothing is still happening.  Its tick does nothing at all, so
    // a zone can host all 980 maps of the old game and only pay for the ones being played.
    [[nodiscard]] bool dormant() const noexcept { return dormant_; }
    [[nodiscard]] std::uint64_t idle_ticks() const noexcept { return idle_ticks_; }
    // How many times a viewer list was cut down to max_viewers: the number that says a crowd is
    // being protected against, and by how much.
    [[nodiscard]] std::uint64_t viewers_capped() const noexcept { return viewers_capped_; }

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
    // Mps2Map / Map2Mps: the old absolute scene coordinates (what scripts pass to SetPos / NewWorld)
    // against this map's local ones (bundle relative).
    [[nodiscard]] Pos to_local(Pos absolute) const noexcept;
    [[nodiscard]] Pos to_absolute(Pos local) const noexcept;
    [[nodiscard]] const KMapData* map() const noexcept { return cfg_.map.get(); }
    [[nodiscard]] std::uint32_t map_id() const noexcept { return cfg_.map ? static_cast<std::uint32_t>(cfg_.map->id) : 0u; }

    std::vector<Packet>& outbox() noexcept { return outbox_; }
    std::vector<Packet> take_outbox();

private:
    friend class KNpcAI;   // like the old KNpcAI, which is a friend of KNpc

    void emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    // A crowded spot can put hundreds of entities in one EntitySpawn, which would pass the 64 KiB
    // the client protocol allows (docs/PROTOCOL.md).  This sends them in pieces that always fit.
    void emit_spawn(const std::vector<std::uint64_t>& sids, const pb::EntitySpawn& spawn);
    static constexpr int kSpawnChunk = 48;
    void viewers_of(Cell c, std::vector<std::uint64_t>& sids, EntityId exclude) const;
    // Who can see this cell, computed once per cell per tick.  With a crowd standing on the
    // same spot (Tống Kim) the same answer was being recomputed for every single command; the
    // cache is dropped whenever the set of players or their cells changes (MASTER SPEC 26, 96).
    const std::vector<std::uint64_t>& viewers_cached(Cell c) const;
    void invalidate_viewers() const noexcept { viewer_cache_.clear(); }
    void fill_info(const KNpc& e, pb::EntityInfo& out) const;
    void emit_move(const KNpc& e);
    void on_cell_change(KNpc& e, Cell from, Cell to);
    void wander(KNpc& e);
    // entity sleeping (SPEC 44, 45)
    void build_awake_cells();
    [[nodiscard]] std::int32_t awake_radius_cells() const noexcept;
    [[nodiscard]] bool is_awake(const KNpc& e) const;
    // combat (KNpc::DoAttack / OnSpecial1 / DoHurt / DoDeath / DoRevive of the old core)
    void apply_template(KNpc& e) const;
    void update_action(KNpc& e);
    [[nodiscard]] int reach_of(const KNpc& e) const noexcept;
    [[nodiscard]] bool in_reach(const KNpc& a, const KNpc& b) const noexcept;
    void start_attack(KNpc& e, KNpc& target);
    void begin_action(KNpc& e, KNpc& target, std::uint32_t frames);
    void approach(KNpc& e, const KNpc& target);
    void hit(KNpc& attacker, KNpc& target);
    bool check_hit_target(int ar, int df, int ignore = 0);   // KNpc::CheckHitTarget
    void check_trap(KNpc& e);                                // KNpc::CheckTrap (players, every frame with m_ProcessAI)
    void process_state(KNpc& e);                            // KNpc::ProcessState: natural life regeneration
    // KNpc::Init -> g_pNpcTemplate[id][level]: the level data of a template, computed once per (id, level, series)
    [[nodiscard]] const KNpcLevelData& level_data_of(const KNpcTemplate& t, int level, int series) const;
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
    KPathFinder paths_;   // reused A* buffers: no allocation of map sized arrays in the tick
    // MASTER SPEC 5 / 36: entities live in one table that hands out handles with a generation,
    // so a stale id (a missile in flight, a target in a packet, a Lua variable) can never reach
    // the creature that took the slot.  The live entities are contiguous for the hot loops.
    entity::EntityTable<KNpc> entities_;
    std::unordered_map<std::uint64_t, EntityId> players_;      // sid -> entity
    std::unordered_map<std::uint64_t, pb::RoleData> roles_;    // sid -> persistent data
    std::vector<Packet> outbox_;
    std::uint64_t tick_ = 0;
    std::minstd_rand rng_;
    std::vector<EntityId> scratch_ids_, scratch_entered_, scratch_left_;
    std::vector<std::uint64_t> scratch_sids_;
    mutable std::unordered_map<std::uint64_t, KNpcLevelData> level_cache_;   // (template id, level, series) -> level data
    std::vector<KWorldChange> world_changes_;
    mutable std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> viewer_cache_;   // cell -> player sids
    std::unordered_set<std::uint64_t> awake_cells_;   // cells with a player within the largest vision
    std::size_t awake_entities_ = 0;
    std::uint64_t idle_ticks_ = 0;   // consecutive ticks with no player and nothing awake
    mutable std::uint64_t viewers_capped_ = 0;
    bool dormant_ = false;
    std::int32_t max_vision_ = 0;                     // largest vision radius spawned here
    // per phase cost of this map's tick (MASTER SPEC 22, 53, 96): "map.<id>.tick.<phase>"
    std::unique_ptr<core::TickProfile> profile_;
    std::unordered_map<std::uint32_t, bool> trap_warned_;   // trap ids without a script, warned once
};

} // namespace jx::zone
