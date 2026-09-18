// KSkillList of the JX2 server (jx_linux_y KNpc+0x248: Add 0x080E5420, IncreaseLevel 0x080E5010,
// the current levels 0x080E56A0, the increment nodes 0x080E5BF0, the cool downs 0x080E4540 /
// 0x080E4640 / 0x080E4740 / 0x080E47B0, the experience 0x080E5D90 / 0x080E4F20, the role data
// 0x080E48D0 / 0x080C0240) and the player's part (KPlayer::AddSkillPoint 0x080BD460, the skill
// experience of a blow 0x0808AA38, allskill_v 0x080993A0) - docs/LINUX-SERVER.md §15.  Every
// number here is worked out by hand from the binary.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSkillList.h"
#include "jx/zone/KSubWorld.h"

using namespace jx::zone;   // the MAGIC_ATTRIB ids
using jx::EntityId;

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

// the level script of the test rows: the experience a level needs (100 x level), the damage
// enhance a level gives skill 1001 (5 x level), a defence for the passive skill
constexpr const char* kLevelScript = R"lua(
function GetSkillLevelData(levelname, data, level)
    if data == "e" and levelname == "skill_skillexp_v" then return tostring(100 * level) end
    if data == "d" and levelname == "addskilldamage1" then return "1001,0," .. tostring(5 * level) end
    if data == "p" and levelname == "armordefense_v" then return "10,1,0" end
    if data == "h" and levelname == "addskillexp1" then return "1002,50,0" end
    return ""
end
)lua";

// the level-up script of skill 1006: two levels per point (SetSkillLevel through the script api)
constexpr const char* kLevelUpScript = R"lua(
function main(points)
    SetSkillLevel(1006, GetCurrentMagicLevel(1006, 0) + points * 2)
end
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_skilllist_test";
    std::filesystem::create_directories(root / "script" / "skill");
    std::ofstream(root / "script" / "skill" / "list.lua") << kLevelScript;
    std::ofstream(root / "script" / "skill" / "lvup.lua") << kLevelUpScript;
    return root.generic_string();
}

// 1001 a plain initiative skill (ReqLevel 10, MaxLevel 20); 1002 an exp skill (MaxLevel 5);
// 1003 a passive (style 3, ReqLevel 12); 1004 a weapon skill; 1005 a missile skill whose levels
// raise 1001's damage; 1006 with a level-up script; 1007 a blow that gives 1002 experience
std::shared_ptr<const KSkillTable> skill_table()
{
    KSkillTable t;
    int row = 2;
    auto add = [&](int id, const char* style, std::vector<std::pair<std::string, std::string>> extra) {
        std::unordered_map<std::string, std::string> cells{{"SkillId", std::to_string(id)}, {"SkillStyle", style}, {"Series", "-1"},
                                                           {"DoHurt", "0"}, {"IsPhysical", "1"}, {"LvlSetScript", "\\script\\skill\\list.lua"},
                                                           {"LvlSetting1", "life_v"}, {"LvlData1", "x"}, {"MaxLevel", "20"}, {"ReqLevel", "1"}};
        for (auto& [k, v] : extra) cells[k] = v;
        KSkillRow r = KSkillRow::from_cells(cells);
        r.row = row++;
        r.max_level = std::stoi(cells["MaxLevel"]);   // TSkillInfo + 8 (skills.json max_level, not a cell of the row block)
        t.add(r);
    };
    add(1001, "2", {{"TargetEnemy", "1"}, {"ReqLevel", "10"}});
    add(1002, "2", {{"TargetEnemy", "1"}, {"IsExpSkill", "1"}, {"MaxLevel", "5"}, {"LvlSetting1", "skill_skillexp_v"}, {"LvlData1", "e"}});
    add(1003, "3", {{"TargetSelf", "1"}, {"ReqLevel", "12"}, {"MaxLevel", "10"}, {"LvlSetting1", "armordefense_v"}, {"LvlData1", "p"}});
    add(1004, "2", {{"TargetEnemy", "1"}, {"WeaponSkill", "1"}, {"MaxLevel", "1"}});
    add(1005, "0", {{"TargetEnemy", "1"}, {"MaxLevel", "10"}, {"LvlSetting1", "addskilldamage1"}, {"LvlData1", "d"}});
    add(1006, "2", {{"TargetEnemy", "1"}, {"MaxLevel", "9"}, {"LevelUpScript", "\\script\\skill\\lvup.lua"}});
    add(1007, "2", {{"TargetEnemy", "1"}, {"LvlSetting1", "addskillexp1"}, {"LvlData1", "h"}});
    return std::make_shared<const KSkillTable>(std::move(t));
}

