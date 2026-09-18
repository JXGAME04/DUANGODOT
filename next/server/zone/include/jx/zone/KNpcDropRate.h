// The drop table of an npc (KNpcTemplate::m_pItemDropRate; KItemDropRate of the JX2 server),
// as `jxassets export-npcres` writes it into npcs.json "droprates" from the file the DropRateFile
// column names.  Defaults are the loader's (jx_linux_y 0x080A3B80), the roll is GenRandomItem's
// (0x08083BB0) - see KSubWorld::gen_random_item and docs/LINUX-SERVER.md §9.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace jx::zone {

struct KDropEntry {
    int genre = 0;
    int quality = 0;      // 0 white, 1 gold, 2 platina (sockets), ...: KItemSet::Add's second argument
    int detail = 0;
    int particular = 0;
    int rate = 0;         // RandRate: its share of RandRange
    int min_level = -1;   // -1 = from the npc level through the [Main] scales
    int max_level = -1;
    int series = -1;      // -1 = the table's, and that -1 = the npc's own
    int enchasable_rate = -1;
    int min_socket = -1;
    int max_socket = -1;
    std::array<int, 6> magic_level{};   // MagicLevel1..6: fixed prefix / suffix levels (0 = rolled)
};

struct KNpcDropRate {
    std::string source;
    int count = 0;
    int rand_range = 0;
    int magic_rate = 0;
    int money_rate = 20;          // percent of the rolls that are money
    int money_scale = 50;         // money = experience * scale / 100 (KNpc::LoseMoney)
    int min_level_scale = 20;     // the item level range from the npc level: (level-1)/scale + 1
    int max_level_scale = 10;
    int min_level = 1;
    int max_level = 10;
    int series = -1;
    int enchasable_rate = 0;
    int min_socket = 1;
    int max_socket = 1;
    int team_share = 0;
    int team_share_rate = 0;
    std::vector<KDropEntry> entries;
};

} // namespace jx::zone
