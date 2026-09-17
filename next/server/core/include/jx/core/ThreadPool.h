// jx::core::ThreadPool - the only place that creates threads (MASTER SPEC 2, 14, 15, 16, 87).
//
// There is no thread per player, per NPC or per map: a fixed, named, configurable pool of
// workers takes work items.  The count comes from the configuration, defaulting to what the
// machine reports (SPEC 15); nothing is pinned to a core until a profiler says it helps
// (SPEC 16).  Shutdown joins every thread (SPEC 87), and an exception never escapes a worker
// (SPEC 88).
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace jx::core {

struct ThreadPoolConfig {
    std::string name = "worker";   // thread name prefix, used in logs and metrics
    unsigned threads = 0;          // 0 = hardware_concurrency (SPEC 15)
    unsigned min_threads = 1;
    unsigned max_threads = 0;      // 0 = no cap
};

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(ThreadPoolConfig cfg);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Any thread.  The task runs on one of the workers; exceptions are caught and logged.
    // Returns false when the pool is stopping - the caller must then run the work itself.
    bool submit(Task task);
    // Waits until the queue is empty and no worker is running a task (tests and shutdown).
    void wait_idle();
    // Stops accepting work, wakes the workers, joins them.  Idempotent.
    void stop();

    [[nodiscard]] unsigned size() const noexcept { return static_cast<unsigned>(threads_.size()); }
    [[nodiscard]] const std::string& name() const noexcept { return cfg_.name; }
    [[nodiscard]] std::size_t queued() const;
    [[nodiscard]] std::uint64_t completed() const noexcept { return completed_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t failed() const noexcept { return failed_.load(std::memory_order_relaxed); }
    // Busy nanoseconds of one worker since start: utilisation per worker (SPEC 52).
    [[nodiscard]] std::uint64_t busy_ns(unsigned worker) const;

    // How many threads the machine reports (SPEC 15); never 0.
    static unsigned hardware_threads() noexcept;
    // Resolves cfg.threads / min / max into the number of threads to start.
    static unsigned resolve_threads(const ThreadPoolConfig& cfg) noexcept;
    // The index of the calling thread inside its pool, or -1 when it is not a pool thread.
    static int this_worker_index() noexcept;

private:
    void run(unsigned index);

    ThreadPoolConfig cfg_;
    mutable std::mutex mutex_;
    std::condition_variable work_cv_;
    std::condition_variable idle_cv_;
    std::deque<Task> queue_;
    std::vector<std::thread> threads_;
    std::vector<std::atomic<std::uint64_t>> busy_ns_;
    std::atomic<std::uint64_t> completed_{0};
    std::atomic<std::uint64_t> failed_{0};
    unsigned running_tasks_ = 0;
    bool stopping_ = false;
};

} // namespace jx::core