// a list with a manager of its own and a host that counts the passive casts / removals
struct Bench {
    Quiet quiet;
    std::shared_ptr<const KSkillTable> table = skill_table();
    KScriptCache cache{make_scripts()};
    KSkillManager mgr{table, &cache};
    KSkillList list;
    KSkillListHost host;
    std::vector<int> cast;      // the skills cast as passives (id x 100 + level)
    std::vector<int> removed;   // the states removed
    explicit Bench(int npc_level = 20)
    {
        host.skills = &mgr;
        host.npc_level = npc_level;
        host.cast_passive = [this](const KSkill& sk) { cast.push_back(sk.row.id * 100 + sk.level); };
        host.remove_state = [this](int id) { removed.push_back(id); };
    }
};

KSubWorldConfig small_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;
    c.map_npcs = false;
    c.skills = skill_table();
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    return c;
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_stats()->set_strength(100);
    r.mutable_stats()->set_dexterity(100);
    r.mutable_stats()->set_vitality(50);
    r.mutable_stats()->set_energy(10);
    r.mutable_stats()->set_hp_max(500);
    r.mutable_stats()->set_hp(500);
    r.mutable_stats()->set_mp_max(50);
    r.mutable_stats()->set_skill_point(3);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

void add_role_skill(jx::pb::RoleData& r, int id, int level, int exp = 0)
{
    jx::pb::RoleSkill* s = r.add_skills();
    s->set_id(static_cast<std::uint32_t>(id));
    s->set_level(static_cast<std::uint32_t>(level));
    s->set_exp(static_cast<std::uint32_t>(exp));
}

KMagicAttrib attrib(int type, int v0, int v1 = 0, int v2 = 0)
{
    KMagicAttrib m;
    m.type = type;
    m.value = {v0, v1, v2};
    return m;
}

struct Arena {
    Quiet quiet;
    KSubWorld w;
    EntityId hero;
    EntityId pig;
    KNpc* h = nullptr;
    KNpc* p = nullptr;
    explicit Arena(const jx::pb::RoleData& r) : w(small_world())
    {
        Pos at;
        REQUIRE(w.spawn_player(7, r, hero, at) == jx::pb::RESULT_OK);
        pig = w.spawn_npc("pig", Pos{2050, 2000}, 418, 0, KNpcKind::monster);
        h = w.mutable_entity(hero);
        p = w.mutable_entity(pig);
        REQUIRE(h != nullptr);
        REQUIRE(p != nullptr);
        p->base.life_max = 1000;
        p->cur.life_max = p->cur.life_max_yan = 1000;
        p->cur.life = 1000;
        p->cur.defend = 0;
        p->camp = p->current_camp = camp_animal;
        p->cur.physics_resist_max = 100;
        h->cur.physics_damage.value = {10, 0, 10};
    }
    std::vector<Packet> sent(std::uint16_t msg_id)
    {
        std::vector<Packet> out;
        for (Packet& pk : w.take_outbox()) {
            if (pk.msg_id == msg_id) out.push_back(std::move(pk));
        }
        return out;
    }
};

} // namespace

