// KLuaScript / KScriptCache on Lua 5.4 running Lua 4 style level scripts, and
// KNpcTemplateSet::level_data following KNpcTemplate::InitNpcLevelData.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

#include "jx/log.hpp"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KScriptCache.h"

using jx::zone::KLuaScript;
using jx::zone::KNpcLevelData;
using jx::zone::KNpcTemplate;
using jx::zone::KNpcTemplateSet;
using jx::zone::KScriptCache;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
    }
};

// A tiny level script shaped like the converted ones: real Lua 5.4 (string.find, math.floor, #),
// Include, "a|b" parameter cells, GetNpcLevelData / GetNpcKeyData.  This is what
// services/pkg/jxlua produces from the old Lua 4 sources.
constexpr const char* kLevelScript = R"lua(
Include("\\script\\npclevelscript\\lib.lua");
IncludeLib("Core");

function GetParam(strParam, index)
nLastBegin = 1
for i=1, index - 1 do
nBegin = string.find(strParam, "|", nLastBegin)
nLastBegin = nBegin + 1
end;
strnum = string.sub(strParam, nLastBegin)
nEnd = string.find(strnum, "|")
if nEnd == nil then
return strnum
end
return string.sub(strnum,1,nEnd -1);
end;

function GetData(Level, Param1, Param2)
result = Param2 * Level + Param1;
return math.floor(result);
end;

function GetNpcLevelData(Series, Level, StyleName, ParamStr)
	if(StyleName=="PhysicsResist") then
		return 7
	end
	if(ParamStr=="") then
		return 1
	end
Param1 = GetParam(ParamStr,1);
Param2 = GetParam(ParamStr,2);
return GetData(Level, Param1, Param2);
end;

function GetNpcKeyData(Series, Level, StyleName, Param1, Param2, Param3)
if (StyleName == "Life") then
return 4*Quadratic(Level, Param1, Param2, Param3);
end;
if (StyleName == "Exp") then
return math.floor(#({1,2,3}) * Level * 1.5);
end;
print("fallthrough", StyleName);
result = Param1 * Level * Level + Param2 * Level + Param3;
return result;
end;
)lua";

constexpr const char* kLib = R"lua(
--二次函数，取整y=ax^2+bx+c
function Quadratic(x,a,b,c)
	return math.floor(a*x*x+b*x+c);
end;
)lua";

// A level script that builds its answer as a string the way zhengjiakangxing.lua and hundreds of others do
// (Param2String): Lua 4 printed 15 + level as "16", Lua 5.4 prints 15 + 1.0 as "16.0" and 10 / 5 as "2.0"
constexpr const char* kStrings = R"lua(
function Param2String(Param1, Param2, Param3)
return Param1..","..Param2..","..Param3
end;
function GetSkillLevelData(levelname, data, level)
if (levelname == "allres_p") then
return Param2String(15+level, 12, 0)
end;
if (levelname == "half") then
return Param2String(level/2, level*10/5, 1.5*level)
end;
if (levelname == "name") then
return "1108.0,"..level..",skill_v1.0"
end;
return ""
end;
)lua";

// Untouched Lua 4: %upvalue does not parse, and getn is not a global of Lua 5.4.
constexpr const char* kLua4 = R"lua(
local tb = {1,2}
function Count() return getn(%tb) end
)lua";

// Writes the scripts under <tmp>/script/npclevelscript and returns the root.
std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_lua_test";
    std::filesystem::create_directories(root / "script" / "npclevelscript");
    std::ofstream(root / "script" / "npclevelscript" / "npclevelscript.lua") << kLevelScript;
    std::ofstream(root / "script" / "npclevelscript" / "lib.lua") << kLib;
    std::ofstream(root / "script" / "npclevelscript" / "broken.lua") << "function ( oops";
    std::ofstream(root / "script" / "npclevelscript" / "lua4.lua") << kLua4;
    std::ofstream(root / "script" / "npclevelscript" / "strings.lua") << kStrings;
    return root.generic_string();
}

// The converted tree, when `python tools/dev.py lua` has been run on this machine.
std::string converted_root(const char* name)
{
    const std::filesystem::path p = std::filesystem::path(JX_NEXT_DIR) / "data" / "script" / name;
    return p.generic_string();
}

} // namespace

TEST_CASE("game paths resolve under the script root, lower-cased", "[lua]")
{
    CHECK(KLuaScript::resolve("D:/srv", R"(\Script\NpcLevelScript\Animal.lua)") == "D:/srv/script/npclevelscript/animal.lua");
    CHECK(KLuaScript::resolve("/srv", "script/x.lua") == "/srv/script/x.lua");
}

