// KSubWorldChat.cpp - a line spoken on a channel (docs/LINUX-SERVER.md §19).
//
// jx_linux_y 0x081E3710 (a channel line the relay handed over): the speaker is a player of this server, not forbidden
// (Player+0x38c), the sentence is at most 0x95 bytes, the channel is one the relay announced and its cost type is what
// the client claimed, the cost is paid (0x080502A0), then the line goes back to the relay with the target class of the
// channel's letter: 'T' the team (Player+0x5998; no team -> dropped), 'S' the speaker (the people who see it), 'B'
// everybody.  The zone delivers the same sets itself; the relay-only classes (WORLD, faction, tong) are delivered by
// KGameServer over every map of the zone.
#include <algorithm>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KSubWorld.h"

namespace jx::zone {

bool KSubWorld::chat(std::uint64_t sid, std::string_view text, pb::ChatChannel channel, std::string_view target)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    if (cfg_.gm_chat && gm_command(sid, text)) return true;   // TextGMFilter runs before the text is spoken
    KNpc& e = entities_.at(pit->second);
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    if (e.player.forbid_talk) {   // 0x081E387A: ForbitTalk
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("reason", "forbidden")});
        return false;
    }
    if (text.size() > kChatSentenceMax) {   // 0x081E38AC
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("reason", "too long"),
                                                 log::kv("len", text.size())});
        return false;
    }
    if (channel == pb::CH_SYSTEM || channel == pb::CH_TONG) {   // the server's own lines; no tongs yet
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("reason", "no such channel")});
        return false;
    }
    // the receivers first: a team line without a team costs nothing (0x081E3A4B drops it before the relay hears of it)
    std::vector<std::uint64_t> sids;
    switch (channel) {
    case pb::CH_TEAM: {
        const KTeam* t = team_of(e);
        if (t == nullptr) {
            log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("reason", "no team")});
            return false;
        }
        sids = t->people();
        break;
    }
    case pb::CH_WHISPER: {
        const KNpc* to = nullptr;
        for (const auto& [osid, oid] : players_) {
            const KNpc* o = entities_.find(oid);
            if (o != nullptr && o->name == target) {
                to = o;
                break;
            }
        }
        if (to == nullptr) {   // the 2003 chat_feedback: nobody of that name here
            log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("reason", "no such player"),
                                                     log::kv("target", target)});
            return false;
        }
        sids = {to->sid};
        if (to->sid != sid) sids.push_back(sid);   // the speaker sees its own line
        break;
    }
    case pb::CH_NEARBY:
        sids = e.watchers;   // whoever SEES the speaker - the speaker's own client included
        break;
    default:
        break;   // WORLD / CITY / FACTION: the server spreads it
    }
    if (!chat_pay(e, chat_cost_type(channel))) return false;
    pb::ChatMsg msg;
    msg.set_entity_id(e.id.value);
    msg.set_name(e.name);
    msg.set_text(std::string(text));
    msg.set_channel(channel);
    if (channel == pb::CH_WORLD || channel == pb::CH_CITY || channel == pb::CH_FACTION) {
        KChatBroadcast b;
        b.channel = channel;
        b.faction = channel == pb::CH_FACTION ? e.player.faction.current : -1;
        msg.SerializeToString(&b.payload);
        chat_broadcasts_.push_back(std::move(b));
        log::debug("zone.chat", "chat", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("len", text.size()),
                                         log::kv("receivers", "zone")});
        return true;
    }
    emit(sids, static_cast<std::uint16_t>(pb::G2C_CHAT_MSG), msg);
    log::debug("zone.chat", "chat", {log::kv("entity", e.id), log::kv("channel", static_cast<int>(channel)), log::kv("len", text.size()),
                                     log::kv("receivers", sids.size())});
    return true;
}

std::vector<KChatBroadcast> KSubWorld::take_chat_broadcasts()
{
    std::vector<KChatBroadcast> out;
    out.swap(chat_broadcasts_);
    return out;
}

// 0x080502A0(server, player, type): SetChatFlag (Player+0x394 bit 0) refuses everything; type 0 is free; type 1 pays the
// global 0x8badf14 (never written: 0) and passes; 2..4 need the row's Level, its Money in the bag and ManaPercent of the
// mana maximum in hand, then take the money (the money log "C_Chat") and the mana; above 4 nothing may be said.  The
// CheckTimeCount / ConsumeTimeCount hooks of \script\global\chat_timecount_limit.lua (types 2 and 4, 0x080ACF90 /
// 0x080ACF40) are not called: the script is not in the tree.  StaminaPercent is loaded and never charged.
bool KSubWorld::chat_pay(KNpc& e, int type)
{
    if (e.player.chat_flag) {   // 0x080502DD
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("reason", "chat flag")});
        return false;
    }
    if (type <= 1) return true;   // 0x080502F7 / 0x08050470: free, or the unset global money
    if (type > KChatCostTable::kTypes - 1) return false;   // 0x08050310
    const KChatCostRow row = cfg_.chat_cost ? cfg_.chat_cost->rows[static_cast<std::size_t>(type)] : KChatCostRow{};
    if (static_cast<int>(e.level) < row.level) {   // 0x08050330
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("type", type), log::kv("reason", "level"), log::kv("level", e.level),
                                                 log::kv("need", row.level)});
        return false;
    }
    KItemList* list = items_of(e.sid);
    const int money = list != nullptr ? list->money(room_equipment) : 0;
    if (row.money > money) {   // 0x08050345
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("type", type), log::kv("reason", "money"), log::kv("money", money),
                                                 log::kv("need", row.money)});
        return false;
    }
    const int mana_cost = row.mana_percent * e.mana_max() / 100;   // 0x08050351: imul, / 100
    if (mana_cost > e.mana()) {   // 0x08050375
        log::debug("zone.chat", "chat refused", {log::kv("entity", e.id), log::kv("type", type), log::kv("reason", "mana"), log::kv("mana", e.mana()),
                                                 log::kv("need", mana_cost)});
        return false;
    }
    if (row.money > 0 && list != nullptr && list->cost_money(row.money)) {   // 0x080503A5 KPlayer::Pay + the money log
        send_money(e.sid);
        log::info("zone.money", "script money", {log::kv("entity", e.id), log::kv("reason", "C_Chat"), log::kv("amount", -row.money),
                                                 log::kv("money", list->money(room_equipment))});
    }
    if (mana_cost > 0) {
        e.cur.mana -= mana_cost;   // 0x080503E4
        send_player_attrib(e.sid);
    }
    log::debug("zone.chat", "chat cost", {log::kv("entity", e.id), log::kv("type", type), log::kv("money", row.money), log::kv("mana", mana_cost)});
    return true;
}

}   // namespace jx::zone
