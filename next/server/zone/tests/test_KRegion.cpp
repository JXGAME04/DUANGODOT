#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

#include "jx/zone/KRegion.h"

using jx::EntityId;
using jx::zone::KRegionGrid;
using jx::zone::Cell;
using jx::zone::Pos;

namespace {

std::vector<EntityId> sorted(std::vector<EntityId> v)
{
    std::sort(v.begin(), v.end());
    return v;
}

std::vector<EntityId> in_view(const KRegionGrid& g, Cell c)
{
    std::vector<EntityId> out;
    g.for_each_in_view(c, [&](EntityId id) { out.push_back(id); });
    return sorted(out);
}

} // namespace

TEST_CASE("cells are floor divisions of the position", "[aoi]")
{
    KRegionGrid g(512);
    CHECK(g.cell_of(Pos{0, 0}) == Cell{0, 0});
    CHECK(g.cell_of(Pos{511, 511}) == Cell{0, 0});
    CHECK(g.cell_of(Pos{512, 1023}) == Cell{1, 1});
    CHECK(g.cell_of(Pos{-1, -512}) == Cell{-1, -1});
    CHECK(g.cell_of(Pos{-513, 0}) == Cell{-2, 0});
    CHECK(g.in_view(Cell{0, 0}, Cell{1, 1}));
    CHECK_FALSE(g.in_view(Cell{0, 0}, Cell{2, 0}));
}

TEST_CASE("insert, move and remove keep the buckets consistent", "[aoi]")
{
    KRegionGrid g(100);
    const EntityId a{1}, b{2}, c{3};
    g.insert(a, Pos{10, 10});
    g.insert(b, Pos{150, 10});     // neighbour cell
    g.insert(c, Pos{350, 10});     // two cells away: not visible from a
    CHECK(g.size() == 3);
    CHECK(in_view(g, g.cell_of(Pos{10, 10})) == std::vector<EntityId>{a, b});
    CHECK(in_view(g, g.cell_of(Pos{350, 10})) == std::vector<EntityId>{c});

    Cell from, to;
    CHECK_FALSE(g.move(a, Pos{90, 90}, from, to));   // same cell
    CHECK(g.move(a, Pos{250, 10}, from, to));
    CHECK(from == Cell{0, 0});
    CHECK(to == Cell{2, 0});
    CHECK(*g.cell_of(a) == Cell{2, 0});
    CHECK(in_view(g, Cell{2, 0}) == std::vector<EntityId>{a, b, c});

    g.remove(b);
    CHECK(g.size() == 2);
    CHECK_FALSE(g.contains(b));
    CHECK(g.cell_of(b) == nullptr);
    CHECK(in_view(g, Cell{2, 0}) == std::vector<EntityId>{a, c});
    g.remove(b);   // idempotent
    CHECK(g.size() == 2);
}

TEST_CASE("view diff lists what appears and what vanishes when crossing cells", "[aoi]")
{
    KRegionGrid g(100);
    // one entity per column x = 0..5 on row 0
    for (std::int32_t i = 0; i < 6; ++i) g.insert(EntityId{static_cast<std::uint64_t>(i + 1)}, Pos{i * 100 + 50, 50});

    std::vector<EntityId> entered, left;
    g.view_diff(Cell{2, 0}, Cell{3, 0}, entered, left);          // one step right
    CHECK(sorted(entered) == std::vector<EntityId>{EntityId{5}});  // column 4 appears
    CHECK(sorted(left) == std::vector<EntityId>{EntityId{2}});     // column 1 vanishes

    g.view_diff(Cell{0, 0}, Cell{5, 0}, entered, left);          // teleport
    CHECK(sorted(entered) == std::vector<EntityId>{EntityId{5}, EntityId{6}});
    CHECK(sorted(left) == std::vector<EntityId>{EntityId{1}, EntityId{2}});

    g.view_diff(Cell{2, 0}, Cell{2, 0}, entered, left);
    CHECK(entered.empty());
    CHECK(left.empty());
}