TEST_CASE("a converted level script runs on plain Lua 5.4 with Include", "[lua]")
{
    Quiet q;
    const std::string root = make_scripts();
    KLuaScript s;
    REQUIRE(s.init(root));
    REQUIRE(s.load(R"(\script\npclevelscript\npclevelscript.lua)"));
    CHECK(s.has_function("GetNpcKeyData"));
    CHECK(s.has_function("Quadratic"));   // from the Include
    CHECK_FALSE(s.has_function("Nothing"));

    // "1|0" -> 1, "0|10" -> 10 * level (strfind / strsub / floor of the old dialect)
    CHECK(s.call_number("GetNpcLevelData", {0.0, 19.0, std::string("Level1"), std::string("1|0")}) == 1.0);
    CHECK(s.call_number("GetNpcLevelData", {0.0, 19.0, std::string("Level2"), std::string("0|10")}) == 190.0);
    CHECK(s.call_number("GetNpcLevelData", {0.0, 19.0, std::string("PhysicsResist"), std::string("5|0")}) == 7.0);
    // Life = 4 * Quadratic(10, 1, 2, 3) = 4 * 123; Exp uses getn; "AR" falls through to the quadratic
    CHECK(s.call_number("GetNpcKeyData", {0.0, 10.0, std::string("Life"), 1.0, 2.0, 3.0}) == 492.0);
    CHECK(s.call_number("GetNpcKeyData", {0.0, 10.0, std::string("Exp"), 0.0, 0.0, 0.0}) == 45.0);
    CHECK(s.call_number("GetNpcKeyData", {0.0, 10.0, std::string("AR"), 0.07, 2.5, 10.0}) == 42.0);
    CHECK_FALSE(s.call_number("Nothing", {}).has_value());

    KLuaScript bad;
    REQUIRE(bad.init(root));
    CHECK_FALSE(bad.load(R"(\script\npclevelscript\broken.lua)"));
    CHECK_FALSE(bad.load(R"(\script\npclevelscript\missing.lua)"));
}

TEST_CASE("an unconverted Lua 4 script is rejected: there is no compatibility layer", "[lua]")
{
    Quiet q;
    const std::string root = make_scripts();
    KLuaScript s;
    REQUIRE(s.init(root));
    CHECK_FALSE(s.load(R"(\script\npclevelscript\lua4.lua)"));   // %upvalue does not parse
    CHECK_FALSE(s.has_function("getn"));
    CHECK_FALSE(s.has_function("strfind"));
    CHECK_FALSE(s.has_function("floor"));
    CHECK(s.has_function("IncludeLib"));   // the engine's own function, kept as a no-op
}

TEST_CASE("the script cache loads once and remembers failures", "[lua]")
{
    Quiet q;
    KScriptCache cache(make_scripts());
    KLuaScript* a = cache.get(R"(\script\npclevelscript\npclevelscript.lua)");
    REQUIRE(a != nullptr);
    CHECK(cache.get("/Script/NpcLevelScript/NPCLEVELSCRIPT.lua") == a);
    CHECK(cache.get(R"(\script\npclevelscript\missing.lua)") == nullptr);
    CHECK(cache.get(R"(\script\npclevelscript\missing.lua)") == nullptr);
    CHECK(cache.size() == 2);
}

