#include "jx/zone/KLuaScript.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "jx/log.hpp"
#include "jx/zone/ScriptFuns.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace jx::zone {
namespace {

// What Lua 4.0 had as globals and Lua 5 moved into libraries: the old scripts call these bare.
constexpr const char* kLua4Prelude = R"lua(
getn = function(t) return #t end
floor = math.floor ceil = math.ceil sqrt = math.sqrt abs = math.abs min = math.min max = math.max
mod = math.fmod random = math.random randomseed = math.randomseed
strfind = string.find strsub = string.sub strlen = string.len strlower = string.lower strupper = string.upper
strrep = string.rep strbyte = string.byte strchar = string.char format = string.format gsub = string.gsub
tinsert = table.insert tremove = table.remove sort = table.sort
IncludeLib = function(name) end
)lua";

constexpr const char* kSelfKey = "jx.KLuaScript";

KLuaScript* self_of(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, kSelfKey);
    auto* self = static_cast<KLuaScript*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return self;
}

// Include("\\script\\x.lua"): loads the file into the calling state.
int l_include(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    KLuaScript* self = self_of(L);
    if (self == nullptr || !self->include(path)) {
        log::warn("lua", "Include failed", {log::kv("file", self ? self->file() : ""), log::kv("include", path)});
    }
    return 0;
}

// print(...) goes to the structured log instead of stdout.
int l_print(lua_State* L)
{
    std::ostringstream out;
    const int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        std::size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) out << '\t';
        out.write(s, static_cast<std::streamsize>(len));
        lua_pop(L, 1);
    }
    KLuaScript* self = self_of(L);
    log::info("lua", "print", {log::kv("file", self ? self->file() : ""), log::kv("text", out.str())});
    return 0;
}

} // namespace

KLuaScript::KLuaScript() = default;

KLuaScript::~KLuaScript()
{
    if (L_ != nullptr) lua_close(L_);
}

bool KLuaScript::init(const std::string& root)
{
    if (L_ != nullptr) lua_close(L_);
    root_ = root;
    L_ = luaL_newstate();
    if (L_ == nullptr) return false;
    luaL_openlibs(L_);
    lua_pushlightuserdata(L_, this);
    lua_setfield(L_, LUA_REGISTRYINDEX, kSelfKey);
    lua_pushcfunction(L_, l_include);
    lua_setglobal(L_, "Include");
    lua_pushcfunction(L_, l_print);
    lua_setglobal(L_, "print");
    RegisterGameScriptFuns(L_);   // KLuaScript::RegisterFunctions(GameScriptFuns)
    if (luaL_dostring(L_, kLua4Prelude) != LUA_OK) {
        log::error("lua", "prelude failed", {log::kv("error", lua_tostring(L_, -1))});
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

std::string KLuaScript::resolve(const std::string& root, const std::string& game_path)
{
    std::string p = game_path;
    for (char& c : p) {
        if (c == '\\') c = '/';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    while (!p.empty() && p.front() == '/') p.erase(p.begin());
    return (std::filesystem::path(root) / p).generic_string();
}

std::filesystem::path KLuaScript::os_path(const std::string& resolved_utf8)
{
#ifdef _WIN32
    // UTF-8 -> UTF-16 -> GBK (code page 936); a path that is not GBK-encodable stays as it is
    const int wlen = MultiByteToWideChar(CP_UTF8, 0, resolved_utf8.data(), static_cast<int>(resolved_utf8.size()), nullptr, 0);
    if (wlen <= 0) return std::filesystem::path(resolved_utf8);
    std::wstring wide(static_cast<std::size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, resolved_utf8.data(), static_cast<int>(resolved_utf8.size()), wide.data(), wlen);
    BOOL lost = FALSE;
    const int glen = WideCharToMultiByte(936, 0, wide.data(), wlen, nullptr, 0, nullptr, &lost);
    if (glen <= 0 || lost) return std::filesystem::path(wide);
    std::string gbk(static_cast<std::size_t>(glen), '\0');
    WideCharToMultiByte(936, 0, wide.data(), wlen, gbk.data(), glen, nullptr, nullptr);
    return std::filesystem::path(gbk);   // narrow = the ANSI code page: the bytes the folders were created with
#else
    return std::filesystem::path(resolved_utf8);
#endif
}

bool KLuaScript::run_file(const std::string& path, const char* what)
{
    if (L_ == nullptr) return false;
    std::ifstream in(os_path(path), std::ios::binary);
    if (!in) {
        log::warn("lua", "script not found", {log::kv("what", what), log::kv("path", path)});
        return false;
    }
    std::string code((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string chunk = "@" + path;
    if (luaL_loadbuffer(L_, code.data(), code.size(), chunk.c_str()) != LUA_OK || lua_pcall(L_, 0, 0, 0) != LUA_OK) {
        log::error("lua", "script failed", {log::kv("what", what), log::kv("path", path), log::kv("error", lua_tostring(L_, -1))});
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

bool KLuaScript::load(const std::string& game_path)
{
    file_ = game_path;
    return run_file(resolve(root_, game_path), "Load");
}

bool KLuaScript::include(const std::string& game_path)
{
    return run_file(resolve(root_, game_path), "Include");
}

bool KLuaScript::has_function(const char* name) const
{
    if (L_ == nullptr) return false;
    lua_getglobal(L_, name);
    const bool ok = lua_isfunction(L_, -1);
    lua_pop(L_, 1);
    return ok;
}

std::optional<double> KLuaScript::call_number(const char* name, const std::vector<Arg>& args)
{
    if (L_ == nullptr) return std::nullopt;
    const int top = lua_gettop(L_);   // SafeCallBegin
    lua_getglobal(L_, name);
    if (!lua_isfunction(L_, -1)) {
        lua_settop(L_, top);
        return std::nullopt;
    }
    for (const Arg& a : args) {
        if (const auto* n = std::get_if<double>(&a)) {
            lua_pushnumber(L_, *n);
        } else {
            const auto& s = std::get<std::string>(a);
            lua_pushlstring(L_, s.data(), s.size());
        }
    }
    if (lua_pcall(L_, static_cast<int>(args.size()), 1, 0) != LUA_OK) {
        log::warn("lua", "call failed", {log::kv("file", file_), log::kv("function", name), log::kv("error", lua_tostring(L_, -1))});
        lua_settop(L_, top);
        return std::nullopt;
    }
    std::optional<double> result;
    int isnum = 0;
    const double v = lua_tonumberx(L_, -1, &isnum);   // Lua_ValueToNumber: strings of digits count too
    if (isnum) result = v;
    lua_settop(L_, top);   // SafeCallEnd
    return result;
}

} // namespace jx::zone
