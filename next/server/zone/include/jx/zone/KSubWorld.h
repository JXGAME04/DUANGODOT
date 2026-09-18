// The simulation.  World is pure logic: no sockets, no clock.  The server feeds it commands and
// tick() calls; World answers by appending Packets (already serialized protobuf) to its outbox.
// This keeps it unit-testable and, later, replayable from a recording.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
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
#include "jx/zone/KItem.h"
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
    // How many OTHER players one client is told about, nearest first.  A crowd otherwise makes the
    // traffic grow with the square of the number of players: measured, 1846 players in one spot
    // produced 1 058 242 packets a second.  The old server bounded it with MAX_BROADCAST_COUNT =
    // 100 receivers per packet (Core/Src/KRegion.h:9); the number is kept, but it now limits what
    // a client KNOWS instead of who a packet reaches, so no client is ever left with a ghost
    // (see KViewer below and KInterest.cpp).  0 = no limit.
    std::int32_t max_viewers = 100;
    // The same limit for everything that is not a player (shopkeepers, monsters, pets): how many
    // of them one client is told about, nearest first.  A town has a few hundred.
    std::int32_t max_known_npcs = 300;
    // How often a client's surroundings are looked at again when nothing forced it, in ticks.
    // Four ticks at 18 Hz is 0,22 s; the view is wider than the screen by more than anybody
    // walks in that time, so nothing pops up inside the picture.
    std::uint32_t interest_period = 4;
    // An entity a client knows is only taken away once it is this many cells OUTSIDE the view.
    // Without the slack somebody pacing along a cell edge appears and vanishes on every step.
    std::int32_t view_slack = 1;
    std::int32_t view_cells = 0;   // derived from view_width/view_height; set only by tests
    // How many entities one look around may add to what a client knows.  More than this waits for
    // the next tick, nearest first: a player walking into a packed town is told about it over a
    // few ticks instead of in one burst, and so is everybody he walks in on (N4).
    std::int32_t spawn_budget = 48;
    // Movement is told at two rates (N3).  A client hears about what moves within near_radius of
    // its character the moment it happens, one EntityMove each; what moves farther away is gathered
    // and sent every far_period ticks as one EntityMoves frame carrying the latest state of each
    // mover.  A move only needs to be on time where it can matter - a fight, a trade, somebody
    // walking up - and a quarter of the screen's width (320 units, about three characters) covers
    // that; the rest of the view is scenery that may lag a third of a second, and in a crowd that
    // is where most of the packets went.  The same radius decides when a full client (max_viewers)
    // trades a far player for one that walked up (KInterest.cpp).
    // near_radius 0 = a quarter of view_width; far_period 0 or 1 = everything at once, as before.
    std::int32_t near_radius = 0;
    std::uint32_t far_period = 6;
    std::uint32_t default_speed = 200;   // units per second
    std::uint32_t max_players = 2000;
    std::uint32_t seed = 1;              // npc wander rng
    std::shared_ptr<const KMapData> map;  // optional: walkability + spawn + npcs override the fields above
    bool map_npcs = true;                // place the npcs listed in the map bundle
    bool spawn_from_config = false;      // keep spawn_point even when a map bundle has its own
    std::shared_ptr<const KNpcTemplateSet> templates;   // npcs.txt numbers (frames, life, damage, ai); optional
    std::shared_ptr<const KItemLibrary> items;          // the item tables (settings\item, every version); optional
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

