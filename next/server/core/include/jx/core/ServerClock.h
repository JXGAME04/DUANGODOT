// jx::core::ServerClock - the one source of time for gameplay (MASTER SPEC 59, 61).
//
// Gameplay never calls GetTickCount(), time(), rand()-style ad hoc timing: every cooldown,
// buff, missile, AI budget and timer is expressed in ServerTick, and every tick has a known
// duration (TickRate).  That is what makes a simulation reproducible from "input + tick +
// state" and replayable from a recording.
//
// Wall clock time (unix_ms) exists only for logs, database rows and protocol timestamps.
#pragma once

#include <cstdint>

#include "jx/clock.hpp"

namespace jx::core {

// SPEC 21: one monotonically growing tick number for the whole server.
using ServerTick = std::uint64_t;

// How long one tick lasts.  SPEC 20: configurable, never hard coded at the call site.
class TickRate {
public:
    explicit TickRate(std::uint32_t hz = 30) noexcept
        : hz_(hz == 0 ? 1u : hz), step_(Nanos{1'000'000'000LL / static_cast<std::int64_t>(hz == 0 ? 1u : hz)})
    {
    }

    [[nodiscard]] std::uint32_t hz() const noexcept { return hz_; }
    [[nodiscard]] Nanos step() const noexcept { return step_; }
    [[nodiscard]] double step_ms() const noexcept { return static_cast<double>(step_.count()) / 1e6; }

    // Durations <-> ticks.  Rounds up so "500 ms cooldown" never expires early.
    [[nodiscard]] ServerTick ticks_from_ms(std::int64_t ms) const noexcept
    {
        if (ms <= 0) return 0;
        const std::int64_t ns = ms * 1'000'000LL;
        return static_cast<ServerTick>((ns + step_.count() - 1) / step_.count());
    }
    [[nodiscard]] std::int64_t ms_from_ticks(ServerTick ticks) const noexcept
    {
        return static_cast<std::int64_t>(ticks) * step_.count() / 1'000'000LL;
    }

private:
    std::uint32_t hz_;
    Nanos step_;
};

// The server's clock: the current tick, the rate, and monotonic/wall time.  One instance per
// GameServer process (the simulation reads it, FixedTick advances it).  Tests pass a
// ManualClock so nothing depends on the machine being fast.
class ServerClock {
public:
    explicit ServerClock(TickRate rate = TickRate{}, const IClock* clock = nullptr) noexcept
        : rate_(rate), clock_(clock)
    {
    }

    [[nodiscard]] const TickRate& rate() const noexcept { return rate_; }
    [[nodiscard]] ServerTick tick() const noexcept { return tick_; }

    // FixedTick owns these two; nothing else may move the clock forward.
    void advance(ServerTick ticks = 1) noexcept { tick_ += ticks; }
    void reset(ServerTick tick = 0) noexcept { tick_ = tick; }

    // Monotonic time (never jumps when the machine's date changes).
    [[nodiscard]] Nanos now() const noexcept { return clock_ != nullptr ? clock_->now() : steady_now(); }
    // Wall clock, for logs / database / protocol only.
    [[nodiscard]] std::int64_t unix_ms() const noexcept { return unix_millis(); }

    // Time of a past or future tick relative to now, in ticks (no floating point in gameplay).
    [[nodiscard]] bool reached(ServerTick deadline) const noexcept { return tick_ >= deadline; }
    [[nodiscard]] ServerTick after_ms(std::int64_t ms) const noexcept { return tick_ + rate_.ticks_from_ms(ms); }

private:
    TickRate rate_;
    const IClock* clock_ = nullptr;   // null = the process steady clock
    ServerTick tick_ = 0;
};

} // namespace jx::core
