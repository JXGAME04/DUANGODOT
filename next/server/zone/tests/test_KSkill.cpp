// KSkill / KSkillManager: the skill table and the numbers per level the way the JX2 server
// makes them (jx_linux_y: KSkillManager::Init 0x080E7200, KSkill::GetInfoFromTabFile 0x080E9200,
// InstanceSkill 0x080E6E10, LoadSkillLevelData 0x080EE4B0, ParseString2MagicAttrib 0x080EE380,
// AddMagicAttrib 0x080EDCC0 - docs/LINUX-SERVER.md §11).
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "jx/log.hpp"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"

using jx::zone::KMagicAttrib;
using jx::zone::KScriptCache;
using jx::zone::KSkill;
using jx::zone::KSkillManager;
using jx::zone::KSkillRow;
using jx::zone::KSkillTable;
using jx::zone::magic_attrib_id;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::error;
        jx::log::init(o);
    }
};

// A level script in the shape of the real ones (script\skill\*.lua after `dev.py lua`):
// GetSkillLevelData answers with a number, a string of digits, or "v1,v2,v3".
constexpr const char* kLevelScript = R"lua(
function GetSkillLevelData(levelname, data, level)
    if data ~= "t_skill" then return "" end
    if levelname == "skill_cost_v" then return 3 + level end
    if levelname == "physicsenhance_p" then return "100" end
    if levelname == "addphysicsdamage_p" then return "25,-1,2" end
    if levelname == "lifemax_v" then return "50,0,0" end
    if levelname == "missle_speed_v" then return "18" end
    if levelname == "stopper" then return nil end
    if levelname == "attackrating_p" then return "7,0,0" end
    return ""
end
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_skill_test";
    std::filesystem::create_directories(root / "script" / "skill");
    std::ofstream(root / "script" / "skill" / "t.lua") << kLevelScript;
    return root.generic_string();
}

KMagicAttrib attrib(int type, int v0, int v1 = 0, int v2 = 0)
{
    KMagicAttrib m;
    m.type = type;
    m.value = {v0, v1, v2};
    return m;
}

// the converted script trees of this machine (python tools/dev.py lua), "" when none
std::string converted_roots()
{
    std::string roots;
    for (const char* name : {"server1", "binserver"}) {
        const std::filesystem::path p = std::filesystem::path(JX_NEXT_DIR) / "data" / "script" / name;
        if (std::filesystem::exists(p / "script")) {
            if (!roots.empty()) roots += ";";
            roots += p.generic_string();
        }
    }
    return roots;
}

} // namespace

TEST_CASE("KSG_StringGetInt / SkipSymbol read v1,v2,v3 the way the binary does", "[skill]")
{
    CHECK(KSkill::parse_values("25,-1,2") == std::array<int, 3>{25, -1, 2});
    CHECK(KSkill::parse_values("\"7,8,9") == std::array<int, 3>{7, 8, 9});   // the leading '"' is skipped
    CHECK(KSkill::parse_values("100") == std::array<int, 3>{100, 0, 0});
    CHECK(KSkill::parse_values(" -4 , 5") == std::array<int, 3>{-4, 5, 0});
    CHECK(KSkill::parse_values("25.0,3") == std::array<int, 3>{25, 0, 0});    // '.' is not ',': the rest stays 0
    CHECK(KSkill::parse_values("abc") == std::array<int, 3>{0, 0, 0});
    CHECK(KSkill::parse_values("") == std::array<int, 3>{0, 0, 0});
}

