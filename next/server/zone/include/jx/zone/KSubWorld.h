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
#include <array>
#include <vector>

#include <google/protobuf/message_lite.h>

#include "jx/client.pb.h"
#include "jx/common.pb.h"
#include "jx/role.pb.h"
#include "jx/core/FixedTick.h"
#include "jx/entity/EntityTable.h"
#include "jx/ids.hpp"
#include "jx/zone/KItem.h"
#include "jx/zone/KItemChangeRes.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KRegion.h"
#include "jx/zone/KFaction.h"
#include "jx/zone/KMission.h"
#include "jx/zone/KRevivePos.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KObj.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcGold.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KPathFinder.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KPlayerChat.h"
#include "jx/zone/KPlayerEvent.h"
#include "jx/zone/KPlayerTask.h"
#include "jx/zone/KTaskManager.h"
#include "jx/zone/KPlayerTeam.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KTabFile.h"
#include "jx/zone/KMissle.h"

namespace jx::zone {

struct KSubWorldConfig {
    std::uint32_t zone_id = 1;
    std::string name = "Test Field";
    std::uint32_t tick_hz = 20;
    // seconds added to the wall clock for the scripts: GetCurServerTime 0x08103800 / GetLocalDate 0x0812A140 of jx_linux_y
    // add [0x9789ee4] / [0x9789ee8] to time(0) once the clock flag [0x9789ee0] is set (one offset here)
    std::int64_t time_offset = 0;
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
    // the percent of a blow between players (and partners): KNpc::CalcDamage 0x0808A368 reads NpcSet+0x1490
    // ([0x8BADF50]), which KNpcSet::Init 0x080A0810 (0x080A0926) fills from \settings\npc\PKRate.ini [PK]
    // rate, default 20 - the reference server's file says 20 too (the other nine keys are the PK rules of B3c-4)
    int pk_damage_percent = 20;
    std::shared_ptr<const KMapData> map;  // optional: walkability + spawn + npcs override the fields above
    bool map_npcs = true;                // place the npcs listed in the map bundle
    bool spawn_from_config = false;      // keep spawn_point even when a map bundle has its own
    std::shared_ptr<const KNpcTemplateSet> templates;   // npcs.txt numbers (frames, life, damage, ai); optional
    // settings/npc/NpcGoldTemplate.txt (npc_gold.json of jxassets export-npc-gold): the gold monster kinds; optional -
    // without it nothing turns gold (SetGoldTypeAndBackData 0x0809D916 returns on an empty table)
    std::shared_ptr<const KNpcGoldTemplateSet> gold;
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
    std::shared_ptr<const KWeaponSkillTable> weapon_skills;   // the weapon -> physical skill table (KSkill.h); null = the basic attacks
    std::shared_ptr<const KAbradeRate> abrade_rate;           // AbradeRate.ini (KItem.h; jxassets export-abrade-rate); null = nothing wears
    std::shared_ptr<const KChatCostTable> chat_cost;          // chatcost.ini (KPlayerChat.h; jxassets export-chat-cost); null = every channel free
    std::shared_ptr<const KTaskDefTable> task_def;            // settings/task/player_task_def.txt (KPlayerTask.h; jxassets export-task-def): which task values the client is told; null = none
    std::shared_ptr<const KTaskManager> tasks;                // settings/task (KTaskManager.h; jxassets export-task-tables): the TASKSYS library of the scripts; null = the library answers nothing
    std::shared_ptr<const KKillEventTable> kill_events;      // settings/npc/player/event_killnpc.txt (KPlayerEvent.h; jxassets export-kill-events): what a kill counts; null = nothing
    std::shared_ptr<const KItemChangeRes> item_res;           // settings/item/*Res.txt (jxassets export-item-res); null = everyone keeps the bare look
    std::shared_ptr<const KRevivePosTable> revive_pos;      // revivepos.ini (jxassets export-revive-pos): the revive / reference points of every map; null = spawn points only
    // \settings\task\missions.txt + \settings\timertask.txt (jxassets export-missions): the script of a mission id and of a
    // timer id (docs/LINUX-SERVER.md §33); null = the missions open without scripts
    std::shared_ptr<const KMissionTable> missions;
    std::shared_ptr<const KFaction> faction;                // 门派设定.ini (jxassets export-faction): the eleven factions; null = no faction can be joined
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
    // a line spoken on a channel (docs §19): the GM filter first, then the rules of 0x081E3710 / 0x080502A0 and the
    // receivers of the channel; WORLD / CITY / FACTION lines wait in take_chat_broadcasts() for the server
    bool chat(std::uint64_t sid, std::string_view text, pb::ChatChannel channel = pb::CH_NEARBY, std::string_view target = {});
    std::vector<KChatBroadcast> take_chat_broadcasts();
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
    // C2G_ADD_SKILL_POINT: KPlayer::AddSkillPoint (jx_linux_y 0x080BD460) - the skill's level-up
    // script when it names one, else `points` levels against m_nSkillPoint, MaxLevel (+ a reborn
    // character's addon), the character's level (level + points <= level + 1 - ReqLevel) and the
    // reborn table; answered with G2C_SKILL_LEVEL where the old server sent its 0x5e packet
    bool add_skill_point_request(std::uint64_t sid, int skill_id, int points, std::uint32_t seq);
    // G2C_SKILL_LIST: every skill the character holds (s2c_synccurplayerskill), on entering the world
    void send_skill_list(std::uint64_t sid);
    // G2C_SKILL_LEVEL: the 0x5e packet {skill, level, m_nSkillPoint, exp in 1/1024} (a level of -1 = removed)
    void send_skill_level(std::uint64_t sid, int skill_id, int level, int exp_percent, std::uint32_t seq = 0, bool level_up = false);
    // KSkillList::AddSkillExp 0x080E5D90 with the player's part: the sync when the bar moved by
    // 8/1024 or the level, the skill's OnLevelUp script.  True when the level moved.
    bool give_skill_exp(KNpc& e, const KMagicAttrib& attrib, bool percent_mode);
    // KNpc 0x080847B0: the cool down of a skill just cast (TimePerCast of the level, less the
    // state modifier of skill_mintimepercast_v) starts at this frame
    void set_skill_cool_time(KNpc& e, int skill_id, int level);
    // KPlayer::ForbitSkill 0x080B2950 / SetAForbitSkill 0x080AE9E0: every skill / one skill locked
    // or freed, the client told (G2C_SKILL_FORBID, the 0x63 packet)
    void forbit_skill(KNpc& e, bool forbid);
    // KNpc::SetAura 0x08087290: an IsAura skill held (its current level 1..63) becomes the aura +0x244 and puts its
    // StateSpecialId icon up; anything else clears the aura (and marks the icons 2).  The client asks through
    // C2G_SET_AURA (0x080DC460, cell 111: ForbitAura makes it a clear); Lua SetNpcAuraSkill and a template's Skill5 too
    void set_aura(KNpc& e, int skill_id);
    void set_aura_request(std::uint64_t sid, int skill_id);
    // 0x080873B0(npc, skill, level): the 0x85 packet of the child skill to the players around (a player's only when
    // sync_aura; a hidden npc none), then an IsAura skill casts its child (InstanceSkill(child, level)) at the npc's spot
    void cast_skill_effect(KNpc& e, int skill_id, int level);
    // KNpc 0x08087160: the six icons rebuilt (state_flag 2 -> 1): a player's Player+0x7dec icon at priority 100, the
    // aura's StateSpecialId, then every state's; 0x08079F60: the 0x7a packet of the icons to the players around
    void rebuild_state_icons(KNpc& e);
    void emit_state_icons(const KNpc& e);
    void set_a_forbit_skill(KNpc& e, int skill_id, int forbid);
    // what KSkillList needs of the world for this npc: the skill manager, its level, the cast of a
    // passive skill on itself (Cast(sk, idx, -1, idx, 0, 0, 1)) and the removal of a state
    [[nodiscard]] KSkillListHost skill_host(KNpc& e);
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
    void lose_treasure(KNpc& dead, EntityId killer, EntityId best);
    void lose_treasure_shared(KNpc& dead, const KNpc& killer, const KNpcDropRate& table);   // 0x080843A0: [Main] IsTeamShare
    // KNpc::LoseSingleItem -> GenRandomItem of jx_linux_y (0x08083BB0): one roll of a drop table
    std::optional<KItem> gen_random_item(const KNpcDropRate& table, int npc_level, int npc_series, int luck);
    // Give an item to a player (a script, a drop picked up, a quest reward): into the bag, onto a
    // stack where it can; the client is told.  0 when it does not fit.
    std::uint32_t give_item(std::uint64_t sid, KItem item);
    bool take_item(std::uint64_t sid, std::uint32_t id);   // the item is gone (used up, taken by a script)
    // Lua SyncItem 0x08114EF0 -> KItemList::SyncItem 0x081FB9A0: the item as it is now to its owner (G2C_ITEM_ADD)
    void sync_item(std::uint64_t sid, std::uint32_t id);
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
    // level / series above 0 / -1 override the template's (KNpcSet::Add 0x0813A770 packs template << 16 | level and
    // hands the series on: the create-npc skill's npc takes the attribute's level and the skill's Series)
    // 0x08085250 (after KNpcSet::Add for a placement / the script's AddNpc / a bKind Add): cell 5 = the template's aura
    // (AuraSkillId at its level), cell 6 = its passive skill when the row is style 3, cast on the npc itself at once
    void init_template_skills(KNpc& e);
    EntityId spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius = 0,
                       KNpcKind kind = KNpcKind::npc, std::uint32_t level = 0, int series = -1, int boss_flag = 0);
    // KNpcGold::SetGoldTypeAndBackData 0x0809D8D0 (KNpcGold.h): a backed-up (is_gold), not yet gold npc turns gold
    // when g_Random(1 000 000) < rate and the table is not empty - `type` 1..count-1 picks that row, anything else
    // the map's GoldenType, else a random row; the row's skill (through GetNpcLevelData "Level5") into cell 5 and
    // set as the aura, the numbers changed (gold_apply), the 0x9a packet {npc, kind} to the players around.
    // Returns true when it turned gold.
    bool set_gold_type(KNpc& e, int rate, int type);
    // Lua AddNpc(.., 2) 0x0811BE2B: BackData 0x0809D560 then SetGoldTypeAndBackData with rate 1 000 000 - a gold npc for sure
    void make_gold_npc(KNpc& e);
    // KNpcGold::RecoverBackData 0x0809E070: the backup back, cell 5 taken out (0x080E52D0), the aura cleared (SetAura 0)
    void recover_gold(KNpc& e);
    // KPlayer::UpdataCurData 0x080AF550 for a player's npc (a piece put on / taken off, a point spent): ClearAttrib, the
    // points, ReCalcStateEffect 0x0807D270 (every held state applied again), ReCalcEquip 0x080AF3E0, then the sync
    void recalc_player(KNpc& e);
    // stamina.ini's run cost / threshold of a player by its PK state (0x0808BE0B .. 0x0808BE76, 0x08080C5F .. 0x08080CCA):
    // 0 ExerciseRunSub, 1 FightRunSub, 2 and anything else KillRunSub
    [[nodiscard]] int run_stamina_sub(const KNpc& e) const noexcept;
    // the units a player covers in a second: m_CurrentRunSpeed a frame (0x08080C01 -> 0x08080900) while the stamina reaches the
    // run cost, else m_CurrentWalkSpeed a frame (0x08080C86: DoWalk on foot 0x0807B430, the step 0x08080B70)
    [[nodiscard]] std::uint32_t player_move_speed(const KNpc& e) const noexcept;
    // the 0x9a packet {0x9a, npc id, word kind} (0x0809DF66, 0x0807A870 within 100): G2C_NPC_GOLD
    void emit_gold(const KNpc& e);
    [[nodiscard]] const KMapSettings& map_settings() const noexcept;
    // 0x080EFBE0: the map's NpcSeriesAuto roll - g_Random(total) against the summed weights (metal 0 .. earth 4); 0 without
    // the flag, a total of 1 or less, or a roll below the first sum (a zero metal weight and a roll of 0 give metal all the same)
    [[nodiscard]] int random_series();
    // 0x080EFB90: the map's NpcAutoLevel roll - g_Random(max + 1 - min) + min; max == min -> max; 1 without the flag
    [[nodiscard]] int random_level();
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
    // KPlayer::Earn 0x080AAED0 -> KItemList::Earn(room 0, n) 0x081FC990: n < 0 refused, the room's sum may not turn
    // negative (KRoom 0x081F8B10), then the 0x61 money sync (0x081FAFD0) - events.lua OnRoomMoneyChange(room, money) of
    // the binary is not called (the event system is not ported).  KPlayer::Pay 0x080A9450 -> KItemList::Pay 0x081FC940:
    // more than the bag holds -> 0.  cash = Player+0x508c, the bag's money.
    bool earn(std::uint64_t sid, int n);
    bool pay(std::uint64_t sid, int n);
    [[nodiscard]] int cash(std::uint64_t sid) const;
    // Msg2Player: one line in the player's chat window.
    void msg_to_player(std::uint64_t sid, std::string_view text);
    // KPlayer::ExecuteScript: runs fn(param) of the script (a game path) for the player.
    bool execute_script(const std::string& game_path, const char* fn, KNpc& player, int param = 0);
    // the same with any arguments (CallFunction(fn, 0, "dddddddd", ...) of 0x080AD300)
    bool execute_script_args(const std::string& game_path, const char* fn, KNpc& player, const std::vector<KLuaScript::Arg>& args);

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
    // KPlayerSet::SearchPlayer 0x080C6010: the player on this map whose name is exactly this (the old set keeps a
    // name -> index tree); nullptr for an empty name or nobody
    [[nodiscard]] const KNpc* find_player_by_name(std::string_view name) const;
    // KSubWorld 0x080EFEE0(name): the first npc of this map (its region lists, players skipped) whose name is exactly this;
    // nullptr for an empty name or none
    [[nodiscard]] const KNpc* find_npc_by_name(std::string_view name) const;
    // the 0x3e9 node of Lua DelNpc 0x08107536: the npc goes at the map's next frame, not inside the script that asked
    void queue_remove_npc(EntityId id);
    // the mission values of the map (KSubWorld+0x484b8.. of jx_linux_y: 100 ints the scripts share through GetMissionV /
    // SetMissionV; in memory only, like the old server's)
    static constexpr int kMissionValues = 100;
    [[nodiscard]] int mission_value(int idx) const noexcept;
    void set_mission_value(int idx, int value) noexcept;
    // the mission strings of the map (SubWorld+0x48648.., 100 of 0xc8 bytes: GetMissionS / SetMissionS, idx 1..100)
    static constexpr int kMissionStrings = 100;
    [[nodiscard]] const std::string& mission_string(int idx) const noexcept;
    void set_mission_string(int idx, std::string_view text);
    // a system line (CH_SYSTEM) to every player of this map: Lua Msg2SubWorld 0x08105170 (kind 0 of 0x081C9220 reaches every
    // player of the old process) and Msg2Map 0x08105080 for this map
    void msg_to_all(std::string_view text);
    // the missions of this map (KMissionArray of 2003, SubWorld+0x60.. of jx_linux_y; docs §33).  Every call into a mission
    // script has no player behind it (KMission::ExecuteScript / 0x082095A0)
    [[nodiscard]] KMission* find_mission(int id);
    [[nodiscard]] const KMission* find_mission(int id) const;
    KMission* open_mission(int id);                        // OpenMission 0x081332F0: made (when new), InitMission of its script
    bool run_mission(int id);                              // RunMission 0x08132E50: RunMission of the script
    bool close_mission(int id);                            // CloseMission 0x081327E0: EndMission, StopMission, removed
    bool join_mission(int id, const KNpc& player, int group);   // JoinMission 0x08137E40: JoinMission(player, group) of the script
    bool mission_remove_player(int id, EntityId player);   // RemovePlayer + OnLeave(player) of the script (2003 KMission)
    void mission_message(const KMission& m, int group, std::string_view text);   // Msg2All (group 0) / Msg2Group: a system line each
    // a function of a script with no player (KMission::ExecuteScript 2003: only the SubWorld global set)
    bool execute_script_world(const std::string& game_path, const char* fn, const std::vector<KLuaScript::Arg>& args);
    // the script timers of AddTimer 0x08100D40 (docs §34): fn(param, id) of the script that armed it runs after `frames`;
    // one number back = the next period (0 = done), two = the period and the new param, none = done (0x081CC300).  0 = not made
    std::uint32_t add_script_timer(std::uint64_t frames, const std::string& script, const std::string& fn, int param);
    bool del_script_timer(std::uint32_t id);   // DelTimer 0x08100CA0: true when it was there
    [[nodiscard]] std::size_t script_timer_count() const noexcept { return script_timers_.size(); }
    // AddStatData 0x080FF550 -> 0x081D0420(0x978c0a0, name, n, 0): the server's statistics counters (a map here)
    void add_stat_data(const std::string& name, int n);
    [[nodiscard]] long long stat_data(const std::string& name) const;
    // KNpc 0x0807B2D0 (Lua SetTmpCamp): +0x1900 = camp, then the 0xd1 packet {npc, camp} - to the player itself when the npc is
    // one (0x0807B294), else to the watchers (0x0807B2C2); EntityCamp.tmp_camp here
    void set_tmp_camp(KNpc& e, int camp);
    // AddGlobalNews 0x08125A90 / AddGlobalCountNews 0x081258E0: the 0x63 packet (ui 5) through the relay to every server
    // (0x08077560) - here a chat broadcast with the script action as its payload: the server sends it to every session
    void broadcast_script_action(int ui_id, std::string_view text, int text_id, int param, int count);
    // the 0x63 packet to one player (KPlayer 0x080A8510): Say / Talk / Describe / GiveItemUI / PutMessage build on it
    void send_script_action(const KNpc& e, int ui_id, std::string_view text, int text_id, const std::vector<std::string>& options, int param,
                            bool interactive, bool notify = false);
    // AddMapTrap 0x08102700 -> 0x080EFCF0 -> KRegion::AddTrap 0x080E1260: a trap cell a script added (world units in), checked
    // after the map's own; the same cell again replaces its script
    bool add_script_trap(Pos at, const std::string& script);
    [[nodiscard]] std::size_t script_trap_count() const noexcept { return script_traps_.size(); }
    // NpcChat 0x0812E5A0 -> 0x081C9380: the 0xfb packet {npc, text} to the watchers (NpcChat here), at once or after `frames`
    // (0x08139430: only while the npc is still there)
    void emit_npc_chat(const KNpc& e, std::string_view text);
    void npc_chat_later(const KNpc& e, std::string_view text, std::uint64_t frames);
    [[nodiscard]] std::size_t pending_npc_chats() const noexcept { return npc_chats_.size(); }
    // SaveNow / SaveQuickly (Player+0x244 / +0x248): the sessions whose character asked to be saved this frame
    std::vector<std::uint64_t> take_save_requests();
    // the maps this zone hosts (KSubWorldSet of the old server): SubWorldID2Idx / SubWorldIdx2ID answer from it; empty =
    // only this map
    void set_hosted_maps(std::vector<std::uint32_t> maps) { hosted_maps_ = std::move(maps); }
    [[nodiscard]] bool hosts_map(std::uint32_t map) const noexcept;
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
    // ---- the do_skill command of a npc (docs/LINUX-SERVER.md §16) ----
    // C2G_CAST_SKILL, the NpcSkillCommand handler 0x080DD130: a skill of 1..1999 that is not an
    // aura, at a target (target set, p1 = -1) or at a spot (x, y); queued through send_command
    // and, unlike the binary's next-frame pick-up, worked off at once
    bool cast_skill_request(std::uint64_t sid, int skill_id, int p1, int p2, EntityId target, std::uint32_t seq);
    // KNpc::SendCommand(do_skill, ...) 0x0809B750: only for a skill the list holds, five at most
    bool send_command(KNpc& e, int skill_id, int p1, int p2, EntityId target);
    // KNpc::ProcessCommand 0x0809B9E0 (+ 0x0809B510 aging): the first command checked, cast when
    // the target is within the active skill's radius, walked toward within 300 more, dropped beyond
    void process_command(KNpc& e);
    // 0x0809B840: 0 = cast now, 1 = the npc is busy (wait), 2 = drop the command
    int check_command(KNpc& e, KNpcCommand& c);
    // KNpc::CastSkill 0x08088350: the checks of the current skill, its cost, the sync to the
    // clients around, then do_skill; false when refused (the npc stands)
    bool cast_skill(KNpc& e, int p1, int p2, EntityId target);
    // KNpc::DoSkill 0x08088150: the action (do_magic 6 / do_attack 7) with its frames from the
    // attack / cast speed; the cast itself comes at 60 % of them (on_skill)
    bool do_skill(KNpc& e, const KSkill& sk, int p1, int p2, EntityId target);
    // 0x08085020, the fire: the current skill on the kept target / spot, then its cool down
    void on_skill(KNpc& e);
    // KSkill::CanCastSkill 0x080E8AE0 (docs §14): the target rules, a player's weapon / horse
    // limits, the reach by style; may turn a spot cast of a self skill into one on oneself
    bool can_cast_skill(const KSkill& sk, KNpc& launcher, int& p1, int& p2, EntityId& target);
    // KNpc 0x08079A90: the physical skill of the worn weapon through the table (basic attacks
    // without one); 0x080E8C05: the EqtLimit the weapon answers to
    [[nodiscard]] int weapon_physics_skill(const KNpc& e);
    [[nodiscard]] int weapon_eqt_limit(const KNpc& e);
    // 0x08078B10: mana (0) / stamina (1) / life (2) of a player against the cost; check only or paid
    bool cost_skill(KNpc& e, int type, int cost, bool check_only);
    // 0x08086D90 KNpc::SetActiveSkill(cell): m_ActiveSkillID and m_CurrentAttackRadius from the cell
    bool set_active_skill(KNpc& e, int slot);
    // 0x080848B0 KNpc::GetCurrentSkill: m_ActiveSkillID at its current level in the list
    [[nodiscard]] const KSkill* current_skill(KNpc& e);
    // GetSkill + InstanceSkill; without a table the built-in basic attacks (a plain blow for an unknown id)
    [[nodiscard]] const KSkill* skill_instance(int id, int level);
    static constexpr int kCommandSkill = 5;          // do_skill
    static constexpr int kCommandApproach = 300;     // 0x0809BAFF: walked toward within radius + 300
    // ---- hiding: [hide] 200 of a state (docs/LINUX-SERVER.md §16.1) ----
    // KNpc::IsInvisibleTo 0x08079200: hidden, the npc is seen by its own client only
    [[nodiscard]] bool invisible_to(const KNpc& e, EntityId viewer) const noexcept;
    // KNpc::SetHide 0x0807FF80: the players around forget it (the 0x4f packet) / look at it again
    void set_hide(KNpc& e, int value);
    // 0x0807D4C0: the hiding breaks - the state skills of [hide] Data1.. come off
    void break_hide(KNpc& e);
    // ---- the moves of a style-1 skill: KNpc 0x08087F70, MisslesForm 8..13 (docs/LINUX-SERVER.md §16.2) ----
    // DoSkill's style-1 branch: the move of the form set going (false: no such form, or no way)
    bool do_special_skill(KNpc& e, const KSkill& sk);
    static constexpr int kJumpSteps = 40;          // +0x12a0 (KNpc::Init 0x0807DDD4): a jump reaches 40 steps of the step length at most
    static constexpr int kMoveMinDistance = 20;    // 0x08087D3E / 0x08084D43: a jump or a blink shorter than this is no move
    // ---- the wear of the worn pieces: KItemList 0x08201940 (docs/LINUX-SERVER.md §16.3) ----
    // every worn piece but the mask rolls its wear for `mode` (0 an attack, 1 a hit taken, 2 a step):
    // a point lost is synced, a piece at 0 turns broken and goes into the bag (or is destroyed)
    void abrade_equipments(KNpc& e, int mode);
    // 0x08201D90(list, n): every worn piece but the mask loses n percent (the death penalty, KPlayer 0x080B9FA0)
    void abrade_equipments_percent(KNpc& e, int percent);
    // ---- a player's death and revive: KNpc 0x08089920 / DoDeath 0x080896C0 / OnDeath 0x08088D50, KNpc::Revive
    //      0x080833B0, KPlayer::Revive 0x080AD9F0 (docs/LINUX-SERVER.md §16.4) ----
    // C2G_REVIVE (the handler slot 118 -> 0x080AEBC0(player, 12), KPlayer::Revive(0)): back at the revive point
    bool revive_request(std::uint64_t sid, std::uint32_t seq);
    // KPlayer::AddExp 0x080B00C0 on a player (the level-difference rule of a kill; Lua AddExp 0x0811A140): the
    // experience, a level up with its life sync, the attributes, the passives that open at the new level
    void give_player_exp(KNpc& e, int exp, int npc_level);
    // Lua AddOwnExp 0x081126C0 -> KPlayer 0x080AFEA0: the experience as given, without CalcExp or the bonuses
    void give_player_exp_direct(KNpc& e, std::int64_t exp);
    // KSkill 0x080E8770 - style 4, the npc a skill makes (docs §16.5); the spawn itself waits for the end of the tick
    bool cast_create_npc(const KSkill& sk, KNpc& launcher, const KCastParams& p);
    // KNpc::SetHorse 0x0807D520 (docs §16.6): nothing while frozen_action; mounting while hidden breaks the hiding
    void set_horse(KNpc& e, int n);
    // the ride toggle 0x080AEFA0 (C2G_RIDE): true when the state changed
    bool ride_request(std::uint64_t sid, bool on, std::uint32_t seq);
    // the 0x71 packet (handler 0x080DC300): the script event 2, riding -> refused, then 0x08078AA0(npc, sit ? 8 : 1) - refused
    // while frozen_action (+0x1479, the mask 0x11e covers 1 and 8); the action runs at 0x08088640: 8 -> KNpc::DoSit 0x0807B550,
    // 1 -> DoStand 0x08080030
    bool sit_request(std::uint64_t sid, bool sit, std::uint32_t seq);
    // ---- teams (KPlayerTeam.h; docs/LINUX-SERVER.md §17; KSubWorldTeam.cpp) ----
    // the 0x53 packet (sub-command 1..11, a npc, a flag) -> the KPlayer / KPlayerTeam handlers of jx_linux_y
    bool team_request(std::uint64_t sid, int cmd, EntityId target, int flag);
    [[nodiscard]] const KTeam* team_of(const KNpc& e) const noexcept;
    [[nodiscard]] KTeam* team_of(const KNpc& e) noexcept;
    [[nodiscard]] const KTeamSet& teams() const noexcept { return teams_; }
    [[nodiscard]] KTeam* mutable_team(int id) noexcept { return teams_.get(id); }   // the script api (ChangeTeamFeature)
    // KTeam::CalcCaptainPower 0x080CC960 / CheckFull 0x080CC990
    [[nodiscard]] int team_members_max(const KTeam& t) const noexcept;
    [[nodiscard]] bool team_full(const KTeam& t) const noexcept;
    // 0x080CC620(team, player): the captain and members within reach of a player (the death exp loss, the luck of a drop)
    [[nodiscard]] int team_near_count(const KNpc& e) const noexcept;
    [[nodiscard]] bool team_may_take(const KNpc& e, const KNpc& object) const noexcept;   // ServerPickUpItem 0x080B826C: a team mate's drop
    void team_leave(KNpc& e);                 // KPlayer::LeaveTeam 0x080B7C60 (a player leaving the world too, 0x080C55D7)
    void team_send_self(const KNpc& e);       // KPlayer::SendSelfTeamInfo 0x080AA7F0
    // KPlayer::AddExpTeam 0x080B03E0: a kill's experience shared with the team mates within 1024 units on the same map
    void add_exp_team(KNpc& anchor, int exp, int npc_level, EntityId killer);
    // the trade and the sign over the head (docs/LINUX-SERVER.md §18): the client's C2G_TRADE (KSubWorldTrade.cpp)
    bool trade_request(std::uint64_t sid, int cmd, EntityId target, int arg, std::string_view text);
    // the npc dialog (docs §20): the 0x6e packet - the npc's script main() for the player when it is a dialoger (or at
    // peace) within twice its dialog radius; the 0x5f packet - the answer runs the function of that answer in the script
    bool dialog_npc_request(std::uint64_t sid, EntityId npc);
    bool dialog_answer(std::uint64_t sid, int index, int kind);
    // Lua Say / Talk: the 0x63 packet to the player and the answer functions kept on it
    void dialog_say(KNpc& e, std::string_view text, int text_id, const std::vector<std::string>& answers);
    void dialog_talk(KNpc& e, std::string_view callback, const std::vector<std::string>& pages);
    // Lua Describe 0x081242A0: Say's shape with the ui id 12 (the client's npc description window), the answers within 0x1f4 bytes
    void dialog_describe(KNpc& e, std::string_view text, int text_id, const std::vector<std::string>& answers);
    // Lua TaskTip 0x08122730: the 0xb6 packet {0xb6, 0x10, text} of 0x40 bytes -> G2C_TASK_TIP (the client's system message pane)
    void task_tip(KNpc& e, std::string_view text, int kind = 0);   // kind 1 = SendTaskOrder (the 0xb6 packet without the 0x10 byte)
    // Lua GiveItemUI 0x0812BBA0: the 0x63 packet with the ui id 0xb (the give-item box of the client), the confirm / cancel
    // functions as the two answers, the sixth argument's function for the box's changes, the player's box lists cleared
    void dialog_give_item_ui(KNpc& e, std::string_view title, std::string_view content, int text_id, std::string_view confirm_fun,
                             std::string_view cancel_fun, int param, std::string_view select_fun, bool notify);
    // the 0x89 packet (0x080DB680 -> 0x080AC560): the pieces of the box checked and kept (0x080AB980), then kind 0 -> the
    // sixth argument's function in the npc's script with the count (0x080AC400), else the confirm function of the dialog
    // script with the count (0x080AC4B0, the answers cleared first)
    bool give_items_request(std::uint64_t sid, int kind, const std::vector<KGiveItemEntry>& entries);
    // SetUiGiveItemMsg 0x0810B020 (kind 0, the 0xd8 packet) / SetUiGiveItemMoreConfirmMsg 0x0810AF50 (kind 1, 0xdf)
    void give_item_msg(KNpc& e, int kind, std::string_view text);
    // Lua AddNote 0x08124DC0: the 0x63 packet with the ui id 3 (UI_NOTEINFO) - a line for the client's journal with a number
    void dialog_add_note(KNpc& e, std::string_view text, int text_id, int param);
    // Lua AskClientForNumber 0x08115CA0 / AskClientForString 0x08115E90: the 0xa3 packet (G2C_SCRIPT_ASK) and the function
    // kept as the first answer; kind 1 = a number (the number pad), 0 = a string (the input box with the default)
    void dialog_ask_client(KNpc& e, int kind, std::string_view fn, int min, int max, std::string_view title, std::string_view default_text);
    // the 0x82 packet (0x080DB490 -> 0x081F6830): kind 3 -> the answer function with the number (0x081F6680), kind 2 -> with the
    // text (0x081F6730, an empty one dropped); the answers are cleared first (0x080A7E60); anything else is ignored
    bool script_input_request(std::uint64_t sid, int kind, std::int64_t number, std::string_view text);
    // the task values (docs §21, KSubWorldTask.cpp): KPlayer::SetTaskValue 0x080A9190 (a change; a SYNC_FLAG id with `sync` goes to
    // the client as G2C_TASK_VALUE), the 0xa7 packet of one id (0x080A8CC0), SyncTaskValueMore 0x080A9550 (G2C_TASK_VALUES of
    // eighty), the enter-world sync 0x080B9CF0, the client's 0xaa packet 0x080DB070 (a CLIENT_FLAG id only)
    void task_set_value(KNpc& e, int id, int value, bool sync);
    void task_send_value(const KNpc& e, int id);
    bool task_sync_more(const KNpc& e, int first, int last, bool only_non_zero);
    void task_login_sync(const KNpc& e);
    bool task_value_request(std::uint64_t sid, int id, int value);
    // the task system of the scripts (docs §22, KSubWorldTaskSys.cpp): the status bits and the temp values of a task in the
    // task values, written back through 0x0820E1E0 (a change goes to the client as G2C_TASK_VALUE whatever the table says)
    void task_set_value_synced(KNpc& e, int id, int value);
    void task_write_temp(KNpc& e, const task_status::KTaskTemp& temp);
    [[nodiscard]] std::optional<int> task_status(const KNpc& e, std::string_view name) const;   // GetTaskStatus 0x0820E800
    bool task_set_status(KNpc& e, std::string_view name, int status);                           // SetTaskStatus 0x0820E720
    bool task_start(KNpc& e, std::string_view name);                                            // StartTask 0x0820E4E0
    bool task_close(KNpc& e, std::string_view name);                                            // CloseTask 0x0820E430
    [[nodiscard]] std::optional<int> task_temp(const KNpc& e, std::string_view name, std::string_view key) const;   // GetTmpValue 0x0820DF10
    bool task_set_temp(KNpc& e, std::string_view name, std::string_view key, int value);       // SetTmpValue 0x0820E5C0
    const char* task_first(KNpc& e);                                                            // FirstTask 0x08174E30
    const char* task_next(KNpc& e);                                                             // NextTask 0x08174D40
    bool task_select(KNpc& e, const char* fn, int task_id);                                     // SelectTaskStart / Finish / Award
    // the kill events (docs §23, KSubWorldEvent.cpp): AddPlayerEvent 0x081560C0 / RemovePlayerEvent 0x08156050 on the player,
    // the walk of the killer's events at the end of a npc's death frames (0x08083720 -> 0x08155F80 -> 0x08156330)
    bool player_event_add(KNpc& e, int id);
    bool player_event_remove(KNpc& e, int id);
    void fire_kill_events(const KNpc& dead);
    [[nodiscard]] int npc_power(const KNpc& e) const noexcept;   // 0x08079750: 0 player, 1 npc, 2 gold, 3 boss
    [[nodiscard]] bool trading(const KNpc& e) const noexcept;   // KPlayer::CheckTrading 0x080A7E90
    void trade_cancel(KNpc& e);                                 // 0x080AE380: both sides, the boxes back, the menu states restored
    void set_menu_state(KNpc& e, int state, std::string_view sentence, EntityId dest);   // KPlayerMenuState::SetState 0x080C29D0
    void restore_menu_state(KNpc& e);                                                    // 0x080C2ED0
    void sys_msg(std::uint64_t sid, int id, EntityId who = EntityId{});                  // the 0x86 packet {8, id, npc}
    // KPlayerPK (Player+0x5a50): SetPKState 0x080C3740, SetPKValue 0x080C38C0, AddPKValue 0x080C3930, the packet 0x76 handler 0x080DBE00
    bool pk_set_state(KNpc& e, int state, bool force);
    void pk_set_value(KNpc& e, int value);
    void pk_add_value(KNpc& e, int add);
    bool pk_state_request(std::uint64_t sid, int state);
    void emit_pk(const KNpc& e);   // G2C_ENTITY_PK: the state & 3 of the 0x4a / 0x4b sync for the watchers
    [[nodiscard]] const KNpc* owner_of(EntityId id) const;       // 0x08078E80: a companion's master, else the npc itself
    // KNpc::GetPKRelation 0x0807A350 (KNpc::DeathCalcPKValue of 2003): the death mode 0..4 and the PK points the killer's owner gains
    int death_calc_pk_value(const KNpc& victim, const KNpc* killer_owner, const KNpc* victim_owner, int& points) const;
    void death_punish_pk(KNpc& e, EntityId killer);              // KNpc::DeathPunish 0x080B9FA0 for a player's kill
    [[nodiscard]] int exp_percent(const KNpc& e) const noexcept; // 0x080A8120: the exp held, in percent of the level's
    // KNpc::DoSit 0x0807B550: already sitting -> nothing; a run attack (0x12) is ended first; m_Doing = 8, the 0x83 packet
    // {npc id} to the players around and the 0x9f {6, 1} to oneself (0x080796D0), the frame counter 0 / m_SitFrame
    void do_sit(KNpc& e);
    void leave_sit(KNpc& e);
    // C2G_SKILL_DESC: the numbers of a skill level for its tip (KSkill::GetDesc 0x006FBC90 of the 2.0 client; docs/CLIENT-2.0.md §10)
    void skill_desc_request(std::uint64_t sid, int skill_id, int level);
    // KPlayer::SetFaction 0x080AEEC0 (docs §16.7): the faction named `name` joined - its camp on the npc, the 0x7b packet;
    // false when the table has no such name or refuses the character's series
    bool set_faction(KNpc& e, std::string_view name);
    // KPlayer::ClearFaction 0x080AEDE0: the current faction becomes -1, the camp C_FREE, the 0x7c packet
    void clear_faction(KNpc& e);
    // KNpc::SetCamp 0x0807B7B0 / SetCurrentCamp 0x0807B850: m_Camp / m_CurrentCamp and the 0x59 / 0x58 packet around
    void set_camp(KNpc& e, int camp);
    void set_current_camp(KNpc& e, int camp);
    [[nodiscard]] const KFaction* faction_table() const noexcept { return cfg_.faction.get(); }
    // KSubWorldSet 0x080F6D20: the revive / reference point `ref` of `map` in absolute Mps (revivepos.ini), nothing
    // without the table or the point
    [[nodiscard]] std::optional<Pos> revive_point(std::uint32_t map, int ref) const noexcept;
    // KPlayer::Revive(type, force): 0 at the revive point (fight mode off), 1 where it lies in fight mode, 2 where
    // it lies out of it; a character that is not dead is stood up instead (unless forced)
    bool player_revive(KNpc& e, int type, bool force);

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
    // ---- teams (KSubWorldTeam.cpp)
    KTeamSet teams_;
    bool team_create(KNpc& e);                                   // KPlayerTeam::CreateTeam 0x080CE3C0
    bool team_set_state(KNpc& e, bool open);                     // KPlayer::SetTeamState 0x080B1CF0
    bool team_set_open(KTeam& t);                                // KTeam::SetTeamOpen 0x080CD960
    bool team_set_close(KTeam& t);                               // KTeam::SetTeamClose 0x080CCA80
    bool team_add_member(KTeam& t, KNpc& p);                     // KTeam::AddMember 0x080CC9D0
    void team_delete_member(KTeam& t, KNpc& p);                  // KTeam::DeleteMember 0x080CD5E0
    void team_hand_over(KTeam& t);                               // 0x080CD480: the captaincy to the first member of the captain's camp
    void team_apply_add(KNpc& e, EntityId target);               // KPlayer::S2CSendAddTeamInfo 0x080B8000
    bool team_accept(KNpc& e, EntityId target);                  // KPlayer::AddTeamMember 0x080B75B0
    void team_kick(KNpc& e, EntityId target);                    // KPlayer::TeamKickOne 0x080B9880
    void team_change_captain(KNpc& e, EntityId target);          // KPlayer::TeamChangeCaptain 0x080B9400
    void team_dismiss(KNpc& e);                                  // KPlayer::TeamDismiss 0x080B7DE0
    void team_invite(KNpc& e, EntityId target);                  // KPlayerTeam::InviteAdd 0x080CE150
    void team_reply_invite(KNpc& e, EntityId captain, bool ok);  // KPlayerTeam::GetInviteReply 0x080CCBA0
    void team_info(KNpc& e, EntityId target);                    // KPlayer::S2CSendTeamInfo 0x080B1AD0
    void team_join(KTeam& t, int id, KNpc& newcomer, KNpc& captain);   // the common tail of AddTeamMember / GetInviteReply
    void team_event(std::uint64_t sid, pb::TeamEventKind kind, EntityId who = EntityId{}, int arg = 0);
    void team_event_all(const KTeam& t, pb::TeamEventKind kind, EntityId who = EntityId{}, int arg = 0);
    void team_send_self_all(const KTeam& t);
    void team_sync_captain(KTeam& t);   // every member's KPlayerTeam::captain_npc = the captain's npc
    // the trade (KSubWorldTrade.cpp)
    bool trade_apply_open(KNpc& e, std::string_view sentence);   // KPlayer::TradeApplyOpen 0x080AE590
    bool trade_apply_close(KNpc& e);                             // the 0x6a packet 0x080AE320
    bool trade_apply_start(KNpc& e, EntityId target);           // the 0x6b packet 0x080B4DE0
    bool trade_reply(KNpc& e, EntityId applicant, bool accept); // c2sTradeReplyStart 0x080BAFD0
    bool trade_money(KNpc& e, int money);                        // the 0x6c packet 0x080AE510
    bool trade_decision(KNpc& e, int decision);                  // the 0x6d packet 0x080B2C70
    bool trade_exchange(KNpc& e, KNpc& p);                       // 0x080B2EC7..: the second ok
    void trade_sync(KNpc& e);                                    // KPlayer::SyncTradeState 0x080A85B0
    void trade_item_sync(const KNpc& e, std::uint32_t id, bool removed);   // ExchangeItem 0x08207172: the partner sees my box
    void trade_clear_box(KNpc& e);                               // 0x081FC8D0(list, 2): the box back into the bag
    [[nodiscard]] KNpc* trade_partner(const KNpc& e);
    void emit_menu_state(const KNpc& e, EntityId dest);
    [[nodiscard]] KNpc* team_player(std::uint64_t sid);
    [[nodiscard]] KNpc* find_around_player(const KNpc& e, EntityId npc);   // KPlayer::FindAroundPlayer 0x080B1610: a player in the regions around

