#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jx/ids.hpp"
#include "jx/zone/KMath.h"
#include "jx/zone/KNpcAttrib.h"
#include "jx/zone/KNpcGold.h"
#include "jx/zone/KObj.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KRegion.h"
#include "jx/zone/KSkillList.h"

namespace jx::zone {

// Positions are simulated in fixed point (kSub sub-units per world unit) with integer math only,
// so every platform produces bit-identical results for the same inputs (replayable ticks).
inline constexpr std::int64_t kSub = 256;

enum class KNpcKind : std::uint8_t { player = 1, npc = 2, monster = 3, drop = 4 };

// KNpc::m_Doing of the old game, the part the zone simulates (knock_back = do_knockback 0x18 of
// the JX2 server: pushed over frame_total frames to knock_dest).
// The zone's own numbering; the m_Doing of jx_linux_y in brackets: stand (1), walk (3), attack = do_attack (7),
// hurt (9), death (10), revive (21), knock_back (24), magic = do_magic (6); the moves of a style-1 skill
// (docs/LINUX-SERVER.md §16.2): jump (4), special_skill (14), run (18), special_cast (19), jump_attack (20), blink (23)
// sit = do_sit (8): KNpc::DoSit 0x0807B550, the frame 0x08087880 holds the last picture; ProcessState feeds life / mana / stamina (0x0808BBE6)
enum class KDoing : std::uint8_t { stand = 0, walk, attack, hurt, death, revive, knock_back, magic, jump, special_skill, run, special_cast, jump_attack, blink, sit };

// NPC_COMMAND of the old core: the JX2 ring of five at KNpc+0x169c (24 bytes each) that
// KNpc::SendCommand 0x0809B750 fills - only do_skill (5) goes through it - and
// KNpc::ProcessCommand 0x0809B9E0 works off, one a frame; 0x0809B510 counts `life` down.
struct KNpcCommand {
    int cmd = 0;          // +0x00 5 = do_skill
    int skill_id = 0;     // +0x04
    int param1 = -1;      // +0x08 a spot's x, or -1 for a target
    int param2 = 0;       // +0x0c a spot's y (the target's index in the binary: EntityId here)
    EntityId target;      //       the npc aimed at when param1 == -1
    int param3 = 0;       // +0x10 (0)
    int life = 0;         // +0x14 frames the command may wait (0x12 = 18)
};

// KSkillList::m_Skills[1..4] of a npc (Skill1..4 / Level1..4 of npcs.txt) as far as KNpcAI needs it.
struct KNpcSkillSlot {
    int id = 0;
    int level = 0;            // from the level script (Level1..4 cells); 0 = SetActiveSkill fails
    bool known = false;       // in skills.txt; unknown skills leave m_CurrentAttackRadius alone (GetSkill == NULL)
    int attack_radius = 0;    // KSkill::GetAttackRadius
    bool melee = false;
    bool target_self = false;
};

// KNpc::m_DamageRecord of the JX2 server (KNpc+0x1724, three cells of {player, damage, ttl};
// KDamageRecord::Add 0x0809BC70): who hurt this npc how much, so that its experience is shared
// by damage dealt when it dies (0x0809BDD0: exp x damage / life max per cell).  A cell is worth
// 0x4B0 = 1200 frames after its last hit; a fourth attacker finds no cell and gets nothing.
struct KDamageRecord {
    EntityId player;
    int damage = 0;
    int ttl = 0;
};
inline constexpr int kDamageRecordCells = 3;
inline constexpr int kDamageRecordTtl = 0x4B0;

// KStateNode of the old core: one skill's timed state on this npc (m_StateSkillList, KNpc+0x234
// of jx_linux_y; a node is 0x170 bytes).  `states` keeps the NEGATED values the skill applied, so
// taking the state off is applying them once more (KNpc::SetStateSkillEffect 0x08086260,
// RemoveStateSkillEffect 0x0807D310, the countdown at the end of ProcessState 0x0808B8A8).
inline constexpr int kMaxSkillState = 20;   // MAX_SKILL_STATE
struct KStateNode {
    int skill_id = 0;           // +0x10
    int level = 0;              // +0x14
    int left_time = 0;          // +0x18  frames left; -1 = until removed (a passive skill)
    bool refresh = false;       // +0x1c  SetStateSkillEffect's 9th argument
    int param = 0;              // +0x20  its 10th
    int extra = 0;              // +0x16c its 12th
    int special_id = 0;         // +0x164 KSkill::StateSpecialId - the state's icon
    int priority = 0;           // +0x168 KSkill::StatePriority
    std::array<KMagicAttrib, kMaxSkillState> states{};   // +0x24
};

// The five auto-skill lists of a npc (KNpc+0x182c, +0x1850, +0x1874, +0x1898, +0x18bc; KNpc::Init
// 0x0807DBD0 names them "auto cast per n frame", 受到攻击自动施放, 攻击命中自动施放, 生命濒危自动施放,
// 死亡自动施放): every frame (0x0808BE80), when hit and when hitting (ReceiveDamage 0x0808B507 /
// 0x0808B1D3), when the life falls under a quarter (0x0808B13A); the last is never walked.  The
// attributes autocastskill 272, autoreplyskill 195 and autoattackskill 196 fill the first three
// (0x08189000); the every-frame list and the on-cast map are cleared by ClearAttrib (0x0807F5AC).
enum class KAutoSkillList : int { every_frame = 0, hit_reply, on_hit, life_quarter, on_death };
inline constexpr std::size_t kAutoSkillLists = 5;

// One entry of such a list (0x3c bytes of the binary), keyed by skill id << 8 | level.
struct KAutoSkillEntry {
    int rate = 0;                 // +0x14: the percent, added up by every attribute with the key (a negated one takes it back)
    int interval = 0;             // +0x18: MinPerCastTime - frames between two casts on the same target
    bool own_skill = false;       // +0x34: the npc's own skill: its skill list decides (0x080E4540) and its cooldown starts (0x080847B0)
    int at_target = 0;            // +0x38: 1 = cast on the npc handed over, else on the one the list is walked for
    std::unordered_map<std::uint64_t, std::uint64_t> next_tick;   // +0x1c: per target, the frame the next cast may come (0x08188A10)
};

// KNpc+0x19d8..: one attribute of one skill's states is changed by this much when that skill is
// cast (0x080792C0, called by CreateMissleMagicAttribsData and CastPassivitySkill).
struct KStateModifier {
    int skill_id = 0;   // +0x19d8
    int attrib = 0;     // +0x19dc  the MAGIC_ATTRIB id to change
    int index = 0;      // +0x19e0  which nValue
    int delta = 0;      // +0x19e4
};

struct KNpc {
    EntityId id;
    KNpcKind kind = KNpcKind::npc;
    std::string name;

