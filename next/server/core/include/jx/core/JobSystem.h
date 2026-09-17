// jx::core::JobSystem - where computation runs (MASTER SPEC 17, 18, 23).
//
// The job system is not the ownership system.  Ownership says *who may mutate* an entity; the
// job system only says *where a computation runs*.  A job therefore reads its input, computes,
// and returns a result or a command buffer (SPEC 18) - it never writes into the world.
//
// Every call here is a barrier (SPEC 23): the caller waits until the whole batch is finished,
// so a later phase can never read half-updated state.  The calling thread works too, so a
// small batch does not pay for a wakeup.
#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "jx/core/ThreadPool.h"

namespace jx::core {

class JobSystem {
public:
    explicit JobSystem(ThreadPool& pool) noexcept : pool_(pool) {}

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Runs fn(i) for every i in [0, n) and waits.  grain = smallest number of items one worker
    // takes (0 = decide from n and the pool size).
    void parallel_for(std::size_t n, const std::function<void(std::size_t)>& fn, std::size_t grain = 0);

    // Runs independent jobs and waits for all of them.
    void run_all(std::vector<std::function<void()>> jobs);

    // Fire and forget (background work that nobody waits for this tick).
    void submit(std::function<void()> job) { pool_.submit(std::move(job)); }

    [[nodiscard]] ThreadPool& pool() noexcept { return pool_; }
    [[nodiscard]] unsigned workers() const noexcept { return pool_.size(); }

private:
    ThreadPool& pool_;
};

} // namespace jx::core
