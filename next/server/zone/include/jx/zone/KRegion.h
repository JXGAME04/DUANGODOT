// Area of interest: a uniform grid.  An entity in cell C is visible to every player whose cell is
// within view_x cells horizontally and view_y cells vertically, and vice versa, so visibility stays
// symmetric.
//
// The two counts are separate because the world is not seen as a square.  The old renderer draws
// a scene point at (x, y/2), so a 1024 x 768 screen shows 1024 units of x but 1536 units of y: the
// area a player must be told about is a rectangle, taller than it is wide in world units.  The
// counts are derived from that rectangle in KSubWorld, never guessed here.
#pragma once

#include <algorithm>
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
    // view_y defaults to view_x, which keeps the old square behaviour for callers that do not care.
    explicit KRegionGrid(std::int32_t cell_size, std::int32_t view_x = 1, std::int32_t view_y = -1);

    [[nodiscard]] Cell cell_of(Pos p) const noexcept;
    [[nodiscard]] const Cell* cell_of(EntityId id) const;
    [[nodiscard]] bool in_view(Cell a, Cell b) const noexcept
    {
        return std::abs(a.cx - b.cx) <= view_x_ && std::abs(a.cy - b.cy) <= view_y_;
    }
    [[nodiscard]] std::int32_t cell_size() const noexcept { return cell_size_; }
    [[nodiscard]] std::int32_t view_cells() const noexcept { return view_x_; }
    [[nodiscard]] std::int32_t view_cells_x() const noexcept { return view_x_; }
    [[nodiscard]] std::int32_t view_cells_y() const noexcept { return view_y_; }
    [[nodiscard]] std::size_t size() const noexcept { return where_.size(); }
    [[nodiscard]] bool contains(EntityId id) const { return where_.contains(id); }

    // `player` marks an entity that makes its neighbourhood interesting: the world keeps the
    // areas around players awake and lets the rest sleep (MASTER SPEC 44, 45).
    void insert(EntityId id, Pos p, bool player = false);
    void remove(EntityId id);
    // Re-files id under the cell of p.  Returns true when the cell changed (from/to filled).
    bool move(EntityId id, Pos p, Cell& from, Cell& to);

    // One cell of a view, as for_each_view_cell hands it out: who is in it and its versions.  The
    // versions change whenever something is filed in the cell or taken away, so a viewer that
    // remembers them knows which cells hold anything it has not looked at yet (KInterest.cpp).
    struct CellView {
        const std::vector<EntityId>* players;
        const std::vector<EntityId>* others;
        std::uint32_t players_version;
        std::uint32_t others_version;
    };
    [[nodiscard]] std::size_t view_cell_count() const noexcept
    {
        return static_cast<std::size_t>(2 * view_x_ + 1) * static_cast<std::size_t>(2 * view_y_ + 1);
    }
    // fn(index, CellView) for every cell in view of c, in a fixed order (index 0 .. count - 1); a
    // cell nothing was ever filed in comes as empty lists with versions 0.
    template <class Fn>
    void for_each_view_cell(Cell c, Fn&& fn) const
    {
        static const std::vector<EntityId> none;
        std::size_t index = 0;
        for (std::int32_t cy = c.cy - view_y_; cy <= c.cy + view_y_; ++cy) {
            for (std::int32_t cx = c.cx - view_x_; cx <= c.cx + view_x_; ++cx, ++index) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) {
                    fn(index, CellView{&none, &none, 0, 0});
                } else {
                    fn(index, CellView{&it->second.players, &it->second.others, it->second.players_version, it->second.others_version});
                }
            }
        }
    }

    // How many players stand in this cell, and where they are: the caller builds its awake set.
    [[nodiscard]] std::size_t players_in(Cell c) const;
    template <class Fn>
    void for_each_player_cell(Fn&& fn) const
    {
        for (const auto& [k, count] : players_) {
            if (count > 0) fn(Cell{static_cast<std::int32_t>(k >> 32), static_cast<std::int32_t>(k & 0xFFFFFFFFull)});
        }
    }
    [[nodiscard]] std::size_t player_cells() const noexcept { return players_.size(); }

    // Visits every entity in the view neighbourhood of c (including c itself): players first,
    // then everything else, cell by cell.
    template <class Fn>
    void for_each_in_view(Cell c, Fn&& fn) const
    {
        for (std::int32_t cy = c.cy - view_y_; cy <= c.cy + view_y_; ++cy) {
            for (std::int32_t cx = c.cx - view_x_; cx <= c.cx + view_x_; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second.players) fn(id);
                for (const EntityId id : it->second.others) fn(id);
            }
        }
    }

    // The same, one kind at a time.  The interest management asks for them separately: in a crowd
    // a client that already knows all the players it is allowed to know must still find a monster
    // that appears, without walking over three thousand players to get to it.
    template <class Fn>
    void for_each_player_in_view(Cell c, Fn&& fn) const
    {
        for (std::int32_t cy = c.cy - view_y_; cy <= c.cy + view_y_; ++cy) {
            for (std::int32_t cx = c.cx - view_x_; cx <= c.cx + view_x_; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second.players) fn(id);
            }
        }
    }
    template <class Fn>
    void for_each_other_in_view(Cell c, Fn&& fn) const
    {
        for (std::int32_t cy = c.cy - view_y_; cy <= c.cy + view_y_; ++cy) {
            for (std::int32_t cx = c.cx - view_x_; cx <= c.cx + view_x_; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second.others) fn(id);
            }
        }
    }

    // Visits every entity filed in a cell that touches the square of half-side `radius` around p
    // (a superset of the circle; callers measure the exact distance).
    template <class Fn>
    void for_each_within(Pos p, std::int32_t radius, Fn&& fn) const
    {
        const Cell a = cell_of(Pos{p.x - radius, p.y - radius});
        const Cell b = cell_of(Pos{p.x + radius, p.y + radius});
        for (std::int32_t cy = a.cy; cy <= b.cy; ++cy) {
            for (std::int32_t cx = a.cx; cx <= b.cx; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second.players) fn(id);
                for (const EntityId id : it->second.others) fn(id);
            }
        }
    }
    template <class Fn>
    void for_each_player_within(Pos p, std::int32_t radius, Fn&& fn) const
    {
        const Cell a = cell_of(Pos{p.x - radius, p.y - radius});
        const Cell b = cell_of(Pos{p.x + radius, p.y + radius});
        for (std::int32_t cy = a.cy; cy <= b.cy; ++cy) {
            for (std::int32_t cx = a.cx; cx <= b.cx; ++cx) {
                const auto it = cells_.find(key(Cell{cx, cy}));
                if (it == cells_.end()) continue;
                for (const EntityId id : it->second.players) fn(id);
            }
        }
    }

    // The same, own cell first and then ring by ring outwards, and fn says whether to go on
    // (return false to stop).  For "a few players near this spot": in a packed place the own cell
    // alone holds hundreds, so a caller that needs four stops after four instead of walking the
    // thousand within the radius.
    template <class Fn>
    void for_each_player_near(Pos p, std::int32_t radius, Fn&& fn) const
    {
        const Cell c = cell_of(p);
        const Cell a = cell_of(Pos{p.x - radius, p.y - radius});
        const Cell b = cell_of(Pos{p.x + radius, p.y + radius});
        const std::int32_t rings = std::max({c.cx - a.cx, b.cx - c.cx, c.cy - a.cy, b.cy - c.cy, 0});
        for (std::int32_t r = 0; r <= rings; ++r) {
            for (std::int32_t cy = c.cy - r; cy <= c.cy + r; ++cy) {
                for (std::int32_t cx = c.cx - r; cx <= c.cx + r; ++cx) {
                    if (r > 0 && cy != c.cy - r && cy != c.cy + r && cx != c.cx - r && cx != c.cx + r) continue;   // inside the ring
                    if (cx < a.cx || cx > b.cx || cy < a.cy || cy > b.cy) continue;
                    const auto it = cells_.find(key(Cell{cx, cy}));
                    if (it == cells_.end()) continue;
                    for (const EntityId id : it->second.players) {
                        if (!fn(id)) return;
                    }
                }
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
    std::int32_t view_x_;
    std::int32_t view_y_;
    // Players and everything else are filed apart, and each entity remembers its slot, so taking
    // one out of a cell that holds three thousand costs the same as out of a cell that holds three.
    struct Bucket {
        std::vector<EntityId> players;
        std::vector<EntityId> others;
        // bumped whenever a player / another entity is filed here or taken away: a client whose
        // view cells kept their versions has nothing new to look at (KInterest.cpp)
        std::uint32_t players_version = 0;
        std::uint32_t others_version = 0;
    };
    struct Placed {
        Cell cell;
        bool player = false;
        std::uint32_t slot = 0;   // index in the bucket's list of its kind
    };
    void file(EntityId id, Cell c, bool player);
    std::unordered_map<std::uint64_t, Bucket> cells_;
    std::unordered_map<EntityId, Placed, IdHash> where_;
    std::unordered_map<std::uint64_t, std::uint32_t> players_;   // cell -> players standing there
};

} // namespace jx::zone
