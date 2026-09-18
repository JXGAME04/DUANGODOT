#pragma once

// KNpcAttribModify of the old core: the one place a KMagicAttrib (of an item, a skill, a state)
// changes a npc.  The JX2 server keeps a table ProcessFunc[id] (jx_linux_y: the constructor
// 0x08099600 fills 217 entries, KNpcAttribModify::ModifyAttrib 0x08095B40 dispatches, KNpc::
// ModifyAttrib 0x0807D210 is the entry); every entry was read (tools/re/re_attribmod.py, the
// listing in docs/linux/jx_linux_attribmod.tsv) and is reproduced here with the same arithmetic
// - the `_p` percents multiply the BASE value (m_LifeMax, not the current one), a pair keeps
// the sum in both cells, sorbdamage is held at 0..500, cold damage sets its freeze time to
// min(54, 4 x (v / 5) + 10), and so on (docs/LINUX-SERVER.md §10).
//
// Removing an attribute (taking a piece off) is done by the caller through the same table with
// the values negated (KItem::ApplyMagicAttribToNPC / 0x08068440); `removing` marks that pass
// for the few entries that act differently (ignorenegativestate_p clears the stun and freeze
// only when adding).

#include "jx/zone/KMagicAttrib.h"

#include <cstdint>
#include <functional>

namespace jx::zone {

struct KNpc;
class KItemList;
class KPlayerSet;

struct KSkillListHost;

struct KNpcAttribModifyContext {
    const KPlayerSet* tables = nullptr;   // the level tables (strength_v .. energy_v of a player)
    const KItemList* items = nullptr;     // the player's items (SetNpcPhysicsDamage after a strength change)
    bool removing = false;                // the values are the negated ones of a piece taken off
    std::uint64_t tick = 0;               // the frame (autocastskill starts its wait from it, 0x08188A10)
    KSkillListHost* skill_host = nullptr; // what KSkillList needs of the world (allskill_v); null = the change is not applied
    const std::function<void(int)>* set_hide = nullptr;   // KNpc::SetHide 0x0807FF80 (the world tells the players around); null = the field only
};

class KNpcAttribModify {
public:
    // What one entry does to the npc.  Returns false when the id has no entry (the binary's
    // table has a null there) or the entry is one the zone does not carry yet (skills, features,
    // scripts) - the caller logs it once.
    static bool modify(KNpc& npc, const KMagicAttrib& m, const KNpcAttribModifyContext& ctx);
    // true when the id has an entry that touches the npc's numbers (the ones modify() applies)
    [[nodiscard]] static bool handled(int id) noexcept;
};

} // namespace jx::zone
