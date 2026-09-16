// jx clock - monotonic time, injectable clocks for tests and the fixed-step accumulator used by
// the zone loop ("fix your timestep": feed real elapsed time, run exactly N ticks, so the
// simulation is deterministic regardless of frame rate and can be replayed from a recording).
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace jx {

using Nanos = std::chrono::nanoseconds;

inline Nanos steady_now() noexcept
{
    return std::chrono::duration_cast<Nanos>(std::chrono::steady_clock::now().time_since_epoch());
}

inline std::int64_t unix_millis() noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

class IClock {
public:
    virtual ~IClock() = default;
    [[nodiscard]] virtual Nanos now() const noexcept = 0;
};

class SteadyClock final : public IClock {
public:
    [[nodiscard]] Nanos now() const noexcept override { return steady_now(); }
};

// Test clock: time only moves when advance()/set() is called.
class ManualClock final : public IClock {
public:
    explicit ManualClock(Nanos start = Nanos{0}) noexcept : now_(start) {}
    [[nodiscard]] Nanos now() const noexcept override { return now_; }
    void advance(Nanos delta) noexcept { now_ += delta; }
    void set(Nanos time) noexcept { now_ = time; }
private:
    Nanos now_;
};

class FixedStep {
public:
    explicit FixedStep(std::uint32_t hertz, std::uint32_t max_ticks_per_update = 5) noexcept
        : hz_(hertz == 0 ? 1u : hertz),
          max_ticks_(max_ticks_per_update == 0 ? 1u : max_ticks_per_update),
          step_(Nanos{1'000'000'000LL / static_cast<std::int64_t>(hz_)})
    {
    }

    [[nodiscard]] Nanos step() const noexcept { return step_; }
    [[nodiscard]] std::uint32_t hz() const noexcept { return hz_; }

    // Feed the wall time elapsed since the previous update.  Returns how many ticks the caller must
    // simulate now (0..max_ticks).  Any backlog beyond max_ticks is dropped (and counted) so a stall
    // never turns into a spiral of death.
    std::uint32_t update(Nanos elapsed) noexcept
    {
        if (elapsed.count() < 0) elapsed = Nanos{0};
        acc_ += elapsed;
        const auto total = static_cast<std::uint64_t>(acc_.count() / step_.count());
        const auto run = static_cast<std::uint32_t>(std::min<std::uint64_t>(total, max_ticks_));
        dropped_ += total - run;
        acc_ -= Nanos{static_cast<std::int64_t>(total) * step_.count()};
        tick_ += run;
        return run;
    }

    // Interpolation factor in [0,1) for rendering between two ticks.
    [[nodiscard]] double alpha() const noexcept
    {
        return static_cast<double>(acc_.count()) / static_cast<double>(step_.count());
    }
    [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }
    [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }
    [[nodiscard]] Nanos accumulator() const noexcept { return acc_; }

    void reset() noexcept
    {
        acc_ = Nanos{0};
        tick_ = 0;
        dropped_ = 0;
    }

private:
    std::uint32_t hz_;
    std::uint32_t max_ticks_;
    Nanos step_;
    Nanos acc_{0};
    std::uint64_t tick_ = 0;
    std::uint64_t dropped_ = 0;
};

} // namespace jx