    std::int64_t fx = 0, fy = 0;   // current position (sub-units)
    std::int64_t tx = 0, ty = 0;   // next waypoint (sub-units)
    std::vector<Pos> path;         // waypoints after the current one; last = final destination
    bool moving = false;
    std::uint32_t dir = 0;         // facing 0..63 (old g_GetDirIndex: 0 = down, clockwise on screen)
    std::uint32_t speed = 0;       // world units per second
    std::uint32_t move_seq = 0;    // last MoveReq.seq (players)

    std::uint32_t level = 1;       // m_Level (KNpc+0x20)
    std::uint32_t series = 0;      // m_Series (+0x28)
    std::uint32_t sex = 0;         // m_nSex (+0x152c)
    std::uint32_t template_id = 0;

    // The numbers the npc fights with (KNpcAttrib.h): `base` is what the template, the role data
    // and the level tables gave (m_LifeMax ...), `cur` what equipment, skills and states made of
    // it (m_CurrentLifeMax ...); KNpcAttribModify changes `cur`, clear_attrib() rebuilds it.
    KNpcAttrib base;
    KNpcCurrentAttrib cur;
    // the player behind a player's npc (KPlayer.h): points, experience, level-ups
    KPlayer player;

    // combat / animation state (KNpc::m_Doing, m_Frames).  One frame = one zone tick: the zone
    // ticks at 18 Hz like the old logic loop, so the frame counts of npcs.txt keep their meaning.
    KDoing doing = KDoing::stand;
    std::uint32_t frame_total = 0;   // m_Frames.nTotalFrame
    std::uint32_t frame_cur = 0;     // m_Frames.nCurrentFrame
    EntityId attack_target;          // kept attacking until it dies or we are told to move
    Pos knock_dest;                  // +0x14a0 / +0x14a4: where a knock back (KDoing::knock_back) ends
    // the cast in progress (the same +0x14a0 / +0x14a4 in the binary; CastSkill 0x08088350 keeps
    // p1 / p2 there for the fire at 60 % of the action, 0x08085020)
    int cast_param1 = -1;            // a spot's x, or -1 for a target
    int cast_param2 = 0;             // a spot's y
    EntityId cast_target;            // the npc aimed at when cast_param1 == -1
    // the moves of style 1 (KNpc 0x08087F70; docs/LINUX-SERVER.md §16.2).  A move overwrites +0x14a0 / +0x14a4
    // (cast_param1/2, knock_dest) with the spot it goes to; the child skill is cast at the pair kept here.
    int cast_kept1 = -1;             // +0x14a8: the cast's p1 as given (-1 = a target)
    int cast_kept2 = 0;              // +0x14ac: its p2 (the spot's y)
    EntityId cast_kept_target;       // the target when cast_kept1 == -1
    int jump_steps = 0;              // +0x195c: the frames of a jump = its way / the step length
    int jump_dir = -1;               // +0x1960: its direction (g_GetDirIndex; -1 on the spot)
    int jump_arc = 0;                // +0x1938 = 5 x (steps - 1): the constant of the height curve
    int height = 0;                  // +0x2c: the height in the air this frame (a jump; 0 on the ground)
    int phase = 0;                   // +0x1964: 0 the jump / 1 the strike of a jump attack; the cast index of a multi cast
    int run_counter = 0;             // +0x164c: the frames of a run attack so far
    int run_bonus = 0;               // +0x14b0: the Param1 on the run speed while running (off again when it ends)
    std::deque<KNpcCommand> commands;   // +0x169c..: the do_skill commands waiting (kCommandQueue at most)
    static constexpr std::size_t kCommandQueue = 5;   // the ring of five (+0x171c set when full)
    static constexpr int kCommandLife = 18;           // the 0x12 the skill handler 0x080DD130 hands SendCommand
    Pos knock_from;                  // where it started (the way between is walked frame by frame)
    std::uint32_t approach_tries = 0;   // walks toward an out-of-reach target, a few times at most
    // m_LifeState: a medicine at work - `value` life every GAME_UPDATE_TIME frames for `time`
    // frames (KNpcAttribModify::LifePotionV, KNpc::ProcessState; jx_linux_y 0x08097E70 / 0x0808B7BC)
    struct PotionState {
        int value = 0;
        int time = 0;
    };
    PotionState life_state;
    PotionState mana_state;           // m_ManaState (+0x200 / +0x208)
    // m_PoisonState: `value` = damage per tick (+0x1c0), `time` = frames left (+0x1c8), and the
    // frames between two ticks (+0x1c4); set and merged by 0x0807BD60 from ReceiveDamage
    PotionState poison_state;
    int poison_interval = 0;
    PotionState freeze_state;         // +0x1d8
    PotionState stun_state;           // +0x1e8
    // the skills' timed states (KStateNode) and what the combat code keeps between two blows
    std::vector<KStateNode> state_skills;   // m_StateSkillList +0x234
    int damage_lock = 0;                    // +0x1694: ReceiveDamage refuses while it is not 0
    // +0x181c: how the npc came to be - 1 the "GoldBoss" record of the server data base (0x0820A460 -> 0x080F0412),
    // 2 the script's AddNpc with an 8th argument of 1 (0x0811BF2A; 3 with remove-on-death), 0 a placement of
    // Region_S.dat (0x080E2850 -> KNpcSet::Add 0x0809FBD0 never sets it), one a skill made (0x0813A770 with bKind 0)
    // or a player.  Non-zero: the attacker's add_boss_damage counts (0x08079750 == 3), every tenth frame casts the
    // template's aura in cell 5 (0x0808BAF6), and it never turns gold (0x08086088)
    int boss_flag = 0;
    // KNpc+0x88 KNpcGold: the gold (elite) monster state and its backup (KNpcGold.h)
    KNpcGold gold;
    // +0x14dc .. +0x14ec: the equipment rows the clients draw (KItemChangeRes::equip_res of the worn pieces, 0x0807ACB0):
    // helm, armour, weapon, horse (-1 none), mantle (-1); +0x1504: bumped with every change so a client can tell a stale look
    int helm_res = 0;
    int armor_res = 0;
    int weapon_res = 0;
    int horse_res = -1;
    int mantle_res = -1;
    std::uint8_t res_version = 0;
    // +0x1818 (m_nCurPKPunishState of 2003): 3 = a PK-battle death (no penalty, GetPKRelation 0x0807A3C2 returns 3); the scripts of
    // the arenas set it (0x0810F400) - nothing in the zone does yet
    int pk_punish_state = 0;
    // +0x174c: the drop table the death rolls on - the template's DropRateFile (SetTemplate 0x080830BA), the map's
    // `<id>_NormalDropRate` for a placement (0x0809FD30), its `<id>_GoldenDropRate` while gold (0x08086073); a
    // lower-cased game path here, the table's index in the binary
    std::string drop_rate_file;
    EntityId last_damage_id;                // m_nLastDamageIdx +0x1598 (CalcDamage)
    EntityId last_poison_id;                // +0x15a0: who poisoned us last (0x0807BD60)
    KStateModifier state_modifier;          // +0x19d8..
    std::array<std::map<int, KAutoSkillEntry>, kAutoSkillLists> auto_skills{};   // +0x182c.. (KAutoSkillList)
    std::map<int, std::map<int, int>> on_cast_skills;   // +0x18ec: skill id -> {skill id -> percent} cast along with it at its level (oncastskill 274, 0x080821C0)
    int crowd_block_rate = 0;               // +0x1390: min(25, npcs within 256 / addblockrate[0] x addblockrate[1]) once a second (players, 0x0808C078)
    int mana_skill_enhance = 0;             // +0x139c: manatoskill_enhance x mana / mana max once a second (0x0808BFFC); part of a cast's enhance
    // the state icons the client is shown (six, sorted by priority toward the end; 0x08079240):
    // +0x54 the cells, +0x50 the first used cell (6 = none, KNpc::Init), +0x4c what changed
    // (1 = an icon came, 2 = a state came or went; the sync sets it back to 0)
    struct KStateIcon {
        int id = 0;
        int priority = 0;
    };
    std::array<KStateIcon, 6> state_icons{};
    int state_icon_first = 6;
    int state_flag = 0;
    bool potion_counter = false;      // KPlayer+0x86a4 / +0x86a8: StartPotionCounter .. GetPotionCount
    int potion_count = 0;
    std::uint64_t loop_frames = 0;   // m_LoopFrames: ticks alive, for the periodic state update
    // KNpc::Load from the template (KNpcTemplate); the AttackSpeed column is the attack length
    std::uint32_t attack_frame = 20;
    std::uint32_t cast_frame = 20;   // m_CastFrame: length of a non-melee skill (KNpc::DoSkill)
    std::uint32_t hurt_frame = 10;
    std::uint32_t death_frame = 15;
    std::uint32_t revive_frame = 2400;
    static constexpr std::uint32_t kSitFrame = 15;   // m_SitFrame +0x1930: KNpc::Init 0x0807E09B sets 15, nothing else writes it
    bool level_data_from_script = false;

