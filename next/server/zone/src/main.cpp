// jx_zone - JX NEXT zone server.
//   jx_zone [--config config/zone.json] [--port N] [--set key=value]...
// Configuration precedence: file < environment (JX_ZONE__PORT=...) < --set / --port.
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>

#include "jx/config.hpp"
#include "jx/log.hpp"
#include "jx/net/asio.hpp"
#include "jx/zone/KGameServer.h"

namespace {

constexpr const char* kVersion = "0.1.0";

void usage()
{
    std::puts("jx_zone [--config <file>] [--port <n>] [--set <key=value>]... [--log-level <lvl>]\n"
              "  --config     JSON config (default: config/zone.json when it exists)\n"
              "  --set k=v    override any config key, e.g. --set zone.tick_hz=30\n"
              "  --port n     shortcut for --set zone.port=n\n"
              "  --log-level  shortcut for --set log.level=<lvl>");
}

} // namespace

int main(int argc, char** argv)
{
    std::string config_path = std::filesystem::exists("config/zone.json") ? "config/zone.json" : "";
    std::map<std::string, std::string> overrides;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--config") config_path = value("--config");
        else if (a == "--port") overrides["zone.port"] = value("--port");
        else if (a == "--log-level") overrides["log.level"] = value("--log-level");
        else if (a == "--set") {
            const std::string kv = value("--set");
            const auto eq = kv.find('=');
            if (eq == std::string::npos) {
                std::fprintf(stderr, "--set expects key=value\n");
                return 2;
            }
            overrides[kv.substr(0, eq)] = kv.substr(eq + 1);
        } else if (a == "--help" || a == "-h") {
            usage();
            return 0;
        } else {
            std::fprintf(stderr, "unknown argument %s\n", a.c_str());
            usage();
            return 2;
        }
    }

    jx::config::Config cfg;
    if (!config_path.empty()) {
        try {
            cfg = jx::config::Config::from_file(config_path);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 2;
        }
    }
    cfg.apply_env("JX_");
    cfg.apply_overrides(overrides);

    jx::log::Options lo;
    lo.process = "zone";
    lo.default_level = jx::log::level_from_name(cfg.get_string("log.level", "info"));
    lo.console = cfg.get_bool("log.console", true);
    lo.file = cfg.get_string("log.file", "");
    jx::log::init(lo);
    jx::log::set_levels(cfg.get_string("log.levels", ""));
    jx::log::info("boot", "zone starting", {jx::log::kv("version", kVersion), jx::log::kv("config", config_path.empty() ? "(defaults)" : config_path)});
    jx::log::info("cfg", "effective config", {jx::log::kv("json", cfg.dump(-1))});

    jx::zone::KGameServerConfig zc;
    zc.listen_address = cfg.get_string("zone.listen", zc.listen_address);
    zc.port = static_cast<std::uint16_t>(cfg.get_int("zone.port", zc.port));
    zc.save_interval_s = static_cast<std::uint32_t>(cfg.get_int("zone.save_interval_s", zc.save_interval_s));
    zc.stats_interval_s = static_cast<std::uint32_t>(cfg.get_int("zone.stats_interval_s", zc.stats_interval_s));
    auto& w = zc.world;
    w.zone_id = static_cast<std::uint32_t>(cfg.get_int("zone.id", w.zone_id));
    w.name = cfg.get_string("zone.name", w.name);
    w.tick_hz = static_cast<std::uint32_t>(cfg.get_int("zone.tick_hz", w.tick_hz));
    w.width = static_cast<std::int32_t>(cfg.get_int("zone.width", w.width));
    w.height = static_cast<std::int32_t>(cfg.get_int("zone.height", w.height));
    w.spawn_point.x = static_cast<std::int32_t>(cfg.get_int("zone.spawn.x", w.spawn_point.x));
    w.spawn_point.y = static_cast<std::int32_t>(cfg.get_int("zone.spawn.y", w.spawn_point.y));
    w.spawn_from_config = cfg.has("zone.spawn.x") && cfg.has("zone.spawn.y");
    w.cell_size = static_cast<std::int32_t>(cfg.get_int("zone.cell_size", w.cell_size));
    w.view_cells = static_cast<std::int32_t>(cfg.get_int("zone.view_cells", w.view_cells));
    w.default_speed = static_cast<std::uint32_t>(cfg.get_int("zone.default_speed", w.default_speed));
    w.max_players = static_cast<std::uint32_t>(cfg.get_int("zone.max_players", w.max_players));
    w.seed = static_cast<std::uint32_t>(cfg.get_int("zone.seed", w.seed));
    w.map_npcs = cfg.get_bool("zone.map_npcs", true);
    const auto test_npcs = cfg.get_int("zone.test_npcs", 0);
    const std::string map_dir = cfg.get_string("zone.map_dir", "");
    if (!map_dir.empty()) {
        std::string error;
        auto map = jx::zone::KMapData::load(map_dir, &error);
        if (!map) {
            jx::log::fatal("boot", "map bundle failed", {jx::log::kv("dir", map_dir), jx::log::kv("error", error)});
            jx::log::shutdown();
            return 1;
        }
        w.map = std::make_shared<const jx::zone::KMapData>(std::move(*map));
    }
    // npcs.txt numbers (exported next to the maps: client/assets/npcres/npcs.json)
    std::string npcres_file = cfg.get_string("zone.npcres_file", "");
    if (npcres_file.empty() && !map_dir.empty()) {
        npcres_file = (std::filesystem::path(map_dir).parent_path().parent_path() / "npcres" / "npcs.json").string();
    }
    if (!npcres_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KNpcTemplateSet::load(npcres_file, &error)) {
            w.templates = std::make_shared<const jx::zone::KNpcTemplateSet>(std::move(*t));
        } else {
            jx::log::warn("boot", "npc templates unavailable, default combat numbers", {jx::log::kv("file", npcres_file), jx::log::kv("error", error)});
        }
    }
    // the old server folder holding script\ (npc level scripts through Lua 5.4); empty = placeholder numbers
    const std::string script_root = cfg.get_string("zone.script_root", "");
    if (!script_root.empty()) {
        // "a;b": the reference server first, then the fallback (KScriptCache tries each root)
        auto cache = std::make_shared<jx::zone::KScriptCache>(script_root);
        std::size_t with_scripts = 0;
        for (const std::string& root : cache->roots()) {
            if (std::filesystem::exists(std::filesystem::path(root) / "script")) ++with_scripts;
        }
        if (with_scripts > 0) {
            w.scripts = cache;
            jx::log::info("boot", "level scripts", {jx::log::kv("root", script_root), jx::log::kv("roots_with_script", with_scripts)});
        } else {
            jx::log::warn("boot", "zone.script_root has no script folder, npc life / damage use placeholders", {jx::log::kv("dir", script_root)});
        }
    } else {
        jx::log::warn("boot", "zone.script_root not set, npc life / damage use placeholders (docs/NPCRES.md)");
    }

    // every map this zone hosts (a portal needs its target loaded): zone.maps = "1,3,7" under zone.maps_dir;
    // the default map (zone.map_dir) is the first entry
    zc.worlds.push_back(w);
    {
        const std::string maps_dir = cfg.get_string("zone.maps_dir", "client/assets/maps");
        std::string list = cfg.get_string("zone.maps", "");
        for (char& c : list) if (c == ';' || c == ' ') c = ',';
        std::size_t start = 0;
        while (start <= list.size()) {
            const std::size_t comma = list.find(',', start);
            const std::string item = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
            start = comma == std::string::npos ? list.size() + 1 : comma + 1;
            if (item.empty()) continue;
            const auto id = static_cast<std::uint32_t>(std::strtoul(item.c_str(), nullptr, 10));
            if (id == 0 || (w.map && static_cast<std::uint32_t>(w.map->id) == id)) continue;
            const std::string dir = (std::filesystem::path(maps_dir) / std::to_string(id)).string();
            std::string error;
            auto map = jx::zone::KMapData::load(dir, &error);
            if (!map) {
                jx::log::warn("boot", "map bundle skipped", {jx::log::kv("map", id), jx::log::kv("dir", dir), jx::log::kv("error", error)});
                continue;
            }
            jx::zone::KSubWorldConfig wc = w;
            wc.map = std::make_shared<const jx::zone::KMapData>(std::move(*map));
            wc.spawn_from_config = false;
            zc.worlds.push_back(std::move(wc));
        }
        jx::log::info("boot", "maps hosted", {jx::log::kv("count", zc.worlds.size())});
    }

    asio::io_context io;
    jx::zone::KGameServer server(io, zc);
    if (const auto ec = server.start()) {
        jx::log::fatal("boot", "zone cannot start", {jx::log::kv("error", ec.message())});
        jx::log::shutdown();
        return 1;
    }
    // a ring of wandering npcs around the (map-adjusted) spawn point so a fresh client sees movement;
    // templates 1000 + i are exported by `dev.py assets` (jxassets export-npcres -templates ...)
    const jx::zone::Pos spawn = server.world().config().spawn_point;
    for (std::int64_t i = 0; i < test_npcs; ++i) {
        const std::int32_t dx = static_cast<std::int32_t>((i % 4) * 160) - 240;
        const std::int32_t dy = static_cast<std::int32_t>((i / 4) * 160) - 80;
        const jx::EntityId id = server.world().spawn_npc("npc" + std::to_string(i + 1), jx::zone::Pos{spawn.x + dx, spawn.y + dy},
                                                        static_cast<std::uint32_t>(1000 + i), 200, jx::zone::KNpcKind::monster);   // attackable
        // the templates are active hunters (AIMode 1, vision 1200): passive here (AIMode 4, strike back only)
        // so a fresh character can look around the spawn point; the real monsters keep their data
        server.world().set_ai_mode(id, 4);
    }

    asio::signal_set signals(io, SIGINT, SIGTERM);
#ifdef _WIN32
    signals.add(SIGBREAK);   // console window closed / taskkill without /F
#endif
    signals.async_wait([&](const std::error_code&, int sig) {
        jx::log::info("boot", "signal received", {jx::log::kv("signal", sig)});
        server.stop();
        io.stop();
    });

    jx::log::info("boot", "zone ready", {jx::log::kv("port", server.port()), jx::log::kv("test_npcs", test_npcs),
                                         jx::log::kv("map", map_dir.empty() ? "(grid)" : map_dir), jx::log::kv("entities", server.world().entity_count())});
    io.run();
    jx::log::info("boot", "zone exit");
    jx::log::shutdown();
    return 0;
}
