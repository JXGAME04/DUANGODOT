#pragma once

// KSkill / KSkillManager of the old core (Core/Src/KSkills.h, KSkillManager.h) the way the JX2
// server keeps them (jx_linux_y, docs/LINUX-SERVER.md §11), read function by function:
//
//   KSkillManager::Init            0x080E7200  \settings\Skills.txt row by row: m_SkillInfo[id-1] =
//                                              {row, style, MaxLevel, the row's data}, so the LAST row
//                                              of an id wins; ids outside 1..2000 and negative styles
//                                              are skipped
//   KSkill::GetInfoFromTabFile     0x080E9200  the 60 columns the server reads, with their defaults
//                                              (KSkillRow below; offsets are those of KSkill)
//   KSkillManager::InstanceSkill   0x080E6E10  one KSkill per (id, level), made on first use: the info
//                                              block copied in, then LoadSkillLevelData(level, row);
//                                              only styles 0..4 and 14 (13 = thief skills, apart)
//   KSkill::LoadSkillLevelData     0x080EE4B0  the four attribute lists start empty; for i = 1..20
//                                              with LvlSetting_i != "" and LvlData_i not starting with
//                                              '0': GetSkillLevelData(setting, data, level) of the
//                                              LvlSetScript -> a number (printed "%d") or a string
//                                              "v1,v2,v3"; anything else ends the loop
//   KSkill::ParseString2MagicAttrib 0x080EE380 name -> id through the name map of KMagicDesc
//                                              (skill_desc 318 refused), a leading '"' skipped, three
//                                              KSG_StringGetInt separated by KSG_StringSkipSymbol(',')
//   KSkill::AddMagicAttrib         0x080EDCC0  which list or field an attribute goes to: add_attrib()
//
// The numbers per level therefore come from the same Lua scripts the JX2 server runs
// (script\skill\*.lua, converted to Lua 5.4 by `python tools/dev.py lua`), not from a table.

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jx/zone/KMagicAttrib.h"

