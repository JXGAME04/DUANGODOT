#include "jx/zone/KSkill.h"

#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <mutex>
#include <utility>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KScriptCache.h"

namespace jx::zone {

namespace {

// KTabFile::GetInteger (0x08228170): the default for a missing / empty cell, else strtol
int cell_int(const std::unordered_map<std::string, std::string>& cells, const char* column, int def)
{
    const auto it = cells.find(column);
    if (it == cells.end() || it->second.empty()) return def;
    return static_cast<int>(std::strtol(it->second.c_str(), nullptr, 10));
}

std::string cell_str(const std::unordered_map<std::string, std::string>& cells, const std::string& column)
{
    const auto it = cells.find(column);
    return it == cells.end() ? std::string{} : it->second;
}

// KSG_StringSkipSymbol (0x08226BC0): blanks, then the symbol when it is there
bool skip_symbol(const char*& p, char symbol) noexcept
{
    while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p))) ++p;
    if (*p == symbol) {
        ++p;
        return true;
    }
    return false;
}

// KSG_StringGetInt (0x08226C20): blanks, an optional '-', digits; no digit = the default
int get_int(const char*& p, int def) noexcept
{
    while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p))) ++p;
    bool negative = false;
    if (*p == '-') {
        negative = true;
        ++p;
        while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p))) ++p;
    }
    if (!std::isdigit(static_cast<unsigned char>(*p))) return def;
    int n = 0;
    while (std::isdigit(static_cast<unsigned char>(*p))) {
        n = n * 10 + (*p - '0');
        ++p;
    }
    return negative ? -n : n;
}

} // namespace

int magic_attrib_id(const std::string& name)
{
    static std::unordered_map<std::string, int> ids;
    static std::once_flag once;
    std::call_once(once, [] {
        for (int id = 0; id < magic_id_count; ++id) {
            const char* n = magic_attrib_name(id);
            if (n != nullptr && n[0] != '\0') ids.emplace(n, id);
        }
    });
    const auto it = ids.find(name);
    return it == ids.end() ? -1 : it->second;
}