    void emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    // A crowded spot can put hundreds of entities in one EntitySpawn, which would pass the 64 KiB
    // the client protocol allows (docs/PROTOCOL.md).  This sends them in pieces that always fit.
    void emit_spawn(const std::vector<std::uint64_t>& sids, const pb::EntitySpawn& spawn);
    static constexpr int kSpawnChunk = 48;
    // interest management (KInterest.cpp): who is told about what
    void run_interest();                                   // once per tick: every client that is due looks around
    void look_around(std::uint64_t sid, KViewer& v);       // forget what left, learn what is near, within the budget
    void entity_gone(KNpc& e, bool keep_self = false);     // it leaves the world: every client that knows it is told
    void wake_viewers_near(const KNpc& e);                 // the viewers around it look again next tick (a hiding ended)
    // the moves of style 1 (KSkills.cpp)
    void end_run(KNpc& e);                    // the run bonus off: the "m_Doing == 0x12" prologue of DoStand / DoSkill / the moves
    void stop_action(KNpc& e);                // a swing or a move under way ends (DoStand / DoWalk take over)
    bool jump_to(KNpc& e, Pos to);            // 0x08087CF0: the way of a jump, its frames and direction
    void start_jump(KNpc& e);                 // 0x0807B320: m_Doing 4, the 0x54 packet
    bool jump_frame(KNpc& e);                 // 0x080818F0 + 0x080817E0: a frame in the air; false when it landed
    bool start_special_skill(KNpc& e);        // 0x08084930 (form 8)
    void special_skill_frame(KNpc& e);        // 0x08087620
    bool start_run_attack(KNpc& e);           // 0x08084A10 (form 11)
    void run_frame(KNpc& e);                  // 0x080853B0
    bool start_special_cast(KNpc& e);         // 0x08084B40 (form 12)
    void special_cast_frame(KNpc& e);         // 0x08086E50
    bool start_blink(KNpc& e);                // 0x08084C90 (form 13)
    void blink_frame(KNpc& e);                // 0x08080760
    bool start_jump_attack(KNpc& e);          // 0x080807E0 (form 10)
    void jump_attack_frame(KNpc& e);          // 0x08084E00
    void cast_child_skill(KNpc& e, bool style0_only);   // the ChildSkillId at the kept target / spot
    void wear_result(KNpc& e, KItemList& list, int part, std::uint32_t id, int before, int left);   // after KItem::Abrade: the sync / the break
    void on_death_player(KNpc& e, EntityId killer, int mode);   // KNpc::OnDeath 0x08088D50 by the death mode: the experience and the money lost
    void player_corpse(KNpc& e);                      // KNpc::Revive 0x080833B0 for a player: the corpse waits, the states off
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
    [[nodiscard]] bool in_reach_of(const KNpc& a, const KNpc& b, int radius) const noexcept;   // within a skill's AttackRadius
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
    void emit_action(const KNpc& e, pb::Action action, EntityId target, int skill_id = 0, int skill_level = 0, Pos aim = Pos{});
    void emit_life(const KNpc& e, std::int32_t delta, EntityId source);
    void emit_ride(const KNpc& e);
    // 0x0807ACB0 for a player: the five equipment rows from the worn pieces (KItemChangeRes::equip_res of parts 0 / 1 / 3 / 10,
    // the mantle -1), +0x1504 bumped and the look told around (the 0xad packet 0x0807A9D0 -> G2C_ENTITY_RES) when it changed
    void update_equip_res(KNpc& e);
    void emit_res(const KNpc& e);
    void emit_camp(const KNpc& e);
    void emit_player_faction(const KNpc& e);
    // the 0x87 packet of SetStateSkillEffect 0x08086892 / RemoveStateSkillEffect 0x0807D40A to the player's client
    void emit_state(const KNpc& e, const KStateNode& node, bool removed);
    // G2C_MISSLE: a missile born / flying / gone to the launcher's watchers (the 2.0 client runs CastMissles itself; docs/CLIENT-2.0.md §11)
    void emit_missle(const KMissle& m, bool removed, bool collided = false);
    // the experience of a dead npc to the players in its damage records (0x0809BDD0)
    EntityId share_experience(KNpc& dead, EntityId killer);   // 0x0809BDD0: returns the damage record that hurt it most (the owner of the drop)
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
    std::array<int, kMissionValues> mission_values_{};          // SubWorld+0x484b8..: GetMissionV / SetMissionV
    std::array<std::string, kMissionStrings> mission_strings_;  // SubWorld+0x48648..: GetMissionS / SetMissionS
    std::vector<std::uint32_t> hosted_maps_;                    // the zone's maps (set by KGameServer)
    std::vector<EntityId> pending_removes_;                     // DelNpc: taken out at the next frame (the 0x3e9 nodes)
    void flush_pending_removes();
    std::vector<KMission> missions_;                            // KMissionArray: the missions open on this map
    void mission_tick();                                        // KMission::Activate: the timers due
    struct KScriptTimer {   // one 0x114-byte timer object of jx_linux_y (0x081CDA80): +4 the function, +0x108 the script, +0x10c
        std::uint32_t id = 0;   // the param, +0x110 the id the manager 0x82e8cac gave it
        std::uint64_t fire_tick = 0;
        std::uint64_t frames = 0;
        std::string script;
        std::string fn;
        int param = 0;
    };
    std::vector<KScriptTimer> script_timers_;
    std::uint32_t next_script_timer_ = 0;
    std::unordered_map<std::string, long long> stat_data_;
    void script_timer_tick();                                   // the AddTimer timers due (0x081CC300)
    struct KScriptTrap {   // KRegion::AddTrap 0x080E1260 from a script: ids from 0x40000000 up, beside the map's trap ids
        std::uint32_t id = 0;
        int cx = 0, cy = 0;
        std::string script;
    };
    std::vector<KScriptTrap> script_traps_;
    [[nodiscard]] const KScriptTrap* script_trap_at(Pos p) const;
    struct KNpcChatLater {   // the 0x110-byte timer object of NpcChat (0x0812E77B): npc, its id, the text
        std::uint64_t fire_tick = 0;
        EntityId npc;
        std::string text;
    };
    std::vector<KNpcChatLater> npc_chats_;
    void npc_chat_tick();
    std::vector<std::uint64_t> save_requests_;
    void save_request_tick();
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
    void load_skills(KNpc& e, const pb::RoleData& role);   // KPlayer::LoadPlayerFightSkillList 0x080C0240
    void save_skills(const KNpc& e, pb::RoleData& out) const;   // KSkillList 0x080E48D0
    void load_task_values(KNpc& e, const pb::RoleData& role);   // KPlayer::LoadPlayerTaskList 0x080C0050
    void save_task_values(const KNpc& e, pb::RoleData& out) const;   // KPlayer::SavePlayerTaskList 0x080BF1C0 / Serialize 0x080CB6A0
    void load_player_events(KNpc& e, const pb::RoleData& role);   // the extra block of TRoleData (0x080BEB60)
    void save_player_events(const KNpc& e, pb::RoleData& out) const;
    void save_items(std::uint64_t sid, pb::RoleData& out) const;
    void item_result(std::uint64_t sid, std::uint32_t seq, pb::Result result);
    void item_moved(std::uint64_t sid, std::uint32_t id, std::uint32_t seq);
    void item_changed(std::uint64_t sid, std::uint32_t id);   // G2C_ITEM_ADD with the item as it is now (a stack changed)
    void send_money(std::uint64_t sid);
    // KItemList::EnoughAttrib needs the player's numbers
    [[nodiscard]] std::function<int(int)> attrib_of(const KNpc& e) const;
    void object_tick(KNpc& e, std::vector<EntityId>& expired);   // KObj::Activate for items and money
    void remove_object(EntityId id);
    // a npc out of the world at once (the 0x3e9 node of the binary, run at the map's next frame 0x080F29A8)
    void remove_npc(EntityId id);
    // KPlayer::Clear 0x080B60A0: the summons of a leaving player go, their records are freed
    void release_summons(EntityId owner);
    void flush_pending_summons();
    void flush_doomed();
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
    // a create-npc cast of this tick: KNpcSet::Add ran inside the cast in the binary; here the entity table must not
    // move under the caller, so the npc is made when the tick ends (its record is already taken)
    struct KPendingSummon {
        EntityId launcher;
        std::uint32_t template_id = 0;
        std::uint32_t level = 0;
        int series = -1;
        Pos at;
        std::size_t record = 0;
    };
    std::vector<KPendingSummon> pending_summons_;
    std::vector<EntityId> doomed_;   // npcs with remove_on_death whose corpse settled this tick (the 0x3e9 nodes)
    void flush_pending_drops();
    std::uint32_t random_percent() { return static_cast<std::uint32_t>(rng_() % 100); }   // g_Random(100)
    std::vector<Packet> outbox_;
    std::uint64_t tick_ = 0;
    std::minstd_rand rng_;
    std::vector<EntityId> scratch_ids_;
    std::vector<std::uint64_t> scratch_sids_;
    mutable std::unordered_map<std::uint64_t, KNpcLevelData> level_cache_;   // (template id, level, series) -> level data
    std::vector<KWorldChange> world_changes_;
    std::vector<KChatBroadcast> chat_broadcasts_;
    // 0x080502A0: the cost of a line by chatcost.ini type - false when it cannot be paid (nothing is taken then)
    bool chat_pay(KNpc& e, int type);
    // after an exp change: the log, a level up (life sync, the passives that open), the attrib sync (the 0xc6 packet)
    void player_exp_changed(KNpc& e, std::int64_t exp, std::uint32_t level_before);
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
