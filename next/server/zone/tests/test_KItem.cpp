// Items: the tables, the grid, a player's list and the generator - the rules of KItemList.cpp /
// KInventory.cpp / KItemGenerator.cpp of the old core, on a small table of our own and, when the
// exported tables are there, on the real ones.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KItem.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KNpcTemplate.h"
#include "jx/zone/KObj.h"
#include "jx/zone/KRandom.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/ScriptFuns.h"

using jx::zone::KInventory;
using jx::zone::KItem;
using jx::zone::KItemGenerator;
using jx::zone::KItemGenre;
using jx::zone::KItemList;
using jx::zone::KItemTemplateSet;

namespace {

// A tiny items.json in the shape jxassets export-items writes
const char* const kTables = R"({
 "version": "000",
 "equipment": {
  "horse": [
   {"row": 1, "name": "Ngua", "genre": 0, "detail": 10, "particular": 0, "image": "h.spr", "obj": 9, "w": 2, "h": 2, "intro": "",
    "series": -1, "price": 100, "level": 1, "basics": [{"type": 30, "range": {"min": 7, "max": 7}}], "reqs": []}
  ],
  "meleeweapon": [
   {"row": 1, "name": "Kiem 1", "genre": 0, "detail": 0, "particular": 0, "image": "a.spr", "obj": 9, "w": 1, "h": 3, "intro": "",
    "series": 2, "price": 100, "level": 1,
    "basics": [{"type": 28, "range": {"min": 4, "max": 4}}, {"type": 29, "range": {"min": 7, "max": 9}}, {"type": 31, "range": {"min": 20, "max": 20}}],
    "reqs": [{"type": 36, "para": 5}, {"type": 32, "para": 20}]},
   {"row": 2, "name": "Kiem 2", "genre": 0, "detail": 0, "particular": 0, "image": "b.spr", "obj": 9, "w": 1, "h": 3, "intro": "",
    "series": 2, "price": 200, "level": 2, "basics": [{"type": 28, "range": {"min": 6, "max": 6}}], "reqs": []}
  ],
  "armor": [
   {"row": 1, "name": "Ao 1", "genre": 0, "detail": 2, "particular": 0, "image": "c.spr", "obj": 22, "w": 2, "h": 3, "intro": "",
    "series": 0, "price": 100, "level": 1, "basics": [{"type": 30, "range": {"min": 50, "max": 50}}], "reqs": [{"type": 38, "para": 1}]}
  ],
  "ring": [
   {"row": 1, "name": "Nhan", "genre": 0, "detail": 3, "particular": 0, "image": "r.spr", "obj": 1, "w": 1, "h": 1, "intro": "",
    "series": -1, "price": 10, "level": 1, "basics": [], "reqs": []}
  ]
 },
 "gold": [
  {"row": 1, "name": "Hoang kim", "genre": 0, "detail": 0, "particular": 0, "image": "g.spr", "obj": 9, "w": 1, "h": 3, "intro": "",
   "series": 1, "price": 1000, "level": 10, "basics": [{"type": 28, "range": {"min": 18, "max": 18}}], "reqs": [],
   "magic_ids": [1, 2, 0, 0, 0, 0], "group": 21, "ex_group": 0, "group_serial": 1}
 ],
 "medicine": [
  {"row": 1, "name": "Thuoc 1", "genre": 1, "detail": 0, "particular": 0, "image": "p.spr", "obj": 18, "w": 1, "h": 1, "intro": "",
   "price": 50, "level": 1, "stackable": 0, "attribs": [{"attrib": 153, "value": 10, "time": 100}]},
  {"row": 2, "name": "Thuoc 2", "genre": 1, "detail": 0, "particular": 0, "image": "p.spr", "obj": 18, "w": 1, "h": 1, "intro": "",
   "price": 80, "level": 2, "stackable": 1, "max_stack": 10, "attribs": [{"attrib": 153, "value": 30, "time": 50}, {"attrib": 154, "value": 5, "time": 50}, {"attrib": 153, "value": 999, "time": 999}]}
 ],
 "quest": [
  {"row": 1, "name": "Tram", "genre": 4, "detail": 0, "image": "q.spr", "obj": 41, "w": 1, "h": 1, "intro": "", "particular": 0, "can_sell": 0, "max_stack": 0},
  {"row": 2, "name": "Tui", "genre": 4, "detail": 1, "image": "q.spr", "obj": 41, "w": 1, "h": 1, "intro": "", "particular": 0, "can_sell": 0, "max_stack": 20}
 ],
 "town_portal": [{"row": 1, "name": "Phu", "genre": 5, "image": "t.spr", "obj": 38, "w": 1, "h": 1, "price": 500, "intro": ""}],
 "magic": [
  {"row": 1, "name": "Sac", "pos": 1, "class": -1, "level": 1, "kind": 100, "ranges": [{"min": 1, "max": 3}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [100000, 100000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
  {"row": 2, "name": "Ben", "pos": 1, "class": -1, "level": 1, "kind": 101, "ranges": [{"min": 10, "max": 20}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [100000, 0, 100000, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
  {"row": 3, "name": "Hiem", "pos": 1, "class": 2, "level": 3, "kind": 102, "ranges": [{"min": 5, "max": 5}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
  {"row": 4, "name": "cua Manh", "pos": 0, "class": -1, "level": 1, "kind": 200, "ranges": [{"min": 1, "max": 1}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [100000, 100000, 100000, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
  {"row": 5, "name": "cua Sac", "pos": 0, "class": -1, "level": 1, "kind": 100, "ranges": [{"min": 7, "max": 7}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [100000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
  {"row": 6, "name": "Cam", "pos": 1, "class": -1, "level": 1, "kind": 139, "ranges": [{"min": 1, "max": 1}, {"min": -1, "max": -1}, {"min": 0, "max": 0}], "intro": "",
   "drop_rates": [0, 0, 0, 100000, 0, 0, 0, 0, 0, 0, 0, 0]}
 ],
 "magic_limits": [{"type": 139, "min": [0, 0, 0], "max": [0, 0, 0]}],
 "gold_magic": [
  {"row": 1, "name": "M1", "kind": 126, "ranges": [{"min": 5, "max": 10}, {"min": -1, "max": -1}, {"min": 6, "max": 6}], "intro": ""},
  {"row": 2, "name": "M2", "kind": 85, "ranges": [{"min": 100, "max": 100}, {"min": 0, "max": 0}, {"min": 0, "max": 0}], "intro": ""}
 ],
 "suites": [{"suite": 21, "count": 5}],
 "scripts": [
  {"row": 1, "name": "Ky nang", "genre": 6, "detail": 0, "particular": 7, "image": "s.spr", "obj": 1, "w": 1, "h": 1, "intro": "", "price": 0,
   "script": "", "skill": 141, "show_level": 0, "short_key": 0, "max_stack": 5, "reg_series": 0}
 ]
})";

std::string write_tables()
{
    const auto path = std::filesystem::temp_directory_path() / "jxnext_items_test.json";
    std::ofstream(path, std::ios::binary) << kTables;
    return path.string();
}

KItemTemplateSet load_test_set()
{
    KItemTemplateSet set;
    std::string error;
    REQUIRE(set.load(write_tables(), &error));
    return set;
}

} // namespace

TEST_CASE("item tables are looked up the way KItemGenerator picked its rows", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    CHECK(set.version() == "000");
    CHECK(set.size() == 12);   // the horse row came with B3c-6
    // equipment: row = particular * 10 + level - 1
    REQUIRE(set.equipment(jx::zone::equip_meleeweapon, 0, 1) != nullptr);
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 1)->name == "Kiem 1");
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 2)->name == "Kiem 2");
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 3) == nullptr);
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 1, 1) == nullptr);   // particular 1 would be row 10
    CHECK(set.equipment(jx::zone::equip_armor, 0, 1)->width == 2);
    // medicine: row = detail * 5 + level - 1; quest: row = detail; gold: row id
    CHECK(set.medicine(0, 1)->name == "Thuoc 1");
    CHECK(set.medicine(0, 2)->name == "Thuoc 2");
    CHECK(set.medicine(0, 2)->stackable);
    CHECK(set.medicine(0, 3) == nullptr);
    CHECK(set.quest(1)->max_stack == 20);
    CHECK(set.quest(2) == nullptr);
    CHECK(set.gold(1)->group == 21);
    CHECK(set.gold(0) == nullptr);
    CHECK(set.town_portal()->price == 500);
    CHECK(set.magic_script(0, 7)->skill == 141);
    CHECK(set.magic_script(0, 8) == nullptr);
    CHECK(set.suite_activate_count(21) == 5);
    CHECK(set.suite_activate_count(22) == 0);
}

TEST_CASE("the generator rolls base attributes and copies requirements (SetAttrib_CBR)", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    KItemGenerator gen(set, 0, 7);
    std::set<int> seen;
    for (int i = 0; i < 40; ++i) {
        auto item = gen.equipment(jx::zone::equip_meleeweapon, 0, 3, 1);
        REQUIRE(item.has_value());
        CHECK(item->genre == KItemGenre::equip);
        CHECK(item->series == 3);   // Gen_Equipment sets the series asked for
        CHECK(item->level == 1);
        CHECK(item->name() == "Kiem 1");
        CHECK(item->base[0].type == jx::zone::magic_weapondamagemin_v);
        CHECK(item->base[0].value[0] == 4);
        CHECK(item->base[1].value[0] >= 7);
        CHECK(item->base[1].value[0] <= 9);
        seen.insert(item->base[1].value[0]);
        CHECK(item->durability == 20);   // from the durability base attribute
        CHECK(item->max_durability() == 20);
        CHECK(item->require[0].type == jx::zone::magic_requirelevel);
        CHECK(item->require[0].value[0] == 5);
        CHECK(item->require[1].type == jx::zone::magic_requirestr);
    }
    CHECK(seen.size() == 3);   // 7, 8 and 9 all come out: GetRandomNumber is inclusive
    auto ring = gen.equipment(jx::zone::equip_ring, 0, 0, 1);
    REQUIRE(ring.has_value());
    CHECK(ring->durability == -1);   // no durability attribute: never wears
    auto med = gen.medicine(0, 1);
    REQUIRE(med.has_value());
    CHECK(med->genre == KItemGenre::medicine);
    CHECK(med->base[0].type == jx::zone::magic_lifepotion_v);
    CHECK(med->base[0].value[0] == 10);
    CHECK(med->base[0].value[1] == 100);
    auto tui = gen.quest(1, 50);
    REQUIRE(tui.has_value());
    CHECK(tui->count == 20);   // clamped to the stack
    CHECK(tui->level == 1);
    CHECK(gen.quest(9, 1) == std::nullopt);
    auto sc = gen.magic_script(0, 7, 3, 1, 2);
    REQUIRE(sc.has_value());
    CHECK(sc->level == 3);
    CHECK(sc->group == 141);   // the skill id goes into the group, as Gen_MAScript did
    CHECK(sc->count == 2);
}

TEST_CASE("a gold piece rolls its magic within the gold_magic ranges, better with luck", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    KItemGenerator gen(set, 0, 11);
    int low = 0, high = 0;
    for (int i = 0; i < 200; ++i) {
        auto g = gen.gold(0, 1);
        REQUIRE(g.has_value());
        CHECK(g->ex_type == 1);
        CHECK(g->gen_param == 1);
        CHECK(g->group == 21);
        CHECK(g->magic[0].type == 126);
        CHECK(g->magic[0].value[0] >= 5);
        CHECK(g->magic[0].value[0] <= 10);
        CHECK(g->magic[0].value[2] == 6);
        CHECK(g->magic[1].type == 85);
        CHECK(g->magic[1].value[0] == 100);   // a fixed range rolls its only value
        if (g->magic[0].value[0] <= 7) ++low;
        auto lucky = gen.gold(200, 1);
        if (lucky->magic[0].value[0] == 10) ++high;
    }
    CHECK(low > 100);      // luck 0: mostly the lower half
    CHECK(high == 200);    // luck 200: always the top
}

TEST_CASE("KRandom is g_Random of the old engine: the same seed gives the same numbers", "[item]")
{
    jx::zone::KRandom r(42);   // nRandomSeed = 42 of KRandom.cpp
    CHECK(r(100) == 7);        // (42 * 3877 + 29573) % 100
    CHECK(r(100) == 12);
    CHECK(r(100) == 89);
    CHECK(r(1000000) == 494550);
    CHECK(r(4) == 3);
    CHECK(r(10) == 8);
    CHECK(r(0) == 0);          // g_Random(0) is 0 and leaves the seed alone
    const std::uint32_t s = r.seed();
    CHECK(r.random(-5) == 0);
    CHECK(r.seed() == s);
    CHECK(r.between(7, 7) == 7);
    r.seed(1);
    CHECK(r.random(100) == 50);
    CHECK(r.between(10, 20) >= 10);
}

TEST_CASE("the magic index lists a row under every type, series and level it allows (m_CMAIT)", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    REQUIRE(set.magic().size() == 6);
    using V = std::vector<int>;
    // prefixes for a melee weapon: rows 1 and 2 at level 1, row 3 (series 2, level 3) from level 3 on
    REQUIRE(set.magic_candidates(1, jx::zone::equip_meleeweapon, 2, 1) != nullptr);
    CHECK((*set.magic_candidates(1, jx::zone::equip_meleeweapon, 2, 1) == V{0, 1}));
    CHECK((*set.magic_candidates(1, jx::zone::equip_meleeweapon, 2, 3) == V{0, 1, 2}));
    CHECK((*set.magic_candidates(1, jx::zone::equip_meleeweapon, 2, 10) == V{0, 1, 2}));
    CHECK((*set.magic_candidates(1, jx::zone::equip_meleeweapon, 0, 3) == V{0, 1}));   // series 0 never sees row 3
    // suffixes: an armor has only "cua Manh"; a ring has only the forbidden prefix
    CHECK((*set.magic_candidates(0, jx::zone::equip_armor, 4, 1) == V{3}));
    CHECK((*set.magic_candidates(0, jx::zone::equip_meleeweapon, 1, 1) == V{3, 4}));
    CHECK((*set.magic_candidates(1, jx::zone::equip_ring, 0, 1) == V{5}));
    CHECK(set.magic_candidates(0, jx::zone::equip_ring, 0, 1)->empty());
    // outside the table: NULL of GetCMIT
    CHECK(set.magic_candidates(2, 0, 0, 1) == nullptr);
    CHECK(set.magic_candidates(1, jx::zone::kMagicTypes, 0, 1) == nullptr);
    CHECK(set.magic_candidates(1, 0, 5, 1) == nullptr);
    CHECK(set.magic_candidates(1, 0, 0, 0) == nullptr);
    CHECK(set.magic_candidates(1, 0, 0, 11) == nullptr);
    // the limit table
    REQUIRE(set.magic_limit(139) != nullptr);
    CHECK(set.magic_limit(139)->max[0] == 0);
    CHECK(set.magic_limit(100) == nullptr);
}

TEST_CASE("Gen_MagicAttrib: prefixes on even slots, suffixes on odd ones, no kind twice, values inside the ranges", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    KItemGenerator gen(set, 0, 5);
    const jx::zone::KMagicLevels four{1, 1, 1, 1, 0, 0};
    int first_kind_100 = 0, first_kind_101 = 0, with_three = 0;
    for (int i = 0; i < 200; ++i) {
        auto item = gen.equipment(jx::zone::equip_meleeweapon, 0, 2, 1, &four, 0);
        REQUIRE(item.has_value());
        // slot 0 is a prefix of kind 100 or 101, slot 1 the suffix "cua Manh" (200) or "cua Sac" (100)
        CHECK((item->magic[0].type == 100 || item->magic[0].type == 101));
        CHECK((item->magic[1].type == 200 || item->magic[1].type == 100));
        if (item->magic[0].type == 100) ++first_kind_100;
        if (item->magic[0].type == 101) ++first_kind_101;
        std::set<int> kinds;
        int n = 0;
        for (const auto& a : item->magic) {
            if (a.type == 0) continue;
            CHECK(kinds.insert(a.type).second);   // a kind goes on a piece once
            CHECK(a.value[1] == -1);
            CHECK(a.value[2] == 0);
            if (a.type == 101) {
                CHECK(a.value[0] >= 10);
                CHECK(a.value[0] <= 20);
            }
            if (a.type == 200) CHECK(a.value[0] == 1);
            ++n;
        }
        CHECK(n >= 2);   // the first prefix and the first suffix always have a candidate
        CHECK(n <= 3);   // two prefix kinds and two suffix rows sharing a kind with one of them: three at most
        CHECK(item->magic[3].type == 0);
        if (n == 3) ++with_three;
    }
    CHECK(first_kind_100 > 30);
    CHECK(first_kind_101 > 30);
    CHECK(with_three > 30);
    // a prefix of kind 100 on slot 0 rules the suffix "cua Sac" (kind 100) out
    // an armor: "Ben" then "cua Manh", then nothing is left for a second prefix
    const jx::zone::KMagicLevels six{1, 1, 1, 1, 1, 1};
    for (int i = 0; i < 20; ++i) {
        auto armor = gen.equipment(jx::zone::equip_armor, 0, 0, 1, &six, 0);
        REQUIRE(armor.has_value());
        CHECK(armor->magic[0].type == 101);
        CHECK(armor->magic[1].type == 200);
        CHECK(armor->magic[2].type == 0);
    }
    // no levels: a white piece; a mask would be white too
    auto white = gen.equipment(jx::zone::equip_meleeweapon, 0, 2, 1);
    REQUIRE(white.has_value());
    CHECK(white->magic[0].type == 0);
    const jx::zone::KMagicLevels none{};
    auto still_white = gen.equipment(jx::zone::equip_meleeweapon, 0, 2, 1, &none, 0);
    REQUIRE(still_white.has_value());
    CHECK(still_white->magic[0].type == 0);
    // the ring: its only prefix (allskill_v 139) is forbidden by magicattrib_limit -> after 21 tries no item
    const jx::zone::KMagicLevels one{1, 0, 0, 0, 0, 0};
    CHECK_FALSE(gen.equipment(jx::zone::equip_ring, 0, 0, 1, &one, 0).has_value());
    CHECK(gen.equipment(jx::zone::equip_ring, 0, 0, 1).has_value());
    // the check itself
    std::array<jx::zone::KMagicAttrib, 6> magic{};
    magic[0].type = 139;
    magic[0].value = {0, 0, 0};
    CHECK_FALSE(gen.check_new_item_attrib(magic));
    magic[0].type = 100;
    CHECK(gen.check_new_item_attrib(magic));
}

TEST_CASE("the inventory grid places, refuses overlap and finds room column by column", "[item]")
{
    KInventory inv(6, 10);
    CHECK(inv.place(0, 0, 1, 1, 3));
    CHECK_FALSE(inv.place(0, 2, 2, 1, 1));   // the sword's bottom cell
    CHECK(inv.place(0, 3, 2, 2, 3));
    CHECK_FALSE(inv.place(5, 8, 3, 2, 3));   // out of the grid
    CHECK(inv.at(1, 4) == 2);
    int x = -1, y = -1;
    REQUIRE(inv.find_room(1, 1, x, y));
    CHECK((x == 0 && y == 6));               // first free cell of column 0, like KInventory::FindRoom
    REQUIRE(inv.find_room(2, 3, x, y));
    CHECK((x == 0 && y == 6));
    CHECK_FALSE(inv.pick_up(2, 0, 0, 2, 3));  // another item's cells: refused (KInventory::PickUpItem checks every cell)
    CHECK(inv.pick_up(2, 0, 3, 2, 3));
    CHECK(inv.at(1, 4) == 0);
    CHECK_FALSE(inv.find_room(7, 1, x, y));
}

TEST_CASE("a player's list: add, stack, move, swap, equip with requirements, unequip", "[item]")
{
    const KItemTemplateSet set = load_test_set();
    KItemGenerator gen(set, 0, 3);
    KItemList list;
    const auto sword = list.add(*gen.equipment(jx::zone::equip_meleeweapon, 0, 3, 1), jx::zone::room_equipment);
    REQUIRE(sword != 0);
    CHECK(list.place_of(sword)->room == jx::zone::room_equipment);
    CHECK(list.item_at(jx::zone::room_equipment, 0, 2) == sword);   // 1 x 3 from the first free column
    const auto armor = list.add(*gen.equipment(jx::zone::equip_armor, 0, 0, 1), jx::zone::room_equipment);
    REQUIRE(armor != 0);
    CHECK(list.place_of(armor)->x == 0);
    CHECK(list.place_of(armor)->y == 3);
    // stacks: two bags of 15 join into one of 20 and one of 10
    int into = 0;
    const auto bag1 = list.add_or_stack(*gen.quest(1, 15), jx::zone::room_equipment, &into);
    const auto bag2 = list.add_or_stack(*gen.quest(1, 15), jx::zone::room_equipment, &into);
    CHECK(into == static_cast<int>(bag1));
    CHECK(bag2 != bag1);
    CHECK(list.find(bag1)->count == 20);
    CHECK(list.find(bag2)->count == 10);
    CHECK(list.size() == 4);

    // move to a free place, refuse an occupied one, swap two
    CHECK(list.move(sword, jx::zone::room_repository, 2, 2));
    CHECK(list.item_at(jx::zone::room_equipment, 0, 0) == 0);
    CHECK(list.item_at(jx::zone::room_repository, 2, 4) == sword);
    CHECK_FALSE(list.move(armor, jx::zone::room_repository, 2, 3));   // overlaps the sword
    CHECK(list.place_of(armor)->room == jx::zone::room_equipment);     // untouched
    CHECK(list.swap(bag1, bag2));
    CHECK(list.find(bag1)->count == 20);

    // equipment: level 5 required for the sword, the player is level 3
    const auto level3 = [](int type) { return type == jx::zone::magic_requirelevel ? 3 : (type == jx::zone::magic_requiresex ? 1 : 50); };
    const auto level9 = [](int type) { return type == jx::zone::magic_requirelevel ? 9 : (type == jx::zone::magic_requiresex ? 1 : 50); };
    CHECK_FALSE(list.can_equip(*list.find(sword), jx::zone::itempart_weapon, level3));
    CHECK_FALSE(list.equip(sword, jx::zone::itempart_weapon, level3));
    CHECK(list.can_equip(*list.find(sword), jx::zone::itempart_weapon, level9));
    CHECK_FALSE(list.can_equip(*list.find(sword), jx::zone::itempart_head, level9));   // wrong part
    CHECK(list.equip(sword, jx::zone::itempart_weapon, level9));
    CHECK(list.equipped(jx::zone::itempart_weapon) == sword);
    CHECK(list.place_of(sword)->room == jx::zone::room_body);
    CHECK(list.item_at(jx::zone::room_repository, 2, 2) == 0);
    CHECK(list.weapon_damage().first == 4);
    CHECK(list.weapon_type() == jx::zone::equip_meleeweapon);
    // the armor wants sex 1: a man (0) cannot wear it
    const auto man = [](int type) { return type == jx::zone::magic_requiresex ? 0 : 50; };
    CHECK_FALSE(list.equip(armor, jx::zone::itempart_body, man));
    CHECK(list.equip(armor, jx::zone::itempart_body, level9));
    CHECK(list.armor_defense() == 50);
    // GetEquipEnhance: the sword is fire (3) - a wood (1) character feeds it, a metal (0) one does
    // not; the body's activating parts (ring2, belt) are empty; the horse and beyond count 3
    CHECK(list.equip_enhance(jx::zone::itempart_weapon, 1) == 1);
    CHECK(list.equip_enhance(jx::zone::itempart_weapon, 0) == 0);
    CHECK(list.equip_enhance(jx::zone::itempart_body, 4) == 1);   // earth feeds the metal armor
    CHECK(list.equip_enhance(jx::zone::itempart_body, 0) == 0);
    CHECK(list.equip_enhance(jx::zone::itempart_horse, 0) == 3);
    CHECK(list.equip_enhance(jx::zone::itempart_head, 0) == 0);   // nothing worn there
    CHECK(list.equip_enhance(99, 0) == 0);
    // a second sword goes on the hand and the first comes back to the bag
    const auto sword2 = list.add(*gen.equipment(jx::zone::equip_meleeweapon, 0, 3, 2), jx::zone::room_equipment);
    CHECK(list.equip(sword2, jx::zone::itempart_weapon, level9));
    CHECK(list.equipped(jx::zone::itempart_weapon) == sword2);
    CHECK(list.place_of(sword)->room == jx::zone::room_equipment);
    CHECK(list.weapon_damage().first == 6);
    CHECK(list.unequip(jx::zone::itempart_weapon));
    CHECK(list.equipped(jx::zone::itempart_weapon) == 0);
    CHECK(list.weapon_type() == -1);
    // a ring fits either ring slot
    const auto ring = list.add(*gen.equipment(jx::zone::equip_ring, 0, 0, 1), jx::zone::room_equipment);
    CHECK(list.equip(ring, jx::zone::itempart_ring2, level9));
    // remove frees the cells and ids keep growing
    const auto n = list.next_id();
    CHECK(list.remove(bag2));
    CHECK(list.find(bag2) == nullptr);
    CHECK(list.next_id() == n);
    // money
    CHECK(list.add_money(jx::zone::room_equipment, 300));
    CHECK(list.cost_money(120));
    CHECK_FALSE(list.cost_money(1000));
    CHECK(list.money() == 180);
}

TEST_CASE("the exported item tables load when they are there", "[item]")
{
    const std::filesystem::path p = std::filesystem::path(JX_NEXT_DIR) / "client" / "assets" / "items" / "v000.json";
    if (!std::filesystem::exists(p)) {
        WARN("no exported item tables at " << p.string() << " (python tools/dev.py assets)");
        return;
    }
    KItemTemplateSet set;
    std::string error;
    REQUIRE(set.load(p.string(), &error));
    CHECK(set.size() > 1000);
    REQUIRE(set.equipment(jx::zone::equip_meleeweapon, 0, 1) != nullptr);
    CHECK(!set.equipment(jx::zone::equip_meleeweapon, 0, 1)->name.empty());
    REQUIRE(set.gold(1) != nullptr);
    KItemGenerator gen(set, 0, 1);
    auto g = gen.gold(100, 1);
    REQUIRE(g.has_value());
    CHECK(g->group > 0);
    CHECK(g->magic[0].type != 0);
}

TEST_CASE("the real 004 tables: swords of level 5 roll prefixes and suffixes, never the forbidden allskill_v", "[item]")
{
    const std::filesystem::path p = std::filesystem::path(JX_NEXT_DIR) / "client" / "assets" / "items" / "v004.json";
    if (!std::filesystem::exists(p)) {
        WARN("no exported item tables at " << p.string() << " (python tools/dev.py assets)");
        return;
    }
    KItemTemplateSet set;
    std::string error;
    REQUIRE(set.load(p.string(), &error));
    REQUIRE(set.magic_limit(139) != nullptr);   // "no more +N all skills" of magicattrib_limit.txt
    REQUIRE(set.magic_candidates(1, jx::zone::equip_meleeweapon, 0, 5) != nullptr);
    CHECK(set.magic_candidates(1, jx::zone::equip_meleeweapon, 0, 5)->size() > 5);
    KItemGenerator gen(set, 4, 99);   // version 4: g_Random(1000000) * 100 / (10 * luck + 100) against the 004 rates
    const jx::zone::KMagicLevels six{5, 5, 5, 5, 5, 5};
    int made = 0, with_magic = 0, with_four = 0;
    for (int i = 0; i < 300; ++i) {
        auto item = gen.equipment(jx::zone::equip_meleeweapon, 0, 0, 5, &six, 50);
        if (!item) continue;   // the limit rejected 21 rolls: rare, allowed
        ++made;
        std::set<int> kinds;
        int n = 0;
        for (std::size_t k = 0; k < item->magic.size(); ++k) {
            const auto& a = item->magic[k];
            if (a.type == 0) {
                for (std::size_t m = k; m < item->magic.size(); ++m) CHECK(item->magic[m].type == 0);   // the list ends
                break;
            }
            CHECK(a.type != 139);
            CHECK(kinds.insert(a.type).second);
            // the value lies inside the range of some row of that kind at that position
            bool inside = false;
            for (const auto& row : set.magic()) {
                if (row.kind == a.type && row.pos == 1 - static_cast<int>(k & 1) && a.value[0] >= row.ranges[0].first &&
                    a.value[0] <= row.ranges[0].second) {
                    inside = true;
                }
            }
            CHECK(inside);
            ++n;
        }
        if (n > 0) ++with_magic;
        if (n >= 4) ++with_four;
    }
    CHECK(made > 250);
    CHECK(with_magic > made / 2);
    CHECK(with_four > 0);
}

TEST_CASE("a character's items survive spawn -> snapshot -> spawn, worn pieces included", "[item][world]")
{
    jx::log::Options lo;
    lo.console = false;
    lo.default_level = jx::log::Level::warn;
    jx::log::init(lo);
    auto lib = std::make_shared<jx::zone::KItemLibrary>();
    {
        KItemTemplateSet set;
        std::string error;
        REQUIRE(set.load(write_tables(), &error));
        lib->add(3, std::move(set));
    }
    jx::zone::KSubWorldConfig cfg;
    cfg.zone_id = 1;
    cfg.width = cfg.height = 4096;
    cfg.spawn_point = jx::zone::Pos{2000, 2000};
    cfg.items = lib;
    jx::zone::KSubWorld w(cfg);

    jx::pb::RoleData role;
    role.set_player_id(11);
    role.set_name("A");
    role.set_level(9);
    role.set_money(500);
    role.set_bank_money(70);
    jx::EntityId id;
    jx::zone::Pos p;
    REQUIRE(w.spawn_player(1, role, id, p) == jx::pb::RESULT_OK);
    auto gen = w.item_generator();
    REQUIRE(gen.has_value());
    KItemList* list = w.items_of(1);
    REQUIRE(list != nullptr);
    CHECK(list->money() == 500);
    const auto sword = list->add(*gen->equipment(jx::zone::equip_meleeweapon, 0, 2, 1), jx::zone::room_equipment);
    const auto armor = list->add(*gen->equipment(jx::zone::equip_armor, 0, 0, 1), jx::zone::room_equipment);
    const auto bag = list->add_or_stack(*gen->quest(1, 7), jx::zone::room_repository, nullptr);
    const auto goldpiece = list->add(*gen->gold(50, 1), jx::zone::room_equipment);
    REQUIRE((sword && armor && bag && goldpiece));
    const auto level9 = [](int type) { return type == jx::zone::magic_requirelevel ? 9 : 50; };   // level 9, plenty of strength
    REQUIRE(list->equip(sword, jx::zone::itempart_weapon, level9));
    const int magic0 = list->find(goldpiece)->magic[0].value[0];

    jx::pb::RoleData saved;
    REQUIRE(w.role_snapshot(1, saved));
    CHECK(saved.items_size() == 4);
    CHECK(saved.money() == 500);
    CHECK(saved.bank_money() == 70);
    CHECK(saved.next_item_id() == list->next_id());
    const std::uint32_t next_before = list->next_id();   // `list` dies with the player below (its KItemList is erased)
    REQUIRE(w.remove_player(1));

    // the same character comes back: same ids, same places, same rolled values
    REQUIRE(w.spawn_player(2, saved, id, p) == jx::pb::RESULT_OK);
    const KItemList* again = w.items_of(2);
    REQUIRE(again != nullptr);
    CHECK(again->size() == 4);
    CHECK(again->equipped(jx::zone::itempart_weapon) == sword);
    CHECK(again->place_of(sword)->room == jx::zone::room_body);
    CHECK(again->place_of(bag)->room == jx::zone::room_repository);
    CHECK(again->find(bag)->count == 7);
    CHECK(again->find(goldpiece)->magic[0].value[0] == magic0);   // no re-roll
    CHECK(again->find(goldpiece)->name() == "Hoang kim");
    CHECK(again->find(sword)->durability == 20);
    CHECK(again->next_id() == next_before);
    CHECK(again->money(jx::zone::room_repository) == 70);

    // an item whose table set is gone is dropped, the rest stays
    jx::pb::RoleData broken = saved;
    broken.mutable_items(0)->set_version(9);
    REQUIRE(w.remove_player(2));
    REQUIRE(w.spawn_player(3, broken, id, p) == jx::pb::RESULT_OK);
    CHECK(w.items_of(3)->size() == 3);
}

// ---- the item protocol of the world (M11 C1) -------------------------------------------------

namespace {

template <class Msg>
Msg decode_packet(const jx::zone::Packet& p)
{
    Msg m;
    REQUIRE(m.ParseFromString(p.payload));
    return m;
}

std::vector<jx::zone::Packet> packets(const std::vector<jx::zone::Packet>& all, std::uint64_t sid, jx::pb::MsgId id)
{
    std::vector<jx::zone::Packet> out;
    for (const auto& p : all) {
        if (p.msg_id == id && std::find(p.sids.begin(), p.sids.end(), sid) != p.sids.end()) out.push_back(p);
    }
    return out;
}

struct ItemWorld {
    std::shared_ptr<jx::zone::KItemLibrary> lib = std::make_shared<jx::zone::KItemLibrary>();
    std::unique_ptr<jx::zone::KSubWorld> w;
    jx::EntityId id;
    jx::zone::Pos p;

    explicit ItemWorld(std::shared_ptr<const jx::zone::KAbradeRate> abrade = nullptr,
                       std::shared_ptr<const jx::zone::KItemChangeRes> item_res = nullptr)
    {
        jx::log::Options lo;
        lo.console = false;
        lo.default_level = jx::log::Level::warn;
        jx::log::init(lo);
        KItemTemplateSet set;
        std::string error;
        REQUIRE(set.load(write_tables(), &error));
        lib->add(1, std::move(set));
        jx::zone::KSubWorldConfig cfg;
        cfg.zone_id = 1;
        cfg.width = cfg.height = 4096;
        cfg.spawn_point = jx::zone::Pos{2000, 2000};
        cfg.items = lib;
        cfg.abrade_rate = std::move(abrade);
        cfg.item_res = std::move(item_res);
        w = std::make_unique<jx::zone::KSubWorld>(cfg);
        jx::pb::RoleData role;
        role.set_player_id(11);
        role.set_name("A");
        role.set_level(9);
        role.mutable_stats()->set_strength(25);   // the level-2 sword needs strength 20 (a fresh Shaolin has 35)
        role.mutable_stats()->set_dexterity(25);
        role.mutable_stats()->set_hp_max(204);
        role.set_money(500);
        REQUIRE(w->spawn_player(1, role, id, p) == jx::pb::RESULT_OK);
    }
    KItemList& list() { return *w->items_of(1); }
    jx::zone::KItemGenerator gen() { return *w->item_generator(); }
};

} // namespace

TEST_CASE("entering the world brings the whole item list, then every change is told once", "[item][world]")
{
    ItemWorld iw;
    auto out = iw.w->take_outbox();
    const auto lists = packets(out, 1, jx::pb::G2C_ITEM_LIST);
    REQUIRE(lists.size() == 1);
    auto sync = decode_packet<jx::pb::InventorySync>(lists[0]);
    CHECK(sync.items_size() == 0);
    CHECK(sync.money() == 500);

    // a script gives a sword: G2C_ITEM_ADD carries the whole view (name, size, attributes)
    auto g = iw.gen();
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));
    REQUIRE(sword != 0);
    out = iw.w->take_outbox();
    const auto adds = packets(out, 1, jx::pb::G2C_ITEM_ADD);
    REQUIRE(adds.size() == 1);
    const auto view = decode_packet<jx::pb::ItemAdd>(adds[0]).item();
    CHECK(view.id() == sword);
    CHECK(view.name() == "Kiem 1");
    CHECK(view.width() == 1);
    CHECK(view.height() == 3);
    CHECK(view.room() == jx::zone::room_equipment);
    CHECK(view.max_durability() == 20);
    CHECK(view.base_size() == 3);
    CHECK(view.require_size() == 2);
    CHECK(view.price() == 100);

    // the next entry brings the sword in the list
    jx::pb::RoleData saved;
    REQUIRE(iw.w->role_snapshot(1, saved));
    REQUIRE(iw.w->remove_player(1));
    jx::EntityId id2;
    jx::zone::Pos p2;
    REQUIRE(iw.w->spawn_player(2, saved, id2, p2) == jx::pb::RESULT_OK);
    out = iw.w->take_outbox();
    const auto lists2 = packets(out, 2, jx::pb::G2C_ITEM_LIST);
    REQUIRE(lists2.size() == 1);
    sync = decode_packet<jx::pb::InventorySync>(lists2[0]);
    REQUIRE(sync.items_size() == 1);
    CHECK(sync.items(0).id() == sword);
    CHECK(sync.items(0).name() == "Kiem 1");
}

TEST_CASE("a move request: free cells, an item under the target trades places, refusals carry the seq", "[item][world]")
{
    ItemWorld iw;
    auto g = iw.gen();
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));   // 1 x 3 at (0,0)
    const auto ring = iw.w->give_item(1, *g.equipment(jx::zone::equip_ring, 0, 0, 1));           // 1 x 1, column by column: (0,3)
    REQUIRE((sword && ring));
    REQUIRE(iw.list().place_of(sword)->x == 0);
    REQUIRE(iw.list().place_of(ring)->x == 0);
    REQUIRE(iw.list().place_of(ring)->y == 3);
    iw.w->take_outbox();

