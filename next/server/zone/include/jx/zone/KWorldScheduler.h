// KWorldScheduler - which simulation worker ticks which map instance (MASTER SPEC 8, 9).
//
// A map is never nailed to a thread.  The scheduler places every instance on a worker, watches
// what each instance actually costs per tick, and moves instances off a worker that is running
// hot.  Because an instance only ever changes owner between ticks (never during one), the
// "one mutable entity, one owner" rule holds while the assignment changes.
#pragma once

#include <cstdint>
#include <vector>

namespace jx::zone {

class KMapInstance;

struct KWorkerLoad {
    unsigned worker = 0;
    double cost_ms = 0;        // sum of the average tick cost of the instances it owns
    std::size_t instances = 0;
    std::size_t players = 0;
};

class KWorldScheduler {
public:
    // workers: how many simulation workers exist (>= 1).
    explicit KWorldScheduler(unsigned workers) noexcept : workers_(workers == 0 ? 1u : workers) {}

    [[nodiscard]] unsigned workers() const noexcept { return workers_; }

    // First placement: the least loaded worker takes the new instance.
    void assign(std::vector<KMapInstance*>& instances, KMapInstance& fresh) const;
    // Places every instance (used at boot).
    void assign_all(std::vector<KMapInstance*>& instances) const;

    // Looks at the measured cost and moves instances from the hottest worker to the coldest
    // one while that makes the difference smaller.  Returns how many instances moved.
    // hysteresis: only act when the hottest worker costs this much more than the coldest.
    std::size_t rebalance(std::vector<KMapInstance*>& instances, double hysteresis = 1.25) const;

    // What each worker currently carries (metrics, logs, tests).
    [[nodiscard]] std::vector<KWorkerLoad> loads(const std::vector<KMapInstance*>& instances) const;

private:
    unsigned workers_;
};

} // namespace jx::zone
