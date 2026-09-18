// Items: the tables, the grid, a player's list and the generator - the rules of KItemList.cpp /
// KInventory.cpp / KItemGenerator.cpp of the old core, on a small table of our own and, when the
// exported tables are there, on the real ones.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
    CHECK(set.size() == 11);
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

    ItemWorld()
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
        w = std::make_unique<jx::zone::KSubWorld>(cfg);
        jx::pb::RoleData role;
        role.set_player_id(11);
        role.set_name("A");
        role.set_level(9);
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

    // an unknown item, the trade box, the quick slots for a sword
    REQUIRE_FALSE(iw.w->item_move_request(1, 999, jx::zone::room_equipment, 0, 0, 25));
    REQUIRE_FALSE(iw.w->item_move_request(1, ring, jx::zone::room_trade, 0, 0, 26));
    REQUIRE_FALSE(iw.w->item_move_request(1, sword, jx::zone::room_immediacy, 0, 0, 27));
    out = iw.w->take_outbox();
    results = packets(out, 1, jx::pb::G2C_ITEM_RESULT);
    REQUIRE(results.size() == 3);
    CHECK(decode_packet<jx::pb::ItemResult>(results[0]).result() == jx::pb::RESULT_NOT_FOUND);
    CHECK(decode_packet<jx::pb::ItemResult>(results[1]).result() == jx::pb::RESULT_BAD_REQUEST);
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

TEST_CASE("eating a medicine: LifePotionV merges, heals every 10 frames, the item goes or its stack shrinks", "[item][world]")
{
    ItemWorld iw;
    auto g = iw.gen();
    jx::zone::KNpc* me = const_cast<jx::zone::KNpc*>(iw.w->find_player(1));
    REQUIRE(me != nullptr);
    me->life_max = 1000;
    me->life = 100;
    me->life_replenish = 0;
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
    CHECK(me->life == 100);
    iw.w->tick();
    CHECK(me->life == 110);
    out = iw.w->take_outbox();
    REQUIRE(packets(out, 1, jx::pb::G2C_ENTITY_LIFE).size() == 1);
    CHECK(decode_packet<jx::pb::EntityLife>(packets(out, 1, jx::pb::G2C_ENTITY_LIFE)[0]).delta() == 10);
    for (int i = 0; i < 90; ++i) iw.w->tick();
    CHECK(me->life == 200);   // 10 heals in all
    CHECK(me->life_state.time == 0);
    iw.w->tick();
    CHECK(me->life == 200);   // and no more

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
    me->life_replenish_percent = 50;
    me->life = 100;
    me->life_state = {20, 10};
    for (int i = 0; i < 10; ++i) iw.w->tick();
    CHECK(me->life == 110);

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
    me->forbid_medicine = true;
    REQUIRE_FALSE(iw.w->item_use_request(1, d, 47));
    me->forbid_medicine = false;
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
    ctx = jx::zone::KScriptContext{};

    // the chat: "?gm ds <lua>" runs for the player only while gm_chat is on
    REQUIRE(iw.w->chat(1, "?gm ds AddItem(0,0,0,1,2,0)"));
    out = iw.w->take_outbox();
    CHECK(iw.list().size() == 7);                                   // off: it was said, not run
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