    // to a free spot
    REQUIRE(iw.w->item_move_request(1, ring, jx::zone::room_equipment, 4, 5, 21));
    auto out = iw.w->take_outbox();
    auto moves = packets(out, 1, jx::pb::G2C_ITEM_MOVE);
    REQUIRE(moves.size() == 1);
    auto mv = decode_packet<jx::pb::ItemMove>(moves[0]);
    CHECK(mv.id() == ring);
    CHECK(mv.x() == 4);
    CHECK(mv.y() == 5);
    CHECK(mv.seq() == 21);

    // onto the sword's cells: the sword takes the ring's old place (it fits there: 1 x 3 at (4,5))
    REQUIRE(iw.w->item_move_request(1, ring, jx::zone::room_equipment, 0, 1, 22));
    out = iw.w->take_outbox();
    moves = packets(out, 1, jx::pb::G2C_ITEM_MOVE);
    REQUIRE(moves.size() == 2);
    CHECK(iw.list().place_of(ring)->x == 0);
    CHECK(iw.list().place_of(ring)->y == 1);
    CHECK(iw.list().place_of(sword)->x == 4);
    CHECK(iw.list().place_of(sword)->y == 5);

    // outside the grid: refused with the seq and a reason; nothing moved
    REQUIRE_FALSE(iw.w->item_move_request(1, ring, jx::zone::room_equipment, 6, 0, 23));
    REQUIRE_FALSE(iw.w->item_move_request(1, sword, jx::zone::room_equipment, 5, 9, 24));   // 1 x 3 would hang out
    out = iw.w->take_outbox();
    auto results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 2);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).seq() == 23);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_FULL);
    CHECK(decode_packet<jx::pb::ItemResult>(results[1]).seq() == 24);
    CHECK(packets(out, 1, jx::pb::G2C_ITEM_MOVE).empty());

    // an unknown item, the trade box out of a trade (WRONG_STATE: the box opens with the trade, [trade]), the quick slots for a sword
    REQUIRE_FALSE(iw.w->item_move_request(1, 999, jx::zone::room_equipment, 0, 0, 25));
    REQUIRE_FALSE(iw.w->item_move_request(1, ring, jx::zone::room_trade, 0, 0, 26));
    REQUIRE_FALSE(iw.w->item_move_request(1, sword, jx::zone::room_immediacy, 0, 0, 27));
    out = iw.w->take_outbox();
    results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 3);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_NOT_FOUND);
    CHECK(decode_packet<jx::pb::ItemResult>(results[1]).result() == jx::pb::RESULT_WRONG_STATE);
    CHECK(decode_packet<jx::pb::ItemResult>(results[2]).result() == jx::pb::RESULT_BAD_REQUEST);

    // the repository is reachable; the quick slots take one medicine of a kind
    REQUIRE(iw.w->item_move_request(1, ring, jx::zone::room_repository, 2, 2, 28));
    const auto med1 = iw.w->give_item(1, *g.medicine(0, 1));
    const auto med2 = iw.w->give_item(1, *g.medicine(0, 1));
    REQUIRE((med1 && med2 && med1 != med2));
    REQUIRE(iw.w->item_move_request(1, med1, jx::zone::room_immediacy, 0, 0, 29));
    REQUIRE_FALSE(iw.w->item_move_request(1, med2, jx::zone::room_immediacy, 1, 0, 30));   // same detail type already there
    out = iw.w->take_outbox();
    results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 1);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).seq() == 30);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_ALREADY_EXISTS);
}

