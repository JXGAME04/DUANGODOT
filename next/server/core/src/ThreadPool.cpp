#include "jx/core/ThreadPool.h"

#include <algorithm>
#include <exception>

#include "jx/clock.hpp"
#include "jx/log.hpp"

namespace jx::core {
namespace {

// Which pool worker is the calling thread?  -1 for the main / network / test thread.
thread_local int t_worker_index = -1;

} // namespace

unsigned ThreadPool::hardware_threads() noexcept
{
    const unsigned n = std::thread::hardware_concurrency();
    return n == 0 ? 1u : n;
}

unsigned ThreadPool::resolve_threads(const ThreadPoolConfig& cfg) noexcept
{
    unsigned n = cfg.threads != 0 ? cfg.threads : hardware_threads();
    if (cfg.max_threads != 0) n = std::min(n, cfg.max_threads);
    n = std::max(n, std::max(1u, cfg.min_threads));
    return n;
}

int ThreadPool::this_worker_index() noexcept { return t_worker_index; }

ThreadPool::ThreadPool(ThreadPoolConfig cfg) : cfg_(std::move(cfg))
{
    const unsigned n = resolve_threads(cfg_);
    busy_ns_ = std::vector<std::atomic<std::uint64_t>>(n);
    threads_.reserve(n);
    for (unsigned i = 0; i < n; ++i) {
        threads_.emplace_back([this, i] { run(i); });
    }
    log::info("core.pool", "thread pool started",
              {log::kv("name", cfg_.name), log::kv("threads", n), log::kv("hardware", hardware_threads())});
}

ThreadPool::~ThreadPool() { stop(); }

void ThreadPool::run(unsigned index)
{
    t_worker_index = static_cast<int>(index);
    for (;;) {
        Task task;
        {
            std::unique_lock lock(mutex_);
            work_cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (stopping_ && queue_.empty()) break;
            task = std::move(queue_.front());
            queue_.pop_front();
            ++running_tasks_;
        }
        const Nanos start = steady_now();
        try {
            task();
            completed_.fetch_add(1, std::memory_order_relaxed);
        } catch (const std::exception& e) {
            // SPEC 88: an exception must never cross the thread boundary and kill the process.
            failed_.fetch_add(1, std::memory_order_relaxed);
            log::error("core.pool", "task threw", {log::kv("pool", cfg_.name), log::kv("worker", index), log::kv("error", e.what())});
        } catch (...) {
            failed_.fetch_add(1, std::memory_order_relaxed);
            log::error("core.pool", "task threw", {log::kv("pool", cfg_.name), log::kv("worker", index), log::kv("error", "unknown")});
        }
        busy_ns_[index].fetch_add(static_cast<std::uint64_t>((steady_now() - start).count()), std::memory_order_relaxed);
        {
            std::lock_guard lock(mutex_);
            --running_tasks_;
            if (queue_.empty() && running_tasks_ == 0) idle_cv_.notify_all();
        }
    }
    t_worker_index = -1;
}

bool ThreadPool::submit(Task task)
{
    if (!task) return false;
    {
        std::lock_guard lock(mutex_);
        if (stopping_ || threads_.empty()) return false;
        queue_.push_back(std::move(task));
    }
    work_cv_.notify_one();
    return true;
}

void ThreadPool::wait_idle()
{
    std::unique_lock lock(mutex_);
    idle_cv_.wait(lock, [this] { return queue_.empty() && running_tasks_ == 0; });
}

void ThreadPool::stop()
{
    {
        std::lock_guard lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }
    work_cv_.notify_all();
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
    threads_.clear();
    log::info("core.pool", "thread pool stopped",
              {log::kv("name", cfg_.name), log::kv("completed", completed_.load()), log::kv("failed", failed_.load())});
}

std::size_t ThreadPool::queued() const
{
    std::lock_guard lock(mutex_);
    return queue_.size();
}

std::uint64_t ThreadPool::busy_ns(unsigned worker) const
{
    if (worker >= busy_ns_.size()) return 0;
    return busy_ns_[worker].load(std::memory_order_relaxed);
}

} // namespace jx::core
