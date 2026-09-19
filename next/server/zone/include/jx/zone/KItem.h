// Items: the tables they come from, one item, the grid it lies in, and a player's item list.
//
// Ported from the old Core - KItem.h, KBasPropTbl.h (the tables), KInventory.h (the grid),
// KItemList.h (rooms and equipment slots), KItemGenerator.cpp (making an item from a row) - with
// the same numbers: the bag is 6 x 10 cells, the repository 6 x 10, the trade box 10 x 4, three
// quick slots, fifteen equipment parts (ITEM_PART).  The tables are read from the JSON that
// `jxassets export-items` writes out of settings\item (docs/OLD-TO-NEW.md); a template is
// found the way KItemGenerator found its row: equipment by (detail, particular, level) as row
// particular * 10 + level - 1 (a mask by particular alone), medicine as detail * 5 + level - 1,
// a quest item by detail, a gold piece by its row id, a script item by (detail, particular).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "jx/role.pb.h"
#include "jx/zone/KRandom.h"
#include "jx/zone/KMagicAttrib.h"

namespace jx::zone {

// ITEMGENRE of KItem.h
enum class KItemGenre : int {
    equip = 0,
    medicine = 1,
    mine = 2,
    materials = 3,
    task = 4,
    town_portal = 5,
    magic_script = 6,
    broken = 7,
};

// EQUIPDETAILTYPE of KItem.h (+ the three JX2 parts)
enum KEquipDetail : int {
    equip_meleeweapon = 0,
    equip_rangeweapon,
    equip_armor,
    equip_ring,
    equip_amulet,
    equip_boots,
    equip_belt,
    equip_helm,
    equip_cuff,
    equip_pendant,
    equip_horse,
    equip_mask,
    equip_mantle,
    equip_signet,
    equip_shipin,
    equip_detailnum,
};

// ITEM_PART of GameDataDef.h: the equipment slots on the body
enum KItemPart : int {
    itempart_head = 0,
    itempart_body,
    itempart_belt,
    itempart_weapon,
    itempart_foot,
    itempart_cuff,
    itempart_amulet,
    itempart_ring1,
    itempart_ring2,
    itempart_pendant,
    itempart_horse,
    itempart_mask,
    itempart_mantle,
    itempart_signet,
    itempart_shipin,
    itempart_num,
};

// The rooms a player's items lie in (INVENTORY_ROOM without the trade backups) and the equipment
// slots as a pseudo room, so one (room, x, y) names any place an item can be.
// jx_linux_y keeps fifteen rooms (KItemList+0x4c8c + room x 0x1c; Init 0x081FF0B0): 0 the bag 6x10, 1 the repository 6x10,
// 2 the trade box 8x4, 3 the extended repository 12x20, 5 the quick slots 9x1, 7 / 8 / 9 / 14 more 6x10 pages - the zone
// keeps the four of the 2004 game.
enum KItemRoom : int {
    room_equipment = 0,   // the bag (6 x 10)
    room_repository,      // the storage box (6 x 10)
    room_trade,           // the trade box (8 x 4: KItemList::Init of jx_linux_y 0x081FF129, the 2.0 window's SelfItemsBox)
    room_immediacy,       // the quick slots (3 x 1)
    room_num,
    room_body = 10,       // worn: x = KItemPart
};

constexpr int kEquipmentRoomWidth = 6, kEquipmentRoomHeight = 10;
constexpr int kRepositoryRoomWidth = 6, kRepositoryRoomHeight = 10;
constexpr int kTradeRoomWidth = 8, kTradeRoomHeight = 4;
constexpr int kImmediacyRoomWidth = 3, kImmediacyRoomHeight = 1;

// MAGIC_ATTRIB ids (magic_weapondamagemin_v = 28 ...) and KMagicAttrib: KMagicAttrib.h

// One row of an item table (KBASICPROP_* of KBasPropTbl.h, all kinds in one shape)
struct KItemTemplate {
    int row = 0;                 // 1-based data row in its table
    KItemGenre genre = KItemGenre::equip;
    int detail = 0;
    int particular = 0;
    std::string name;
    std::string image;           // \spr\item\... as the game wrote it
    int obj = 0;                 // the object dropped on the ground (ObjData.txt row)
    int width = 1, height = 1;   // cells in a bag
    std::string intro;
    int series = -1;
    int price = 0;
    int level = 0;
    struct Basic { int type; int min; int max; };
    std::vector<Basic> basics;                 // KEQCP_BASIC x 7: rolled between min and max when the item is made
    std::vector<KMagicAttrib> reqs;            // KEQCP_REQ x 6: value[0] = the requirement
    std::vector<KMagicAttrib> med_attribs;     // medicine: value[0] amount, value[1] time (game loops)
    std::vector<int> magic_ids;                // gold: rows of gold_magic
    int group = 0, ex_group = 0, group_serial = 0;   // gold: the set
    std::string script;                        // script item
    int skill = 0;
    int max_stack = 0;
    bool stackable = false;      // 是否叠放 of potion.txt / magicscript.txt: EatMecidine takes one off the stack, else the item
    int can_sell = 1;
};

// KMAGICATTRIB_TABFILE: a prefix / suffix (magicattrib.txt)
struct KMagicTemplate {
    int row = 0;
    std::string name;
    int pos = 0;      // 1 prefix, 0 suffix
    int series = -1;  // required series, -1 any
    int level = 0;
    int kind = 0;
    std::array<std::pair<int, int>, 3> ranges{};
    std::vector<int> drop_rates;   // per equipment detail type (m_DropRate[MATF_CBDR]: 12 in the JX2 server)
};

// magicattrib_limit.txt of the JX2 server (one per version folder; jx_linux_y loader 0x0806BE00,
// check 0x08069D10): an attribute type a new item may only roll with every parameter k strictly
// inside (min[k], max[k]); a max of -1 (the default of a missing column) turns that parameter's
// check off.  The 004 table forbids allskill_v (139) with 0..0: "no more +N all skills".
struct KMagicLimit {
    int type = 0;
    std::array<int, 3> min{-1, -1, -1};
    std::array<int, 3> max{-1, -1, -1};
};

// MATF_* of KBasPropTbl.h as the JX2 server has them: 12 equipment types (the drop-rate columns of
// magicattrib.txt: melee, range, armor, ring, amulet, boots, belt, helm, cuff, pendant, horse,
// mask), five series, ten levels
constexpr int kMagicTypes = 12, kMagicSeries = 5, kMagicLevels = 10;

// KLibOfBPT: every table of one item set (one version folder)
class KItemTemplateSet {
public:
    // items/<set>.json of jxassets export-items; error text on failure
    bool load(const std::string& path, std::string* error);

