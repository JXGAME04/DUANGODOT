// jx::entity::EntityHandle - a reference to an entity that cannot come back to life
// (MASTER SPEC 5, 36).
//
// The old server addressed npcs by a raw array index (`Npc[MAX_NPC]`).  When a slot was reused,
// every stale index silently pointed at a different creature: a missile still flying, a target
// id in a packet, a Lua variable.  The fix is a generation counter next to the index:
//
//     index 500, generation 12   ->  handle 500:12
//     the entity dies            ->  slot 500 now has generation 13
//     the old handle 500:12      ->  invalid, lookup returns nothing
//
// The two halves are packed into the 64 bit jx::EntityId that already travels on the wire, so
// nothing else in the protocol changes:
//
//     bits 63..32 = generation (starts at 1, never 0)
//     bits 31..0  = slot index
//
// A handle is therefore never 0 for a live entity, and EntityId{} keeps meaning "none".
#pragma once

#include <cstdint>
#include <string>

#include "jx/ids.hpp"

namespace jx::entity {

inline constexpr std::uint32_t kInvalidIndex = 0xFFFFFFFFu;

[[nodiscard]] constexpr std::uint32_t index_of(EntityId id) noexcept
{
    return static_cast<std::uint32_t>(id.value & 0xFFFFFFFFull);
}

[[nodiscard]] constexpr std::uint32_t generation_of(EntityId id) noexcept
{
    return static_cast<std::uint32_t>(id.value >> 32u);
}

[[nodiscard]] constexpr EntityId make_handle(std::uint32_t index, std::uint32_t generation) noexcept
{
    return EntityId{(static_cast<std::uint64_t>(generation) << 32u) | static_cast<std::uint64_t>(index)};
}

// A handle is well formed when it carries a generation; generation 0 is reserved for "none".
[[nodiscard]] constexpr bool is_handle(EntityId id) noexcept { return generation_of(id) != 0u; }

// The next generation of a slot: wraps around 32 bits without ever landing on 0.
[[nodiscard]] constexpr std::uint32_t next_generation(std::uint32_t generation) noexcept
{
    const std::uint32_t next = generation + 1u;
    return next == 0u ? 1u : next;
}

// "500:12" - how a handle appears in logs and error messages.
[[nodiscard]] std::string to_string(EntityId id);

} // namespace jx::entity