TEST_CASE("AddMagicAttrib puts every id where 0x080EDCC0 puts it", "[skill]")
{
    KSkill s;
    s.row.id = 77;
    // skill parameters 1..12 write fields
    s.add_attrib(attrib(magic_attrib_id("skill_cost_v"), 12));
    s.add_attrib(attrib(magic_attrib_id("skill_param2_v"), 5, 9));   // nValue[1], the old quirk
    s.add_attrib(attrib(magic_attrib_id("skill_appendskill"), 1230, 3));
    s.add_attrib(attrib(magic_attrib_id("skill_appendskill"), 0, 3));   // 0 is not appended
    s.add_attrib(attrib(magic_attrib_id("skill_waittime"), 6));
    CHECK(s.row.cost == 12);
    CHECK(s.row.param2 == 9);
    CHECK(s.row.wait_time == 6);
    REQUIRE(s.append_skills.size() == 1);
    CHECK(s.append_skills[0] == std::pair<int, int>{1230, 3});
    // 304..322
    s.add_attrib(attrib(304, 321, 0, 61));   // addskilldamage1: {v1, v3, v2}
    s.add_attrib(attrib(310, 54));           // skill_attackradius
    s.add_attrib(attrib(311, 1, 0, 900));    // skill_startevent: flag from v1 > 0, skill id from v3
    s.add_attrib(attrib(315, 80));           // skill_dohurt
    CHECK(s.add_skill_damage[0].skill_id == 321);
    CHECK(s.add_skill_damage[0].value == 61);
    CHECK(s.add_skill_damage[0].param == 0);
    CHECK(s.row.attack_radius == 54);
    CHECK(s.row.start_event);
    CHECK(s.row.start_skill_id == 900);
    CHECK(s.row.do_hurt == 80);
    // the missle list
    s.add_attrib(attrib(16, 18));     // missle_speed_v
    s.add_attrib(attrib(325, 300));   // missle_range
    REQUIRE(s.missle_attrib_count == 2);
    CHECK(s.missle_attribs[0].type == 16);
    CHECK(s.missle_attribs[1].type == 325);
    // damage attributes at fixed slots
    s.add_attrib(attrib(56, 10));            // attackrating_v -> slot 0
    s.add_attrib(attrib(57, 20));            // attackrating_p -> slot 0 again
    s.add_attrib(attrib(59, 30, 0, 40));     // physicsdamage_v -> slot 2
    s.add_attrib(attrib(65, 100));           // physicsenhance_p -> slot 2 again
    s.add_attrib(attrib(73, 0, 0, 5));       // addskillexp1 -> slot 15, own id when v1 == 0, not counted
    s.add_attrib(attrib(75, 30, 1, 1));      // seriesdamage_p -> slot 17, only v1, not counted
    s.add_attrib(attrib(76, 9));             // damage_reserve4: dropped
    CHECK(s.damage_attrib_count == 4);
    CHECK(s.damage_attribs[0].type == 57);
    CHECK(s.damage_attribs[0].value[0] == 20);
    CHECK(s.damage_attribs[2].type == 65);
    CHECK(s.damage_attribs[2].value[0] == 100);
    CHECK(s.damage_attribs[15].type == 73);
    CHECK(s.damage_attribs[15].value == std::array<int, 3>{77, 0, 5});
    CHECK(s.damage_attribs[17].type == 75);
    CHECK(s.damage_attribs[17].value == std::array<int, 3>{30, 0, 0});
    CHECK(s.damage_attribs[1].type == 0);
    // everything else: nValue[1] decides between immediate and state
    s.add_attrib(attrib(magic_attrib_id("life_v"), 10));          // immediate
    s.add_attrib(attrib(magic_attrib_id("life_v"), 10, 5));       // state
    s.add_attrib(attrib(magic_attrib_id("weapondamagemin_v"), 4, 0, 6));   // an item id through a skill: immediate, nValue[1] = 0
    s.add_attrib(attrib(magic_attrib_id("addphysicsdamage_p"), 25, -1, 2));
    REQUIRE(s.immediate_attrib_count == 2);
    REQUIRE(s.state_attrib_count == 2);
    CHECK(s.immediate_attribs[0].value == std::array<int, 3>{10, 0, 0});
    CHECK(s.immediate_attribs[1].type == magic_attrib_id("weapondamagemin_v"));
    CHECK(s.immediate_attribs[1].value == std::array<int, 3>{4, 0, 6});
    CHECK(s.state_attribs[0].value == std::array<int, 3>{10, 5, 0});
    CHECK(s.state_attribs[1].type == magic_attrib_id("addphysicsdamage_p"));
    CHECK(s.state_attribs[1].value == std::array<int, 3>{25, -1, 2});
    // the name map
    CHECK(magic_attrib_id("lifemax_v") == jx::zone::magic_lifemax_v);
    CHECK(magic_attrib_id("nothing_like_this") == -1);
    CHECK_FALSE(s.parse_string_to_magic_attrib("skill_desc", "1,2,3"));   // 318 is refused
    CHECK_FALSE(s.parse_string_to_magic_attrib("", "1,2,3"));
    CHECK_FALSE(s.parse_string_to_magic_attrib("bogus", "1,2,3"));
    CHECK(s.parse_string_to_magic_attrib("manamax_v", "\"9,0,0"));
    CHECK(s.immediate_attrib_count == 3);
    CHECK(s.immediate_attribs[2].type == magic_attrib_id("manamax_v"));
}

