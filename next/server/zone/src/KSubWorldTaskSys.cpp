// KSubWorldTaskSys.cpp - the task system of the scripts on a player (docs/LINUX-SERVER.md §22): the status of a task
// and its temp values live in the task values (KPlayerTask), the tables in KTaskManager.
//
// jx_linux_y builds an iterator object (0x0820E170) over the player's task values for every call of the TASKSYS
// library: 0x0820DF90 decodes the temp structure, 0x0820E800 / 0x0820E720 read and write the two status bits,
// 0x0820E4E0 / 0x0820E430 / 0x0820E5C0 change the structure and 0x0820E250 writes it back through 0x0820E1E0 (a
// changed value goes to the client as the 0xa7 packet whatever its SYNC_FLAG), 0x0820DCD0 copies the task ids of the
// groups into the player's list (0x9786620 + 0x78 + index * 12) that FirstTask / NextTask walk.
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "jx/log.hpp"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KTaskManager.h"
#include "jx/zone/ScriptFuns.h"

namespace jx::zone {

namespace {

constexpr const char* kTaskFunctionScript = R"(\script\task\system\task_function.lua)";   // 0x08256720

}   // namespace

void KSubWorld::task_set_value_synced(KNpc& e, int id, int value)
{
    if (e.player.task.get_save_val(id) == value) return;   // 0x0820E20A
    task_set_value(e, id, value, true);
    task_send_value(e, id);   // 0x0820E238: the 0xa7 packet whatever the table says
}

void KSubWorld::task_write_temp(KNpc& e, const task_status::KTaskTemp& temp)
{
    for (const auto& [id, value] : temp.encode()) task_set_value_synced(e, id, value);
}

std::optional<int> KSubWorld::task_status(const KNpc& e, std::string_view name) const
{
    if (!cfg_.tasks) return std::nullopt;
    const auto ordinal = cfg_.tasks->ordinal_of(name);   // 0x08170440 then 0x08170100 / 0x08172820
    if (!ordinal) return std::nullopt;
    return task_status::status_of(e.player.task, *ordinal);
}

bool KSubWorld::task_set_status(KNpc& e, std::string_view name, int status)
{
    if (!cfg_.tasks) return false;
    const auto ordinal = cfg_.tasks->ordinal_of(name);
    if (!ordinal) return false;
    const int id = task_status::value_id(*ordinal);
    const int value = task_status::status_value(e.player.task.get_save_val(id), *ordinal, status);
    task_set_value_synced(e, id, value);
    log::debug("zone.task", "task status changed", {log::kv("entity", e.id), log::kv("task", std::string(name)), log::kv("status", status & 3)});
    return true;
}

bool KSubWorld::task_start(KNpc& e, std::string_view name)
{
    if (!cfg_.tasks) return false;
    const auto id = cfg_.tasks->id_of(name);
    if (!id) return false;
    task_status::KTaskTemp temp;
    temp.decode(e.player.task);
    if (temp.groups.count(*id) != 0) return false;                      // 0x0820E51D: already started
    if (temp.slots() > task_status::kTempSlots) return false;           // 0x0820E538: no room for another group
    temp.groups[*id];
    task_write_temp(e, temp);
    log::debug("zone.task", "task started", {log::kv("entity", e.id), log::kv("task", std::string(name)), log::kv("id", *id)});
    return true;
}

bool KSubWorld::task_close(KNpc& e, std::string_view name)
{
    if (!cfg_.tasks) return false;
    const auto id = cfg_.tasks->id_of(name);
    if (!id) return false;
    task_status::KTaskTemp temp;
    temp.decode(e.player.task);
    if (temp.groups.erase(*id) == 0) return false;   // 0x0820E467: not started
    task_write_temp(e, temp);
    log::debug("zone.task", "task closed", {log::kv("entity", e.id), log::kv("task", std::string(name)), log::kv("id", *id)});
    return true;
}

std::optional<int> KSubWorld::task_temp(const KNpc& e, std::string_view name, std::string_view key) const
{
    if (!cfg_.tasks) return std::nullopt;
    const auto id = cfg_.tasks->id_of(name);
    if (!id) return std::nullopt;
    task_status::KTaskTemp temp;
    temp.decode(e.player.task);
    const auto g = temp.groups.find(*id);
    if (g == temp.groups.end()) return std::nullopt;   // 0x0820DF2F: no group (the read creates none that lasts)
    const auto v = g->second.find(task_status::key_hash(key));
    if (v == g->second.end()) return std::nullopt;
    return v->second;
}

bool KSubWorld::task_set_temp(KNpc& e, std::string_view name, std::string_view key, int value)
{
    if (!cfg_.tasks) return false;
    const auto id = cfg_.tasks->id_of(name);
    if (!id) return false;
    task_status::KTaskTemp temp;
    temp.decode(e.player.task);
    temp.groups[*id][task_status::key_hash(key)] = static_cast<int>(static_cast<std::uint16_t>(value));   // 0x0820E5CC: a word; a group when none
    task_write_temp(e, temp);
    return true;
}

const char* KSubWorld::task_first(KNpc& e)
{
    // 0x0820DCD0: the ids of the groups, in key order, into the player's list; the cursor at the first
    e.player.task_list.clear();
    e.player.task_cursor = 0;
    if (!cfg_.tasks) return nullptr;
    task_status::KTaskTemp temp;
    temp.decode(e.player.task);
    for (const auto& [id, sub] : temp.groups) e.player.task_list.push_back(id);
    return task_next(e);
}

const char* KSubWorld::task_next(KNpc& e)
{
    if (!cfg_.tasks || e.player.task_cursor >= e.player.task_list.size()) {   // 0x08174D93: the end clears the list
        e.player.task_list.clear();
        e.player.task_cursor = 0;
        return nullptr;
    }
    const int id = e.player.task_list[e.player.task_cursor++];
    return cfg_.tasks->name_of(id);   // 0x08170060: nil when the id names nothing (the cursor moved on)
}

bool KSubWorld::task_select(KNpc& e, const char* fn, int task_id)
{
    // SelectTaskStart / SelectTaskFinish / SelectTaskAward (0x08174690 / 0x081745E0 / 0x08174530): the menu
    // functions of task_function.lua with the task id (0x080AD300 CallFunction(fn, 0, "d", id))
    if (!cfg_.scripts) return false;
    KLuaScript* script = cfg_.scripts->get(kTaskFunctionScript);
    if (script == nullptr) {
        log::warn("zone.task", "task function script missing", {log::kv("entity", e.id), log::kv("function", fn)});
        return false;
    }
    const bool ok = execute_script(kTaskFunctionScript, fn, e, task_id);
    log::debug("zone.task", "task menu", {log::kv("entity", e.id), log::kv("function", fn), log::kv("id", task_id), log::kv("ok", ok)});
    return ok;
}

}   // namespace jx::zone
