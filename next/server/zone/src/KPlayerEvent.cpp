#include "jx/zone/KPlayerEvent.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace jx::zone {

bool KPlayerEvent::add(int id, int count)
{
    if (find(id) != nullptr) return true;   // 0x081560F0..0x08156143: already there, still 1
    if (static_cast<int>(entries.size()) > kMax) return false;   // 0x081560E7: more than 0x3f held -> refused
    entries.push_back(KPlayerEventEntry{id & 0xffff, count & 0xffff});
    return true;
}

bool KPlayerEvent::remove(int id)
{
    const auto it = std::find_if(entries.begin(), entries.end(), [id](const KPlayerEventEntry& e) { return e.id == id; });
    if (it == entries.end()) return false;
    entries.erase(it);   // 0x08156093: memmove of the tail
    return true;
}

KPlayerEventEntry* KPlayerEvent::find(int id) noexcept
{
    for (KPlayerEventEntry& e : entries) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

const KPlayerEventEntry* KPlayerEvent::find(int id) const noexcept
{
    for (const KPlayerEventEntry& e : entries) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

bool KKillEventRow::matches(int a_map, int a_template, int a_power, int a_level) const noexcept
{
    const auto ok = [](int want, int have) { return want == -1 || have == -1 || want == have; };
    return ok(map, a_map) && ok(npc_template, a_template) && ok(power, a_power) && ok(level, a_level);
}

void KKillEventTable::add(KKillEventRow row)
{
    if (row.task_id > 0x176f) return;   // 0x08156923
    const int id = row.id;
    rows_.emplace(id, std::move(row));
}

const KKillEventRow* KKillEventTable::find(int id) const noexcept
{
    const auto it = rows_.find(id);
    return it == rows_.end() ? nullptr : &it->second;
}

namespace {

int int_of(const nlohmann::json& r, const char* key, int def)
{
    const auto it = r.find(key);
    if (it == r.end()) return def;
    if (it->is_number()) return it->get<int>();
    if (it->is_boolean()) return it->get<bool>() ? 1 : 0;
    return def;
}

std::string string_of(const nlohmann::json& r, const char* key)
{
    const auto it = r.find(key);
    return it != r.end() && it->is_string() ? it->get<std::string>() : std::string();
}

}   // namespace

std::optional<KKillEventTable> KKillEventTable::load(const std::string& file, std::string* error)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + file;
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return std::nullopt;
    }
    const auto rows = j.find("rows");
    if (rows == j.end() || !rows->is_array()) {
        if (error) *error = "no rows";
        return std::nullopt;
    }
    KKillEventTable t;
    if (const auto s = j.find("source"); s != j.end() && s->is_string()) t.source = s->get<std::string>();
    for (const auto& r : *rows) {
        if (!r.is_object()) continue;
        KKillEventRow row;
        row.id = int_of(r, "id", -1);
        row.script = string_of(r, "script");
        row.function = string_of(r, "function");
        row.task_id = int_of(r, "task_id", -1);
        row.only_once = int_of(r, "only_once", 0) != 0;
        row.total = int_of(r, "total", -1);
        row.map = int_of(r, "map", -1);
        row.npc_template = int_of(r, "npc_template", -1);
        row.power = int_of(r, "power", -1);
        row.level = int_of(r, "level", -1);
        row.npc_name = string_of(r, "npc_name");
        t.add(std::move(row));
    }
    return t;
}

int npc_power_of(bool player, bool boss, bool gold) noexcept
{
    if (player) return 0;   // 0x0807975B: kind 1
    if (boss) return 3;     // 0x08079761: +0x181c
    return gold ? 2 : 1;    // 0x08079779: GoldKind >= 1
}

}   // namespace jx::zone
