#include "jx/core/JobSystem.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>

#include "jx/log.hpp"

namespace jx::core {
namespace {

// A counter the caller waits on.  (std::latch would do, but this also lets the calling thread
// pull work while it waits.)
class Latch {
public:
    explicit Latch(std::size_t n) : remaining_(n) {}

    void done()
    {
        std::lock_guard lock(mutex_);
        if (remaining_ > 0 && --remaining_ == 0) cv_.notify_all();
    }
    void wait()
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return remaining_ == 0; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t remaining_;
};

void run_guarded(const std::function<void()>& fn, const char* what)
{
    try {
        fn();
    } catch (const std::exception& e) {
        log::error("core.job", "job threw", {log::kv("job", what), log::kv("error", e.what())});
    } catch (...) {
        log::error("core.job", "job threw", {log::kv("job", what), log::kv("error", "unknown")});
    }
}

} // namespace

void JobSystem::parallel_for(std::size_t n, const std::function<void(std::size_t)>& fn, std::size_t grain)
{
    if (n == 0 || !fn) return;
    const std::size_t workers = std::max<std::size_t>(1, pool_.size());
    if (grain == 0) {
        // aim for a few chunks per worker so a slow chunk cannot stall the whole batch
        grain = std::max<std::size_t>(1, n / (workers * 4));
    }
    const std::size_t chunks = (n + grain - 1) / grain;
    if (chunks <= 1 || workers == 1) {
        for (std::size_t i = 0; i < n; ++i) fn(i);
        return;
    }

    // The calling thread takes the last chunk itself, the rest go to the pool.
    Latch latch(chunks - 1);
    for (std::size_t c = 0; c + 1 < chunks; ++c) {
        const std::size_t begin = c * grain;
        const std::size_t end = std::min(begin + grain, n);
        const bool queued = pool_.submit([&fn, &latch, begin, end] {
            run_guarded([&] {
                for (std::size_t i = begin; i < end; ++i) fn(i);
            }, "parallel_for");
            latch.done();
        });
        if (!queued) {   // the pool is shutting down: the caller does the work itself
            run_guarded([&] {
                for (std::size_t i = begin; i < end; ++i) fn(i);
            }, "parallel_for.inline");
            latch.done();
        }
    }
    const std::size_t begin = (chunks - 1) * grain;
    run_guarded([&] {
        for (std::size_t i = begin; i < n; ++i) fn(i);
    }, "parallel_for.self");
    latch.wait();
}

void JobSystem::run_all(std::vector<std::function<void()>> jobs)
{
    if (jobs.empty()) return;
    if (jobs.size() == 1 || pool_.size() <= 1) {
        for (auto& j : jobs) run_guarded(j, "job");
        return;
    }
    Latch latch(jobs.size() - 1);
    for (std::size_t i = 0; i + 1 < jobs.size(); ++i) {
        auto job = std::move(jobs[i]);
        const bool queued = pool_.submit([job, &latch] {
            run_guarded(job, "job");
            latch.done();
        });
        if (!queued) {
            run_guarded(job, "job.inline");
            latch.done();
        }
    }
    run_guarded(jobs.back(), "job.self");
    latch.wait();
}

} // namespace jx::core
