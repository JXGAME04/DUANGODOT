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
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>

#include <fmt/chrono.h>
#include <nlohmann/json.hpp>
#include <spdlog/async.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace jx::log {
namespace {

// What the text console says instead of the English of the code (config/log.<language>.json).
struct Catalog {
    std::unordered_map<std::string, std::string> levels, categories, fields, messages;
    std::size_t size() const { return levels.size() + categories.size() + fields.size() + messages.size(); }
};

struct State {
    std::shared_ptr<spdlog::logger> logger;           // the file: JSON lines
    std::shared_ptr<spdlog::logger> console;          // the console: text (or the same JSON)
    Catalog catalog;
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

const std::string& translated(const std::unordered_map<std::string, std::string>& table, const std::string& key)
{
    const auto it = table.find(key);
    return it != table.end() && !it->second.empty() ? it->second : key;
}

// Columns a UTF-8 string takes on screen, close enough for padding: one per code point.
std::size_t columns(std::string_view s)
{
    std::size_t n = 0;
    for (const char c : s) {
        if ((static_cast<unsigned char>(c) & 0xC0u) != 0x80u) ++n;
    }
    return n;
}

std::filesystem::path find_catalog(const Options& o)
{
    if (!o.catalog.empty()) return o.catalog;
    const std::filesystem::path name = std::filesystem::path("config") / ("log." + o.language + ".json");
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::current_path(ec);
    for (int up = 0; up < 4 && !dir.empty(); ++up) {
        if (std::filesystem::exists(dir / name, ec)) return dir / name;
        if (!dir.has_parent_path() || dir.parent_path() == dir) break;
        dir = dir.parent_path();
    }
    return {};
}

Catalog load_catalog(const std::filesystem::path& path)
{
    Catalog c;
    if (path.empty()) return c;
    std::ifstream in(path, std::ios::binary);
    if (!in) return c;
    const nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_object()) return c;
    const auto take = [&j](const char* key, std::unordered_map<std::string, std::string>& into) {
        const auto it = j.find(key);
        if (it == j.end() || !it->is_object()) return;
        for (const auto& [k, v] : it->items()) {
            if (v.is_string()) into[k] = v.get<std::string>();
        }
    };
    take("levels", c.levels);
    take("categories", c.categories);
    take("fields", c.fields);
    take("messages", c.messages);
    return c;
}

// %* of the console pattern: the level in the catalogue's words, padded so the lines align.
class LevelLabel final : public spdlog::custom_flag_formatter {
public:
    explicit LevelLabel(std::unordered_map<std::string, std::string> labels) : labels_(std::move(labels)) {}

    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
    {
        static constexpr const char* kNames[] = {"trace", "debug", "info", "warn", "error", "fatal", "off"};
        static constexpr const char* kEnglish[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"};
        const int level = std::clamp(static_cast<int>(msg.level), 0, 6);
        const auto it = labels_.find(kNames[level]);
        const std::string label = it != labels_.end() ? it->second : std::string(kEnglish[level]);
        dest.append(label.data(), label.data() + label.size());
        for (std::size_t n = columns(label); n < 12; ++n) dest.push_back(' ');
    }

    std::unique_ptr<spdlog::custom_flag_formatter> clone() const override { return std::make_unique<LevelLabel>(labels_); }

private:
    std::unordered_map<std::string, std::string> labels_;
};

spdlog::level::level_enum spd_level(Level level) noexcept
{
    switch (level) {
    case Level::trace: return spdlog::level::trace;
    case Level::debug: return spdlog::level::debug;
    case Level::info:  return spdlog::level::info;
    case Level::warn:  return spdlog::level::warn;
    case Level::error: return spdlog::level::err;
    case Level::fatal: return spdlog::level::critical;
    case Level::off:   return spdlog::level::off;
    }
    return spdlog::level::info;
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

    const bool text_console = options.console && options.console_style != "json";
    s.catalog = text_console && options.language != "en" ? load_catalog(find_catalog(options)) : Catalog{};
    spdlog::sink_ptr console_sink;
    if (options.console) {
#ifdef _WIN32
        ::SetConsoleOutputCP(CP_UTF8);     // the sentences are UTF-8; a Windows console defaults to an OEM page
#endif
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        if (text_console) {
            auto formatter = std::make_unique<spdlog::pattern_formatter>();
            formatter->add_flag<LevelLabel>('*', s.catalog.levels).set_pattern("%H:%M:%S.%e %^%*%$ %v");
            console_sink->set_formatter(std::move(formatter));
        }
    }
    std::vector<spdlog::sink_ptr> sinks;
    if (console_sink && !text_console) {
        sinks.push_back(console_sink);     // JSON on the console: the same lines as the file
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
        if (text_console) {
            s.console = std::make_shared<spdlog::async_logger>("jx.console", console_sink, spdlog::thread_pool(),
                                                               spdlog::async_overflow_policy::overrun_oldest);
        }
    } else {
        s.logger = std::make_shared<spdlog::logger>("jx", sinks.begin(), sinks.end());
        if (text_console) s.console = std::make_shared<spdlog::logger>("jx.console", console_sink);
    }
    for (const spdlog::sink_ptr& sink : sinks) {
        sink->set_pattern("%v");            // the line is already a complete JSON object
    }
    s.logger->set_level(spdlog::level::trace);
    s.logger->flush_on(spdlog::level::warn);
    if (s.console) {
        s.console->set_level(spdlog::level::trace);
        s.console->flush_on(spdlog::level::info);   // a person is watching: no line may wait
        spdlog::drop("jx.console");
        spdlog::register_logger(s.console);
    }
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
    if (s.console) {
        s.console->flush();
        spdlog::drop("jx.console");
        s.console.reset();
    }
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
    if (s.console) s.console->flush();
}

std::size_t catalog_size() noexcept
{
    return state().catalog.size();
}

// "[khởi động] Zone đã mở cổng, chờ gateway kết nối · cổng=17001 · phiên=42"
std::string format_text(Level level, std::string_view category, std::string_view msg,
                        const std::vector<Field>& fields, const Context& context)
{
    (void)level;
    const Catalog& c = state().catalog;
    // a category is translated by its first part: "zone.fight" reads as "zone" + ".fight"
    std::string cat(category);
    const auto dot = cat.find('.');
    const std::string head = cat.substr(0, dot);
    std::string shown = translated(c.categories, cat);
    if (shown == cat && dot != std::string::npos) shown = translated(c.categories, head) + cat.substr(dot);
    std::string line = "[" + shown + "]";
    for (std::size_t n = columns(line); n < 14; ++n) line.push_back(' ');
    line += translated(c.messages, std::string(msg));
    const auto add = [&line, &c](const std::string& key, const std::string& value) {
        line += " \xC2\xB7 ";     // a middle dot between the parts
        line += translated(c.fields, key);
        line += '=';
        line += value;
    };
    for (const Field& f : fields) add(f.key, f.value);
    if (context.sid)  add("sid", std::to_string(context.sid));
    if (context.pid)  add("pid", std::to_string(context.pid));
    if (context.zone) add("zone", std::to_string(context.zone));
    return line;
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
        s.logger->log(spd_level(level), "{}", line);
    }
    if (s.console) {
        s.console->log(spd_level(level), "{}", format_text(level, category, msg, fields, c));
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