KSkillRow KSkillRow::from_cells(const std::unordered_map<std::string, std::string>& cells)
{
    KSkillRow r;
    // 0x080E9200, in the binary's order and with its defaults
    r.id = cell_int(cells, "SkillId", 0);
    r.req_level = cell_int(cells, "ReqLevel", 0) & 0xffff;
    r.missles_form = cell_int(cells, "MisslesForm", 0);
    r.style = cell_int(cells, "SkillStyle", 0);
    r.char_anim_id = cell_int(cells, "CharAnimId", 0);
    r.cost_type = cell_int(cells, "SkillCostType", 0);
    r.missles_generate = cell_int(cells, "MslsGenerate", 0);
    r.name = cell_str(cells, "SkillName");
    r.eqt_limit = cell_int(cells, "EqtLimit", -2);
    r.horse_limit = cell_int(cells, "HorseLimit", 0);
    r.do_hurt = cell_int(cells, "DoHurt", 100);
    r.child_skill_num = cell_int(cells, "ChildSkillNum", 0);
    r.char_class = cell_int(cells, "CharClass", 0);
    r.is_physical = cell_int(cells, "IsPhysical", 0) != 0;
    r.weapon_skill = cell_int(cells, "WeaponSkill", 0) != 0;
    r.is_aura = cell_int(cells, "IsAura", 0) != 0;
    r.use_attack_rate = cell_int(cells, "IsUseAR", 0) != 0;
    r.target_only = cell_int(cells, "TargetOnly", 0) != 0;
    r.target_enemy = cell_int(cells, "TargetEnemy", 0) != 0;
    r.target_ally = cell_int(cells, "TargetAlly", 0) != 0;
    r.target_other = cell_int(cells, "TargetOther", 0) != 0;
    r.target_obj = cell_int(cells, "TargetObj", 0) != 0;
    r.target_no_npc = cell_int(cells, "TargetNoNpc", 0) != 0;
    r.base_skill = cell_int(cells, "BaseSkill", 0) != 0;
    r.by_missle = cell_int(cells, "ByMissle", 0) != 0;
    r.child_skill_id = cell_int(cells, "ChildSkillId", 0);
    r.fly_event = cell_int(cells, "FlyEvent", 0) != 0;
    r.start_event = cell_int(cells, "StartEvent", 0) != 0;
    r.collide_event = cell_int(cells, "CollideEvent", 0) != 0;
    r.vanished_event = cell_int(cells, "VanishedEvent", 0) != 0;
    r.fly_skill_id = cell_int(cells, "FlySkillId", 0);
    r.start_skill_id = cell_int(cells, "StartSkillId", 0);
    r.vanished_skill_id = cell_int(cells, "VanishedSkillId", 0);
    r.collide_skill_id = cell_int(cells, "CollidSkillId", 0);
    r.cost = cell_int(cells, "CostValue", 0);
    r.time_per_cast = cell_int(cells, "TimePerCast", 0);
    r.time_per_cast_on_horse = cell_int(cells, "TimePerCastOnHorse", 0);
    r.param1 = cell_int(cells, "Param1", 0);
    r.param2 = cell_int(cells, "Param2", 0);
    r.child_skill_level = cell_int(cells, "ChildSkillLevel", 0);
    r.event_skill_level = cell_int(cells, "EventSkillLevel", 0);
    r.is_melee = cell_int(cells, "IsMelee", 0) != 0;
    r.fly_event_time = cell_int(cells, "FlyEventTime", 0);
    r.is_exp_skill = cell_int(cells, "IsExpSkill", 0) != 0;
    r.series = cell_int(cells, "Series", 0);
    r.missles_generate_data = cell_int(cells, "MslsGenerateData", 0);
    r.max_shadow_num = cell_int(cells, "MaxShadowNum", 0);
    r.attack_radius = cell_int(cells, "AttackRadius", 50);
    r.wait_time = cell_int(cells, "WaitTime", 0);
    r.client_send = cell_int(cells, "ClientSend", 0) != 0;
    r.target_self = cell_int(cells, "TargetSelf", 0) != 0;
    r.stop_when_move = cell_int(cells, "StopWhenMove", 0);
    r.heel_at_parent = cell_int(cells, "HeelAtParent", 0) != 0;
    r.peace_can_use = cell_int(cells, "PeaceCanUse", 0) != 0;
    r.state_special_id = cell_int(cells, "StateSpecialId", 0);
    r.state_priority = cell_int(cells, "StatePriority", 0);
    r.relative_pos_type = cell_int(cells, "RelativePosType", 0);
    // 0x080E9D10: m_eRelation from the target flags
    r.relation = 0;
    if (r.target_enemy) r.relation |= skill_relation_enemy;
    if (r.target_ally) r.relation |= skill_relation_ally;
    if (r.target_self) r.relation |= skill_relation_self;
    if (r.target_no_npc) r.relation |= skill_relation_no_npc;
    if (r.target_other) r.relation |= skill_relation_other;
    // 0x080E9D72 / 0x080E9DE5: the two script paths, 'A'..'Z' lower-cased, hashed with g_FileName2Id
    auto lower = [](std::string s) {
        for (char& c : s) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
        }
        return s;
    };
    r.level_set_script = lower(cell_str(cells, "LvlSetScript"));
    r.level_up_script = lower(cell_str(cells, "LevelUpScript"));
    // read by LoadSkillLevelData (0x080EE5D5): "LvlSetting%d" / "LvlData%d", default ""
    for (int i = 0; i < kSkillLevelSettings; ++i) {
        r.level_setting[static_cast<std::size_t>(i)] = cell_str(cells, "LvlSetting" + std::to_string(i + 1));
        r.level_data[static_cast<std::size_t>(i)] = cell_str(cells, "LvlData" + std::to_string(i + 1));
    }
    return r;
}

int KSkill::damage_slot(int id) noexcept
{
    // the jump table at 0x08258498: ids 56..75 -> the block that stores the slot
    static constexpr int slots[20] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    if (id < 56 || id > 75) return -1;
    return slots[id - 56];
}

