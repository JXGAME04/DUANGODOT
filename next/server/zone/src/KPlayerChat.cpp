#include "jx/zone/KPlayerChat.h"

#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>

namespace jx::zone {

std::optional<KChatCostTable> KChatCostTable::load(const std::string& file, std::string* error)
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
    KChatCostTable t;
    const auto rows = j.find("rows");
    if (rows == j.end() || !rows->is_array()) {
        if (error) *error = "no rows";
        return std::nullopt;
    }
    for (std::size_t i = 0; i < t.rows.size() && i < rows->size(); ++i) {
        const auto& r = (*rows)[i];
        if (!r.is_object()) continue;
        t.rows[i].level = r.value("level", 0);
        t.rows[i].money = r.value("money", 0);
        t.rows[i].mana_percent = r.value("mana_percent", 0);
        t.rows[i].stamina_percent = r.value("stamina_percent", 0);
    }
    return t;
}

int chat_cost_type(pb::ChatChannel channel) noexcept
{
    switch (channel) {
    case pb::CH_CITY: return 2;      // relay_channcfg.ini [broadcast] name=CITY cost = 2
    case pb::CH_FACTION: return 3;   // [faction] cost = 3
    case pb::CH_WORLD: return 4;     // relay_channel.ini [WORLD] cost = 4
    case pb::CH_NEARBY:              // [screen] cost = 0
    case pb::CH_TEAM:                // [team] cost = 0
    case pb::CH_TONG:                // [tong] cost = 0
    case pb::CH_WHISPER:             // [system] defCost = 0
    default: return 0;
    }
}

}   // namespace jx::zone