    [[nodiscard]] const std::string& version() const noexcept { return version_; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

    // KItemGenerator::Gen_Equipment: row = particular * 10 + level - 1, a mask by particular
    [[nodiscard]] const KItemTemplate* equipment(int detail, int particular, int level) const;
    // Gen_Medicine: row = detail * 5 + level - 1
    [[nodiscard]] const KItemTemplate* medicine(int detail, int level) const;
    // Gen_Quest: row = detail
    [[nodiscard]] const KItemTemplate* quest(int detail) const;
    // the DetailType of the questkey.txt row with that 名称 (the JX2 script api takes names:
    // DelItem("Thư giới thiệu") looks the row up in \settings\item\questkey.txt), -1 when none
    [[nodiscard]] int quest_detail_of(const std::string& name) const;
    // Gen_TownPortal: the first row
    [[nodiscard]] const KItemTemplate* town_portal() const;
    // Gen_GoldEquip: row id 1-based
    [[nodiscard]] const KItemTemplate* gold(int row_id) const;
    // Gen_MAScript: FindMAScriptRecord(detail, particular)
    [[nodiscard]] const KItemTemplate* magic_script(int detail, int particular) const;
    [[nodiscard]] const KMagicTemplate* gold_magic(int row_id) const;   // magicattrib_ge.txt, 1-based
    [[nodiscard]] const std::vector<KMagicTemplate>& magic() const noexcept { return magic_; }
    [[nodiscard]] int suite_activate_count(int suite) const;
    // KLibOfBPT::GetCMIT (jx_linux_y 0x08070B50): the rows of magicattrib.txt a (prefix 1 / suffix
    // 0, equipment detail type, series, level 1..10) may draw from - indices into magic().  Built
    // the way 0x08070D00 builds m_CMAIT: a row is listed under every type it has a rate for, every
    // series it allows (-1 = all five) and every level from its own up to 10.  nullptr when the
    // place is outside the table (the old code returned NULL and the roll stopped).
    [[nodiscard]] const std::vector<int>* magic_candidates(int pos, int detail, int series, int level) const;
    [[nodiscard]] const KMagicLimit* magic_limit(int type) const;
    [[nodiscard]] std::size_t magic_limit_count() const noexcept { return limits_.size(); }

private:
    void build_magic_index();
    std::string version_;
    std::size_t count_ = 0;
    std::array<std::vector<KItemTemplate>, equip_detailnum> equipment_;
    std::vector<KItemTemplate> gold_, medicine_, quest_, town_portal_, scripts_;
    std::vector<KMagicTemplate> magic_, gold_magic_;
    std::map<int, int> suites_;
    std::vector<std::vector<int>> cmait_;   // [pos][type][series][level - 1], 2 x 12 x 5 x 10
    std::map<int, KMagicLimit> limits_;
};

// Every item table set the zone knows, by version: items/v000.json .. of jxassets export-items
// (the JX2 server keeps one folder per version and an item remembers its version), or
// items/base.json as version 0 when there are no version folders.
class KItemLibrary {
public:
    // loads every set in the folder; false with an error when none could be read
    bool load_dir(const std::string& dir, std::string* error);
    bool add(std::uint32_t version, KItemTemplateSet set);
    [[nodiscard]] const KItemTemplateSet* set(std::uint32_t version) const;
    [[nodiscard]] std::size_t size() const noexcept { return sets_.size(); }
    [[nodiscard]] std::vector<std::uint32_t> versions() const;
    // the newest version: what new items are made from unless the caller says otherwise
    [[nodiscard]] std::uint32_t default_version() const noexcept { return default_version_; }
    void set_default_version(std::uint32_t v) noexcept { default_version_ = v; }

private:
    std::map<std::uint32_t, KItemTemplateSet> sets_;
    std::uint32_t default_version_ = 0;
};

// KItem: one item a player holds (the server side of KItem.h)
struct KItem {
    std::uint32_t id = 0;        // unique within the player's list, never reused while it lives
    std::uint32_t version = 0;   // the table set it was made from
    KItemGenre genre = KItemGenre::equip;
    int detail = 0;
    int particular = 0;
    int level = 0;
    int series = -1;
    int count = 1;               // stack
    int durability = -1;         // -1 = never wears (KItem::m_nCurrentDur, +0x308)
    int amulet_tier = 0;         // +0x344 (byte): the tier 0..10 of an amulet (0x0806AD90 raises it); above 5 the AdvPlatina wear rates apply - nothing sets it in the zone yet
    int ex_type = 0;             // ITEMEXTENDTYPE: 0 normal, 1 gold, 2 platina, 3 purple
    int gen_param = 0;           // gold: row id; script: row
    std::uint32_t rand_seed = 0; // +0x1e0: the seed the piece was rolled from (Gen_Equipment 0x0806B3A0 stores g_GetRandomSeed();
                                 // Lua_NewItem 0x0811F230 with a seed sets the RNG to it first, 0x0811F824) - ITEM_GetItemRandSeed
    std::array<int, 6> magic_level{};   // +0x1e4..+0x1f8: the six levels of the roll; SetItemMagicLevel 0x080FD020 writes one (the
                                        // scripts tag a quest item with a task id there)
    int luck = 0;                // +0x200: the luck of the roll (GetItemProp's sixth value, 0x080FF33B)
    int group = 0, ex_group = 0, group_serial = 0;
    std::array<KMagicAttrib, 7> base{};     // m_aryBaseAttrib
    std::array<KMagicAttrib, 6> require{};  // m_aryRequireAttrib
    std::array<KMagicAttrib, 6> magic{};    // m_aryMagicAttrib
    std::array<KMagicAttrib, 2> magic_ex{}; // m_aryMagicAttribEx
    const KItemTemplate* tpl = nullptr;     // never null for a made item