TEST_CASE("cells: Add, FindSame, the levels, max level and req level, the counts", "[skilllist]")
{
    Bench b;
    // 0x080E5420: a new cell at the level asked, MaxLevel of the row, ReqLevel of the instance
    CHECK(b.list.add(1001, 3, 0, 0, 0, b.host) == 1);
    const KNpcSkill* c = b.list.cell(1);
    REQUIRE(c != nullptr);
    CHECK(c->id == 1001);
    CHECK(c->level == 3);
    CHECK(c->current_level == 3);
    CHECK(c->max_level == 20);
    CHECK(c->req_level == 10);
    CHECK_FALSE(c->only_inc);
    CHECK(b.list.find_same(1001) == 1);
    CHECK(b.list.find_same(9999) == 0);
    CHECK(b.list.get_level(1001) == 3);
    CHECK(b.list.get_current_level(1001, true) == 3);
    CHECK(b.list.get_level(1002) == 0);
    // added again: the same cell moves to the new level (IncreaseLevel by the difference)
    CHECK(b.list.add(1001, 5, 0, 0, 0, b.host) == 1);
    CHECK(c->level == 5);
    CHECK(c->current_level == 5);
    // a max level override and a reborn addon
    CHECK(b.list.add(1002, 2, 40, 0, 3, b.host) == 2);
    CHECK(b.list.cell(2)->max_level == 5 + 3);
    CHECK(b.list.cell(2)->exp == 40);
    CHECK(b.list.add(1004, 1, 0, 7, 0, b.host) == 3);
    CHECK(b.list.cell(3)->max_level == 7);
    // refused: a level below 0, an id of 0
    CHECK(b.list.add(1001, -1, 0, 0, 0, b.host) == 0);
    CHECK(b.list.add(0, 1, 0, 0, 0, b.host) == 0);
    // the counts: 0x080E4380 every cell held; 0x080E4B30 without the weapon and exp skills;
    // 0x080E4C00 the levels of those
    CHECK(b.list.get_count() == 3);
    CHECK(b.list.get_count(true, &b.mgr) == 3);
    CHECK(b.list.get_count(false, &b.mgr) == 1);
    CHECK(b.list.get_total_level(&b.mgr) == 5);
    // 0x080E4310: a npc's Skill1..4 straight into a cell, max level = the level
    b.list.set_npc_skill(4, 1005, 2);
    CHECK(b.list.cell(4)->id == 1005);
    CHECK(b.list.cell(4)->level == 2);
    CHECK(b.list.cell(4)->current_level == 2);
    CHECK(b.list.cell(4)->max_level == 2);
    b.list.set_npc_skill(5, 1005, 0);   // a level of 0 is ignored
    CHECK(b.list.cell(5)->id == 0);
    // 0x080E48D0 / 0x080C0240: the role data round trip keeps id, level and experience
    const std::vector<KSkillSaved> saved = b.list.serialize();
    REQUIRE(saved.size() == 4);
    CHECK(saved[1].id == 1002);
    CHECK(saved[1].level == 2);
    CHECK(saved[1].exp == 40);
    Bench b2;
    b2.list.deserialize(saved, 0, b2.host);
    CHECK(b2.list.get_level(1001) == 5);
    CHECK(b2.list.get_level(1002) == 2);
    CHECK(b2.list.cell(b2.list.find_same(1002))->exp == 40);
    CHECK(b2.list.get_level(1005) == 2);
}

TEST_CASE("IncreaseLevel: the experience resets, the enhance map follows, passives are cast or removed", "[skilllist]")
{
    Bench b(20);
    // 0x080E4CA0: skill 1005's addskilldamage1 gives 1001 five per level
    CHECK(b.list.add(1005, 1, 0, 0, 0, b.host) == 1);
    CHECK(b.list.enhance.at(1001) == 5);
    CHECK(b.list.increase_level(1, 1, b.host) == 1);
    CHECK(b.list.cell(1)->level == 2);
    CHECK(b.list.enhance.at(1001) == 10);
    b.list.skills[1].exp = 77;
    CHECK(b.list.increase_level(1, -1, b.host) == 1);
    CHECK(b.list.skills[1].exp == 0);   // +0x20 reset
    CHECK(b.list.enhance.at(1001) == 5);
    // 0x080E51B9: a passive skill is cast on the way up when the npc's level allows ...
    CHECK(b.list.add(1003, 1, 0, 0, 0, b.host) == 2);
    REQUIRE(b.cast.size() == 1);
    CHECK(b.cast[0] == 1003 * 100 + 1);
    CHECK(b.list.increase_level(2, 2, b.host) == 1);
    REQUIRE(b.cast.size() == 2);
    CHECK(b.cast[1] == 1003 * 100 + 3);
    // ... and its state goes when the level drops to 0 (0x080E5280)
    b.list.remove(1003, b.host);
    REQUIRE(b.removed.size() == 1);
    CHECK(b.removed[0] == 1003);
    CHECK(b.list.find_same(1003) == 0);
    // a npc below the passive's ReqLevel learns it without a cast
    Bench low(5);
    CHECK(low.list.add(1003, 1, 0, 0, 0, low.host) == 1);
    CHECK(low.cast.empty());
    // 0x080E4A00: reaching exactly ReqLevel casts it (KPlayer::LevelUp)
    low.host.npc_level = 12;
    CHECK(low.list.cast_passives_at_level(low.host));
    REQUIRE(low.cast.size() == 1);
    low.host.npc_level = 13;
    CHECK_FALSE(low.list.cast_passives_at_level(low.host));
    // a bad cell / a zero delta
    CHECK(b.list.increase_level(0, 1, b.host) == 0);
    CHECK(b.list.increase_level(1, 0, b.host) == 1);
}

