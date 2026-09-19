// KPlayerTask.h - the task values of a character: the numbers the scripts keep on it (KPlayerTask of the 2003
// Core/Src/KPlayerTask.h: nSave[] / nClear[], GetSaveVal / SetSaveVal / GetClearVal / SetClearVal; KPlayer::m_cTask)
// and the table that says which of them the client is told about (settings/task/player_task_def.txt).
//
// jx_linux_y keeps KPlayerTask at Player+0x809c: m_nTaskTemp[256] at +0 (GetTaskTemp 0x080CB5A0 / SetTaskTemp
// 0x080CB5C0: ids 0..0xff, never saved) and a std::map<int, int> of the saved values at +0x54c (ids 0..0x176f:
// GetSaveVal 0x080CB540 = 0 when absent, SetSaveVal 0x080CB720 erases the entry for a 0, GetBits 0x080CB5E0 /
// SetBits 0x080CB910 read and write `count` bits from `start` of a value, ClearRange 0x080CBC40, Serialize 0x080CB6A0
// writes the {int id, int value} pairs KPlayer::SavePlayerTaskList 0x080BF1C0 keeps in TRoleData at [+0x17f]..[+0x18b]
// and LoadPlayerTaskList 0x080C0050 reads back).  KPlayer::SetTaskValue 0x080A9190(player, id, value, bSync): nothing
// when unchanged, else SetSaveVal and, when the id has SYNC_FLAG in the table (0x978bf7c) and bSync, the 0xa7 packet
// {0xa7, int id, int value} (0x080A8CC0); SyncTaskValueMore 0x080A9550 sends the 0xb5 packet of eighty {id, value}.
// docs/LINUX-SERVER.md §21.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace jx::zone {

inline constexpr int kTaskValueCount = 0x1770;    // the saved ids 0..0x176f (`cmp 0x176f / ja` of every accessor)
inline constexpr int kTaskTempCount = 256;        // m_nTaskTemp (`cmp 0xff / ja`)
inline constexpr int kTaskSyncMoreBatch = 80;     // the 0xb5 packet: 1 + 80 x 8 = 0x281 bytes (0x080A9550; the 2.0 client reads 79 of them, 0x00651384)
inline constexpr int kTaskRepute = 100;   // TASKVALUE_REPUTE of the 2003 source: AddRepute 0x08117290 / GetRepute 0x08117230
inline constexpr int kTaskTraceId = 1;            // SetTask / SetBitTask on id 1 log "TraceTaskValue Set\t%d\t%d\t%s\t%s" (0x08116812 / 0x08109032)
// ids the engine keeps for itself (KTaskFuns.h of 2003; SavePlayerTaskList 0x080BF1C0 refills the first two)
inline constexpr int kTaskWayPointBegin = 201;    // TASKVALUE_SAVEWAYPOINT_BEGIN: m_PlayerWayPointList (+0x294), 3 ids
inline constexpr int kTaskWayPointCount = 3;
inline constexpr int kTaskStationBegin = 210;     // TASKVALUE_SAVESTATION_BEGIN: m_PlayerStationList (+0x2a0), 20 stations in 10 ids of two 16-bit halves
inline constexpr int kTaskStationCount = 10;
inline constexpr int kTaskDisabledTeam = 0x87;    // bit 0x400: Lua DisabledTeam / KPlayerTeam::CreateTeam (0x080A8C80)
inline constexpr int kTaskTeamFlag = 0xb41;       // bit 2 <-> Player+0x7cfc (0x080A9240; the 0xaa packet of the client, not ported)
inline constexpr int kTaskLoginSyncFirst = 1000;  // the enter-world stage 0x080B9CF0 ends with SyncTaskValueMore(1000, 1070, only non-zero)
inline constexpr int kTaskLoginSyncLast = 1070;

// one row of settings/task/player_task_def.txt (jxassets export-task-def -> task_def.json): TASK_ID_FIRST, TASK_ID_LAST
// (0 = the first alone), TASK_NAME, SYNC_FLAG, CLIENT_FLAG
struct KTaskDefRow {
    int first = 0;
    int last = 0;
    bool sync = false;     // the zone tells the client the value whenever a script changes it, and all of them at the login
    bool client = false;   // the client may set the value itself (the 0xaa packet, 0x080DB070)
    std::string name;
};

struct KTaskDefRange {
    int first = 0;
    int last = 0;
};

// the flags of one id (the byte at +0x14 of a node of the map at 0x978bf7c)
enum KTaskDefFlag : unsigned {
    task_def_sync = 1,
    task_def_client = 2,
};

// The two maps the loader 0x081C6E00 fills from the table (this = 0x978bf60): the ranges of the SYNC_FLAG rows keyed by
// their first id (insert-unique: the first row of a `first` stays; the map at +0x4), and the flags of every id of a row
// with either flag (the map at +0x1c: written for each id, so a later row overwrites an earlier one).  A row whose
// TASK_ID_FIRST is 0 is skipped (0x081C6EEC); TASK_ID_LAST 0 means the first id alone (0x081C6EF5).
class KTaskDefTable {
public:
    static std::optional<KTaskDefTable> load(const std::string& file, std::string* error);
    void add(const KTaskDefRow& row);
    [[nodiscard]] unsigned flags(int id) const noexcept;
    [[nodiscard]] bool synced(int id) const noexcept { return (flags(id) & task_def_sync) != 0; }
    [[nodiscard]] bool client_may_set(int id) const noexcept { return (flags(id) & task_def_client) != 0; }
    [[nodiscard]] const std::map<int, KTaskDefRange>& sync_ranges() const noexcept { return ranges_; }
    [[nodiscard]] std::size_t size() const noexcept { return flags_.size(); }   // the ids with a flag
    std::string source;

private:
    std::map<int, KTaskDefRange> ranges_;
    std::map<int, unsigned char> flags_;
};

struct KPlayerTask {
    std::array<int, kTaskTempCount> temp{};   // m_nTaskTemp +0
    std::map<int, int> saved;                 // +0x54c: never holds a 0

    [[nodiscard]] int get_save_val(int id) const noexcept;   // GetSaveVal 0x080CB540: 0 outside 0..0x176f or when absent
    void set_save_val(int id, int value);                    // SetSaveVal 0x080CB720: outside the range nothing; a 0 erases
    [[nodiscard]] int get_temp(int id) const noexcept;       // GetClearVal 0x080CB5A0: 0 outside 0..0xff
    void set_temp(int id, int value) noexcept;               // SetClearVal 0x080CB5C0
    // GetBits 0x080CB5E0: `count` bits from `start` of the value as an unsigned number (0 when the id is out of range,
    // count <= 0, start < 0, start + count > 32 or the value absent)
    [[nodiscard]] std::uint32_t get_bits(int id, int start, int count) const noexcept;
    // SetBits 0x080CB910: those bits replaced by the low bits of `value`, the others kept (a missing value starts from 0);
    // false when the arguments fail the same checks.  It writes through SetSaveVal: the client is not told
    bool set_bits(int id, int start, int count, int value);
    void clear_range(int first, int count);                  // ClearRange 0x080CBC40: the ids first..first+count-1 erased
    void release() noexcept;                                 // KPlayerTask::Release: everything 0
    [[nodiscard]] std::vector<std::pair<int, int>> serialize() const;   // Serialize 0x080CB6A0: the non-zero values in id order
    // 0x080CB66F..0x080CB68F: (0xffffffff << start) & (0xffffffff >> (32 - count - start))
    [[nodiscard]] static std::uint32_t bit_mask(int start, int count) noexcept;
};

}   // namespace jx::zone
