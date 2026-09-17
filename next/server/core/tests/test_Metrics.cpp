// Metrics: measured from the first day (MASTER SPEC 51, 52, 53, 96).
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <thread>
#include <vector>

#include "jx/core/Metrics.h"

using jx::Nanos;
using jx::core::Counter;
using jx::core::Gauge;
using jx::core::Metrics;
using jx::core::ScopedTiming;
using jx::core::Timing;

TEST_CASE("counters and gauges", "[core][metrics]")
{
    Metrics m;
    auto& packets = m.counter("net.rx.packets");
    packets.add();
    packets.add(9);
    CHECK(packets.value() == 10);
    CHECK(&m.counter("net.rx.packets") == &packets);   // the same name is the same metric

    auto& players = m.gauge("world.players");
    players.set(120);
    players.add(-20);
    CHECK(players.value() == 100);

    const std::string json = m.to_json();
    CHECK(json.find("\"net.rx.packets\":10") != std::string::npos);
    CHECK(json.find("\"world.players\":100") != std::string::npos);
}

TEST_CASE("a timing reports average, max and percentiles", "[core][metrics]")
{
    Timing t;
    // 99 samples of 1 ms and one of 100 ms: the average stays low, P99 finds the outlier
    for (int i = 0; i < 99; ++i) t.add(Nanos{1'000'000});
    t.add(Nanos{100'000'000});

    const auto s = t.snapshot();
    CHECK(s.count == 100);
    CHECK(s.avg_ms > 1.9);
    CHECK(s.avg_ms < 2.1);
    CHECK(s.max_ms > 99.0);
    CHECK(s.max_ms < 101.0);
    CHECK(s.p50_ms > 0.9);
    CHECK(s.p50_ms < 1.3);      // histogram buckets over-estimate by at most ~19 %
    CHECK(s.p95_ms < 1.3);
    CHECK(s.p99_ms < 1.3);      // the 100 ms sample is the 100th: above P99
    CHECK(s.total_ms > 198.0);

    Timing t2;
    for (int i = 0; i < 50; ++i) t2.add(Nanos{1'000'000});
    for (int i = 0; i < 50; ++i) t2.add(Nanos{50'000'000});
    const auto s2 = t2.snapshot();
    CHECK(s2.p50_ms < 1.3);
    CHECK(s2.p95_ms > 40.0);    // half the samples are slow: P95 must see them
    CHECK(s2.p99_ms > 40.0);

    t2.reset();
    CHECK(t2.snapshot().count == 0);
}

TEST_CASE("timing buckets cover the whole range", "[core][metrics]")
{
    CHECK(Timing::bucket_of(0) == 0);
    CHECK(Timing::bucket_of(1) == 0);
    CHECK(Timing::bucket_of(1'000'000) < Timing::kBucketCount);
    // monotone: a longer duration never lands in an earlier bucket
    int previous = -1;
    for (std::uint64_t ns = 1; ns < 1'000'000'000ull; ns = ns * 3 / 2 + 1) {
        const int b = Timing::bucket_of(ns);
        CHECK(b >= previous);
        CHECK(Timing::bucket_upper_ns(b) >= ns);
        previous = b;
    }
}

TEST_CASE("scoped timing measures a block", "[core][metrics]")
{
    Metrics m;
    auto& t = m.timing("tick.total");
    {
        ScopedTiming s(t);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto snap = t.snapshot();
    CHECK(snap.count == 1);
    CHECK(snap.max_ms >= 1.0);

    ScopedTiming early(t);
    const Nanos measured = early.stop();
    CHECK(measured.count() >= 0);
    CHECK(t.snapshot().count == 2);   // stop() counts once, the destructor does not count again
}

TEST_CASE("metrics survive concurrent writers", "[core][metrics][thread]")
{
    Metrics m;
    auto& c = m.counter("hits");
    auto& t = m.timing("work");
    std::vector<std::thread> threads;
    threads.reserve(8);
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            for (int k = 0; k < 5000; ++k) {
                c.add();
                t.add(Nanos{1000});
            }
        });
    }
    for (auto& th : threads) th.join();
    CHECK(c.value() == 40000);
    CHECK(t.snapshot().count == 40000);
}