TEST_CASE("the increments: the every-skill node, only_inc cells, the bonus on a skill learned or unlearned", "[skilllist]")
{
    Bench b;
    CHECK(b.list.add(1001, 3, 0, 0, 0, b.host) == 1);
    // 0x080E5BF0 with id 0: every skill held gains, the node keeps the sum
    CHECK(b.list.add_level_inc(0, 2, b.host) == 1);
    CHECK(b.list.cell(1)->current_level == 5);
    CHECK(b.list.cell(1)->level == 3);
    CHECK(b.list.get_current_level(1001, true) == 5);
    CHECK(b.list.get_current_level(1001, false) == 3);   // 0x080E4487: the nodes taken off
    CHECK(b.list.get_all_inc() == 2);
    REQUIRE(b.list.inc_nodes.size() == 1);
    CHECK(b.list.add_level_inc(0, 2, b.host) == 1);
    CHECK(b.list.cell(1)->current_level == 7);
    CHECK(b.list.inc_nodes[0].inc == 4);
    // 0x080E50B4: a skill learned from level 0 takes the every-skill increments at once
    CHECK(b.list.add(1005, 1, 0, 0, 0, b.host) == 2);
    CHECK(b.list.cell(2)->level == 1);
    CHECK(b.list.cell(2)->current_level == 5);
    CHECK(b.list.enhance.at(1001) == 25);   // the enhance of the current level (5 x 5)
    // 0x080E5054: unlearned, it gives them back and the cell is freed
    b.list.remove(1005, b.host);
    CHECK(b.list.find_same(1005) == 0);
    CHECK(b.list.enhance.at(1001) == 0);
    // the node erased when its sum is 0, the current levels back
    CHECK(b.list.add_level_inc(0, -4, b.host) == 1);
    CHECK(b.list.inc_nodes.empty());
    CHECK(b.list.cell(1)->current_level == 3);
    // 0x080E5CAC: a skill not held gets a cell that lives on the increments only ...
    CHECK(b.list.add_level_inc(1002, 1, b.host) == 1);
    const int idx = b.list.find_same(1002);
    REQUIRE(idx != 0);
    CHECK(b.list.cell(idx)->only_inc);
    CHECK(b.list.cell(idx)->level == 0);
    CHECK(b.list.cell(idx)->current_level == 1);
    CHECK(b.list.serialize().size() == 1);   // and is not saved
    // ... and goes when they do (0x080E596C)
    CHECK(b.list.add_level_inc(1002, -1, b.host) == 1);
    CHECK(b.list.find_same(1002) == 0);
    // 0x080E52D0: a skill unlearned while an increment holds it stays as an only_inc cell
    CHECK(b.list.add_level_inc(1001, 1, b.host) == 1);
    CHECK(b.list.cell(1)->current_level == 4);
    b.list.remove(1001, b.host);
    CHECK(b.list.cell(1)->id == 1001);
    CHECK(b.list.cell(1)->only_inc);
    CHECK(b.list.cell(1)->level == 0);
    CHECK(b.list.cell(1)->current_level == 1);
    // KNpc::ClearAttrib 0x0807F341: the current levels back to the learned ones
    CHECK(b.list.add(1005, 2, 0, 0, 0, b.host) == 2);
    CHECK(b.list.add_level_inc(0, 3, b.host) == 1);
    CHECK(b.list.cell(2)->current_level == 5);
    CHECK(b.list.enhance.at(1001) == 25);
    b.list.clear_attrib(&b.mgr);
    CHECK(b.list.cell(2)->current_level == 2);
    CHECK(b.list.cell(1)->current_level == 0);
    CHECK(b.list.enhance.at(1001) == 10);
    // 0x080E56A0: the cap of 64 on every cell, and the quirk of exactly 64 on one cell
    CHECK(b.list.change_current_level(0, 100, b.host) == 1);
    CHECK(b.list.cell(2)->current_level == 64);
    CHECK(b.list.change_current_level(2, -62, b.host) == 1);
    CHECK(b.list.cell(2)->current_level == 2);
    CHECK(b.list.change_current_level(2, 62, b.host) == 0);
    CHECK(b.list.cell(2)->current_level == 64);
}

