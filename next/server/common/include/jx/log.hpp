// jx::log - structured logging shared by every JX NEXT process.
//
// One JSON object per line, same schema as the Go services and the Godot client (docs/LOGGING.md):
//   {"ts":"2026-09-16T08:00:00.123456Z","lvl":"info","cat":"net","proc":"zone",
//    "sid":42,"pid":7,"zone":3,"tick":99,"msg":"connected","addr":"127.0.0.1"}
// Levels are decided per category at run time (set_levels("net=trace,zone.tick=debug")), the last
// N lines are kept in a ring buffer that is written next to the log file on fatal().
#pragma once

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
    std::filesystem::path file;                        // empty = no file sink
    std::size_t rotate_bytes = 32u * 1024u * 1024u;    // rotating file sink
    std::size_t rotate_files = 5;
    std::size_t ring_capacity = 10000;                 // lines kept for crash dumps
    std::string process;                               // "zone", "tool", ... -> "proc" field
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