std::array<int, 3> KSkill::parse_values(const std::string& value)
{
    // 0x080EE405..0x080EE489: KSG_StringSkipSymbol(&p, '"'); v1 = GetInt(&p, 0); SkipSymbol(&p, ',');
    // v2 = GetInt(&p, 0); SkipSymbol(&p, ','); v3 = GetInt(&p, 0)
    const char* p = value.c_str();
    std::array<int, 3> v{0, 0, 0};
    skip_symbol(p, '"');
    v[0] = get_int(p, 0);
    skip_symbol(p, ',');
    v[1] = get_int(p, 0);
    skip_symbol(p, ',');
    v[2] = get_int(p, 0);
    return v;
}

bool KSkill::parse_string_to_magic_attrib(const std::string& name, const std::string& value)
{
    if (name.empty()) return false;                           // 0x080EE38C
    const int id = magic_attrib_id(name);                     // the map at 0x0830EB98: not there -> 0
    if (id < 0 || id == magic_skill_desc) return false;       // 0x080EE3FB: 0x13e refused
    KMagicAttrib m;
    m.type = id;
    m.value = parse_values(value);
    add_attrib(m);                                            // 0x080EDCC0
    return true;
}

void KSkill::add_attrib(const KMagicAttrib& m)
{
    const int id = m.type;
    const int v1 = m.value[0], v2 = m.value[1], v3 = m.value[2];
    auto push = [](std::array<KMagicAttrib, kSkillAttribs>& list, int& count, const KMagicAttrib& a) {
        if (count < 0 || count >= kSkillAttribs) return;   // the binary has no check: 20 settings can never fill 21
        list[static_cast<std::size_t>(count)] = a;
        ++count;
    };
    // 0x080EDCE0: missle_exp_begin+1..missle_exp_end-1 (325..338) and missle_begin+1..missle_end-1 (15..25)
    if ((id >= 325 && id <= 338) || (id >= 15 && id <= 25)) {
        push(missle_attribs, missle_attrib_count, m);
        return;
    }
    // 0x080EDCFB / 0x080EDD70: the skill parameters 304..322 and 1..12
    if ((id >= 304 && id <= 322) || (id >= 1 && id <= 12)) {
        switch (id) {
        case magic_skill_cost_v: row.cost = v1; break;                           // +0x98
        case magic_skill_costtype_v: row.cost_type = v1; break;                  // +0x9c
        case magic_skill_mintimepercast_v: row.time_per_cast = v1; break;        // +0xa0
        case magic_skill_misslenum_v: row.child_skill_num = v1; break;           // +0xb4
        case magic_skill_misslesform_v: row.missles_form = v1; break;            // +0xc8
        case magic_skill_param1_v: row.param1 = v1; break;                       // +0x50
        case magic_skill_param2_v: row.param2 = v2; break;                       // +0x54: nValue[1], the old quirk
        case magic_skill_skillexp_v: skill_exp = v1; break;                      // +0x11c
        case magic_skill_waittime: row.wait_time = v1; break;                    // +0x4c
        case magic_skill_mintimepercastonhorse_v: row.time_per_cast_on_horse = v1; break;   // +0xa4
        case magic_skill_appendskill:                                            // +0x694 vector
            if (v1 != 0) append_skills.emplace_back(v1, v2);
            break;
        case magic_skill_eventskilllevel: row.event_skill_level = v1; break;     // +0x100
        case 304: case 305: case 306: case 307: case 308: case 309: {            // addskilldamage1..6 -> +0x120 + i*12
            AddSkillDamage& d = add_skill_damage[static_cast<std::size_t>(id - 304)];
            d.skill_id = v1;
            d.value = v3;
            d.param = v2;
            break;
        }
        case 310: row.attack_radius = v1; break;                                 // skill_attackradius -> +0x48
        case 311: row.start_event = v1 > 0; row.start_skill_id = v3; break;      // skill_startevent -> +0xe0 / +0xf4
        case 312: row.fly_event = v1 > 0; row.fly_skill_id = v3; break;          // skill_flyevent -> +0xdc / +0xec
        case 313: row.collide_event = v1 > 0; row.collide_skill_id = v3; break;  // skill_collideevent -> +0xe4 / +0xfc
        case 314: row.vanished_event = v1 > 0; row.vanished_skill_id = v3; break;// skill_vanishedevent -> +0xe8 / +0xf8
        case 315: row.do_hurt = v1; break;                                       // skill_dohurt -> +0x5c
        case 316: row.by_missle = v1 != 0; break;                                // skill_bymissle -> +0xc4
        default: break;                                                          // skill_showevent (317), 319..322: nothing
        }
        return;
    }
    // 0x080EDD78: the damage attributes 56..75 at fixed slots; 76..82 (damage_reserve) are dropped
    if (id >= 56 && id <= 82) {
        const int slot = damage_slot(id);
        if (slot < 0) return;
        KMagicAttrib& d = damage_attribs[static_cast<std::size_t>(slot)];
        d.type = id;
        if (id == 75) {                       // seriesdamage_p: only nValue[0] is written (0x080EE1AC)
            d.value[0] = v1;
            return;
        }
        d.value = {v1, v2, v3};
        // addskillexp1 / addskillexp2: a zero nValue[0] means this skill's own id (0x080EDF16 / 0x080EDF42)
        if ((id == 73 || id == 74) && v1 == 0) d.value[0] = row.id;
        if (slot <= 14) ++damage_attrib_count;   // 0x080EDF78..: slots 15, 16, 17 do not count
        return;
    }
    // everything else (13, 14, 26..55, 83..303, 323, 324, 339, 340): nValue[1] decides (0x080EDDF0)
    if (v2 == 0) {
        KMagicAttrib a = m;
        a.value[1] = 0;
        push(immediate_attribs, immediate_attrib_count, a);   // +0x3fc, nValue[1] written as 0
    } else {
        push(state_attribs, state_attrib_count, m);           // +0x540
    }
}

