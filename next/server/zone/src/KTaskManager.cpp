#include "jx/zone/KTaskManager.h"

#include <exception>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace jx::zone {

namespace {

// the JSON of jxassets export-task-tables carries the bytes of the files as Latin-1 runes: back to the bytes the
// scripts compare with (every code point below 0x100 is one byte; anything else - never written - is kept as is)
std::string latin1_bytes(const std::string& utf8)
{
    std::string out;
    out.reserve(utf8.size());
    for (std::size_t i = 0; i < utf8.size(); ++i) {
        const auto c = static_cast<unsigned char>(utf8[i]);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if ((c == 0xC2 || c == 0xC3) && i + 1 < utf8.size()) {
            const auto d = static_cast<unsigned char>(utf8[i + 1]);
            out.push_back(static_cast<char>(((c & 0x1F) << 6) | (d & 0x3F)));
            ++i;
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

std::vector<std::string> cells_of(const nlohmann::json& row)
{
    std::vector<std::string> cells;
    const auto it = row.find("cells");
    if (it == row.end() || !it->is_array()) return cells;
    cells.reserve(it->size());
    for (const auto& c : *it) cells.push_back(c.is_string() ? latin1_bytes(c.get<std::string>()) : std::string());
    return cells;
}

std::string string_of(const nlohmann::json& row, const char* key)
{
    const auto it = row.find(key);
    return it != row.end() && it->is_string() ? latin1_bytes(it->get<std::string>()) : std::string();
}

int int_of(const nlohmann::json& row, const char* key)
{
    const auto it = row.find(key);
    return it != row.end() && it->is_number() ? it->get<int>() : 0;
}

void load_table(const nlohmann::json& rows, KTaskTable& table)
{
    if (!rows.is_array()) return;
    for (const auto& r : rows) {
        if (!r.is_object()) continue;
        table.add(string_of(r, "key"), cells_of(r));
    }
}

}   // namespace

const std::string* KTaskMatrix::cell(int row, int col) const noexcept
{
    if (row < 0 || row >= row_count() || col < 0 || col >= cols) return nullptr;
    const auto& r = rows[static_cast<std::size_t>(row)];
    if (col >= static_cast<int>(r.size())) return nullptr;
    return &r[static_cast<std::size_t>(col)];
}

void KTaskTable::add(std::string key, std::vector<std::string> cells)
{
    KTaskMatrix& m = groups_[std::move(key)];
    if (static_cast<int>(cells.size()) > m.cols) m.cols = static_cast<int>(cells.size());
    m.rows.push_back(std::move(cells));
}

const KTaskMatrix* KTaskTable::find(std::string_view key) const noexcept
{
    const auto it = groups_.find(key);
    return it == groups_.end() ? nullptr : &it->second;
}

// ---- the manager ------------------------------------------------------------------------------

void KTaskManager::add_task(int id, std::string name, int event, std::string type, std::vector<std::string> cells)
{
    KTaskRecord r;
    r.id = id;
    r.name = std::move(name);
    r.event = event;
    r.type = std::move(type);
    r.ordinal = static_cast<int>(tasks_.size());
    const std::size_t index = tasks_.size();
    // the maps keep the first row of a name / id (insert-unique like the binary's std::map inserts)
    by_name_.emplace(r.name, index);
    by_id_.emplace(r.id, index);
    by_event_[r.event].push_back(r.name);
    id_table_.add(std::to_string(r.id), std::move(cells));
    tasks_.push_back(std::move(r));
}

void KTaskManager::add_event_row(std::string key, std::vector<std::string> cells)
{
    event_table_.add(std::move(key), std::move(cells));
}

void KTaskManager::add_type(KTaskType type)
{
    std::string name = type.name;
    types_.emplace(std::move(name), std::move(type));
}

const KTaskRecord* KTaskManager::by_name(std::string_view name) const noexcept
{
    const auto it = by_name_.find(name);
    return it == by_name_.end() ? nullptr : &tasks_[it->second];
}

const KTaskRecord* KTaskManager::by_id(int id) const noexcept
{
    const auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &tasks_[it->second];
}

const char* KTaskManager::name_of(int id) const noexcept
{
    const KTaskRecord* r = by_id(id);
    return r == nullptr ? nullptr : r->name.c_str();
}

std::optional<int> KTaskManager::id_of(std::string_view name) const noexcept
{
    const KTaskRecord* r = by_name(name);
    if (r == nullptr) return std::nullopt;
    return r->id;
}

std::optional<int> KTaskManager::ordinal_of(std::string_view name) const noexcept
{
    const KTaskRecord* r = by_name(name);
    if (r == nullptr) return std::nullopt;
    return r->ordinal;
}

std::optional<int> KTaskManager::event_of(std::string_view name) const noexcept
{
    const KTaskRecord* r = by_name(name);
    if (r == nullptr) return std::nullopt;
    return r->event;
}

int KTaskManager::event_task_count(int event) const noexcept
{
    const auto it = by_event_.find(event);
    return it == by_event_.end() ? 0 : static_cast<int>(it->second.size());
}

const std::string* KTaskManager::event_task(int event, int index) const noexcept
{
    const auto it = by_event_.find(event);
    if (it == by_event_.end() || index < 0 || index >= static_cast<int>(it->second.size())) return nullptr;   // 0x08170255
    return &it->second[static_cast<std::size_t>(index)];
}

const KTaskMatrix* KTaskManager::id_matrix(std::string_view key) const noexcept { return id_table_.find(key); }
const KTaskMatrix* KTaskManager::event_matrix(std::string_view key) const noexcept { return event_table_.find(key); }

const KTaskType* KTaskManager::type_of(std::string_view name) const noexcept
{
    const KTaskRecord* r = by_name(name);
    if (r == nullptr) return nullptr;
    const auto it = types_.find(r->type);
    return it == types_.end() ? nullptr : &it->second;
}

const KTaskMatrix* KTaskManager::condition(std::string_view name) const noexcept
{
    const KTaskType* t = type_of(name);
    return t == nullptr ? nullptr : t->condition.find(name);
}

const KTaskMatrix* KTaskManager::entity(std::string_view name) const noexcept
{
    const KTaskType* t = type_of(name);
    return t == nullptr ? nullptr : t->entity.find(name);
}

const KTaskMatrix* KTaskManager::award(std::string_view name) const noexcept
{
    const KTaskType* t = type_of(name);
    return t == nullptr ? nullptr : t->award.find(name);
}

const KTaskMatrix* KTaskManager::talk(std::string_view name) const noexcept
{
    const KTaskType* t = type_of(name);
    return t == nullptr ? nullptr : t->talk.find(name);
}

std::optional<KTaskManager> KTaskManager::load(const std::string& file, std::string* error)
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
    const auto tasks = j.find("tasks");
    if (tasks == j.end() || !tasks->is_array()) {
        if (error) *error = "no tasks";
        return std::nullopt;
    }
    KTaskManager m;
    if (const auto s = j.find("source"); s != j.end() && s->is_string()) m.source = s->get<std::string>();
    for (const auto& r : *tasks) {
        if (!r.is_object()) continue;
        m.add_task(int_of(r, "id"), string_of(r, "name"), int_of(r, "event"), string_of(r, "type"), cells_of(r));
    }
    if (const auto ev = j.find("events"); ev != j.end()) load_table(*ev, m.event_table_);
    if (const auto types = j.find("types"); types != j.end() && types->is_array()) {
        for (const auto& t : *types) {
            if (!t.is_object()) continue;
            KTaskType type;
            type.name = string_of(t, "name");
            if (const auto c = t.find("condition"); c != t.end()) load_table(*c, type.condition);
            if (const auto c = t.find("entity"); c != t.end()) load_table(*c, type.entity);
            if (const auto c = t.find("award"); c != t.end()) load_table(*c, type.award);
            if (const auto c = t.find("talk"); c != t.end()) load_table(*c, type.talk);
            m.add_type(std::move(type));
        }
    }
    return m;
}

// ---- the status bits and the temp values ------------------------------------------------------

namespace task_status {

int value_id(int ordinal) noexcept
{
    // 0x0820E84A..0x0820E855: (ordinal + 15 when negative) >> 4, plus 0x7d0
    return kStatusValueFirst + (ordinal < 0 ? (ordinal + 15) / 16 : ordinal / 16);
}

namespace {

int bit_pair(int ordinal) noexcept
{
    // 0x0820E838..0x0820E848: (ordinal % 16) * 2, the remainder as the fpu makes it (toward zero)
    return (ordinal % 16) * 2;
}

}   // namespace

std::uint32_t hi_bit(int ordinal) noexcept { return 1u << (31 - bit_pair(ordinal)); }
std::uint32_t lo_bit(int ordinal) noexcept { return 1u << (30 - bit_pair(ordinal)); }

int status_of(const KPlayerTask& task, int ordinal) noexcept
{
    const auto v = static_cast<std::uint32_t>(task.get_save_val(value_id(ordinal)));
    return ((v & hi_bit(ordinal)) != 0 ? 2 : 0) | ((v & lo_bit(ordinal)) != 0 ? 1 : 0);
}

int status_value(int old_value, int ordinal, int status) noexcept
{
    auto v = static_cast<std::uint32_t>(old_value);
    v = (status & 2) != 0 ? (v | hi_bit(ordinal)) : (v & ~hi_bit(ordinal));   // 0x0820E7A9 / 0x0820E7F0
    v = (status & 1) != 0 ? (v | lo_bit(ordinal)) : (v & ~lo_bit(ordinal));   // 0x0820E7C4 / 0x0820E7F8
    return static_cast<int>(v);
}

std::uint32_t key_hash(std::string_view s) noexcept
{
    std::uint32_t h = 0;
    std::uint32_t n = 0;
    for (const char ch : s) {
        ++n;
        const auto c = static_cast<std::int32_t>(static_cast<signed char>(ch));
        std::uint32_t x = static_cast<std::uint32_t>(c * static_cast<std::int32_t>(n)) + h;
        if (x >= 0x8000000bu) x -= 0x8000000bu;
        h = x * 0xffffffefu;
    }
    return h ^ 0x12345678u;
}

bool KTaskTemp::decode(const KPlayerTask& task)
{
    groups.clear();
    const int count = task.get_save_val(kTempCountId);
    if (count <= 0) return true;
    int pos = kTempFirst;
    for (int i = 0; i < count; ++i) {
        const int key = task.get_save_val(pos);
        const int n = task.get_save_val(pos + 1);
        pos += 2;
        if (pos + 2 * n + 2 > kTempLast) return false;   // 0x0820E00C: the structure runs past the values
        auto& sub = groups[key];
        for (int j = 0; j < n; ++j) {
            const auto k = static_cast<std::uint32_t>(task.get_save_val(pos));
            const int v = task.get_save_val(pos + 1);
            pos += 2;
            sub.emplace(k, v);   // 0x080DF800 insert-unique: the first pair of a key stays
        }
        if (pos > kTempLast - 2 && i + 1 < count) return false;   // 0x0820E112
    }
    return true;
}

int KTaskTemp::slots() const noexcept
{
    int n = 0;
    for (const auto& [key, sub] : groups) n += 2 + 2 * static_cast<int>(sub.size());
    return n;
}

std::vector<std::pair<int, int>> KTaskTemp::encode() const
{
    std::vector<std::pair<int, int>> out;
    int pos = kTempFirst;
    for (const auto& [key, sub] : groups) {
        out.emplace_back(pos, key);
        out.emplace_back(pos + 1, static_cast<int>(sub.size()));
        pos += 2;
        for (const auto& [k, v] : sub) {
            out.emplace_back(pos, static_cast<int>(k));
            out.emplace_back(pos + 1, v);
            pos += 2;
        }
    }
    out.emplace_back(kTempCountId, static_cast<int>(groups.size()));
    for (; pos <= kTempLast; ++pos) out.emplace_back(pos, 0);   // 0x0820E378..0x0820E3AE
    return out;
}

}   // namespace task_status

}   // namespace jx::zone
