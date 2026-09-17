#include "jx/zone/map.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <queue>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"

namespace jx::zone {

std::optional<MapData> MapData::load(const std::filesystem::path& dir, std::string* error)
{
    auto fail = [&](const std::string& msg) {
        if (error) *error = msg;
        return std::nullopt;
    };
    std::ifstream in(dir / "map.json", std::ios::binary);
    if (!in) return fail("cannot open " + (dir / "map.json").string());
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        return fail(std::string("map.json: ") + e.what());
    }
    MapData m;
    try {
        m.id = j.value("id", 0);
        m.name = j.value("name", "");
        m.cell = j.value("cell_size", 32);
        m.cells_x = j.value("cells_x", 0);
        m.cells_y = j.value("cells_y", 0);
        m.scene_w = j.value("scene_w", 0);
        m.scene_h = j.value("scene_h", 0);
        if (j.contains("spawn") && j["spawn"].is_array() && j["spawn"].size() == 2) {
            m.spawn = Pos{j["spawn"][0].get<int>(), j["spawn"][1].get<int>()};
        }
        for (const auto& n : j.value("npcs", nlohmann::json::array())) {
            NpcPlacement p;
            p.template_id = n.value("template_id", 0u);
            p.name = n.value("name", "");
            p.pos = Pos{n.value("x", 0), n.value("y", 0)};
            p.frame = n.value("frame", 0);
            p.kind = n.value("kind", 0);
            p.script = n.value("script", "");
            m.npcs.push_back(std::move(p));
        }
    } catch (const std::exception& e) {
        return fail(std::string("map.json fields: ") + e.what());
    }
    if (m.cells_x <= 0 || m.cells_y <= 0 || m.cell <= 0) return fail("map.json: bad grid size");
    std::ifstream ob(dir / "obstacle.bin", std::ios::binary);
    if (!ob) return fail("cannot open " + (dir / "obstacle.bin").string());
    m.obstacle.assign(std::istreambuf_iterator<char>(ob), std::istreambuf_iterator<char>());
    const auto expected = static_cast<std::size_t>(m.cells_x) * static_cast<std::size_t>(m.cells_y);
    if (m.obstacle.size() != expected) {
        return fail("obstacle.bin: size " + std::to_string(m.obstacle.size()) + " != " + std::to_string(expected));
    }
    if (m.scene_w == 0) m.scene_w = m.cells_x * m.cell;
    if (m.scene_h == 0) m.scene_h = m.cells_y * m.cell;
    std::size_t blocked = 0;
    for (const auto b : m.obstacle) blocked += (b != 0);
    log::info("map", "map loaded", {log::kv("id", m.id), log::kv("name", m.name), log::kv("cells", std::to_string(m.cells_x) + "x" + std::to_string(m.cells_y)),
                                    log::kv("blocked_pct", blocked * 100 / expected), log::kv("npcs", m.npcs.size()),
                                    log::kv("spawn", std::to_string(m.spawn.x) + "," + std::to_string(m.spawn.y))});
    return m;
}

MapData MapData::synthetic(int cells_x, int cells_y, int cell)
{
    MapData m;
    m.cell = cell;
    m.cells_x = cells_x;
    m.cells_y = cells_y;
    m.scene_w = cells_x * cell;
    m.scene_h = cells_y * cell;
    m.obstacle.assign(static_cast<std::size_t>(cells_x) * static_cast<std::size_t>(cells_y), 0);
    m.spawn = m.cell_center(cells_x / 2, cells_y / 2);
    return m;
}

void MapData::set_blocked(int cx, int cy, std::uint8_t kind)
{
    if (in_bounds(cx, cy)) obstacle[static_cast<std::size_t>(cy) * static_cast<std::size_t>(cells_x) + static_cast<std::size_t>(cx)] = kind;
}

Pos MapData::nearest_walkable(Pos p, int max_radius) const noexcept
{
    const int cx = p.x / cell, cy = p.y / cell;
    if (walkable_cell(cx, cy)) return p;
    for (int r = 1; r <= max_radius; ++r) {
        Pos best;
        long best_d = -1;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) != r && std::abs(dy) != r) continue;   // ring only
                if (!walkable_cell(cx + dx, cy + dy)) continue;
                const Pos c = cell_center(cx + dx, cy + dy);
                const long d = static_cast<long>(c.x - p.x) * (c.x - p.x) + static_cast<long>(c.y - p.y) * (c.y - p.y);
                if (best_d < 0 || d < best_d) {
                    best_d = d;
                    best = c;
                }
            }
        }
        if (best_d >= 0) return best;
    }
    return p;
}