    // KNpcAI state (server side of KNpc.h), named after the old members
    int npc_kind = 0;                 // NPCKIND of npcs.txt (0 normal, 2 partner, 3 dialoger, 4 bird, 5 mouse); players are kind_player
    int camp = 4;                     // m_Camp (NPCCAMP; KNpc::Init: camp_free)  +0x21c
    int current_camp = 4;             // m_CurrentCamp  +0x220
    int ai_mode = 0;                  // m_AiMode (AIMode column; 0 = no ai)
    int ai_param[11] = {};            // m_AiParam[MAX_AI_PARAM]: [0..9] = AIParam1..10, [10] = max skill radius squared
    std::uint32_t ai_max_time = 25;   // m_AIMAXTime: ticks between two decisions
    std::uint64_t next_ai_time = 0;   // m_NextAITime
    int ai_add_life_time = 0;         // m_AiAddLifeTime: heals cast so far
    EntityId people_id;               // m_nPeopleIdx: the enemy locked on, or the last one that hurt us
    int active_skill_id = 0;          // m_ActiveSkillID
    bool active_skill_melee = false;
    bool active_skill_self = false;
    KNpcSkillSlot skills[5];          // m_SkillList.m_Skills[1..4] as KNpcAI sees them (the template's facts)
    // m_SkillList +0x248 (KSkillList.h): every skill held with its levels, cool downs, increments and the
    // per-skill damage enhance map (+0x115c); a npc's Skill1..4 sit in cells 1..4 like the binary's
    KSkillList skill_list;
    KSkillManager* skill_mgr = nullptr;   // g_SkillManager of the map, for what the list does on its own (ClearAttrib)
    std::array<KDamageRecord, kDamageRecordCells> damage_records{};
    // KDamageRecord::Add: the attacker's own cell, else a free one, else nothing
    void add_damage_record(EntityId who, int damage) noexcept
    {
        KDamageRecord* free = nullptr;
        for (KDamageRecord& r : damage_records) {
            if (r.player == who) {
                r.damage += damage;
                r.ttl = kDamageRecordTtl;
                return;
            }
            if (free == nullptr && r.player.value == 0) free = &r;
        }
        if (free == nullptr) return;
        free->player = who;
        free->damage = damage;
        free->ttl = kDamageRecordTtl;
    }
    void clear_damage_records() noexcept { damage_records.fill(KDamageRecord{}); }
    // KDamageRecord::Add 0x0809BC70: a player in a team hits under its captain's name (g_Team[Player+0x5998].captain)
    [[nodiscard]] static EntityId damage_record_key(const KNpc& attacker) noexcept
    {
        if (attacker.kind == KNpcKind::player && attacker.player.team.flag && attacker.player.team.captain_npc != 0) {
            return EntityId{attacker.player.team.captain_npc};
        }
        return attacker.id;
    }
    // KNpcKind::drop - an object on the ground (KObj): what the zone keeps of it; the item itself
    // lives in KSubWorld::ground_items_
    KGroundObject object;
    // players: KPlayer / trap state
    bool fight_mode = false;          // m_FightMode (SetFightState of the gate scripts)
    std::uint32_t trap_script_id = 0; // m_TrapScriptID: the trap under the feet, so a trap fires once per entry
    // [hide] (200) of a state: while > 0 only its own client sees the npc (KNpc::IsInvisibleTo
    // 0x08079200 = KSubWorld::invisible_to).  KNpc::SetHide 0x0807FF80 tells the players around;
    // the cast, the death and a mount break it (0x0807D4C0 = KSubWorld::break_hide).
    int hide = 0;                     // +0x19a0
    // +0x244 the aura kept casting (KNpc::SetAura 0x08087290: an IsAura skill held; ProcessState casts its child at
    // the npc's own spot every GAME_UPDATE_TIME frames through 0x080873B0)
    int aura_skill_id = 0;
    bool hide_syncing = false;        // +0x19a4: set while the "hidden" packet 0x4f goes out - invisible to itself only then
    // a npc a create-npc skill (style 4, 0x080E8770) made: KNpcSet::Add 0x0813A770 got 1 as its 7th argument, which
    // sets byte +0x1824 - KNpc::Revive 0x080833B0 then posts the 0x3e9 "remove (index, id)" node for the map's next
    // frame (0x080F29A8) instead of reviving it - and the skill writes the launcher's index to +0x1828 (nothing reads
    // it back; docs/LINUX-SERVER.md §16.5)
    bool remove_on_death = false;     // +0x1824
    EntityId summon_master;           // +0x1828
    // KNpc::SetHorse 0x0807D520: 1 while the worn horse is ridden (the equip 0x081FE380 / unequip 0x081FFFB0 of part 10,
    // the ride toggle 0x080AEFA0); the 0x20 flag of the 0x4c / 0x4d sync, HorseLimit of CanCastSkill, the horse column of
    // SetSkillCoolTime, KPlayer::ReCalcEquip counts the horse only while ridden (docs/LINUX-SERVER.md §16.6)
    int horse = 0;                    // +0x199c

