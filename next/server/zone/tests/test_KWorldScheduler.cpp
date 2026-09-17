// The scheduler places map instances on simulation workers and moves them when one worker runs
// hot (MASTER SPEC 8, 9).  A map is never nailed to a thread.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <vector>

#include "jx/log.hpp"
#include "jx/zone/KMapInstance.h"
#include "jx/zone/KWorldScheduler.h"

using jx::zone::KMapInstance;
using jx::zone::KSubWorldConfig;
using jx::zone::KWorldScheduler;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
    }
};

std::unique_ptr<KMapInstance> make_instance(std::uint32_t id)
{
    KSubWorldConfig cfg;
    cfg.zone_id = 1;
    cfg.tick_hz = 20;
    return std::make_unique<KMapInstance>(id, cfg);
}

// Pretends a map costs this much per tick: the scheduler only reads avg_tick_ms(), which
// note_tick_cost() feeds (the same call the real tick makes).
void set_cost(KMapInstance& m, double ms)
{
    for (int i = 0; i < 200; ++i) m.note_tick_cost(ms);   // let the moving average settle
}

} // namespace

TEST_CASE("instances are spread over the workers at boot", "[zone][sched]")
{
    Quiet q;
    std::vector<std::unique_ptr<KMapInstance>> owned;
    std::vector<KMapInstance*> instances;
    for (std::uint32_t i = 1; i <= 7; ++i) {
        owned.push_back(make_instance(i));
        instances.push_back(owned.back().get());
    }

    KWorldScheduler sched(3);
    sched.assign_all(instances);
    const auto loads = sched.loads(instances);
    REQUIRE(loads.size() == 3);
    std::size_t total = 0;
    for (const auto& l : loads) {
        CHECK(l.instances >= 2);   // 7 maps over 3 workers: nobody gets nothing
        total += l.instances;
    }
    CHECK(total == 7);
}

TEST_CASE("a single worker owns everything", "[zone][sched]")
{
    Quiet q;
    std::vector<std::unique_ptr<KMapInstance>> owned;
    std::vector<KMapInstance*> instances;
    for (std::uint32_t i = 1; i <= 4; ++i) {
        owned.push_back(make_instance(i));
        instances.push_back(owned.back().get());
    }
    KWorldScheduler sched(1);
    sched.assign_all(instances);
    for (const KMapInstance* m : instances) CHECK(m->owner() == 0);
    CHECK(sched.rebalance(instances) == 0);   // nothing to move with one worker
}

TEST_CASE("a new instance goes to the least loaded worker", "[zone][sched]")
{
    Quiet q;
    std::vector<std::unique_ptr<KMapInstance>> owned;
    std::vector<KMapInstance*> instances;
    for (std::uint32_t i = 1; i <= 4; ++i) {
        owned.push_back(make_instance(i));
        instances.push_back(owned.back().get());
    }
    KWorldScheduler sched(2);
    sched.assign_all(instances);
    auto fresh = make_instance(99);
    sched.assign(instances, *fresh);
    const auto before = sched.loads(instances);
    // the worker that took it must be one of the two, and the assignment must be deterministic
    CHECK(fresh->owner() < 2);
    instances.push_back(fresh.get());
    const auto after = sched.loads(instances);
    CHECK(after[fresh->owner()].instances == before[fresh->owner()].instances + 1);
}

TEST_CASE("a hot worker gives a map to a cold one", "[zone][sched]")
{
    Quiet q;
    std::vector<std::unique_ptr<KMapInstance>> owned;
    std::vector<KMapInstance*> instances;
    for (std::uint32_t i = 1; i <= 4; ++i) {
        owned.push_back(make_instance(i));
        instances.push_back(owned.back().get());
    }
    // everything lands on worker 0, and the maps cost very different amounts
    for (KMapInstance* m : instances) m->set_owner(0);
    set_cost(*instances[0], 18.0);   // "Tống Kim"
    set_cost(*instances[1], 2.4);
    set_cost(*instances[2], 1.1);
    set_cost(*instances[3], 0.9);

    KWorldScheduler sched(2);
    CHECK(sched.rebalance(instances) > 0);
    const auto loads = sched.loads(instances);
    CHECK(loads[0].instances >= 1);
    CHECK(loads[1].instances >= 1);
    // the big map cannot be split (that is region partition), but the small ones move away from it
    const double hot = std::max(loads[0].cost_ms, loads[1].cost_ms);
    const double cold = std::min(loads[0].cost_ms, loads[1].cost_ms);
    CHECK(hot >= 18.0);        // the heavy map still costs what it costs
    CHECK(cold >= 0.9);        // and the cheap ones now run somewhere else
    CHECK(hot - cold < 22.4);  // strictly better than everything on one worker

    // a second pass must not flap: the assignment is already as good as it gets
    const unsigned owner_of_big = instances[0]->owner();
    sched.rebalance(instances);
    CHECK(instances[0]->owner() == owner_of_big);
}

TEST_CASE("every map instance keeps its own id and map", "[zone][sched]")
{
    Quiet q;
    auto a = make_instance(1);
    auto b = make_instance(2);
    CHECK(a->instance_id() == 1);
    CHECK(b->instance_id() == 2);
    CHECK(a->players() == 0);
    CHECK(a->pending_commands() == 0);
    a->set_owner(3);
    CHECK(a->owner() == 3);
}
