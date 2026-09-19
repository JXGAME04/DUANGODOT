// The tab files of the old server as its scripts read them (docs/LINUX-SERVER.md §24): KTabFile (Engine/Src/KTabFile.cpp
// of 2003: CreateTabOffset, GetValue, FindColumn; jx_linux_y 0x08227BE0 FindRow), the cache behind TabFile_Load 0x0814AEF0 /
// TabFile_UnLoad 0x0814B040 and the library TabFile_GetRowCount / GetColCount / GetCell / Search / SetCell / Save.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KTabFile.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KNpc;
using jx::zone::KScriptCache;
using jx::zone::KTabFile;
using jx::zone::KTabFileCache;

namespace {

// a header of three columns, a description row, three rows (one short, one with a fourth cell), an empty line and a
// last line without a line end
constexpr const char* kTable = "Id\tName\tLevel\r\nmo ta\tten\tcap\r\n1\tSoi\t5\r\n2\tGa\r\n3\tCao\t7\tthua\r\n\r\n4\tHo\t9";

constexpr const char* kScript = R"(
function main(param)
    g_load = TabFile_Load("\\settings\\test\\a.txt", "A")
    g_again = TabFile_Load("\\settings\\test\\a.txt", "A")
    g_other = TabFile_Load("\\settings\\test\\b.txt", "A")
    g_missing = TabFile_Load("\\settings\\test\\nothing.txt", "B")
    g_rows = TabFile_GetRowCount("A")
    g_cols = TabFile_GetColCount("A")
    g_cell = TabFile_GetCell("A", 3, 2)
    g_byname = TabFile_GetCell("A", 3, "Level")
    g_empty = TabFile_GetCell("A", 4, 3)
    g_out = TabFile_GetCell("A", 9, 1)
    g_nokey = TabFile_GetCell("Z", 1, 1)
    g_search = TabFile_Search("A", "Name", "Cao")
    g_searchnum = TabFile_Search("A", 1, "4")
    g_searchnone = TabFile_Search("A", "Name", "Meo")
    g_set = TabFile_SetCell("A", 3, 2, "Soi xam")
    g_set_read = TabFile_GetCell("A", 3, 2)
    g_save = TabFile_Save("A")
    g_unload = TabFile_UnLoad("A")
    g_unload2 = TabFile_UnLoad("A")
    g_after = TabFile_GetCell("A", 3, 2)
    g_rows_after = TabFile_GetRowCount("A")
end
function Str(v)
    if v == nil then return "nil" end
    return tostring(v)
end
function Is(name, expected)
    if Str(_G[name]) == expected then return 1 end
    return 0
end
)";

std::string make_files()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_tabfile_test";
    std::filesystem::create_directories(root / "settings" / "test");
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "settings" / "test" / "a.txt", std::ios::binary) << kTable;
    std::ofstream(root / "settings" / "test" / "b.txt", std::ios::binary) << "X\tY\n1\t2\n";
    std::ofstream(root / "script" / "test" / "tab.lua") << kScript;
    return root.string();
}

jx::zone::KSubWorldConfig tab_world(const std::string& root)
{
    jx::zone::KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.view_cells = 1;
    c.interest_period = 1;
    c.view_slack = 0;
    c.far_period = 1;
    c.spawn_point = jx::zone::Pos{2000, 2000};
    c.seed = 7;
    c.player_set = std::make_shared<jx::zone::KPlayerSet>();
    c.scripts = std::make_shared<KScriptCache>(root);
    jx::zone::g_TabFiles().set_roots("/nowhere;" + root);
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
    r.set_level(10);
    auto* s = r.mutable_stats();
    s->set_strength(35);
    s->set_dexterity(25);
    s->set_vitality(25);
    s->set_energy(15);
    s->set_hp_max(204);
    s->set_hp(204);
    s->set_mp_max(100);
    s->set_mp(100);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(x);
    r.mutable_position()->mutable_pos()->set_y(y);
    return r;
}

} // namespace

