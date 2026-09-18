// Items: tables, one item, the grid, a player's list, the generator.  See KItem.h.
#include "jx/zone/KItem.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "jx/zone/KMath.h"

namespace jx::zone {

namespace {

using nlohmann::json;

int geti(const json& j, const char* key, int def = 0)
{
    const auto it = j.find(key);
    return it != j.end() && it->is_number() ? it->get<int>() : def;
}

std::string gets(const json& j, const char* key)
{
    const auto it = j.find(key);
    return it != j.end() && it->is_string() ? it->get<std::string>() : std::string();
}

KItemTemplate common(const json& j, KItemGenre genre)
{
    KItemTemplate t;
    t.row = geti(j, "row");
    t.genre = genre;
    t.detail = geti(j, "detail");
    t.particular = geti(j, "particular");
    t.name = gets(j, "name");
    t.image = gets(j, "image");
    t.obj = geti(j, "obj");
    t.width = std::max(1, geti(j, "w", 1));
    t.height = std::max(1, geti(j, "h", 1));
    t.intro = gets(j, "intro");
    t.series = geti(j, "series", -1);
    t.price = geti(j, "price");
    t.level = geti(j, "level");
    t.max_stack = geti(j, "max_stack");
    t.stackable = geti(j, "stackable") != 0;
    t.can_sell = geti(j, "can_sell", 1);
    return t;
}

KItemTemplate equipment_of(const json& j)
{
    KItemTemplate t = common(j, KItemGenre::equip);
    if (const auto it = j.find("basics"); it != j.end()) {
        for (const auto& b : *it) {
            const auto& r = b.at("range");
            t.basics.push_back({geti(b, "type"), geti(r, "min"), geti(r, "max")});
        }
    }
    if (const auto it = j.find("reqs"); it != j.end()) {
        for (const auto& r : *it) {
            KMagicAttrib a;
            a.type = geti(r, "type");
            a.value[0] = geti(r, "para");
            t.reqs.push_back(a);
        }
    }
    if (const auto it = j.find("magic_ids"); it != j.end()) {
        for (const auto& m : *it) t.magic_ids.push_back(m.get<int>());
    }
    t.group = geti(j, "group");
    t.ex_group = geti(j, "ex_group");
    t.group_serial = geti(j, "group_serial");
    return t;
}

} // namespace

// ---- KItemTemplateSet ---------------------------------------------------------------------

bool KItemTemplateSet::load(const std::string& path, std::string* error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        if (error) *error = path + ": " + e.what();
        return false;
    }
    version_ = gets(j, "version");
    count_ = 0;
    static const char* const kTables[equip_detailnum] = {"meleeweapon", "rangeweapon", "armor", "ring", "amulet", "boot",
                                                         "belt", "helm", "cuff", "pendant", "horse", "mask", "mantle", "signet", "shipin"};
    if (const auto eq = j.find("equipment"); eq != j.end()) {
        for (std::size_t d = 0; d < equip_detailnum; ++d) {
            equipment_[d].clear();
            const auto it = eq->find(kTables[d]);
            if (it == eq->end()) continue;
            for (const auto& row : *it) equipment_[d].push_back(equipment_of(row));
            count_ += equipment_[d].size();
        }
    }
    gold_.clear();
    if (const auto it = j.find("gold"); it != j.end()) {
        for (const auto& row : *it) gold_.push_back(equipment_of(row));
    }
    medicine_.clear();
    if (const auto it = j.find("medicine"); it != j.end()) {
        for (const auto& row : *it) {
            KItemTemplate t = common(row, KItemGenre::medicine);
            if (const auto a = row.find("attribs"); a != row.end()) {
                for (const auto& m : *a) {
                    KMagicAttrib x;
                    x.type = geti(m, "attrib");
                    x.value[0] = geti(m, "value");
                    x.value[1] = geti(m, "time");
                    t.med_attribs.push_back(x);
                }
            }
            medicine_.push_back(std::move(t));
        }
    }
    quest_.clear();
    if (const auto it = j.find("quest"); it != j.end()) {
        for (const auto& row : *it) {
            KItemTemplate t = common(row, KItemGenre::task);
            t.level = 1;   // KItem::operator=(KBASICPROP_QUEST)
            quest_.push_back(std::move(t));
        }
    }
    town_portal_.clear();
    if (const auto it = j.find("town_portal"); it != j.end()) {
        for (const auto& row : *it) town_portal_.push_back(common(row, KItemGenre::town_portal));
    }
    scripts_.clear();
    if (const auto it = j.find("scripts"); it != j.end()) {
        for (const auto& row : *it) {
            KItemTemplate t = common(row, KItemGenre::magic_script);
            t.script = gets(row, "script");
            t.skill = geti(row, "skill");
            scripts_.push_back(std::move(t));
        }
    }
    const auto read_magic = [](const json& arr, std::vector<KMagicTemplate>& into) {
        into.clear();
        for (const auto& row : arr) {
            KMagicTemplate m;
            m.row = geti(row, "row");
            m.name = gets(row, "name");
            m.pos = geti(row, "pos");
            m.series = geti(row, "class", -1);
            m.level = geti(row, "level");
            m.kind = geti(row, "kind");
            if (const auto r = row.find("ranges"); r != row.end()) {
                std::size_t i = 0;
                for (const auto& p : *r) {
                    if (i < 3) m.ranges[i++] = {geti(p, "min"), geti(p, "max")};
                }
            }
            if (const auto d = row.find("drop_rates"); d != row.end()) {
                for (const auto& x : *d) m.drop_rates.push_back(x.get<int>());
            }
            into.push_back(std::move(m));
        }
    };
    if (const auto it = j.find("magic"); it != j.end()) read_magic(*it, magic_);
    if (const auto it = j.find("gold_magic"); it != j.end()) read_magic(*it, gold_magic_);
    suites_.clear();
    if (const auto it = j.find("suites"); it != j.end()) {
        for (const auto& row : *it) suites_[geti(row, "suite")] = geti(row, "count");
    }
    limits_.clear();
    if (const auto it = j.find("magic_limits"); it != j.end()) {
        for (const auto& row : *it) {
            KMagicLimit lim;
            lim.type = geti(row, "type");
            if (lim.type < 1 || lim.type > 0x153) continue;   // the loader of the JX2 server skips those rows
            const auto read3 = [&](const char* key, std::array<int, 3>& into) {
                const auto a = row.find(key);
                if (a == row.end()) return;
                std::size_t k = 0;
                for (const auto& x : *a) {
                    if (k < 3 && x.is_number()) into[k] = x.get<int>();
                    ++k;
                }
            };
            read3("min", lim.min);
            read3("max", lim.max);
            limits_[lim.type] = lim;   // the last row of a type wins, as in the std::map of the server
        }
    }
    build_magic_index();
    count_ += gold_.size() + medicine_.size() + quest_.size() + town_portal_.size() + scripts_.size();
    return true;
}