    [[nodiscard]] int width() const noexcept { return tpl ? tpl->width : 1; }
    [[nodiscard]] int height() const noexcept { return tpl ? tpl->height : 1; }
    [[nodiscard]] int price() const noexcept { return tpl ? tpl->price : 0; }
    [[nodiscard]] int max_stack() const noexcept { return tpl ? tpl->max_stack : 0; }
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] int max_durability() const noexcept;      // KItem::GetMaxDurability: the durability base attribute, -1 when none
    [[nodiscard]] int total_magic_level() const noexcept;   // KItem::GetTotalMagicLevel
    // KItem::Abrade (jx_linux_y 0x08066570): one in `range` chance to lose a point; returns the durability
    // left (0 = broke now), -1 when the piece does not wear (no durability, already 0, no range)
    int abrade(int range, std::minstd_rand& rng);
    // KItem 0x080658B0(this, n): n percent of the durability off (1..100, a piece that wears); returns what
    // is left (0 = broke now), -1 when it does not wear, the old value when n is out of range or it is 0
    int abrade_percent(int percent) noexcept;

    // RoleData.items: what is saved with the character and how it comes back.  from_proto finds
    // the template again through the library (false when its table set or row is gone).
    void to_proto(pb::ItemData& out) const;
    static bool from_proto(const pb::ItemData& in, const KItemLibrary& lib, KItem& out);
};

// KInventory: a grid of cells, each holding the id of the item lying on it (0 = free).  An item
// of w x h cells covers a rectangle; the top-left cell is its position.
class KInventory {
public:
    KInventory() = default;
    KInventory(int width, int height) { init(width, height); }
    void init(int width, int height);
    void clear();
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t at(int x, int y) const noexcept;
    [[nodiscard]] bool check_room(int x, int y, int w, int h) const noexcept;   // KInventory::CheckRoom: the rectangle is inside and free
    [[nodiscard]] int free_cells() const noexcept;   // KItemList::CalcFreeCellCount 0x081F8A90: the cells holding nothing, column by column
    bool place(int x, int y, std::uint32_t id, int w, int h);                   // KInventory::PlaceItem
    bool pick_up(std::uint32_t id, int x, int y, int w, int h);                 // KInventory::PickUpItem
    [[nodiscard]] bool find_room(int w, int h, int& x, int& y) const;           // KInventory::FindRoom: column by column, like the old one
    [[nodiscard]] int money() const noexcept { return money_; }
    void set_money(int m) noexcept { money_ = m < 0 ? 0 : m; }

private:
    int width_ = 0, height_ = 0;
    std::vector<std::uint32_t> cells_;
    int money_ = 0;
};

// Where an item lies: a room and a cell, or a body part (room_body, x = part)
struct KItemPlace {
    int room = room_equipment;
    int x = 0, y = 0;
};

// KItemList: everything one player owns and where it is.
// KItemSet+0x20.. of the JX2 server: \settings\item\AbradeRate.ini, read by KItemSet::Init 0x0806E250
// (jxassets export-abrade-rate -> abrade_rate.json).  The one-in-N chance a worn piece loses a point
// of durability, by what the player does (mode 0 an attack [Attack], 1 a hit taken [Defend], 2 a step
// [Move]) and by part (KItemPart; 15 slots, Head..Mask named in the ini), with a second table
// [AdvPlatina_*] for an amulet above tier 5 (KItemSet 0x0806D560); 0 = never.  [Repair] carries the
// repair price scales and the warning line (docs/LINUX-SERVER.md §16.3).
struct KAbradeRate {
    static constexpr int kModes = 3;
    std::array<std::array<int, itempart_num>, kModes> rate{};
    std::array<std::array<int, itempart_num>, kModes> adv_rate{};
    int repair_item_price_scale = 0;   // KItemSet+0x188
    int repair_magic_price_scale = 0;  // +0x18c
    int repair_warning_baseline = 0;   // +0x190
    static std::optional<KAbradeRate> load(const std::string& file, std::string* error);
    // 0x0806D560(set, item, mode, part): 0 outside the tables; the AdvPlatina table for an amulet (detail 4)
    // whose tier is above 5, the plain one otherwise
    [[nodiscard]] int range_of(const KItem& item, int mode, int part) const noexcept;
};

class KItemList {
public:
    KItemList();
    void clear();

    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
    [[nodiscard]] const KItem* find(std::uint32_t id) const;
    [[nodiscard]] KItem* find_mutable(std::uint32_t id);
    [[nodiscard]] std::optional<KItemPlace> place_of(std::uint32_t id) const;
    [[nodiscard]] std::uint32_t item_at(int room, int x, int y) const;     // 0 = nothing there
    [[nodiscard]] std::uint32_t equipped(int part) const;                  // KItemList::GetEquipment
    [[nodiscard]] const KInventory& room(int room) const { return rooms_[static_cast<std::size_t>(room)]; }
    void each(const std::function<void(const KItem&, const KItemPlace&)>& fn) const;