namespace jx::zone {

class KScriptCache;

inline constexpr int kMaxSkill = 2000;          // 0x7d0: the JX2 manager's m_SkillInfo (JX1: MAX_SKILL 2400)
inline constexpr int kMaxSkillLevel = 64;       // MAX_SKILLLEVEL: m_pOrdinSkill[id][level]
inline constexpr int kSkillLevelSettings = 20;  // MAXSKILLLEVELSETTINGNUM: LvlSetting1..20 / LvlData1..20
inline constexpr int kSkillAttribs = 20;        // each of the four KMagicAttrib lists of a KSkill (0x144 bytes apart)

// eSKillStyle of SkillDef.h; the JX2 manager instantiates 0..4 and 14, hands 13 to KThiefSkill
enum KSkillStyle : int {
    skill_style_missles = 0,
    skill_style_melee = 1,
    skill_style_initiative_npc_state = 2,
    skill_style_passivity_npc_state = 3,
    skill_style_create_npc = 4,
    skill_style_thief = 13,
    skill_style_jx2_14 = 14,
};

// m_eRelation of KSkill (+0x90), built by GetInfoFromTabFile from the Target* columns
enum KSkillRelation : int {
    skill_relation_other = 1,
    skill_relation_self = 2,
    skill_relation_ally = 4,
    skill_relation_enemy = 8,
    skill_relation_no_npc = 0x20,
};

// KSkillManager::TSkillInfo (row, style, max level) plus the data block of the row that
// InstanceSkill copies into a new KSkill (+0x4 .. +0x114).  Every field is read by
// GetInfoFromTabFile (0x080E9200) with the default noted; the KSkill offset is in the comment.
struct KSkillRow {
    int row = 0;                          // m_nTabFileRowId: 1-based row of Skills.txt (header = 1); 0 = no skill
    int max_level = 0;                    // MaxLevel (TSkillInfo + 8; KSkillManager::GetSkillMaxLevel)
    std::string name;                     // SkillName        +0x04 (32 bytes)
    int id = 0;                           // SkillId          +0x24
    int style = 0;                        // SkillStyle       +0x28 (also TSkillInfo + 4)
    int char_anim_id = 0;                 // CharAnimId       +0x2c
    bool is_physical = false;             // IsPhysical       +0x30
    bool weapon_skill = false;            // WeaponSkill      +0x34 (JX2)
    bool is_aura = false;                 // IsAura           +0x38
    bool is_melee = false;                // IsMelee          +0x3c
    bool use_attack_rate = false;         // IsUseAR          +0x40
    bool is_exp_skill = false;            // IsExpSkill       +0x44 (JX2)
    int attack_radius = 50;               // AttackRadius     +0x48, default 0x32
    int wait_time = 0;                    // WaitTime         +0x4c
    int param1 = 0;                       // Param1           +0x50
    int param2 = 0;                       // Param2           +0x54
    int series = 0;                       // Series           +0x58 (JX2)
    int do_hurt = 100;                    // DoHurt           +0x5c, default 0x64 (JX1 defaulted to 1)
    int client_send = 0;                  // ClientSend       +0x60 (1: no payload is made, 2: the client casts it alone - CastMissles 0x080ECB10)
    int state_special_id = 0;             // StateSpecialId   +0x64
    int state_priority = 0;               // StatePriority    +0x68 (JX2)
    int req_level = 0;                    // ReqLevel         +0x6c (WORD)
    int char_class = 0;                   // CharClass        +0x70
    bool target_only = false;             // TargetOnly       +0x74
    bool target_enemy = false;            // TargetEnemy      +0x78
    bool target_other = false;            // TargetOther      +0x7c (JX2)
    bool target_ally = false;             // TargetAlly       +0x80
    bool target_obj = false;              // TargetObj        +0x84
    bool target_self = false;             // TargetSelf       +0x88
    bool target_no_npc = false;           // TargetNoNpc      +0x8c (JX2)
    int relation = 0;                     // m_eRelation      +0x90 (KSkillRelation bits)
    bool peace_can_use = false;           // PeaceCanUse      +0x94 (JX2)
    int cost = 0;                         // CostValue        +0x98 (m_nCost; skill_cost_v overrides)
    int cost_type = 0;                    // SkillCostType    +0x9c
    int time_per_cast = 0;                // TimePerCast      +0xa0
    int time_per_cast_on_horse = 0;       // TimePerCastOnHorse +0xa4 (JX2)
    int eqt_limit = -2;                   // EqtLimit         +0xa8, default -2
    int horse_limit = 0;                  // HorseLimit       +0xac
    int stop_when_move = 0;               // StopWhenMove     +0xb0
    int child_skill_num = 0;              // ChildSkillNum    +0xb4
    int child_skill_id = 0;               // ChildSkillId     +0xb8
    int child_skill_level = 0;            // ChildSkillLevel  +0xbc
    bool base_skill = false;              // BaseSkill        +0xc0
    bool by_missle = false;               // ByMissle         +0xc4
    int missles_form = 0;                 // MisslesForm      +0xc8
    int missles_generate = 0;             // MslsGenerate     +0xcc
    int missles_generate_data = 0;        // MslsGenerateData +0xd0
    bool heel_at_parent = false;          // HeelAtParent     +0xd4
    int relative_pos_type = 0;            // RelativePosType  +0xd8 (JX2)
    bool fly_event = false;               // FlyEvent         +0xdc
    bool start_event = false;             // StartEvent       +0xe0
    bool collide_event = false;           // CollideEvent     +0xe4
    bool vanished_event = false;          // VanishedEvent    +0xe8
    int fly_skill_id = 0;                 // FlySkillId       +0xec
    int fly_event_time = 0;               // FlyEventTime     +0xf0
    int start_skill_id = 0;               // StartSkillId     +0xf4
    int vanished_skill_id = 0;            // VanishedSkillId  +0xf8
    int collide_skill_id = 0;             // CollidSkillId    +0xfc
    int event_skill_level = 0;            // EventSkillLevel  +0x100
    int max_shadow_num = 0;               // MaxShadowNum     +0x108
    std::string level_up_script;          // LevelUpScript, lower-cased  (+0x10c holds its g_FileName2Id)
    std::string level_set_script;         // LvlSetScript, lower-cased   (+0x110 holds its g_FileName2Id)
    std::array<std::string, kSkillLevelSettings> level_setting{};   // LvlSetting1..20 (read by LoadSkillLevelData)
    std::array<std::string, kSkillLevelSettings> level_data{};      // LvlData1..20

    // GetInfoFromTabFile over the cells of one row (skills.json: column -> cell text; a missing
    // or empty cell is the default like KTabFile::GetInteger, anything else is strtol).
    static KSkillRow from_cells(const std::unordered_map<std::string, std::string>& cells);
};

// One skill at one level.
class KSkill {
public:
    KSkillRow row;                        // the info block, changed by the skill_* attributes of the level
    int level = 0;                        // m_ulLevel  +0x114

