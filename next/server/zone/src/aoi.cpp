#include "jx/zone/aoi.hpp"

#include <algorithm>

namespace jx::zone {

AoiGrid::AoiGrid(std::int32_t cell_size, std::int32_t view_cells)
    : cell_size_(cell_size > 0 ? cell_size : 1), view_(view_cells >= 0 ? view_cells : 0)
{
}

Cell AoiGrid::cell_of(Pos p) const noexcept
{
    // floor division so negative coordinates map to a stable cell as well
    const auto fdiv = [](std::int32_t a, std::int32_t b) noexcept {
        return (a >= 0) ? a / b : -((-a + b - 1) / b);
    };
    return Cell{fdiv(p.x, cell_size_), fdiv(p.y, cell_size_)};
}

const Cell* AoiGrid::cell_of(EntityId id) const
{
    const auto it = where_.find(id);
    return it == where_.end() ? nullptr : &it->second;
}

void AoiGrid::insert(EntityId id, Pos p)
{
    remove(id);
    const Cell c = cell_of(p);
    cells_[key(c)].push_back(id);
    where_[id] = c;
}

void AoiGrid::remove(EntityId id)
{
    const auto it = where_.find(id);
    if (it == where_.end()) return;
    const auto bucket = cells_.find(key(it->second));
    if (bucket != cells_.end()) {
        auto& v = bucket->second;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        if (v.empty()) cells_.erase(bucket);
    }
    where_.erase(it);
}

bool AoiGrid::move(EntityId id, Pos p, Cell& from, Cell& to)
{
    const auto it = where_.find(id);
    if (it == where_.end()) {
        insert(id, p);
        from = to = cell_of(p);
        return false;
    }
    from = it->second;
    to = cell_of(p);
    if (from == to) return false;
    remove(id);
    cells_[key(to)].push_back(id);
    where_[id] = to;
    return true;
}

void AoiGrid::view_diff(Cell from, Cell to, std::vector<EntityId>& entered, std::vector<EntityId>& left) const
{
    entered.clear();
    left.clear();
    for (std::int32_t cy = to.cy - view_; cy <= to.cy + view_; ++cy) {
        for (std::int32_t cx = to.cx - view_; cx <= to.cx + view_; ++cx) {
            const Cell c{cx, cy};
            if (in_view(from, c)) continue;
            const auto it = cells_.find(key(c));
            if (it != cells_.end()) entered.insert(entered.end(), it->second.begin(), it->second.end());
        }
    }
    for (std::int32_t cy = from.cy - view_; cy <= from.cy + view_; ++cy) {
        for (std::int32_t cx = from.cx - view_; cx <= from.cx + view_; ++cx) {
            const Cell c{cx, cy};
            if (in_view(to, c)) continue;
            const auto it = cells_.find(key(c));
            if (it != cells_.end()) left.insert(left.end(), it->second.begin(), it->second.end());
        }
    }
}

} // namespace jx::zone