// What one client has been told about.  This is the heart of the interest management: a client
// receives updates ONLY for the entities in `known`, and every entity in `known` is taken away
// with a despawn when it leaves.  The old rule cut each broadcast to the 100 nearest sessions
// (MAX_BROADCAST_COUNT in Core/Src/KRegion.h), which is cheap but forgets who was told what: a
// client kept the ghost of a player whose departure was cut off, and saw a monster stand alive
// whose death it never received.  The limit now sits on the client's side - how many players it is
// told about at all - so the traffic is bounded the same way and nothing can go stale.
struct KViewer {
    EntityId self;
    std::vector<EntityId> known;       // sorted by id; always holds `self` once the client has its own spawn
    std::int32_t known_players = 0;    // how many of them are OTHER players (what max_viewers limits)
    std::uint64_t next_look = 0;       // tick of the next routine look around
    std::uint64_t next_swap = 0;       // a FULL client looks for nearer players to trade in only this often
    bool dirty = true;                 // look at the next opportunity: just arrived, changed cell, or still catching up
    std::vector<EntityId> far_pending; // sorted; known entities that moved far from this client, not told yet (N3)
    std::uint64_t next_far_flush = 0;  // tick at which far_pending goes out as one EntityMoves
    // What the last look saw: the cell, and the version of every cell in view (KRegionGrid), one
    // 64-bit word per cell (players << 32 | others).  A cell whose version is unchanged holds
    // exactly what it held last time, so its candidates are not walked again (KInterest.cpp).
    Cell last_cell;
    std::vector<std::uint64_t> cell_versions;
    std::int32_t last_room_players = 0;   // the room there was at the last look: more room now = look at everything again
    std::int32_t last_room_npcs = 0;
    bool looked = false;               // last_cell and the versions are meaningful
    bool carry = false;                // the last look ran out of budget: there is more to learn
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
    // Items (M11).  Each answers the client: G2C_ITEM_MOVE / ADD / REMOVE when something changed,
    // G2C_ITEM_RESULT with the reason when nothing did.
    bool item_move_request(std::uint64_t sid, std::uint32_t id, int room, int x, int y, std::uint32_t seq);
    bool item_equip_request(std::uint64_t sid, std::uint32_t id, int part, std::uint32_t seq);
    bool item_unequip_request(std::uint64_t sid, int part, std::uint32_t seq);
    bool item_use_request(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    bool item_drop_request(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    // Give an item to a player (a script, a drop picked up, a quest reward): into the bag, onto a
    // stack where it can; the client is told.  0 when it does not fit.
    std::uint32_t give_item(std::uint64_t sid, KItem item);
    bool take_item(std::uint64_t sid, std::uint32_t id);   // the item is gone (used up, taken by a script)
    void send_item_list(std::uint64_t sid);
    void fill_item_view(const KItem& item, const KItemPlace& place, pb::ItemView& out) const;

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
    // How many times a client could not be told about a player in view because it already knows
    // max_viewers of them: the number that says a crowd is being protected against, and by how much.
    [[nodiscard]] std::uint64_t viewers_capped() const noexcept { return viewers_capped_; }
    // what the interest pass did, in all: looks around, known entities re-checked, candidates weighed
    struct LookStats {
        std::uint64_t looks = 0, looks_dirty = 0, looks_carry = 0, looks_idle = 0, known_checked = 0, candidates = 0, learned = 0, forgotten = 0;
    };
    [[nodiscard]] const LookStats& look_stats() const noexcept { return look_stats_; }
    void reset_look_stats() noexcept { look_stats_ = {}; }
    // Where this map's tick spends its time, phase by phase (MASTER SPEC 22, 53): what a load test
    // and a benchmark read to say WHICH part is slow instead of guessing.
    [[nodiscard]] core::TickProfile& profile() noexcept { return *profile_; }
    // What the client of this session has been told about (nullptr when it is not on this map).
    [[nodiscard]] const KViewer* viewer(std::uint64_t sid) const;

    [[nodiscard]] std::size_t player_count() const noexcept { return players_.size(); }
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const KNpc* find_entity(EntityId id) const;
    [[nodiscard]] const KNpc* find_player(std::uint64_t sid) const;
    [[nodiscard]] std::vector<std::uint64_t> session_ids() const;
    // Copies the stored RoleData with the current position (what PlayerSave sends).
    bool role_snapshot(std::uint64_t sid, pb::RoleData& out) const;
    // What the player carries (KItemList).  Loaded from RoleData.items when the player spawns
    // (an item whose table row is gone is dropped with a warning), saved in every snapshot.
    [[nodiscard]] KItemList* items_of(std::uint64_t sid);
    [[nodiscard]] const KItemList* items_of(std::uint64_t sid) const;
    // The item tables and a generator on the default version; null without tables.
    [[nodiscard]] const KItemLibrary* item_library() const noexcept { return cfg_.items.get(); }
    [[nodiscard]] std::optional<KItemGenerator> item_generator(std::uint32_t version = 0);
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
    // interest management (KInterest.cpp): who is told about what
    void run_interest();                                   // once per tick: every client that is due looks around
    void look_around(std::uint64_t sid, KViewer& v);       // forget what left, learn what is near, within the budget
    void entity_gone(KNpc& e, bool keep_self = false);     // it leaves the world: every client that knows it is told
    void drop_viewer(std::uint64_t sid);                   // the session leaves: nobody is watched by it any more
    static constexpr int kSwapsPerLook = 4;                // how many far players a full client trades for near ones per look
    static constexpr std::uint64_t kSwapEveryLooks = 4;    // ... and it looks for them every 4th routine look (about 0,9 s)
    void fill_info(const KNpc& e, pb::EntityInfo& out) const;
    void fill_move(const KNpc& e, pb::EntityMove& out) const;
    void emit_move(const KNpc& e);
    void flush_far();                                      // once per tick: the far movements that are due, one frame per client
    [[nodiscard]] std::int32_t near_radius() const noexcept;
    [[nodiscard]] std::int64_t near_radius2() const noexcept;
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
    std::unordered_map<std::uint64_t, KItemList> items_;       // sid -> what the player carries
    void load_items(std::uint64_t sid, const pb::RoleData& role);
    void save_items(std::uint64_t sid, pb::RoleData& out) const;
    void item_result(std::uint64_t sid, std::uint32_t seq, pb::Result result);
    void item_moved(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    void item_changed(std::uint64_t sid, std::uint32_t id);   // G2C_ITEM_ADD with the item as it is now (a stack changed)
    void send_money(std::uint64_t sid);
    // KItemList::EnoughAttrib needs the player's numbers
    [[nodiscard]] std::function<int(int)> attrib_of(const KNpc& e) const;
    void process_potions(KNpc& e);   // the 补血状态 block of KNpc::ProcessState, every frame
    std::vector<Packet> outbox_;
    std::uint64_t tick_ = 0;
    std::minstd_rand rng_;
    std::vector<EntityId> scratch_ids_;
    std::vector<std::uint64_t> scratch_sids_;
    mutable std::unordered_map<std::uint64_t, KNpcLevelData> level_cache_;   // (template id, level, series) -> level data
    std::vector<KWorldChange> world_changes_;
    std::unordered_map<std::uint64_t, KViewer> viewers_;   // sid -> what that client has been told about
    struct Near {                                          // a candidate of a look around
        std::int64_t dist2;
        EntityId id;
    };
    std::vector<Near> scratch_near_players_, scratch_near_npcs_, scratch_far_known_;
    std::vector<std::uint64_t> scratch_due_;
    std::unordered_set<std::uint64_t> awake_cells_;   // cells with a player within the largest vision
    std::size_t awake_entities_ = 0;
    std::uint64_t idle_ticks_ = 0;   // consecutive ticks with no player and nothing awake
    std::uint64_t viewers_capped_ = 0;
    LookStats look_stats_;
    bool dormant_ = false;
    std::int32_t max_vision_ = 0;                     // largest vision radius spawned here
    // per phase cost of this map's tick (MASTER SPEC 22, 53, 96): "map.<id>.tick.<phase>"
    std::unique_ptr<core::TickProfile> profile_;
    std::unordered_map<std::uint32_t, bool> trap_warned_;   // trap ids without a script, warned once
};

} // namespace jx::zone
