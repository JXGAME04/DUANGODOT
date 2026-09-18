// The objects that lie on the ground (KObj / KObjSet of the old core, the server side): a dropped
// item, a pile of money.  The kinds and their pictures come from \settings\obj\ObjData.txt, the
// money pictures from MoneyObj.txt, both exported by `jxassets export-objdata` into objdata.json.
//
// The rules, from KObj.cpp (Activate, SetItemBelong) and KPlayer::ServerPickUpItem, checked
// against jx_linux_y (SetItemBelong 0x080A4D90: 600 frames; ServerPickUpItem: distance^2 40000):
//   an object lives LifeTime frames (ObjData, 2400 for items) and is then removed;
//   an object dropped for a player belongs to that player for OBJ_BELONG_TIME = 600 frames, then
//   anybody may pick it up; the player has to stand within 200 units (40000 squared).
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace jx::zone {

constexpr int kObjBelongTime = 600;             // OBJ_BELONG_TIME (KObj.h)
constexpr std::int64_t kPickUpDistance2 = 40000;   // PLAYER_PICKUP_SERVER_DISTANCE (GameDataDef.h)

enum class KObjKind : std::uint8_t { other = 0, item = 1, money = 2 };

// One row of ObjData.txt
struct KObjTemplate {
    int id = 0;
    std::string name;
    KObjKind kind = KObjKind::other;
    std::string kind_name;   // as the table spells it
    int life_time = 0;       // frames; 0 = for ever
    int height = 0;
    std::string image;       // sprite id under client/assets/sprites ("" = the client has no picture)
    std::string drop_image;
};

// ObjData.txt + MoneyObj.txt
class KObjDataSet {
public:
    bool load(const std::string& path, std::string* error);
    [[nodiscard]] const KObjTemplate* find(int id) const;
    [[nodiscard]] std::size_t size() const noexcept { return objects_.size(); }
    // KObjSet::AddMoneyObj: the first MoneyObj row whose cap holds the amount
    [[nodiscard]] int money_obj(int amount) const;

private:
    std::map<int, KObjTemplate> objects_;
    std::vector<std::pair<int, int>> money_;   // (max, obj id), ascending
};

// The server side of a dropped object (KObj members the zone keeps)
struct KGroundObject {
    KObjKind kind = KObjKind::item;
    int obj_id = 0;              // ObjData row
    int money = 0;               // Obj_Kind_Money: the amount
    std::uint64_t belong = 0;    // player id it is kept for (0 = anybody), KObj::m_nBelong
    int belong_ticks = 0;        // KObj::m_nBelongTime
    int life_ticks = 0;          // KObj::m_nLifeTime (<= 0 with a LifeTime of 0: never removed)
    bool forever = false;
};

} // namespace jx::zone