    // The next id a new item gets (persisted with the player so ids never repeat after a reload)
    [[nodiscard]] std::uint32_t next_id() const noexcept { return next_id_; }
    void set_next_id(std::uint32_t id) noexcept { next_id_ = id; }

    // Put an item at a place (KItemList::Add): the cells must be free.  With auto_place the first
    // free spot of the room is used (KInventory::FindRoom).  Returns the item id, 0 when it does
    // not fit.  An item with id 0 gets the next id.
    std::uint32_t add(KItem item, int room, int x, int y);
    std::uint32_t add(KItem item, int room);
    // A stackable item joins a stack of the same kind in that room first (up to max_stack)
    std::uint32_t add_or_stack(KItem item, int room, int* stacked_into);
    bool remove(std::uint32_t id);                                    // KItemList::Remove
    bool move(std::uint32_t id, int room, int x, int y);              // ExchangeItem for an empty target
    bool swap(std::uint32_t id, std::uint32_t other);                 // two items trade places when both fit
    // KItemList::ExchangeItem in one step: the item goes to (room, x, y); the one item under that
    // rectangle, if any, takes the item's old place (the hand of the old game put it back there).
    // False when two items lie under the target, the other does not fit the old place, or the
    // item comes off the body (that is unequip).  `displaced` gets the other item's id.
    bool exchange(std::uint32_t id, int room, int x, int y, std::uint32_t* displaced);
    // KInventory::CheckSameDetailType: an item of that genre and detail type in the room (not `except`)
    [[nodiscard]] std::uint32_t same_detail_in(int room, KItemGenre genre, int detail, std::uint32_t except = 0) const;