    // What LoadSkillLevelData put here through add_attrib.  The counts are what the binary
    // increments; a list is never cleared, it is only refilled from the start (LoadSkillLevelData
    // resets the four counts and a KSkill is loaded once per level).
    std::array<KMagicAttrib, kSkillAttribs> missle_attribs{};      // +0x174, count +0x2b4: ids 15..25 and 325..338
    std::array<KMagicAttrib, kSkillAttribs> damage_attribs{};      // +0x2b8, count +0x3f8: ids 56..75 at FIXED slots (damage_slot)
    std::array<KMagicAttrib, kSkillAttribs> immediate_attribs{};   // +0x3fc, count +0x53c: the rest with nValue[1] == 0 (nValue[1] stored as 0)
    std::array<KMagicAttrib, kSkillAttribs> state_attribs{};       // +0x540, count +0x680: the rest with nValue[1] != 0
    int missle_attrib_count = 0;
    int damage_attrib_count = 0;
    int immediate_attrib_count = 0;
    int state_attrib_count = 0;
    // addskilldamage1..6 (304..309): {nValue[0], nValue[2], nValue[1]} at +0x120 + i * 12
    struct AddSkillDamage {
        int skill_id = 0;
        int value = 0;
        int param = 0;
    };
    std::array<AddSkillDamage, 6> add_skill_damage{};
    std::vector<std::pair<int, int>> append_skills;   // skill_appendskill (11): {nValue[0], nValue[1]} pushed when nValue[0] != 0 (+0x694)
    int skill_exp = 0;                                // skill_skillexp_v (8)  +0x11c
    bool level_data_loaded = false;                   // LoadSkillLevelData found and ran the level script

    // 0x080EDCC0: the damage attributes 56..75 have one slot each (two ids share slots 0 and 2)
    static constexpr int kDamageSlots = 18;
    [[nodiscard]] static int damage_slot(int id) noexcept;

    // 0x080EE4B0
    void load_skill_level_data(int level, KScriptCache* scripts);
    // 0x080EE380: false when the name is unknown or skill_desc
    bool parse_string_to_magic_attrib(const std::string& name, const std::string& value);
    // 0x080EDCC0
    void add_attrib(const KMagicAttrib& m);
    // KSG_StringSkipSymbol('"') + KSG_StringGetInt, ',', GetInt, ',', GetInt (0x08226BC0 / 0x08226C20):
    // "25,-1,2" -> {25, -1, 2}; a value that is not a number is 0 and stops the rest ("25.0,3" -> {25, 0, 0})
    [[nodiscard]] static std::array<int, 3> parse_values(const std::string& value);