void KSkill::load_skill_level_data(int lvl, KScriptCache* scripts)
{
    missle_attrib_count = 0;      // +0x2b4
    damage_attrib_count = 0;      // +0x3f8
    immediate_attrib_count = 0;   // +0x53c
    state_attrib_count = 0;       // +0x680
    level_data_loaded = false;
    if (row.row <= 1) return;     // 0x080EE4C2
    level = lvl;                  // +0x114
    if (row.level_set_script.empty()) {   // the binary treats a skill without a level script as fatal
        log::warn("skill", "skill has no level script", {log::kv("skill", row.id), log::kv("name", row.name)});
        return;
    }
    KLuaScript* script = scripts != nullptr ? scripts->get(row.level_set_script) : nullptr;
    if (script == nullptr) {
        // The binary dies here (0x080EE52A).  This machine lacks 156 of the 285 level scripts
        // (docs/HANDOVER.md §0.4), among them the basic attacks', so the settings are given
        // 0 - what the JX1 fallback scripts of bin/Server answer - and the skill stays usable.
        log::warn("skill", "skill level script missing", {log::kv("skill", row.id), log::kv("name", row.name), log::kv("script", row.level_set_script)});
        for (int i = 0; i < kSkillLevelSettings; ++i) {
            const std::string& setting = row.level_setting[static_cast<std::size_t>(i)];
            const std::string& data = row.level_data[static_cast<std::size_t>(i)];
            if (setting.empty()) continue;
            if (!data.empty() && data[0] == '0') continue;
            parse_string_to_magic_attrib(setting, "0");
        }
        return;
    }
    for (int i = 0; i < kSkillLevelSettings; ++i) {   // 0x080EE5D2, i = 1..20
        const std::string& setting = row.level_setting[static_cast<std::size_t>(i)];
        const std::string& data = row.level_data[static_cast<std::size_t>(i)];
        if (setting.empty()) continue;                 // 0x080EE67B
        if (!data.empty() && data[0] == '0') continue; // 0x080EE688: a data cell starting with '0'
        const auto result = script->call_value("GetSkillLevelData", {setting, data, static_cast<double>(lvl)});
        if (!result) {
            // 0x080EE7B4: "khi lay du lieu cap %d (%s, %s) loi" - the loop ends here
            log::warn("skill", "GetSkillLevelData gave neither a number nor a string", {log::kv("skill", row.id), log::kv("level", lvl), log::kv("setting", setting), log::kv("data", data)});
            break;
        }
        std::string value;
        if (const auto* n = std::get_if<double>(&*result)) {
            value = std::to_string(static_cast<int>(*n));   // 0x080EE741: fistp with truncation, then "%d"
        } else {
            value = std::get<std::string>(*result);
        }
        parse_string_to_magic_attrib(setting, value);
    }
    level_data_loaded = true;
}

