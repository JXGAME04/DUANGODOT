// KLuaScript of the old engine (Engine/Src/KLuaScript.h) on standard Lua 5.4: one interpreter
// state per script file, Load / Include / CallFunction.  The old scripts are Lua 4.0 and run
// unchanged thanks to a small compatibility prelude (getn, strfind, floor, mod ... as globals).
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

    // KLuaScript::Init: a fresh state with the standard libraries, the Lua 4 prelude and the
    // engine functions (Include, print).  `root` is the old server folder that holds `script\`.
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
