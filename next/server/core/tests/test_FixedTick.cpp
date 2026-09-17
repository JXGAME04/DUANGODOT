// FixedTick: the loop runs a whole number of ticks from real elapsed time and never spirals
// (MASTER SPEC 20, 22, 23, 54).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "jx/core/FixedTick.h"

using jx::Nanos;
using jx::core::FixedTick;
using jx::core::Metrics;
using jx::core::ServerClock;
using jx::core::ServerTick;
using jx::core::TickPhase;
using jx::core::TickProfile;
using jx::core::TickRate;

TEST_CASE("elapsed time becomes whole ticks", "[core][tick]")
{
    ServerClock clock(TickRate{30});
    FixedTick driver(clock);
    std::vector<ServerTick> ran;
    const auto record = [&](ServerTick t) { ran.push_back(t); };

    CHECK(driver.update(Nanos{10'000'000}, record) == 0);   // 10 ms: not a whole tick yet
    CHECK(ran.empty());
    CHECK(driver.update(Nanos{25'000'000}, record) == 1);   // 35 ms total -> one tick, 1.7 ms left
    REQUIRE(ran.size() == 1);
    CHECK(ran[0] == 1);
    CHECK(clock.tick() == 1);

    ran.clear();
    CHECK(driver.update(Nanos{100'000'000}, record) == 3);   // 101.7 ms -> three ticks
    CHECK(ran == std::vector<ServerTick>{2, 3, 4});
    CHECK(driver.dropped() == 0);
}

TEST_CASE("a stall drops the backlog instead of spiralling", "[core][tick]")
{
    ServerClock clock(TickRate{30});
    FixedTick driver(clock, 5);
    std::vector<ServerTick> ran;
    // the machine froze for two seconds: 60 ticks are due, only five may run
    CHECK(driver.update(Nanos{2'000'000'000}, [&](ServerTick t) { ran.push_back(t); }) == 5);
    CHECK(ran.size() == 5);
    CHECK(driver.dropped() == 55);
    CHECK(clock.tick() == 5);

    driver.reset();
    CHECK(driver.dropped() == 0);
    CHECK(driver.update(Nanos{-1}, [](ServerTick) { FAIL("negative time must not run a tick"); }) == 0);
}

TEST_CASE("tick phases are timed under their spec names", "[core][tick][metrics]")
{
    Metrics m;
    TickProfile profile(m, "test");
    {
        auto whole = profile.whole();
        { auto p = profile.phase(TickPhase::movement); }
        { auto p = profile.phase(TickPhase::ai); }
        { auto p = profile.phase(TickPhase::snapshot); }
    }
    CHECK(profile.total().count() == 1);
    CHECK(profile.phase_timing(TickPhase::movement).count() == 1);
    CHECK(profile.phase_timing(TickPhase::ai).count() == 1);
    CHECK(profile.phase_timing(TickPhase::combat).count() == 0);

    const std::string json = m.to_json();
    CHECK(json.find("test.tick.total") != std::string::npos);
    CHECK(json.find("test.tick.movement") != std::string::npos);
    CHECK(json.find("test.tick.entity_transfer") != std::string::npos);
    CHECK(std::string(jx::core::phase_name(TickPhase::commit_commands)) == "commit");
}