// KLibOfBPT::InitMALib / the m_CMAIT builder of the JX2 server (jx_linux_y 0x08070D00)
void KItemTemplateSet::build_magic_index()
{
    cmait_.assign(static_cast<std::size_t>(2 * kMagicTypes * kMagicSeries * kMagicLevels), {});
    for (std::size_t i = 0; i < magic_.size(); ++i) {
        const KMagicTemplate& m = magic_[i];
        if (m.pos < 0 || m.pos > 1) continue;
        for (int type = 0; type < kMagicTypes && static_cast<std::size_t>(type) < m.drop_rates.size(); ++type) {
            if (m.drop_rates[static_cast<std::size_t>(type)] == 0) continue;   // never on this kind of equipment
            int s0 = m.series, s1 = m.series;
            if (m.series == -1) {
                s0 = 0;
                s1 = kMagicSeries - 1;
            } else if (m.series < 0 || m.series >= kMagicSeries) {
                continue;
            }
            for (int s = s0; s <= s1; ++s) {
                for (int level = std::max(m.level, 1); level <= kMagicLevels; ++level) {
                    const auto at = static_cast<std::size_t>(((m.pos * kMagicTypes + type) * kMagicSeries + s) * kMagicLevels + level - 1);
                    cmait_[at].push_back(static_cast<int>(i));
                }
            }
        }
    }
}

const std::vector<int>* KItemTemplateSet::magic_candidates(int pos, int detail, int series, int level) const
{
    if (pos < 0 || pos > 1 || detail < 0 || detail >= kMagicTypes || series < 0 || series >= kMagicSeries || level < 1 ||
        level > kMagicLevels || cmait_.empty()) {
        return nullptr;
    }
    return &cmait_[static_cast<std::size_t>(((pos * kMagicTypes + detail) * kMagicSeries + series) * kMagicLevels + level - 1)];
}

const KMagicLimit* KItemTemplateSet::magic_limit(int type) const
{
    const auto it = limits_.find(type);
    return it == limits_.end() ? nullptr : &it->second;
}

const KItemTemplate* KItemTemplateSet::equipment(int detail, int particular, int level) const
{
    if (detail < 0 || detail >= equip_detailnum) return nullptr;
    const auto& rows = equipment_[static_cast<std::size_t>(detail)];
    const int i = detail == equip_mask ? particular : particular * 10 + level - 1;
    if (i < 0 || static_cast<std::size_t>(i) >= rows.size()) return nullptr;
    return &rows[static_cast<std::size_t>(i)];
}

const KItemTemplate* KItemTemplateSet::medicine(int detail, int level) const
{
    const int i = detail * 5 + level - 1;
    if (i < 0 || static_cast<std::size_t>(i) >= medicine_.size()) return nullptr;
    return &medicine_[static_cast<std::size_t>(i)];
}

const KItemTemplate* KItemTemplateSet::quest(int detail) const
{
    if (detail < 0 || static_cast<std::size_t>(detail) >= quest_.size()) return nullptr;
    return &quest_[static_cast<std::size_t>(detail)];
}

int KItemTemplateSet::quest_detail_of(const std::string& name) const
{
    for (const auto& t : quest_) {
        if (t.name == name) return t.detail;
    }
    return -1;
}

const KItemTemplate* KItemTemplateSet::town_portal() const
{
    return town_portal_.empty() ? nullptr : &town_portal_[0];
}

const KItemTemplate* KItemTemplateSet::gold(int row_id) const
{
    if (row_id <= 0 || static_cast<std::size_t>(row_id) > gold_.size()) return nullptr;
    return &gold_[static_cast<std::size_t>(row_id - 1)];
}

const KItemTemplate* KItemTemplateSet::magic_script(int detail, int particular) const
{
    for (const auto& t : scripts_) {
        if (t.detail == detail && t.particular == particular) return &t;
    }
    return nullptr;
}

const KMagicTemplate* KItemTemplateSet::gold_magic(int row_id) const
{
    if (row_id <= 0 || static_cast<std::size_t>(row_id) > gold_magic_.size()) return nullptr;
    return &gold_magic_[static_cast<std::size_t>(row_id - 1)];
}

int KItemTemplateSet::suite_activate_count(int suite) const
{
    const auto it = suites_.find(suite);
    return it == suites_.end() ? 0 : it->second;
}

// ---- KItemLibrary -------------------------------------------------------------------------

