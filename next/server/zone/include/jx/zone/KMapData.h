// Map bundle produced by jxassets export-map: map.json + obstacle.bin.
// The zone only needs the walkability grid (32x32 scene-unit cells), the spawn point, the npc
// placements and the trap cells; the client uses the same bundle for drawing.
#pragma once

#include <array>
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
    // +0x63f54 `%d_NpcSeriesAuto`: a placed monster (kind 0) rolls its series at load (KRegion::LoadNpc 0x080E28F6) and at every
    // revive (0x08085E98) by the five weights of `%d_NpcSeriesMetal/Wood/Water/Fire/Earth` (+0x63f58 ..: the loader 0x080F1346
    // keeps them summed up - `series_sums` here; 0x080EFBE0: g_Random(total) against the sums, the first that is above wins)
    int npc_series_auto = 0;
    std::array<int, 5> npc_series{};        // the raw weights (metal, wood, water, fire, earth)
    std::array<int, 5> series_sums{};       // +0x63f58 .. +0x63f68 as the loader leaves them
    // +0x63f6c `%d_NpcAutoLevelFlag` with +0x63f70 Max / +0x63f74 Min: a placed monster's level at load (0x080EFB90: g_Random(max
    // + 1 - min) + min; max == min -> max); a bad pair (either < 1 or max < min) logs "MapList.ini error:npc level error!" and is 1 / 1
    int npc_auto_level_flag = 0;
    int npc_auto_level_max = 1;
    int npc_auto_level_min = 1;

    // 0x080F1346 .. 0x080F1370 (with the flag) / 0x080F1BBF (without: zeros), 0x080F1D5A .. 0x080F1D96
    void finish() noexcept
    {
        series_sums = {};
        if (npc_series_auto != 0) {
            series_sums[0] = npc_series[0];
            series_sums[1] = series_sums[0] + npc_series[1];
            series_sums[2] = series_sums[1] + npc_series[2];
            series_sums[3] = series_sums[2] + npc_series[3];
            series_sums[4] = series_sums[3] + npc_series[4];
        }
        if (npc_auto_level_flag == 0 || npc_auto_level_max <= 0 || npc_auto_level_min <= 0 || npc_auto_level_max < npc_auto_level_min) {
            npc_auto_level_max = 1;
            npc_auto_level_min = 1;
        }
    }
};

// A named region of a 3D map (the reference client's scn_area_list row + its MarkArea polygon, map.json "areas"):
// TaskScnArea.LogicTick 0x52ee00 of the reference picks, every frame, the area of the highest priority whose polygon
// holds the character (ScnUnit.InArea 0x4a2e30: even-odd crossings on the ground plane, a bounding box first) and tells
// the server; the row's "是否是安全区(切换战斗模式)" flag (column 5) says whether that area is a safe one - entering it
// switches the fight mode.  The old maps do this with their gate traps (SetFightState of script/maps, LINUX-SERVER §16.8).
struct KMapArea {
    std::uint32_t id = 0;
    std::string name;          // the mark's name ("safe", "fight", "ExitArea_wld")
    bool safe = false;         // column 5: 1 = peace inside (fight mode off), else fight mode on
    int priority = 0;          // column 4: the higher wins where areas overlap
    std::vector<Pos> poly;     // scene units, the mark's nodes in order
    int min_x = 0, min_y = 0, max_x = 0, max_y = 0;   // the bounding box of poly

    void finish();
    [[nodiscard]] bool contains(Pos p) const noexcept;
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
    std::vector<KMapArea> areas;   // the 3D map's regions (none on the old 2D maps)

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
    // The area under a position: the highest priority among those holding it (the first of equal ones, as the
    // reference loop keeps the first strictly greater priority); 0 = none.
    [[nodiscard]] std::uint32_t area_at(Pos p) const noexcept;
    [[nodiscard]] const KMapArea* area(std::uint32_t area_id) const noexcept;
    // Adds an area from its polygon (scene units); the bounding box is computed here.
    void add_area(std::uint32_t area_id, const std::string& area_name, bool safe, int priority, std::vector<Pos> poly);
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
