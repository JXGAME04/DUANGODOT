#include "jx/zone/KPlayerTeam.h"

namespace jx::zone {

int KTeamSet::create(std::uint64_t captain)
{
    if (captain == 0) return -1;
    for (std::size_t i = 0; i < teams_.size(); ++i) {
        if (teams_[i].empty()) {
            teams_[i].release();
            teams_[i].captain = captain;
            return static_cast<int>(i);
        }
    }
    if (teams_.size() >= kMaxTeams) return -2;   // 0x080CC310
    KTeam t;
    t.captain = captain;
    teams_.push_back(t);
    return static_cast<int>(teams_.size() - 1);
}

KTeam* KTeamSet::get(int id) noexcept
{
    if (id < 0 || static_cast<std::size_t>(id) >= teams_.size()) return nullptr;
    return &teams_[static_cast<std::size_t>(id)];
}

const KTeam* KTeamSet::get(int id) const noexcept
{
    if (id < 0 || static_cast<std::size_t>(id) >= teams_.size()) return nullptr;
    return &teams_[static_cast<std::size_t>(id)];
}

}  // namespace jx::zone
