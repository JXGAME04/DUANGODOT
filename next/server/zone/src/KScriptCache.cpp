#include "jx/zone/KScriptCache.h"

#include <cctype>

#include "jx/log.hpp"

namespace jx::zone {

KLuaScript* KScriptCache::get(const std::string& game_path)
{
    std::string key = game_path;
    for (char& c : key) {
        if (c == '/') c = '\\';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (key.empty()) return nullptr;
    const auto it = scripts_.find(key);
    if (it != scripts_.end()) return it->second.get();
    auto script = std::make_unique<KLuaScript>();
    if (!script->init(root_) || !script->load(key)) {
        scripts_.emplace(key, nullptr);
        return nullptr;
    }
    log::info("lua", "script loaded", {log::kv("file", key)});
    KLuaScript* raw = script.get();
    scripts_.emplace(key, std::move(script));
    return raw;
}

} // namespace jx::zone
