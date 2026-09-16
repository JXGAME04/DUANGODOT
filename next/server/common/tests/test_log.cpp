#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
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
