#include "jx/zone/KScriptCache.h"

#include <cctype>
#include <filesystem>

#include "jx/log.hpp"

namespace jx::zone {

KScriptCache::KScriptCache(const std::string& roots)
{
    std::string cur;
    for (const char c : roots + ";") {
        if (c == ';') {
            while (!cur.empty() && (cur.back() == ' ' || cur.back() == '\t')) cur.pop_back();
            while (!cur.empty() && (cur.front() == ' ' || cur.front() == '\t')) cur.erase(cur.begin());
            if (!cur.empty()) roots_.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
}

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
    std::string root;
    for (const std::string& r : roots_) {
        if (std::filesystem::exists(KLuaScript::os_path(KLuaScript::resolve(r, key)))) {
            root = r;
            break;
        }
    }
    if (root.empty()) {
        log::warn("lua", "script not found in any root", {log::kv("file", key), log::kv("roots", roots_.size())});
        scripts_.emplace(key, nullptr);
        return nullptr;
    }
    auto script = std::make_unique<KLuaScript>();
    if (!script->init(root) || !script->load(key)) {
        scripts_.emplace(key, nullptr);
        return nullptr;
    }
    log::info("lua", "script loaded", {log::kv("file", key)});
    KLuaScript* raw = script.get();
    scripts_.emplace(key, std::move(script));
    return raw;
}

} // namespace jx::zone
