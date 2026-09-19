// jx_zone - JX NEXT zone server.
//   jx_zone [--config config/zone.json] [--port N] [--set key=value]...
// Configuration precedence: file < environment (JX_ZONE__PORT=...) < --set / --port.
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "jx/config.hpp"
#include "jx/log.hpp"
#include "jx/net/asio.hpp"
#include "jx/zone/KGameServer.h"
#include "jx/zone/KPlayerChat.h"

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
    lo.console_style = cfg.get_string("log.console_style", "text");   // "text" for a person, "json" for a pipe
    lo.language = cfg.get_string("log.lang", "vi");                   // "vi" reads config/log.vi.json, "en" = as in the code
    lo.catalog = cfg.get_string("log.catalog", "");
    lo.file = cfg.get_string("log.file", "");
    jx::log::init(lo);
    jx::log::set_levels(cfg.get_string("log.levels", ""));
    jx::log::info("boot", "zone starting", {jx::log::kv("version", kVersion), jx::log::kv("config", config_path.empty() ? "(defaults)" : config_path)});
    // one line per setting: what the zone really runs with, after the file, JX_* and --set
    for (const auto& [key, value] : cfg.flatten()) {
        jx::log::info("cfg", "setting", {jx::log::kv("key", key), jx::log::kv("value", value)});
    }

    jx::zone::KGameServerConfig zc;
    zc.listen_address = cfg.get_string("zone.listen", zc.listen_address);
    zc.port = static_cast<std::uint16_t>(cfg.get_int("zone.port", zc.port));
    zc.save_interval_s = static_cast<std::uint32_t>(cfg.get_int("zone.save_interval_s", zc.save_interval_s));
    zc.stats_interval_s = static_cast<std::uint32_t>(cfg.get_int("zone.stats_interval_s", zc.stats_interval_s));
    // MASTER SPEC 15 / 83: the number of simulation workers is configuration, not a constant in
    // the code; 0 means "decide from hardware_concurrency and the number of maps".
    zc.simulation_threads = static_cast<std::uint32_t>(cfg.get_int("zone.simulation_threads", 0));
    zc.rebalance_interval_s = static_cast<std::uint32_t>(cfg.get_int("zone.rebalance_interval_s", zc.rebalance_interval_s));
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
    w.view_width = static_cast<std::int32_t>(cfg.get_int("zone.view_width", w.view_width));
    w.view_height = static_cast<std::int32_t>(cfg.get_int("zone.view_height", w.view_height));
    w.max_viewers = static_cast<std::int32_t>(cfg.get_int("zone.max_viewers", w.max_viewers));
    w.max_known_npcs = static_cast<std::int32_t>(cfg.get_int("zone.max_known_npcs", w.max_known_npcs));
    w.interest_period = static_cast<std::uint32_t>(std::max<std::int64_t>(1, cfg.get_int("zone.interest_period", w.interest_period)));
    w.view_slack = static_cast<std::int32_t>(cfg.get_int("zone.view_slack", w.view_slack));
    w.spawn_budget = static_cast<std::int32_t>(cfg.get_int("zone.spawn_budget", w.spawn_budget));
    w.near_radius = static_cast<std::int32_t>(cfg.get_int("zone.near_radius", w.near_radius));
    w.item_version = static_cast<std::uint32_t>(cfg.get_int("zone.item_version", 0));
    w.gm_chat = cfg.get_bool("zone.gm_chat", false);
    if (w.gm_chat) jx::log::warn("boot", "gm chat commands on", {jx::log::kv("hint", "?gm ds <lua> runs for anyone: development only")});
    w.far_period = static_cast<std::uint32_t>(cfg.get_int("zone.far_period", w.far_period));
    w.default_speed = static_cast<std::uint32_t>(cfg.get_int("zone.default_speed", w.default_speed));
    w.max_players = static_cast<std::uint32_t>(cfg.get_int("zone.max_players", w.max_players));
    w.seed = static_cast<std::uint32_t>(cfg.get_int("zone.seed", w.seed));
    w.map_npcs = cfg.get_bool("zone.map_npcs", true);
    const auto test_npcs = cfg.get_int("zone.test_npcs", 0);
    // their level: the real templates are level-200 monsters (200000 life, a blow kills a fresh character); the
    // owner asked for weak ones to try skills on (2026-09-18)
    const auto test_npc_level = static_cast<std::uint32_t>(std::clamp<std::int64_t>(cfg.get_int("zone.test_npc_level", 10), 1, 200));
    // 0 = plain; n = every test npc is gold of kind n at once, like the script's NPCINFO_AddBlueNpc (BackData +
    // SetGoldTypeAndBackData(1 000 000, n): n = the table's count or more picks the map's GoldenType / a random row)
    const auto test_npc_gold = static_cast<int>(std::clamp<std::int64_t>(cfg.get_int("zone.test_npc_gold", 0), 0, 1000));
    // their templates, "id,id,..." taken in turn (the animals of npcs.txt by default; dev.py assets exports their looks)
    std::vector<std::uint32_t> test_npc_templates;
    {
        std::stringstream parts(cfg.get_string("zone.test_npc_templates", "11,42,5,9"));
        std::string part;
        while (std::getline(parts, part, ',')) {
            try {
                test_npc_templates.push_back(static_cast<std::uint32_t>(std::stoul(part)));
            } catch (const std::exception&) {
            }
        }
    }
    if (test_npc_templates.empty()) test_npc_templates = {11, 42, 5, 9};
    // the item tables (jxassets export-items): without them players carry nothing and item
    // scripts do nothing, which is said here once instead of on every AddItem
    const std::string items_dir = cfg.get_string("zone.items_dir", "client/assets/items");
    if (!items_dir.empty()) {
        auto lib = std::make_shared<jx::zone::KItemLibrary>();
        std::string error;
        if (lib->load_dir(items_dir, &error)) {
            std::string versions;
            for (const auto v : lib->versions()) versions += (versions.empty() ? "" : ",") + std::to_string(v);
            jx::log::info("boot", "item tables loaded", {jx::log::kv("dir", items_dir), jx::log::kv("versions", versions),
                                                         jx::log::kv("default_version", lib->default_version()),
                                                         jx::log::kv("items", lib->set(lib->default_version())->size())});
            if (!error.empty()) jx::log::warn("boot", "item table skipped", {jx::log::kv("error", error)});
            w.items = lib;
        } else {
            jx::log::warn("boot", "no item tables", {jx::log::kv("dir", items_dir), jx::log::kv("error", error)});
        }
    }
    // the player tables (jxassets export-player: level_exp, level_add, stamina.ini, basevalue.ini)
    const std::string player_file = cfg.get_string("zone.player_file", "client/assets/player.json");
    if (!player_file.empty()) {
        auto ps = std::make_shared<jx::zone::KPlayerSet>();
        std::string error;
        if (ps->load(player_file, &error)) {
            w.player_set = ps;
            jx::log::info("boot", "player tables loaded", {jx::log::kv("file", player_file), jx::log::kv("level_2_exp", static_cast<std::int64_t>(ps->level_exp(2)))});
        } else {
            jx::log::warn("boot", "no player tables", {jx::log::kv("file", player_file), jx::log::kv("error", error)});
        }
    }
    // the skill table (jxassets export-skills): the rows of settings\skills.txt for every map's
    // KSkillManager; the numbers per level come from the converted skill scripts at run time
    const std::string skills_file = cfg.get_string("zone.skills_file", "client/assets/skills.json");
    if (!skills_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KSkillTable::load(skills_file, &error)) {
            w.skills = std::make_shared<const jx::zone::KSkillTable>(std::move(*t));
            jx::log::info("boot", "skill table loaded", {jx::log::kv("file", skills_file), jx::log::kv("skills", w.skills->size())});
        } else {
            jx::log::warn("boot", "no skill table", {jx::log::kv("file", skills_file), jx::log::kv("error", error)});
        }
    }
    // the weapon -> physical skill table (jxassets export-weapon-skill); without it the basic attacks 1 / 2
    const std::string weapon_file = cfg.get_string("zone.weapon_skill_file", "client/assets/weapon_skill.json");
    if (!weapon_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KWeaponSkillTable::load(weapon_file, &error)) {
            w.weapon_skills = std::make_shared<const jx::zone::KWeaponSkillTable>(std::move(*t));
            jx::log::info("boot", "weapon skill table loaded", {jx::log::kv("file", weapon_file), jx::log::kv("count", w.weapon_skills->size())});
        } else {
            jx::log::warn("boot", "no weapon skill table", {jx::log::kv("file", weapon_file), jx::log::kv("error", error)});
        }
    }
    // the looks of worn pieces (jxassets export-item-res: settings/item/*Res.txt, KItemSet 0x08068D90); without it every
    // character keeps the bare rows and no horse is drawn
    const std::string item_res_file = cfg.get_string("zone.item_res_file", items_dir.empty() ? std::string() : items_dir + "/item_res.json");
    if (!item_res_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KItemChangeRes::load(item_res_file, &error)) {
            w.item_res = std::make_shared<const jx::zone::KItemChangeRes>(std::move(*t));
            jx::log::info("boot", "item res tables loaded", {jx::log::kv("file", item_res_file), jx::log::kv("gold", w.item_res->gold.size())});
        } else {
            jx::log::warn("boot", "no item res tables", {jx::log::kv("file", item_res_file), jx::log::kv("error", error)});
        }
    }
    // the wear table (jxassets export-abrade-rate): how fast a worn piece loses durability; without it nothing wears
    const std::string abrade_file = cfg.get_string("zone.abrade_rate_file", "client/assets/abrade_rate.json");
    if (!abrade_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KAbradeRate::load(abrade_file, &error)) {
            w.abrade_rate = std::make_shared<const jx::zone::KAbradeRate>(std::move(*t));
            jx::log::info("boot", "abrade rate table loaded", {jx::log::kv("file", abrade_file)});
        } else {
            jx::log::warn("boot", "no abrade rate table", {jx::log::kv("file", abrade_file), jx::log::kv("error", error)});
        }
    }
    // the chat cost table (jxassets export-chat-cost, \settings\npc\player\chatcost.ini): what a line on the city / faction /
    // world channels asks of the speaker; without it every channel is free
    const std::string chat_cost_file = cfg.get_string("zone.chat_cost_file", "client/assets/chat_cost.json");
    if (!chat_cost_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KChatCostTable::load(chat_cost_file, &error)) {
            w.chat_cost = std::make_shared<const jx::zone::KChatCostTable>(std::move(*t));
            jx::log::info("boot", "chat cost table loaded", {jx::log::kv("file", chat_cost_file)});
        } else {
            jx::log::warn("boot", "no chat cost table", {jx::log::kv("file", chat_cost_file), jx::log::kv("error", error)});
        }
    }
    // the task value table (jxassets export-task-def, \settings\task\player_task_def.txt): which task values the client is told
    // about (SYNC_FLAG) and which it may set (CLIENT_FLAG); without it no value leaves the zone (docs/LINUX-SERVER.md §21)
    const std::string task_def_file = cfg.get_string("zone.task_def_file", "client/assets/task_def.json");
    if (!task_def_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KTaskDefTable::load(task_def_file, &error)) {
            const std::size_t ids = t->size();
            const std::size_t ranges = t->sync_ranges().size();
            w.task_def = std::make_shared<const jx::zone::KTaskDefTable>(std::move(*t));
            jx::log::info("boot", "task def table loaded", {jx::log::kv("file", task_def_file), jx::log::kv("ids", ids), jx::log::kv("ranges", ranges)});
        } else {
            jx::log::warn("boot", "no task def table", {jx::log::kv("file", task_def_file), jx::log::kv("error", error)});
        }
    }
    // the task system tables (jxassets export-task-tables, \settings\task): what the TASKSYS library of the scripts reads
    // (task_id.txt, task_type.txt and the tables of every kind, task_event.txt); without them the library answers nothing (docs/LINUX-SERVER.md §22)
    const std::string task_tables_file = cfg.get_string("zone.task_tables_file", "client/assets/task_tables.json");
    if (!task_tables_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KTaskManager::load(task_tables_file, &error)) {
            const std::size_t tasks = t->task_count();
            const std::size_t types = t->type_count();
            const std::size_t events = t->event_count();
            w.tasks = std::make_shared<const jx::zone::KTaskManager>(std::move(*t));
            jx::log::info("boot", "task tables loaded", {jx::log::kv("file", task_tables_file), jx::log::kv("tasks", tasks), jx::log::kv("types", types), jx::log::kv("events", events)});
        } else {
            jx::log::warn("boot", "no task tables", {jx::log::kv("file", task_tables_file), jx::log::kv("error", error)});
        }
    }
    // the kill events (jxassets export-kill-events, \settings\npc\player\event_killnpc.txt): what a kill counts for the events a
    // script registered on a character (AddPlayerEvent); without it a kill counts nothing (docs/LINUX-SERVER.md §23)
    const std::string kill_events_file = cfg.get_string("zone.kill_events_file", "client/assets/kill_events.json");
    if (!kill_events_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KKillEventTable::load(kill_events_file, &error)) {
            const std::size_t rows = t->size();
            w.kill_events = std::make_shared<const jx::zone::KKillEventTable>(std::move(*t));
            jx::log::info("boot", "kill events loaded", {jx::log::kv("file", kill_events_file), jx::log::kv("rows", rows)});
        } else {
            jx::log::warn("boot", "no kill events", {jx::log::kv("file", kill_events_file), jx::log::kv("error", error)});
        }
    }
    // the old server folders (";" apart) the scripts' TabFile_Load reads its tables from (\settings\...); without them the
    // TabFile_* library loads nothing (docs/LINUX-SERVER.md §24)
    const std::string settings_root = cfg.get_string("zone.settings_root", "");
    jx::zone::g_TabFiles().set_roots(settings_root);
    if (settings_root.empty()) {
        jx::log::warn("boot", "zone.settings_root not set, the scripts' TabFile_Load finds nothing");
    } else {
        jx::log::info("boot", "tab file roots", {jx::log::kv("roots", settings_root)});
    }
    // the revive / reference points of every map (jxassets export-revive-pos): where a fresh character is born in its
    // village and where the revive / SetRevPos put a character; without it the spawn point of each map stands in
    const std::string revive_file = cfg.get_string("zone.revive_pos_file", "client/assets/revive_pos.json");
    if (!revive_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KRevivePosTable::load(revive_file, &error)) {
            w.revive_pos = std::make_shared<const jx::zone::KRevivePosTable>(std::move(*t));
            jx::log::info("boot", "revive point table loaded", {jx::log::kv("file", revive_file), jx::log::kv("maps", w.revive_pos->maps.size())});
        } else {
            jx::log::warn("boot", "no revive point table", {jx::log::kv("file", revive_file), jx::log::kv("error", error)});
        }
    }
    // the eleven factions (jxassets export-faction): SetFaction of the script api, the camp of a member (docs §16.7)
    const std::string faction_file = cfg.get_string("zone.faction_file", "client/assets/faction.json");
    if (!faction_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KFaction::load(faction_file, &error)) {
            w.faction = std::make_shared<const jx::zone::KFaction>(std::move(*t));
            jx::log::info("boot", "faction table loaded", {jx::log::kv("file", faction_file)});
        } else {
            jx::log::warn("boot", "no faction table", {jx::log::kv("file", faction_file), jx::log::kv("error", error)});
        }
    }
    // the missile table (jxassets export-missles): the rows of settings\missles.txt the skills fire
    const std::string missles_file = cfg.get_string("zone.missles_file", "client/assets/missles.json");
    if (!missles_file.empty()) {
        std::string error;
        if (auto t = jx::zone::KMissleTable::load(missles_file, &error)) {
            w.missles = std::make_shared<const jx::zone::KMissleTable>(std::move(*t));
            jx::log::info("boot", "missile table loaded", {jx::log::kv("file", missles_file), jx::log::kv("missles", w.missles->size())});
        } else {
            jx::log::warn("boot", "no missile table", {jx::log::kv("file", missles_file), jx::log::kv("error", error)});
        }
    }
    // the ground objects (jxassets export-objdata): without them nothing can be dropped
    const std::string objdata = cfg.get_string("zone.objdata", "client/assets/objdata.json");
    if (!objdata.empty()) {
        auto od = std::make_shared<jx::zone::KObjDataSet>();
        std::string error;
        if (od->load(objdata, &error)) {
            w.objdata = od;
        } else {
            jx::log::warn("boot", "no object data", {jx::log::kv("file", objdata), jx::log::kv("error", error)});
        }
    }
    w.money_rate_percent = static_cast<int>(cfg.get_int("zone.money_rate_percent", 100));
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
        // settings/npc/NpcGoldTemplate.txt next to it (jxassets export-npc-gold): the gold monster kinds
        const std::filesystem::path gold_file = std::filesystem::path(npcres_file).parent_path() / "npc_gold.json";
        if (auto g = jx::zone::KNpcGoldTemplateSet::load(gold_file, &error)) {
            jx::log::info("boot", "gold monster kinds", {jx::log::kv("file", gold_file.string()), jx::log::kv("rows", g->count()), jx::log::kv("client_rows", g->client_rows)});
            w.gold = std::make_shared<const jx::zone::KNpcGoldTemplateSet>(std::move(*g));
        } else {
            jx::log::warn("boot", "no gold monster table, nothing turns gold", {jx::log::kv("file", gold_file.string()), jx::log::kv("error", error)});
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
        std::vector<std::uint32_t> ids;
        if (list == "all" || list == "*") {
            // every bundle python tools/dev.py assets exported: the whole old game is 980 maps
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(maps_dir, ec)) {
                if (!entry.is_directory(ec)) continue;
                const std::string name = entry.path().filename().string();
                if (name.empty() || name.find_first_not_of("0123456789") != std::string::npos) continue;
                ids.push_back(static_cast<std::uint32_t>(std::strtoul(name.c_str(), nullptr, 10)));
            }
            std::sort(ids.begin(), ids.end());
            if (ec) jx::log::warn("boot", "maps dir unreadable", {jx::log::kv("dir", maps_dir), jx::log::kv("error", ec.message())});
        } else {
            std::size_t start = 0;
            while (start <= list.size()) {
                const std::size_t comma = list.find(',', start);
                const std::string item = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                start = comma == std::string::npos ? list.size() + 1 : comma + 1;
                if (!item.empty()) ids.push_back(static_cast<std::uint32_t>(std::strtoul(item.c_str(), nullptr, 10)));
            }
        }
        const auto load_start = std::chrono::steady_clock::now();
        for (const std::uint32_t id : ids) {
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
            // one script cache (one set of Lua states) per map instance: two workers must never
            // run the same lua_State at the same time (MASTER SPEC 42)
            if (!script_root.empty() && w.scripts) wc.scripts = std::make_shared<jx::zone::KScriptCache>(script_root);
            zc.worlds.push_back(std::move(wc));
        }
        const auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - load_start).count();
        jx::log::info("boot", "maps hosted", {jx::log::kv("count", zc.worlds.size()), jx::log::kv("load_ms", load_ms)});
    }

    asio::io_context io;
    jx::zone::KGameServer server(io, zc);
    if (const auto ec = server.start()) {
        jx::log::fatal("boot", "zone cannot start", {jx::log::kv("error", ec.message())});
        jx::log::shutdown();
        return 1;
    }
    // a ring of wandering npcs around the (map-adjusted) spawn point so a fresh client sees movement;
    // their templates are exported by `dev.py assets` (jxassets export-npcres -templates ...)
    const jx::zone::Pos spawn = server.world().config().spawn_point;
    for (std::int64_t i = 0; i < test_npcs; ++i) {
        const std::int32_t dx = static_cast<std::int32_t>((i % 4) * 160) - 240;
        const std::int32_t dy = static_cast<std::int32_t>((i / 4) * 160) - 80;
        const std::uint32_t tpl = test_npc_templates[static_cast<std::size_t>(i) % test_npc_templates.size()];
        const jx::EntityId id = server.world().spawn_npc("npc" + std::to_string(i + 1), jx::zone::Pos{spawn.x + dx, spawn.y + dy},
                                                        tpl, 200, jx::zone::KNpcKind::monster, test_npc_level, -1, 0);   // attackable, wander 200; placed like a Region_S.dat npc (+0x181c = 0)
        // the templates are active hunters (AIMode 1, vision 1200): passive here (AIMode 4, strike back only)
        // so a fresh character can look around the spawn point; the real monsters keep their data
        server.world().set_ai_mode(id, 4);
        if (test_npc_gold > 0) {
            if (jx::zone::KNpc* e = server.world().mutable_entity(id)) {
                jx::zone::gold_back_data(*e);                          // NPCINFO_AddBlueNpc 0x081C27A2 / AddNpc(.., 2) 0x0811BE3C
                server.world().set_gold_type(*e, 1000000, test_npc_gold);   // 0x081C27C8: SetGoldTypeAndBackData(1 000 000, 0) - the kind asked here
                server.world().set_ai_mode(id, 4);
            }
        }
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

    jx::log::info("boot", "zone ready", {jx::log::kv("port", server.port()), jx::log::kv("test_npcs", test_npcs), jx::log::kv("test_npc_level", test_npc_level),
                                         jx::log::kv("map", map_dir.empty() ? "(grid)" : map_dir), jx::log::kv("entities", server.world().entity_count())});
    io.run();
    jx::log::info("boot", "zone exit");
    jx::log::shutdown();
    return 0;
}
