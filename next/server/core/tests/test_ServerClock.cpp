// ServerClock / ServerTick: gameplay time is ticks, never wall clock (MASTER SPEC 21, 59, 61).
#include <catch2/catch_test_macros.hpp>

#include "jx/core/ServerClock.h"

using jx::ManualClock;
using jx::Nanos;
using jx::core::ServerClock;
using jx::core::ServerTick;
using jx::core::TickRate;

TEST_CASE("a tick rate converts between milliseconds and ticks", "[core][clock]")
{
    const TickRate r30{30};
    CHECK(r30.hz() == 30);
    CHECK(r30.step() == Nanos{33'333'333});
    CHECK(r30.step_ms() > 33.3);
    CHECK(r30.step_ms() < 33.4);

    // a cooldown never expires early: 100 ms is 4 ticks at 30 Hz (3 would be 99.99 ms)
    CHECK(r30.ticks_from_ms(100) == 4);
    CHECK(r30.ticks_from_ms(33) == 1);
    CHECK(r30.ticks_from_ms(34) == 2);
    CHECK(r30.ticks_from_ms(0) == 0);
    CHECK(r30.ticks_from_ms(-5) == 0);
    CHECK(r30.ms_from_ticks(30) == 999);   // 30 * 33.333 ms

    const TickRate r18{18};   // the logic rate of the old game
    // the invariant is what matters: a duration in ticks is never shorter than it was asked for
    // (the step is a whole number of nanoseconds, so 18 ticks are 9.99...e8 ns, not exactly 1 s)
    CHECK(r18.ticks_from_ms(1000) == 19);
    CHECK(r18.ms_from_ticks(r18.ticks_from_ms(1000)) >= 1000);
    CHECK(r30.ms_from_ticks(r30.ticks_from_ms(250)) >= 250);
    CHECK(TickRate{0}.hz() == 1);   // never divides by zero
}

TEST_CASE("the server clock only moves when the tick driver advances it", "[core][clock]")
{
    ManualClock manual;
    ServerClock clock(TickRate{20}, &manual);
    CHECK(clock.tick() == 0);
    CHECK(clock.now() == Nanos{0});

    clock.advance();
    CHECK(clock.tick() == 1);
    clock.advance(9);
    CHECK(clock.tick() == 10);

    manual.advance(Nanos{500});
    CHECK(clock.now() == Nanos{500});   // wall time moves on its own, the tick does not

    // deadlines are expressed in ticks, so the same input always gives the same result
    const ServerTick deadline = clock.after_ms(500);   // 10 ticks at 20 Hz
    CHECK(deadline == 20);
    CHECK_FALSE(clock.reached(deadline));
    clock.advance(10);
    CHECK(clock.reached(deadline));

    clock.reset();
    CHECK(clock.tick() == 0);
    CHECK(clock.unix_ms() > 1'600'000'000'000);   // wall clock is still available for logs
}