TEST_CASE("equip and unequip requests: the part its kind goes to, requirements, the piece that comes off", "[item][world]")
{
    ItemWorld iw;   // level 9, series 0, sex 0
    auto g = iw.gen();
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));   // needs level 5, strength 20
    const auto sword2 = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 2));
    const auto armor = iw.w->give_item(1, *g.equipment(jx::zone::equip_armor, 0, 0, 1));         // needs sex 1
    REQUIRE((sword && sword2 && armor));
    iw.w->take_outbox();

    REQUIRE(iw.w->item_equip_request(1, sword, -1, 31));
    CHECK(iw.list().equipped(jx::zone::itempart_weapon) == sword);
    auto out = iw.w->take_outbox();
    auto moves = packets(out, 1, jx::pb::G2C_ITEM_MOVE);
    REQUIRE(moves.size() == 1);
    CHECK(decode_packet<jx::pb::ItemMove>(moves[0]).room() == jx::zone::room_body);
    CHECK(decode_packet<jx::pb::ItemMove>(moves[0]).x() == jx::zone::itempart_weapon);

    // the second sword replaces the first, which goes back to the bag: two moves
    REQUIRE(iw.w->item_equip_request(1, sword2, jx::zone::itempart_weapon, 32));
    CHECK(iw.list().equipped(jx::zone::itempart_weapon) == sword2);
    CHECK(iw.list().place_of(sword)->room == jx::zone::room_equipment);
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_MOVE).size() == 2);

    // wrong sex for the armor, a sword on the head: refused
    REQUIRE_FALSE(iw.w->item_equip_request(1, armor, -1, 33));
    REQUIRE_FALSE(iw.w->item_equip_request(1, sword, jx::zone::itempart_head, 34));
    out = iw.w->take_outbox();
    auto results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 2);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_BAD_REQUEST);

    REQUIRE(iw.w->item_unequip_request(1, jx::zone::itempart_weapon, 35));
    CHECK(iw.list().equipped(jx::zone::itempart_weapon) == 0);
    REQUIRE_FALSE(iw.w->item_unequip_request(1, jx::zone::itempart_weapon, 36));   // nothing there now
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_MOVE).size() == 1);
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_RESULT).size() == 1);
    CHECK(decode_packet<jx::pb::ItemResult>(packets(out, 1, jx::pb::G2C_ITEM_RESULT)[0]).result() == jx::pb::RESULT_NOT_FOUND);
}

