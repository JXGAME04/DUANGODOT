#pragma once

// The "gold" (elite) monsters of the JX2 server - a feature the 2004 source does not have.  jx_linux_y keeps a
// KNpcGoldTemplate table (\settings\npc\NpcGoldTemplate.txt, 30 rows of 0xac bytes at 0x8BACAC0, the count at
// +0x1428, KNpcGoldTemplate::Init 0x0809CCC0) and a KNpcGold at KNpc+0x88 (0x12c bytes; the ctor 0x0809D4C0,
// BackData 0x0809D560, SetGoldTypeAndBackData 0x0809D8D0, RecoverBackData 0x0809E070, GetGoldKind 0x0809CC90).
// A placed monster is backed up as a candidate (BackData) when its map has `<id>_AutoGoldenNpc` or the
// placement is a KSPNpc.bSpecialNpc; every revive rolls the map's chance in a million (0x08085E70, 2 000 000 =
// always when the map has none) and turns it gold for one life: its numbers multiplied by a row of the table,
// its ai swapped, the row's skill in cell 5 as an aura, the map's `<id>_GoldenDropRate`; the end of its death
// frames puts everything back (0x08083720 -> RecoverBackData).  Lua adds always-gold ones (NPCINFO_AddBlueNpc
// 0x081C25F0, AddNpc with an 8th argument of 2).  docs/LINUX-SERVER.md §16.12

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "jx/zone/KMagicAttrib.h"

namespace jx::zone {

struct KNpc;

// one row of NpcGoldTemplate.txt (npc_gold.json "rows"; jxassets export-npc-gold reads the columns like the loader)
struct KNpcGoldTemplate {
    std::string name;             // 类型: what the row is called (the loaders only check it is not empty)
    int exp = 1;                  // +0x00 percent of m_Experience (+0x15a8)
    int life = 1;                 // +0x04 percent of both life maximums (+0x1a14 / +0x1a18)
    int life_replenish = 1;       // +0x08 percent of +0x1190
    int attack_rating = 1;        // +0x0c percent of +0x1258
    int defense = 1;              // +0x10 percent of +0x125c
    int min_damage = 1;           // +0x14 percent of the minimum of every damage block (+0x11bc, +0x120c, +0x11dc, +0x121c ...)
    int max_damage = 1;           // +0x18 percent of the maximum of every damage block, and of the poison damages
    int treasure = 0;             // +0x1c replaces m_CurrentTreasure (+0x1378)
    int walk_speed = 0;           // +0x20 added to +0x1288
    int run_speed = 0;            // +0x24 added to +0x128c
    int attack_speed = 0;         // +0x28 added to +0x1a34 and +0x1a38
    int cast_speed = 0;           // +0x2c added to +0x1a3c and +0x1a40
    int skill_id = 0;             // +0x30 the SkillId whose SkillName the row names (0x080A1D80); 0 = none
    std::string skill_level;      // +0x34 the level cell of that skill: GetNpcLevelData(series, level, "Level5", cell) (0x080A1F90)
    int fire_resist = 0;          // +0x54 percent of the current fire resist (+0x19ec) - applied TWICE (0x0809D969 .. 0x0809D998)
    int fire_resist_max = 0;      // +0x58 percent of the fire resist maximum (+0x126c)
    int cold_resist = 0;          // +0x5c (twice) / +0x60
    int cold_resist_max = 0;
    int light_resist = 0;         // +0x64 (twice) / +0x68
    int light_resist_max = 0;
    int poison_resist = 0;        // +0x6c (twice) / +0x70
    int poison_resist_max = 0;
    int physics_resist = 0;       // +0x74 (twice) / +0x78
    int physics_resist_max = 0;
    int ai_mode = 0;              // +0x7c replaces m_AiMode (+0x1654)
    std::array<int, 10> ai_params{};   // +0x80 .. +0xa4 replace m_AiParam[0..9] (+0x1658 ..)
    int ai_max_time = 100;        // +0xa8 replaces m_AIMAXTime (the byte +0x14c0)
};

// the loaded table: rows in file order (gold type = the 0-based index; the 0x4c / 0x9a packets carry index + 1)
class KNpcGoldTemplateSet {
public:
    static std::optional<KNpcGoldTemplateSet> load(const std::filesystem::path& file, std::string* error);
    static std::optional<KNpcGoldTemplateSet> parse(const std::string& json, std::string* error);