TEST_CASE("cool downs and the forbid flags", "[skilllist]")
{
    Bench b;
    CHECK(b.list.add(1001, 3, 0, 0, 0, b.host) == 1);
    // 0x080E4640: next cast = frame + length, the length kept
    b.list.set_next_cast_time(1001, 100, 30);
    CHECK(b.list.next_cast_time(1001) == 130);
    CHECK(b.list.cool_down_time(1001) == 30);
    // 0x080E4540: the cool down, the npc's level against ReqLevel (10), a level of 0 skips that
    CHECK_FALSE(b.list.can_cast(1001, 129, 20));
    CHECK(b.list.can_cast(1001, 130, 20));
    CHECK_FALSE(b.list.can_cast(1001, 130, 9));
    CHECK(b.list.can_cast(1001, 130, 0));
    CHECK_FALSE(b.list.can_cast(1002, 130, 20));   // not held
    CHECK(b.list.is_cooling(1001, 129));
    CHECK_FALSE(b.list.is_cooling(1001, 130));
    // 0x080E4740: reduceskillcd - the next cast nearer, the length shorter, never past 0
    b.list.reduce_cool_time(1001, 10);
    CHECK(b.list.next_cast_time(1001) == 120);
    CHECK(b.list.cool_down_time(1001) == 20);
    b.list.reduce_cool_time(1001, 500);
    CHECK(b.list.next_cast_time(1001) == 120);
    CHECK(b.list.cool_down_time(1001) == 20);
    // 0x080E47B0: a pending cool down is dropped, an only_inc cell is left alone
    b.list.add_level_inc(1005, 1, b.host);
    b.list.set_next_cast_time(1005, 100, 50);
    b.list.clear_cool_time(110);
    CHECK(b.list.next_cast_time(1001) == 0);
    CHECK(b.list.cool_down_time(1001) == 0);
    CHECK(b.list.next_cast_time(1005) == 150);
    b.list.set_next_cast_time(1001, 100, 5);
    b.list.clear_cool_time(110);   // already over: kept
    CHECK(b.list.next_cast_time(1001) == 105);
    // 0x080AE9E0 / 0x080E45C0 / 0x080E4610: one skill, then every skill; a cell added later starts locked
    b.list.set_forbid(1001, 1);
    CHECK(b.list.is_forbidden(1001));
    CHECK_FALSE(b.list.can_cast(1001, 200, 20));
    b.list.set_forbid(1001, 0);
    CHECK(b.list.can_cast(1001, 200, 20));
    b.list.set_forbid_all(1);
    CHECK(b.list.is_forbidden(1001));
    CHECK(b.list.add(1002, 1, 0, 0, 0, b.host) != 0);
    CHECK(b.list.is_forbidden(1002));
    b.list.set_forbid_all(0);
    CHECK_FALSE(b.list.is_forbidden(1001));
    CHECK_FALSE(b.list.is_forbidden(1002));
}

