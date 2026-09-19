#include "jx/zone/KMission.h"

#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>

namespace jx::zone {

namespace {

const std::string kNoScript;

void read_scripts(const nlohmann::json& j, const char* key, std::unordered_map<int, std::string>& out)
{
    const auto it = j.find(key);
    if (it == j.end() || !it->is_object()) return;
    for (const auto& [id, value] : it->items()) {
        if (!value.is_string()) continue;
        out[std::stoi(id)] = value.get<std::string>();
    }
}

} // namespace

std::optional<KMissionTable> KMissionTable::load(const std::string& file, std::string* error)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + file;
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        in >> j;
        KMissionTable t;
        read_scripts(j, "missions", t.missions);
        read_scripts(j, "timers", t.timers);
        return t;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return std::nullopt;
    }
}

const std::string& KMissionTable::mission_script(int id) const
{
    const auto it = missions.find(id);
    return it == missions.end() ? kNoScript : it->second;
}

const std::string& KMissionTable::timer_script(int id) const
{
    const auto it = timers.find(id);
    return it == timers.end() ? kNoScript : it->second;
}

int KMission::add_player(EntityId player, std::uint64_t player_id, int group, std::uint64_t now)
{
    if (player.value == 0 || player_id == 0) return 0;   // 2003: ulPlayerIndex >= MAX_PLAYER || ulPlayerID == 0
    if (entries_.empty()) entries_.emplace_back();       // [0] stays unused
    std::size_t slot = 1;
    while (slot < entries_.size() && entries_[slot].used) ++slot;
    if (slot == entries_.size()) entries_.emplace_back();
    KMissionPlayer& e = entries_[slot];
    e.player = player;
    e.player_id = player_id;
    e.group = group;
    e.join_tick = now;
    e.param1 = 0;
    e.param2 = 0;
    e.used = true;
    return static_cast<int>(slot);
}

bool KMission::remove_player(EntityId player)
{
    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].used && entries_[i].player == player) {
            entries_[i] = KMissionPlayer{};
            return true;
        }
    }
    return false;
}

int KMission::data_index(EntityId player) const
{
    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].used && entries_[i].player == player) return static_cast<int>(i);
    }
    return 0;
}

const KMissionPlayer* KMission::entry(int data_index) const
{
    if (data_index <= 0 || static_cast<std::size_t>(data_index) >= entries_.size()) return nullptr;
    const KMissionPlayer& e = entries_[static_cast<std::size_t>(data_index)];
    return e.used ? &e : nullptr;
}

KMissionPlayer* KMission::entry_mutable(int data_index)
{
    return const_cast<KMissionPlayer*>(static_cast<const KMission*>(this)->entry(data_index));
}

std::pair<int, EntityId> KMission::next_player(int idx, int group) const
{
    for (std::size_t i = idx < 0 ? 1 : static_cast<std::size_t>(idx) + 1; i < entries_.size(); ++i) {
        const KMissionPlayer& e = entries_[i];
        if (!e.used) continue;
        if (group != 0 && e.group != group) continue;
        return {static_cast<int>(i), e.player};
    }
    return {0, EntityId{}};
}

int KMission::player_count(int group) const
{
    int n = 0;
    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].used && (group == 0 || entries_[i].group == group)) ++n;
    }
    return n;
}

std::vector<EntityId> KMission::players(int group) const
{
    std::vector<EntityId> out;
    for (std::size_t i = 1; i < entries_.size(); ++i) {
        if (entries_[i].used && (group == 0 || entries_[i].group == group)) out.push_back(entries_[i].player);
    }
    return out;
}

bool KMission::start_timer(int timer_id, std::uint64_t interval, std::uint64_t now)
{
    if (timers_.size() >= static_cast<std::size_t>(kMaxTimers)) return false;   // KLinkArrayTemplate::Add: no slot left
    // SetTimer(0, id) closes the entry (2003 KTimerTaskFun::SetTimer): it sits there and never fires
    timers_.push_back({interval == 0 ? 0 : timer_id, interval == 0 ? 0 : now + interval, interval});
    return true;
}

void KMission::stop_timer(int timer_id)
{
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        if (it->id == timer_id) {   // GetData(&Timer with the id): the first one
            timers_.erase(it);      // CloseTimer, then Remove (LuaStopMissionTimer 2003)
            return;
        }
    }
}

std::uint64_t KMission::rest_time(int timer_id, std::uint64_t now) const
{
    for (const KMissionTimer& t : timers_) {
        if (t.id == timer_id) return now > t.fire_tick ? 0 : t.fire_tick - now;   // GetRestTime
    }
    return 0;
}

std::vector<int> KMission::fire_timers(std::uint64_t now)
{
    std::vector<int> due;
    for (KMissionTimer& t : timers_) {
        if (t.fire_tick == 0 || t.fire_tick > now) continue;   // 0x080F9495 / 0x080F949F
        t.fire_tick = now + t.interval;                        // 0x080F94A6: the next period first
        if (t.id != 0) due.push_back(t.id);                    // 0x080F94A9: an id names the script
    }
    return due;
}

void KMission::clear()
{
    entries_.clear();
    timers_.clear();
}

} // namespace jx::zone
