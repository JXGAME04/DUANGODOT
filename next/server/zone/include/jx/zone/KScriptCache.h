// KScriptCache of the old engine (g_GetScript): one loaded KLuaScript per script file, created
// on first use and kept for the life of the zone.
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "jx/zone/KLuaScript.h"

namespace jx::zone {

class KScriptCache {
public:
    // NPC_LEVELSCRIPT_FILENAME (CoreUseNameDef.h): the level script of templates that name none.
    static constexpr const char* kNpcLevelScript = R"(\script\npclevelscript\npclevelscript.lua)";

    explicit KScriptCache(std::string root) : root_(std::move(root)) {}
    [[nodiscard]] const std::string& root() const noexcept { return root_; }

    // g_GetScript: the script of a game path (`\script\...`), loaded on first use; nullptr when
    // the file is missing or fails to run (the failure is remembered, not retried every call).
    KLuaScript* get(const std::string& game_path);
    [[nodiscard]] std::size_t size() const noexcept { return scripts_.size(); }

private:
    std::string root_;
    std::unordered_map<std::string, std::unique_ptr<KLuaScript>> scripts_;   // key: lower-cased path; nullptr = failed
};

} // namespace jx::zone
