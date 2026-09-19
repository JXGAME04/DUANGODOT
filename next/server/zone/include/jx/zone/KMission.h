// KMission - a mission of a map: the players in it by group, its timers and the script that drives it (KMission.h /
// KMissionArray.h of the 2003 source - the class jx_linux_y still runs: one mission is 0x73a0 bytes at SubWorld+0x60 +
// i * 0x73a0, the 100 values, the id at +0x190 (0x08138135), the player entries (TMissionPlayerInfo, 24 bytes) from +0x308
// with their used-index list at +0x7394 (0x08135550), the three timer entries (KTimerTaskFun) from +0x1a4 with their list
// at +0x1ec (0x082096B0)).  \settings\task\missions.txt names the script of a mission id (KTabFile 0x9780b00: row id + 1,
// column 2 - 0x0813313A), \settings\timertask.txt the script whose OnTimer a timer id runs (KTimerTaskFun::m_TimerTaskTab);
// jxassets export-missions writes both to missions.json.  Every call into a mission script has no player behind it
// (KMission::ExecuteScript 2003 sets the SubWorld global only; 0x082095A0 of jx_linux_y): InitMission (OpenMission
// 0x0813377E), RunMission (0x08133150), EndMission (CloseMission 0x08132B0A), OnLeave(player) (RemovePlayer),
// JoinMission(player, group) (0x08138266), OnTimer (the timer's script, 0x08209610).  docs/LINUX-SERVER.md §33.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jx/ids.hpp"

namespace jx::zone {

struct KMissionTable {
    std::unordered_map<int, std::string> missions;   // mission id -> the script's game path
    std::unordered_map<int, std::string> timers;     // timer id -> the script's game path (its OnTimer)
    static std::optional<KMissionTable> load(const std::string& file, std::string* error);
    [[nodiscard]] const std::string& mission_script(int id) const;
    [[nodiscard]] const std::string& timer_script(int id) const;
};

// TMissionPlayerInfo of 2003: one entry of a mission's player list (the data index the scripts hold is 1-based)
struct KMissionPlayer {
    EntityId player;               // m_ulPlayerIndex (the entity id here)
    std::uint64_t player_id = 0;   // m_ulPlayerID
    int group = 0;                 // m_ucPlayerGroup
    std::uint64_t join_tick = 0;   // m_ulJoinTime (g_SubWorldSet.GetGameTime())
    int param1 = 0;                // SetPMParam / GetPMParam 1
    int param2 = 0;                // 2
    bool used = false;
};

// KTimerTaskFun of a mission: fires every `interval` frames - KTimerTaskFun::Activate 0x080F9480 re-arms fire = now +
// interval before it runs the script (the 2003 code says so too); SetTimer(0) closes the entry (fire 0)
struct KMissionTimer {
    int id = 0;
    std::uint64_t fire_tick = 0;
    std::uint64_t interval = 0;
};

class KMission {
public:
    static constexpr int kMaxTimers = 3;   // MAX_TIMER_PERMISSION: the three entries of +0x1a4..+0x1ec

    int id = 0;
    std::string script;

    // the player list - KMission::AddPlayer 2003: the lowest free slot (KLinkArrayTemplate::FindFree), the join time now;
    // returns the data index (1-based), 0 when nothing could be added
    int add_player(EntityId player, std::uint64_t player_id, int group, std::uint64_t now);
    bool remove_player(EntityId player);                                     // RemovePlayer: the slot freed
    [[nodiscard]] int data_index(EntityId player) const;                     // GetMissionPlayer_DataIndex: 0 = not in
    [[nodiscard]] const KMissionPlayer* entry(int data_index) const;        // GetMissionPlayer_PlayerIndex / _GroupId
    [[nodiscard]] KMissionPlayer* entry_mutable(int data_index);
    // GetNextPlayer 2003: the next used entry above `idx` (of `group` when group != 0) -> {data index, player}; {0, none}
    [[nodiscard]] std::pair<int, EntityId> next_player(int idx, int group) const;
    [[nodiscard]] int player_count(int group) const;   // GetGroupPlayerCount: group 0 = everyone
    [[nodiscard]] std::vector<EntityId> players(int group) const;   // Msg2All (group 0) / Msg2Group

    // the timers: StartMissionTimer adds an entry (a fourth one is refused, the array has three), StopMissionTimer closes
    // and removes the first with that id, GetTimerRestTimer reads the first with that id
    bool start_timer(int timer_id, std::uint64_t interval, std::uint64_t now);
    void stop_timer(int timer_id);
    [[nodiscard]] std::uint64_t rest_time(int timer_id, std::uint64_t now) const;
    [[nodiscard]] std::size_t timer_count() const noexcept { return timers_.size(); }
    // the ids of the timers due now, each re-armed first (KTimerTaskFun::Activate: fire = now + interval)
    std::vector<int> fire_timers(std::uint64_t now);
    void clear();   // StopMission: the players and timers gone

private:
    std::vector<KMissionPlayer> entries_;   // [0] unused: the scripts' data indices start at 1
    std::vector<KMissionTimer> timers_;
};

} // namespace jx::zone
