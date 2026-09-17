// CommandQueue: how a non-owner thread asks the owner to change state (MASTER SPEC 19, 30, 73).
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "jx/core/CommandQueue.h"
#include "jx/core/EventQueue.h"

using jx::core::CommandQueue;
using jx::core::EventQueue;

namespace {

struct DamageCommand {
    std::uint64_t source = 0;
    std::uint64_t target = 0;
    std::int32_t amount = 0;
};

} // namespace

TEST_CASE("commands are applied by the owner in push order", "[core][queue]")
{
    CommandQueue<DamageCommand> q;
    CHECK(q.empty());
    q.push(DamageCommand{1, 2, 10});
    q.push(DamageCommand{1, 2, 20});
    CHECK(q.size() == 2);

    std::int32_t total = 0;
    std::vector<std::int32_t> order;
    const std::size_t n = q.drain([&](DamageCommand c) {
        total += c.amount;
        order.push_back(c.amount);
    });
    CHECK(n == 2);
    CHECK(total == 30);
    CHECK(order == std::vector<std::int32_t>{10, 20});
    CHECK(q.empty());
    CHECK(q.pushed() == 2);
}

TEST_CASE("a command pushed while draining runs in the next batch", "[core][queue]")
{
    CommandQueue<int> q;
    q.push(1);
    int seen = 0;
    q.drain([&](int v) {
        seen += v;
        if (v == 1) q.push(2);   // a command that queues a command must not spin the loop
    });
    CHECK(seen == 1);
    CHECK(q.size() == 1);
    q.drain([&](int v) { seen += v; });
    CHECK(seen == 3);
}

TEST_CASE("many producers, one owner", "[core][queue][thread]")
{
    CommandQueue<int> q;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 2000;
    std::atomic<bool> go{false};
    std::vector<std::thread> producers;
    producers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&, t] {
            while (!go.load()) {
            }
            for (int i = 0; i < kPerThread; ++i) q.push(t);
        });
    }
    go.store(true);

    // the owner drains while the producers are still pushing
    std::vector<int> per_thread(kThreads, 0);
    int drained = 0;
    while (drained < kThreads * kPerThread) {
        drained += static_cast<int>(q.drain([&](int t) { ++per_thread[static_cast<std::size_t>(t)]; }));
    }
    for (auto& p : producers) p.join();
    CHECK(drained == kThreads * kPerThread);
    for (int t = 0; t < kThreads; ++t) CHECK(per_thread[static_cast<std::size_t>(t)] == kPerThread);
}

TEST_CASE("a bounded queue refuses instead of growing for ever", "[core][queue]")
{
    CommandQueue<int> q;
    CHECK(q.push(1, 2));
    CHECK(q.push(2, 2));
    CHECK_FALSE(q.push(3, 2));   // full: backpressure, not unbounded memory (SPEC 70)
    CHECK(q.dropped() == 1);
    CHECK(q.size() == 2);
    const auto items = q.take();
    CHECK(items == std::vector<int>{1, 2});
    CHECK(q.empty());
}

TEST_CASE("events leave the owner the same way", "[core][queue]")
{
    EventQueue<std::string> events;
    events.emplace("player entered");
    events.emplace("player left");
    const auto taken = events.take();
    REQUIRE(taken.size() == 2);
    CHECK(taken[0] == "player entered");
    CHECK(events.empty());
}