TEST_CASE("GetInfoFromTabFile reads the cells with the binary's defaults", "[skill]")
{
    std::unordered_map<std::string, std::string> cells{
        {"SkillName", "Thử"}, {"SkillId", "4"}, {"SkillStyle", "3"}, {"ReqLevel", "10"}, {"EqtLimit", "2"},
        {"MisslesForm", "7"}, {"TargetEnemy", "1"}, {"TargetSelf", "1"}, {"TargetNoNpc", "1"},
        {"LvlSetScript", "\\Script\\Skill\\Shaolin.lua"}, {"LvlSetting1", "addphysicsdamage_p"}, {"LvlData1", "shaolin_gunfa"},
        {"LvlData4", "shaolin_gunfa"}, {"CostValue", "2x"},
    };
    const KSkillRow r = KSkillRow::from_cells(cells);
    CHECK(r.id == 4);
    CHECK(r.style == 3);
    CHECK(r.req_level == 10);
    CHECK(r.eqt_limit == 2);
    CHECK(r.missles_form == 7);
    CHECK(r.attack_radius == 50);   // an empty cell is the default 0x32
    CHECK(r.do_hurt == 100);        // default 0x64
    CHECK(r.cost == 2);             // strtol
    CHECK(r.relation == (jx::zone::skill_relation_enemy | jx::zone::skill_relation_self | jx::zone::skill_relation_no_npc));
    CHECK(r.level_set_script == "\\script\\skill\\shaolin.lua");   // 'A'..'Z' lower-cased
    CHECK(r.level_setting[0] == "addphysicsdamage_p");
    CHECK(r.level_data[0] == "shaolin_gunfa");
    CHECK(r.level_setting[3].empty());
    CHECK(r.level_data[3] == "shaolin_gunfa");
}

TEST_CASE("LoadSkillLevelData asks the level script setting by setting", "[skill]")
{
    Quiet q;
    KScriptCache cache(make_scripts());
    std::unordered_map<std::string, std::string> cells{
        {"SkillId", "9"}, {"SkillStyle", "0"}, {"CostValue", "1"}, {"LvlSetScript", "\\script\\skill\\t.lua"},
        {"LvlSetting1", "skill_cost_v"}, {"LvlData1", "t_skill"},
        {"LvlSetting2", "physicsenhance_p"}, {"LvlData2", "t_skill"},
        {"LvlSetting3", "addphysicsdamage_p"}, {"LvlData3", "t_skill"},
        {"LvlSetting4", "lifemax_v"}, {"LvlData4", "t_skill"},
        {"LvlSetting5", "missle_speed_v"}, {"LvlData5", "t_skill"},
        {"LvlSetting6", "attackrating_p"}, {"LvlData6", "0"},          // data starting with '0': skipped
        {"LvlSetting8", "bogus"}, {"LvlData8", "t_skill"},             // unknown name: nothing
        {"LvlSetting9", "skill_desc"}, {"LvlData9", "t_skill"},        // refused
        {"LvlSetting10", "stopper"}, {"LvlData10", "t_skill"},         // nil: the loop ends
        {"LvlSetting11", "attackrating_p"}, {"LvlData11", "t_skill"},  // never reached
    };
    KSkill s;
    s.row = KSkillRow::from_cells(cells);
    s.row.row = 2;
    s.load_skill_level_data(1, &cache);
    CHECK(s.level_data_loaded);
    CHECK(s.level == 1);
    CHECK(s.row.cost == 4);   // 3 + level, a number printed "%d"
    REQUIRE(s.damage_attrib_count == 1);
    CHECK(s.damage_attribs[2].type == magic_attrib_id("physicsenhance_p"));
    CHECK(s.damage_attribs[2].value == std::array<int, 3>{100, 0, 0});
    CHECK(s.damage_attribs[0].type == 0);   // attackrating_p never applied
    REQUIRE(s.state_attrib_count == 1);
    CHECK(s.state_attribs[0].type == magic_attrib_id("addphysicsdamage_p"));
    CHECK(s.state_attribs[0].value == std::array<int, 3>{25, -1, 2});
    REQUIRE(s.immediate_attrib_count == 1);
    CHECK(s.immediate_attribs[0].type == magic_attrib_id("lifemax_v"));
    CHECK(s.immediate_attribs[0].value == std::array<int, 3>{50, 0, 0});
    REQUIRE(s.missle_attrib_count == 1);
    CHECK(s.missle_attribs[0].value == std::array<int, 3>{18, 0, 0});

    // a second load starts the lists again
    s.load_skill_level_data(2, &cache);
    CHECK(s.row.cost == 5);
    CHECK(s.damage_attrib_count == 1);

    // no script: nothing loaded, no crash
    KSkill none;
    none.row = s.row;
    none.row.level_set_script = "\\script\\skill\\missing.lua";
    none.load_skill_level_data(1, &cache);
    CHECK_FALSE(none.level_data_loaded);
    CHECK(none.state_attrib_count == 0);
    // row 1 (the header) is never a skill
    none.row.row = 1;
    none.load_skill_level_data(1, &cache);
    CHECK_FALSE(none.level_data_loaded);
}