    [[nodiscard]] bool alive() const noexcept { return doing != KDoing::death && doing != KDoing::revive; }
    // m_ProcessAI: the ai only decides while the npc stands or walks (DoSkill / DoAttack / DoHurt /
    // DoDeath clear the flag, OnSkill / OnHurt / Revive set it again).
    [[nodiscard]] bool process_ai() const noexcept { return doing == KDoing::stand || doing == KDoing::walk; }
    // +0x194c == 0 of the binary: an action (a swing, a cast, a move of style 1) is under way
    [[nodiscard]] bool in_action() const noexcept
    {
        return doing == KDoing::attack || doing == KDoing::magic || doing == KDoing::jump || doing == KDoing::special_skill ||
               doing == KDoing::run || doing == KDoing::special_cast || doing == KDoing::jump_attack || doing == KDoing::blink;
    }
    // KNpc::WaitForFrame: advances the action; true when its frames ran out (counter wraps to 0).
    bool wait_for_frame() noexcept
    {
        ++frame_cur;
        if (frame_cur < frame_total) return false;
        frame_cur = 0;
        return true;
    }
    // KNpc::IsReachFrame
    [[nodiscard]] bool reach_frame(std::uint32_t percent) const noexcept { return frame_cur == frame_total * percent / 100; }

