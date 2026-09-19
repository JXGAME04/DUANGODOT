// KNpcTemplate of the old core (Settings/npcs.txt): the per-template numbers the zone simulates
// with.  Read from the exported npcres/npcs.json (jxassets export-npcres) so the client and the
// zone use the same table.  The level-dependent numbers (life, damage, exp, resistances, skill
// levels) come from the template's level script exactly like KNpcTemplate::InitNpcLevelData
// (see level_data); without the scripts the raw *Param columns serve as placeholders.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "jx/zone/KNpcDropRate.h"

namespace jx::zone {

class KScriptCache;

// One of the Skill1..4 / Level1..4 slots (KSkillList::m_Skills) plus what skills.txt says about it.
struct KNpcTemplateSkill {
    int id = 0;
    double level_a = 0;        // level = a + b * npc level (GetNpcLevelData -> GetData) when no script runs
    double level_b = 0;
    bool known = false;        // present in skills.txt (an unknown skill keeps m_CurrentAttackRadius)
    int attack_radius = 0;     // KSkill::GetAttackRadius
    bool melee = false;        // IsMelee: DoAttack (AttackFrame) instead of a cast (CastFrame)
    bool target_self = false;  // TargetSelf: a heal / buff on the caster
    int style = 0;
};

struct KNpcTemplate {
    std::uint32_t id = 0;
    std::string name;   // UTF-8
    int kind = 0;       // NPCKIND (0 normal, 1 player, 2 partner, 3 dialoger, 4 bird, 5 mouse)
    int camp = 4;       // NPCCAMP (KNpc::Init: camp_free)
    int series = 0;
    int stature = 0;
    std::uint32_t stand_frame = 15;
    std::uint32_t stand_frame1 = 15;
    std::uint32_t walk_frame = 15;
    std::uint32_t run_frame = 15;
    std::uint32_t attack_frame = 20;   // the AttackSpeed column (KNpcTemplate.cpp reads it into m_AttackFrame)
    std::uint32_t cast_frame = 20;     // the CastSpeed column (m_CastFrame)
    std::uint32_t hurt_frame = 10;
    std::uint32_t death_frame = 12;
    std::uint32_t hit_recover = 0;
    std::uint32_t revive_frame = 2400;
    std::uint32_t life_param = 1;      // placeholders when no level script runs
    std::uint32_t min_damage = 1;
    std::uint32_t max_damage = 3;
    std::uint32_t defense = 0;
    // server side (KNpcTemplate.cpp #ifdef _SERVER): movement and the KNpcAI columns
    int walk_speed = 5;                // scene units per frame (KNpc::ServeMove)
    int run_speed = 10;
    int ai_mode = 0;                   // AIMode: 1..3 active, 4..6 passive, 0 none
    int ai_param[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 5};   // AIParam1..10 (m_AiParam[0..9])
    std::uint32_t ai_max_time = 25;    // AIMaxTime: frames between two decisions
    int vision_radius = 40;            // VisionRadius
    int active_radius = 30;            // ActiveRadius
    KNpcTemplateSkill skills[5];       // slots 1..4; 0 unused
    // the LevelScript column and the raw cells InitNpcLevelData hands to it (ExpParam..3,
    // LifeParam..3, LifeReplenish, ARParam..3, DefenseParam..3, Min/MaxDamageParam..3, *Resist, Level1..4)
    std::string level_script;          // "" = NPC_LEVELSCRIPT_FILENAME
    int treasure = 0;                  // Treasure: how many drop rolls its death is worth (KNpc::m_CurrentTreasure)
    std::string drop_rate_file;        // DropRateFile: the drop table (lower-cased game path), "" = none
    std::unordered_map<std::string, std::string> cells;
    [[nodiscard]] std::string cell(const std::string& column) const
    {
        const auto it = cells.find(column);
        return it == cells.end() ? std::string{} : it->second;
    }
};

// What KNpcTemplate::InitNpcLevelData computes for one (template, level, series).
struct KNpcLevelData {
    bool from_script = false;          // false = placeholders (no script cache or script)
    std::uint32_t exp = 0;             // m_Experience
    std::uint32_t life_max = 0;        // m_LifeMax
    int life_replenish = 0;            // m_LifeReplenish
    std::uint32_t attack_rating = 0;   // m_AttackRating
    std::uint32_t defend = 0;          // m_Defend
    std::uint32_t min_damage = 0;      // m_PhysicsDamage.nValue[0]
    std::uint32_t max_damage = 0;      // m_PhysicsDamage.nValue[2]
    int fire_resist = 0;
    int cold_resist = 0;
    int light_resist = 0;
    int poison_resist = 0;
    int physics_resist = 0;
    int skill_level[5] = {};           // KSkillList::SetNpcSkill levels (0 = slot unusable)
    // InitNpcLevelData 0x080A37A0 (jx_linux_y): the AuraSkillId / PasstSkillId columns with their level cells run through
    // the level script ("AuraSkillLevel" / "PasstSkillLevel": a + b * level) and clamped to 64 (0x080A3AB2 / 0x080A3B0A);
    // a level of 0 drops the id (+0x10fc/+0x1100, +0x1104/+0x1108 of the level record)
    int aura_skill_id = 0;
    int aura_skill_level = 0;
    int passive_skill_id = 0;
    int passive_skill_level = 0;
};

class KNpcTemplateSet {
public:
    // Loads npcres/npcs.json; on failure returns nullopt and puts the reason in *error.
    static std::optional<KNpcTemplateSet> load(const std::string& file, std::string* error);
    [[nodiscard]] const KNpcTemplate* find(std::uint32_t id) const;
    // the drop table a template names (npcs.json "droprates"), null when it was not exported
    [[nodiscard]] const KNpcDropRate* drop_rate(const std::string& file) const;
    void add_drop_rate(std::string file, KNpcDropRate table) { droprates_[std::move(file)] = std::move(table); }
    [[nodiscard]] std::size_t drop_rate_count() const noexcept { return droprates_.size(); }
    [[nodiscard]] std::size_t size() const noexcept { return templates_.size(); }
    void add(KNpcTemplate t) { templates_[t.id] = std::move(t); }

    // KNpcTemplate::InitNpcLevelData through the template's level script (or the default one);
    // with scripts == nullptr or no script the placeholders of the raw columns are returned.
    [[nodiscard]] static KNpcLevelData level_data(const KNpcTemplate& t, int level, int series, KScriptCache* scripts);

private:
    std::unordered_map<std::uint32_t, KNpcTemplate> templates_;
    std::unordered_map<std::string, KNpcDropRate> droprates_;
};

} // namespace jx::zone
