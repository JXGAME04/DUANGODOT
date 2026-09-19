// KTaskManager.h - the task system the scripts drive through the TASKSYS library of the old server (IncludeLib("TASKSYS");
// task_function.lua / task_main.lua): the tables of settings/task (task_id.txt, task_type.txt with the condition /
// entity / award / talk table of each kind, task_event.txt), the status of a task (two bits in the task values from 2000
// on) and the temp values of a task (a structure serialized into the task values 2200..2299).
//
// jx_linux_y: the manager at 0x9786620 (Init 0x081725F0: three tables 0x08170990 from the pairs at 0x82e6140 -
// task_event.txt, task_id.txt, task_type.txt, key column 1 - then 0x08171390 for task_id.txt and 0x08172030 for the
// kinds); 0x08170060 name by id, 0x08170440 / 0x081702F0 id by name, 0x08172820 ordinal by name (the row from 0),
// 0x08170830 event by name, 0x081701A0 / 0x08170200 the tasks of an event; the matrix of a task in a table 0x08175BE0
// (rows x columns without the key column, both from 1 in Lua).  The status of a task: 0x0820E800 / 0x0820E720 - the
// value 0x7d0 + ordinal / 16, bits 31 - 2 (ordinal % 16) (worth 2) and 30 - 2 (ordinal % 16) (worth 1).  The temp
// values: 0x0820DF90 reads value 0x898 = the number of groups, then from 0x899 [task id, n, (key, value) x n] per
// group; 0x0820E250 writes it back in key order and zeroes the rest to 0x8fb; StartTask 0x0820E4E0 (a new group when
// the structure has room: 2 per group + 2 per pair <= 0x62), CloseTask 0x0820E430, SetTmpValue 0x0820E5C0 (the key is
// the hash 0x0821DF00 of the type name), GetTmpValue 0x0820DF10; FirstTask / NextTask 0x0820DCD0 walk the groups.
// docs/LINUX-SERVER.md §22.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jx/zone/KPlayerTask.h"

namespace jx::zone {

// the rows of one key of a table: rows x cols cells (the key column left out), both from 1 like the scripts count
struct KTaskMatrix {
    std::vector<std::vector<std::string>> rows;
    int cols = 0;
    [[nodiscard]] int row_count() const noexcept { return static_cast<int>(rows.size()); }
    // 0x08175BE0: nullptr outside 0 <= row < rows, 0 <= col < cols (the Lua glue takes 1 off first)
    [[nodiscard]] const std::string* cell(int row, int col) const noexcept;
};

// a table of the loader 0x08170990: the rows grouped by their key in the order they came
class KTaskTable {
public:
    void add(std::string key, std::vector<std::string> cells);
    [[nodiscard]] const KTaskMatrix* find(std::string_view key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return groups_.size(); }

private:
    std::map<std::string, KTaskMatrix, std::less<>> groups_;
};

// one row of task_type.txt with its four tables (task_type.txt: TaskType, ConditionFile, EntityFile, AwardFile, TalkFile)
struct KTaskType {
    std::string name;
    KTaskTable condition;
    KTaskTable entity;
    KTaskTable award;
    KTaskTable talk;
};

// one row of task_id.txt (TaskID, TaskName, EventID, TaskType, CanCancel, TaskText) and its ordinal
struct KTaskRecord {
    int id = 0;
    std::string name;
    int event = 0;
    std::string type;
    int ordinal = 0;   // the row from 0 (the description row of the file is row 0)
};

class KTaskManager {
public:
    static std::optional<KTaskManager> load(const std::string& file, std::string* error);

    // the ordinal is the position: the description row of task_id.txt counts too (the loader starts at row 2)
    void add_task(int id, std::string name, int event, std::string type, std::vector<std::string> cells);
    void add_event_row(std::string key, std::vector<std::string> cells);
    void add_type(KTaskType type);