    // KNpc::ClearAttrib (jx_linux_y 0x0807EE60): the current block from the base block, the
    // skill levels back to their own, the potion states dropped when `clear_state` (the
    // equipment, skills and states are applied again by the caller - KPlayer::UpdataCurData).
    void clear_attrib(bool clear_state, int sit_add_per_mille) noexcept
    {
        cur.clear(base, sit_add_per_mille);
        skill_list.clear_attrib(skill_mgr);   // 0x0807F341: the current levels back to the learned ones
        hide = 0;                             // 0x0807F3DE / 0x0807F4A6: the hiding too (a state puts it back when applied again)
        hide_syncing = false;
        if (kind == KNpcKind::player) {       // 0x08082C73..0x08082C87: Player+0x86f8 / +0x8700 / +0x86fc = 0
            player.not_add_pkvalue_p = 0;
            player.pk_punish_weaken = 0;
            player.pk_punish_enhance = 0;
        }
        auto_skills[static_cast<std::size_t>(KAutoSkillList::every_frame)].clear();   // 0x0807F5AC: the every-frame list and
        on_cast_skills.clear();                                                        // 0x0807F5F3: the on-cast map, always
        if (clear_state) {
            life_state = PotionState{};
            mana_state = PotionState{};
            poison_state = PotionState{};
            poison_interval = 0;
            freeze_state = PotionState{};
            stun_state = PotionState{};
        }
    }
    // the KStateNode of a skill, if it holds a state on this npc
    [[nodiscard]] KStateNode* state_of(int skill_id) noexcept
    {
        for (KStateNode& n : state_skills) {
            if (n.skill_id == skill_id) return &n;
        }
        return nullptr;
    }
    // the shorthands the rest of the zone reads
    [[nodiscard]] int life() const noexcept { return cur.life; }
    [[nodiscard]] int life_max() const noexcept { return cur.life_max_v(); }
    [[nodiscard]] int mana() const noexcept { return cur.mana; }
    [[nodiscard]] int mana_max() const noexcept { return cur.mana_max_v(); }

