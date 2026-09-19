// KSubWorldTask.cpp - the task values of a player and the client that mirrors them (docs/LINUX-SERVER.md §21).
//
// jx_linux_y: KPlayer::SetTaskValue 0x080A9190 (unchanged -> nothing; SetSaveVal; a SYNC_FLAG id with bSync -> the 0xa7
// packet 0x080A8CC0 {0xa7, int id, int value}), SyncTaskValueMore 0x080A9550 (the 0xb5 packet: eighty {int id, int value}
// per packet, a range within 0..0x176f with first <= last, the zeros left out on request), the enter-world stage
// 0x080B9CF0 (every id of every SYNC_FLAG range as a 0xa7, then SyncTaskValueMore(1000, 1070, 1)), the client's 0xaa
// packet 0x080DB070 (a CLIENT_FLAG id set without a sync back), KPlayer::LoadPlayerTaskList 0x080C0050 /
// SavePlayerTaskList 0x080BF1C0 (the {id, value} pairs of TRoleData).
#include <algorithm>
#include <utility>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KPlayerTask.h"
#include "jx/zone/KSubWorld.h"

namespace jx::zone {

void KSubWorld::task_set_value(KNpc& e, int id, int value, bool sync)
{
    if (e.kind != KNpcKind::player) return;
    KPlayerTask& task = e.player.task;
    if (task.get_save_val(id) == value) return;   // 0x080A91C0: the same value changes nothing and tells nobody
    task.set_save_val(id, value);
    if (sync && cfg_.task_def && cfg_.task_def->synced(id)) task_send_value(e, id);   // 0x080A91D4..0x080A9235
}

void KSubWorld::task_send_value(const KNpc& e, int id)
{
    if (static_cast<unsigned>(id) > 0x176fu) return;   // 0x080A8CCF
    pb::TaskValue m;
    m.set_id(id);
    m.set_value(e.player.task.get_save_val(id));
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TASK_VALUE), m);
}

bool KSubWorld::task_sync_more(const KNpc& e, int first, int last, bool only_non_zero)
{
    // 0x080A9562..0x080A957E: both ends within 0..0x176f and first <= last
    if (last < 0 || first < 0 || last >= kTaskValueCount || first >= kTaskValueCount || first > last) return false;
    pb::TaskValues m;
    for (int id = first; id <= last; ++id) {
        const int value = e.player.task.get_save_val(id);
        if (only_non_zero && value == 0) continue;   // 0x080A9655
        pb::TaskValue* v = m.add_values();
        v->set_id(id);
        v->set_value(value);
        if (m.values_size() >= kTaskSyncMoreBatch) {   // 0x080A9632: the eightieth pair sends the packet
            emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TASK_VALUES), m);
            m.clear_values();
        }
    }
    if (m.values_size() > 0) emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TASK_VALUES), m);   // 0x080A9668
    return true;
}

void KSubWorld::task_login_sync(const KNpc& e)
{
    int sent = 0;
    if (cfg_.task_def) {
        for (const auto& [key, r] : cfg_.task_def->sync_ranges()) {   // 0x080B9DA0..0x080B9E5F
            (void)key;
            for (int id = r.first; id <= r.last; ++id) {
                task_send_value(e, id);
                ++sent;
            }
        }
    }
    task_sync_more(e, kTaskLoginSyncFirst, kTaskLoginSyncLast, true);   // 0x080B9E7F
    log::debug("zone.task", "task values synced", {log::kv("entity", e.id), log::kv("count", sent)});
}

bool KSubWorld::task_value_request(std::uint64_t sid, int id, int value)
{
    const KNpc* c = find_player(sid);
    KNpc* e = c == nullptr ? nullptr : find_mutable(c->id);
    if (e == nullptr) return false;
    if (!cfg_.task_def || !cfg_.task_def->client_may_set(id)) {   // 0x080DB0C4: bit 1 of the id's flags
        log::debug("zone.task", "task value from client refused", {log::kv("entity", e->id), log::kv("id", id), log::kv("value", value)});
        return false;
    }
    task_set_value(*e, id, value, false);   // 0x080DB0FB: bSync = 0
    log::debug("zone.task", "task value from client", {log::kv("entity", e->id), log::kv("id", id), log::kv("value", value)});
    // 0x080DB100: id 0xb41 (kTaskTeamFlag) also runs 0x080A9240(player, value & 2) - the team flag at Player+0x7cfc (not ported)
    return true;
}

void KSubWorld::load_task_values(KNpc& e, const pb::RoleData& role)
{
    e.player.task.release();
    int kept = 0;
    for (const pb::RoleTaskValue& v : role.task_values()) {
        if (v.id() > 0x176fu) continue;   // 0x080C0187: an id past the table is dropped
        e.player.task.set_save_val(static_cast<int>(v.id()), v.value());
        ++kept;
    }
    log::debug("zone.player", "task values loaded", {log::kv("entity", e.id), log::kv("count", kept)});
}

void KSubWorld::save_task_values(const KNpc& e, pb::RoleData& out) const
{
    out.clear_task_values();
    for (const auto& [id, value] : e.player.task.serialize()) {
        pb::RoleTaskValue* v = out.add_task_values();
        v->set_id(static_cast<std::uint32_t>(id));
        v->set_value(value);
    }
}

}   // namespace jx::zone