    [[nodiscard]] const KTaskRecord* by_name(std::string_view name) const noexcept;   // 0x081702F0
    [[nodiscard]] const KTaskRecord* by_id(int id) const noexcept;                    // 0x08170060 / 0x08170100
    [[nodiscard]] const char* name_of(int id) const noexcept;
    [[nodiscard]] std::optional<int> id_of(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<int> ordinal_of(std::string_view name) const noexcept;   // 0x08172820
    [[nodiscard]] std::optional<int> event_of(std::string_view name) const noexcept;     // 0x08170830
    [[nodiscard]] int event_task_count(int event) const noexcept;                       // 0x081701A0
    [[nodiscard]] const std::string* event_task(int event, int index) const noexcept;   // 0x08170200 (index from 0)
    [[nodiscard]] const KTaskMatrix* id_matrix(std::string_view key) const noexcept;      // TaskId: the task_id.txt rows of an id (as text)
    [[nodiscard]] const KTaskMatrix* event_matrix(std::string_view key) const noexcept;   // TaskEvent: the task_event.txt rows of an id
    [[nodiscard]] const KTaskType* type_of(std::string_view name) const noexcept;         // 0x081726C0 + 0x08170630
    [[nodiscard]] const KTaskMatrix* condition(std::string_view name) const noexcept;     // 0x08172990
    [[nodiscard]] const KTaskMatrix* entity(std::string_view name) const noexcept;        // 0x08172E40
    [[nodiscard]] const KTaskMatrix* award(std::string_view name) const noexcept;         // 0x08172CB0
    [[nodiscard]] const KTaskMatrix* talk(std::string_view name) const noexcept;          // 0x08172B20
    [[nodiscard]] std::size_t task_count() const noexcept { return tasks_.size(); }
    [[nodiscard]] std::size_t type_count() const noexcept { return types_.size(); }
    [[nodiscard]] std::size_t event_count() const noexcept { return event_table_.size(); }
    std::string source;

private:
    std::vector<KTaskRecord> tasks_;
    std::map<std::string, std::size_t, std::less<>> by_name_;
    std::map<int, std::size_t> by_id_;
    std::map<int, std::vector<std::string>> by_event_;
    KTaskTable id_table_;
    KTaskTable event_table_;
    std::map<std::string, KTaskType, std::less<>> types_;
};

// the status bits and the temp values kept in KPlayerTask
namespace task_status {

inline constexpr int kStatusValueFirst = 0x7d0;   // 2000: sixteen tasks a value, two bits each (0x0820E855)
inline constexpr int kTempCountId = 0x898;        // 2200: how many groups follow (0x0820DF9C)
inline constexpr int kTempFirst = 0x899;          // 2201: the first slot of the groups
inline constexpr int kTempLast = 0x8fb;           // 2299: the last slot (0x0820E00C, 0x0820E378)
inline constexpr int kTempSlots = 0x62;           // 98: what StartTask allows the structure to hold (0x0820E538)

[[nodiscard]] int value_id(int ordinal) noexcept;              // 0x7d0 + ordinal / 16
[[nodiscard]] std::uint32_t hi_bit(int ordinal) noexcept;      // 1 << (31 - 2 (ordinal % 16)): worth 2
[[nodiscard]] std::uint32_t lo_bit(int ordinal) noexcept;      // 1 << (30 - 2 (ordinal % 16)): worth 1
[[nodiscard]] int status_of(const KPlayerTask& task, int ordinal) noexcept;                 // 0x0820E800: 0..3
[[nodiscard]] int status_value(int old_value, int ordinal, int status) noexcept;            // 0x0820E720: the value with the two bits set
[[nodiscard]] std::uint32_t key_hash(std::string_view s) noexcept;                          // 0x0821DF00

// the temp values of a player: task id -> (key -> value), decoded from / encoded into the task values
struct KTaskTemp {
    std::map<int, std::map<std::uint32_t, int>> groups;
    // 0x0820DF90: true when the whole structure was read (false when it runs past 0x8fb - what was read stays)
    bool decode(const KPlayerTask& task);
    // 0x0820DDF0: 2 per group + 2 per pair
    [[nodiscard]] int slots() const noexcept;
    // 0x0820E250: the (id, value) writes in the binary's order - every group in key order (id, n, the pairs in key
    // order), the count at 0x898, then zeros up to 0x8fb
    [[nodiscard]] std::vector<std::pair<int, int>> encode() const;
};

}   // namespace task_status

}   // namespace jx::zone