TEST_CASE("the experience of an exp skill: the bar, the level up, the cap, the percent mode, the no-level-up flag", "[skilllist]")
{
    Bench b;
    CHECK(b.list.add(1002, 1, 0, 0, 0, b.host) == 1);   // level 1 needs 100
    KSkillList::ExpResult r = b.list.add_skill_exp(attrib(magic_addskillexp1, 1002, 30, 0), false, b.host);
    CHECK(r.handled);
    CHECK(r.old_level == 1);
    CHECK_FALSE(r.level_reached);
    CHECK(b.list.cell(1)->exp == 30);
    CHECK(b.list.exp_percent(1, &b.mgr) == 307);   // 30 << 10 / 100
    // the need reached: level 2, the experience reset by IncreaseLevel
    r = b.list.add_skill_exp(attrib(magic_addskillexp1, 1002, 70, 0), false, b.host);
    CHECK(r.level_reached);
    CHECK(r.level_up);
    CHECK(b.list.cell(1)->level == 2);
    CHECK(b.list.cell(1)->exp == 0);
    // nValue[2] bit 0: the need (200) reached but the level stays, the experience capped at it
    r = b.list.add_skill_exp(attrib(magic_addskillexp1, 1002, 250, 1), false, b.host);
    CHECK(r.level_reached);
    CHECK_FALSE(r.level_up);
    CHECK(b.list.cell(1)->level == 2);
    CHECK(b.list.cell(1)->exp == 200);
    CHECK(b.list.exp_percent(1, &b.mgr) == 1024);
    // the percent mode: 5000 = half of the need (100 of 200) - and the capped 200 carries on
    r = b.list.add_skill_exp(attrib(magic_addskillexp1, 1002, 5000, 0), true, b.host);
    CHECK(r.level_up);
    CHECK(b.list.cell(1)->level == 3);
    // at MaxLevel (5) nothing moves
    b.list.add(1002, 5, 0, 0, 0, b.host);
    r = b.list.add_skill_exp(attrib(magic_addskillexp1, 1002, 5000, 0), false, b.host);
    CHECK(r.handled);
    CHECK_FALSE(r.level_reached);
    CHECK(b.list.cell(1)->level == 5);
    CHECK(b.list.cell(1)->exp == 0);
    // a skill that is not an exp skill, or not held: nothing
    CHECK(b.list.add(1001, 1, 0, 0, 0, b.host) == 2);
    CHECK_FALSE(b.list.add_skill_exp(attrib(magic_addskillexp1, 1001, 50, 0), false, b.host).handled);
    CHECK_FALSE(b.list.add_skill_exp(attrib(magic_addskillexp1, 1005, 50, 0), false, b.host).handled);
    CHECK(b.list.exp_percent(2, &b.mgr) == 0);
    // 0x080E5370: RollbackSkill takes the plain skills back to 0 and tells how many levels went
    b.list.add(1004, 1, 0, 0, 0, b.host);
    CHECK(b.list.rollback(b.host) == 1);   // 1001 only: 1002 is an exp skill, 1004 a weapon skill
    CHECK(b.list.get_level(1001) == 0);
    CHECK(b.list.get_level(1002) == 5);
    CHECK(b.list.get_level(1004) == 1);
}

TEST_CASE("a player's skills: loaded from the role data, synced to the client, saved back", "[skilllist]")
{
    jx::pb::RoleData r = role(70, "Hero", Pos{2000, 2000});
    add_role_skill(r, 1001, 3);
    add_role_skill(r, 1002, 2, 50);
    Arena a(r);
    CHECK(a.h->skill_list.get_level(1001) == 3);
    CHECK(a.h->skill_list.get_level(1002) == 2);
    CHECK(a.h->skill_list.cell(a.h->skill_list.find_same(1002))->exp == 50);
    CHECK(a.h->skill_mgr == a.w.skills());
    // the s2c_synccurplayerskill of the old server: every skill held, on entering
    const auto lists = a.sent(static_cast<std::uint16_t>(jx::pb::G2C_SKILL_LIST));
    REQUIRE(lists.size() == 1);
    jx::pb::SkillListSync sync;
    REQUIRE(sync.ParseFromString(lists[0].payload));
    REQUIRE(sync.skills_size() == 2);
    CHECK(sync.skills(0).skill_id() == 1001);
    CHECK(sync.skills(0).level() == 3);
    CHECK(sync.skills(0).req_level() == 10);
    CHECK(sync.skills(0).max_level() == 20);
    CHECK(sync.skills(1).exp_percent() == 256);   // 50 of 200 at level 2
    // the round trip through the role data
    jx::pb::RoleData out;
    REQUIRE(a.w.role_snapshot(7, out));
    REQUIRE(out.skills_size() == 2);
    CHECK(out.skills(1).id() == 1002);
    CHECK(out.skills(1).level() == 2);
    CHECK(out.skills(1).exp() == 50);
    // a pig's Skill1..4 sit in its cells too (KNpc::Init copies the template's list)
    CHECK(a.p->skill_list.get_count() == 0);   // the test pig has no template skills
}

