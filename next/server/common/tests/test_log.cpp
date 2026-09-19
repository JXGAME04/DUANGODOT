#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "jx/ids.hpp"
#include "jx/log.hpp"

namespace {

jx::log::Options quiet(std::size_t ring = 100)
{
    jx::log::Options o;
    o.console = false;
    o.ring_capacity = ring;
    o.process = "test";
    o.default_level = jx::log::Level::trace;
    return o;
}

nlohmann::json last_line()
{
    const auto lines = jx::log::ring_snapshot();
    if (lines.empty()) throw std::runtime_error("ring is empty");
    return nlohmann::json::parse(lines.back());
}

} // namespace

TEST_CASE("log line is one JSON object with the standard fields", "[log]")
{
    jx::log::init(quiet());
    jx::log::info("net", "connected", {jx::log::kv("addr", "127.0.0.1"), jx::log::kv("port", 15622)});

    const auto j = last_line();
    CHECK(j.at("lvl") == "info");
    CHECK(j.at("cat") == "net");
    CHECK(j.at("proc") == "test");
    CHECK(j.at("msg") == "connected");
    CHECK(j.at("addr") == "127.0.0.1");
    CHECK(j.at("port") == "15622");
    CHECK_FALSE(j.contains("sid"));

    const std::string ts = j.at("ts");
    CHECK(ts.size() == 27);          // 2026-09-16T08:00:00.123456Z
    CHECK(ts[10] == 'T');
    CHECK(ts.back() == 'Z');

    // ordered_json keeps the schema order so raw lines stay readable
    const std::string raw = jx::log::ring_snapshot().back();
    CHECK(raw.rfind("{\"ts\":", 0) == 0);
    CHECK(raw.find("\"lvl\"") < raw.find("\"cat\""));
    CHECK(raw.find("\"cat\"") < raw.find("\"msg\""));

    jx::log::shutdown();
}

TEST_CASE("thread context is merged into every line", "[log]")
{
    jx::log::init(quiet());
    {
        jx::log::ScopedContext ctx(jx::log::Context{42, 7, 3, 99});
        jx::log::debug("zone.tick", "tick");
        const auto j = last_line();
        CHECK(j.at("sid") == 42);
        CHECK(j.at("pid") == 7);
        CHECK(j.at("zone") == 3);
        CHECK(j.at("tick") == 99);
    }
    jx::log::info("zone", "after scope");
    const auto j = last_line();
    CHECK_FALSE(j.contains("sid"));
    CHECK_FALSE(j.contains("tick"));

    jx::log::context().pid = 5;
    jx::log::info("zone", "manual context");
    CHECK(last_line().at("pid") == 5);
    jx::log::context() = jx::log::Context{};
    jx::log::shutdown();
}

TEST_CASE("typed ids format as plain numbers in fields", "[log][ids]")
{
    const auto f = jx::log::kv("pid", jx::PlayerId{7});
    CHECK(f.key == "pid");
    CHECK(f.value == "7");
}

TEST_CASE("levels are resolved per category by longest dotted prefix", "[log]")
{
    using jx::log::Level;
    jx::log::init(quiet());
    jx::log::set_levels("net=trace, zone.tick=debug, =warn");

    CHECK(jx::log::effective_level("net") == Level::trace);
    CHECK(jx::log::effective_level("net.recv") == Level::trace);
    CHECK(jx::log::effective_level("zone.tick") == Level::debug);
    CHECK(jx::log::effective_level("zone.tick.ai") == Level::debug);
    CHECK(jx::log::effective_level("zone") == Level::warn);
    CHECK(jx::log::effective_level("other") == Level::warn);

    CHECK(jx::log::enabled("zone.tick", Level::debug));
    CHECK_FALSE(jx::log::enabled("zone.tick", Level::trace));
    CHECK_FALSE(jx::log::enabled("zone", Level::info));
    CHECK(jx::log::enabled("zone", Level::error));
    CHECK_FALSE(jx::log::enabled("net", Level::off));

    jx::log::set_level("zone", Level::info);
    CHECK(jx::log::effective_level("zone.ai") == Level::info);
    CHECK(jx::log::effective_level("zone.tick") == Level::debug);   // more specific rule still wins

    const auto before = jx::log::ring_snapshot().size();
    jx::log::trace("zone.ai", "filtered out");
    CHECK(jx::log::ring_snapshot().size() == before);
    jx::log::info("zone.ai", "kept");
    CHECK(jx::log::ring_snapshot().size() == before + 1);

    jx::log::shutdown();
}

TEST_CASE("level names round trip", "[log]")
{
    using jx::log::Level;
    for (Level l : {Level::trace, Level::debug, Level::info, Level::warn, Level::error, Level::fatal, Level::off}) {
        CHECK(jx::log::level_from_name(jx::log::level_name(l)) == l);
    }
    CHECK(jx::log::level_from_name("WARNING") == Level::info);   // unknown -> info (names are lower case by design)
    CHECK(jx::log::level_from_name("warning") == Level::warn);
    CHECK(jx::log::level_from_name(" debug ") == Level::debug);
}

