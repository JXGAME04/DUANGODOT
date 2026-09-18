// KLuaScript of the old engine (Engine/Src/KLuaScript.h) on standard Lua 5.4: one interpreter
// state per script file, Load / Include / CallFunction.  There is no Lua 4 compatibility layer:
// the scripts themselves are real Lua 5.4, converted once by `python tools/dev.py lua`
// (services/pkg/jxlua).  See docs/SCRIPTS.md.
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct lua_State;

namespace jx::zone {

class KLuaScript {
public:
    using Arg = std::variant<double, std::string>;

    KLuaScript();
    ~KLuaScript();
    KLuaScript(const KLuaScript&) = delete;
    KLuaScript& operator=(const KLuaScript&) = delete;

    // KLuaScript::Init: a fresh state with the standard libraries and the engine functions
    // (Include, IncludeLib, print).  `root` is the folder that holds `script\`.
    bool init(const std::string& root);
    // KLuaScript::Load: runs the file (its functions become globals of this state).  Game paths
    // look like `\script\npclevelscript\animal.lua` and are resolved under root, lower-cased.
    bool load(const std::string& game_path);
    // Include(...) from Lua and from C++: the same state, the same resolution.
    bool include(const std::string& game_path);
    [[nodiscard]] bool has_function(const char* name) const;
    // CallFunction with numbers / strings in and one number out (nullopt when the function is
    // missing, raises an error or returns nothing numeric).
    std::optional<double> call_number(const char* name, const std::vector<Arg>& args);
    // CallFunction with one result read the way KSkill::LoadSkillLevelData reads it (jx_linux_y
    // 0x080EE4B0): a number when Lua_IsNumber says so (a string of digits counts), else the
    // string when Lua_IsString says so, else nullopt (also when the function is missing or fails).
    std::optional<Arg> call_value(const char* name, const std::vector<Arg>& args);
    // KLuaScript::LoadBuffer + ExecuteCode: runs a piece of code in this state (the GM's
    // `?gm ds Say("abc")`); the error text is returned through `error` when it fails.
    bool do_string(const std::string& code, const char* name, std::string* error = nullptr);

    [[nodiscard]] const std::string& file() const noexcept { return file_; }
    [[nodiscard]] const std::string& root() const noexcept { return root_; }
    [[nodiscard]] lua_State* state() const noexcept { return L_; }

    // `\script\a\b.lua` -> `<root>/script/a/b.lua` (lower-cased, forward slashes, UTF-8).
    static std::string resolve(const std::string& root, const std::string& game_path);
    // The on-disk path of a resolved UTF-8 path.  The old data names its folders in GBK; on Windows
    // those bytes sit in the file system through the ANSI code page, so the UTF-8 path is turned
    // back into GBK bytes and handed to the narrow (ANSI) path constructor.
    static std::filesystem::path os_path(const std::string& resolved_utf8);

private:
    bool run_file(const std::string& path, const char* what);

    lua_State* L_ = nullptr;
    std::string root_;
    std::string file_;
};

} // namespace jx::zone
