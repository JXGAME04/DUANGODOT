#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "jx/config.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace {

const char* const kSample = R"json({
    // comments are allowed
    "log": { "level": "info", "file": "zone.log", "rotate_bytes": 1048576 },
    "net": { "port": 15622, "tls": false, "host": "127.0.0.1" },
    "tick": { "hz": 20.0 }
})json";

void set_env(const char* key, const char* value)
{
#ifdef _WIN32
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

} // namespace

TEST_CASE("typed getters read dotted paths", "[config]")
{
    const auto cfg = jx::config::Config::from_json_text(kSample);
    CHECK(cfg.get_string("log.level", "x") == "info");
    CHECK(cfg.get_string("log.file", "x") == "zone.log");
    CHECK(cfg.get_int("net.port", 0) == 15622);
    CHECK(cfg.get_int("log.rotate_bytes", 0) == 1048576);
    CHECK(cfg.get_int("tick.hz", 0) == 20);
    CHECK(cfg.get_bool("net.tls", true) == false);
    CHECK(cfg.get_string("net.port", "") == "15622");   // numbers stringify
    CHECK(cfg.has("net.host"));
    CHECK_FALSE(cfg.has("net.missing"));
    CHECK(cfg.get_string("net.missing", "def") == "def");
    CHECK(cfg.get_int("net.host", 9) == 9);              // not a number -> default
    CHECK(cfg.get_bool("log.level", true) == true);      // not a bool -> default
    CHECK_FALSE(cfg.get_string("nope.x").has_value());
}

TEST_CASE("overrides are auto-typed and create missing sections", "[config]")
{
    auto cfg = jx::config::Config::from_json_text(kSample);
    cfg.apply_overrides({{"log.level", "debug"}, {"net.port", "16666"}, {"net.tls", "true"}, {"new.section.key", "hello"}});
    CHECK(cfg.get_string("log.level", "") == "debug");
    CHECK(cfg.get_int("net.port", 0) == 16666);
    CHECK(cfg.json().at("net").at("port").is_number_integer());
    CHECK(cfg.get_bool("net.tls", false) == true);
    CHECK(cfg.json().at("net").at("tls").is_boolean());
    CHECK(cfg.get_string("new.section.key", "") == "hello");
    CHECK(cfg.get_string("net.host", "") == "127.0.0.1");   // untouched

    cfg.set("log", "plain");            // replacing a section with a scalar ...
    cfg.set("log.level", "warn");       // ... and setting a child again re-creates the object
    CHECK(cfg.get_string("log.level", "") == "warn");
}

TEST_CASE("string values parse as numbers and bools when asked", "[config]")
{
    jx::config::Config cfg;
    cfg.set("a", "\"42\"");
    cfg.set("b", "\"yes\"");
    cfg.set("c", "\"off\"");
    cfg.set("d", "\"12abc\"");
    CHECK(cfg.json().at("a").is_string());
    CHECK(cfg.get_int("a", 0) == 42);
    CHECK(cfg.get_bool("b", false) == true);
    CHECK(cfg.get_bool("c", true) == false);
    CHECK(cfg.get_int("d", -1) == -1);
    CHECK(cfg.get_bool("d", true) == true);
}

TEST_CASE("environment keys map JX_SECTION__KEY to section.key", "[config]")
{
    using jx::config::env_key_to_path;
    CHECK(env_key_to_path("JX_LOG__LEVEL") == "log.level");
    CHECK(env_key_to_path("JX_NET__PORT") == "net.port");
    CHECK(env_key_to_path("JX_LOG__ROTATE_BYTES") == "log.rotate_bytes");
    CHECK(env_key_to_path("JX_A__B__C") == "a.b.c");
    CHECK(env_key_to_path("JX_").empty());
    CHECK(env_key_to_path("PATH").empty());
    CHECK(env_key_to_path("JXNEXT_X", "JXNEXT_") == "x");

    auto cfg = jx::config::Config::from_json_text(kSample);
    cfg.apply_env_list({"JX_LOG__LEVEL=trace", "JX_NET__PORT=17000", "HOME=/nowhere", "JX_=bad", "=weird"});
    CHECK(cfg.get_string("log.level", "") == "trace");
    CHECK(cfg.get_int("net.port", 0) == 17000);
    CHECK_FALSE(cfg.has("home"));
}

TEST_CASE("the real process environment is applied", "[config]")
{
    set_env("JX_TEST__FROM_ENV", "on");
    jx::config::Config cfg;
    cfg.apply_env();
    CHECK(cfg.get_bool("test.from_env", false) == true);
}

TEST_CASE("files load and errors are reported", "[config]")
{
    const auto dir = std::filesystem::temp_directory_path() / "jxnext-test-config";
    std::filesystem::create_directories(dir);
    const auto path = dir / "zone.json";
    {
        std::ofstream out(path, std::ios::binary);
        out << kSample;
    }
    const auto cfg = jx::config::Config::from_file(path);
    CHECK(cfg.get_int("net.port", 0) == 15622);

    CHECK_THROWS_AS(jx::config::Config::from_file(dir / "missing.json"), std::runtime_error);
    CHECK_THROWS_AS(jx::config::Config::from_json_text("{ not json"), std::runtime_error);
    CHECK_THROWS_AS(jx::config::Config::from_json_text("[1,2]"), std::runtime_error);
    std::filesystem::remove_all(dir);
}

TEST_CASE("merge is a deep merge where the newer document wins", "[config]")
{
    auto cfg = jx::config::Config::from_json_text(kSample);
    cfg.merge(nlohmann::json::parse(R"({"net":{"port":1},"extra":{"k":"v"}})"));
    CHECK(cfg.get_int("net.port", 0) == 1);
    CHECK(cfg.get_string("net.host", "") == "127.0.0.1");
    CHECK(cfg.get_string("extra.k", "") == "v");
}
