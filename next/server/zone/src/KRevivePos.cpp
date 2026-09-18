#include "jx/zone/KRevivePos.h"

#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>

namespace jx::zone {

std::optional<KRevivePosTable> KRevivePosTable::load(const std::string& file, std::string* error)
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
    KRevivePosTable t;
    try {
        const auto maps = j.find("maps");
        if (maps == j.end() || !maps->is_object()) {
            if (error) *error = "no maps object in " + file;
            return std::nullopt;
        }
        for (const auto& [key, value] : maps->items()) {
            const auto map_id = static_cast<std::uint32_t>(std::stoul(key));
            Map m;
            if (const auto r = value.find("region"); r != value.end() && r->is_array() && r->size() == 2) {
                m.region_lo = r->at(0).get<int>();
                m.region_hi = r->at(1).get<int>();
            }
            if (const auto p = value.find("points"); p != value.end() && p->is_object()) {
                for (const auto& [ref, xy] : p->items()) {
                    if (!xy.is_array() || xy.size() != 2) continue;
                    m.points[std::stoi(ref)] = Pos{xy.at(0).get<std::int32_t>(), xy.at(1).get<std::int32_t>()};
                }
            }
            t.maps[map_id] = std::move(m);
        }
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return std::nullopt;
    }
    return t;
}

std::optional<Pos> KRevivePosTable::point(std::uint32_t map, int ref) const noexcept
{
    const auto m = maps.find(map);
    if (m == maps.end()) return std::nullopt;
    const auto p = m->second.points.find(ref);
    if (p == m->second.points.end()) return std::nullopt;
    return p->second;
}

std::optional<std::pair<int, int>> KRevivePosTable::region(std::uint32_t map) const noexcept
{
    const auto m = maps.find(map);
    if (m == maps.end() || (m->second.region_lo == 0 && m->second.region_hi == 0)) return std::nullopt;
    return std::make_pair(m->second.region_lo, m->second.region_hi);
}

void KRevivePosTable::add(std::uint32_t map, int ref, Pos at)
{
    Map& m = maps[map];
    m.points[ref] = at;
    if (m.region_lo == 0 && m.region_hi == 0) m.region_lo = m.region_hi = ref;
    else {
        if (ref < m.region_lo) m.region_lo = ref;
        if (ref > m.region_hi) m.region_hi = ref;
    }
}

} // namespace jx::zone
