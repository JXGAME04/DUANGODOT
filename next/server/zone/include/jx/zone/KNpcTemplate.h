// KNpcTemplate of the old core (Settings/npcs.txt): the per-template numbers the zone simulates
// with.  Read from the exported npcres/npcs.json (jxassets export-npcres) so the client and the
// zone use the same table.  Life / damage still wait for the level scripts (Lua): until then the
// raw *Param columns are used as placeholders (see docs/NPCRES.md).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace jx::zone {

// One of the Skill1..4 / Level1..4 slots (KSkillList::m_Skills) plus what skills.txt says about it.
struct KNpcTemplateSkill {
    int id = 0;
    double level_a = 0;        // level = a + b * npc level (GetNpcLevelData -> GetData)
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
    std::uint32_t life_param = 1;
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
};

class KNpcTemplateSet {
public:
    // Loads npcres/npcs.json; on failure returns nullopt and puts the reason in *error.
    static std::optional<KNpcTemplateSet> load(const std::string& file, std::string* error);
    [[nodiscard]] const KNpcTemplate* find(std::uint32_t id) const;
    [[nodiscard]] std::size_t size() const noexcept { return templates_.size(); }
    void add(KNpcTemplate t) { templates_[t.id] = std::move(t); }

private:
    std::unordered_map<std::uint32_t, KNpcTemplate> templates_;
};

} // namespace jx::zone