    // Equipment.  KItemList::GetEquipPlace / Fit: which part a detail type goes to
    [[nodiscard]] static int equip_place(int detail) noexcept;
    [[nodiscard]] static bool fits(int detail, int part) noexcept;
    // KItemList::CanEquip: the part fits and every requirement holds.  `attrib(type)` answers the
    // requirement's subject for this player (level, series, sex, strength, ...).
    [[nodiscard]] bool can_equip(const KItem& item, int part, const std::function<int(int)>& attrib) const;
    // KItemList::Equip / UnEquip: the item leaves its cells and takes the part; what was worn there
    // goes back to the bag (the first free spot) - false when nothing fits
    bool equip(std::uint32_t id, int part, const std::function<int(int)>& attrib);
    // the same without the requirement check: restoring what was worn when the character was saved
    bool wear(std::uint32_t id, int part);
    bool unequip(int part, int to_room = room_equipment);
    // KItemList::GetEquipEnhance of the JX2 server (jx_linux_y 0x081FD2C0): how many suffixes of
    // the piece worn at `part` are awake - one when the character's element feeds the piece's,
    // one more for each of the two "activating" parts (ms_ActivedEquip, 0x082E7460) whose worn
    // piece's element feeds it; the horse and the JX2 parts after it always count 3.
    [[nodiscard]] int equip_enhance(int part, int player_series) const;
    // KItemList::GetWeaponDamage (jx_linux_y 0x081F9310) / GetWeaponType: the weapon's damage with
    // its magic and enhance, or bare hands (cur_strength / 5 + 1)
    [[nodiscard]] std::pair<int, int> weapon_damage(int cur_strength = 0) const;
    [[nodiscard]] int weapon_type() const;   // -1 none, else the detail type of the weapon worn (0x081F92E0: 0 melee, 1 ranged)
    [[nodiscard]] int weapon_particular() const;   // -1 none, else the particular (the kind of weapon) of the weapon worn (0x081F9F70)
    [[nodiscard]] int armor_defense() const;  // every worn piece's armordefense_v added up