TEST_CASE("KSkillTable keeps the last row of an id; KSkillManager instantiates per level and style", "[skill]")
{
    Quiet q;
    const std::filesystem::path p = std::filesystem::temp_directory_path() / "jxnext_skill_test" / "skills.json";
    std::filesystem::create_directories(p.parent_path());
    std::ofstream(p) << R"({"rows": [
        {"row": 2, "id": 5, "style": 0, "max_level": 20, "cells": {"SkillName": "A", "SkillId": "5", "SkillStyle": "0", "LvlSetScript": "\\script\\skill\\t.lua", "LvlSetting1": "skill_cost_v", "LvlData1": "t_skill"}},
        {"row": 3, "id": 6, "style": 7, "max_level": 1, "cells": {"SkillId": "6", "SkillStyle": "7"}},
        {"row": 4, "id": 5, "style": 2, "max_level": 30, "cells": {"SkillName": "A2", "SkillId": "5", "SkillStyle": "2", "LvlSetScript": "\\script\\skill\\t.lua", "LvlSetting1": "skill_cost_v", "LvlData1": "t_skill"}},
        {"row": 5, "id": 8, "style": 13, "max_level": 1, "cells": {"SkillId": "8", "SkillStyle": "13"}},
        {"row": 6, "id": 9, "style": 14, "max_level": 1, "cells": {"SkillId": "9", "SkillStyle": "14"}},
        {"row": 7, "id": 2001, "style": 0, "max_level": 1, "cells": {"SkillId": "2001"}},
        {"row": 8, "id": 10, "style": -1, "max_level": 1, "cells": {"SkillId": "10", "SkillStyle": "-1"}}
    ]})";
    std::string error;
    auto table = KSkillTable::load(p.string(), &error);
    REQUIRE(table);
    CHECK(table->size() == 4);   // 5, 6, 8, 9
    REQUIRE(table->info(5) != nullptr);
    CHECK(table->info(5)->row == 4);   // the last row wins (m_SkillInfo is overwritten row by row)
    CHECK(table->info(5)->name == "A2");
    CHECK(table->max_level(5) == 30);
    CHECK(table->style(5) == 2);
    CHECK(table->style(7) == -1);
    CHECK(table->max_level(2001) == 0);

    KScriptCache cache(make_scripts());
    auto shared = std::make_shared<const KSkillTable>(std::move(*table));
    KSkillManager m(shared, &cache);
    const KSkill* a1 = m.get(5, 1);
    REQUIRE(a1 != nullptr);
    CHECK(a1->level == 1);
    CHECK(a1->row.cost == 4);
    CHECK(m.get(5, 1) == a1);           // cached
    const KSkill* a2 = m.get(5, 2);
    REQUIRE(a2 != nullptr);
    CHECK(a2 != a1);
    CHECK(a2->row.cost == 5);
    CHECK(m.instances() == 2);
    CHECK(m.get(6, 1) == nullptr);      // style 7: not instantiated by InstanceSkill
    CHECK(m.get(8, 1) == nullptr);      // style 13: thief skills live elsewhere
    CHECK(m.get(9, 1) != nullptr);      // style 14 is
    CHECK(m.get(5, 0) == nullptr);
    CHECK(m.get(5, 65) == nullptr);
    CHECK(m.get(2001, 1) == nullptr);
    CHECK(m.get(7, 1) == nullptr);
}