TEST_CASE("ring buffer keeps only the last N lines", "[log]")
{
    jx::log::init(quiet(3));
    for (int i = 0; i < 5; ++i) jx::log::info("t", fmt::format("line{}", i));
    const auto lines = jx::log::ring_snapshot();
    REQUIRE(lines.size() == 3);
    CHECK(nlohmann::json::parse(lines.front()).at("msg") == "line2");
    CHECK(nlohmann::json::parse(lines.back()).at("msg") == "line4");
    jx::log::shutdown();
}

TEST_CASE("fatal writes the log file and dumps the ring next to it", "[log]")
{
    const auto dir = std::filesystem::temp_directory_path() / "jxnext-test-log";
    std::filesystem::remove_all(dir);

    auto o = quiet();
    o.file = dir / "zone.log";
    jx::log::init(o);
    jx::log::info("boot", "starting");
    jx::log::fatal("boot", "cannot bind", {jx::log::kv("port", 16666)});
    jx::log::shutdown();

    REQUIRE(std::filesystem::exists(dir / "zone.log"));
    CHECK(std::filesystem::file_size(dir / "zone.log") > 0);
    REQUIRE(std::filesystem::exists(dir / "zone.crash.log"));
    {
        std::ifstream in(dir / "zone.crash.log");
        std::string line;
        std::string last;
        std::size_t count = 0;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            ++count;
            last = line;
        }
        CHECK(count == 2);
        CHECK(nlohmann::json::parse(last).at("lvl") == "fatal");
        CHECK(nlohmann::json::parse(last).at("port") == "16666");
    }
    std::filesystem::remove_all(dir);
}

// MASTER SPEC 49: a simulation worker hands the line to a logging thread instead of writing the
// file itself.  Every line must still reach the file, and the process must survive many threads
// logging at once.
TEST_CASE("asynchronous logging keeps every line", "[log][async]")
{
    const auto dir = std::filesystem::temp_directory_path() / "jxnext-test-log-async";
    std::filesystem::remove_all(dir);

    auto o = quiet();
    o.file = dir / "async.log";
    o.async = true;
    o.async_queue = 8192;
    jx::log::init(o);

    constexpr int kThreads = 4;
    constexpr int kLines = 500;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kLines; ++i) {
                jx::log::warn("worker", "line", {jx::log::kv("thread", t), jx::log::kv("i", i)});
            }
        });
    }
    for (auto& th : threads) th.join();
    jx::log::flush();
    jx::log::shutdown();

    REQUIRE(std::filesystem::exists(dir / "async.log"));
    std::size_t count = 0;
    {
        std::ifstream in(dir / "async.log");
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) ++count;
        }
    }
    CHECK(count == kThreads * kLines);
    std::filesystem::remove_all(dir);
}

// The console is for a person: one sentence per line, in the catalogue's language, while the ring
// (and the file) keep the English JSON a tool reads.
TEST_CASE("text console speaks through the catalogue, the ring stays JSON", "[log][console]")
{
    const auto dir = std::filesystem::temp_directory_path() / "jx_log_console_test";
    std::filesystem::create_directories(dir);
    const auto catalog = dir / "log.vi.json";
    {
        std::ofstream out(catalog, std::ios::binary);
        out << R"({"levels":{"info":"THÔNG TIN"},"categories":{"boot":"khởi động","zone":"zone"},)"
            << R"("fields":{"port":"cổng","sid":"phiên"},"messages":{"zone listening":"Zone đã mở cổng, chờ gateway kết nối"}})";
    }
    jx::log::Options o = quiet();
    o.console = true;          // the sink writes to this test's stdout; what matters is format_text
    o.console_style = "text";
    o.language = "vi";
    o.catalog = catalog;
    o.async = false;
    jx::log::init(o);
    REQUIRE(jx::log::catalog_size() == 6);

    jx::log::Context ctx;
    ctx.sid = 42;
    const std::vector<jx::log::Field> fields{jx::log::kv("port", 19001), jx::log::kv("addr", "0.0.0.0")};
    CHECK(jx::log::format_text(jx::log::Level::info, "boot", "zone listening", fields, ctx) ==
          "[khởi động]   Zone đã mở cổng, chờ gateway kết nối \xC2\xB7 cổng=19001 \xC2\xB7 addr=0.0.0.0 \xC2\xB7 phiên=42");
    // what the catalogue does not know stays English; a category is translated by its first part
    CHECK(jx::log::format_text(jx::log::Level::warn, "zone.fight", "a brand new message", {}, jx::log::Context{}) ==
          "[zone.fight]  a brand new message");

    jx::log::info("boot", "zone listening", {jx::log::kv("port", 19001)});
    const auto j = last_line();
    CHECK(j["msg"] == "zone listening");     // English for the tools
    CHECK(j["port"] == "19001");

    // English console: no catalogue is read at all
    o.language = "en";
    jx::log::init(o);
    CHECK(jx::log::catalog_size() == 0);
    jx::log::shutdown();
    std::filesystem::remove_all(dir);
}
