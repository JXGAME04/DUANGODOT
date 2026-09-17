// jx::log - structured logging shared by every JX NEXT process.
//
// One JSON object per line, same schema as the Go services and the Godot client (docs/LOGGING.md):
//   {"ts":"2026-09-16T08:00:00.123456Z","lvl":"info","cat":"net","proc":"zone",
//    "sid":42,"pid":7,"zone":3,"tick":99,"msg":"connected","addr":"127.0.0.1"}
// That is what goes to the FILE, for tools.  The CONSOLE is for the person running the server:
//   14:32:05.123 THÔNG TIN    [khởi động] Zone đã mở cổng, chờ gateway kết nối · cổng=17001
// one sentence per line, in the language of the catalogue (config/log.vi.json maps every English
// `msg`, category and field name to Vietnamese; what it does not know stays English).
// Levels are decided per category at run time (set_levels("net=trace,zone.tick=debug")), the last
// N lines are kept in a ring buffer that is written next to the log file on fatal().
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

namespace jx::log {

enum class Level : int { trace = 0, debug, info, warn, error, fatal, off };

const char* level_name(Level level) noexcept;
Level level_from_name(std::string_view name) noexcept;   // unknown -> info

struct Field {
    std::string key;
    std::string value;
};

// kv("port", 15622) -> Field{"port", "15622"}; any fmt-formattable value.
template <class T>
Field kv(std::string_view key, const T& value)
{
    return Field{std::string(key), fmt::format("{}", value)};
}

struct Options {
    Level default_level = Level::info;
    bool console = true;
    // "text": readable lines (see above); "json": the same JSON lines as the file, for a console
    // that is piped into a tool.
    std::string console_style = "text";
    // Language of the text console: "vi" reads the catalogue, "en" prints the messages as the code
    // has them.  The catalogue is looked for at `catalog`, else config/log.<language>.json below
    // the working directory and its parents.
    std::string language = "vi";
    std::filesystem::path catalog;
    std::filesystem::path file;                        // empty = no file sink
    std::size_t rotate_bytes = 32u * 1024u * 1024u;    // rotating file sink
    std::size_t rotate_files = 5;
    std::size_t ring_capacity = 10000;                 // lines kept for crash dumps
    std::string process;                               // "zone", "tool", ... -> "proc" field
    // MASTER SPEC 49: a simulation worker must never write a file synchronously.  With async
    // the formatted line is handed to a logging thread; the queue is bounded, and a full queue
    // overwrites the oldest line instead of blocking the tick (the count is logged on shutdown).
    bool async = true;
    std::size_t async_queue = 16384;
};

void init(const Options& options);
void shutdown();
void flush();

void set_level(std::string_view category, Level level);   // "" = default level
void set_levels(std::string_view spec);                   // "net=trace,zone.tick=debug,=warn"
Level effective_level(std::string_view category) noexcept; // longest matching prefix on '.'
bool enabled(std::string_view category, Level level) noexcept;

void write(Level level, std::string_view category, std::string_view msg,
           std::initializer_list<Field> fields = {});
void write(Level level, std::string_view category, std::string_view msg,
           const std::vector<Field>& fields);

inline void trace(std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {}) { write(Level::trace, cat, msg, f); }
inline void debug(std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {}) { write(Level::debug, cat, msg, f); }
inline void info (std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {}) { write(Level::info,  cat, msg, f); }
inline void warn (std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {}) { write(Level::warn,  cat, msg, f); }
inline void error(std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {}) { write(Level::error, cat, msg, f); }
// fatal also flushes and dumps the ring buffer (<file>.crash.log next to the log file, or cwd).
void fatal(std::string_view cat, std::string_view msg, std::initializer_list<Field> f = {});

// How many lines the async queue had to drop because it was full (0 in normal operation).
std::uint64_t dropped_lines() noexcept;

std::vector<std::string> ring_snapshot();
void dump_ring(const std::filesystem::path& path);

// Correlation context merged into every line written on the calling thread.
struct Context {
    std::uint64_t sid = 0;    // session id (assigned by the gateway)
    std::uint64_t pid = 0;    // player id
    std::uint32_t zone = 0;
    std::uint64_t tick = 0;
};
Context& context() noexcept;

// The text-console form of one line (what init(console_style = "text") prints), for tests and tools.
std::string format_text(Level level, std::string_view category, std::string_view msg,
                        const std::vector<Field>& fields, const Context& context);
// How many sentences / categories / field names the loaded catalogue translates (0 = none loaded).
std::size_t catalog_size() noexcept;

class ScopedContext {
public:
    explicit ScopedContext(const Context& ctx) noexcept;
    ~ScopedContext();
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
private:
    Context saved_;
};

} // namespace jx::log