    // The basic attacks of Skills.txt (row 2 = skill 1 melee, row 3 = skill 2 ranged) at level 1
    // with every level number 0, for a map without a skill table (tests) and for a machine
    // without their level scripts: the row's own columns, and physicsenhance_p / attackrating_p
    // at 0 - what the JX1 fallback scripts of bin/Server answer.  nullptr for any other id.
    [[nodiscard]] static const KSkill* basic_attack(int id);
};

// The damage types of KNpc::CalcDamage (the jump tables 0x082549E4 / 0x08254A34 of the resists:
// 0 physics, 1 fire, 2 cold, 3 light, 4 poison; 5 = magic, 6 = the damage returned).
enum KDamageType : int {
    damage_physics = 0,
    damage_fire = 1,
    damage_cold = 2,
    damage_light = 3,
    damage_poison = 4,
    damage_magic = 5,
    damage_return = 6,
};

// MAGIC_ATTRIB name -> id, the map KMagicDesc's constructor fills next to its name table
// (0x0830EB98; the names of KMagicAttribId.h); -1 when unknown.
[[nodiscard]] int magic_attrib_id(const std::string& name);

// The damage attributes a cast carries, as KNpc::AppendSkillEffect (0x0807CE70) lays them out for
// KNpc::ReceiveDamage (0x0808A4A0): one fixed slot per kind.  The skill's own list
// (KSkill::damage_attribs, KSkill::damage_slot) holds the same kinds one slot earlier, with
// seriesdamage_p last.
enum KDamageSlot : int {
    damage_slot_series = 0,          // seriesdamage_p 75 + the launcher's seriesenhance_p
    damage_slot_attack_rating = 1,   // attackrating_v 56: the launcher's rating (+ base x attackrating_p)
    damage_slot_ignore_defense = 2,  // ignoredefense_p 58
    damage_slot_physics = 3,         // physicsdamage_v 59 (from physicsenhance_p 65 or physicsdamage_v)
    damage_slot_cold = 4,            // colddamage_v 60
    damage_slot_fire = 5,            // firedamage_v 61
    damage_slot_light = 6,           // lightingdamage_v 62
    damage_slot_poison = 7,          // poisondamage_v 63
    damage_slot_magic = 8,           // magicdamage_v 64
    damage_slot_steal_life = 9,      // steallife_p 66 (physical skills)
    damage_slot_steal_mana = 10,     // stealmana_p 67
    damage_slot_steal_stamina = 11,  // stealstamina_p 68
    damage_slot_knock_back = 12,     // knockback_p 69
    damage_slot_deadly_strike = 13,  // deadlystrike_p 70 (physical skills)
    damage_slot_fatally_strike = 14, // fatallystrike_p 71
    damage_slot_stun = 15,           // stun_p 72
    damage_slot_add_skill_exp1 = 16, // addskillexp1 73
    damage_slot_add_skill_exp2 = 17, // addskillexp2 74
    damage_slot_count = 18,
};

// KMissleMagicAttribsData of the old core as the JX2 server builds it
// (KSkill::CreateMissleMagicAttribsData 0x080E9E90; a node is 0x29c bytes): everything one cast
// carries to the npcs it reaches - the skill's state attributes (copied), its immediate ones, and
// the damage attributes with the launcher's numbers merged in.  The appended skills of the cast
// (skill_appendskill) follow as further nodes (+0x298), each carried to the same targets.
struct KMissleMagicAttribsData {
    int skill_id = 0;                                                  // +0
    int level = 0;                                                     // +4
    std::array<KMagicAttrib, kSkillAttribs> state_attribs{};           // +0x8
    int state_count = 0;                                               // +0x148
    std::array<KMagicAttrib, kSkillAttribs> immediate_attribs{};       // +0x14c (a pointer into the KSkill there)
    int immediate_count = 0;                                           // +0x150
    std::array<KMagicAttrib, kSkillAttribs> damage_attribs{};          // +0x154 (KDamageSlot layout)
    int damage_count = 0;                                              // +0x294
};
using KMissleMagicAttribsList = std::vector<KMissleMagicAttribsData>;

// The rows of skills.json (jxassets export-skills) the way KSkillManager::Init keeps them.
class KSkillTable {
public:
    static std::optional<KSkillTable> load(const std::string& file, std::string* error);
    // \settings\attribconstdata.ini as KSkillManager::Init reads it (0x080E7532: for every
    // MAGIC_ATTRIB name a section with Count and Data0..): the numbers of an attribute, or
    // nullptr.  The fight uses [returnskill_p] / [ignoreskill_p] (the state skill, its frames,
    // then the skills they answer), [autoreplyskill] / [autoattackskill] (the skills that do NOT
    // wake the auto skills) and [staticmagicshield_v] (the states that go when the shield breaks).
    [[nodiscard]] const std::vector<int>* attrib_data(int attrib_id) const noexcept;
    void set_attrib_data(int attrib_id, std::vector<int> values) { attrib_data_[attrib_id] = std::move(values); }
    // m_SkillInfo[id - 1]; nullptr when the id has no row
    [[nodiscard]] const KSkillRow* info(int id) const;
    // the SkillId of the row named so - the scripts' SetSkillLevel("name") goes through
    // KTabFile::GetInteger(Skills.txt, szRowName, "SkillId"); 0 when no row has the name
    [[nodiscard]] int id_of(const std::string& name) const;
    [[nodiscard]] int max_level(int id) const;    // KSkillManager::GetSkillMaxLevel: 0 without a row
    [[nodiscard]] int style(int id) const;        // KSkillManager::GetSkillStyle: -1 without a row
    [[nodiscard]] std::size_t size() const noexcept { return info_.size(); }
    void add(KSkillRow row) { info_[row.id] = std::move(row); }

private:
    std::unordered_map<int, KSkillRow> info_;
    std::unordered_map<int, std::vector<int>> attrib_data_;
};

// g_SkillManager: the per-level instances.  One per map instance, because the level data is
// computed by the map's own Lua states (KScriptCache).
class KSkillManager {
public:
    KSkillManager(std::shared_ptr<const KSkillTable> table, KScriptCache* scripts);

    // KSkillManager::GetSkill(id, level) + InstanceSkill (0x080E6E10): nullptr outside 1..2000 /
    // 1..64, for an id without a row, and for the styles the binary does not instantiate here
    // (5..12, and 13 = thief); otherwise the instance of that level, made on first use.
    const KSkill* get(int id, int level);
    [[nodiscard]] const KSkillTable* table() const noexcept { return table_.get(); }
    [[nodiscard]] std::size_t instances() const noexcept { return skills_.size(); }

private:
    std::shared_ptr<const KSkillTable> table_;
    KScriptCache* scripts_ = nullptr;
    std::unordered_map<std::uint32_t, std::unique_ptr<KSkill>> skills_;   // (id << 8 | level) -> m_pOrdinSkill[id-1][level-1]
};

} // namespace jx::zone