TEST_CASE("KPlayer::AddSkillPoint: every check in its order", "[skilllist]")
{
    jx::pb::RoleData r = role(70, "Hero", Pos{2000, 2000});
    add_role_skill(r, 1001, 3);   // ReqLevel 10
    add_role_skill(r, 1002, 1);   // an exp skill
    add_role_skill(r, 1004, 1);   // MaxLevel 1
    add_role_skill(r, 1006, 1);   // with a level-up script
    Arena a(r);
    a.w.take_outbox();
    const auto level_msgs = [&] {
        std::vector<jx::pb::SkillLevelSync> out;
        for (const auto& pk : a.sent(static_cast<std::uint16_t>(jx::pb::G2C_SKILL_LEVEL))) {
            jx::pb::SkillLevelSync s;
            REQUIRE(s.ParseFromString(pk.payload));
            out.push_back(s);
        }
        return out;
    };
    // 0x080BD8F9: level 5 cannot hold 1001 at 4 (4 > 5 + 1 - 10): refused, the 0x5e packet still sent
    CHECK_FALSE(a.w.add_skill_point_request(7, 1001, 1, 11));
    auto m = level_msgs();
    REQUIRE(m.size() == 1);
    CHECK(m[0].level() == 3);
    CHECK(m[0].seq() == 11);
    CHECK(m[0].skill_point() == 3);
    a.h->level = 20;
    // a point spent: the level, the points left, the passive / enhance bookkeeping through IncreaseLevel
    CHECK(a.w.add_skill_point_request(7, 1001, 1, 12));
    m = level_msgs();
    REQUIRE(m.size() == 1);
    CHECK(m[0].skill_id() == 1001);
    CHECK(m[0].level() == 4);
    CHECK(m[0].skill_point() == 2);
    CHECK(a.h->skill_list.get_level(1001) == 4);
    CHECK(a.h->player.skill_point == 2);
    // not enough points: refused without a packet (0x080BD54E)
    CHECK_FALSE(a.w.add_skill_point_request(7, 1001, 5, 13));
    CHECK(level_msgs().empty());
    CHECK(a.h->player.skill_point == 2);
    // an exp skill grows by use, not by points (0x080BD977): no packet
    CHECK_FALSE(a.w.add_skill_point_request(7, 1002, 1, 14));
    CHECK(level_msgs().empty());
    // a one-level skill (0x080BD72C): no packet
    CHECK_FALSE(a.w.add_skill_point_request(7, 1004, 1, 15));
    CHECK(level_msgs().empty());
    // a skill not held (0x080BD49E): no packet
    CHECK_FALSE(a.w.add_skill_point_request(7, 1003, 1, 16));
    CHECK(level_msgs().empty());
    // MaxLevel (0x080BD7A5): refused with the packet; a reborn addon lifts it
    a.h->player.skill_point = 40;
    CHECK_FALSE(a.w.add_skill_point_request(7, 1001, 17, 17));   // 4 + 17 = 21 > 20
    m = level_msgs();
    REQUIRE(m.size() == 1);
    CHECK(m[0].level() == 4);
    a.h->player.reborn = 1;
    a.h->player.skill_max_level_addons = 2;
    a.h->level = 40;
    CHECK(a.w.add_skill_point_request(7, 1001, 17, 18));
    CHECK(a.h->skill_list.get_level(1001) == 21);
    // 0x080BD5B3: a skill with a level-up script leaves the points to the script (two levels per point here)
    a.h->player.skill_point = 5;
    CHECK(a.w.add_skill_point_request(7, 1006, 2, 19));
    m = level_msgs();
    REQUIRE(m.size() >= 1);
    CHECK(a.h->skill_list.get_level(1006) == 5);
    CHECK(a.h->player.skill_point == 5);   // the script did not spend them
    CHECK(m.back().level() == 5);
    CHECK(m.back().seq() == 19);
}

