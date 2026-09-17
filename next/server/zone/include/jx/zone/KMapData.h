// Map bundle produced by jxassets export-map: map.json + obstacle.bin.
// The zone only needs the walkability grid (32x32 scene-unit cells), the spawn point and the
// npc placements; the client uses the same bundle for drawing.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "jx/zone/KRegion.h"

namespace jx::zone {

struct KNpcPlacement {
    std::uint32_t template_id = 0;
    std::string name;      // UTF-8
    Pos pos;               // scene units
    int frame = 0;
    int kind = 0;          // NPCKIND of the old GameDataDef.h: 0 monster, 3 dialoger, 4 bird, 5 mouse
    std::string script;
    int level = 0;
    int camp = 0;
    int series = 0;
    int dir = 0;           // facing 0..63
    bool client_only = false;   // Npc_C.dat ambient npc (the old client spawned it locally)
};

class KMapData {
public:
    // Loads <dir>/map.json and <dir>/obstacle.bin; returns nullopt and fills *error on failure.
    static std::optional<KMapData> load(const std::filesystem::path& dir, std::string* error);
    // Builds a synthetic map for tests: every cell walkable unless marked later.
    static KMapData synthetic(int cells_x, int cells_y, int cell = 32);

    int id = 0;
    std::string name;
    int cell = 32;
    int cells_x = 0, cells_y = 0;
    int scene_w = 0, scene_h = 0;
    Pos spawn;
    std::vector<std::uint8_t> obstacle;   // cells_x * cells_y, row major, 0 = walkable
    std::vector<KNpcPlacement> npcs;

    [[nodiscard]] bool in_bounds(int cx, int cy) const noexcept
    {
        return cx >= 0 && cy >= 0 && cx < cells_x && cy < cells_y;
    }
    [[nodiscard]] bool walkable_cell(int cx, int cy) const noexcept
    {
        return in_bounds(cx, cy) && obstacle[static_cast<std::size_t>(cy) * static_cast<std::size_t>(cells_x) + static_cast<std::size_t>(cx)] == 0;
    }
    void set_blocked(int cx, int cy, std::uint8_t kind = 1);
    [[nodiscard]] bool walkable(Pos p) const noexcept { return walkable_cell(p.x / cell, p.y / cell); }
    [[nodiscard]] Pos cell_center(int cx, int cy) const noexcept
    {
        return Pos{cx * cell + cell / 2, cy * cell + cell / 2};
    }
    // Closest walkable position to p (p itself when walkable); p when nothing within max_radius cells.
    [[nodiscard]] Pos nearest_walkable(Pos p, int max_radius = 16) const noexcept;
    // True when every cell crossed by the segment a-b is walkable.
    [[nodiscard]] bool line_of_sight(Pos a, Pos b) const noexcept;
    // Waypoints (scene units) leading from 'from' to 'to' (last element == to when reachable),
    // smoothed with line-of-sight so straight stretches are single segments.  Empty when
    // unreachable or the search budget is exhausted.
    [[nodiscard]] std::vector<Pos> find_path(Pos from, Pos to, std::size_t max_expand = 40000) const;

private:
    [[nodiscard]] std::vector<Pos> smooth(Pos from, const std::vector<Pos>& points) const;
};

} // namespace jx::zone
