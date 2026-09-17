// KMapInstance - one running copy of a map, with exactly one owner (MASTER SPEC 6, 7, 37, 38).
//
// A map id is not an instance: the same map can run several times (dungeon copies, Tống Kim
// channels, private maps), so the instance carries its own id.  At any moment exactly one
// simulation worker ticks it, and only that worker touches its entities.  Everyone else uses
// the inbox; whatever the world needs from the outside leaves through the event queue.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "jx/core/CommandQueue.h"
#include "jx/core/EventQueue.h"
#include "jx/core/Metrics.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KWorldCommand.h"

namespace jx::zone {

class KMapInstance {
public:
    KMapInstance(std::uint32_t instance_id, KSubWorldConfig cfg);

    [[nodiscard]] std::uint32_t instance_id() const noexcept { return instance_id_; }
    [[nodiscard]] std::uint32_t map_id() const noexcept { return world_.map_id(); }
    [[nodiscard]] const std::string& name() const noexcept { return world_.config().name; }

    // The world is only safe to touch from the owner worker (or before the server starts).
    [[nodiscard]] KSubWorld& world() noexcept { return world_; }
    [[nodiscard]] const KSubWorld& world() const noexcept { return world_; }

    // Any thread: ask the owner to do something.
    void post(KWorldCommand cmd) { inbox_.push(std::move(cmd)); }
    [[nodiscard]] std::size_t pending_commands() const { return inbox_.size(); }

    // Owner worker only: drain the inbox, simulate one tick, collect what the world produced.
    void tick();

    // Server thread, after the parallel phase: collect what to send / persist.
    std::vector<Packet> take_outbox() { return world_.take_outbox(); }
    std::vector<KWorldEvent> take_events() { return events_.take(); }

    // ---- ownership (SPEC 7, 8: assigned by the scheduler, never hard coded) ----
    [[nodiscard]] unsigned owner() const noexcept { return owner_; }
    void set_owner(unsigned worker) noexcept { owner_ = worker; }

    // ---- workload, for the scheduler and the metrics (SPEC 9, 53) ----
    [[nodiscard]] double last_tick_ms() const noexcept { return last_tick_ms_; }
    // Exponential moving average: what the scheduler balances on.
    [[nodiscard]] double avg_tick_ms() const noexcept { return avg_tick_ms_; }
    [[nodiscard]] std::size_t players() const noexcept { return world_.player_count(); }
    [[nodiscard]] std::size_t entities() const noexcept { return world_.entity_count(); }
    [[nodiscard]] std::uint64_t ticks() const noexcept { return world_.tick_count(); }
    [[nodiscard]] std::uint64_t commands_applied() const noexcept { return commands_; }
    // Records what a tick cost (tick() calls it; a load test or a unit test can feed a number).
    void note_tick_cost(double ms) noexcept
    {
        last_tick_ms_ = ms;
        avg_tick_ms_ = avg_tick_ms_ == 0.0 ? ms : avg_tick_ms_ * 0.9 + ms * 0.1;
    }

private:
    void apply(KWorldCommand& cmd);
    void apply_client_packet(const KCmdClientPacket& cmd);
    void emit_save(std::uint64_t sid, bool final);

    std::uint32_t instance_id_;
    KSubWorld world_;
    core::CommandQueue<KWorldCommand> inbox_;
    core::EventQueue<KWorldEvent> events_;
    unsigned owner_ = 0;
    double last_tick_ms_ = 0;
    double avg_tick_ms_ = 0;
    std::uint64_t commands_ = 0;
    core::Timing* tick_timing_ = nullptr;   // "map.<id>.tick" in the process metrics
    std::string drain_metric_;              // "map.<id>.tick.drain_network"
};

} // namespace jx::zone