bool MapData::line_of_sight(Pos a, Pos b) const noexcept
{
    // supercover traversal over cells (every cell the segment touches must be walkable)
    int x0 = a.x / cell, y0 = a.y / cell;
    const int x1 = b.x / cell, y1 = b.y / cell;
    const int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int n = dx + dy;
    if (!walkable_cell(x0, y0)) return false;
    while (n-- > 0) {
        const int e2 = 2 * err;
        if (e2 > -dy && e2 < dx) {
            // diagonal step: both orthogonal neighbours must be free (no corner cutting)
            if (!walkable_cell(x0 + sx, y0) || !walkable_cell(x0, y0 + sy)) return false;
            x0 += sx;
            y0 += sy;
            err += dx - dy;
            --n;
        } else if (e2 > -dy) {
            x0 += sx;
            err -= dy;
        } else {
            y0 += sy;
            err += dx;
        }
        if (!walkable_cell(x0, y0)) return false;
    }
    return true;
}

std::vector<Pos> MapData::find_path(Pos from, Pos to, std::size_t max_expand) const
{
    const int sx = from.x / cell, sy = from.y / cell;
    const int tx = to.x / cell, ty = to.y / cell;
    if (!in_bounds(sx, sy) || !walkable_cell(tx, ty)) return {};
    if (sx == tx && sy == ty) return {to};
    if (line_of_sight(from, to)) return {to};

    const std::size_t total = static_cast<std::size_t>(cells_x) * static_cast<std::size_t>(cells_y);
    const auto idx = [&](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(cells_x) + static_cast<std::size_t>(x); };
    std::vector<int> g(total, -1);
    std::vector<int> parent(total, -1);
    std::vector<std::uint8_t> closed(total, 0);
    struct Node {
        int f, g, x, y;
        bool operator>(const Node& o) const noexcept { return f > o.f; }
    };
    const auto heuristic = [&](int x, int y) {
        const int dx = std::abs(x - tx), dy = std::abs(y - ty);
        return 10 * (dx + dy) - 6 * std::min(dx, dy);   // octile with 10/14 costs
    };
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    g[idx(sx, sy)] = 0;
    open.push(Node{heuristic(sx, sy), 0, sx, sy});
    std::size_t expanded = 0;
    bool found = false;
    static constexpr int kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static constexpr int kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    while (!open.empty()) {
        const Node cur = open.top();
        open.pop();
        const std::size_t ci = idx(cur.x, cur.y);
        if (closed[ci]) continue;
        closed[ci] = 1;
        if (cur.x == tx && cur.y == ty) {
            found = true;
            break;
        }
        if (++expanded > max_expand) break;
        for (int d = 0; d < 8; ++d) {
            const int nx = cur.x + kDx[d], ny = cur.y + kDy[d];
            if (!walkable_cell(nx, ny)) continue;
            if (d >= 4 && (!walkable_cell(cur.x + kDx[d], cur.y) || !walkable_cell(cur.x, cur.y + kDy[d]))) continue;
            const std::size_t ni = idx(nx, ny);
            if (closed[ni]) continue;
            const int ng = cur.g + (d >= 4 ? 14 : 10);
            if (g[ni] >= 0 && ng >= g[ni]) continue;
            g[ni] = ng;
            parent[ni] = static_cast<int>(ci);
            open.push(Node{ng + heuristic(nx, ny), ng, nx, ny});
        }
    }
    if (!found) return {};
    std::vector<Pos> cells;
    for (int i = static_cast<int>(idx(tx, ty)); i >= 0; i = parent[static_cast<std::size_t>(i)]) {
        cells.push_back(cell_center(i % cells_x, i / cells_x));
        if (i == static_cast<int>(idx(sx, sy))) break;
    }
    std::reverse(cells.begin(), cells.end());
    cells.erase(cells.begin());     // the start cell
    if (!cells.empty()) cells.back() = to;   // finish exactly on the requested point
    return smooth(from, cells);
}

std::vector<Pos> MapData::smooth(Pos from, const std::vector<Pos>& points) const
{
    std::vector<Pos> out;
    Pos anchor = from;
    std::size_t i = 0;
    while (i < points.size()) {
        std::size_t far = i;
        for (std::size_t k = i + 1; k < points.size(); ++k) {
            if (line_of_sight(anchor, points[k])) far = k;
            else break;
        }
        out.push_back(points[far]);
        anchor = points[far];
        i = far + 1;
    }
    return out;
}

} // namespace jx::zone
