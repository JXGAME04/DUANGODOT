#include "jx/log.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <deque>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <fmt/chrono.h>
#include <nlohmann/json.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace jx::log {
namespace {

struct State {
    std::shared_ptr<spdlog::logger> logger;
    Options options;
    std::shared_mutex levels_mutex;
    std::unordered_map<std::string, Level> levels;   // category -> level ("" = default)
    std::mutex ring_mutex;
    std::deque<std::string> ring;
    bool initialised = false;
};

State& state()
{
    static State s;
    return s;
}

thread_local Context t_context;

std::string timestamp_utc()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto secs = time_point_cast<seconds>(now);
    const auto micros = duration_cast<microseconds>(now - secs).count();
    const std::time_t t = system_clock::to_time_t(secs);
    return fmt::format("{:%Y-%m-%dT%H:%M:%S}.{:06}Z", fmt::gmtime(t), micros);
}

std::uint64_t thread_id_hash()
{
    return static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

void push_ring(State& s, const std::string& line)
{
    std::lock_guard lock(s.ring_mutex);
    s.ring.push_back(line);
    while (s.ring.size() > s.options.ring_capacity && !s.ring.empty()) {
        s.ring.pop_front();
    }
}

std::string trim(std::string_view v)
{
    const auto b = v.find_first_not_of(" \t\r\n");
    if (b == std::string_view::npos) return {};
    const auto e = v.find_last_not_of(" \t\r\n");
    return std::string(v.substr(b, e - b + 1));
}

} // namespace

const char* level_name(Level level) noexcept
{
    switch (level) {
    case Level::trace: return "trace";
    case Level::debug: return "debug";
    case Level::info:  return "info";
    case Level::warn:  return "warn";
    case Level::error: return "error";
    case Level::fatal: return "fatal";
    case Level::off:   return "off";
    }
    return "info";
}

Level level_from_name(std::string_view name) noexcept
{
    const std::string n = trim(name);
    if (n == "trace") return Level::trace;
    if (n == "debug") return Level::debug;
    if (n == "info")  return Level::info;
    if (n == "warn" || n == "warning") return Level::warn;
    if (n == "error") return Level::error;
    if (n == "fatal") return Level::fatal;
    if (n == "off")   return Level::off;
    return Level::info;
}

void init(const Options& options)
{
    State& s = state();
    shutdown();
    s.options = options;

    std::vector<spdlog::sink_ptr> sinks;
    if (options.console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    if (!options.file.empty()) {
        std::filesystem::create_directories(options.file.parent_path().empty() ? std::filesystem::path(".") : options.file.parent_path());
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            options.file.string(), options.rotate_bytes, options.rotate_files));
    }
    if (sinks.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }
    if (options.async) {
        // one background thread drains the queue; overrun_oldest keeps a stalled disk from
        // blocking a simulation worker (MASTER SPEC 49, 70)
        spdlog::init_thread_pool(options.async_queue == 0 ? 16384 : options.async_queue, 1);
        s.logger = std::make_shared<spdlog::async_logger>("jx", sinks.begin(), sinks.end(),
                                                          spdlog::thread_pool(),
                                                          spdlog::async_overflow_policy::overrun_oldest);
    } else {
        s.logger = std::make_shared<spdlog::logger>("jx", sinks.begin(), sinks.end());
    }
    s.logger->set_pattern("%v");            // the line is already a complete JSON object
    s.logger->set_level(spdlog::level::trace);
    s.logger->flush_on(spdlog::level::warn);
    // register so the periodic flusher covers it: a killed process loses at most one second
    spdlog::drop("jx");
    spdlog::register_logger(s.logger);
    spdlog::flush_every(std::chrono::seconds(1));

    {
        std::unique_lock lock(s.levels_mutex);
        s.levels.clear();
        s.levels[""] = options.default_level;
    }
    {
        std::lock_guard lock(s.ring_mutex);
        s.ring.clear();
    }
    s.initialised = true;
}

