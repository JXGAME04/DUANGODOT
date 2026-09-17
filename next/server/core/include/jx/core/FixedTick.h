// jx::core::FixedTick - the fixed step game loop driver (MASTER SPEC 20, 22, 23).
//
// The loop never runs "as fast as it can": real elapsed time is fed in, and the driver decides
// how many whole ticks must be simulated.  A stall never turns into a spiral of death because
// the backlog beyond max_catch_up is dropped and counted.
//
// The phase list of SPEC 22 is declared here as an enum so every phase can be timed with the
// same names in the metrics; a phase costing more than its share shows up immediately.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "jx/clock.hpp"
#include "jx/core/Metrics.h"
#include "jx/core/ServerClock.h"

namespace jx::core {

// SPEC 22: the phases of one tick, in order.  Phases are timed, not scheduled, by this type:
// who runs what stays the decision of the world / job system (SPEC 17).
enum class TickPhase : int {
    drain_network = 0,
    validate_input,
    movement,
    spatial_update,
    ai,
    combat,
    missile,
    buff,
    commit_commands,
    entity_transfer,
    interest,
    snapshot,
    network_send,
    persistence,
    metrics,
    count_
};

const char* phase_name(TickPhase phase) noexcept;

// Drives a ServerClock from real time.  Not thread safe: one driver per loop.
class FixedTick {
public:
    // max_catch_up: how many ticks a single update() may run before the rest is dropped.
    explicit FixedTick(ServerClock& clock, std::uint32_t max_catch_up = 5) noexcept
        : clock_(clock), max_catch_up_(max_catch_up == 0 ? 1u : max_catch_up)
    {
    }

    // Feeds the time elapsed since the previous call and runs fn once per due tick.  Returns
    // how many ticks were run.  fn receives the tick number it is simulating.
    template <class Fn>
    std::uint32_t update(Nanos elapsed, Fn&& fn)
    {
        const std::uint32_t due = pending(elapsed);
        for (std::uint32_t i = 0; i < due; ++i) {
            clock_.advance();
            fn(clock_.tick());
        }
        return due;
    }

    // The same accounting without running anything (for a caller that ticks several worlds).
    std::uint32_t pending(Nanos elapsed) noexcept
    {
        if (elapsed.count() < 0) elapsed = Nanos{0};
        acc_ += elapsed;
        const std::int64_t step = clock_.rate().step().count();
        const auto total = static_cast<std::uint64_t>(acc_.count() / step);
        const auto run = static_cast<std::uint32_t>(total < max_catch_up_ ? total : max_catch_up_);
        dropped_ += total - run;
        acc_ -= Nanos{static_cast<std::int64_t>(total) * step};
        return run;
    }

    [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }
    [[nodiscard]] Nanos accumulator() const noexcept { return acc_; }
    [[nodiscard]] std::uint32_t max_catch_up() const noexcept { return max_catch_up_; }
    void reset() noexcept
    {
        acc_ = Nanos{0};
        dropped_ = 0;
    }

private:
    ServerClock& clock_;
    std::uint32_t max_catch_up_;
    Nanos acc_{0};
    std::uint64_t dropped_ = 0;
};

// Times the phases of one tick into a metrics registry (SPEC 51: "ngay từ đầu đo").
// Usage:
//     TickProfile profile(metrics, "zone");
//     { auto s = profile.phase(TickPhase::movement); ... }
class TickProfile {
public:
    TickProfile(Metrics& metrics, std::string_view scope);

    // Times one phase for as long as the returned object lives.
    [[nodiscard]] ScopedTiming phase(TickPhase p) { return ScopedTiming(*phases_[static_cast<std::size_t>(p)]); }
    // The whole tick.
    [[nodiscard]] ScopedTiming whole() { return ScopedTiming(*total_); }
    [[nodiscard]] Timing& total() noexcept { return *total_; }
    [[nodiscard]] Timing& phase_timing(TickPhase p) noexcept { return *phases_[static_cast<std::size_t>(p)]; }

private:
    Timing* total_ = nullptr;
    Timing* phases_[static_cast<std::size_t>(TickPhase::count_)]{};
};

} // namespace jx::core
