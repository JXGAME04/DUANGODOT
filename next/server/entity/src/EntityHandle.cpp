#include "jx/entity/EntityHandle.h"

#include <fmt/format.h>

namespace jx::entity {

std::string to_string(EntityId id)
{
    if (id.value == 0) return "none";
    return fmt::format("{}:{}", index_of(id), generation_of(id));
}

} // namespace jx::entity