TEST_CASE("the exported skill table and the converted skill scripts give the JX2 numbers", "[skill]")
{
    const std::filesystem::path p = std::filesystem::path(JX_NEXT_DIR) / "client" / "assets" / "skills.json";
    const std::string roots = converted_roots();
    if (!std::filesystem::exists(p) || roots.empty()) {
        WARN("no exported skill table at " << p.string() << " or no converted scripts (python tools/dev.py assets / lua)");
        return;
    }
    Quiet q;
    std::string error;
    auto table = KSkillTable::load(p.string(), &error);
    REQUIRE(table);
    CHECK(table->size() > 1500);
    // Thiếu Lâm Côn pháp (skill 4): the row of settings\skills.txt
    const KSkillRow* r4 = table->info(4);
    REQUIRE(r4 != nullptr);
    CHECK(r4->name == "Thiếu Lâm Côn pháp");
    CHECK(r4->style == 3);
    CHECK(r4->req_level == 10);
    CHECK(r4->max_level == 20);
    CHECK(r4->eqt_limit == 2);
    CHECK(r4->missles_form == 7);
    CHECK(r4->attack_radius == 0);    // the cell says "0" (a sect skill has no reach of its own)
    CHECK(r4->level_set_script == "\\script\\skill\\shaolin.lua");
    CHECK(r4->level_setting[0] == "addphysicsdamage_p");
    CHECK(r4->level_data[0] == "shaolin_gunfa");
    // the basic attack (skill 1)
    const KSkillRow* r1 = table->info(1);
    REQUIRE(r1 != nullptr);
    CHECK(r1->is_melee);
    CHECK(r1->target_enemy);
    CHECK(r1->use_attack_rate);
    CHECK(r1->weapon_skill);
    CHECK(r1->attack_radius == 100);
    CHECK(r1->do_hurt == 80);
    CHECK(r1->child_skill_id == 64);
    CHECK(r1->relation == jx::zone::skill_relation_enemy);

    // shaolin.lua: shaolin_gunfa = { addphysicsdamage_p = {{{1,25},{20,100}}, {{1,-1},{2,-1}}, {{1,2},{2,2}}},
    //   attackratingenhance_p = {{{1,35},{20,275}}, {{1,-1},{2,-1}}}, deadlystrikeenhance_p = {{{1,6},{20,45,Conic}}, {{1,-1},{2,-1}}} }
    KScriptCache cache(roots);
    auto shared = std::make_shared<const KSkillTable>(std::move(*table));
    KSkillManager m(shared, &cache);
    const KSkill* s1 = m.get(4, 1);
    REQUIRE(s1 != nullptr);
    REQUIRE(s1->level_data_loaded);
    REQUIRE(s1->state_attrib_count == 3);
    CHECK(s1->state_attribs[0].type == magic_attrib_id("addphysicsdamage_p"));
    CHECK(s1->state_attribs[0].value == std::array<int, 3>{25, -1, 2});
    CHECK(s1->state_attribs[1].type == magic_attrib_id("attackratingenhance_p"));
    CHECK(s1->state_attribs[1].value == std::array<int, 3>{35, -1, 0});
    CHECK(s1->state_attribs[2].type == magic_attrib_id("deadlystrikeenhance_p"));
    CHECK(s1->state_attribs[2].value == std::array<int, 3>{6, -1, 0});
    CHECK(s1->immediate_attrib_count == 0);
    CHECK(s1->damage_attrib_count == 0);
    const KSkill* s20 = m.get(4, 20);
    REQUIRE(s20 != nullptr);
    REQUIRE(s20->state_attrib_count == 3);
    CHECK(s20->state_attribs[0].value == std::array<int, 3>{100, -1, 2});
    CHECK(s20->state_attribs[1].value == std::array<int, 3>{275, -1, 0});
    CHECK(s20->state_attribs[2].value == std::array<int, 3>{45, -1, 0});   // Conic reaches its end point exactly
    const KSkill* s10 = m.get(4, 10);
    REQUIRE(s10 != nullptr);
    CHECK(s10->state_attribs[0].value[0] == 60);    // Line: 25 + (100 - 25) * 9 / 19 = 60.5 -> floor
    CHECK(s10->state_attribs[1].value[0] == 148);   // 35 + 240 * 9 / 19 = 148.68
    // the basic attack: physicsenhance_p / attackrating_p / skill_cost_v through its own script
    const KSkill* b = m.get(1, 1);
    REQUIRE(b != nullptr);
    if (b->level_data_loaded) {
        CHECK(b->damage_attribs[0].type == magic_attrib_id("attackrating_p"));
        CHECK(b->damage_attribs[2].type == magic_attrib_id("physicsenhance_p"));
        CHECK(b->damage_attrib_count == 2);
    } else {
        WARN("the level script of skill 1 (\\script\\skill\\special\\...) is not in the converted trees");
    }
}
