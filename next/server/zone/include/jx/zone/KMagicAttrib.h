#pragma once

// KMagicAttrib.h of the old core: one attribute of an item, a skill or a state - an id of
// MAGIC_ATTRIB (KMagicAttribId.h, generated from the JX2 binary's name table) and up to three
// parameters.  Applied to a npc by KNpcAttribModify (KNpcAttribModify.h).

#include <array>

#include "jx/zone/KMagicAttribId.h"

namespace jx::zone {

struct KMagicAttrib {
    int type = 0;
    std::array<int, 3> value{0, 0, 0};
    [[nodiscard]] bool empty() const noexcept { return type == 0; }
};

} // namespace jx::zone