bool KItemLibrary::load_dir(const std::string& dir, std::string* error)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        if (error) *error = "no folder " + dir;
        return false;
    }
    std::vector<std::pair<std::uint32_t, fs::path>> found;
    fs::path base;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.size() == 9 && name[0] == 'v' && name.ends_with(".json") && std::all_of(name.begin() + 1, name.begin() + 4, ::isdigit)) {
            found.emplace_back(static_cast<std::uint32_t>(std::stoul(name.substr(1, 3))), entry.path());
        } else if (name == "base.json") {
            base = entry.path();
        }
    }
    if (found.empty() && !base.empty()) found.emplace_back(0u, base);
    std::string errors;
    for (const auto& [version, path] : found) {
        KItemTemplateSet set;
        std::string e;
        if (!set.load(path.string(), &e)) {
            errors += (errors.empty() ? "" : "; ") + e;
            continue;
        }
        add(version, std::move(set));
    }
    if (sets_.empty()) {
        if (error) *error = errors.empty() ? "no items/*.json in " + dir : errors;
        return false;
    }
    if (error) *error = errors;
    return true;
}

bool KItemLibrary::add(std::uint32_t version, KItemTemplateSet set)
{
    sets_[version] = std::move(set);
    default_version_ = sets_.rbegin()->first;
    return true;
}

const KItemTemplateSet* KItemLibrary::set(std::uint32_t version) const
{
    const auto it = sets_.find(version);
    return it == sets_.end() ? nullptr : &it->second;
}

std::vector<std::uint32_t> KItemLibrary::versions() const
{
    std::vector<std::uint32_t> out;
    for (const auto& [v, s] : sets_) out.push_back(v);
    return out;
}

// ---- KItem --------------------------------------------------------------------------------

namespace {

void magic_to_proto(const KMagicAttrib& a, pb::ItemMagic& out)
{
    out.set_type(static_cast<std::uint32_t>(a.type));
    for (const int v : a.value) out.add_value(v);
}

KMagicAttrib magic_from_proto(const pb::ItemMagic& in)
{
    KMagicAttrib a;
    a.type = static_cast<int>(in.type());
    for (int i = 0; i < in.value_size() && i < 3; ++i) a.value[static_cast<std::size_t>(i)] = in.value(i);
    return a;
}

} // namespace

void KItem::to_proto(pb::ItemData& out) const
{
    out.set_id(id);
    out.set_version(version);
    out.set_genre(static_cast<std::uint32_t>(genre));
    out.set_detail(static_cast<std::uint32_t>(detail));
    out.set_particular(static_cast<std::uint32_t>(particular));
    out.set_level(static_cast<std::uint32_t>(level));
    out.set_series(series);
    out.set_count(static_cast<std::uint32_t>(count));
    out.set_durability(durability);
    out.set_ex_type(static_cast<std::uint32_t>(ex_type));
    out.set_gen_param(static_cast<std::uint32_t>(gen_param));
    out.set_group(static_cast<std::uint32_t>(group));
    out.set_ex_group(static_cast<std::uint32_t>(ex_group));
    out.set_group_serial(static_cast<std::uint32_t>(group_serial));
    for (const auto& a : base) if (!a.empty()) magic_to_proto(a, *out.add_base());
    for (const auto& a : require) if (!a.empty()) magic_to_proto(a, *out.add_require());
    for (const auto& a : magic) if (!a.empty()) magic_to_proto(a, *out.add_magic());
    for (const auto& a : magic_ex) if (!a.empty()) magic_to_proto(a, *out.add_magic_ex());
}

bool KItem::from_proto(const pb::ItemData& in, const KItemLibrary& lib, KItem& out)
{
    const KItemTemplateSet* set = lib.set(in.version());
    if (set == nullptr) return false;
    const auto genre = static_cast<KItemGenre>(in.genre());
    const KItemTemplate* t = nullptr;
    switch (genre) {
    case KItemGenre::equip:
        t = in.ex_type() == 1 ? set->gold(static_cast<int>(in.gen_param()))
                              : set->equipment(static_cast<int>(in.detail()), static_cast<int>(in.particular()), static_cast<int>(in.level()));
        break;
    case KItemGenre::medicine: t = set->medicine(static_cast<int>(in.detail()), static_cast<int>(in.level())); break;
    case KItemGenre::task: t = set->quest(static_cast<int>(in.detail())); break;
    case KItemGenre::town_portal: t = set->town_portal(); break;
    case KItemGenre::magic_script: t = set->magic_script(static_cast<int>(in.detail()), static_cast<int>(in.particular())); break;
    default: break;
    }
    if (t == nullptr) return false;
    out = KItem{};
    out.id = in.id();
    out.version = in.version();
    out.genre = genre;
    out.detail = static_cast<int>(in.detail());
    out.particular = static_cast<int>(in.particular());
    out.level = static_cast<int>(in.level());
    out.series = in.series();
    out.count = static_cast<int>(in.count() == 0 ? 1 : in.count());
    out.durability = in.durability();
    out.ex_type = static_cast<int>(in.ex_type());
    out.gen_param = static_cast<int>(in.gen_param());
    out.group = static_cast<int>(in.group());
    out.ex_group = static_cast<int>(in.ex_group());
    out.group_serial = static_cast<int>(in.group_serial());
    out.tpl = t;
    for (int i = 0; i < in.base_size() && i < 7; ++i) out.base[static_cast<std::size_t>(i)] = magic_from_proto(in.base(i));
    for (int i = 0; i < in.require_size() && i < 6; ++i) out.require[static_cast<std::size_t>(i)] = magic_from_proto(in.require(i));
    for (int i = 0; i < in.magic_size() && i < 6; ++i) out.magic[static_cast<std::size_t>(i)] = magic_from_proto(in.magic(i));
    for (int i = 0; i < in.magic_ex_size() && i < 2; ++i) out.magic_ex[static_cast<std::size_t>(i)] = magic_from_proto(in.magic_ex(i));
    return true;
}