    // money of the bag and the repository (KItemList::GetMoney / AddMoney / CostMoney)
    [[nodiscard]] int money(int room = room_equipment) const noexcept { return rooms_[static_cast<std::size_t>(room)].money(); }
    void set_money(int room, int m) noexcept { rooms_[static_cast<std::size_t>(room)].set_money(m); }
    bool add_money(int room, int m) noexcept;
    bool cost_money(int m) noexcept;          // from the bag

private:
    struct Entry {
        KItem item;
        KItemPlace place;
    };
    bool take_off_cells(Entry& e);
    std::map<std::uint32_t, Entry> items_;
    std::array<KInventory, room_num> rooms_;
    std::array<std::uint32_t, itempart_num> equip_{};
    std::uint32_t next_id_ = 1;
};

// The prefix / suffix levels of a piece to make: slot i (even = prefix, odd = suffix) rolls an
// attribute of that level, 0 ends the list, -1 is a platina socket (pnaryMALevel of the old code)
using KMagicLevels = std::array<int, 6>;

// KItemGenerator: an item from a table row, with the dice of the old core (KRandom).  Equipment
// white or with prefixes / suffixes (Gen_MagicAttrib of jx_linux_y 0x0806AF70), medicine, quest
// items, town portals, script items and gold pieces.
class KItemGenerator {
public:
    // Gen_Equipment gives a rolled set of attributes up to this many tries when magicattrib_limit
    // rejects it (jx_linux_y 0x0806B3A0: 0x15), then makes no item
    static constexpr int kGenEquipmentTries = 21;

    explicit KItemGenerator(const KItemTemplateSet& set, std::uint32_t version = 0, std::uint32_t seed = 1)
        : set_(set), version_(version), rng_(seed) {}
    void seed(std::uint32_t s) noexcept { rng_.seed(s); }
    [[nodiscard]] std::uint32_t seed() const noexcept { return rng_.seed(); }
    [[nodiscard]] std::uint32_t version() const noexcept { return version_; }
    [[nodiscard]] const KItemTemplateSet& set() const noexcept { return set_; }

    // KItemGenerator::Gen_Equipment (jx_linux_y 0x0806B3A0): the row's base attributes rolled,
    // then - with magic levels, not for a mask - Gen_MagicAttrib and the magicattrib_limit check,
    // rolled again up to kGenEquipmentTries times.  No levels (or all zero) is a white item.
    std::optional<KItem> equipment(int detail, int particular, int series, int level, const KMagicLevels* magic_levels = nullptr,
                                   int luck = 0);
    std::optional<KItem> medicine(int detail, int level);
    std::optional<KItem> quest(int detail, int count);
    std::optional<KItem> town_portal();
    std::optional<KItem> magic_script(int detail, int particular, int level, int series, int count);
    std::optional<KItem> gold(int luck, int row_id);                                    // Gen_GoldEquip on the server

    // KItemGenerator::Gen_MagicAttrib of jx_linux_y (0x0806AF70), the same rule as the old source:
    // for each slot i while levels[i] != 0 - prefix for even i, suffix for odd i - the candidates
    // are the rows of magic_candidates(pos, detail, series, level) not used yet on this piece, of
    // a kind not chosen yet, whose drop rate for this type beats a roll `decide`; one of them is
    // drawn evenly and its three parameters rolled inside their ranges.  `decide` depends on the
    // table version the piece comes from (the drop-rate scale of magicattrib.txt changed):
    //   version 0..1: g_Random(100) / (luck / 10 + 1)
    //   version 2..3: g_Random(1000000) / (luck / 10 + 1)
    //   version 4.. : g_Random(1000000) * 100 / (10 * luck + 100)
    // No candidate ends the list.  Returns false only without levels (never here).
    bool gen_magic_attrib(int detail, const KMagicLevels& levels, int series, int luck, std::array<KMagicAttrib, 6>& out);
    // CheckNewItemAttrib (0x08069D10): every rolled attribute against magicattrib_limit
    [[nodiscard]] bool check_new_item_attrib(const std::array<KMagicAttrib, 6>& magic) const;

    // GetRandomNumber(min, max) of the old core: inclusive
    int random_between(int lo, int hi) noexcept { return rng_.between(lo, hi); }
    int random(int n) noexcept { return rng_.random(n); }   // g_Random(n): 0 .. n-1

private:
    void set_attrib_cbr(KItem& item, const KItemTemplate& t);   // KItem::SetAttrib_CBR: rolled base attributes + requirements
    static void set_attrib_ma(KItem& item, const std::array<KMagicAttrib, 6>& magic);   // KItem::SetAttrib_MA
    const KItemTemplateSet& set_;
    std::uint32_t version_;
    KRandom rng_;
};

} // namespace jx::zone
