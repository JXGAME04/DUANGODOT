// KPlayerChat.h - the chat of a player: the channels, the cost of speaking in one and who hears it.
//
// The JX2 server (jx_linux_y) leaves the channels to the relay (s3relay_y: relay_channcfg.ini names the classes
// TEAM 'T', \F faction 'F', \O tong 'O', NEARBY 'S', CITY 'B', \U union 'U' and relay_channel.ini adds [WORLD] cost 4)
// and only checks a line before the relay spreads it (0x081E3710): the player may not be forbidden (Player+0x38c, Lua
// ForbitTalk), the sentence is at most 0x95 bytes (0x081E38AC), the channel must be one the relay announced with the
// cost type the client claims (0x081E390A) and the cost is paid (0x080502A0 over \settings\npc\player\chatcost.ini:
// type 0 free, 1 money, 2 city, 3 tong / faction, 4 world - Level, Money, ManaPercent of the type); then the line goes
// back to the relay with a target class ('T' the team id, 'S' the speaker, 'B' broadcast).  The zone keeps the same
// rules and delivers itself (docs/LINUX-SERVER.md §19).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "jx/client.pb.h"

namespace jx::zone {

inline constexpr std::size_t kChatSentenceMax = 0x95;   // 0x081E38AC: a longer sentence is dropped

// one [type] of chatcost.ini (KNpcSet 0x8BACAC0 + 0x1440 + type * 16: Level +0, Money +4, ManaPercent +8, StaminaPercent +0xc)
struct KChatCostRow {
    int level = 0;
    int money = 0;
    int mana_percent = 0;
    int stamina_percent = 0;   // read by the loader 0x080A0D49, used by nothing
};

// \settings\npc\player\chatcost.ini (jxassets export-chat-cost -> chat_cost.json): the five cost types
struct KChatCostTable {
    static constexpr int kTypes = 5;
    std::array<KChatCostRow, kTypes> rows{};
    static std::optional<KChatCostTable> load(const std::string& file, std::string* error);
};

// the cost type of a channel: relay_channcfg.ini ([team] cost 0, [faction] 3, [tong] 0, [screen] 0, [broadcast] CITY 2)
// and relay_channel.ini ([WORLD] cost 4); a whisper is the relay's [system] defCost 0
[[nodiscard]] int chat_cost_type(pb::ChatChannel channel) noexcept;

// a line for people beyond this map (WORLD / CITY / FACTION): the map hands it to KGameServer, which sends it to every
// session of the zone that qualifies
struct KChatBroadcast {
    std::uint32_t msg_id = 0;   // 0 = G2C_CHAT_MSG; a script action (G2C_SCRIPT_ACTION) for AddGlobalNews - the relay's broadcast
                                // to every server (0x08077560, class 0x21 target -1) is the server's fan-out here
    pb::ChatChannel channel = pb::CH_WORLD;
    int faction = -1;      // CH_FACTION: KPlayerFaction::current of the speaker
    std::string payload;   // the serialized ChatMsg
};

}   // namespace jx::zone
