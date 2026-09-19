// KPlayerEvent.h - the events a script registers on a player (AddPlayerEvent) and the kill-event table that fires them
// (\settings\npc\player\event_killnpc.txt).
//
// jx_linux_y: the object at Player+0x8064 holds a vector of {word event id, word count} (+0x8068 begin, +0x806c end):
// Add 0x081560C0 (sixty-three at most; an id already there counts as added), Remove 0x08156050, Clear 0x08155F60;
// KPlayer::LoadFrom restores them from the extra block of TRoleData (0x080BEB60: records of {count, id} words) and the
// save writes them back.  The table at 0x8bb2b54 (loader 0x08156760) maps an event id to {script +0x2c, function
// +0x30, task value +0x34, only once +0x38, total +0x3c, map +0x14, npc template +0x18, power +0x1c, level +0x20}.
// When a npc dies (0x08083720, the end of its death frames; the killer is Npc[+0x1598]'s player +0x1908) 0x08155F80
// walks the killer's events in order: the first one whose definition matches the map / template / power / level (each
// -1 = any; 0x08156330) counts one more kill; when the count reaches the total the script's function is called with
// (player index, task value id, event id, map, template, count, power, level) ("dddddddd", 0x080AD300) and the event
// is removed (only once) or its count starts over; the count is then mirrored into the definition's task value
// (SetTaskValue + the 0xa7 packet) and no other event of the kill is looked at.  docs/LINUX-SERVER.md §23.
#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace jx::zone {

struct KPlayerEventEntry {
    int id = 0;      // word
    int count = 0;   // word: the kills so far
};

struct KPlayerEvent {
    static constexpr int kMax = 63;   // 0x081560E7: a sixty-fourth is refused
    std::vector<KPlayerEventEntry> entries;

    // 0x081560C0: true when the id is there afterwards (already there: nothing changes); false when full
    bool add(int id, int count = 0);
    bool remove(int id);              // 0x08156050: true when it was there
    void clear() noexcept { entries.clear(); }   // 0x08155F60
    [[nodiscard]] KPlayerEventEntry* find(int id) noexcept;
    [[nodiscard]] const KPlayerEventEntry* find(int id) const noexcept;
};

// one row of event_killnpc.txt: 事件ID, 事件响应脚本, 事件响应函数, 任务ID, 只响应一次, 杀NPC总数, 地图ID, NPC模板ID, NPC能力
// (-1 any, 0 player, 1 npc, 2 gold, 3 boss), NPC级别, NPC名字, 说明
struct KKillEventRow {
    int id = 0;
    std::string script;      // a game path (\script\...)
    std::string function;
    int task_id = 0;         // the task value the count is mirrored into (<= 0x176f, 0x08156923)
    bool only_once = false;
    int total = 0;
    int map = -1;
    int npc_template = -1;
    int power = -1;
    int level = -1;
    std::string npc_name;
    // 0x08156367..0x081563C7: every filter is -1 (any) or equal; a kill's own -1 matches too
    [[nodiscard]] bool matches(int a_map, int a_template, int a_power, int a_level) const noexcept;
};

class KKillEventTable {
public:
    static std::optional<KKillEventTable> load(const std::string& file, std::string* error);
    void add(KKillEventRow row);   // a task value past 0x176f is dropped like the loader does; a second row of an id is ignored
    [[nodiscard]] const KKillEventRow* find(int id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return rows_.size(); }
    std::string source;

private:
    std::map<int, KKillEventRow> rows_;
};

// 0x08079750: what a npc is to the kill events - a player 0, a boss (+0x181c) 3, a gold one 2, any other 1
[[nodiscard]] int npc_power_of(bool player, bool boss, bool gold) noexcept;

}   // namespace jx::zone
