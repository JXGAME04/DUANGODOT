// Map bundle produced by jxassets export-map: map.json + obstacle.bin.
// The zone only needs the walkability grid (32x32 scene-unit cells), the spawn point, the npc
// placements and the trap cells; the client uses the same bundle for drawing.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
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
    bool special = false;       // KSPNpc.bSpecialNpc: KNpcSet::Add 0x0809FBD0 backs it up as a gold candidate whatever the map says (0x0809FCA2)
};

// the map's `<id>_*` keys of maplist.ini the JX2 server keeps per KSubWorld (0x080F1416 ..; map.json "settings")
struct KMapSettings {
    int auto_golden_npc = 0;        // +0x63e7c: the chance in a million that a placed monster revives gold; 0 = every revive (0x080861AE hands 2 000 000)
    int golden_type = 0;            // +0x63e84: the 1-based row of NpcGoldTemplate.txt every gold monster of the map takes; 0 = a random row
    std::string golden_drop_rate;   // +0x63e80: the drop table a gold monster uses while gold ("" = keeps its own)
    std::string normal_drop_rate;   // +0x63e88: the drop table every placed monster uses instead of its template's ("" = the template's)
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
    // where the bundle sits on the old world's region grid (region_left * 512, region_top * 1024):
    // scripts and the old server speak absolute Mps coordinates, the bundle is local to this corner
    Pos origin;
    std::vector<std::uint8_t> obstacle;   // cells_x * cells_y, row major, 0 = walkable
    std::vector<KNpcPlacement> npcs;
    KMapSettings settings;
    // KRegion::m_dwTrap: the trap script id of every cell (0 = none), and the script each id names
    std::vector<std::uint32_t> trap;
    std::unordered_map<std::uint32_t, std::string> trap_scripts;   // id -> `\script\...lua` ("" = unknown)

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
    // KRegion::GetTrap / KSubWorld::GetTrap: the trap script id under a position (0 = none).
    [[nodiscard]] std::uint32_t trap_at(Pos p) const noexcept
    {
        const int cx = p.x / cell, cy = p.y / cell;
        if (!in_bounds(cx, cy) || trap.empty()) return 0;
        return trap[static_cast<std::size_t>(cy) * static_cast<std::size_t>(cells_x) + static_cast<std::size_t>(cx)];
    }
    [[nodiscard]] std::string trap_script(std::uint32_t trap_id) const
    {
        const auto it = trap_scripts.find(trap_id);
        return it == trap_scripts.end() ? std::string{} : it->second;
    }
    // Marks n cells from (cx, cy) rightwards with a trap (KRegion::LoadServerTrap: one KSPTrap run).
    void set_trap(int cx, int cy, int n, std::uint32_t trap_id, const std::string& script);
    // Closest walkable position to p (p itself when walkable); p when nothing within max_radius cells.
    [[nodiscard]] Pos nearest_walkable(Pos p, int max_radius = 16) const noexcept;
    // True when every cell crossed by the segment a-b is walkable.
    [[nodiscard]] bool line_of_sight(Pos a, Pos b) const noexcept;
    // Waypoints (scene units) leading from 'from' to 'to' (last element == to when reachable),
    // smoothed with line-of-sight so straight stretches are single segments.  Empty when
    // unreachable or the search budget is exhausted.
    //
    // This allocates working arrays the size of the map on every call: fine for a test or a one
    // off query, far too slow inside a tick.  The simulation uses KPathFinder, which keeps its
    // buffers between queries.
    [[nodiscard]] std::vector<Pos> find_path(Pos from, Pos to, std::size_t max_expand = 40000) const;
    // Straight stretches become single segments (used by KPathFinder as well).
    [[nodiscard]] std::vector<Pos> smooth_path(Pos from, const std::vector<Pos>& points) const;

private:
    [[nodiscard]] std::vector<Pos> smooth(Pos from, const std::vector<Pos>& points) const;
};

} // namespace jx::zone