// KNpc::SetHorse 0x0807D520 through the equip 0x081FE380 (part 10 -> 1) / unequip 0x081FFFB0 (-> 0) of the horse, the ride
// toggle 0x080AEFA0, KPlayer::ReCalcEquip 0x080AF4EA (the horse counts only while ridden), frozen_action +0x1479
TEST_CASE("a horse: worn = ridden, its defence counts only while riding, the ride toggle, frozen_action", "[item][world]")
{
    ItemWorld iw;
    auto g = iw.gen();
    const auto horse = iw.w->give_item(1, *g.equipment(jx::zone::equip_horse, 0, 1, 1));
    REQUIRE(horse);
    jx::zone::KNpc* me = iw.w->mutable_entity(iw.id);
    REQUIRE(me != nullptr);
    const int defend = me->cur.defend;
    CHECK(me->horse == 0);
    iw.w->take_outbox();
    REQUIRE(iw.w->item_equip_request(1, horse, -1, 41));
    me = iw.w->mutable_entity(iw.id);
    CHECK(iw.list().equipped(jx::zone::itempart_horse) == horse);
    CHECK(me->horse == 1);
    CHECK(me->cur.defend == defend + 7);
    auto out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ENTITY_RIDE).size() == 1);
    CHECK(decode_packet<jx::pb::EntityRide>(packets(out, 1, jx::pb::G2C_ENTITY_RIDE)[0]).riding());
    // dismount: the horse stays worn but gives nothing (0x080AF4EA)
    REQUIRE(iw.w->ride_request(1, false, 42));
    me = iw.w->mutable_entity(iw.id);
    CHECK(me->horse == 0);
    CHECK(iw.list().equipped(jx::zone::itempart_horse) == horse);
    CHECK(me->cur.defend == defend);
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ENTITY_RIDE).size() == 1);
    CHECK_FALSE(decode_packet<jx::pb::EntityRide>(packets(out, 1, jx::pb::G2C_ENTITY_RIDE)[0]).riding());
    // the same state again: nothing (0x080AF060)
    CHECK_FALSE(iw.w->ride_request(1, false, 43));
    CHECK(packets(iw.w->take_outbox(), 1, jx::pb::G2C_ENTITY_RIDE).empty());
    // mount again; frozen_action (+0x1479) locks SetHorse both ways
    REQUIRE(iw.w->ride_request(1, true, 44));
    CHECK(iw.w->mutable_entity(iw.id)->horse == 1);
    CHECK(iw.w->mutable_entity(iw.id)->cur.defend == defend + 7);
    iw.w->mutable_entity(iw.id)->cur.frozen_action = true;
    CHECK_FALSE(iw.w->ride_request(1, false, 45));
    CHECK(iw.w->mutable_entity(iw.id)->horse == 1);
    iw.w->mutable_entity(iw.id)->cur.frozen_action = false;
    // sitting refuses the toggle (0x080AEFA0: m_Doing == 8) and riding refuses the sit (0x080DC367)
    REQUIRE(iw.w->ride_request(1, false, 145));
    REQUIRE(iw.w->sit_request(1, true, 146));
    CHECK(iw.w->mutable_entity(iw.id)->doing == jx::zone::KDoing::sit);
    CHECK_FALSE(iw.w->ride_request(1, true, 147));
    CHECK(iw.w->mutable_entity(iw.id)->horse == 0);
    REQUIRE(iw.w->sit_request(1, false, 148));
    REQUIRE(iw.w->ride_request(1, true, 149));
    CHECK(iw.w->mutable_entity(iw.id)->horse == 1);
    CHECK_FALSE(iw.w->sit_request(1, true, 150));
    CHECK(iw.w->mutable_entity(iw.id)->doing != jx::zone::KDoing::sit);
    // the horse comes off: dismounted (0x08200311); without a horse the toggle is refused (0x080AF066)
    REQUIRE(iw.w->item_unequip_request(1, jx::zone::itempart_horse, 46));
    CHECK(iw.w->mutable_entity(iw.id)->horse == 0);
    CHECK(iw.w->mutable_entity(iw.id)->cur.defend == defend);
    CHECK_FALSE(iw.w->ride_request(1, true, 47));
    // a character that logs in with the horse worn is riding (0x080C1F83 + the equip pass)
    REQUIRE(iw.w->item_equip_request(1, horse, -1, 48));
    jx::pb::RoleData saved;
    REQUIRE(iw.w->role_snapshot(1, saved));
    REQUIRE(iw.w->remove_player(1));
    jx::EntityId again;
    jx::zone::Pos at;
    REQUIRE(iw.w->spawn_player(1, saved, again, at) == jx::pb::RESULT_OK);
    CHECK(iw.w->mutable_entity(again)->horse == 1);
    CHECK(iw.w->mutable_entity(again)->cur.defend == defend + 7);
}

TEST_CASE("eating a medicine: LifePotionV merges, heals every 10 frames, the item goes or its stack shrinks", "[item][world]")
{
    ItemWorld iw;
    auto g = iw.gen();
    jx::zone::KNpc* me = const_cast<jx::zone::KNpc*>(iw.w->find_player(1));
    REQUIRE(me != nullptr);
    me->cur.life_max = me->cur.life_max_yan = 1000;
    me->cur.life = 100;
    me->cur.life_replenish = 0;
    const auto med = iw.w->give_item(1, *g.medicine(0, 1));   // 10 life every 10 frames for 100 frames
    REQUIRE(med != 0);
    iw.w->take_outbox();

    REQUIRE(iw.w->item_use_request(1, med, 41));
    CHECK(me->life_state.value == 10);
    CHECK(me->life_state.time == 100);
    auto out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_REMOVE).size() == 1);   // not stackable: the potion is gone
    CHECK(iw.list().find(med) == nullptr);

    // KNpc::ProcessState: time-- each frame, +value when time % 10 == 0 -> the first heal after 10 frames
    for (int i = 0; i < 9; ++i) iw.w->tick();
    CHECK(me->life() == 100);
    iw.w->tick();
    CHECK(me->life() == 110);
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ENTITY_LIFE).size() == 1);
    CHECK(decode_packet<jx::pb::EntityLife>(packets(out, 1, jx::pb::G2C_ENTITY_LIFE)[0]).delta() == 10);
    for (int i = 0; i < 90; ++i) iw.w->tick();
    CHECK(me->life() == 200);   // 10 heals in all
    CHECK(me->life_state.time == 0);
    iw.w->tick();
    CHECK(me->life() == 200);   // and no more

    // a second potion while one works: time = max, value = weighted (KNpcAttribModify::LifePotionV)
    const auto a = iw.w->give_item(1, *g.medicine(0, 1));    // 10 x 100
    const auto b = iw.w->give_item(1, *g.medicine(0, 2));    // 30 x 50 (stackable), the third attribute is beyond what the server reads
    REQUIRE((a && b));
    CHECK(iw.list().find(b)->base[2].empty());
    REQUIRE(iw.w->item_use_request(1, a, 42));
    REQUIRE(iw.w->item_use_request(1, b, 43));
    CHECK(me->life_state.time == 100);
    CHECK(me->life_state.value == (10 * 100 + 30 * 50) / 100);
    CHECK(iw.list().find(b) == nullptr);   // a stack of one is the last one

    // the percent of the Linux server: 50% halves the heal
    me->cur.life_replenish_percent = 50;
    me->cur.life = 100;
    me->life_state = {20, 10};
    for (int i = 0; i < 10; ++i) iw.w->tick();
    CHECK(me->life() == 110);

    // a stack of three: one goes each time, the client sees the new count, then the removal
    KItem stack = *g.medicine(0, 2);
    stack.count = 3;
    const auto c = iw.w->give_item(1, stack);
    REQUIRE(c != 0);
    iw.w->take_outbox();
    REQUIRE(iw.w->item_use_request(1, c, 44));
    CHECK(iw.list().find(c)->count == 2);
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    CHECK(decode_packet<jx::pb::ItemAdd>(packets(out, 1, jx::pb::G2C_ITEM_ADD)[0]).item().count() == 2);
    REQUIRE(iw.w->item_use_request(1, c, 45));
    REQUIRE(iw.w->item_use_request(1, c, 46));
    CHECK(iw.list().find(c) == nullptr);

    // forbit_takemedicine, a dead player, a sword: refused
    const auto d = iw.w->give_item(1, *g.medicine(0, 1));
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));
    iw.w->take_outbox();
    me->cur.forbid_medicine = true;
    REQUIRE_FALSE(iw.w->item_use_request(1, d, 47));
    me->cur.forbid_medicine = false;
    REQUIRE_FALSE(iw.w->item_use_request(1, sword, 48));
    REQUIRE_FALSE(iw.w->item_drop_request(1, sword, 49));   // no ground yet (M11 D)
    out = iw.w->take_outbox();
    const auto results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 3);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_WRONG_STATE);
    CHECK(decode_packet<jx::pb::ItemResult>(results[1]).result() == jx::pb::RESULT_BAD_REQUEST);
    CHECK(decode_packet<jx::pb::ItemResult>(results[2]).result() == jx::pb::RESULT_BAD_REQUEST);
    CHECK(iw.list().find(d) != nullptr);
}


// ---- the script api and the GM chat (M11 E, part 1) ----------------------------------------

