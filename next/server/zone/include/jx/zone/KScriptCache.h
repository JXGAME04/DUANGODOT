// KScriptCache of the old engine (g_GetScript): one loaded KLuaScript per script file, created
// on first use and kept for the life of the zone.
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jx/zone/KLuaScript.h"

namespace jx::zone {

class KScriptCache {
public:
    // NPC_LEVELSCRIPT_FILENAME (CoreUseNameDef.h): the level script of templates that name none.
    static constexpr const char* kNpcLevelScript = R"(\script\npclevelscript\npclevelscript.lua)";

    // roots: "a;b" = the reference server folder first, then the fallback (the project server keeps
    // the per-map trap scripts the Linux one lacks); each script loads from the first root that has it.
    explicit KScriptCache(const std::string& roots);
    [[nodiscard]] const std::vector<std::string>& roots() const noexcept { return roots_; }

    // g_GetScript: the script of a game path (`\script\...`), loaded on first use; nullptr when
    // the file is missing or fails to run (the failure is remembered, not retried every call).
    KLuaScript* get(const std::string& game_path);
    [[nodiscard]] std::size_t size() const noexcept { return scripts_.size(); }

private:
    std::vector<std::string> roots_;
    std::unordered_map<std::string, std::unique_ptr<KLuaScript>> scripts_;   // key: lower-cased path; nullptr = failed
};

} // namespace jx::zone
