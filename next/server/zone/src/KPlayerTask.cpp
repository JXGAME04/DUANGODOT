#include "jx/zone/KPlayerTask.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace jx::zone {

namespace {

// `cmp id, 0x176f; ja` of every accessor: a negative id fails like a big one
bool valid_task_id(int id) noexcept { return static_cast<unsigned>(id) <= 0x176fu; }
bool valid_temp_id(int id) noexcept { return static_cast<unsigned>(id) <= 0xffu; }
// GetBits / SetBits: count > 0, start >= 0, start + count <= 32 (0x080CB603..0x080CB611, 0x080CB933..0x080CB949)
bool valid_bits(int start, int count) noexcept { return count > 0 && start >= 0 && start + count <= 32; }

// a flag of task_def.json: a bool, or the 0 / 1 of the table
bool flag_of(const nlohmann::json& row, const char* key)
{
    const auto it = row.find(key);
    if (it == row.end()) return false;
    if (it->is_boolean()) return it->get<bool>();
    if (it->is_number()) return it->get<int>() == 1;
    return false;
}

int int_of(const nlohmann::json& row, const char* key)
{
    const auto it = row.find(key);
    return it != row.end() && it->is_number() ? it->get<int>() : 0;
}

}   // namespace

std::uint32_t KPlayerTask::bit_mask(int start, int count) noexcept
{
    const std::uint32_t all = 0xffffffffu;
    const int high = 32 - count - start;
    const std::uint32_t hi = high >= 32 || high < 0 ? 0u : (all >> high);
    const std::uint32_t lo = start >= 32 || start < 0 ? 0u : (all << start);
    return hi & lo;
}

int KPlayerTask::get_save_val(int id) const noexcept
{
    if (!valid_task_id(id)) return 0;
    const auto it = saved.find(id);
    return it == saved.end() ? 0 : it->second;
}

void KPlayerTask::set_save_val(int id, int value)
{
    if (!valid_task_id(id)) return;
    if (value == 0) {   // 0x080CB738 -> 0x080CB7B8: the node erased
        saved.erase(id);
        return;
    }
    saved[id] = value;
}

int KPlayerTask::get_temp(int id) const noexcept
{
    return valid_temp_id(id) ? temp[static_cast<std::size_t>(id)] : 0;
}

void KPlayerTask::set_temp(int id, int value) noexcept
{
    if (valid_temp_id(id)) temp[static_cast<std::size_t>(id)] = value;
}

std::uint32_t KPlayerTask::get_bits(int id, int start, int count) const noexcept
{
    if (!valid_task_id(id) || !valid_bits(start, count)) return 0;
    const auto it = saved.find(id);
    if (it == saved.end() || it->second == 0) return 0;   // 0x080CB668..0x080CB66D
    return (static_cast<std::uint32_t>(it->second) & bit_mask(start, count)) >> start;
}

bool KPlayerTask::set_bits(int id, int start, int count, int value)
{
    if (!valid_task_id(id) || !valid_bits(start, count)) return false;
    const std::uint32_t mask = bit_mask(start, count);
    const std::uint32_t bits = (static_cast<std::uint32_t>(value) << start) & mask;
    const auto it = saved.find(id);
    // 0x080CB9A9: a value there keeps its other bits; none -> the bits alone (0x080CB9F0)
    const std::uint32_t old = it == saved.end() ? 0u : static_cast<std::uint32_t>(it->second);
    set_save_val(id, static_cast<int>((old & ~mask) | bits));
    return true;
}

void KPlayerTask::clear_range(int first, int count)
{
    if (!valid_task_id(first)) return;
    // 0x080CBC5B..0x080CBC72: the end is first + count, at most 0x1770; nothing when first is not below it
    const long long end = std::min<long long>(static_cast<long long>(first) + count, kTaskValueCount);
    if (first >= end) return;
    saved.erase(saved.lower_bound(first), saved.lower_bound(static_cast<int>(end)));
}

void KPlayerTask::release() noexcept
{
    temp.fill(0);
    saved.clear();
}

std::vector<std::pair<int, int>> KPlayerTask::serialize() const
{
    std::vector<std::pair<int, int>> out;
    out.reserve(saved.size());
    for (const auto& [id, value] : saved) {
        if (value != 0) out.emplace_back(id, value);   // 0x080CB6E8: a zero is not written
    }
    return out;
}

// ---- the table ----------------------------------------------------------------------------------

void KTaskDefTable::add(const KTaskDefRow& row)
{
    if (row.first == 0) return;   // 0x081C6EEC
    int last = row.last == 0 ? row.first : row.last;   // 0x081C6EF5
    const unsigned flags = (row.sync ? task_def_sync : 0u) | (row.client ? task_def_client : 0u);
    if (flags == 0) return;   // 0x081C6E8D: neither flag - the row leaves nothing
    if (row.sync) ranges_.emplace(row.first, KTaskDefRange{row.first, last});   // insert-unique (0x081C6F27..0x081C6FA4)
    // (the binary walks first..last however far apart they are; ids the values can never have are left out here)
    last = std::min(last, kTaskValueCount - 1);
    for (int id = row.first; id <= last; ++id) flags_[id] = static_cast<unsigned char>(flags);   // 0x081C6FD0..0x081C703F
}

unsigned KTaskDefTable::flags(int id) const noexcept
{
    const auto it = flags_.find(id);
    return it == flags_.end() ? 0u : it->second;
}

std::optional<KTaskDefTable> KTaskDefTable::load(const std::string& file, std::string* error)
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
    KTaskDefTable t;
    if (const auto s = j.find("source"); s != j.end() && s->is_string()) t.source = s->get<std::string>();
    for (const auto& r : *rows) {
        if (!r.is_object()) continue;
        KTaskDefRow row;
        row.first = int_of(r, "first");
        row.last = int_of(r, "last");
        row.sync = flag_of(r, "sync");
        row.client = flag_of(r, "client");
        if (const auto n = r.find("name"); n != r.end() && n->is_string()) row.name = n->get<std::string>();
        t.add(row);
    }
    return t;
}

}   // namespace jx::zone
