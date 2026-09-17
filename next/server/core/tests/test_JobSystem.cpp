// JobSystem: where computation runs, with a barrier at the end (MASTER SPEC 17, 18, 23).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "jx/core/JobSystem.h"

using jx::core::JobSystem;
using jx::core::ThreadPool;
using jx::core::ThreadPoolConfig;

TEST_CASE("parallel_for visits every index exactly once", "[core][job]")
{
    ThreadPool pool(ThreadPoolConfig{"jobs", 4, 1, 0});
    JobSystem jobs(pool);

    constexpr std::size_t kN = 10000;
    std::vector<int> seen(kN, 0);
    jobs.parallel_for(kN, [&](std::size_t i) { seen[i] += 1; });
    CHECK(std::accumulate(seen.begin(), seen.end(), 0) == static_cast<int>(kN));
    CHECK(*std::max_element(seen.begin(), seen.end()) == 1);

    // when the batch returns, every job is finished: that is the barrier of SPEC 23
    std::atomic<int> running{0};
    std::atomic<int> max_running{0};
    jobs.parallel_for(64, [&](std::size_t) {
        const int now = running.fetch_add(1) + 1;
        int prev = max_running.load();
        while (now > prev && !max_running.compare_exchange_weak(prev, now)) {
        }
        running.fetch_sub(1);
    }, 1);
    CHECK(running.load() == 0);
    CHECK(max_running.load() >= 1);
}

TEST_CASE("a small batch does not need the pool", "[core][job]")
{
    ThreadPool pool(ThreadPoolConfig{"jobs-small", 4, 1, 0});
    JobSystem jobs(pool);
    std::vector<std::size_t> order;
    jobs.parallel_for(3, [&](std::size_t i) { order.push_back(i); }, 8);   // one chunk: runs inline
    CHECK(order == std::vector<std::size_t>{0, 1, 2});
    CHECK(pool.completed() == 0);
}

TEST_CASE("run_all waits for every job", "[core][job]")
{
    ThreadPool pool(ThreadPoolConfig{"jobs-all", 3, 1, 0});
    JobSystem jobs(pool);
    std::atomic<int> done{0};
    std::vector<std::function<void()>> batch;
    for (int i = 0; i < 12; ++i) batch.emplace_back([&] { done.fetch_add(1); });
    jobs.run_all(std::move(batch));
    CHECK(done.load() == 12);
}

TEST_CASE("a throwing job does not break the barrier", "[core][job]")
{
    ThreadPool pool(ThreadPoolConfig{"jobs-throw", 2, 1, 0});
    JobSystem jobs(pool);
    std::atomic<int> done{0};
    jobs.parallel_for(8, [&](std::size_t i) {
        if (i % 2 == 0) throw std::runtime_error("job failed");
        done.fetch_add(1);
    }, 1);
    CHECK(done.load() == 4);   // the batch still returned: the caller is not stuck
}

TEST_CASE("the job system works without a running pool", "[core][job]")
{
    ThreadPool pool(ThreadPoolConfig{"jobs-stopped", 2, 1, 0});
    JobSystem jobs(pool);
    pool.stop();
    std::atomic<int> done{0};
    jobs.parallel_for(100, [&](std::size_t) { done.fetch_add(1); }, 1);
    CHECK(done.load() == 100);   // the caller runs the work itself instead of deadlocking
}
