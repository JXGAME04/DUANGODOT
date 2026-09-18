// The simulation.  World is pure logic: no sockets, no clock.  The server feeds it commands and
// tick() calls; World answers by appending Packets (already serialized protobuf) to its outbox.
// This keeps it unit-testable and, later, replayable from a recording.
#pragma once

#include <cstdint>
#include <deque>
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
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KRegion.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KObj.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KPathFinder.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KMissle.h"

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
    // the percent of a blow between players (and partners), KNpc::CalcDamage 0x0808A368 reads it
    // from the global 0x08BADF50; its loader is not found yet (B3, the PK rules) - 100 = as is
    int pk_damage_percent = 100;
    std::shared_ptr<const KMapData> map;  // optional: walkability + spawn + npcs override the fields above
    bool map_npcs = true;                // place the npcs listed in the map bundle
    bool spawn_from_config = false;      // keep spawn_point even when a map bundle has its own
    std::shared_ptr<const KNpcTemplateSet> templates;   // npcs.txt numbers (frames, life, damage, ai); optional
    std::shared_ptr<const KItemLibrary> items;          // the item tables (settings\item, every version); optional
    std::shared_ptr<const KObjDataSet> objdata;         // ObjData.txt / MoneyObj.txt (objdata.json): what a drop looks like; optional
    int money_rate_percent = 100;                       // [ServerConfig] MoneyRate of gamesetting.ini: dropped money x this / 100
    std::uint32_t item_version = 0;                     // the table set new items are made from (g_SubWorldSet+0x34 of jx_linux_y); 0 = the newest
    // KGMCommand.cpp: a chat line "?gm ds <lua>" runs the code for the player, "?gm dw <lua>" for
    // the world.  Only while this is on (a development server); accounts with a GM flag come later.
    bool gm_chat = false;
    std::shared_ptr<KScriptCache> scripts;              // the old server folder with script\ (level scripts); optional
    // settings/npc/player of the old server (player.json): level tables, stamina.ini, basevalue.ini;
    // optional - without it the built-in defaults of KPlayerSet apply
    std::shared_ptr<const KPlayerSet> player_set;
    // settings/skills.txt of the old server (skills.json of jxassets export-skills): every skill's
    // row; the numbers per level come from the skill scripts (KScriptCache) at run time.  Optional.
    std::shared_ptr<const KSkillTable> skills;
    // settings/missles.txt of the old server (missles.json of jxassets export-missles): the missile
    // templates the skills fire (KSkill::CreateMissle).  Optional; a skill whose template is
    // missing fires the zero template, which falls to the ground at once (like the binary's
    // unfilled array cell).
    std::shared_ptr<const KMissleTable> missles;
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
    // C2G_ADD_POINT: KPlayer::AddBaseStrength / Dexterity / Vitality / Engergy with the check
    // against m_nAttributePoint; answered with G2C_PLAYER_ATTRIB either way
    bool add_point_request(std::uint64_t sid, int attribute, int points, std::uint32_t seq);
    // G2C_PLAYER_ATTRIB: the character's own numbers to its client
    void send_player_attrib(std::uint64_t sid, std::uint32_t seq = 0);
    bool item_drop_request(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    // KPlayer::ServerPickUpItem: the thing on the ground goes into the bag (or the purse)
    bool pick_up_request(std::uint64_t sid, EntityId object, std::uint32_t seq);
    // KObjSet::Add / AddMoneyObj: an item / a pile of money appears on the ground near `at`
    // (KSubWorld::GetFreeObjPos), kept for `belong` (a player id, 0 = anybody).  Null id when the
    // object data is missing.
    EntityId drop_item(KItem item, Pos at, std::uint64_t belong);
    EntityId drop_money(int amount, Pos at, std::uint64_t belong);
    [[nodiscard]] const KItem* ground_item(EntityId object) const;
    [[nodiscard]] std::size_t ground_object_count() const noexcept { return ground_items_.size() + ground_money_; }
    // KNpc::OnDeath (the drops): Treasure rolls of the template's drop table for the killer
    void lose_treasure(KNpc& dead, EntityId killer);
    // KNpc::LoseSingleItem -> GenRandomItem of jx_linux_y (0x08083BB0): one roll of a drop table
    std::optional<KItem> gen_random_item(const KNpcDropRate& table, int npc_level, int npc_series, int luck);
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
    // the entity to change (the script api and the tests set numbers on it)
    [[nodiscard]] KNpc* mutable_entity(EntityId id);
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
    [[nodiscard]] std::uint32_t item_version() const noexcept { return cfg_.item_version; }
    // TextGMFilter of KGMCommand.cpp: true when the text was a GM command (handled, not chat)
    bool gm_command(std::uint64_t sid, std::string_view text);
    // g_SkillManager of this map: the skills at every level, made from the table and this map's
    // Lua states on first use; nullptr without a skill table
    [[nodiscard]] KSkillManager* skills() noexcept { return skills_.get(); }
    // KNpcSet::GetRelation (server side): NPC_RELATION bits between two entities.
    [[nodiscard]] int relation(const KNpc& a, const KNpc& b) const noexcept;

    // ---- the fight, function by function from the JX2 server (jx_linux_y; the addresses in
    // docs/LINUX-SERVER.md §12).  KSkills.cpp holds the casts, KNpc.cpp the blows and the states.
    struct KCastParams {
        EntityId target;      // the npc aimed at (nParam1 == -1, nParam2 = its index of the old call)
        Pos pos;              // or a spot on the map (nParam1 / nParam2 = x / y) when at_pos
        bool at_pos = false;
        int dir = -1;         // or a direction 0..64 to fire in (nParam1 == -2, nParam2 = the direction)
        int wait_time = 0;    // nWaitTime (negative is taken as 0)
        int extra = 0;        // the 7th argument of KSkill::Cast, handed on to the state as its 8th
    };
    // TOrdinSkillParam of the old KSkills.cpp: what CastMissles hands its generators (the block at
    // [ebp-0x5c] of 0x080ECAC0)
    struct KOrdinSkillParam {
        EntityId launcher;        // +0    the npc that fires
        int launcher_type = 0;    // +8    always 0 (a generator refuses anything else)
        int parent_missle = 0;    // +0xc  the missile the cast came from (0 = the npc), the children's m_nParentMissleIndex
        int parent_type = 0;      // +0x10 2 when a missile casts (the launcher type of the child casts)
        int wait_time = 0;        // +0x20 nWaitTime of the cast
        EntityId target;          // +0x24 the npc aimed at (none = -1 of the old call)
    };
    // KSkill::Cast 0x080EA920 for a npc launcher (eLauncherType 0): style 0 CastMissles 0x080ECAC0,
    // 2 CastInitiativeSkill 0x080EAC90, 3 CastPassivitySkill 0x080E8530, 14 the instant missile
    // 0x080EA720; 1 and 5..13 nothing, 4 (a npc made) not yet.
    bool skill_cast(const KSkill& skill, KNpc& launcher, const KCastParams& p);
    // the same for a missile launcher (eLauncherType 2: the events of a missile and the child casts
    // of a cast that came from one).  Only style 0 is meant for it in the binary - the other styles
    // would take the missile's index for a npc's - so they are refused here.
    bool missle_skill_cast(const KSkill& skill, int missle_index, const KCastParams& p);
    // KSkill::CastMissles 0x080ECAC0: the missiles of a cast by the skill's MisslesForm, then the
    // start event; `by` is the missile that casts (eLauncherType 2), null for the npc `launcher`
    // (which, for a missile, is the npc that fired that missile).  1 like the binary, 0 refused.
    int cast_missles(const KSkill& skill, KMissle* by, KNpc& launcher, const KCastParams& p);
    // the instant missile of a style 14 skill (0x080EA720): the start event, one missile at the
    // target's spot that deals its area blow and vanishes in the same call
    int cast_instant_missle(const KSkill& skill, KNpc& launcher, const KCastParams& p);
    // KMissleSet::Activate 0x08076EB0: every missile's frame (0x08076950), then the vanished ones
    // are taken out (the deferred nodes 0xfa1 of the binary).  KSubWorld::tick calls it after the
    // npcs, as KRegion::Activate 0x080E2660 does.
    void activate_missles();
    [[nodiscard]] const KMissle* missle(int index) const noexcept
    {
        return index > 0 && static_cast<std::size_t>(index) < missles_.size() && missles_[static_cast<std::size_t>(index)].used()
                   ? &missles_[static_cast<std::size_t>(index)] : nullptr;
    }
    [[nodiscard]] std::size_t missle_count() const noexcept { return live_missles_; }
    [[nodiscard]] const std::deque<KMissle>& missles() const noexcept { return missles_; }
    // Map2Mps of a missile (0x08074940): its position in scene units
    [[nodiscard]] static Pos missle_pos(const KMissle& m) noexcept
    {
        return Pos{m.map_x * kMissleCell + (m.x_offset >> 10), m.map_y * kMissleCell + (m.y_offset >> 10)};
    }
    // the start delay of the i-th missile of a cast (0x080E8650, the jump table 0x08258424 on MslsGenerate)
    int missle_start_life_time(const KSkill& skill, int i);
    bool cast_initiative_skill(const KSkill& skill, KNpc& launcher, int param1, EntityId target, int wait_time, int extra,
                               int time_override = 0, bool refresh = false, int param10 = 0, int param12 = 0);
    bool cast_passivity_skill(const KSkill& skill, KNpc& launcher, int extra);
    // 0x080EAB90: the StartEvent cast and the launcher's auto-skill map (0x080821C0, B2b)
    void skill_start_event(const KSkill& skill, KNpc& launcher, const KCastParams& p);
    // KSkill::CreateMissleMagicAttribsData 0x080E9E90: the skill's payload for this launcher, then
    // the payloads of its appended skills; false when the skill is ClientSend
    bool create_missle_magic_attribs_data(const KSkill& skill, KNpc& launcher, KMissleMagicAttribsList& out);
    // KNpc::ReceiveDamage 0x0808A4A0: 1 = the blow landed (states may follow), 0 = nothing happened
    int receive_damage(KNpc& target, KNpc& attacker, int series, bool melee, const KMagicAttrib* damage, bool use_ar, int do_hurt, int relation, int skill_id);
    // KNpc::CalcDamage 0x08089C90: 1 = still alive (or nothing to do), 0 = dead or refused
    int calc_damage(KNpc& target, KNpc& attacker, int min, int max, int type, bool melee, const int* dynamic_shield, int* dealt, int do_hurt, bool is_return);
    // 0x0807BCD0: the resist of the target against a damage type, 0..95
    [[nodiscard]] static int calc_resist(const KNpc& target, const KNpc* attacker, int type, bool melee, bool is_return) noexcept;
    // KNpc::AppendSkillEffect 0x0807CE70 with its five element helpers: the launcher's numbers
    // merged into the skill's damage attributes, one slot per kind (KDamageSlot)
    void append_skill_effect(const KNpc& launcher, bool physical, bool melee, const std::array<KMagicAttrib, kSkillAttribs>& src,
                             std::array<KMagicAttrib, kSkillAttribs>& des, int enhance);
    // KNpc::SetStateSkillEffect 0x08086260: returns -1 (refused), the frames the state had left
    // (it was there already) or 0
    int set_state_skill_effect(KNpc& target, EntityId launcher, int skill_id, int level, const KMagicAttrib* states, int count, int time,
                               int param8 = 0, bool refresh = false, int param10 = 0, bool reflected = false, int param12 = 0);
    void set_immediately_skill_effect(KNpc& target, EntityId launcher, const KMagicAttrib* attribs, int count);   // 0x0807D5A0
    void remove_state_skill_effect(KNpc& target, int skill_id, bool notify);                                      // 0x0807D310
    void modify_attrib(KNpc& target, EntityId launcher, const KMagicAttrib& m, bool removing);                    // KNpc::ModifyAttrib 0x0807D210
    // 0x0807BD60: the poison state set, or merged with the one running
    void set_poison(KNpc& target, EntityId launcher, int damage, int time, int interval);
    // 0x0807BAA0: the five resist maximums moved by a five-elements blow
    void modify_five_resist_max(KNpc& target, int enhance, int p);
    // KNpc::KnockBack 0x08087940: pushed `distance` away from the launcher over `frames` frames
    void knock_back(KNpc& target, const KNpc& launcher, int frames, int distance);
    // KNpc::OnHurt 0x0807F780: the stagger, shorter with hit recover; 0x0807F9D0 rolls DoHurt first
    void do_hurt(KNpc& e, int anti_hit_recover, EntityId source);
    void do_hurt_chance(KNpc& target, int do_hurt, const KNpc& attacker);
    // 0x08188BB0 over one auto-skill list of `owner` (KAutoSkillList): `self_key` is the npc the
    // list is walked for - it keys the wait between two casts and is the target of a cast that is
    // not "at target"; `other` is the target of one that is
    void trigger_auto_skills(KNpc& owner, KAutoSkillList which, EntityId self_key, EntityId other);
    // 0x080821C0: the launcher's on-cast map for a skill it just cast at `level`: every {skill,
    // percent} of it cast at the same level, target and spot
    void cast_on_cast_skills(KNpc& launcher, int skill_id, int level, const KCastParams& p);
    // 0x08081B70: the spot a knock back (or a dash) may reach on the way from the npc to `to`, in
    // steps of its step length; false when the way is no way (a step of 0, off the map, an unknown
    // barrier).  `distance` comes back as the way found, `fly` passes jump barriers and npcs.
    bool knock_back_free_spot(const KNpc& e, Pos& to, int& distance, bool fly) const;
    // KRegion::GetBarrier 0x080E0A30 through KSubWorld 0x080F0530: the barrier kind under a spot
    // (the low nibble of the cell; a diagonal cell - high nibble 2..5 - passes on one side of its
    // line by the offsets inside the cell), 0 when none, -1 off the map
    [[nodiscard]] int barrier_kind(Pos at) const noexcept;
    // KNpc::ProcessState 0x0808B610 every frame (the regeneration part every GAME_UPDATE_TIME
    // frames when `regen`): the poison ticks, the freeze, the stun, the potions, the states run
    // out.  True when the npc does nothing else this frame (stunned, or the odd frame of a freeze).
    bool process_frame_state(KNpc& e, bool regen);
    // 0x0808BFD4: once a second (m_LoopFrames % 18) the crowd block rate and the mana skill enhance
    void per_second_attribs(KNpc& e);
    // the skill a swing carries: the npc's active skill, a player's basic attack (1 melee / 2 ranged)
    [[nodiscard]] const KSkill* swing_skill(const KNpc& e);
    // KNpc::OnSkill: the cast at 60 % of the swing (the active skill on the target)
    void on_skill(KNpc& e, KNpc& target);

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
    // the frames of a swing: base x 100 / (100 + m_CurrentAttackSpeed), at least one
    [[nodiscard]] static std::uint32_t attack_length(const KNpc& e, std::uint32_t base) noexcept;
    // the player tables (KSubWorldConfig::player_set, or the defaults)
    [[nodiscard]] const KPlayerSet& tables() const noexcept;
    void approach(KNpc& e, const KNpc& target);
    bool check_hit_target(int ar, int df, int ignore = 0);   // KNpc::CheckHitTarget 0x0807ED60
    void check_trap(KNpc& e);                                // KNpc::CheckTrap (players, every frame with m_ProcessAI)
    void process_state(KNpc& e);                            // the every-GAME_UPDATE_TIME part of KNpc::ProcessState: the regeneration
    // KNpc::Init -> g_pNpcTemplate[id][level]: the level data of a template, computed once per (id, level, series)
    [[nodiscard]] const KNpcLevelData& level_data_of(const KNpcTemplate& t, int level, int series) const;
    void do_death(KNpc& e, EntityId killer);
    // the helpers of KNpc.cpp
    void add_five_resists(KNpc& e, int delta) noexcept;
    void refresh_state(KNpc& target, EntityId launcher, KStateNode& node, const KMagicAttrib* states, int count);
    bool apply_special_state(KNpc& target, int attrib_id);   // the state of [returnskill_p] / [ignoreskill_p] on oneself
    [[nodiscard]] const std::vector<int>* attrib_data(int attrib_id) const noexcept;   // attribconstdata.ini through the skill table
    [[nodiscard]] bool in_attrib_data(int attrib_id, int value, std::size_t from) const noexcept;
    [[nodiscard]] int count_npcs_within(const KNpc& e, int radius) const;   // 0x0807A0E0
    [[nodiscard]] int weapon_enhance_percent(const KNpc& e);   // KPlayer 0x080B0D50: addphysicsdamage_p of the weapon worn
    [[nodiscard]] int skill_list_level(const KNpc& e, int skill_id) const noexcept;   // KSkillList::GetLevel(list, id, 1) 0x080E4440
    void sync_life(const KNpc& e, int life_before, EntityId source);   // G2C_ENTITY_LIFE when the life moved
    void tick_state_skills(KNpc& e);   // the state list part of ProcessState (0x0808B8B6)
    // ---- the missiles (KMissle.cpp; docs/LINUX-SERVER.md §13) ----
    [[nodiscard]] int missle_cells_x() const noexcept;   // the map in 32-unit cells (the old regions x 16)
    [[nodiscard]] int missle_cells_y() const noexcept;
    [[nodiscard]] const KSkill* skill_of(int id, int level);   // 0x08076CE0: id 1..1999, level 1..63, instanced on first use
    [[nodiscard]] const KMissleTemplate* missle_template(int id) const noexcept;
    KMissle* missle_add(Pos at);                                  // KMissleSet::Add 0x08076F00
    void missle_remove(KMissle& m);                               // KMissleSet::Remove 0x08076E00 + 0x08074CF0
    bool missle_set_pos(KMissle& m, Pos p) noexcept;              // Mps2Map 0x080EF7F0 into the missile
    bool missle_set_pos_fine(KMissle& m, std::int64_t x1024, std::int64_t y1024) noexcept;   // the same in 1/1024 units (a npc's spot)
    void create_missle(const KSkill& skill, KNpc& launcher, int missle_id, KMissle& m);   // KSkill::CreateMissle 0x080EA310
    std::shared_ptr<const KMissleMagicAttribsList> missle_attribs(const KSkill& skill, KNpc& launcher);   // 0x080EA2D0
    // the per-missile part of the generators: the slot, CreateMissle, the target, the delay, the vector
    KMissle* missle_fire(const KSkill& skill, const KOrdinSkillParam& ctx, KNpc& launcher, Pos at, int dir, int dir_index, int i,
                         const std::shared_ptr<const KMissleMagicAttribsList>& list, const int* vector, bool vector_always);
    int cast_wall(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at);                    // 0x080EBB20
    int cast_line(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at);                    // 0x080EC2F0
    int cast_extractive_line_missle(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos from, int ux, int uy, Pos to);   // 0x080EBF00
    int cast_spread(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at);                  // 0x080EB150
    int cast_circle(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at);                  // 0x080EB720
    int cast_zone(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at);                    // 0x080EC690
    int cast_child_skill(const KSkill& skill, const KOrdinSkillParam& ctx, int px, int py, int i);      // 0x080EAFF0
    EntityId cast_target_position(const KCastParams& p, Pos& out) const;                                // 0x080EED70
    void missle_frame(KMissle& m);                    // 0x08076950: one missile's frame
    void missle_activate(KMissle& m);                 // KMissle::Activate 0x080760E0
    bool missle_prepare_fly(KMissle& m);              // PrePareFly 0x08076550
    int missle_on_fly(KMissle& m, int speed, bool check);   // OnFly 0x080758E0: 0 flew, 1 collided, 2 vanish
    bool missle_test_barrier(const KMissle& m) const noexcept;   // TestBarrier 0x080F05C0: Obstacle_Normal / Obstacle_Jump under it
    bool missle_check_beyond_region(KMissle& m, int dx, int dy);   // 0x08074F10: the move in 1/1024 units; false = off the map
    bool missle_relative_base(const KMissle& m, Pos& out) const;   // 0x08074D70: the anchor of RelativePosType
    int missle_check_collision(KMissle& m);           // CheckCollision 0x08075770: -1 the ground, 0 nothing, 1 something
    [[nodiscard]] bool missle_relation_ok(const KNpc& npc, const KNpc& launcher, int flags) const;   // the filter of 0x080F2280 / 0x080F2610 / 0x080E20B0
    EntityId npc_at_cell(int cx, int cy, const KNpc& launcher, int flags) const;      // 0x080E20B0
    EntityId first_npc_in_square(const KMissle& m, const KNpc& launcher, int range) const;   // 0x080748A0 + 0x080F2610
    EntityId missle_exact_target(const KMissle& m, const KNpc& launcher) const;       // 0x080749A0
    int missle_process_collision(KMissle& m, int range);   // 0x08075630: the blows within `range` cells, -1 when the interval is not due
    int missle_process_collision(KMissle& m);              // 0x08075710: the same with DmgRange, nothing for a ClientSend missile
    bool missle_process_damage(KMissle& m, KNpc& target);  // ProcessDamage 0x080753F0
    void missle_do_collision(KMissle& m);                  // DoCollision 0x08075340
    void missle_do_vanish(KMissle& m, bool event);         // DoVanish 0x08075210
    void missle_event(const KSkill& skill, int type, KMissle& m);   // OnMissleEvent 0x080EE810: 2 fly, 3 collide, 4 vanished
    void do_revive(KNpc& e);
    void revive(KNpc& e);
    void emit_action(const KNpc& e, pb::Action action, EntityId target);
    void emit_life(const KNpc& e, std::int32_t delta, EntityId source);
    // KPlayer::UpdataCurData for a player's npc after its equipment changed, then the sync
    void recalc_player(KNpc& e);
    // the experience of a dead npc to the players in its damage records (0x0809BDD0)
    void share_experience(KNpc& dead);
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
    std::unique_ptr<KLuaScript> gm_script_;                    // the state "?gm ds" code runs in (made on first use)
    std::unique_ptr<KSkillManager> skills_;                    // g_SkillManager: one per map (its Lua states are this map's)
    // g_MissleSet of this map: slot 0 is never used (a missile index of 0 means none, as in the
    // binary); a deque so that a cast during the missile frame never moves the missiles under it
    std::deque<KMissle> missles_;
    std::vector<int> free_missles_;
    std::size_t live_missles_ = 0;
    void load_items(std::uint64_t sid, const pb::RoleData& role);
    void save_items(std::uint64_t sid, pb::RoleData& out) const;
    void item_result(std::uint64_t sid, std::uint32_t seq, pb::Result result);
    void item_moved(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    void item_changed(std::uint64_t sid, std::uint32_t id);   // G2C_ITEM_ADD with the item as it is now (a stack changed)
    void send_money(std::uint64_t sid);
    // KItemList::EnoughAttrib needs the player's numbers
    [[nodiscard]] std::function<int(int)> attrib_of(const KNpc& e) const;
    void object_tick(KNpc& e, std::vector<EntityId>& expired);   // KObj::Activate for items and money
    void remove_object(EntityId id);
    [[nodiscard]] Pos free_object_pos(Pos at) const;              // KSubWorld::GetFreeObjPos
    std::unordered_map<std::uint64_t, KItem> ground_items_;       // object entity id -> the item lying there
    std::size_t ground_money_ = 0;
    // A death inside the tick loop cannot add entities (the table's vector would move under the
    // loop's references): what a monster drops waits here until the loop is over.
    struct KPendingDrop {
        std::optional<KItem> item;
        int money = 0;
        Pos at;
        std::uint64_t belong = 0;
    };
    std::vector<KPendingDrop> pending_drops_;
    void flush_pending_drops();
    std::uint32_t random_percent() { return static_cast<std::uint32_t>(rng_() % 100); }   // g_Random(100)
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
