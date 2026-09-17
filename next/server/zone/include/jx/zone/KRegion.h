// Area of interest: a uniform grid.  An entity in cell C is visible to every player whose cell is
// within view_cells of C (Chebyshev distance) and vice versa, so visibility is symmetric.
#pragma once

#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <vector>

#include "jx/ids.hpp"

namespace jx::zone {

struct Pos {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const Pos&) const noexcept = default;
};

struct Cell {
    std::int32_t cx = 0;
    std::int32_t cy = 0;
    constexpr bool operator==(const Cell&) const noexcept = default;
};

class KRegionGrid {
public:
    explicit KRegionGrid(std::int32_t cell_size, std::int32_t view_cells = 1);

    [[nodiscard]] Cell cell_of(Pos p) const noexcept;
    [[nodiscard]] const Cell* cell_of(EntityId id) const;
    [[nodiscard]] bool in_view(Cell a, Cell b) const noexcept
    {
        return std::abs(a.cx - b.cx) <= view_ && std::abs(a.cy - b.cy) <= view_;
    }
    [[nodiscard]] std::int32_t cell_size() const noexcept { return cell_size_; }
    [[nodiscard]] std::int32_t view_cells() const noexcept { return view_; }
    [[nodiscard]] std::size_t size() const noexcept { return where_.size(); }
    [[nodiscard]] bool contains(EntityId id) const { return where_.contains(id); }

    void insert(EntityId id, Pos p);
    void remove(EntityId id);
    // Re-files id under the cell of p.  Returns true when the cell changed (from/to filled).
    bool move(EntityId id, Pos p, Cell& from, Cell& to);

    // Visits every entity in the view neighbourhood of c (including c itself).
    template <class Fn>
    void for_each_in_view(Cell c, Fn&& fn) const
    {
        for (std::int32_t cy = c.cy - view_; cy <= c.cy + view_; ++cy) {
            for (std::int32_t cx = c.cx - view_; cx <= c.cx + view_; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second) fn(id);
            }
        }
    }

    // Entities that become visible when moving from 'from' to 'to' (entered) and those that stop
    // being visible (left).  The moving entity itself may appear in the lists; callers filter it.
    void view_diff(Cell from, Cell to, std::vector<EntityId>& entered, std::vector<EntityId>& left) const;

private:
    static std::uint64_t key(Cell c) noexcept
    {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.cx)) << 32) |
               static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.cy));
    }

    std::int32_t cell_size_;
    std::int32_t view_;
    std::unordered_map<std::uint64_t, std::vector<EntityId>> cells_;
    std::unordered_map<EntityId, Cell, IdHash> where_;
};

} // namespace jx::zone
