// KPathFinder - A* over the walkability grid, without allocating in the tick (MASTER SPEC 47, 48).
//
// The first version asked KMapData for a path, and every single query allocated and zeroed three
// arrays the size of the whole map (800 x 1056 cells = 7.6 MB for one query on Phượng Tường).
// With a crowd walking around that was the most expensive thing the server did - measured, not
// guessed: `drain_network` was 8.8 ms of a 10 ms tick.
//
// This keeps the buffers and marks visited cells with a generation stamp, so a query touches only
// the cells it really expands.  The algorithm and its results are unchanged (same octile costs,
// same smoothing), so movement stays deterministic.
//
// One finder belongs to one map instance: it is state, and state has exactly one owner (SPEC 6).
#pragma once

#include <cstdint>
#include <vector>

#include "jx/zone/KMapData.h"
#include "jx/zone/KRegion.h"

namespace jx::zone {

class KPathFinder {
public:
    KPathFinder() = default;
    explicit KPathFinder(const KMapData* map) { reset(map); }

    // Points the finder at a map (and sizes the buffers once).
    void reset(const KMapData* map);
    [[nodiscard]] const KMapData* map() const noexcept { return map_; }

    // Waypoints from `from` to `to` (last element == to when reachable), smoothed like the old
    // client did.  Empty when unreachable or the search budget is exhausted.
    [[nodiscard]] std::vector<Pos> find(Pos from, Pos to, std::size_t max_expand = 4000);

    // How many cells the last query expanded (metrics / tests).
    [[nodiscard]] std::size_t last_expanded() const noexcept { return last_expanded_; }
    [[nodiscard]] std::uint64_t queries() const noexcept { return queries_; }

private:
    struct Node {
        int f = 0, g = 0, x = 0, y = 0;
        bool operator>(const Node& o) const noexcept { return f > o.f; }
    };

    const KMapData* map_ = nullptr;
    std::vector<int> g_;                  // cost so far, valid when stamp_[i] == generation_
    std::vector<int> parent_;
    std::vector<std::uint32_t> stamp_;    // which query last touched the cell
    std::vector<std::uint32_t> closed_;   // closed in this query when == generation_
    std::vector<Node> heap_;              // reused priority queue storage
    std::vector<Pos> cells_;              // reused path buffer
    std::uint32_t generation_ = 0;
    std::size_t last_expanded_ = 0;
    std::uint64_t queries_ = 0;
};

} // namespace jx::zone