const std::string& KItem::name() const
{
    static const std::string none;
    return tpl ? tpl->name : none;
}

int KItem::max_durability() const noexcept
{
    for (const auto& a : base) {
        if (a.type == magic_durability_v) return a.value[0];
    }
    return -1;
}

int KItem::total_magic_level() const noexcept
{
    int n = 0;
    for (std::size_t i = 0; i < 6; ++i) {
        if (base[i].type != 0) ++n;
    }
    return n;
}

int KItem::abrade(int range, std::minstd_rand& rng)
{
    if (durability == -1 || range <= 0) return -1;
    if (static_cast<int>(rng() % static_cast<unsigned>(range)) == 0) {
        --durability;
        if (durability == 0) return 0;
    }
    return durability;
}

// ---- KInventory ---------------------------------------------------------------------------

void KInventory::init(int width, int height)
{
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    cells_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0);
    money_ = 0;
}

void KInventory::clear()
{
    std::fill(cells_.begin(), cells_.end(), 0u);
    money_ = 0;
}

std::uint32_t KInventory::at(int x, int y) const noexcept
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return 0;
    return cells_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

bool KInventory::check_room(int x, int y, int w, int h) const noexcept
{
    if (x < 0 || y < 0 || w < 1 || h < 1 || x + w > width_ || y + h > height_) return false;
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            if (at(i, j) != 0) return false;
        }
    }
    return true;
}

bool KInventory::place(int x, int y, std::uint32_t id, int w, int h)
{
    if (id == 0 || !check_room(x, y, w, h)) return false;
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            cells_[static_cast<std::size_t>(j) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(i)] = id;
        }
    }
    return true;
}

bool KInventory::pick_up(std::uint32_t id, int x, int y, int w, int h)
{
    if (x < 0 || y < 0 || w < 1 || h < 1 || x + w > width_ || y + h > height_) return false;
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            if (at(i, j) != id) return false;
        }
    }
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            cells_[static_cast<std::size_t>(j) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(i)] = 0;
        }
    }
    return true;
}

bool KInventory::find_room(int w, int h, int& x, int& y) const
{
    if (w < 1 || w > width_ || h < 1 || h > height_) return false;
    for (int i = 0; i < width_ - w + 1; ++i) {      // KInventory::FindRoom walks columns first
        for (int j = 0; j < height_ - h + 1; ++j) {
            if (check_room(i, j, w, h)) {
                x = i;
                y = j;
                return true;
            }
        }
    }
    x = y = 0;
    return false;
}

// ---- KItemList ----------------------------------------------------------------------------

KItemList::KItemList()
{
    rooms_[room_equipment].init(kEquipmentRoomWidth, kEquipmentRoomHeight);
    rooms_[room_repository].init(kRepositoryRoomWidth, kRepositoryRoomHeight);
    rooms_[room_trade].init(kTradeRoomWidth, kTradeRoomHeight);
    rooms_[room_immediacy].init(kImmediacyRoomWidth, kImmediacyRoomHeight);
    equip_.fill(0);
}

void KItemList::clear()
{
    items_.clear();
    for (auto& r : rooms_) r.clear();
    equip_.fill(0);
    next_id_ = 1;
}

const KItem* KItemList::find(std::uint32_t id) const
{
    const auto it = items_.find(id);
    return it == items_.end() ? nullptr : &it->second.item;
}

KItem* KItemList::find_mutable(std::uint32_t id)
{
    const auto it = items_.find(id);
    return it == items_.end() ? nullptr : &it->second.item;
}

std::optional<KItemPlace> KItemList::place_of(std::uint32_t id) const
{
    const auto it = items_.find(id);
    if (it == items_.end()) return std::nullopt;
    return it->second.place;
}

std::uint32_t KItemList::item_at(int room, int x, int y) const
{
    if (room == room_body) return x >= 0 && x < itempart_num ? equip_[static_cast<std::size_t>(x)] : 0;
    if (room < 0 || room >= room_num) return 0;
    return rooms_[static_cast<std::size_t>(room)].at(x, y);
}

std::uint32_t KItemList::equipped(int part) const
{
    return part >= 0 && part < itempart_num ? equip_[static_cast<std::size_t>(part)] : 0;
}

void KItemList::each(const std::function<void(const KItem&, const KItemPlace&)>& fn) const
{
    for (const auto& [id, e] : items_) fn(e.item, e.place);
}

std::uint32_t KItemList::add(KItem item, int room, int x, int y)
{
    if (room < 0 || room >= room_num || item.tpl == nullptr) return 0;
    if (item.id == 0) item.id = next_id_++;
    else if (items_.contains(item.id)) return 0;
    else next_id_ = std::max(next_id_, item.id + 1);
    if (!rooms_[static_cast<std::size_t>(room)].place(x, y, item.id, item.width(), item.height())) return 0;
    const std::uint32_t id = item.id;
    items_[id] = Entry{std::move(item), KItemPlace{room, x, y}};
    return id;
}

std::uint32_t KItemList::add(KItem item, int room)
{
    if (room < 0 || room >= room_num || item.tpl == nullptr) return 0;
    int x = 0, y = 0;
    if (!rooms_[static_cast<std::size_t>(room)].find_room(item.width(), item.height(), x, y)) return 0;
    return add(std::move(item), room, x, y);
}

std::uint32_t KItemList::add_or_stack(KItem item, int room, int* stacked_into)
{
    if (stacked_into) *stacked_into = 0;
    if (item.max_stack() > 1 && room >= 0 && room < room_num) {
        for (auto& [id, e] : items_) {
            KItem& have = e.item;
            if (e.place.room != room || have.genre != item.genre || have.detail != item.detail || have.particular != item.particular ||
                have.level != item.level || have.series != item.series || have.count >= have.max_stack()) {
                continue;
            }
            const int take = std::min(item.count, have.max_stack() - have.count);
            have.count += take;
            item.count -= take;
            if (stacked_into) *stacked_into = static_cast<int>(id);
            if (item.count <= 0) return id;
        }
    }
    return add(std::move(item), room);
}