TEST_CASE("AddItem / AddGoldItem give the player what the tables describe; ?gm ds runs them from the chat", "[item][world][lua]")
{
    ItemWorld iw;
    iw.w->take_outbox();
    // KScriptContext the way execute_script sets it up for a trap script
    jx::zone::KLuaScript script;
    REQUIRE(script.init(""));
    jx::zone::KScriptContext& ctx = jx::zone::g_ScriptContext();
    ctx.world = iw.w.get();
    ctx.player = const_cast<jx::zone::KNpc*>(iw.w->find_player(1));
    ctx.sid = 1;
    REQUIRE(ctx.player != nullptr);

    // AddItem(genre, detail, particular, level, series, luck): a sword of the table into the bag
    CHECK(script.call_number("AddItem", {0.0, 0.0, 0.0, 2.0, 2.0, 0.0}) == 1.0);
    CHECK(iw.list().size() == 1);
    REQUIRE(iw.list().find(1) != nullptr);
    CHECK(iw.list().find(1)->name() == "Kiem 2");
    CHECK(iw.list().find(1)->level == 2);
    // a medicine, a quest item (count 1), the town portal, a script item
    CHECK(script.call_number("AddItem", {1.0, 0.0, 0.0, 1.0, 0.0, 0.0}) == 1.0);
    CHECK(script.call_number("AddItem", {4.0, 1.0, 0.0, 1.0, 0.0, 0.0}) == 1.0);
    CHECK(script.call_number("AddItem", {5.0, 0.0, 0.0, 1.0, 0.0, 0.0}) == 1.0);
    CHECK(script.call_number("AddItem", {6.0, 0.0, 7.0, 1.0, 0.0, 0.0}) == 1.0);
    CHECK(iw.list().size() == 5);
    // fewer than six numbers, a row that is not there: 0 and nothing given
    CHECK(script.call_number("AddItem", {0.0, 0.0, 0.0, 1.0, 2.0}) == 0.0);
    CHECK(script.call_number("AddItem", {0.0, 0.0, 0.0, 9.0, 2.0, 0.0}) == 0.0);
    CHECK(iw.list().size() == 5);
    // AddGoldItem(luck, row) and the script spelling AddGoldItem("where", luck, row)
    CHECK(script.call_number("AddGoldItem", {50.0, 1.0}) == 1.0);
    CHECK(script.call_number("AddGoldItem", {std::string("bag"), 200.0, 1.0}) == 1.0);
    CHECK(script.call_number("AddGoldItem", {0.0, 9.0}) == 0.0);
    CHECK(iw.list().size() == 7);
    int gold = 0;
    iw.list().each([&](const KItem& it, const jx::zone::KItemPlace&) { if (it.ex_type == 1) ++gold; });
    CHECK(gold == 2);
    // the client heard about every one of them
    auto out = iw.w->take_outbox();
    CHECK(packets(out, 1, jx::pb::G2C_ITEM_ADD).size() == 7);

    // the quest-item api of the JX2 server, by detail or by the questkey.txt name:
    // AddStackItem([tag,] count, genre, detail, particular, level, series, luck) stacks "Tui" (max 20)
    // onto the one AddItem gave (KPlayer::AddItem(.., bStack = 1) merges like KItemList::Add does)
    CHECK(script.call_number("AddStackItem", {5.0, 4.0, 1.0, 0.0, 1.0, 0.0, 0.0}) != 0.0);
    CHECK(script.call_number("AddStackItem", {std::string("quest"), 25.0, 4.0, 1.0, 0.0, 1.0, 0.0, 0.0}) != 0.0);   // 25 > max 20: a stack of 1
    CHECK(iw.list().size() == 7);
    int tui = 0;
    iw.list().each([&](const KItem& it, const jx::zone::KItemPlace&) {
        if (it.genre == KItemGenre::task && it.detail == 1) tui += it.count;
    });
    CHECK(tui == 7);
    CHECK(script.call_number("HaveItem", {1.0}) == 1.0);
    CHECK(script.call_number("HaveItem", {std::string("Tui")}) == 1.0);
    CHECK(script.call_number("HaveItem", {std::string("Khong co")}) == 0.0);
    CHECK(script.call_number("HaveItem", {0.0}) == 0.0);        // no "Tram"
    CHECK(script.call_number("GetItemCount", {1.0}) == 1.0);    // one stack, whatever its size
    CHECK(script.call_number("GetItemCountEx", {std::string("Tui")}) == 1.0);
    CHECK(script.call_number("HaveCommonItem", {0.0, 0.0, -1.0}) == 1.0);   // a melee weapon of any particular
    CHECK(script.call_number("HaveCommonItem", {0.0, 3.0, -1.0}) == 0.0);   // no ring
    CHECK(script.call_number("GetTotalItemCount", {}) == 7.0);
    // DelItem("Tui") takes the whole stack; DelCommonItem(0, 0, 0) the sword
    script.call_number("DelItem", {std::string("Tui")});
    CHECK(script.call_number("GetItemCount", {1.0}) == 0.0);
    script.call_number("DelItemEx", {1.0});
    CHECK(script.call_number("HaveItem", {1.0}) == 0.0);
    script.call_number("DelCommonItem", {0.0, 0.0, 0.0});
    CHECK(iw.list().find(1) == nullptr);
    CHECK(iw.list().size() == 5);
    out = iw.w->take_outbox();
    CHECK(packets(out, 1, jx::pb::G2C_ITEM_REMOVE).size() == 2);

    // the money of the bag: Earn(n) (0x08118970 -> KPlayer::Earn 0x080AAED0), Pay(n) (0x08118A90 -> 0x080A9450: 1 paid,
    // 0 when the bag holds less) and GetCash() (0x081116D0: Player+0x508c); every change is a 0x61 money sync (G2C_MONEY)
    const int cash0 = static_cast<int>(script.call_number("GetCash", {}).value_or(-1.0));
    CHECK(cash0 == iw.list().money(jx::zone::room_equipment));
    script.call_number("Earn", {50.0});
    CHECK(script.call_number("GetCash", {}) == static_cast<double>(cash0 + 50));
    script.call_number("Earn", {-5.0});   // n <= 0: nothing happens (0x08118991)
    CHECK(script.call_number("GetCash", {}) == static_cast<double>(cash0 + 50));
    CHECK(script.call_number("Pay", {30.0}) == 1.0);
    CHECK(script.call_number("GetCash", {}) == static_cast<double>(cash0 + 20));
    CHECK(script.call_number("Pay", {static_cast<double>(cash0 + 21)}) == 0.0);   // more than the bag: refused, nothing taken
    CHECK(script.call_number("GetCash", {}) == static_cast<double>(cash0 + 20));
    CHECK(!script.call_number("Pay", {0.0}).has_value());   // n <= 0 returns nothing
    // GetLife / GetMana (0x081124D0 / 0x08112270): 0 the current value, 1 / 2 the base maximum; RestoreLife /
    // RestoreMana (0x08112480 / 0x08112430) fill them to the maximum in use
    ctx.player->cur.mana = 3;
    ctx.player->cur.life = 5;
    CHECK(script.call_number("GetMana", {0.0}) == 3.0);
    CHECK(script.call_number("GetLife", {0.0}) == 5.0);
    CHECK(script.call_number("GetMana", {1.0}) == static_cast<double>(ctx.player->base.mana_max));
    CHECK(script.call_number("GetLife", {2.0}) == static_cast<double>(ctx.player->base.life_max));
    script.call_number("RestoreMana", {});
    script.call_number("RestoreLife", {});
    CHECK(ctx.player->cur.mana == ctx.player->mana_max());
    CHECK(ctx.player->cur.life == ctx.player->life_max());
    out = iw.w->take_outbox();
    CHECK(packets(out, 1, jx::pb::G2C_MONEY).size() == 2);
    ctx = jx::zone::KScriptContext{};

    // the chat: "?gm ds <lua>" runs for the player only while gm_chat is on
    REQUIRE(iw.w->chat(1, "?gm ds AddItem(0,0,0,1,2,0)"));
    out = iw.w->take_outbox();
    CHECK(iw.list().size() == 5);                                   // off: it was said, not run
    CHECK(packets(out, 1, jx::pb::G2C_CHAT_MSG).size() == 1);
    CHECK(decode_packet<jx::pb::ChatMsg>(packets(out, 1, jx::pb::G2C_CHAT_MSG)[0]).text() == "?gm ds AddItem(0,0,0,1,2,0)");

    jx::zone::KSubWorldConfig cfg;
    cfg.zone_id = 1;
    cfg.width = cfg.height = 4096;
    cfg.spawn_point = jx::zone::Pos{2000, 2000};
    cfg.items = iw.lib;
    cfg.gm_chat = true;
    jx::zone::KSubWorld gm(cfg);
    jx::pb::RoleData role;
    role.set_player_id(12);
    role.set_name("GM");
    role.set_level(9);
    jx::EntityId id;
    jx::zone::Pos p;
    REQUIRE(gm.spawn_player(1, role, id, p) == jx::pb::RESULT_OK);
    gm.take_outbox();
    REQUIRE(gm.chat(1, "?gm ds AddItem(0,0,0,1,2,0)"));
    CHECK(gm.items_of(1)->size() == 1);
    out = gm.take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    REQUIRE(packets(out, 1, jx::pb::G2C_CHAT_MSG).size() == 1);   // "GM: ok" to the player, nothing broadcast
    CHECK(decode_packet<jx::pb::ChatMsg>(packets(out, 1, jx::pb::G2C_CHAT_MSG)[0]).text() == "GM: ok");
    // a wrong line answers with the Lua error, an unknown command with a word; neither is chat
    REQUIRE(gm.chat(1, "?gm ds AddItem("));
    REQUIRE(gm.chat(1, "?gm xx 1"));
    out = gm.take_outbox();
    const auto said = packets(out, 1, jx::pb::G2C_CHAT_MSG);
    REQUIRE(said.size() == 2);
    CHECK(decode_packet<jx::pb::ChatMsg>(said[0]).text().rfind("GM: ", 0) == 0);
    CHECK(decode_packet<jx::pb::ChatMsg>(said[0]).text() != "GM: ok");
    CHECK(decode_packet<jx::pb::ChatMsg>(said[1]).text() == "GM: unknown command");
    CHECK(gm.items_of(1)->size() == 1);
    // ordinary talk still goes out as talk
    REQUIRE(gm.chat(1, "xin chao"));
    out = gm.take_outbox();
    CHECK(decode_packet<jx::pb::ChatMsg>(packets(out, 1, jx::pb::G2C_CHAT_MSG)[0]).name() == "GM");
}


// ---- the ground: drops, pick up, throw away (M11 D) --------------------------------------------

namespace {

const char* const kObjData = R"({
 "objects": {
  "9": {"id": 9, "name": "Kiem", "kind": "Item", "life_time": 40, "height": 8, "image": "aaaa0001"},
  "18": {"id": 18, "name": "Thuoc", "kind": "Item", "life_time": 0, "height": 4, "image": "aaaa0002"},
  "22": {"id": 22, "name": "Ao", "kind": "Item", "life_time": 40, "height": 8, "image": "aaaa0003"},
  "41": {"id": 41, "name": "Tui", "kind": "Item", "life_time": 40, "height": 8, "image": "aaaa0004"},
  "267": {"id": 267, "name": "Tien", "kind": "Money", "life_time": 40, "height": 4, "image": "aaaa0005"},
  "268": {"id": 268, "name": "Tien lon", "kind": "Money", "life_time": 40, "height": 4, "image": "aaaa0006"}
 },
 "money": [{"max": 200, "obj": 267}, {"max": 999999999, "obj": 268}]
})";

std::shared_ptr<jx::zone::KObjDataSet> test_objdata()
{
    const auto path = std::filesystem::temp_directory_path() / "jxnext_objdata_test.json";
    std::ofstream(path, std::ios::binary) << kObjData;
    auto od = std::make_shared<jx::zone::KObjDataSet>();
    std::string error;
    REQUIRE(od->load(path.string(), &error));
    return od;
}

// a monster template with a drop table: every roll is a level-1 sword, one roll in five is money
std::shared_ptr<jx::zone::KNpcTemplateSet> test_templates()
{
    auto set = std::make_shared<jx::zone::KNpcTemplateSet>();
    jx::zone::KNpcTemplate t;
    t.id = 418;
    t.name = "pig";
    t.treasure = 3;
    t.camp = jx::zone::camp_animal;   // what a beginner may fight (g_GenOneRelation); the missile only reaches an enemy
    t.drop_rate_file = "\\settings\\item\\test.ini";
    set->add(t);
    jx::zone::KNpcDropRate r;
    r.source = "test.ini";
    r.count = 1;
    r.rand_range = 1000;
    r.money_rate = 20;
    r.money_scale = 50;
    r.min_level_scale = 20;
    r.max_level_scale = 10;
    r.min_level = 1;
    r.max_level = 2;
    jx::zone::KDropEntry e;
    e.genre = 0;
    e.detail = 0;
    e.particular = 0;
    e.rate = 1000;
    r.entries.push_back(e);
    set->add_drop_rate("\\settings\\item\\test.ini", r);
    return set;
}

} // namespace

