#include "jx/zone/KPathFinder.h"

#include <algorithm>
#include <cstdlib>

namespace jx::zone {

void KPathFinder::reset(const KMapData* map)
{
    map_ = map;
    generation_ = 0;
    last_expanded_ = 0;
    if (map_ == nullptr) {
        g_.clear();
        parent_.clear();
        stamp_.clear();
        closed_.clear();
        return;
    }
    const std::size_t total = static_cast<std::size_t>(map_->cells_x) * static_cast<std::size_t>(map_->cells_y);
    g_.assign(total, -1);
    parent_.assign(total, -1);
    stamp_.assign(total, 0);
    closed_.assign(total, 0);
}

std::vector<Pos> KPathFinder::find(Pos from, Pos to, std::size_t max_expand)
{
    last_expanded_ = 0;
    if (map_ == nullptr) return {to};
    const KMapData& m = *map_;
    const int sx = from.x / m.cell, sy = from.y / m.cell;
    const int tx = to.x / m.cell, ty = to.y / m.cell;
    if (!m.in_bounds(sx, sy) || !m.walkable_cell(tx, ty)) return {};
    if (sx == tx && sy == ty) return {to};
    if (m.line_of_sight(from, to)) return {to};

    ++queries_;
    if (++generation_ == 0) {   // wrapped: start over with clean stamps
        std::fill(stamp_.begin(), stamp_.end(), 0);
        std::fill(closed_.begin(), closed_.end(), 0);
        generation_ = 1;
    }
    const std::uint32_t gen = generation_;
    const auto idx = [&](int x, int y) {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m.cells_x) + static_cast<std::size_t>(x);
    };
    const auto heuristic = [&](int x, int y) {
        const int dx = std::abs(x - tx), dy = std::abs(y - ty);
        return 10 * (dx + dy) - 6 * std::min(dx, dy);   // octile with 10/14 costs
    };
    const auto known_g = [&](std::size_t i) { return stamp_[i] == gen ? g_[i] : -1; };

    heap_.clear();
    const auto push = [&](Node n) {
        heap_.push_back(n);
        std::push_heap(heap_.begin(), heap_.end(), std::greater<Node>{});
    };
    const auto pop = [&]() {
        std::pop_heap(heap_.begin(), heap_.end(), std::greater<Node>{});
        const Node n = heap_.back();
        heap_.pop_back();
        return n;
    };

    const std::size_t start = idx(sx, sy);
    stamp_[start] = gen;
    g_[start] = 0;
    parent_[start] = -1;
    push(Node{heuristic(sx, sy), 0, sx, sy});

    std::size_t expanded = 0;
    bool found = false;
    static constexpr int kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static constexpr int kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    while (!heap_.empty()) {
        const Node cur = pop();
        const std::size_t ci = idx(cur.x, cur.y);
        if (closed_[ci] == gen) continue;
        closed_[ci] = gen;
        if (cur.x == tx && cur.y == ty) {
            found = true;
            break;
        }
        if (++expanded > max_expand) break;
        for (int d = 0; d < 8; ++d) {
            const int nx = cur.x + kDx[d], ny = cur.y + kDy[d];
            if (!m.walkable_cell(nx, ny)) continue;
            if (d >= 4 && (!m.walkable_cell(cur.x + kDx[d], cur.y) || !m.walkable_cell(cur.x, cur.y + kDy[d]))) continue;
            const std::size_t ni = idx(nx, ny);
            if (closed_[ni] == gen) continue;
            const int ng = cur.g + (d >= 4 ? 14 : 10);
            const int had = known_g(ni);
            if (had >= 0 && ng >= had) continue;
            stamp_[ni] = gen;
            g_[ni] = ng;
            parent_[ni] = static_cast<int>(ci);
            push(Node{ng + heuristic(nx, ny), ng, nx, ny});
        }
    }
    last_expanded_ = expanded;
    if (!found) return {};

    cells_.clear();
    for (int i = static_cast<int>(idx(tx, ty)); i >= 0; i = parent_[static_cast<std::size_t>(i)]) {
        cells_.push_back(m.cell_center(i % m.cells_x, i / m.cells_x));
        if (i == static_cast<int>(start)) break;
    }
    std::reverse(cells_.begin(), cells_.end());
    cells_.erase(cells_.begin());            // the start cell
    if (!cells_.empty()) cells_.back() = to; // finish exactly on the requested point
    return m.smooth_path(from, cells_);
}

} // namespace jx::zone