TEST_CASE("the skill experience of a blow, the client's bar, allskill_v of a piece worn", "[skilllist]")
{
    jx::pb::RoleData r = role(70, "Hero", Pos{2000, 2000});
    add_role_skill(r, 1002, 1);   // the exp skill, level 1 needs 100
    add_role_skill(r, 1001, 2);
    Arena a(r);
    a.w.take_outbox();
    // 0x080E5D90 through the player's part: 8/1024 moves the bar (G2C_SKILL_LEVEL), a level up says so
    CHECK_FALSE(a.w.give_skill_exp(*a.h, attrib(magic_addskillexp1, 1002, 1, 0), false));   // 10/1024: told
    auto msgs = a.sent(static_cast<std::uint16_t>(jx::pb::G2C_SKILL_LEVEL));
    REQUIRE(msgs.size() == 1);
    jx::pb::SkillLevelSync s;
    REQUIRE(s.ParseFromString(msgs[0].payload));
    CHECK(s.skill_id() == 1002);
    CHECK(s.level() == 1);
    CHECK(s.exp_percent() == 10);
    CHECK_FALSE(s.level_up());
    CHECK(a.w.give_skill_exp(*a.h, attrib(magic_addskillexp1, 1002, 99, 0), false));
    msgs = a.sent(static_cast<std::uint16_t>(jx::pb::G2C_SKILL_LEVEL));
    REQUIRE(msgs.size() == 1);
    REQUIRE(s.ParseFromString(msgs[0].payload));
    CHECK(s.level() == 2);
    CHECK(s.level_up());
    CHECK(a.h->skill_list.get_level(1002) == 2);
    // 0x0808AA38: a blow with addskillexp1 gives it six times in ten - 1007's damage entry names 1002
    const KSkill* blow = a.w.skills()->get(1007, 1);
    REQUIRE(blow != nullptr);
    // the skill's own list keeps addskillexp1 at KSkill::damage_slot(73) = 15; AppendSkillEffect
    // hands it to ReceiveDamage one slot later (damage_slot_add_skill_exp1 = 16)
    REQUIRE(blow->damage_attribs[static_cast<std::size_t>(KSkill::damage_slot(magic_addskillexp1))].value[0] == 1002);
    int exp_before = a.h->skill_list.cell(a.h->skill_list.find_same(1002))->exp;
    int gained = 0;
    for (int i = 0; i < 40 && gained == 0; ++i) {
        KSubWorld::KCastParams p;
        p.target = a.pig;
        a.w.skill_cast(*blow, *a.h, p);
        gained = a.h->skill_list.cell(a.h->skill_list.find_same(1002))->exp - exp_before + (a.h->skill_list.get_level(1002) - 2) * 1000;
    }
    CHECK(gained > 0);
    // the pig (no player behind it) gets nothing from the same entry
    CHECK_FALSE(a.w.give_skill_exp(*a.p, attrib(magic_addskillexp1, 1002, 50, 0), false));
    // 0x080993A0 allskill_v {levels, 0, skill}: through ModifyAttrib, on and off again (negated values)
    a.w.modify_attrib(*a.h, a.hero, attrib(magic_allskill_v, 2, 0, 0), false);
    CHECK(a.h->skill_list.get_current_level(1001, true) == 4);
    CHECK(a.h->skill_list.get_current_level(1001, false) == 2);
    a.w.modify_attrib(*a.h, a.hero, attrib(magic_allskill_v, -2, 0, 0), true);
    CHECK(a.h->skill_list.get_current_level(1001, true) == 2);
    CHECK(a.h->skill_list.inc_nodes.empty());
    // 0x08097250 reduceskillcd1 {skill, 0, frames}
    a.h->skill_list.set_next_cast_time(1001, 0, 100);
    a.w.modify_attrib(*a.h, a.hero, attrib(magic_reduceskillcd1, 1001, 0, 30), false);
    CHECK(a.h->skill_list.next_cast_time(1001) == 70);
    // KPlayer::ForbitSkill / SetAForbitSkill and their packet
    a.w.forbit_skill(*a.h, true);
    CHECK(a.h->skill_list.is_forbidden(1001));
    auto fb = a.sent(static_cast<std::uint16_t>(jx::pb::G2C_SKILL_FORBID));
    REQUIRE(fb.size() == 1);
    jx::pb::SkillForbidSync f;
    REQUIRE(f.ParseFromString(fb[0].payload));
    CHECK(f.skill_id() == 0);
    CHECK(f.forbid());
    a.w.set_a_forbit_skill(*a.h, 1001, 0);
    CHECK_FALSE(a.h->skill_list.is_forbidden(1001));
    CHECK(a.h->skill_list.is_forbidden(1002));
}