TEST_CASE("ground objects: a thrown item lies for anybody, a drop for its player, then goes away; picking up needs to be near", "[item][world][drop]")
{
    jx::log::Options lo;
    lo.console = false;
    lo.default_level = jx::log::Level::warn;
    jx::log::init(lo);
    auto lib = std::make_shared<jx::zone::KItemLibrary>();
    {
        KItemTemplateSet set;
        std::string error;
        REQUIRE(set.load(write_tables(), &error));
        lib->add(1, std::move(set));
    }
    jx::zone::KSubWorldConfig cfg;
    cfg.zone_id = 1;
    cfg.width = cfg.height = 4096;
    cfg.spawn_point = jx::zone::Pos{2000, 2000};
    cfg.items = lib;
    cfg.objdata = test_objdata();
    cfg.interest_period = 1;
    cfg.view_slack = 0;
    cfg.far_period = 1;
    jx::zone::KSubWorld w(cfg);
    jx::pb::RoleData role;
    role.set_player_id(11);
    role.set_name("A");
    role.set_level(9);
    jx::EntityId me;
    jx::zone::Pos p;
    REQUIRE(w.spawn_player(1, role, me, p) == jx::pb::RESULT_OK);
    auto gen = w.item_generator();
    const auto sword = w.give_item(1, *gen->equipment(jx::zone::equip_meleeweapon, 0, 2, 1));
    REQUIRE(sword != 0);
    w.tick();
    w.take_outbox();

    // throw it away: it leaves the bag and appears on the ground next to the player, for anybody
    REQUIRE(w.item_drop_request(1, sword, 51));
    CHECK(w.items_of(1)->size() == 0);
    CHECK(w.ground_object_count() == 1);
    auto out = w.take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_REMOVE).size() == 1);
    w.tick();   // the player's own look notices the new thing on the ground
    out = w.take_outbox();
    const auto spawns = packets(out, 1, jx::pb::G2C_ENTITY_SPAWN);
    REQUIRE(spawns.size() == 1);
    const auto seen = decode_packet<jx::pb::EntitySpawn>(spawns[0]);
    REQUIRE(seen.entities_size() == 1);
    const auto& obj = seen.entities(0);
    CHECK(obj.entity_type() == jx::pb::ENTITY_DROP);
    CHECK(obj.template_id() == 9);   // the sword's ObjData row
    CHECK(obj.name() == "Kiem 1");
    CHECK(obj.count() == 1);
    const jx::EntityId ground{obj.entity_id()};
    REQUIRE(w.ground_item(ground) != nullptr);
    CHECK(w.ground_item(ground)->name() == "Kiem 1");
    CHECK((std::abs(obj.pos().x() - p.x) <= 128 && std::abs(obj.pos().y() - p.y) <= 128));

    // a task item cannot be thrown away
    const auto tui = w.give_item(1, *gen->quest(1, 3));
    REQUIRE_FALSE(w.item_drop_request(1, tui, 52));
    CHECK(w.items_of(1)->size() == 1);
    w.take_outbox();

    // picking it up from far away is refused (40000 = 200 units squared), from near it comes back
    REQUIRE(w.teleport(me, jx::zone::Pos{2500, 2000}));
    REQUIRE_FALSE(w.pick_up_request(1, ground, 53));
    out = w.take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_RESULT).size() == 1);
    CHECK(decode_packet<jx::pb::ItemResult>(packets(out, 1, jx::pb::G2C_ITEM_RESULT)[0]).result() == jx::pb::RESULT_WRONG_STATE);
    REQUIRE(w.teleport(me, jx::zone::Pos{obj.pos().x() + 100, obj.pos().y()}));
    REQUIRE(w.pick_up_request(1, ground, 54));
    CHECK(w.ground_object_count() == 0);
    CHECK(w.items_of(1)->size() == 2);
    out = w.take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    CHECK(decode_packet<jx::pb::ItemAdd>(packets(out, 1, jx::pb::G2C_ITEM_ADD)[0]).item().name() == "Kiem 1");
    REQUIRE_FALSE(w.pick_up_request(1, ground, 55));   // gone

    // money: a pile picked up goes into the purse (KPlayer::Earn) and the client hears the sum
    const auto pile = w.drop_money(150, p, 0);
    REQUIRE(pile.value != 0);
    CHECK(w.find_entity(pile)->template_id == 267);   // MoneyObj: up to 200 coins is row 267
    const auto big = w.drop_money(5000, p, 0);
    CHECK(w.find_entity(big)->template_id == 268);
    REQUIRE(w.teleport(me, w.find_entity(pile)->pos()));
    REQUIRE(w.pick_up_request(1, pile, 56));
    CHECK(w.items_of(1)->money() == 150);
    out = w.take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_MONEY).size() == 1);
    CHECK(decode_packet<jx::pb::MoneySync>(packets(out, 1, jx::pb::G2C_MONEY)[0]).money() == 150);

    // a drop kept for another player: refused until the belong time (600 frames) runs out (a
    // potion: its object lies for ever, so it is still there after that)
    const auto kept = w.drop_item(*gen->medicine(0, 1), p, 99);
    REQUIRE(kept.value != 0);
    REQUIRE(w.teleport(me, w.find_entity(kept)->pos()));
    REQUIRE_FALSE(w.pick_up_request(1, kept, 57));
    out = w.take_outbox();
    CHECK(decode_packet<jx::pb::ItemResult>(packets(out, 1, jx::pb::G2C_ITEM_RESULT)[0]).result() == jx::pb::RESULT_UNAUTHORIZED);
    for (int i = 0; i < jx::zone::kObjBelongTime; ++i) w.tick();
    REQUIRE(w.pick_up_request(1, kept, 58));
    w.take_outbox();

    // an item on the ground vanishes after its LifeTime (40 frames here for a sword), a potion (LifeTime 0) never
    const auto short_lived = w.drop_item(*gen->equipment(jx::zone::equip_meleeweapon, 0, 2, 1), p, 0);
    const auto forever = w.drop_item(*gen->medicine(0, 1), p, 0);
    REQUIRE((short_lived.value != 0 && forever.value != 0));
    for (int i = 0; i < 40; ++i) w.tick();
    CHECK(w.find_entity(short_lived) == nullptr);
    CHECK(w.find_entity(forever) != nullptr);
    CHECK(w.ground_item(short_lived) == nullptr);
    out = w.take_outbox();
    bool despawned = false;
    for (const auto& pk : packets(out, 1, jx::pb::G2C_ENTITY_DESPAWN)) {
        const auto d = decode_packet<jx::pb::EntityDespawn>(pk);
        for (int i = 0; i < d.entity_ids_size(); ++i) despawned = despawned || d.entity_ids(i) == short_lived.value;
    }
    CHECK(despawned);
}

TEST_CASE("a monster killed by a player drops its treasure: money by MoneyRate, items by the table", "[item][world][drop]")
{
    jx::log::Options lo;
    lo.console = false;
    lo.default_level = jx::log::Level::warn;
    jx::log::init(lo);
    auto lib = std::make_shared<jx::zone::KItemLibrary>();
    {
        KItemTemplateSet set;
        std::string error;
        REQUIRE(set.load(write_tables(), &error));
        lib->add(1, std::move(set));
    }
    jx::zone::KSubWorldConfig cfg;
    cfg.zone_id = 1;
    cfg.width = cfg.height = 4096;
    cfg.spawn_point = jx::zone::Pos{2000, 2000};
    cfg.items = lib;
    cfg.objdata = test_objdata();
    cfg.templates = test_templates();
    cfg.tick_hz = 18;
    jx::zone::KSubWorld w(cfg);

    // GenRandomItem: the level range from the npc level through the scales, clamped to the table
    const auto* table = cfg.templates->drop_rate("\\settings\\item\\test.ini");
    REQUIRE(table != nullptr);
    std::set<int> levels;
    for (int i = 0; i < 200; ++i) {
        auto it = w.gen_random_item(*table, 15, 0, 0);   // (15-1)/10+1 = 2 .. (15-1)/20+1 = 1 -> 1..2
        REQUIRE(it.has_value());
        CHECK(it->genre == KItemGenre::equip);
        levels.insert(it->level);
        // the magic slots: k = g_Random(4) + 3 slots at the item level -> the first prefix always rolls
        CHECK((it->magic[0].type == 100 || it->magic[0].type == 101));
        CHECK((it->magic[1].type == 200 || it->magic[1].type == 100));
    }
    CHECK((levels == std::set<int>{1, 2}));
    levels.clear();
    for (int i = 0; i < 50; ++i) levels.insert(w.gen_random_item(*table, 1, 0, 0)->level);   // 0/10+1 = 1 both ends
    CHECK((levels == std::set<int>{1}));
    jx::zone::KNpcDropRate empty = *table;
    empty.entries.clear();
    CHECK_FALSE(w.gen_random_item(empty, 5, 0, 0).has_value());

    // the kill: three rolls, each money (20 %) or a sword
    jx::pb::RoleData role;
    role.set_player_id(11);
    role.set_name("Hero");
    role.set_level(9);
    role.mutable_stats()->set_strength(100);   // hits hard and surely (see test_KSubWorld's role())
    role.mutable_stats()->set_dexterity(100);
    role.mutable_stats()->set_hp_max(500);
    jx::EntityId hero;
    jx::zone::Pos at;
    REQUIRE(w.spawn_player(7, role, hero, at) == jx::pb::RESULT_OK);
    int total_objects = 0;
    for (int kill = 0; kill < 20; ++kill) {
        const jx::EntityId pig = w.spawn_npc("pig", jx::zone::Pos{2050, 2000}, 418, 0, jx::zone::KNpcKind::monster);
        jx::zone::KNpc* e = const_cast<jx::zone::KNpc*>(w.find_entity(pig));
        REQUIRE(e != nullptr);
        CHECK(e->cur.treasure == 3);
        e->cur.experience = 1000;
        e->cur.life = 1;
        REQUIRE(w.attack_request(7, pig, static_cast<std::uint32_t>(kill + 1)));
        // a swing lands at 95 percent at best (MAX_HIT_PERCENT): room for a few more swings
        for (int i = 0; i < 200 && w.find_entity(pig) != nullptr && w.find_entity(pig)->alive(); ++i) w.tick();
        REQUIRE((w.find_entity(pig) == nullptr || !w.find_entity(pig)->alive()));
        total_objects += static_cast<int>(w.ground_object_count());   // the drops land at the end of the tick the death happened in
        for (int i = 0; i < 40; ++i) w.tick();   // everything on the ground is gone after 40 frames
        CHECK(w.ground_object_count() == 0);
        w.take_outbox();
    }
    CHECK(total_objects >= 40);   // 20 kills x 3 rolls, minus the rare rolls that land on nothing
    // money: experience * MoneyScale / 100 x the server's rate
    const auto pile = w.drop_money(1000 * 50 / 100, at, 11);
    REQUIRE(pile.value != 0);
    CHECK(w.find_entity(pile)->object.money == 500);
    CHECK(w.find_entity(pile)->object.belong == 11);
}