    [[nodiscard]] int count() const noexcept { return static_cast<int>(rows.size()); }   // [0x8BADEE8]
    [[nodiscard]] const KNpcGoldTemplate* row(int gold_type) const noexcept
    {
        return gold_type >= 0 && gold_type < count() ? &rows[static_cast<std::size_t>(gold_type)] : nullptr;
    }

    std::vector<KNpcGoldTemplate> rows;
    // how many rows the 2.0 client's own copy has (gamecl.exe 0x006E35C0 -> [0x21a12c0]): its name colour treats a
    // kind above that count as a boss (0x005F2401) - the zone tells the client so it need not carry the table
    int client_rows = 0;
};

// KNpcGold (KNpc+0x88): the candidate flag, the gold state and the backup the recover puts back
struct KNpcGold {
    bool is_gold = false;      // +0x04 a candidate (BackData ran)
    bool is_golding = false;   // +0x08 gold right now
    int gold_type = 0;         // +0x0c the row (0-based)
    // the backup BackData 0x0809D560 takes (+0x18 ..): the resists in use (max of the plain value and its yan twin),
    // their maximums, the drop table, the ai, the ten damage blocks +0x11b8 .. +0x1254, the experience, the life
    // maximum in use, the replenish, the attack rating, the defence and the treasure
    int fire_resist = 0, fire_resist_max = 0;        // +0x18 / +0x1c
    int cold_resist = 0, cold_resist_max = 0;        // +0x20 / +0x24
    int light_resist = 0, light_resist_max = 0;      // +0x28 / +0x2c
    int poison_resist = 0, poison_resist_max = 0;    // +0x30 / +0x34
    int physics_resist = 0, physics_resist_max = 0;  // +0x38 / +0x3c
    std::string drop_rate_file;                      // +0x40 (+0x174c: the drop table's index there, its path here)
    int ai_mode = 0;                                 // +0x44
    std::array<int, 10> ai_params{};                 // +0x48 .. +0x6c
    int ai_max_time = 0;                             // +0x70
    std::array<KMagicAttrib, 10> damage{};           // +0x74 .. +0x110: physics, fire, cold, light, poison damage; physics, cold, light, fire, poison magic
    int experience = 0;                              // +0x114
    int life_max = 0;                                // +0x118
    int life_replenish = 0;                          // +0x11c
    int attack_rating = 0;                           // +0x120
    int defend = 0;                                  // +0x124
    int treasure = 0;                                // +0x128

    // 0x0809CC90 KNpcGold::GetGoldKind: the kind the packets carry - the row + 1 while gold, else 0
    [[nodiscard]] int gold_kind() const noexcept { return is_gold && is_golding ? gold_type + 1 : 0; }
};

// 0x0809D560 KNpcGold::BackData: is_gold = 1, is_golding = 0, gold_type = 0 and the backup of the numbers above
void gold_back_data(KNpc& e);
// the number part of SetGoldTypeAndBackData 0x0809D8D0 (0x0809D93F .. 0x0809DF4B), once the type is chosen and
// the skill is in: every field multiplied, added or replaced by the row, the life refilled
void gold_apply(KNpc& e, const KNpcGoldTemplate& t);
// 0x0809E070 KNpcGold::RecoverBackData without the skill list part: is_golding = 0, the resists, the drop table
// and the ai back; when the type is a row of the table (`t`) the numbers too, the speeds minus the row's, the life
// refilled with the backed-up maximum.  Returns false when there was nothing to recover (not gold, not golding).
bool gold_recover(KNpc& e, const KNpcGoldTemplate* t);
// 0x08079750: 0 a player, 3 a boss (+0x181c != 0), 2 a gold monster (GetGoldKind >= 1), 1 anything else
[[nodiscard]] int npc_class(const KNpc& e) noexcept;

} // namespace jx::zone
