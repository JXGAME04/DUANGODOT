#include "jx/zone/KWorldScheduler.h"

#include <algorithm>

#include "jx/log.hpp"
#include "jx/zone/KMapInstance.h"

namespace jx::zone {
namespace {

// The cost the scheduler balances on: the measured tick average, with a floor so an idle map
// still counts for something (it still pays the fixed cost of being ticked).
double cost_of(const KMapInstance& m) noexcept
{
    const double measured = m.avg_tick_ms();
    return measured > 0.0 ? measured : 0.05;
}

} // namespace

std::vector<KWorkerLoad> KWorldScheduler::loads(const std::vector<KMapInstance*>& instances) const
{
    std::vector<KWorkerLoad> out(workers_);
    for (unsigned w = 0; w < workers_; ++w) out[w].worker = w;
    for (const KMapInstance* m : instances) {
        if (m == nullptr) continue;
        const unsigned w = m->owner() < workers_ ? m->owner() : 0u;
        out[w].cost_ms += cost_of(*m);
        out[w].instances += 1;
        out[w].players += m->players();
    }
    return out;
}

void KWorldScheduler::assign(std::vector<KMapInstance*>& instances, KMapInstance& fresh) const
{
    const auto load = loads(instances);
    const auto coldest = std::min_element(load.begin(), load.end(),
                                          [](const KWorkerLoad& a, const KWorkerLoad& b) {
                                              if (a.cost_ms != b.cost_ms) return a.cost_ms < b.cost_ms;
                                              return a.instances < b.instances;
                                          });
    fresh.set_owner(coldest == load.end() ? 0u : coldest->worker);
}

void KWorldScheduler::assign_all(std::vector<KMapInstance*>& instances) const
{
    // heaviest first, least loaded worker each time: a simple, stable packing
    std::vector<KMapInstance*> order(instances.begin(), instances.end());
    std::sort(order.begin(), order.end(), [](const KMapInstance* a, const KMapInstance* b) {
        return cost_of(*a) > cost_of(*b);
    });
    std::vector<double> cost(workers_, 0.0);
    std::vector<std::size_t> count(workers_, 0);
    for (KMapInstance* m : order) {
        unsigned best = 0;
        for (unsigned w = 1; w < workers_; ++w) {
            if (cost[w] < cost[best] || (cost[w] == cost[best] && count[w] < count[best])) best = w;
        }
        m->set_owner(best);
        cost[best] += cost_of(*m);
        ++count[best];
    }
}

std::size_t KWorldScheduler::rebalance(std::vector<KMapInstance*>& instances, double hysteresis) const
{
    if (workers_ < 2 || instances.size() < 2) return 0;
    std::size_t moved = 0;
    for (int pass = 0; pass < 4; ++pass) {
        auto load = loads(instances);
        const auto hottest = std::max_element(load.begin(), load.end(),
                                              [](const KWorkerLoad& a, const KWorkerLoad& b) { return a.cost_ms < b.cost_ms; });
        const auto coldest = std::min_element(load.begin(), load.end(),
                                              [](const KWorkerLoad& a, const KWorkerLoad& b) { return a.cost_ms < b.cost_ms; });
        if (hottest == load.end() || coldest == load.end() || hottest->worker == coldest->worker) break;
        // only act when the difference is real (SPEC 8: rebalance, do not flap)
        if (hottest->cost_ms < coldest->cost_ms * hysteresis + 0.1) break;
        if (hottest->instances < 2) break;   // a single instance cannot be split here (that is SPEC 11)

        // move the instance that brings the two closest together: the largest one that still
        // leaves the hot worker above the cold one
        KMapInstance* pick = nullptr;
        double best_gap = hottest->cost_ms - coldest->cost_ms;
        for (KMapInstance* m : instances) {
            if (m->owner() != hottest->worker) continue;
            const double c = cost_of(*m);
            const double gap = std::abs((hottest->cost_ms - c) - (coldest->cost_ms + c));
            if (gap < best_gap) {
                best_gap = gap;
                pick = m;
            }
        }
        if (pick == nullptr) break;
        log::info("zone.sched", "map moved to another worker",
                  {log::kv("map", pick->map_id()), log::kv("instance", pick->instance_id()),
                   log::kv("from", hottest->worker), log::kv("to", coldest->worker),
                   log::kv("cost_ms", cost_of(*pick)), log::kv("hot_ms", hottest->cost_ms), log::kv("cold_ms", coldest->cost_ms)});
        pick->set_owner(coldest->worker);
        ++moved;
    }
    return moved;
}

} // namespace jx::zone
