// ThreadPool: the only place that makes threads (MASTER SPEC 2, 14, 15, 87, 88).
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <set>
#include <stdexcept>
#include <thread>

#include "jx/core/ThreadPool.h"

using jx::core::ThreadPool;
using jx::core::ThreadPoolConfig;

TEST_CASE("the thread count comes from the machine and the configuration", "[core][pool]")
{
    CHECK(ThreadPool::hardware_threads() >= 1);
    ThreadPoolConfig cfg;
    cfg.threads = 0;   // auto
    CHECK(ThreadPool::resolve_threads(cfg) == ThreadPool::hardware_threads());
    cfg.threads = 4;
    CHECK(ThreadPool::resolve_threads(cfg) == 4);
    cfg.max_threads = 2;
    CHECK(ThreadPool::resolve_threads(cfg) == 2);
    cfg.threads = 0;
    cfg.max_threads = 0;
    cfg.min_threads = 3;
    CHECK(ThreadPool::resolve_threads(cfg) >= 3);
}

TEST_CASE("tasks run on the workers, never on the caller", "[core][pool]")
{
    ThreadPool pool(ThreadPoolConfig{"test", 4, 1, 0});
    CHECK(pool.size() == 4);
    CHECK(ThreadPool::this_worker_index() == -1);   // the test thread is not a worker

    std::atomic<int> done{0};
    std::mutex mutex;
    std::set<std::thread::id> threads;
    for (int i = 0; i < 200; ++i) {
        CHECK(pool.submit([&] {
            {
                std::lock_guard lock(mutex);
                threads.insert(std::this_thread::get_id());
            }
            CHECK(ThreadPool::this_worker_index() >= 0);
            done.fetch_add(1);
        }));
    }
    pool.wait_idle();
    CHECK(done.load() == 200);
    CHECK(threads.size() > 1);   // the work really was spread
    CHECK(pool.completed() == 200);
    CHECK(pool.failed() == 0);
}

TEST_CASE("a throwing task does not take the process down", "[core][pool]")
{
    ThreadPool pool(ThreadPoolConfig{"test-throw", 2, 1, 0});
    pool.submit([] { throw std::runtime_error("boom"); });
    pool.submit([] { throw 42; });
    std::atomic<bool> after{false};
    pool.submit([&] { after.store(true); });
    pool.wait_idle();
    CHECK(pool.failed() == 2);
    CHECK(after.load());   // the pool keeps working
}

TEST_CASE("stopping joins every thread and refuses new work", "[core][pool]")
{
    ThreadPool pool(ThreadPoolConfig{"test-stop", 2, 1, 0});
    std::atomic<int> ran{0};
    for (int i = 0; i < 50; ++i) pool.submit([&] { ran.fetch_add(1); });
    pool.stop();
    CHECK(pool.size() == 0);
    CHECK_FALSE(pool.submit([] {}));   // after stop the caller must run its own work
    pool.stop();                       // idempotent
    CHECK(ran.load() > 0);
}

TEST_CASE("busy time is measured per worker", "[core][pool][metrics]")
{
    ThreadPool pool(ThreadPoolConfig{"test-busy", 2, 1, 0});
    for (int i = 0; i < 4; ++i) {
        pool.submit([] { std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    }
    pool.wait_idle();
    const std::uint64_t total = pool.busy_ns(0) + pool.busy_ns(1);
    CHECK(total > 10'000'000ull);   // at least the 4 x 5 ms spread over two workers
    CHECK(pool.busy_ns(99) == 0);
}
