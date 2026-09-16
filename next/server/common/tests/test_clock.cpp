#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <vector>

#include "jx/clock.hpp"

using namespace std::chrono_literals;

TEST_CASE("fixed step at 20 Hz turns elapsed time into whole ticks", "[clock]")
{
    jx::FixedStep step(20);
    CHECK(step.step() == 50ms);
    CHECK(step.hz() == 20);

    CHECK(step.update(100ms) == 2);
    CHECK(step.tick() == 2);
    CHECK(step.alpha() == Catch::Approx(0.0));

    CHECK(step.update(25ms) == 0);
    CHECK(step.alpha() == Catch::Approx(0.5));

    CHECK(step.update(25ms) == 1);
    CHECK(step.tick() == 3);
    CHECK(step.alpha() == Catch::Approx(0.0));
    CHECK(step.dropped() == 0);

    CHECK(step.update(-10ms) == 0);   // negative elapsed (clock went backwards) is ignored
    CHECK(step.alpha() == Catch::Approx(0.0));
}

TEST_CASE("fixed step clamps a backlog instead of spiralling", "[clock]")
{
    jx::FixedStep step(20, 5);
    CHECK(step.update(1s) == 5);       // 20 ticks owed, 5 run, 15 dropped
    CHECK(step.tick() == 5);
    CHECK(step.dropped() == 15);
    CHECK(step.alpha() == Catch::Approx(0.0));

    step.reset();
    CHECK(step.tick() == 0);
    CHECK(step.dropped() == 0);
}

TEST_CASE("fixed step is deterministic for the same input sequence", "[clock]")
{
    const std::vector<jx::Nanos> frames = {16ms, 17ms, 16ms, 33ms, 8ms, 120ms, 16ms, 16ms};   // 242 ms

    // no clamping (max_ticks large): every owed tick runs -> 242 ms / 16.666 ms = 14 ticks
    jx::FixedStep a(60, 100);
    jx::FixedStep b(60, 100);
    for (const auto& f : frames) CHECK(a.update(f) == b.update(f));
    CHECK(a.tick() == b.tick());
    CHECK(a.accumulator() == b.accumulator());
    CHECK(a.tick() == 14);
    CHECK(a.dropped() == 0);

    // default clamping (5 ticks per update): the 120 ms frame owes 7 ticks, runs 5, drops 2
    jx::FixedStep c(60);
    jx::FixedStep d(60);
    for (const auto& f : frames) CHECK(c.update(f) == d.update(f));
    CHECK(c.tick() == 12);
    CHECK(c.dropped() == 2);
    CHECK(c.accumulator() == d.accumulator());
}

TEST_CASE("fixed step never divides by zero", "[clock]")
{
    jx::FixedStep step(0, 0);
    CHECK(step.hz() == 1);
    CHECK(step.update(2s) == 1);
    CHECK(step.dropped() == 1);
}

TEST_CASE("manual clock only moves when told", "[clock]")
{
    jx::ManualClock clock(jx::Nanos{0});
    CHECK(clock.now() == jx::Nanos{0});
    clock.advance(1500ms);
    CHECK(clock.now() == 1500ms);
    clock.set(2s);
    CHECK(clock.now() == 2s);

    const jx::IClock& iface = clock;
    CHECK(iface.now() == 2s);
}

TEST_CASE("steady clock is monotonic", "[clock]")
{
    jx::SteadyClock clock;
    const auto t0 = clock.now();
    const auto t1 = clock.now();
    CHECK(t1 >= t0);
    CHECK(jx::unix_millis() > 1'700'000'000'000LL);   // after 2023-11
}