bool KItemList::take_off_cells(Entry& e)
{
    if (e.place.room == room_body) {
        if (e.place.x < 0 || e.place.x >= itempart_num || equip_[static_cast<std::size_t>(e.place.x)] != e.item.id) return false;
        equip_[static_cast<std::size_t>(e.place.x)] = 0;
        return true;
    }
    return rooms_[static_cast<std::size_t>(e.place.room)].pick_up(e.item.id, e.place.x, e.place.y, e.item.width(), e.item.height());
}

bool KItemList::remove(std::uint32_t id)
{
    const auto it = items_.find(id);
    if (it == items_.end()) return false;
    if (!take_off_cells(it->second)) return false;
    items_.erase(it);
    return true;
}

bool KItemList::move(std::uint32_t id, int room, int x, int y)
{
    const auto it = items_.find(id);
    if (it == items_.end() || room < 0 || room >= room_num) return false;
    Entry& e = it->second;
    // the item's own cells do not block it: lift it first, put it back on failure
    const KItemPlace old = e.place;
    if (!take_off_cells(e)) return false;
    if (rooms_[static_cast<std::size_t>(room)].place(x, y, id, e.item.width(), e.item.height())) {
        e.place = KItemPlace{room, x, y};
        return true;
    }
    if (old.room == room_body) equip_[static_cast<std::size_t>(old.x)] = id;
    else rooms_[static_cast<std::size_t>(old.room)].place(old.x, old.y, id, e.item.width(), e.item.height());
    return false;
}

bool KItemList::swap(std::uint32_t id, std::uint32_t other)
{
    const auto a = items_.find(id);
    const auto b = items_.find(other);
    if (a == items_.end() || b == items_.end() || id == other) return false;
    if (a->second.place.room == room_body || b->second.place.room == room_body) return false;
    const KItemPlace pa = a->second.place, pb = b->second.place;
    if (!take_off_cells(a->second) || !take_off_cells(b->second)) return false;
    const bool ok = rooms_[static_cast<std::size_t>(pb.room)].place(pb.x, pb.y, id, a->second.item.width(), a->second.item.height()) &&
                    rooms_[static_cast<std::size_t>(pa.room)].place(pa.x, pa.y, other, b->second.item.width(), b->second.item.height());
    if (ok) {
        a->second.place = pb;
        b->second.place = pa;
        return true;
    }
    // undo whatever went down
    rooms_[static_cast<std::size_t>(pb.room)].pick_up(id, pb.x, pb.y, a->second.item.width(), a->second.item.height());
    rooms_[static_cast<std::size_t>(pa.room)].place(pa.x, pa.y, id, a->second.item.width(), a->second.item.height());
    rooms_[static_cast<std::size_t>(pb.room)].place(pb.x, pb.y, other, b->second.item.width(), b->second.item.height());
    return false;
}

bool KItemList::exchange(std::uint32_t id, int room, int x, int y, std::uint32_t* displaced)
{
    if (displaced) *displaced = 0;
    const auto it = items_.find(id);
    if (it == items_.end() || room < 0 || room >= room_num) return false;
    Entry& a = it->second;
    KInventory& inv = rooms_[static_cast<std::size_t>(room)];
    const int aw = a.item.width(), ah = a.item.height();
    if (x < 0 || y < 0 || x + aw > inv.width() || y + ah > inv.height()) return false;
    std::uint32_t other = 0;
    for (int cy = y; cy < y + ah; ++cy) {
        for (int cx = x; cx < x + aw; ++cx) {
            const std::uint32_t c = inv.at(cx, cy);
            if (c == 0 || c == id) continue;
            if (other != 0 && other != c) return false;   // two items under it: nowhere to put both
            other = c;
        }
    }
    if (other == 0) return move(id, room, x, y);
    if (a.place.room == room_body) return false;
    Entry& b = items_.at(other);
    const KItemPlace pa = a.place;
    const int bw = b.item.width(), bh = b.item.height();
    if (!take_off_cells(a)) return false;
    if (!take_off_cells(b)) {
        rooms_[static_cast<std::size_t>(pa.room)].place(pa.x, pa.y, id, aw, ah);
        return false;
    }
    const KItemPlace pb = b.place;
    bool ok = inv.place(x, y, id, aw, ah);
    if (ok && !rooms_[static_cast<std::size_t>(pa.room)].place(pa.x, pa.y, other, bw, bh)) {
        inv.pick_up(id, x, y, aw, ah);
        ok = false;
    }
    if (!ok) {
        rooms_[static_cast<std::size_t>(pa.room)].place(pa.x, pa.y, id, aw, ah);
        rooms_[static_cast<std::size_t>(pb.room)].place(pb.x, pb.y, other, bw, bh);
        return false;
    }
    a.place = KItemPlace{room, x, y};
    b.place = pa;
    if (displaced) *displaced = other;
    return true;
}

std::uint32_t KItemList::same_detail_in(int room, KItemGenre genre, int detail, std::uint32_t except) const
{
    for (const auto& [id, e] : items_) {
        if (id != except && e.place.room == room && e.item.genre == genre && e.item.detail == detail) return id;
    }
    return 0;
}

