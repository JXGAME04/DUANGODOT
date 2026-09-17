#include "jx/core/Metrics.h"

#include <bit>
#include <cmath>

#include <fmt/format.h>

namespace jx::core {

int Timing::bucket_of(std::uint64_t ns) noexcept
{
    if (ns == 0) return 0;
    const int msb = 63 - std::countl_zero(ns);   // 0 for 1 ns
    int index = 0;
    if (msb < 2) {
        index = msb * kSubBuckets;
    } else {
        const auto sub = static_cast<int>((ns >> (msb - 2)) & 0x3u);
        index = msb * kSubBuckets + sub;
    }
    if (index >= kBucketCount) index = kBucketCount - 1;
    return index;
}

std::uint64_t Timing::bucket_upper_ns(int bucket) noexcept
{
    if (bucket < 0) return 0;
    if (bucket >= kBucketCount) bucket = kBucketCount - 1;
    const int msb = bucket / kSubBuckets;
    const int sub = bucket % kSubBuckets;
    if (msb < 2) return static_cast<std::uint64_t>(1) << msb;
    // values in this bucket are [ (4+sub) << (msb-2), (5+sub) << (msb-2) )
    return static_cast<std::uint64_t>(5 + sub) << (msb - 2);
}

void Timing::add_ns(std::uint64_t ns) noexcept
{
    count_.fetch_add(1, std::memory_order_relaxed);
    sum_ns_.fetch_add(ns, std::memory_order_relaxed);
    std::uint64_t prev = max_ns_.load(std::memory_order_relaxed);
    while (ns > prev && !max_ns_.compare_exchange_weak(prev, ns, std::memory_order_relaxed)) {
    }
    buckets_[static_cast<std::size_t>(bucket_of(ns))].fetch_add(1, std::memory_order_relaxed);
}

void Timing::add(Nanos d) noexcept
{
    add_ns(d.count() < 0 ? 0 : static_cast<std::uint64_t>(d.count()));
}

namespace {

double ns_to_ms(double ns) noexcept { return ns / 1e6; }

} // namespace

Timing::Snapshot Timing::snapshot() const noexcept
{
    Snapshot s;
    s.count = count_.load(std::memory_order_relaxed);
    const auto sum = sum_ns_.load(std::memory_order_relaxed);
    s.total_ms = ns_to_ms(static_cast<double>(sum));
    s.max_ms = ns_to_ms(static_cast<double>(max_ns_.load(std::memory_order_relaxed)));
    if (s.count == 0) return s;
    s.avg_ms = ns_to_ms(static_cast<double>(sum) / static_cast<double>(s.count));

    const auto want = [&](double p) {
        const auto target = static_cast<std::uint64_t>(std::ceil(p * static_cast<double>(s.count)));
        std::uint64_t seen = 0;
        for (int i = 0; i < kBucketCount; ++i) {
            seen += buckets_[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
            if (seen >= target) return ns_to_ms(static_cast<double>(bucket_upper_ns(i)));
        }
        return s.max_ms;
    };
    s.p50_ms = want(0.50);
    s.p95_ms = want(0.95);
    s.p99_ms = want(0.99);
    // a histogram bucket can only over-estimate; never report more than the real maximum
    if (s.p50_ms > s.max_ms) s.p50_ms = s.max_ms;
    if (s.p95_ms > s.max_ms) s.p95_ms = s.max_ms;
    if (s.p99_ms > s.max_ms) s.p99_ms = s.max_ms;
    return s;
}

void Timing::reset() noexcept
{
    count_.store(0, std::memory_order_relaxed);
    sum_ns_.store(0, std::memory_order_relaxed);
    max_ns_.store(0, std::memory_order_relaxed);
    for (auto& b : buckets_) b.store(0, std::memory_order_relaxed);
}

Metrics& Metrics::instance()
{
    static Metrics m;
    return m;
}

Counter& Metrics::counter(std::string_view name)
{
    std::lock_guard lock(mutex_);
    auto& slot = counters_[std::string(name)];
    if (!slot) slot = std::make_unique<Counter>();
    return *slot;
}

Gauge& Metrics::gauge(std::string_view name)
{
    std::lock_guard lock(mutex_);
    auto& slot = gauges_[std::string(name)];
    if (!slot) slot = std::make_unique<Gauge>();
    return *slot;
}

Timing& Metrics::timing(std::string_view name)
{
    std::lock_guard lock(mutex_);
    auto& slot = timings_[std::string(name)];
    if (!slot) slot = std::make_unique<Timing>();
    return *slot;
}

Metrics::Snapshot Metrics::snapshot() const
{
    Snapshot out;
    std::lock_guard lock(mutex_);
    out.counters.reserve(counters_.size());
    for (const auto& [name, c] : counters_) out.counters.emplace_back(name, c->value());
    out.gauges.reserve(gauges_.size());
    for (const auto& [name, g] : gauges_) out.gauges.emplace_back(name, g->value());
    out.timings.reserve(timings_.size());
    for (const auto& [name, t] : timings_) out.timings.emplace_back(name, t->snapshot());
    const auto by_name = [](const auto& a, const auto& b) { return a.first < b.first; };
    std::sort(out.counters.begin(), out.counters.end(), by_name);
    std::sort(out.gauges.begin(), out.gauges.end(), by_name);
    std::sort(out.timings.begin(), out.timings.end(), by_name);
    return out;
}

std::string Metrics::to_json() const
{
    const Snapshot s = snapshot();
    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf), "{{");
    bool first = true;
    const auto comma = [&] {
        if (!first) fmt::format_to(std::back_inserter(buf), ",");
        first = false;
    };
    for (const auto& [name, v] : s.counters) {
        comma();
        fmt::format_to(std::back_inserter(buf), "\"{}\":{}", name, v);
    }
    for (const auto& [name, v] : s.gauges) {
        comma();
        fmt::format_to(std::back_inserter(buf), "\"{}\":{}", name, v);
    }
    for (const auto& [name, t] : s.timings) {
        comma();
        fmt::format_to(std::back_inserter(buf),
                       "\"{}\":{{\"n\":{},\"avg_ms\":{:.3f},\"p95_ms\":{:.3f},\"p99_ms\":{:.3f},\"max_ms\":{:.3f}}}",
                       name, t.count, t.avg_ms, t.p95_ms, t.p99_ms, t.max_ms);
    }
    fmt::format_to(std::back_inserter(buf), "}}");
    return fmt::to_string(buf);
}

void Metrics::reset()
{
    std::lock_guard lock(mutex_);
    for (auto& [name, c] : counters_) c->reset();
    for (auto& [name, g] : gauges_) g->set(0);
    for (auto& [name, t] : timings_) t->reset();
}

} // namespace jx::core
