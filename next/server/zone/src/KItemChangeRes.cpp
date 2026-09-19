#include "jx/zone/KItemChangeRes.h"

#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>

namespace jx::zone {

std::optional<KItemChangeRes> KItemChangeRes::load(const std::string& file, std::string* error)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + file;
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return std::nullopt;
    }
    KItemChangeRes r;
    const auto table = [&](const char* key, Table& into) {
        const auto it = j.find(key);
        if (it == j.end() || !it->is_array()) return;
        for (const auto& row : *it) {
            std::vector<int> cells;
            if (row.is_array()) {
                for (const auto& c : row) cells.push_back(c.is_number_integer() ? c.get<int>() : 0);
            }
            into.push_back(std::move(cells));
        }
    };
    table("melee", r.melee);
    table("range", r.range);
    table("armor", r.armor);
    table("helm", r.helm);
    table("horse", r.horse);
    // 0x08068B70: rows 2.. of the file, key = (word col3 << 16) | word col1 (defaults 0), value col2 (default 2)
    const auto map = [&](const char* key, std::map<std::uint32_t, int>& into) {
        const auto it = j.find(key);
        if (it == j.end() || !it->is_array()) return;
        Table t;
        for (const auto& row : *it) {
            std::vector<int> cells;
            if (row.is_array()) {
                for (const auto& c : row) cells.push_back(c.is_number_integer() ? c.get<int>() : 0);
            }
            t.push_back(std::move(cells));
        }
        for (int row = 2; row <= static_cast<int>(t.size()); ++row) {
            const std::uint32_t id = static_cast<std::uint16_t>(get_integer(t, row, 1, 0));
            const std::uint32_t kind = static_cast<std::uint16_t>(get_integer(t, row, 3, 0));
            into[(kind << 16) | id] = get_integer(t, row, 2, 2);
        }
    };
    map("gold", r.gold);
    map("platina", r.platina);
    return r;
}

int KItemChangeRes::get_integer(const Table& t, int row, int col, int def) noexcept
{
    if (row < 1 || row > static_cast<int>(t.size())) return def;
    const auto& r = t[static_cast<std::size_t>(row - 1)];
    if (col < 1 || col > static_cast<int>(r.size())) return def;
    return r[static_cast<std::size_t>(col - 1)];
}

int KItemChangeRes::weapon_res(int detail, int particular, int level) const noexcept
{
    if (level == 0) return get_integer(melee, 2, 2, 2) - 2;
    const int row = particular * 10 + level;
    switch (detail) {
    case equip_meleeweapon: return get_integer(melee, row + 2, 2, 2) - 2;
    case equip_rangeweapon: return get_integer(range, row + 1, 2, 2) - 2;
    default: return 0;
    }
}

int KItemChangeRes::armor_res(int particular, int level) const noexcept
{
    return get_integer(armor, level == 0 ? 2 : particular * 10 + level + 2, 2, 19) - 2;
}

int KItemChangeRes::helm_res(int particular, int level) const noexcept
{
    return get_integer(helm, level == 0 ? 2 : particular * 10 + level + 2, 2, 19) - 2;
}

int KItemChangeRes::horse_res(int particular, int level) const noexcept
{
    if (level == 0) return -1;
    return get_integer(horse, particular * 10 + level + 2, 2, 2) - 2;
}

int KItemChangeRes::gold_res(int gen_param, int kind) const noexcept
{
    const std::uint32_t key = (static_cast<std::uint32_t>(kind) << 16) | static_cast<std::uint16_t>(gen_param + 1);
    const auto it = gold.find(key);
    return it == gold.end() ? -1 : it->second - 2;
}

int KItemChangeRes::platina_res(int gen_param, int kind, int tier) const noexcept
{
    const std::map<std::uint32_t, int>& m = tier <= 5 ? gold : platina;
    for (;;) {
        const std::uint32_t key = (static_cast<std::uint32_t>(kind) << 16) | static_cast<std::uint16_t>(gen_param + 1);
        const auto it = m.find(key);
        if (it != m.end()) return it->second - 2;
        if (kind == 0) return -1;
        kind = 0;   // 0x080687A8: tried once more as kind 0
    }
}

int KItemChangeRes::equip_res(const KItem* item, int part) const noexcept
{
    // 0x080685B0: the part -> the kind of the gold / platina keys (pairs at 0x8252ba0; unknown -> 0)
    static constexpr int kPartKind[] = {1, 2, 3, 5, 6, 7, 8, 9, 9, 10, 0};
    const int kind = part >= 0 && part < static_cast<int>(sizeof(kPartKind) / sizeof(kPartKind[0])) ? kPartKind[part] : 0;
    if (item != nullptr) {
        if (item->ex_type == 1) return gold_res(item->gen_param, kind);                          // 0x081FE158
        if (item->ex_type == 2) return platina_res(item->gen_param, kind, item->amulet_tier);   // 0x081FE180 (+4 == 4 there)
    }
    const int detail = item ? item->detail : 0;
    const int particular = item ? item->particular : 0;
    const int level = item ? item->level : 0;
    switch (part) {   // 0x08068AC0's jump table 0x8252b40
    case itempart_head: return helm_res(particular, level);
    case itempart_body: return armor_res(particular, level);
    case itempart_weapon: return weapon_res(detail, particular, level);
    case itempart_horse: return horse_res(particular, level);
    case itempart_mantle: return -1;
    default: return 0;
    }
}

}  // namespace jx::zone