int KItemList::equip_place(int detail) noexcept
{
    switch (detail) {
    case equip_meleeweapon:
    case equip_rangeweapon: return itempart_weapon;
    case equip_armor: return itempart_body;
    case equip_helm: return itempart_head;
    case equip_boots: return itempart_foot;
    case equip_ring: return itempart_ring1;
    case equip_amulet: return itempart_amulet;
    case equip_belt: return itempart_belt;
    case equip_cuff: return itempart_cuff;
    case equip_pendant: return itempart_pendant;
    case equip_horse: return itempart_horse;
    case equip_mask: return itempart_mask;
    case equip_mantle: return itempart_mantle;
    case equip_signet: return itempart_signet;
    case equip_shipin: return itempart_shipin;
    default: return -1;
    }
}

bool KItemList::fits(int detail, int part) noexcept
{
    if (detail == equip_ring) return part == itempart_ring1 || part == itempart_ring2;
    const int p = equip_place(detail);
    return p >= 0 && p == part;
}

bool KItemList::can_equip(const KItem& item, int part, const std::function<int(int)>& attrib) const
{
    if (item.genre != KItemGenre::equip || !fits(item.detail, part)) return false;
    // KItemList::EnoughAttrib: strength, dexterity, vitality, energy, level - at least; series,
    // sex, faction - exactly
    for (const auto& r : item.require) {
        if (r.empty()) continue;
        const int have = attrib ? attrib(r.type) : 0;
        switch (r.type) {
        case magic_requirestr:
        case magic_requiredex:
        case magic_requirevit:
        case magic_requireeng:
        case magic_requirelevel:
            if (have < r.value[0]) return false;
            break;
        case magic_requireseries:
        case magic_requiresex:
        case magic_requiremenpai:
            if (have != r.value[0]) return false;
            break;
        default: break;
        }
    }
    return true;
}

bool KItemList::equip(std::uint32_t id, int part, const std::function<int(int)>& attrib)
{
    const auto it = items_.find(id);
    if (it == items_.end() || part < 0 || part >= itempart_num) return false;
    if (!can_equip(it->second.item, part, attrib)) return false;
    return wear(id, part);
}

bool KItemList::wear(std::uint32_t id, int part)
{
    const auto it = items_.find(id);
    if (it == items_.end() || part < 0 || part >= itempart_num) return false;
    Entry& e = it->second;
    if (e.place.room == room_body) return false;
    if (e.item.genre != KItemGenre::equip || !fits(e.item.detail, part)) return false;
    // what is worn there goes to the bag first - and must fit, or nothing happens
    const std::uint32_t worn = equip_[static_cast<std::size_t>(part)];
    const KItemPlace from = e.place;
    if (!take_off_cells(e)) return false;
    if (worn != 0) {
        Entry& w = items_.at(worn);
        int x = 0, y = 0;
        if (!rooms_[room_equipment].find_room(w.item.width(), w.item.height(), x, y)) {
            rooms_[static_cast<std::size_t>(from.room)].place(from.x, from.y, id, e.item.width(), e.item.height());
            return false;
        }
        equip_[static_cast<std::size_t>(part)] = 0;
        rooms_[room_equipment].place(x, y, worn, w.item.width(), w.item.height());
        w.place = KItemPlace{room_equipment, x, y};
    }
    equip_[static_cast<std::size_t>(part)] = id;
    e.place = KItemPlace{room_body, part, 0};
    return true;
}

bool KItemList::unequip(int part, int to_room)
{
    if (part < 0 || part >= itempart_num || to_room < 0 || to_room >= room_num) return false;
    const std::uint32_t id = equip_[static_cast<std::size_t>(part)];
    if (id == 0) return false;
    Entry& e = items_.at(id);
    int x = 0, y = 0;
    if (!rooms_[static_cast<std::size_t>(to_room)].find_room(e.item.width(), e.item.height(), x, y)) return false;
    equip_[static_cast<std::size_t>(part)] = 0;
    rooms_[static_cast<std::size_t>(to_room)].place(x, y, id, e.item.width(), e.item.height());
    e.place = KItemPlace{to_room, x, y};
    return true;
}

// ms_ActivedEquip of KItemList.cpp, checked against jx_linux_y 0x082E7460: the two worn parts
// that can wake the suffixes of each part (the horse and the JX2 parts point at themselves)
static constexpr int kActivedEquip[itempart_horse][2] = {
    {itempart_body, itempart_amulet},    // head
    {itempart_ring2, itempart_belt},     // body
    {itempart_pendant, itempart_cuff},   // belt
    {itempart_amulet, itempart_body},    // weapon
    {itempart_weapon, itempart_head},    // foot
    {itempart_foot, itempart_ring1},     // cuff
    {itempart_belt, itempart_ring2},     // amulet
    {itempart_weapon, itempart_head},    // ring1
    {itempart_cuff, itempart_pendant},   // ring2
    {itempart_foot, itempart_ring1},     // pendant
};

int KItemList::equip_enhance(int part, int player_series) const
{
    if (part < 0 || part >= itempart_num) return 0;
    if (part >= itempart_horse) return 3;
    const KItem* piece = find(equipped(part));
    if (piece == nullptr) return 0;
    int n = g_IsAccrue(player_series, piece->series) ? 1 : 0;
    for (const int other : kActivedEquip[part]) {
        if (const KItem* worn = find(equipped(other))) {
            if (g_IsAccrue(worn->series, piece->series)) ++n;
        }
    }
    return n;
}