TEST_CASE("level_data follows KNpcTemplate::InitNpcLevelData", "[lua][template]")
{
    Quiet q;
    KScriptCache cache(make_scripts());
    KNpcTemplate t;
    t.id = 12;
    t.life_param = 100;
    t.min_damage = 1;
    t.max_damage = 3;
    t.skills[1].id = 53;
    t.skills[1].level_a = 1;
    t.skills[2].id = 197;
    t.skills[2].level_a = 0;
    t.skills[2].level_b = 10;
    t.cells = {{"ExpParam", "50"}, {"LifeParam", "100"}, {"LifeParam1", "1"}, {"LifeParam2", "2"}, {"LifeParam3", "3"},
               {"ARParam", "100"}, {"ARParam1", "0.07"}, {"ARParam2", "2.5"}, {"ARParam3", "10"},
               {"DefenseParam", "200"}, {"DefenseParam1", "1"}, {"DefenseParam2", "0"}, {"DefenseParam3", "0"},
               {"MinDamageParam", "100"}, {"MinDamageParam1", "0"}, {"MinDamageParam2", "1"}, {"MinDamageParam3", "0"},
               {"MaxDamageParam", "100"}, {"MaxDamageParam1", "0"}, {"MaxDamageParam2", "2"}, {"MaxDamageParam3", "0"},
               {"LifeReplenish", "0|0.5"}, {"PhysicsResist", "x"}, {"Level1", "1|0"}, {"Level2", "0|10"}};

    // no script cache: the placeholders of the raw columns
    const KNpcLevelData p = KNpcTemplateSet::level_data(t, 10, 0, nullptr);
    CHECK_FALSE(p.from_script);
    CHECK(p.life_max == 1000);
    CHECK(p.skill_level[1] == 1);
    CHECK(p.skill_level[2] == 100);

    // the (default) level script: every number through GetNpcKeyData / GetNpcLevelData
    const KNpcLevelData d = KNpcTemplateSet::level_data(t, 10, 0, &cache);
    REQUIRE(d.from_script);
    CHECK(d.life_max == 492);          // 100 % of 4 * Quadratic(10, 1, 2, 3)
    CHECK(d.exp == 22);                // 50 % of 45, truncated like the old int member
    CHECK(d.attack_rating == 42);      // "AR" -> the quadratic fallthrough
    CHECK(d.defend == 200);            // 200 % of (1 * 100 + 0 + 0)
    CHECK(d.min_damage == 10);         // 0 * 100 + 1 * 10 + 0
    CHECK(d.max_damage == 20);
    CHECK(d.life_replenish == 5);      // "0|0.5" -> floor(0.5 * 10 + 0)
    CHECK(d.physics_resist == 7);
    CHECK(d.fire_resist == 0);         // empty cell: no call, 0
    CHECK(d.skill_level[1] == 1);
    CHECK(d.skill_level[2] == 100);
    CHECK(d.skill_level[3] == 0);

    // a template naming a missing script falls back to the default one
    t.level_script = R"(\script\npclevelscript\nothere.lua)";
    CHECK(KNpcTemplateSet::level_data(t, 10, 0, &cache).life_max == 492);
}

TEST_CASE("the converted level scripts of the reference server run when present", "[lua][data]")
{
    Quiet q;
    const std::string root = converted_root("server1");
    if (!std::filesystem::exists(root + "/script/npclevelscript/animal.lua")) {
        SUCCEED("no converted scripts here: run python tools/dev.py lua");
        return;
    }
    KScriptCache cache(root);
    KLuaScript* s = cache.get(R"(\script\npclevelscript\animal.lua)");
    REQUIRE(s != nullptr);
    // Heo rừng (npcs.txt row 12): Life 100 | 0.5 | 20 | 50 at level 19 through animal.lua
    KNpcTemplate t;
    t.level_script = R"(\script\npclevelscript\animal.lua)";
    t.cells = {{"LifeParam", "100"}, {"LifeParam1", "0.5"}, {"LifeParam2", "20"}, {"LifeParam3", "50"},
               {"ARParam", "100"}, {"ARParam1", "0.07"}, {"ARParam2", "2.5"}, {"ARParam3", "10"},
               {"MinDamageParam", "100"}, {"MaxDamageParam", "100"}, {"Level1", "1|0"}};
    t.skills[1].id = 53;
    const KNpcLevelData d = KNpcTemplateSet::level_data(t, 19, 0, &cache);
    REQUIRE(d.from_script);
    CHECK(d.life_max > 0);
    CHECK(d.attack_rating > 0);
    CHECK(d.max_damage >= d.min_damage);
    CHECK(d.skill_level[1] == 1);
    CHECK(cache.get(KScriptCache::kNpcLevelScript) != nullptr);
}


TEST_CASE("whole numbers reach the scripts as integers and come back printed the Lua 4 way", "[lua]")
{
    Quiet q;
    const std::string root = make_scripts();
    KLuaScript s;
    REQUIRE(s.init(root));
    REQUIRE(s.load(R"(\script\npclevelscript\strings.lua)"));
    auto str = [&](const char* setting, double level) {
        const auto r = s.call_value("GetSkillLevelData", {std::string(setting), std::string(), level});
        REQUIRE(r);
        REQUIRE(std::holds_alternative<std::string>(*r));
        return std::get<std::string>(*r);
    };
    CHECK(str("allres_p", 1.0) == "16,12,0");          // 15 + level with an integer level, not "16.0,12,0"
    CHECK(str("half", 4.0) == "2,8,6");               // 4 / 2 and 4 * 10 / 5 are floats in 5.4: "2.0" / "8.0" -> "2" / "8"
    CHECK(str("half", 3.0) == "1.5,6,4.5");           // a fraction stays
    CHECK(str("name", 2.0) == "1108,2,skill_v1.0");   // a ".0" inside a word is left alone
    CHECK(KLuaScript::lua4_number_format("0.0 -7.0 10.05 3.0x 3.00") == "0 -7 10.05 3x 3.00");   // 3.0 .. "x" was "3x" in Lua 4 too
}
