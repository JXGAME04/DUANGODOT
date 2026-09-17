// jx_zone - JX NEXT zone server.
//   jx_zone [--config config/zone.json] [--port N] [--set key=value]...
// Configuration precedence: file < environment (JX_ZONE__PORT=...) < --set / --port.
#include <csignal>
#include <cstdio>
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

    asio::io_context io;
    jx::zone::KGameServer server(io, zc);
    if (const auto ec = server.start()) {
        jx::log::fatal("boot", "zone cannot start", {jx::log::kv("error", ec.message())});
        jx::log::shutdown();
        return 1;
    }
    for (std::int64_t i = 0; i < test_npcs; ++i) {
        // a ring of wandering npcs around the spawn point so a fresh client sees movement
        const std::int32_t dx = static_cast<std::int32_t>((i % 4) * 160) - 240;
        const std::int32_t dy = static_cast<std::int32_t>((i / 4) * 160) - 80;
        server.world().spawn_npc("npc" + std::to_string(i + 1), jx::zone::Pos{w.spawn_point.x + dx, w.spawn_point.y + dy},
                                 static_cast<std::uint32_t>(1000 + i), 200);
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