std::optional<KSkillTable> KSkillTable::load(const std::string& file, std::string* error)
{
    auto fail = [&](std::string why) -> std::optional<KSkillTable> {
        if (error) *error = std::move(why);
        return std::nullopt;
    };
    std::ifstream in(file);
    if (!in) return fail("cannot open " + file);
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        return fail(std::string("skills.json: ") + e.what());
    }
    KSkillTable table;
    try {
        const nlohmann::json rows = j.value("rows", nlohmann::json::array());
        for (const auto& r : rows) {
            if (!r.is_object()) continue;
            std::unordered_map<std::string, std::string> cells;
            if (const auto c = r.find("cells"); c != r.end() && c->is_object()) {
                for (const auto& [col, v] : c->items()) {
                    if (v.is_string()) cells[col] = v.get<std::string>();
                }
            }
            KSkillRow row = KSkillRow::from_cells(cells);
            row.row = r.value("row", 0);
            row.max_level = r.value("max_level", 0);
            const int id = r.value("id", row.id);
            // KSkillManager::Init: 1..2000 and a style that is not negative; the last row of an id wins
            if (id < 1 || id > kMaxSkill || row.style < 0 || row.row <= 1) continue;
            row.id = id;
            table.info_[id] = std::move(row);
        }
        // attribconstdata.ini (jxassets export-skills): {"name": [Data0, Data1, ...]} by attribute name
        if (const auto d = j.find("attrib_data"); d != j.end() && d->is_object()) {
            for (const auto& [name, values] : d->items()) {
                const int id = magic_attrib_id(name);
                if (id < 0 || !values.is_array()) continue;
                std::vector<int> v;
                for (const auto& x : values) {
                    if (x.is_number_integer()) v.push_back(x.get<int>());
                }
                table.attrib_data_[id] = std::move(v);
            }
        }
    } catch (const std::exception& e) {
        return fail(std::string("skills.json fields: ") + e.what());
    }
    return table;
}

const KSkillRow* KSkillTable::info(int id) const
{
    const auto it = info_.find(id);
    return it == info_.end() ? nullptr : &it->second;
}

const std::vector<int>* KSkillTable::attrib_data(int attrib_id) const noexcept
{
    const auto it = attrib_data_.find(attrib_id);
    return it == attrib_data_.end() ? nullptr : &it->second;
}