// The wear of the worn pieces: KItemList 0x08201940 with KItem::Abrade 0x08066570 and the rates of
// AbradeRate.ini (KItemSet::Init 0x0806E250, the lookup 0x0806D560), the AbradeToZero way of a piece
// at 0, the percent loss 0x08201D90 / 0x080658B0, the hook of ReceiveDamage 0x0808B148 - docs/LINUX-SERVER.md §16.3
TEST_CASE("equipment wears one in N on an attack, a hit and a step; a piece at 0 goes broken into the bag", "[item][world]")
{
    auto rate = std::make_shared<jx::zone::KAbradeRate>();
    rate->rate[0][jx::zone::itempart_weapon] = 1;   // [Attack] Weapon: every attack
    rate->rate[1][jx::zone::itempart_body] = 1;     // [Defend] Body: every hit
    rate->rate[2][jx::zone::itempart_foot] = 1;     // [Move] Foot: every step
    ItemWorld iw(rate);
    jx::zone::KNpc* hero = iw.w->mutable_entity(iw.id);
    REQUIRE(hero != nullptr);
    // level 9 with plenty of strength; the armor of the tables asks for a sex / series of 1 exactly (EnoughAttrib)
    const auto level9 = [](int type) {
        if (type == jx::zone::magic_requirelevel) return 9;
        if (type == jx::zone::magic_requireseries || type == jx::zone::magic_requiresex) return 1;
        return 50;
    };
    const auto sword = iw.list().add(*iw.gen().equipment(jx::zone::equip_meleeweapon, 0, 2, 1), jx::zone::room_equipment);
    const auto armor = iw.list().add(*iw.gen().equipment(jx::zone::equip_armor, 0, 0, 1), jx::zone::room_equipment);
    REQUIRE((sword && armor));
    REQUIRE(iw.list().equip(sword, jx::zone::itempart_weapon, level9));
    REQUIRE(iw.list().equip(armor, jx::zone::itempart_body, level9));
    iw.list().find_mutable(sword)->durability = 2;
    iw.list().find_mutable(armor)->durability = 3;
    iw.w->take_outbox();
    // [Attack]: the weapon loses a point, the piece is synced (the 0x9b packet = G2C_ITEM_ADD); the body does not
    iw.w->abrade_equipments(*hero, 0);
    CHECK(iw.list().find(sword)->durability == 1);
    CHECK(iw.list().find(armor)->durability == 3);
    CHECK(packets(iw.w->take_outbox(), 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    // [Defend]: the body piece only
    iw.w->abrade_equipments(*hero, 1);
    CHECK(iw.list().find(armor)->durability == 2);
    CHECK(iw.list().find(sword)->durability == 1);
    CHECK(packets(iw.w->take_outbox(), 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    // [Move]: nothing on the feet - nothing happens
    iw.w->abrade_equipments(*hero, 2);
    CHECK(iw.w->take_outbox().empty());
    // the weapon reaches 0 (AbradeToZero): genre 7, off the body into the bag (G2C_ITEM_MOVE), the message
    // G_STR_ITEM_ABRADETOZERO with its name, the attributes recalculated
    iw.w->abrade_equipments(*hero, 0);
    const KItem* s = iw.list().find(sword);
    REQUIRE(s != nullptr);
    CHECK(s->durability == 0);
    CHECK(s->genre == jx::zone::KItemGenre::broken);
    CHECK(iw.list().equipped(jx::zone::itempart_weapon) == 0);
    const auto place = iw.list().place_of(sword);
    REQUIRE(static_cast<bool>(place));
    CHECK(place->room == jx::zone::room_equipment);
    auto out = iw.w->take_outbox();
    CHECK(packets(out, 1, jx::pb::G2C_ITEM_MOVE).size() == 1);
    const auto said = packets(out, 1, jx::pb::G2C_CHAT_MSG);
    REQUIRE(said.size() == 1);
    CHECK(decode_packet<jx::pb::ChatMsg>(said[0]).text().find(s->name()) != std::string::npos);
    // a broken piece is not worn any more, so it wears no further; a piece at 0 would answer -1 anyway
    iw.w->abrade_equipments(*hero, 0);
    CHECK(iw.list().find(sword)->durability == 0);
    // 0x08201D90(list, n): n percent off every worn piece (KItem 0x080658B0), synced
    iw.list().find_mutable(armor)->durability = 100;
    iw.w->abrade_equipments_percent(*hero, 10);
    CHECK(iw.list().find(armor)->durability == 90);
    CHECK(packets(iw.w->take_outbox(), 1, jx::pb::G2C_ITEM_ADD).size() == 1);
    // the hook of ReceiveDamage 0x0808B148: a blow that takes life wears the body piece
    const jx::EntityId pig = iw.w->spawn_npc("pig", jx::zone::Pos{2040, 2000}, 418, 0, jx::zone::KNpcKind::monster);
    jx::zone::KNpc* p = iw.w->mutable_entity(pig);
    hero = iw.w->mutable_entity(iw.id);   // the table may have moved
    REQUIRE((p != nullptr && hero != nullptr));
    p->camp = p->current_camp = jx::zone::camp_animal;
    std::array<jx::zone::KMagicAttrib, jx::zone::kSkillAttribs> d{};
    d[1].type = jx::zone::magic_attackrating_v;
    d[1].value = {1000, 0, 0};
    d[3].type = jx::zone::magic_physicsdamage_v;
    d[3].value = {30, 0, 30};
    hero->cur.defend = 0;
    const int before = hero->life();
    CHECK(iw.w->receive_damage(*hero, *p, -1, true, d.data(), false, 0, jx::zone::relation_enemy, 1) == 1);
    CHECK(hero->life() < before);
    CHECK(iw.list().find(armor)->durability == 89);
}

// ---- the looks of worn pieces (M12 B6b): KItemChangeRes 0x08068AC0 / 0x081FE0E0, the sync 0x0807ACB0 -------

namespace {

// the five tables and the gold map as jxassets export-item-res writes them (row 1 = header; KTabFile rows are 1-based):
// MeleeRes row 2 = bare hands (col 2 = 2 -> 0), row 3 = particular 0 level 1 (-> 5); RangeRes row 2 = particular 0 level 1
// (-> 19); ArmorRes / HelmRes row 2 = nothing worn (20 -> 18), row 13 = particular 1 level 1 (-> 30 / 27); HorseRes row 3 =
// particular 0 level 1 (10 -> 8); GoldEquipRes: id 6 kind 5 (the weapon part's kind, 0x080685B0) -> 31, id 6 kind 0 -> 40
std::string write_item_res()
{
    const auto path = (std::filesystem::temp_directory_path() / "jx_item_res_test.json").string();
    std::ofstream out(path, std::ios::binary);
    out << R"({"melee": [[0, 0], [1, 2], [2, 7]], "range": [[0, 0], [1, 21]],
 "armor": [[0, 0], [1, 20], [2, 0], [3, 0], [4, 0], [5, 0], [6, 0], [7, 0], [8, 0], [9, 0], [10, 0], [11, 0], [12, 32]],
 "helm": [[0, 0], [1, 20], [2, 0], [3, 0], [4, 0], [5, 0], [6, 0], [7, 0], [8, 0], [9, 0], [10, 0], [11, 0], [12, 29]],
 "horse": [[0, 0], [1, 0], [2, 10]],
 "gold": [[0, 0, 0], [6, 33, 5], [6, 42, 0]], "platina": []})";
    return path;
}

} // namespace

TEST_CASE("KItemChangeRes: the rows of the five tables, the gold map, nothing worn", "[item][res]")
{
    std::string error;
    auto t = jx::zone::KItemChangeRes::load(write_item_res(), &error);
    REQUIRE(t);
    // 0x08068A00: bare hands = row 2 of MeleeRes; melee particular 0 level 1 = row 3; range = row particular*10+level+1
    CHECK(t->weapon_res(jx::zone::equip_meleeweapon, 3, 0) == 0);
    CHECK(t->weapon_res(jx::zone::equip_meleeweapon, 0, 1) == 5);
    CHECK(t->weapon_res(jx::zone::equip_rangeweapon, 0, 1) == 19);
    CHECK(t->weapon_res(jx::zone::equip_meleeweapon, 9, 9) == 0);   // outside the table: the default 2 - 2
    // 0x08068980 / 0x08068900: row 2 when nothing is worn (20 - 2), particular 1 level 1 -> row 13
    CHECK(t->armor_res(0, 0) == 18);
    CHECK(t->helm_res(0, 0) == 18);
    CHECK(t->armor_res(1, 0) == 18);
    CHECK(t->armor_res(1, 1) == 30);
    CHECK(t->helm_res(1, 1) == 27);
    CHECK(t->armor_res(5, 5) == 17);   // outside: the default 19 - 2
    // 0x080688B0: level 0 = no horse, else the row's col 2 - 2
    CHECK(t->horse_res(0, 0) == -1);
    CHECK(t->horse_res(0, 1) == 8);
    CHECK(t->horse_res(3, 3) == 0);    // outside: 2 - 2
    // 0x080686A0: (kind << 16 | gen_param + 1) -> col 2 - 2; a miss = -1
    CHECK(t->gold_res(5, 5) == 31);
    CHECK(t->gold_res(5, 0) == 40);
    CHECK(t->gold_res(5, 1) == -1);
    // 0x08068730: tier <= 5 reads the gold map, a miss with a kind falls back to kind 0, the platina map is empty
    CHECK(t->platina_res(5, 1, 3) == 40);
    CHECK(t->platina_res(5, 1, 7) == -1);
    // 0x081FE0E0: nothing worn on each part
    CHECK(t->equip_res(nullptr, jx::zone::itempart_head) == 18);
    CHECK(t->equip_res(nullptr, jx::zone::itempart_body) == 18);
    CHECK(t->equip_res(nullptr, jx::zone::itempart_weapon) == 0);
    CHECK(t->equip_res(nullptr, jx::zone::itempart_horse) == -1);
    CHECK(t->equip_res(nullptr, jx::zone::itempart_mantle) == -1);
    CHECK(t->equip_res(nullptr, jx::zone::itempart_belt) == 0);
}

TEST_CASE("the look of a player: the bare rows at spawn, a horse and a gold armour change it and are told around", "[item][world][res]")
{
    std::string error;
    auto t = jx::zone::KItemChangeRes::load(write_item_res(), &error);
    REQUIRE(t);
    ItemWorld iw(nullptr, std::make_shared<const jx::zone::KItemChangeRes>(std::move(*t)));
    const jx::zone::KNpc* me = iw.w->mutable_entity(iw.id);
    REQUIRE(me != nullptr);
    // 0x080C1F50 / 0x0807ACB0 at load: the bare rows (helm / armour row 18, bare hands 0, no horse, no mantle), version 1
    CHECK(me->helm_res == 18);
    CHECK(me->armor_res == 18);
    CHECK(me->weapon_res == 0);
    CHECK(me->horse_res == -1);
    CHECK(me->mantle_res == -1);
    CHECK(me->res_version == 1);
    // the spawn's EntityInfo carries the rows
    {
        const auto out = iw.w->take_outbox();
        const auto spawns = packets(out, 1, jx::pb::G2C_ENTITY_SPAWN);
        REQUIRE_FALSE(spawns.empty());
        const auto sp = decode_packet<jx::pb::EntitySpawn>(spawns.front());
        REQUIRE(sp.entities_size() >= 1);
        bool found = false;
        for (const auto& e : sp.entities()) {
            if (e.entity_id() != iw.id.value) continue;
            found = true;
            CHECK(e.helm_res() == 18);
            CHECK(e.armor_res() == 18);
            CHECK(e.horse_res() == -1);
        }
        CHECK(found);
    }
    auto g = iw.gen();
    // a horse of particular 0 level 1: HorseRes row 3 -> 8; the G2C_ENTITY_RES with version 2
    const auto horse = iw.w->give_item(1, *g.equipment(jx::zone::equip_horse, 0, 1, 1));
    REQUIRE(horse);
    iw.w->take_outbox();
    REQUIRE(iw.w->item_equip_request(1, horse, -1, 61));
    me = iw.w->mutable_entity(iw.id);
    CHECK(me->horse_res == 8);
    CHECK(me->res_version == 2);
    {
        const auto res = packets(iw.w->take_outbox(), 1, jx::pb::G2C_ENTITY_RES);
        REQUIRE(res.size() == 1);
        const auto r = decode_packet<jx::pb::EntityRes>(res.front());
        CHECK(r.entity_id() == iw.id.value);
        CHECK(r.horse_res() == 8);
        CHECK(r.helm_res() == 18);
        CHECK(r.version() == 2);
    }
    // the ride toggle changes nothing of the look (0x080AEFA0 sends 0x9c only); taking the horse off makes it -1 again
    REQUIRE(iw.w->ride_request(1, false, 62));
    CHECK(packets(iw.w->take_outbox(), 1, jx::pb::G2C_ENTITY_RES).empty());
    CHECK(iw.w->mutable_entity(iw.id)->res_version == 2);
    REQUIRE(iw.w->item_unequip_request(1, jx::zone::itempart_horse, 63));
    CHECK(iw.w->mutable_entity(iw.id)->horse_res == -1);
    CHECK(iw.w->mutable_entity(iw.id)->res_version == 3);
    // a gold sword (ex_type 1, gen_param 5): the gold map's (kind 5 << 16 | 6) -> 31 (0x081FE158), not MeleeRes
    auto goldpiece = *g.equipment(jx::zone::equip_meleeweapon, 0, 1, 1);
    goldpiece.ex_type = 1;
    goldpiece.gen_param = 5;
    const auto gid = iw.w->give_item(1, goldpiece);
    REQUIRE(gid);
    iw.w->take_outbox();
    REQUIRE(iw.w->item_equip_request(1, gid, -1, 64));
    CHECK(iw.w->mutable_entity(iw.id)->weapon_res == 31);
    CHECK(iw.w->mutable_entity(iw.id)->res_version == 4);
    // off again: bare hands (MeleeRes row 2)
    REQUIRE(iw.w->item_unequip_request(1, jx::zone::itempart_weapon, 65));
    CHECK(iw.w->mutable_entity(iw.id)->weapon_res == 0);
    CHECK(iw.w->mutable_entity(iw.id)->res_version == 5);
    // a plain sword of particular 0 level 1: MeleeRes row 3 -> 5
    const auto plain = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 1, 1));
    REQUIRE(plain);
    REQUIRE(iw.w->item_equip_request(1, plain, -1, 66));
    CHECK(iw.w->mutable_entity(iw.id)->weapon_res == 5);
    CHECK(iw.w->mutable_entity(iw.id)->res_version == 6);
}


// ---- the trade (M14 lát G1; docs/LINUX-SERVER.md §18) -------------------------------------------------------------

namespace {

// a second player next to the first: B (sid 2, player 12) at the same spot
jx::EntityId spawn_b(ItemWorld& iw)
{
    jx::pb::RoleData role;
    role.set_player_id(12);
    role.set_name("B");
    role.set_level(9);
    role.mutable_stats()->set_strength(25);
    role.mutable_stats()->set_dexterity(25);
    role.mutable_stats()->set_hp_max(204);
    role.set_money(50);
    jx::EntityId id;
    jx::zone::Pos p;
    REQUIRE(iw.w->spawn_player(2, role, id, p) == jx::pb::RESULT_OK);
    iw.w->tick();
    iw.w->take_outbox();
    return id;
}

std::vector<jx::pb::SysMsg> sys_msgs(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::SysMsg> out;
    for (const auto& p : packets(all, sid, jx::pb::G2C_SYS_MSG)) out.push_back(decode_packet<jx::pb::SysMsg>(p));
    return out;
}

// the two players into a trade: A opens, B applies, A accepts
void start_trade(ItemWorld& iw, jx::EntityId b)
{
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_APPLY_OPEN, jx::EntityId{}, 0, "ban gi cung mua"));
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_APPLY_START, iw.id, 0, ""));
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_REPLY, b, 1, ""));
}

} // namespace

