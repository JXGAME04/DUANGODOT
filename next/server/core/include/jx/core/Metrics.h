// jx::core::Metrics - what the server measures about itself (MASTER SPEC 51, 52, 53, 96).
//
// Measured from the first day, not added after the first overload: total tick time, the cost of
// every tick phase, per worker utilisation, per map cost, queue depths.  Everything here is
// lock free on the hot path (atomics only) because simulation workers write to it every tick.
//
// Timings keep a logarithmic histogram, so average, max, P95 and P99 are available without
// storing every sample (SPEC 96 asks for exactly those).
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jx/clock.hpp"

namespace jx::core {

// A number that only grows (packets, commands, errors).
class Counter {
public:
    void add(std::uint64_t n = 1) noexcept { value_.fetch_add(n, std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t value() const noexcept { return value_.load(std::memory_order_relaxed); }
    void reset() noexcept { value_.store(0, std::memory_order_relaxed); }

private:
    std::atomic<std::uint64_t> value_{0};
};

// A number that goes up and down (players online, queue depth, entities).
class Gauge {
public:
    void set(std::int64_t v) noexcept { value_.store(v, std::memory_order_relaxed); }
    void add(std::int64_t delta) noexcept { value_.fetch_add(delta, std::memory_order_relaxed); }
    [[nodiscard]] std::int64_t value() const noexcept { return value_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::int64_t> value_{0};
};

// A distribution of durations.  add() is a handful of relaxed atomic operations.
class Timing {
public:
    static constexpr int kSubBuckets = 4;    // resolution: 4 buckets per octave (~19 % steps)
    static constexpr int kOctaves = 40;      // 1 ns .. ~18 minutes
    static constexpr int kBucketCount = kSubBuckets * kOctaves;

    struct Snapshot {
        std::uint64_t count = 0;
        double avg_ms = 0;
        double max_ms = 0;
        double p50_ms = 0;
        double p95_ms = 0;
        double p99_ms = 0;
        double total_ms = 0;
    };

    void add(Nanos d) noexcept;
    void add_ns(std::uint64_t ns) noexcept;

    [[nodiscard]] Snapshot snapshot() const noexcept;
    [[nodiscard]] std::uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
    void reset() noexcept;

    // Bucket boundaries, exposed for the tests.
    static int bucket_of(std::uint64_t ns) noexcept;
    static std::uint64_t bucket_upper_ns(int bucket) noexcept;

private:
    std::atomic<std::uint64_t> count_{0};
    std::atomic<std::uint64_t> sum_ns_{0};
    std::atomic<std::uint64_t> max_ns_{0};
    std::array<std::atomic<std::uint64_t>, kBucketCount> buckets_{};
};

// Times a scope into a Timing (the usual way to measure a tick phase).
class ScopedTiming {
public:
    explicit ScopedTiming(Timing& t) noexcept : timing_(&t), start_(steady_now()) {}
    ScopedTiming(ScopedTiming&& other) noexcept : timing_(other.timing_), start_(other.start_) { other.timing_ = nullptr; }
    ScopedTiming(const ScopedTiming&) = delete;
    ScopedTiming& operator=(const ScopedTiming&) = delete;
    ScopedTiming& operator=(ScopedTiming&&) = delete;
    ~ScopedTiming()
    {
        if (timing_ != nullptr) timing_->add(steady_now() - start_);
    }
    // Stops early and returns what was measured.
    Nanos stop() noexcept
    {
        const Nanos d = steady_now() - start_;
        if (timing_ != nullptr) {
            timing_->add(d);
            timing_ = nullptr;
        }
        return d;
    }

private:
    Timing* timing_;
    Nanos start_;
};

// A registry of named metrics.  Names are hierarchical like the log categories:
//   "tick.total", "tick.phase.ai", "map.3.tick", "worker.2.busy_ns", "net.rx.bytes".
// Lookups take a mutex, so a hot path resolves its metric once and keeps the reference.
class Metrics {
public:
    static Metrics& instance();

    Counter& counter(std::string_view name);
    Gauge& gauge(std::string_view name);
    Timing& timing(std::string_view name);

    struct Snapshot {
        std::vector<std::pair<std::string, std::uint64_t>> counters;
        std::vector<std::pair<std::string, std::int64_t>> gauges;
        std::vector<std::pair<std::string, Timing::Snapshot>> timings;
    };
    [[nodiscard]] Snapshot snapshot() const;
    // One JSON object with every metric: goes into the periodic stats log line and, later, into
    // the metrics endpoint.
    [[nodiscard]] std::string to_json() const;
    void reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<Counter>> counters_;
    std::unordered_map<std::string, std::unique_ptr<Gauge>> gauges_;
    std::unordered_map<std::string, std::unique_ptr<Timing>> timings_;
};

// Shorthand for the process registry.
inline Metrics& metrics() { return Metrics::instance(); }

} // namespace jx::core