const KSkill* KSkill::basic_attack(int id)
{
    // Skills.txt of the Linux server, rows 2 and 3 (every column the server reads), the level
    // numbers at 0 - what special\长兵物理攻击.lua / 远程物理攻击.lua of the JX1 fallback answer
    static const auto make = [](int skill_id) {
        std::unordered_map<std::string, std::string> cells = {
            {"SkillName", skill_id == 1 ? "Cong kich vat ly" : "Cong kich vat ly (xa)"},
            {"SkillId", std::to_string(skill_id)}, {"Attrib", "1"}, {"SkillStyle", "0"},
            {"StateSpecialId", "0"}, {"StatePriority", "0"}, {"IsAura", "0"}, {"LRSkill", "0"}, {"NeedShadow", "0"},
            {"AttackRadius", skill_id == 1 ? "100" : "320"}, {"MaxShadowNum", "0"}, {"MslsGenerate", "0"}, {"MslsGenerateData", "0"},
            {"CharClass", "0"}, {"MisslesForm", "1"}, {"ChildSkillId", skill_id == 1 ? "64" : "65"}, {"ChildSkillLevel", "-1"},
            {"ChildSkillNum", "1"}, {"BaseSkill", "1"}, {"CharAnimId", "9"}, {"EventSkillLevel", "0"}, {"IsMelee", "1"},
            {"WaitTime", "5"}, {"ClientSend", "0"}, {"SkillCostType", "0"}, {"CostValue", "0"}, {"TimePerCast", "0"},
            {"TimePerCastOnHorse", "0"}, {"IsPhysical", "1"}, {"TargetOnly", "1"}, {"TargetEnemy", "1"}, {"TargetAlly", "0"},
            {"TargetSelf", "0"}, {"TargetOther", "0"}, {"TargetObj", "0"}, {"TargetNoNpc", "0"}, {"ByMissle", "0"},
            {"IsUseAR", "1"}, {"StartEvent", "0"}, {"StartSkillId", "0"}, {"FlyEvent", "0"}, {"FlySkillId", "0"},
            {"FlyEventTime", "0"}, {"CollideEvent", "0"}, {"CollidSkillId", "0"}, {"VanishedEvent", "0"}, {"VanishedSkillId", "0"},
            {"ReqLevel", "0"}, {"MaxLevel", "0"}, {"EqtLimit", "-2"}, {"HorseLimit", "0"}, {"DoHurt", "80"}, {"WeaponSkill", "1"},
            {"Param1", "0"}, {"Param2", "0"}, {"PeaceCanUse", "0"}, {"ShowEvent", "0"}, {"IsExpSkill", "0"}, {"Series", "-1"},
            {"ShowAddition", "1"},
            {"LvlSetScript", skill_id == 1 ? "\\script\\skill\\special\\changbing_wuli_gongji.lua" : "\\script\\skill\\special\\yuancheng_wuli_gongji.lua"},
            {"LvlSetting1", "physicsenhance_p"}, {"LvlSetting2", "attackrating_p"}, {"LvlSetting3", "skill_cost_v"},
        };
        KSkill s;
        s.row = KSkillRow::from_cells(cells);
        s.row.row = skill_id + 1;
        s.level = 1;
        for (const std::string& setting : s.row.level_setting) {
            if (!setting.empty()) s.parse_string_to_magic_attrib(setting, "0");
        }
        return s;
    };
    static const KSkill melee = make(1);
    static const KSkill ranged = make(2);
    if (id == 1) return &melee;
    if (id == 2) return &ranged;
    return nullptr;
}

int KSkillTable::max_level(int id) const
{
    const KSkillRow* r = info(id);
    return r == nullptr ? 0 : r->max_level;
}

int KSkillTable::style(int id) const
{
    const KSkillRow* r = info(id);
    return r == nullptr || r->row <= 0 ? -1 : r->style;
}

KSkillManager::KSkillManager(std::shared_ptr<const KSkillTable> table, KScriptCache* scripts)
    : table_(std::move(table)), scripts_(scripts)
{
}

const KSkill* KSkillManager::get(int id, int level)
{
    if (id < 1 || id > kMaxSkill || level < 1 || level > kMaxSkillLevel || !table_) return nullptr;
    const std::uint32_t key = (static_cast<std::uint32_t>(id) << 8) | static_cast<std::uint32_t>(level);
    if (const auto it = skills_.find(key); it != skills_.end()) return it->second.get();
    const KSkillRow* info = table_->info(id);
    if (info == nullptr || info->row == 0) return nullptr;   // 0x080E6E3B: m_nTabFileRowId == 0
    // 0x080E6E43..: style < 0 -> none; 13 -> KThiefSkill (not carried); > 13 must be 14; else <= 4
    const int style = info->style;
    if (style < 0 || style == skill_style_thief || (style > 4 && style != skill_style_jx2_14)) return nullptr;
    auto skill = std::make_unique<KSkill>();
    skill->row = *info;                                       // 0x080E6ED3: the info block copied in
    skill->load_skill_level_data(level, scripts_);            // vtable + 0x14
    KSkill* raw = skill.get();
    skills_.emplace(key, std::move(skill));
    return raw;
}

} // namespace jx::zone
