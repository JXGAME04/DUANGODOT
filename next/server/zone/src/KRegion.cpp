#include "jx/zone/KRegion.h"

#include <algorithm>

namespace jx::zone {

KRegionGrid::KRegionGrid(std::int32_t cell_size, std::int32_t view_x, std::int32_t view_y)
    : cell_size_(cell_size > 0 ? cell_size : 1),
      view_x_(view_x >= 0 ? view_x : 0),
      view_y_(view_y < 0 ? (view_x >= 0 ? view_x : 0) : view_y)
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

void KRegionGrid::file(EntityId id, Cell c, bool player)
{
    Bucket& b = cells_[key(c)];
    std::vector<EntityId>& list = player ? b.players : b.others;
    where_[id] = Placed{c, player, static_cast<std::uint32_t>(list.size())};
    list.push_back(id);
    ++(player ? b.players_version : b.others_version);
    if (player) ++players_[key(c)];
}

void KRegionGrid::insert(EntityId id, Pos p, bool player)
{
    remove(id);
    file(id, cell_of(p), player);
}

void KRegionGrid::remove(EntityId id)
{
    const auto it = where_.find(id);
    if (it == where_.end()) return;
    const Placed placed = it->second;
    where_.erase(it);
    const auto bucket = cells_.find(key(placed.cell));
    if (bucket != cells_.end()) {
        std::vector<EntityId>& list = placed.player ? bucket->second.players : bucket->second.others;
        if (placed.slot < list.size() && list[placed.slot] == id) {
            // the last one takes the free slot: no shifting, whatever the crowd
            const EntityId last = list.back();
            list[placed.slot] = last;
            list.pop_back();
            if (last != id) where_[last].slot = placed.slot;
        }
        // the bucket stays, empty, with its versions: a cell that emptied and filled again must not
        // look untouched to a client that summed the versions in between (a map has ~1000 cells)
        ++(placed.player ? bucket->second.players_version : bucket->second.others_version);
    }
    if (placed.player) {
        const auto pit = players_.find(key(placed.cell));
        if (pit != players_.end() && --pit->second == 0) players_.erase(pit);
    }
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
    file(id, to, player);
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
    for (std::int32_t cy = to.cy - view_y_; cy <= to.cy + view_y_; ++cy) {
        for (std::int32_t cx = to.cx - view_x_; cx <= to.cx + view_x_; ++cx) {
            const Cell c{cx, cy};
            if (in_view(from, c)) continue;
            const auto it = cells_.find(key(c));
            if (it == cells_.end()) continue;
            entered.insert(entered.end(), it->second.players.begin(), it->second.players.end());
            entered.insert(entered.end(), it->second.others.begin(), it->second.others.end());
        }
    }
    for (std::int32_t cy = from.cy - view_y_; cy <= from.cy + view_y_; ++cy) {
        for (std::int32_t cx = from.cx - view_x_; cx <= from.cx + view_x_; ++cx) {
            const Cell c{cx, cy};
            if (in_view(to, c)) continue;
            const auto it = cells_.find(key(c));
            if (it == cells_.end()) continue;
            left.insert(left.end(), it->second.players.begin(), it->second.players.end());
            left.insert(left.end(), it->second.others.begin(), it->second.others.end());
        }
    }
}

} // namespace jx::zone