// KItemList::GetWeaponDamage of the JX2 server (jx_linux_y 0x081F9310): the weapon's base min
// (m_aryBaseAttrib[0]) and max ([1]) plus its weapondamagemin_v / weapondamagemax_v magic, both
// x (100 + weapondamageenhance_p) / 100; bare hands hit m_nCurStrength / 5 + 1.
std::pair<int, int> KItemList::weapon_damage(int cur_strength) const
{
    const KItem* w = find(equip_[itempart_weapon]);
    if (w == nullptr) {
        const int bare = cur_strength / 5 + 1;
        return {bare, bare};
    }
    int lo = 0, hi = 0, enhance = 0;
    for (const auto& a : w->base) {
        if (a.type == magic_weapondamagemin_v) lo = a.value[0];
        if (a.type == magic_weapondamagemax_v) hi = a.value[0];
    }
    for (const auto& a : w->magic) {
        if (a.type == magic_weapondamagemin_v) lo += a.value[0];
        else if (a.type == magic_weapondamagemax_v) hi += a.value[0];
        else if (a.type == magic_weapondamageenhance_p) enhance += a.value[0];
    }
    return {lo * (100 + enhance) / 100, hi * (100 + enhance) / 100};
}

int KItemList::weapon_type() const
{
    const KItem* w = find(equip_[itempart_weapon]);
    return w == nullptr ? -1 : w->detail;
}

int KItemList::weapon_particular() const
{
    const KItem* w = find(equip_[itempart_weapon]);
    return w == nullptr ? -1 : w->particular;
}

int KItemList::armor_defense() const
{
    int total = 0;
    for (const std::uint32_t id : equip_) {
        const KItem* it = id ? find(id) : nullptr;
        if (it == nullptr) continue;
        for (const auto& a : it->base) {
            if (a.type == magic_armordefense_v) total += a.value[0];
        }
    }
    return total;
}

bool KItemList::add_money(int room, int m) noexcept
{
    if (room < 0 || room >= room_num || m < 0) return false;
    rooms_[static_cast<std::size_t>(room)].set_money(rooms_[static_cast<std::size_t>(room)].money() + m);
    return true;
}

bool KItemList::cost_money(int m) noexcept
{
    if (m < 0 || rooms_[room_equipment].money() < m) return false;
    rooms_[room_equipment].set_money(rooms_[room_equipment].money() - m);
    return true;
}

// ---- KItemGenerator -----------------------------------------------------------------------

void KItemGenerator::set_attrib_cbr(KItem& item, const KItemTemplate& t)
{
    item.genre = t.genre;
    item.detail = t.detail;
    item.particular = t.particular;
    item.level = t.level;
    item.series = t.series;
    item.tpl = &t;
    item.version = version_;
    // KItem::SetAttrib_Base: every base attribute rolled once between min and max
    std::size_t i = 0;
    for (const auto& b : t.basics) {
        if (i >= item.base.size() || b.type <= 0) continue;
        item.base[i].type = b.type;
        item.base[i].value = {random_between(b.min, b.max), 0, 0};
        if (b.type == magic_durability_v) item.durability = item.base[i].value[0];
        ++i;
    }
    if (item.durability == 0) item.durability = -1;
    i = 0;
    for (const auto& r : t.reqs) {
        if (i >= item.require.size() || r.type <= 0) continue;
        item.require[i++] = r;
    }
}

// KItem::SetAttrib_MA (jx_linux_y 0x08065710): the six magic attributes; indestructible_b among
// them makes the piece never wear
void KItemGenerator::set_attrib_ma(KItem& item, const std::array<KMagicAttrib, 6>& magic)
{
    item.magic = magic;
    for (const auto& a : magic) {
        if (a.type == magic_indestructible_b) item.durability = -1;
    }
}

std::optional<KItem> KItemGenerator::equipment(int detail, int particular, int series, int level, const KMagicLevels* magic_levels, int luck)
{
    const KItemTemplate* t = set_.equipment(detail, particular, level);
    if (t == nullptr) return std::nullopt;
    // a mask never carries prefixes / suffixes (nDetailType != 11 in the JX2 server)
    const bool with_magic = magic_levels != nullptr && detail != equip_mask;
    for (int attempt = 1;; ++attempt) {
        KItem item;
        set_attrib_cbr(item, *t);
        if (detail == equip_mask) item.level = level > 0 ? level : 0;
        else item.series = series;
        if (!with_magic) return item;
        std::array<KMagicAttrib, 6> magic{};
        if (!gen_magic_attrib(detail, *magic_levels, series, luck, magic)) return std::nullopt;
        if (check_new_item_attrib(magic)) {
            set_attrib_ma(item, magic);
            return item;
        }
        random(100);   // the old code stirs the seed before rolling again
        if (attempt >= kGenEquipmentTries) return std::nullopt;
    }
}

bool KItemGenerator::gen_magic_attrib(int detail, const KMagicLevels& levels, int series, int luck, std::array<KMagicAttrib, 6>& out)
{
    out = {};
    const std::vector<KMagicTemplate>& rows = set_.magic();
    std::array<const KMagicTemplate*, 6> chosen{};   // pMagicAttrTable: what this piece has already
    std::vector<int> selected;                       // KBPT_ClassMAIT SelectedMagicTable
    if (luck < 0) luck = 0;
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (levels[i] == 0) break;
        const int pos = 1 - static_cast<int>(i & 1);   // even slots are prefixes (1), odd ones suffixes (0)
        const std::vector<int>* candidates = set_.magic_candidates(pos, detail, series, levels[i]);
        if (candidates == nullptr) break;             // "[GenMagicAttrib] GetCMIT Error"
        int decide;
        if (version_ > 3) decide = static_cast<int>(static_cast<std::int64_t>(random(1000000)) * 100 / (10 * luck + 100));
        else if (version_ <= 1) decide = random(100) / (luck / 10 + 1);
        else decide = random(1000000) / (luck / 10 + 1);
        selected.clear();
        for (const int idx : *candidates) {
            if (idx < 0 || static_cast<std::size_t>(idx) >= rows.size()) continue;
            const KMagicTemplate& m = rows[static_cast<std::size_t>(idx)];
            bool used = false;   // m_nUseFlag: a row goes on a piece once
            for (std::size_t k = 0; k < i; ++k) {
                if (chosen[k] == &m || (chosen[k] != nullptr && chosen[k]->kind == m.kind)) used = true;
            }
            if (used) continue;
            const int rate = static_cast<std::size_t>(detail) < m.drop_rates.size() ? m.drop_rates[static_cast<std::size_t>(detail)] : 0;
            if (rate <= decide) continue;
            selected.push_back(idx);
        }
        if (selected.empty()) break;
        const KMagicTemplate& m = rows[static_cast<std::size_t>(selected[static_cast<std::size_t>(random(static_cast<int>(selected.size())))])];
        chosen[i] = &m;
        out[i].type = m.kind;
        for (std::size_t k = 0; k < 3; ++k) out[i].value[k] = m.ranges[k].first + random(m.ranges[k].second - m.ranges[k].first + 1);
    }
    return true;
}

