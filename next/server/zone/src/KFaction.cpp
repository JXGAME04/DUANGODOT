#include "jx/zone/KFaction.h"

#include <cstddef>
#include <exception>
#include <fstream>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace jx::zone {

KFaction::KFaction()
{
    entries_.resize(kCount);
    for (int i = 0; i < kCount; ++i) entries_[static_cast<std::size_t>(i)].index = i;   // 0x08060CF0: index i, series 0, camp 1
}

std::optional<KFaction> KFaction::load(const std::string& file, std::string* error)
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
    KFaction t;
    try {
        const auto factions = j.find("factions");
        if (factions == j.end() || !factions->is_array()) {
            if (error) *error = "no factions array in " + file;
            return std::nullopt;
        }
        for (const auto& e : *factions) {
            const int index = e.value("index", -1);
            if (index < 0 || index >= kCount) continue;
            t.set(index, e.value("series", 0), e.value("camp", 1), e.value("name", std::string{}), e.value("show_name", std::string{}));
        }
        t.new_name_ = j.value("new_name", std::string{});
        t.old_name_ = j.value("old_name", std::string{});
        if (const auto skills = j.find("skills"); skills != j.end() && skills->is_object()) {
            for (const auto& [key, ids] : skills->items()) {
                if (!ids.is_array()) continue;
                std::vector<int> list;
                for (const auto& id : ids) {
                    if (id.is_number_integer()) list.push_back(id.get<int>());
                }
                t.skills_[std::stoi(key)] = std::move(list);
            }
        }
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return std::nullopt;
    }
    return t;
}

int KFaction::id_by_name(int series, std::string_view name) const noexcept
{
    if (series < 0 || series > kSeriesCount - 1 || name.empty()) return -1;   // 0x08060C06 (unsigned: a negative series refuses) / 0x08060C3B
    for (const KFactionEntry& e : entries_) {
        if (e.name == name) return e.index;
    }
    return -1;
}

bool KFaction::allows(int series, int index) const noexcept
{
    if (series < 0 || series > kSeriesCount - 1 || index < 0 || index > kCount - 1) return false;
    for (const KFactionEntry& e : entries_) {
        if (e.series == series && e.index == index) return true;
    }
    return false;
}

const KFactionEntry* KFaction::entry(int index) const noexcept
{
    if (index < 0 || index >= kCount) return nullptr;
    return &entries_[static_cast<std::size_t>(index)];
}

const std::vector<int>* KFaction::skills(int index) const noexcept
{
    const auto it = skills_.find(index);
    return it == skills_.end() ? nullptr : &it->second;
}

void KFaction::set(int index, int series, int camp, std::string name, std::string show_name)
{
    if (index < 0 || index >= kCount) return;
    KFactionEntry& e = entries_[static_cast<std::size_t>(index)];
    e.index = index;
    e.series = series;
    e.camp = camp;
    e.name = std::move(name);
    e.show_name = std::move(show_name);
}

void KFaction::set_names(std::string new_name, std::string old_name)
{
    new_name_ = std::move(new_name);
    old_name_ = std::move(old_name);
}

void KFaction::set_skills(int index, std::vector<int> ids)
{
    skills_[index] = std::move(ids);
}

// ---- KPlayerFaction ----

bool KPlayerFaction::add(const KFaction& table, int series, int index) noexcept
{
    if (!table.allows(series, index)) return false;   // 0x08060BB0
    current = index;
    count += 1;
    if (count == 1) first = index;
    last = index;
    return true;
}

int KPlayerFaction::camp(const KFaction* table) const noexcept
{
    if (current < 0) return count != 0 ? KFaction::kCampFree : KFaction::kCampBegin;   // 0x080C2648
    if (current > KFaction::kCount - 1 || table == nullptr) return 0;
    const KFactionEntry* e = table->entry(current);
    return e != nullptr && e->camp >= 0 ? e->camp : 0;   // 0x080C2639: a negative camp reads 0
}

std::string KPlayerFaction::name(const KFaction* table) const
{
    if (current == -1) return count != 0 && table != nullptr ? table->old_name() : std::string{};   // 0x080C26C8
    if (current > KFaction::kCount - 1 || table == nullptr) return {};
    const KFactionEntry* e = table->entry(current);
    return e != nullptr ? e->name : std::string{};
}

std::string KPlayerFaction::last_name(const KFaction* table) const
{
    if (last == -1) return count != 0 && table != nullptr ? table->old_name() : std::string{};   // 0x080C27F8
    if (last > KFaction::kCount - 1 || table == nullptr) return {};
    const KFactionEntry* e = table->entry(last);
    return e != nullptr ? e->name : std::string{};
}

} // namespace jx::zone
