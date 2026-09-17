// jx_luacheck - loads every .lua under a folder with the same Lua 5.4 the zone embeds and reports
// the ones that do not parse.  This is the proof that the converted script tree is real Lua 5.4:
// the compatibility prelude is gone, so a leftover Lua 4 construct shows up here as a syntax error.
//
//   jx_luacheck <dir> [<dir>...] [--max-errors N] [--quiet]
//
// Exit code 0 when every file parses, 1 otherwise.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {

std::string lower_ext(const std::filesystem::path& p)
{
    const std::u8string ext8 = p.extension().u8string();
    std::string ext(ext8.begin(), ext8.end());
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

// The script folders are named in GBK, which the ANSI code page of this machine cannot represent,
// and path::string() throws on such a name.  u8string() always works.
std::string utf8_of(const std::filesystem::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

// Loads one file and reports whether it parses; `shown` limits how much noise goes to stderr.
bool check_one(lua_State* L, const std::filesystem::path& p, long long& bytes, bool show)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "jx_luacheck: cannot read %s\n", utf8_of(p).c_str());
        return false;
    }
    std::string code((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    bytes += static_cast<long long>(code.size());
    const std::string chunk = "@" + utf8_of(p);
    const bool ok = luaL_loadbuffer(L, code.data(), code.size(), chunk.c_str()) == LUA_OK;
    if (!ok && show) std::fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);   // the compiled chunk or the error message; never run it
    return ok;
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> dirs;
    int max_errors = 20;
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--max-errors" && i + 1 < argc) {
            max_errors = std::atoi(argv[++i]);
        } else if (a == "--quiet") {
            quiet = true;
        } else {
            dirs.push_back(a);
        }
    }
    if (dirs.empty()) {
        std::fprintf(stderr, "usage: jx_luacheck <dir> [<dir>...] [--max-errors N] [--quiet]\n");
        return 2;
    }

    lua_State* L = luaL_newstate();
    if (L == nullptr) {
        std::fprintf(stderr, "jx_luacheck: out of memory\n");
        return 2;
    }
    luaL_openlibs(L);

    long long checked = 0;
    long long failed = 0;
    long long bytes = 0;
    for (const std::string& dir : dirs) {
        std::error_code ec;
        const std::filesystem::path root(std::u8string(dir.begin(), dir.end()));
        if (!std::filesystem::exists(root, ec)) {
            std::fprintf(stderr, "jx_luacheck: no such path: %s\n", dir.c_str());
            lua_close(L);
            return 2;
        }
        if (std::filesystem::is_regular_file(root, ec)) {
            ++checked;
            if (!check_one(L, root, bytes, failed < max_errors)) ++failed;
            continue;
        }
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;
            if (!it->is_regular_file(ec) || lower_ext(it->path()) != ".lua") continue;
            ++checked;
            if (!check_one(L, it->path(), bytes, failed < max_errors)) ++failed;
        }
    }
    lua_close(L);

    if (!quiet || failed > 0) {
        std::printf("jx_luacheck: %lld scripts, %.1f MB, %lld failed\n",
                    checked, static_cast<double>(bytes) / (1024.0 * 1024.0), failed);
    }
    if (failed > max_errors) {
        std::fprintf(stderr, "... %lld more\n", failed - max_errors);
    }
    return failed == 0 ? 0 : 1;
}
