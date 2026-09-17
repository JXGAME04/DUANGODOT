#include "jx/core/FixedTick.h"

#include <fmt/format.h>
#include <cstddef>

namespace jx::core {

const char* phase_name(TickPhase phase) noexcept
{
    switch (phase) {
    case TickPhase::drain_network: return "drain_network";
    case TickPhase::validate_input: return "validate_input";
    case TickPhase::movement: return "movement";
    case TickPhase::spatial_update: return "spatial_update";
    case TickPhase::ai: return "ai";
    case TickPhase::combat: return "combat";
    case TickPhase::missile: return "missile";
    case TickPhase::buff: return "buff";
    case TickPhase::commit_commands: return "commit";
    case TickPhase::entity_transfer: return "entity_transfer";
    case TickPhase::interest: return "interest";
    case TickPhase::snapshot: return "snapshot";
    case TickPhase::network_send: return "network_send";
    case TickPhase::persistence: return "persistence";
    case TickPhase::metrics: return "metrics";
    case TickPhase::count_: break;
    }
    return "unknown";
}

TickProfile::TickProfile(Metrics& m, std::string_view scope)
{
    total_ = &m.timing(fmt::format("{}.tick.total", scope));
    for (int i = 0; i < static_cast<int>(TickPhase::count_); ++i) {
        phases_[static_cast<std::size_t>(i)] =
            &m.timing(fmt::format("{}.tick.{}", scope, phase_name(static_cast<TickPhase>(i))));
    }
}

} // namespace jx::core