void shutdown()
{
    State& s = state();
    if (s.logger) {
        const bool was_async = s.options.async;
        s.logger->flush();
        spdlog::drop("jx");
        s.logger.reset();
        if (was_async) {
            // the logging thread owns the queue: stopping it drains what is still in flight,
            // so no line written before shutdown is lost (MASTER SPEC 49, 86)
            spdlog::shutdown();
        }
    }
    s.initialised = false;
}

void flush()
{
    State& s = state();
    if (s.logger) s.logger->flush();
}

std::uint64_t dropped_lines() noexcept
{
    const auto pool = spdlog::thread_pool();
    return pool ? static_cast<std::uint64_t>(pool->overrun_counter()) : 0;
}

void set_level(std::string_view category, Level level)
{
    State& s = state();
    std::unique_lock lock(s.levels_mutex);
    s.levels[std::string(category)] = level;
}

void set_levels(std::string_view spec)
{
    std::size_t pos = 0;
    while (pos <= spec.size()) {
        const auto comma = spec.find(',', pos);
        const std::string_view item = spec.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos);
        const auto eq = item.find('=');
        if (eq != std::string_view::npos) {
            set_level(trim(item.substr(0, eq)), level_from_name(item.substr(eq + 1)));
        } else if (!trim(item).empty()) {
            set_level("", level_from_name(item));
        }
        if (comma == std::string_view::npos) break;
        pos = comma + 1;
    }
}

Level effective_level(std::string_view category) noexcept
{
    State& s = state();
    std::shared_lock lock(s.levels_mutex);
    std::string key(category);
    for (;;) {
        const auto it = s.levels.find(key);
        if (it != s.levels.end()) return it->second;
        const auto dot = key.rfind('.');
        if (dot == std::string::npos) break;
        key.erase(dot);
    }
    const auto def = s.levels.find("");
    return def != s.levels.end() ? def->second : Level::info;
}

bool enabled(std::string_view category, Level level) noexcept
{
    if (level == Level::off) return false;
    return static_cast<int>(level) >= static_cast<int>(effective_level(category));
}

void write(Level level, std::string_view category, std::string_view msg, const std::vector<Field>& fields)
{
    if (!enabled(category, level)) return;
    State& s = state();

    nlohmann::ordered_json j;
    j["ts"] = timestamp_utc();
    j["lvl"] = level_name(level);
    j["cat"] = std::string(category);
    if (!s.options.process.empty()) j["proc"] = s.options.process;
    const Context& c = t_context;
    if (c.sid)  j["sid"]  = c.sid;
    if (c.pid)  j["pid"]  = c.pid;
    if (c.zone) j["zone"] = c.zone;
    if (c.tick) j["tick"] = c.tick;
    j["tid"] = thread_id_hash() & 0xFFFFu;
    j["msg"] = std::string(msg);
    for (const Field& f : fields) {
        j[f.key] = f.value;
    }
    const std::string line = j.dump();

    push_ring(s, line);
    if (s.logger) {
        s.logger->log(level >= Level::warn ? spdlog::level::warn : spdlog::level::info, "{}", line);
    }
}

void write(Level level, std::string_view category, std::string_view msg, std::initializer_list<Field> fields)
{
    write(level, category, msg, std::vector<Field>(fields));
}

void fatal(std::string_view cat, std::string_view msg, std::initializer_list<Field> f)
{
    write(Level::fatal, cat, msg, f);
    State& s = state();
    std::filesystem::path dump = s.options.file.empty() ? std::filesystem::path("jx.crash.log")
                                                        : std::filesystem::path(s.options.file).replace_extension(".crash.log");
    dump_ring(dump);
    flush();
}

std::vector<std::string> ring_snapshot()
{
    State& s = state();
    std::lock_guard lock(s.ring_mutex);
    return std::vector<std::string>(s.ring.begin(), s.ring.end());
}

void dump_ring(const std::filesystem::path& path)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    for (const std::string& line : ring_snapshot()) {
        out << line << '\n';
    }
}

Context& context() noexcept
{
    return t_context;
}

ScopedContext::ScopedContext(const Context& ctx) noexcept : saved_(t_context)
{
    t_context = ctx;
}

ScopedContext::~ScopedContext()
{
    t_context = saved_;
}

} // namespace jx::log
