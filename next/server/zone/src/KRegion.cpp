#include "jx/zone/KRegion.h"

#include <algorithm>

namespace jx::zone {

KRegionGrid::KRegionGrid(std::int32_t cell_size, std::int32_t view_cells)
    : cell_size_(cell_size > 0 ? cell_size : 1), view_(view_cells >= 0 ? view_cells : 0)
{
}

Cell KRegionGrid::cell_of(Pos p) const noexcept
{
    // floor division so negative coordinates map to a stable cell as well
    const auto fdiv = [](std::int32_t a, std::int32_t b) noexcept {
        return (a >= 0) ? a / b : -((-a + b - 1) / b);
    };
    return Cell{fdiv(p.x, cell_size_), fdiv(p.y, cell_size_)};
}

const Cell* KRegionGrid::cell_of(EntityId id) const
{
    const auto it = where_.find(id);
    return it == where_.end() ? nullptr : &it->second.cell;
}

void KRegionGrid::insert(EntityId id, Pos p, bool player)
{
    remove(id);
    const Cell c = cell_of(p);
    cells_[key(c)].push_back(id);
    where_[id] = Placed{c, player};
    if (player) ++players_[key(c)];
}

void KRegionGrid::remove(EntityId id)
{
    const auto it = where_.find(id);
    if (it == where_.end()) return;
    const Cell c = it->second.cell;
    const auto bucket = cells_.find(key(c));
    if (bucket != cells_.end()) {
        auto& v = bucket->second;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        if (v.empty()) cells_.erase(bucket);
    }
    if (it->second.player) {
        const auto pit = players_.find(key(c));
        if (pit != players_.end() && --pit->second == 0) players_.erase(pit);
    }
    where_.erase(it);
}

bool KRegionGrid::move(EntityId id, Pos p, Cell& from, Cell& to)
{
    const auto it = where_.find(id);
    if (it == where_.end()) {
        insert(id, p);
        from = to = cell_of(p);
        return false;
    }
    from = it->second.cell;
    to = cell_of(p);
    if (from == to) return false;
    const bool player = it->second.player;
    remove(id);
    cells_[key(to)].push_back(id);
    where_[id] = Placed{to, player};
    if (player) ++players_[key(to)];
    return true;
}

std::size_t KRegionGrid::players_in(Cell c) const
{
    const auto it = players_.find(key(c));
    return it == players_.end() ? 0u : it->second;
}

void KRegionGrid::view_diff(Cell from, Cell to, std::vector<EntityId>& entered, std::vector<EntityId>& left) const
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
