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

struct KNpcTemplate {
    std::uint32_t id = 0;
    std::string name;   // UTF-8
    int kind = 0;
    int series = 0;
    int stature = 0;
    std::uint32_t stand_frame = 15;
    std::uint32_t stand_frame1 = 15;
    std::uint32_t walk_frame = 15;
    std::uint32_t run_frame = 15;
    std::uint32_t attack_frame = 20;   // the AttackSpeed column (KNpcTemplate.cpp reads it into m_AttackFrame)
    std::uint32_t hurt_frame = 10;
    std::uint32_t death_frame = 12;
    std::uint32_t hit_recover = 0;
    std::uint32_t revive_frame = 2400;
    std::uint32_t life_param = 1;
    std::uint32_t min_damage = 1;
    std::uint32_t max_damage = 3;
    std::uint32_t defense = 0;
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
