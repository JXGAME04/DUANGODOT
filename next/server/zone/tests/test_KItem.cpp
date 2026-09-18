// Items: the tables, the grid, a player's list and the generator - the rules of KItemList.cpp /
// KInventory.cpp / KItemGenerator.cpp of the old core, on a small table of our own and, when the
// exported tables are there, on the real ones.
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include "jx/zone/KItem.h"

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
   "price": 50, "level": 1, "attribs": [{"attrib": 153, "value": 10, "time": 100}]}
 ],
 "quest": [
  {"row": 1, "name": "Tram", "genre": 4, "detail": 0, "image": "q.spr", "obj": 41, "w": 1, "h": 1, "intro": "", "particular": 0, "can_sell": 0, "max_stack": 0},
  {"row": 2, "name": "Tui", "genre": 4, "detail": 1, "image": "q.spr", "obj": 41, "w": 1, "h": 1, "intro": "", "particular": 0, "can_sell": 0, "max_stack": 20}
 ],
 "town_portal": [{"row": 1, "name": "Phu", "genre": 5, "image": "t.spr", "obj": 38, "w": 1, "h": 1, "price": 500, "intro": ""}],
 "magic": [],
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
    CHECK(set.size() == 10);
    // equipment: row = particular * 10 + level - 1
    REQUIRE(set.equipment(jx::zone::equip_meleeweapon, 0, 1) != nullptr);
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 1)->name == "Kiem 1");
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 2)->name == "Kiem 2");
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 0, 3) == nullptr);
    CHECK(set.equipment(jx::zone::equip_meleeweapon, 1, 1) == nullptr);   // particular 1 would be row 10
    CHECK(set.equipment(jx::zone::equip_armor, 0, 1)->width == 2);
    // medicine: row = detail * 5 + level - 1; quest: row = detail; gold: row id
    CHECK(set.medicine(0, 1)->name == "Thuoc 1");
    CHECK(set.medicine(0, 2) == nullptr);
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

#include "jx/log.hpp"
#include "jx/zone/KSubWorld.h"

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
    CHECK(again->next_id() == list->next_id());
    CHECK(again->money(jx::zone::room_repository) == 70);

    // an item whose table set is gone is dropped, the rest stays
    jx::pb::RoleData broken = saved;
    broken.mutable_items(0)->set_version(9);
    REQUIRE(w.remove_player(2));
    REQUIRE(w.spawn_player(3, broken, id, p) == jx::pb::RESULT_OK);
    CHECK(w.items_of(3)->size() == 3);
}
