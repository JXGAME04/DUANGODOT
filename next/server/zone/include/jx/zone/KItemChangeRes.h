#pragma once
// KItemChangeRes: what a worn piece makes a character look like - the equipment row of the main characters' part tables
// (Settings/npcres) that a helm / armour / weapon / horse selects.  jx_linux_y keeps the five tables in KItemSet
// (0x08068D90: MeleeRes +0, RangeRes +0x20, ArmorRes +0x40, HelmRes +0x60, HorseRes +0x80) and two maps for the gold /
// platina pieces (GoldEquipRes.txt +0xa0, PlatinaEquipRes.txt +0xb8; 0x08068B70 loads (col3 << 16 | col1) -> col2); the
// old client's KItemChangeRes.cpp reads the same files.  jxassets export-item-res writes them as items/item_res.json.
#include "jx/zone/KItem.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace jx::zone {

struct KItemChangeRes {
    using Table = std::vector<std::vector<int>>;   // every row of the file (row 1 = header), C atoi of each cell
    Table melee, range, armor, helm, horse;
    std::map<std::uint32_t, int> gold;             // (kind << 16 | id) -> col2 of GoldEquipRes.txt (0x08068B70)
    std::map<std::uint32_t, int> platina;

    static std::optional<KItemChangeRes> load(const std::string& file, std::string* error);

    // KTabFile::GetInteger(row, col, default): 1-based, the default outside the table
    static int get_integer(const Table& t, int row, int col, int def) noexcept;

    // 0x08068A00(set, detail, particular, level): level 0 -> MeleeRes row 2 (bare hands); melee -> row particular*10 + level + 2
    // of MeleeRes; range -> row particular*10 + level + 1 of RangeRes; other kinds -> 0 (an unset cell in the binary); col 2,
    // default 2; minus 2
    [[nodiscard]] int weapon_res(int detail, int particular, int level) const noexcept;
    // 0x08068980 (ArmorRes, default 19) / 0x08068900 (HelmRes, default 19): level 0 -> row 2, else row particular*10 + level + 2; minus 2
    [[nodiscard]] int armor_res(int particular, int level) const noexcept;
    [[nodiscard]] int helm_res(int particular, int level) const noexcept;
    // 0x080688B0 (HorseRes, default 2): level 0 -> -1 (no horse), else row particular*10 + level + 2 minus 2
    [[nodiscard]] int horse_res(int particular, int level) const noexcept;
    // 0x080686A0(set, gen_param, kind): the gold map at (kind << 16 | gen_param + 1) -> value - 2, else -1
    [[nodiscard]] int gold_res(int gen_param, int kind) const noexcept;
    // 0x08068730(set, gen_param, kind, tier): tier <= 5 -> the gold map, above -> the platina map; a miss with kind != 0
    // is tried again with kind 0; -1 when neither has it
    [[nodiscard]] int platina_res(int gen_param, int kind, int tier) const noexcept;

    // 0x081FE0E0(list, item, part): the row a piece (null = nothing worn) gives the part it sits on - a gold piece (+4 == 1)
    // through the gold map, a platina one (+4 == 4) through platina_res, the rest through the five tables by detail /
    // particular / level (0x08068AC0: part 0 helm, 1 armour, 3 weapon, 10 horse, 12 mantle = -1, others 0)
    [[nodiscard]] int equip_res(const KItem* item, int part) const noexcept;
};

}  // namespace jx::zone