    std::uint64_t sid = 0;         // gateway session (players only)
    std::uint64_t player_id = 0;
    // The sessions whose client has been sent a spawn for this entity, sorted.  Every update about
    // it goes to exactly these, and so does its despawn (KViewer in KSubWorld.h is the other half).
    std::vector<std::uint64_t> watchers;

    // simple wander behaviour for test npcs without an AIMode (0 = static)
    std::int32_t wander_radius = 0;
    Pos home;                      // m_OriginX / m_OriginY: where the npc was placed
    std::uint64_t next_wander_tick = 0;

    [[nodiscard]] Pos pos() const noexcept
    {
        return Pos{static_cast<std::int32_t>(fx / kSub), static_cast<std::int32_t>(fy / kSub)};
    }
    [[nodiscard]] Pos target() const noexcept
    {
        return Pos{static_cast<std::int32_t>(tx / kSub), static_cast<std::int32_t>(ty / kSub)};
    }
    [[nodiscard]] Pos destination() const noexcept { return path.empty() ? target() : path.back(); }
    void set_pos(Pos p) noexcept
    {
        fx = static_cast<std::int64_t>(p.x) * kSub;
        fy = static_cast<std::int64_t>(p.y) * kSub;
        tx = fx;
        ty = fy;
        path.clear();
        moving = false;
    }
    // Starts moving along waypoints (the first becomes the current target).
    void set_path(std::vector<Pos> waypoints) noexcept
    {
        path = std::move(waypoints);
        if (path.empty()) {
            tx = fx;
            ty = fy;
            moving = false;
            return;
        }
        next_waypoint();
    }
    void set_target(Pos p) noexcept { set_path({p}); }
    // Pops the next waypoint into (tx, ty); returns false when none is left.
    bool next_waypoint() noexcept
    {
        if (path.empty()) {
            moving = false;
            return false;
        }
        const Pos p = path.front();
        path.erase(path.begin());
        tx = static_cast<std::int64_t>(p.x) * kSub;
        ty = static_cast<std::int64_t>(p.y) * kSub;
        moving = (tx != fx || ty != fy);
        if (moving) update_dir();
        return true;
    }
    void update_dir() noexcept
    {
        // 64 directions, 0 = down and clockwise on screen, exactly the old g_GetDirIndex (KMath.h)
        const int d = g_GetDirIndex(fx / kSub, fy / kSub, tx / kSub, ty / kSub);
        if (d >= 0) dir = static_cast<std::uint32_t>(d);
    }
};

} // namespace jx::zone