TEST_CASE("KTabFile: the header's width, every line a row, the cells cut to the width, the empty ones missing", "[tabfile]")
{
    KTabFile t;
    t.parse(kTable);
    CHECK(t.width() == 3);
    CHECK(t.height() == 7);   // header, description, 3 rows, the empty line, the last line without a line end
    std::string s;
    CHECK(t.get_string(1, 1, s));
    CHECK(s == "Id");
    CHECK(t.get_string(3, 2, s));
    CHECK(s == "Soi");
    CHECK(t.get_string(5, 3, s));
    CHECK(s == "7");                 // the fourth cell of that line is dropped
    CHECK_FALSE(t.get_string(4, 3, s));   // the short row: an empty cell is "not there"
    CHECK_FALSE(t.get_string(6, 1, s));   // the empty line
    CHECK(t.get_string(7, 2, s));
    CHECK(s == "Ho");
    CHECK_FALSE(t.get_string(8, 1, s));
    CHECK_FALSE(t.get_string(0, 1, s));
    CHECK_FALSE(t.get_string(1, 4, s));
    // a cut of the copy
    CHECK(t.get_string(3, 2, s, 2));
    CHECK(s == "So");
    // FindColumn: the header cell cut to the name's length (the 2003 quirk: "Lev" finds "Level")
    CHECK(t.find_column("Name") == 2);
    CHECK(t.find_column("Lev") == 3);
    CHECK(t.find_column("Levels") == -1);
    CHECK(t.find_column("Nobody") == -1);
    // FindRow: the whole cell of the column
    CHECK(t.find_row(2, "Cao") == 5);
    CHECK(t.find_row(2, "Ca") == -1);
    CHECK(t.find_row("Name", "Ho") == 7);
    CHECK(t.find_row("Id", "1") == 3);
    CHECK(t.find_row(9, "x") == -1);
    CHECK(t.find_row("Nobody", "x") == -1);
    // SetValue in this copy
    CHECK(t.set_string(3, 2, "Soi xam"));
    CHECK(t.get_string(3, 2, s));
    CHECK(s == "Soi xam");
    CHECK_FALSE(t.set_string(9, 1, "x"));
    // LF only, and a file that is only a header
    KTabFile u;
    u.parse("A\tB\n1\t2\n3\n");
    CHECK(u.width() == 2);
    CHECK(u.height() == 3);
    KTabFile v;
    v.parse("A\tB\tC");
    CHECK(v.width() == 3);
    CHECK(v.height() == 1);
    KTabFile w;
    w.parse("");
    CHECK(w.width() == 1);
    CHECK(w.height() == 1);
}

TEST_CASE("KTabFileCache: the game path in the roots, one table a key, the same path again kept, another refused", "[tabfile]")
{
    const std::string root = make_files();
    KTabFileCache cache("/nowhere;" + root);
    CHECK(cache.roots().size() == 2);
    CHECK(cache.resolve(R"(\settings\test\a.txt)") == (std::filesystem::path(root) / "settings/test/a.txt").string());
    CHECK(cache.resolve("/settings/test/a.txt") == (std::filesystem::path(root) / "settings/test/a.txt").string());
    CHECK(cache.resolve("settings/test/a.txt") == (std::filesystem::path(root) / "settings/test/a.txt").string());
    CHECK(cache.resolve("settings/test/none.txt").empty());
    CHECK(cache.resolve("").empty());
    CHECK(cache.load(R"(\settings\test\a.txt)", "A", false) == 1);
    CHECK(cache.load(R"(\settings\test\a.txt)", "A", false) == 1);
    CHECK(cache.load(R"(\settings\test\b.txt)", "A", false) == 0);
    CHECK(cache.load(R"(\settings\test\none.txt)", "N", false) == 0);
    CHECK(cache.load("", "E", false) == 0);
    CHECK(cache.load(R"(\settings\test\b.txt)", "", false) == 0);
    CHECK(cache.size() == 1);
    REQUIRE(cache.find("A") != nullptr);
    CHECK(cache.find("A")->height() == 7);
    CHECK(cache.find("B") == nullptr);
    CHECK(cache.load(R"(\settings\test\b.txt)", "B", true) == 1);
    CHECK(cache.size() == 2);
    CHECK(cache.unload("A"));
    CHECK_FALSE(cache.unload("A"));
    CHECK(cache.find("A") == nullptr);
    CHECK(cache.size() == 1);
    KTabFileCache none("");
    CHECK(none.load(R"(\settings\test\a.txt)", "A", false) == 0);
}

TEST_CASE("the TabFile_* library of the scripts", "[tabfile][world]")
{
    const std::string root = make_files();
    jx::log::Options o;
    o.console = false;
    o.default_level = jx::log::Level::warn;
    jx::log::init(o);
    jx::zone::KSubWorld w(tab_world(root));
    jx::EntityId a;
    jx::zone::Pos at;
    REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
    KNpc* p = w.mutable_entity(a);
    REQUIRE(p != nullptr);
    REQUIRE(w.execute_script(R"(\script\test\tab.lua)", "main", *p, 0));
    jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\tab.lua)");
    REQUIRE(s != nullptr);
    const auto is = [&](const char* name, const char* expected) {
        return s->call_number("Is", {std::string(name), std::string(expected)}) == 1.0;
    };
    CHECK(is("g_load", "1"));
    CHECK(is("g_again", "1"));
    CHECK(is("g_other", "0"));
    CHECK(is("g_missing", "0"));
    CHECK(is("g_rows", "7"));
    CHECK(is("g_cols", "3"));
    CHECK(is("g_cell", "Soi"));
    CHECK(is("g_byname", "5"));
    CHECK(is("g_empty", ""));      // GetString's default ""
    CHECK(is("g_out", ""));
    CHECK(is("g_nokey", ""));      // no table under the key: "" (0x0814A8A0)
    CHECK(is("g_search", "5"));
    CHECK(is("g_searchnum", "7"));
    CHECK(is("g_searchnone", "-1"));
    CHECK(is("g_set", "1"));
    CHECK(is("g_set_read", "Soi xam"));
    CHECK(is("g_save", "0"));      // the zone never writes the old server's files back
    CHECK(is("g_unload", "1"));
    CHECK(is("g_unload2", "0"));
    CHECK(is("g_after", ""));
    CHECK(is("g_rows_after", "0"));
    CHECK(jx::zone::g_TabFiles().size() == 0);
}