TEST_CASE("the sign over the head and the trade application (SetState 0x080C29D0, TradeApplyStart 0x080B4DE0, ReplyStart 0x080BAFD0)", "[item][world][trade]")
{
    ItemWorld iw;
    const jx::EntityId b = spawn_b(iw);
    jx::zone::KNpc& A = *iw.w->mutable_entity(iw.id);
    jx::zone::KNpc& B = *iw.w->mutable_entity(b);
    // nobody may apply to a player who is not open (0x080B4E60)
    CHECK_FALSE(iw.w->trade_request(2, jx::pb::TRADE_APPLY_START, iw.id, 0, ""));
    // A opens with a sentence: the state 1 to A, the sign to everybody around (B included)
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_APPLY_OPEN, jx::EntityId{}, 0, "ban gi cung mua"));
    CHECK(A.player.menu.state == jx::zone::menu_state_trade_open);
    CHECK(A.player.menu.sentence == "ban gi cung mua");
    {
        auto out = iw.w->take_outbox();
        auto st = packets(out, 1, jx::pb::G2C_TRADE_STATE);
        REQUIRE(st.size() == 1);
        CHECK(decode_packet<jx::pb::TradeState>(st[0]).state() == 1);
        auto ms = packets(out, 2, jx::pb::G2C_ENTITY_MENU_STATE);
        REQUIRE(!ms.empty());
        const auto m = decode_packet<jx::pb::EntityMenuState>(ms.back());
        CHECK(m.entity_id() == iw.id.value);
        CHECK(m.state() == 2);
        CHECK(m.sentence() == "ban gi cung mua");
    }
    // a sentence longer than 255 bytes is cut (0x080AE605)
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_APPLY_OPEN, jx::EntityId{}, 0, std::string(300, 'x')));
    CHECK(A.player.menu.sentence.size() == 255);
    // B applies: A hears the 0x8b {B}; B remembers whom it asked (m_nApplyIdx)
    iw.w->take_outbox();
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_APPLY_START, iw.id, 0, ""));
    CHECK(B.player.trade.apply == 1);
    {
        auto ap = packets(iw.w->take_outbox(), 1, jx::pb::G2C_TRADE_APPLY);
        REQUIRE(ap.size() == 1);
        const auto a = decode_packet<jx::pb::TradeApply>(ap[0]);
        CHECK(a.entity_id() == b.value);
        CHECK(a.name() == "B");
    }
    // a refusal: the 0x86 {8, 0xd} to the applicant, nothing else
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_REPLY, b, 0, ""));
    {
        auto msgs = sys_msgs(iw.w->take_outbox(), 2);
        REQUIRE(msgs.size() == 1);
        CHECK(msgs[0].id() == 0xd);
        CHECK(msgs[0].entity_id() == iw.id.value);
    }
    CHECK_FALSE(iw.w->trading(A));
    CHECK(B.player.trade.apply == 0);
    // a reply to someone who never asked: nothing
    CHECK_FALSE(iw.w->trade_request(1, jx::pb::TRADE_REPLY, b, 1, ""));
    // B asks again, A accepts: both TRADING, both told the partner, the sync flags all off
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_APPLY_START, iw.id, 0, ""));
    iw.w->take_outbox();
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_REPLY, b, 1, ""));
    CHECK(iw.w->trading(A));
    CHECK(iw.w->trading(B));
    CHECK(A.player.trade.dest == 2);
    CHECK(B.player.trade.dest == 1);
    CHECK(A.player.menu.state == jx::zone::menu_state_trading);
    CHECK(B.player.menu.state == jx::zone::menu_state_trading);
    CHECK(A.player.menu.back_state == jx::zone::menu_state_trade_open);   // restored by a cancel (0x080C2ED0)
    {
        auto out = iw.w->take_outbox();
        auto st = packets(out, 2, jx::pb::G2C_TRADE_STATE);
        REQUIRE(st.size() == 1);
        const auto s = decode_packet<jx::pb::TradeState>(st[0]);
        CHECK(s.state() == 2);
        CHECK(s.partner() == iw.id.value);
        CHECK(s.partner_name() == "A");
        auto sy = packets(out, 1, jx::pb::G2C_TRADE_SYNC);
        REQUIRE(sy.size() == 1);
        const auto y = decode_packet<jx::pb::TradeSync>(sy[0]);
        CHECK_FALSE(y.self_lock());
        CHECK_FALSE(y.dest_lock());
        CHECK(y.dest_money() == 0);
    }
    // while trading nobody else may apply, and the team cannot open (CheckTrading 0x080A7E90)
    CHECK_FALSE(iw.w->trade_request(1, jx::pb::TRADE_APPLY_OPEN, jx::EntityId{}, 0, ""));
    // the cancel (decision 0): both out, the state before restored (A open again, B normal), the 0x78 {0} to both
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_DECISION, jx::EntityId{}, 0, ""));
    CHECK_FALSE(iw.w->trading(A));
    CHECK_FALSE(iw.w->trading(B));
    CHECK(A.player.menu.state == jx::zone::menu_state_trade_open);
    CHECK(B.player.menu.state == jx::zone::menu_state_normal);
    {
        auto out = iw.w->take_outbox();
        auto e1 = packets(out, 1, jx::pb::G2C_TRADE_END);
        auto e2 = packets(out, 2, jx::pb::G2C_TRADE_END);
        REQUIRE((e1.size() == 1 && e2.size() == 1));
        CHECK_FALSE(decode_packet<jx::pb::TradeEnd>(e1[0]).ok());
    }
    // the close of the sign (the 0x6a packet)
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_APPLY_CLOSE, jx::EntityId{}, 0, ""));
    CHECK(A.player.menu.state == jx::zone::menu_state_normal);
}

TEST_CASE("the trade box, the money, the lock, the ok and the exchange (0x6c 0x080AE510, 0x6d 0x080B2C70, SyncTradeState 0x080A85B0)", "[item][world][trade]")
{
    ItemWorld iw;
    const jx::EntityId b = spawn_b(iw);
    auto g = iw.gen();
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));
    const auto ring = iw.w->give_item(2, *g.equipment(jx::zone::equip_ring, 0, 0, 1));
    REQUIRE((sword != 0 && ring != 0));
    // out of a trade the box is closed (KItemList::ExchangeItem pos_trade)
    CHECK_FALSE(iw.w->item_move_request(1, sword, jx::zone::room_trade, 0, 0, 1));
    start_trade(iw, b);
    iw.w->take_outbox();
    jx::zone::KNpc& A = *iw.w->mutable_entity(iw.id);
    jx::zone::KNpc& B = *iw.w->mutable_entity(b);
    // A puts the sword on the table: B sees it (G2C_TRADE_ITEM), A's own move is told as usual
    REQUIRE(iw.w->item_move_request(1, sword, jx::zone::room_trade, 0, 0, 2));
    {
        auto out = iw.w->take_outbox();
        auto ti = packets(out, 2, jx::pb::G2C_TRADE_ITEM);
        REQUIRE(ti.size() == 1);
        const auto t = decode_packet<jx::pb::TradeItem>(ti[0]);
        CHECK_FALSE(t.removed());
        CHECK(t.item().id() == sword);
        CHECK(t.item().room() == jx::zone::room_trade);
        CHECK(!packets(out, 1, jx::pb::G2C_ITEM_MOVE).empty());
    }
    // and takes it back: B sees it go
    REQUIRE(iw.w->item_move_request(1, sword, jx::zone::room_equipment, 0, 0, 3));
    {
        auto ti = packets(iw.w->take_outbox(), 2, jx::pb::G2C_TRADE_ITEM);
        REQUIRE(ti.size() == 1);
        CHECK(decode_packet<jx::pb::TradeItem>(ti[0]).removed());
    }
    REQUIRE(iw.w->item_move_request(1, sword, jx::zone::room_trade, 0, 0, 4));
    REQUIRE(iw.w->item_move_request(2, ring, jx::zone::room_trade, 0, 0, 5));
    // the money: at most the bag's (A has 500, B 50); B hears A's through the sync
    CHECK_FALSE(iw.w->trade_request(1, jx::pb::TRADE_MONEY, jx::EntityId{}, 501, ""));
    CHECK_FALSE(iw.w->trade_request(1, jx::pb::TRADE_MONEY, jx::EntityId{}, -1, ""));
    iw.w->take_outbox();
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_MONEY, jx::EntityId{}, 120, ""));
    CHECK(iw.list().money(jx::zone::room_trade) == 120);
    {
        auto sy = packets(iw.w->take_outbox(), 2, jx::pb::G2C_TRADE_SYNC);
        REQUIRE(sy.size() == 1);
        CHECK(decode_packet<jx::pb::TradeSync>(sy[0]).dest_money() == 120);
    }
    // the ok before both locks is only a sync (0x080B2E49)
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_DECISION, jx::EntityId{}, 1, ""));
    CHECK_FALSE(A.player.trade.ok);
    // A locks: no more moves or money on A's side; the partner's ok is cleared with it
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_DECISION, jx::EntityId{}, 2, ""));
    CHECK(A.player.trade.locked);
    CHECK_FALSE(iw.w->trade_request(1, jx::pb::TRADE_MONEY, jx::EntityId{}, 10, ""));
    CHECK_FALSE(iw.w->item_move_request(1, sword, jx::zone::room_equipment, 0, 0, 6));
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_DECISION, jx::EntityId{}, 2, ""));
    iw.w->take_outbox();
    // both locked: A's ok is kept and synced; B's ok makes the exchange
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_DECISION, jx::EntityId{}, 1, ""));
    CHECK(A.player.trade.ok);
    {
        auto sy = packets(iw.w->take_outbox(), 2, jx::pb::G2C_TRADE_SYNC);
        REQUIRE(sy.size() == 1);
        const auto y = decode_packet<jx::pb::TradeSync>(sy[0]);
        CHECK(y.dest_ok());
        CHECK(y.dest_lock());
        CHECK(y.self_lock());
        CHECK_FALSE(y.self_ok());
    }
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_DECISION, jx::EntityId{}, 1, ""));
    // the sword is B's, the ring A's (new ids in the taker's list), the money crossed: A 500 - 120 = 380, B 50 + 120 = 170
    CHECK(iw.list().find(sword) == nullptr);
    CHECK(iw.w->items_of(2)->find(ring) == nullptr);
    int a_rings = 0, b_swords = 0;
    iw.list().each([&](const jx::zone::KItem& it, const jx::zone::KItemPlace& pl) { if (it.detail == jx::zone::equip_ring && pl.room == jx::zone::room_equipment) ++a_rings; });
    iw.w->items_of(2)->each([&](const jx::zone::KItem& it, const jx::zone::KItemPlace& pl) { if (it.detail == jx::zone::equip_meleeweapon && pl.room == jx::zone::room_equipment) ++b_swords; });
    CHECK(a_rings == 1);
    CHECK(b_swords == 1);
    CHECK(iw.list().money(jx::zone::room_equipment) == 380);
    CHECK(iw.w->items_of(2)->money(jx::zone::room_equipment) == 170);
    CHECK(iw.list().money(jx::zone::room_trade) == 0);
    CHECK_FALSE(iw.w->trading(A));
    CHECK_FALSE(iw.w->trading(B));
    CHECK(A.player.menu.state == jx::zone::menu_state_normal);   // 0x080B4448: NORMAL after a trade, not the state before
    {
        auto out = iw.w->take_outbox();
        auto e1 = packets(out, 1, jx::pb::G2C_TRADE_END);
        REQUIRE(e1.size() == 1);
        CHECK(decode_packet<jx::pb::TradeEnd>(e1[0]).ok());
        CHECK(!packets(out, 1, jx::pb::G2C_ITEM_REMOVE).empty());
        CHECK(!packets(out, 1, jx::pb::G2C_ITEM_ADD).empty());
        CHECK(!packets(out, 2, jx::pb::G2C_MONEY).empty());
    }
}

TEST_CASE("a full bag, a death and a leave end a trade the safe way (0x080B2FF2, 0x080AE4B0, KPlayer::Clear)", "[item][world][trade]")
{
    ItemWorld iw;
    const jx::EntityId b = spawn_b(iw);
    auto g = iw.gen();
    // B's bag full of rings (6 x 10 cells)
    std::vector<std::uint32_t> rings;
    for (int i = 0; i < 60; ++i) {
        const auto r = iw.w->give_item(2, *g.equipment(jx::zone::equip_ring, 0, 0, 1));
        if (r == 0) break;
        rings.push_back(r);
    }
    REQUIRE(rings.size() == 60);
    const auto sword = iw.w->give_item(1, *g.equipment(jx::zone::equip_meleeweapon, 0, 2, 1));
    REQUIRE(sword != 0);
    start_trade(iw, b);
    jx::zone::KNpc& A = *iw.w->mutable_entity(iw.id);
    jx::zone::KNpc& B = *iw.w->mutable_entity(b);
    REQUIRE(iw.w->item_move_request(1, sword, jx::zone::room_trade, 0, 0, 1));
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_DECISION, jx::EntityId{}, 2, ""));
    REQUIRE(iw.w->trade_request(2, jx::pb::TRADE_DECISION, jx::EntityId{}, 2, ""));
    REQUIRE(iw.w->trade_request(1, jx::pb::TRADE_DECISION, jx::EntityId{}, 1, ""));
    iw.w->take_outbox();
    // B's ok: the sword does not fit B's bag (0x081FA250 on the presser's list): B, whose bag is full, hears "self room
    // full" (0xb) and loses its ok, A hears "dest room full" (0xc) and keeps its ok; the trade goes on
    CHECK_FALSE(iw.w->trade_request(2, jx::pb::TRADE_DECISION, jx::EntityId{}, 1, ""));
    CHECK(iw.w->trading(A));
    CHECK(A.player.trade.ok);
    CHECK_FALSE(B.player.trade.ok);
    {
        auto out = iw.w->take_outbox();
        auto ma = sys_msgs(out, 1), mb = sys_msgs(out, 2);
        REQUIRE((ma.size() == 1 && mb.size() == 1));
        CHECK(ma[0].id() == 0xc);
        CHECK(mb[0].id() == 0xb);
    }
    CHECK(iw.list().find(sword) != nullptr);
    // A dies: the trade is cancelled for both, the sword back in A's bag
    std::array<jx::zone::KMagicAttrib, jx::zone::kSkillAttribs> hit{};
    hit[0] = jx::zone::KMagicAttrib{jx::zone::magic_seriesdamage_p, {100, 0, 0}};
    hit[1] = jx::zone::KMagicAttrib{jx::zone::magic_attackrating_v, {50000, 0, 0}};
    hit[2] = jx::zone::KMagicAttrib{jx::zone::magic_ignoredefense_p, {1, 0, 0}};
    hit[3] = jx::zone::KMagicAttrib{0, {200000000, 0, 200000000}};
    iw.w->receive_damage(A, A, 0, false, hit.data(), false, 1, 0x1f, 0);
    REQUIRE(A.doing == jx::zone::KDoing::death);
    CHECK_FALSE(iw.w->trading(A));
    CHECK_FALSE(iw.w->trading(B));
    CHECK(iw.list().place_of(sword)->room == jx::zone::room_equipment);
    // a leave while trading: the partner is let go
    A.doing = jx::zone::KDoing::stand;
    A.cur.life = 100;
    start_trade(iw, b);
    CHECK(iw.w->trading(B));
    REQUIRE(iw.w->remove_player(1));
    CHECK_FALSE(iw.w->trading(B));
    CHECK(B.player.menu.state == jx::zone::menu_state_normal);
}
