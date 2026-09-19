// KSubWorldEvent.cpp - the events a script registers on a player and the kills that fire them (docs/LINUX-SERVER.md §23).
//
// jx_linux_y 0x08083720 (the end of a npc's death frames): the killer is the player behind Npc[+0x1598] (+0x1908); its
// events (Player+0x8064) are walked by 0x08155F80 with the map of the npc's subworld ([subworld+0xc]), the npc's
// template (+0x1530), its power (0x08079750) and level (+0x20); 0x08156330 takes the first definition that matches,
// counts the kill, fires the script when the total is reached, mirrors the count into the task value and ends the walk.
#include <algorithm>
#include <string>
#include <utility>

#include "jx/log.hpp"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KPlayerEvent.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/ScriptFuns.h"

namespace jx::zone {

int KSubWorld::npc_power(const KNpc& e) const noexcept
{
    return npc_power_of(e.kind == KNpcKind::player, e.boss_flag != 0, e.gold.gold_kind() >= 1);
}

bool KSubWorld::player_event_add(KNpc& e, int id)
{
    if (e.kind != KNpcKind::player) return false;
    const bool ok = e.player.events.add(id, 0);   // 0x0810C58B: the count starts at 0
    log::debug("zone.task", "player event added", {log::kv("entity", e.id), log::kv("event", id), log::kv("ok", ok), log::kv("count", e.player.events.entries.size())});
    return ok;
}

bool KSubWorld::player_event_remove(KNpc& e, int id)
{
    if (e.kind != KNpcKind::player) return false;
    const bool ok = e.player.events.remove(id);
    log::debug("zone.task", "player event removed", {log::kv("entity", e.id), log::kv("event", id), log::kv("ok", ok)});
    return ok;
}

void KSubWorld::fire_kill_events(const KNpc& dead)
{
    if (dead.kind == KNpcKind::player || !cfg_.kill_events) return;
    const KNpc* hitter = entities_.find(dead.last_damage_id);   // Npc[+0x1598]
    if (hitter == nullptr) return;
    const KNpc* owner = owner_of(hitter->id);   // a companion's kills count for its master (+0x1908 of the companion)
    if (owner == nullptr || owner->kind != KNpcKind::player) return;
    KNpc* killer = entities_.find(owner->id);
    if (killer == nullptr) return;
    const int map = static_cast<int>(map_id());
    const int npc_template = static_cast<int>(dead.template_id);
    const int power = npc_power(dead);
    const int level = static_cast<int>(dead.level);
    KPlayerEvent& events = killer->player.events;
    for (std::size_t i = 0; i < events.entries.size(); ++i) {   // 0x08155FA8: in order; the first match ends the walk
        const int event_id = events.entries[i].id;
        const KKillEventRow* def = cfg_.kill_events->find(event_id);
        if (def == nullptr || !def->matches(map, npc_template, power, level)) continue;
        if (!cfg_.scripts || cfg_.scripts->get(def->script) == nullptr) continue;   // 0x081563CF: no such script -> the next event
        int count = events.entries[i].count + 1;   // 0x08156530
        bool removed = false;
        if (def->total <= count) {   // 0x0815645D: the total reached - the script, then the event is dropped or starts over
            execute_script_args(def->script, def->function.c_str(), *killer,
                                {static_cast<double>(killer->sid), static_cast<double>(def->task_id), static_cast<double>(event_id),
                                 static_cast<double>(map), static_cast<double>(npc_template), static_cast<double>(count),
                                 static_cast<double>(power), static_cast<double>(level)});
            log::info("zone.task", "kill event fired", {log::kv("entity", killer->id), log::kv("event", event_id), log::kv("script", def->script),
                                                        log::kv("function", def->function), log::kv("count", count), log::kv("once", def->only_once)});
            if (def->only_once) {
                events.remove(event_id);   // 0x081566AB
                removed = true;
            } else {
                count = 0;   // 0x08156659: the count starts over
            }
        }
        if (!removed) {
            if (KPlayerEventEntry* entry = events.find(event_id)) entry->count = count & 0xffff;   // 0x08156680: the word of the pair
        }
        // 0x081564C3..0x0815650F: the count into the definition's task value, told to the client whatever the table says
        if (static_cast<unsigned>(def->task_id) <= 0x176fu) {
            task_set_value(*killer, def->task_id, count, true);
            task_send_value(*killer, def->task_id);
        }
        log::debug("zone.task", "kill counted", {log::kv("entity", killer->id), log::kv("event", event_id), log::kv("count", count), log::kv("npc", dead.id)});
        return;
    }
}

void KSubWorld::load_player_events(KNpc& e, const pb::RoleData& role)
{
    e.player.events.clear();
    for (const pb::RolePlayerEvent& ev : role.events()) e.player.events.add(static_cast<int>(ev.id()), static_cast<int>(ev.count()));   // 0x080BEB90
}

void KSubWorld::save_player_events(const KNpc& e, pb::RoleData& out) const
{
    out.clear_events();
    for (const KPlayerEventEntry& ev : e.player.events.entries) {
        pb::RolePlayerEvent* r = out.add_events();
        r->set_id(static_cast<std::uint32_t>(ev.id));
        r->set_count(static_cast<std::uint32_t>(ev.count));
    }
}

}   // namespace jx::zone