bool KItemGenerator::check_new_item_attrib(const std::array<KMagicAttrib, 6>& magic) const
{
    if (set_.magic_limit_count() == 0) return true;
    for (const auto& a : magic) {
        if (a.type <= 0) continue;
        const KMagicLimit* lim = set_.magic_limit(a.type);
        if (lim == nullptr) continue;
        for (std::size_t k = 0; k < 3; ++k) {
            if (lim->max[k] == -1) continue;
            if (a.value[k] <= lim->min[k] || lim->max[k] <= a.value[k]) return false;   // "CheckNewItemMagicAttrib: ... Found MagicAttribData"
        }
    }
    return true;
}

std::optional<KItem> KItemGenerator::medicine(int detail, int level)
{
    const KItemTemplate* t = set_.medicine(detail, level);
    if (t == nullptr) return std::nullopt;
    KItem item;
    item.genre = KItemGenre::medicine;
    item.detail = t->detail;
    item.particular = t->particular;
    item.level = t->level;
    item.tpl = t;
    item.version = version_;
    // KBPT_Medicine of the Linux server reads two attributes (columns 14..19; jx_linux_y
    // 0x081ED430), whatever the file holds beyond them
    std::size_t i = 0;
    for (const auto& a : t->med_attribs) {
        if (i < 2) item.base[i++] = a;
    }
    return item;
}

std::optional<KItem> KItemGenerator::quest(int detail, int count)
{
    const KItemTemplate* t = set_.quest(detail);
    if (t == nullptr) return std::nullopt;
    KItem item;
    item.genre = KItemGenre::task;
    item.detail = t->detail;
    item.particular = t->particular;
    item.level = 1;
    item.tpl = t;
    item.version = version_;
    if (count > 0 && t->max_stack > 0) item.count = std::min(count, t->max_stack);
    return item;
}

std::optional<KItem> KItemGenerator::town_portal()
{
    const KItemTemplate* t = set_.town_portal();
    if (t == nullptr) return std::nullopt;
    KItem item;
    item.genre = KItemGenre::town_portal;
    item.tpl = t;
    item.version = version_;
    return item;
}

std::optional<KItem> KItemGenerator::magic_script(int detail, int particular, int level, int series, int count)
{
    const KItemTemplate* t = set_.magic_script(detail, particular);
    if (t == nullptr) return std::nullopt;
    KItem item;
    item.genre = KItemGenre::magic_script;
    item.detail = t->detail;
    item.particular = t->particular;
    item.level = level > 0 ? level : t->level;
    item.series = series >= 0 && series < 5 ? series : t->series;
    item.tpl = t;
    item.version = version_;
    item.gen_param = t->row;
    if (count > 0 && t->max_stack > 0) item.count = std::min(count, t->max_stack);
    if (t->skill > 0) item.group = t->skill;
    return item;
}

// KItemGenerator::Gen_GoldEquip (server side): the six magic attributes of the piece are rolled
// from their gold_magic rows; luck (0..200) pushes the roll towards the top of the range.  (The
// pieces the drop tables name by quality 1 come here too: KItemSet::Add -> 0x0806A150.)
std::optional<KItem> KItemGenerator::gold(int luck, int row_id)
{
    const KItemTemplate* t = set_.gold(row_id);
    if (t == nullptr) return std::nullopt;
    KItem item;
    set_attrib_cbr(item, *t);
    item.ex_type = 1;
    item.gen_param = row_id;
    if (t->group >= 0) item.group = t->group;
    if (t->ex_group >= 0) item.ex_group = t->ex_group;
    if (t->group_serial >= 0) item.group_serial = t->group_serial;
    if (luck < 0) luck = 0;
    const auto roll = [&](const KMagicTemplate& m) {
        KMagicAttrib a;
        a.type = m.kind;
        a.value[1] = -1;
        a.value[2] = m.ranges[2].first;
        const int dist = m.ranges[0].second - m.ranges[0].first;
        int calc = luck;
        if (dist <= 0) {
            a.value[0] = m.ranges[0].first;
        } else if (calc >= 200) {
            a.value[0] = m.ranges[0].second;
        } else {
            if (calc > 100) calc = 100;
            int mid = dist / 2 + 1;
            if (dist < 10) {
                if (random(100 - calc) < 20) mid = random_between(mid * 10, dist * 10 + 5) / 10;
                else mid = random(mid * 10) / 10;
                a.value[0] = m.ranges[0].first + mid;
            } else if (random(100 - calc) < 20) {
                a.value[0] = m.ranges[0].first + random_between(mid, dist);
            } else {
                a.value[0] = m.ranges[0].first + random(mid);
            }
        }
        return a;
    };
    std::size_t n = 0;
    for (std::size_t i = 0; i < t->magic_ids.size() && i < 6; ++i) {
        const KMagicTemplate* m = set_.gold_magic(t->magic_ids[i]);
        if (m == nullptr) continue;
        item.magic[n++] = roll(*m);
    }
    return item;
}

} // namespace jx::zone
