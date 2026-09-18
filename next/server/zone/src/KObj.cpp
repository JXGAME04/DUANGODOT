#include "jx/zone/KObj.h"

#include <exception>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"

namespace jx::zone {

bool KObjDataSet::load(const std::string& path, std::string* error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        if (error) *error = std::string("objdata.json: ") + e.what();
        return false;
    }
    objects_.clear();
    money_.clear();
    // named copies: items() keeps a reference to the container it walks
    const nlohmann::json objects = j.value("objects", nlohmann::json::object());
    const nlohmann::json money = j.value("money", nlohmann::json::array());
    for (const auto& [key, o] : objects.items()) {
        KObjTemplate t;
        t.id = o.value("id", 0);
        t.name = o.value("name", "");
        t.kind_name = o.value("kind", "");
        t.kind = t.kind_name == "Item" ? KObjKind::item : t.kind_name == "Money" ? KObjKind::money : KObjKind::other;
        t.life_time = o.value("life_time", 0);
        t.height = o.value("height", 0);
        t.image = o.value("image", "");
        t.drop_image = o.value("drop_image", "");
        objects_[t.id] = std::move(t);
    }
    for (const auto& m : money) money_.emplace_back(m.value("max", 0), m.value("obj", 0));
    if (objects_.empty()) {
        if (error) *error = "objdata.json holds no objects";
        return false;
    }
    log::info("zone", "object data loaded", {log::kv("file", path), log::kv("objects", objects_.size()), log::kv("money_rows", money_.size())});
    return true;
}

const KObjTemplate* KObjDataSet::find(int id) const
{
    const auto it = objects_.find(id);
    return it == objects_.end() ? nullptr : &it->second;
}

int KObjDataSet::money_obj(int amount) const
{
    for (const auto& [max, obj] : money_) {
        if (amount <= max) return obj;
    }
    return money_.empty() ? 0 : money_.back().second;
}

} // namespace jx::zone
