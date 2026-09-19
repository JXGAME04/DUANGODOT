#pragma once
// KPlayerTeam / KTeam / KTeamSet of the old game (Core/Src/KPlayerTeam.h, 2002; jx_linux_y: KPlayerTeam at Player+0x5994,
// g_Team at 0x8BB86E0 with 0x30 bytes a team, g_TeamSet 0x8bc67e0).  A team is a captain and up to seven members; the
// captain's leadership level (Player+0x5970, level_lead_exp.txt) says how many may join; an open team takes applicants,
// a closed one does not.  docs/LINUX-SERVER.md §17.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace jx::zone {

constexpr int kTeamMembers = 7;   // MAX_TEAM_MEMBER of the Linux build (the member slots +0xc..+0x24; 2002 had 5)

// what a player keeps of its team (KPlayerTeam; the Linux offsets from Player+0x5994)
struct KPlayerTeam {
    bool flag = false;                 // +0 m_nFlag: in a team
    int id = -1;                       // +4 m_nID: the team (index of KTeamSet)
    int figure = 0;                    // +8 m_nFigure: 0 TEAM_CAPTAIN, 1 TEAM_MEMBER
    std::uint64_t apply_captain = 0;   // +0xc m_nApplyCaptainID: the captain (npc id) one applied to
    std::array<std::uint64_t, kTeamMembers> invite_list{};   // +0x10 m_nInviteList: the players (sids) the captain invited
    int list_pos = 0;                  // +0x2c m_nListPos
    std::uint64_t captain_npc = 0;     // (zone) the captain's npc id - g_Team[id].captain's npc, what 0x0809BC70 keys the damage records by
    bool auto_captain = false;         // +0x30: the lead came from the system (0x080CD4FE) - no taking (0x86 msg 0x24), kicking (0x25),
                                       // appointing (0x26), inviting (0x27) or opening (0x28) until the player creates a team again
    bool can_team = true;              // +0x34 m_bCanTeamFlag: SetCanTeamFlag - may be invited / accepted at all
    bool lua_disabled = false;         // the task value 0x87 bit 0x400 (Lua DisabledTeam / IsDisabledTeam): no creating (error 4), not invitable

    // KPlayerTeam::Release 0x080CC5D0: everything but can_team back to nothing
    void release() noexcept
    {
        flag = false;
        id = -1;
        figure = 0;
        apply_captain = 0;
        invite_list.fill(0);
        list_pos = 0;
        captain_npc = 0;
        auto_captain = false;
    }
    [[nodiscard]] bool captain() const noexcept { return flag && figure == 0 && id >= 0; }
    [[nodiscard]] bool invited(std::uint64_t sid) const noexcept
    {
        for (const std::uint64_t s : invite_list) if (s != 0 && s == sid) return true;
        return false;
    }
};

// one team (KTeam; g_Team[i] of the Linux build: +4 m_nState, +8 m_nCaptain, +0xc m_nMember[7], +0x28 m_nMemNum, +0x2c the
// leadership limit flag).  Players are their session ids here (the binary's player indices).
struct KTeam {
    int state = 0;                     // +4: 1 Team_S_Open (takes applicants), 0 Team_S_Close
    std::uint64_t captain = 0;         // +8: 0 = a free slot in the set
    std::array<std::uint64_t, kTeamMembers> members{};   // +0xc..: 0 = empty
    int count = 0;                     // +0x28 m_nMemNum: the members without the captain
    bool lead_limit = true;            // +0x2c: 0 = CheckFull never full (Lua ChangeTeamFeature); 1 at creation (0x080CC2FC)

    // KTeam::Release 0x080CC3C0 (part)
    void release() noexcept
    {
        state = 0;
        captain = 0;
        members.fill(0);
        count = 0;
        lead_limit = true;
    }
    [[nodiscard]] bool empty() const noexcept { return captain == 0; }
    // KTeam::FindFree 0x080CCA46..: the first empty member slot, -1 when none
    [[nodiscard]] int find_free() const noexcept
    {
        for (int i = 0; i < kTeamMembers; ++i) if (members[static_cast<std::size_t>(i)] == 0) return i;
        return -1;
    }
    // KTeam::FindMemberID 0x080CBF00: the slot of a member, -1 when not a member (the captain is not a slot)
    [[nodiscard]] int find_member(std::uint64_t sid) const noexcept
    {
        if (sid == 0) return -1;
        for (int i = 0; i < kTeamMembers; ++i) if (members[static_cast<std::size_t>(i)] == sid) return i;
        return -1;
    }
    // KTeam::CheckIn 0x080CC320 (part): captain or member
    [[nodiscard]] bool check_in(std::uint64_t sid) const noexcept { return sid != 0 && (captain == sid || find_member(sid) >= 0); }
    // the people of the team: the captain first, then the members that are there
    [[nodiscard]] std::vector<std::uint64_t> people() const
    {
        std::vector<std::uint64_t> out;
        if (captain != 0) out.push_back(captain);
        for (const std::uint64_t m : members) if (m != 0) out.push_back(m);
        return out;
    }
};

// the teams of a world (KTeamSet; g_Team has 1200 rows)
class KTeamSet {
public:
    KTeamSet() { teams_.reserve(kMaxTeams); }   // the rows never move: a KTeam* stays good while the set lives
    // KTeamSet::CreateTeam 0x080CC290: the first free row becomes the captain's team, closed, no members, the leadership
    // limit on; returns its index (-2 when every row is taken)
    int create(std::uint64_t captain);
    [[nodiscard]] KTeam* get(int id) noexcept;
    [[nodiscard]] const KTeam* get(int id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return teams_.size(); }

private:
    static constexpr std::size_t kMaxTeams = 1200;   // 0x4b0 (0x080CC276)
    std::vector<KTeam> teams_;
};

}  // namespace jx::zone
