// The trade between two players and the sign over a player's head, as jx_linux_y runs them (docs/LINUX-SERVER.md §18):
// KPlayerMenuState::SetState 0x080C29D0 / RestoreBackupState 0x080C2ED0, KPlayer::TradeApplyOpen 0x080AE590, the 0x6a
// packet 0x080AE320 (close), 0x6b 0x080B4DE0 (TradeApplyStart), c2sTradeReplyStart 0x080BAFD0, 0x6c 0x080AE510
// (TradeMoveMoney), 0x6d 0x080B2C70 (TradeDecision + the exchange), SyncTradeState 0x080A85B0, the cancel 0x080AE380 /
// 0x080AE4B0 and the trade-box moves of KItemList::ExchangeItem 0x08206110.
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"

namespace jx::zone {

namespace {

// the 0x86 message ids the trade sends (docs/CLIENT-2.0.md §21: MSG_TRADE_SELF_ROOM_FULL 0xb, MSG_TRADE_DEST_ROOM_FULL 0xc,
// MSG_TRADE_REFUSE_APPLY 0xd, 0x2c = "cannot trade" (Player+0x374))
constexpr int kMsgTradeSelfRoomFull = 0xb;
constexpr int kMsgTradeDestRoomFull = 0xc;
constexpr int kMsgTradeRefuseApply = 0xd;
constexpr int kMsgTradeForbidden = 0x2c;

}  // namespace

// ---- the sign over the head (KPlayerMenuState) ---------------------------------------------------------------------

// KPlayerMenuState::SetState 0x080C29D0(menu, player, state, sentence, length, dest npc): a new state keeps the old one
// (and its sentence) as the backup; NORMAL from TEAMOPEN closes an open team; oneself hears s2c_tradechangestate (0 / 1 /
// 2 + the partner), the region s2c_npcsetmenustate (the state and, for TRADEOPEN, the sentence)
void KSubWorld::set_menu_state(KNpc& e, int state, std::string_view sentence, EntityId dest)
{
    if (e.kind != KNpcKind::player || state < menu_state_normal || state >= menu_state_num) return;
    KPlayerMenuState& m = e.player.menu;
    if (m.state != state) {
        m.back_state = m.state;
        m.back_sentence = m.sentence;
        m.state = state;
    }
    m.sentence = sentence.size() > kMenuSentenceMax ? std::string(sentence.substr(0, kMenuSentenceMax)) : std::string(sentence);
    if (state == menu_state_normal && m.back_state == menu_state_team_open) {
        if (KTeam* t = team_of(e); t != nullptr && t->captain == e.sid && t->state != 0) team_set_close(*t);   // 0x080C2A5D..
    }
    emit_menu_state(e, dest);
}

// KPlayerMenuState::RestoreBackupState 0x080C2ED0: back to the state before (a cancelled trade)
void KSubWorld::restore_menu_state(KNpc& e)
{
    if (e.kind != KNpcKind::player) return;
    KPlayerMenuState& m = e.player.menu;
    m.state = m.back_state;
    m.sentence = m.back_sentence;
    emit_menu_state(e, EntityId{});
}

void KSubWorld::emit_menu_state(const KNpc& e, EntityId dest)
{
    const int state = e.player.menu.state;
    if (state == menu_state_normal || state == menu_state_trade_open || state == menu_state_trading) {
        pb::TradeState ts;
        ts.set_state(static_cast<std::uint32_t>(state == menu_state_normal ? 0 : state == menu_state_trade_open ? 1 : 2));
        if (state == menu_state_trading) {
            const KNpc* p = dest.value != 0 ? entities_.find(dest) : nullptr;
            if (p == nullptr) p = find_player(e.player.trade.dest);
            if (p != nullptr) {
                ts.set_partner(p->id.value);
                ts.set_partner_name(p->name);
            }
        }
        emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_STATE), ts);
    }
    pb::EntityMenuState ms;
    ms.set_entity_id(e.id.value);
    ms.set_state(static_cast<std::uint32_t>(state));
    if (state == menu_state_trade_open) ms.set_sentence(e.player.menu.sentence);
    std::vector<std::uint64_t> sids = e.watchers;
    if (std::find(sids.begin(), sids.end(), e.sid) == sids.end()) sids.push_back(e.sid);
    emit(std::move(sids), static_cast<std::uint16_t>(pb::G2C_ENTITY_MENU_STATE), ms);
}

// the 0x86 packet {word 8, word id, dword npc} (0x080A8400): a sentence of the client's string table by id
void KSubWorld::sys_msg(std::uint64_t sid, int id, EntityId who)
{
    pb::SysMsg m;
    m.set_id(static_cast<std::uint32_t>(id));
    if (who.value != 0) {
        m.set_entity_id(who.value);
        if (const KNpc* w = entities_.find(who)) m.set_name(w->name);
    }
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_SYS_MSG), m);
}

// ---- the trade -----------------------------------------------------------------------------------------------------

// KPlayer::CheckTrading 0x080A7E90: the menu state TRADING or m_nIsTrading
bool KSubWorld::trading(const KNpc& e) const noexcept
{
    return e.kind == KNpcKind::player && (e.player.menu.state == menu_state_trading || e.player.trade.trading);
}

KNpc* KSubWorld::trade_partner(const KNpc& e)
{
    if (!e.player.trade.trading) return nullptr;
    KNpc* p = team_player(e.player.trade.dest);
    if (p == nullptr || p->player.trade.dest != e.sid) return nullptr;   // 0x080B2CD3: the partner must point back
    return p;
}

bool KSubWorld::trade_request(std::uint64_t sid, int cmd, EntityId target, int arg, std::string_view text)
{
    KNpc* me = team_player(sid);
    if (me == nullptr) return false;
    switch (cmd) {
    case pb::TRADE_APPLY_OPEN: return trade_apply_open(*me, text);
    case pb::TRADE_APPLY_CLOSE: return trade_apply_close(*me);
    case pb::TRADE_APPLY_START: return trade_apply_start(*me, target);
    case pb::TRADE_REPLY: return trade_reply(*me, target, arg != 0);
    case pb::TRADE_MONEY: return trade_money(*me, arg);
    case pb::TRADE_DECISION: return trade_decision(*me, arg);
    default: return false;
    }
}

// KPlayer::TradeApplyOpen 0x080AE590 {word length, sentence}: alive, not trading, not loading (+0x5f88), the level rule
// 0x080AB930 (nothing when [0x9789f14] is 0), the sentence 0..255 bytes -> the menu state TRADEOPEN with it
bool KSubWorld::trade_apply_open(KNpc& e, std::string_view sentence)
{
    if (!e.alive() || trading(e)) return false;
    set_menu_state(e, menu_state_trade_open, sentence, EntityId{});
    log::debug("zone.trade", "trade open", {log::kv("entity", e.id), log::kv("len", sentence.size())});
    return true;
}

// the 0x6a packet 0x080AE320: not trading -> the menu state NORMAL
bool KSubWorld::trade_apply_close(KNpc& e)
{
    if (trading(e)) return false;
    set_menu_state(e, menu_state_normal, {}, EntityId{});
    return true;
}

// the 0x6b packet 0x080B4DE0 {npc, dword check}: alive, not trading, the world switch [0x830ca40] != 2, the level rule; the
// target in the regions around (FindAroundPlayer 0x080B1610) and in the state TRADEOPEN; both "normal" (0x080A8FC0) and
// neither under the security lock (tbSecurityLock:CheckTrade); the packet check 0x080A79B0 (a client token - not here);
// then my m_nApplyIdx = the target and the 0x8b {player, npc} to it
bool KSubWorld::trade_apply_start(KNpc& e, EntityId target)
{
    if (!e.alive() || trading(e)) return false;
    KNpc* t = find_around_player(e, target);
    if (t == nullptr || t->player.menu.state != menu_state_trade_open) return false;
    if (!t->alive() || trading(*t)) return false;
    e.player.trade.apply = t->sid;   // 0x080B4ED0: the applicant remembers whom it asked
    pb::TradeApply m;
    m.set_entity_id(e.id.value);
    m.set_name(e.name);
    emit({t->sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_APPLY), m);
    log::debug("zone.trade", "trade apply", {log::kv("entity", e.id), log::kv("target", t->id)});
    return true;
}

// c2sTradeReplyStart 0x080BAFD0 {byte reply, dword applicant}: I am not trading and not loading; the applicant is a player
// whose m_nApplyIdx is me (0x080BB041: it asked me); an applicant already trading may only be refused (0x080BB04F..); the
// world switch [0x830ca40] != 2; a trade forbidden on either side (Player+0x374) -> the 0x86 {8, 0x2c}; a refusal or
// different maps -> the 0x86 {8, 0xd} to the applicant; I must still be TRADEOPEN and both normal; the Lua hook
// CheckPlayerTrade of script/global/trade.lua (not here); a partner with an open team closes it, a sitter stands; the
// trade boxes are cleared, KTrade::StartTrade on both, both TRADING
bool KSubWorld::trade_reply(KNpc& e, EntityId applicant, bool accept)
{
    if (trading(e)) return false;
    KNpc* a = entities_.find(applicant);
    if (a == nullptr || a->kind != KNpcKind::player || !a->player.loaded) return false;
    if (a->player.trade.apply != e.sid) return false;   // 0x080BB041
    if (trading(*a) && accept) return false;            // 0x080BB04F..0x080BB066
    if (!accept) {
        sys_msg(a->sid, kMsgTradeRefuseApply, e.id);    // 0x080BB0FA
        a->player.trade.apply = 0;
        return true;
    }
    if (e.player.menu.state != menu_state_trade_open) return false;   // 0x080BB1BA
    if (!e.alive() || !a->alive()) return false;
    a->player.trade.apply = 0;
    if (a->player.menu.state == menu_state_team_open) {   // 0x080BB594: SetTeamState(close)
        if (KTeam* t = team_of(*a); t != nullptr && t->captain == a->sid) team_set_close(*t);
    }
    if (e.doing == KDoing::sit) leave_sit(e);       // 0x080BB562: DoStand
    if (a->doing == KDoing::sit) leave_sit(*a);     // 0x080BB535
    trade_clear_box(e);                              // 0x081FC900 (rooms 2 and 4 back to the bag)
    trade_clear_box(*a);
    e.player.trade.release();
    a->player.trade.release();
    e.player.trade.start(a->sid);
    a->player.trade.start(e.sid);
    set_menu_state(e, menu_state_trading, {}, a->id);
    set_menu_state(*a, menu_state_trading, {}, e.id);
    trade_sync(e);
    log::info("zone.trade", "trade started", {log::kv("entity", e.id), log::kv("partner", a->id)});
    return true;
}

// the 0x6c packet 0x080AE510 {dword money}: trading, not locked, 0 <= money <= the bag's money -> the trade box's money
// (0x081FB020); the partner hears it with the next sync (the 0x77 packet)
bool KSubWorld::trade_money(KNpc& e, int money)
{
    if (!trading(e) || e.player.trade.locked) return false;
    KItemList* list = items_of(e.sid);
    if (list == nullptr || money < 0 || money > list->money(room_equipment)) return false;
    list->set_money(room_trade, money);
    trade_sync(e);
    return true;
}

// KPlayer::SyncTradeState 0x080A85B0: while trading, the partner gets the 0x77 {my trade money} and both get the 0x81
// {self lock, dest lock, self ok, dest ok} from their own side
void KSubWorld::trade_sync(KNpc& e)
{
    if (!trading(e)) return;
    KNpc* p = trade_partner(e);
    if (p == nullptr) return;
    const KItemList* mine = items_of(e.sid);
    const KItemList* theirs = items_of(p->sid);
    const std::uint32_t my_money = mine ? static_cast<std::uint32_t>(std::max(0, mine->money(room_trade))) : 0;
    const std::uint32_t their_money = theirs ? static_cast<std::uint32_t>(std::max(0, theirs->money(room_trade))) : 0;
    pb::TradeSync s;
    s.set_self_lock(e.player.trade.locked);
    s.set_dest_lock(p->player.trade.locked);
    s.set_self_ok(e.player.trade.ok);
    s.set_dest_ok(p->player.trade.ok);
    s.set_self_money(my_money);
    s.set_dest_money(their_money);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_SYNC), s);
    pb::TradeSync t;
    t.set_self_lock(p->player.trade.locked);
    t.set_dest_lock(e.player.trade.locked);
    t.set_self_ok(p->player.trade.ok);
    t.set_dest_ok(e.player.trade.ok);
    t.set_self_money(their_money);
    t.set_dest_money(my_money);
    emit({p->sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_SYNC), t);
}

// the partner's view of my trade box: an item put there (the 0xcc-byte sync of ExchangeItem 0x08207172) or taken back
void KSubWorld::trade_item_sync(const KNpc& e, std::uint32_t id, bool removed)
{
    const KNpc* p = e.player.trade.trading ? find_player(e.player.trade.dest) : nullptr;
    if (p == nullptr) return;
    const KItemList* list = items_of(e.sid);
    pb::TradeItem m;
    m.set_removed(removed);
    if (list != nullptr) {
        if (const KItem* item = list->find(id)) {
            const auto place = list->place_of(id);
            fill_item_view(*item, place ? *place : KItemPlace{}, *m.mutable_item());
        } else {
            m.mutable_item()->set_id(id);
        }
    }
    emit({p->sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_ITEM), m);
}

// the trade box back into the bag (0x081FC8D0(list, 2) at the start / the cancel): what does not fit lies on the ground
// for the owner (the binary's bag always has the cells the items came from)
void KSubWorld::trade_clear_box(KNpc& e)
{
    KItemList* list = items_of(e.sid);
    if (list == nullptr) return;
    std::vector<std::uint32_t> ids;
    list->each([&](const KItem& item, const KItemPlace& place) {
        if (place.room == room_trade) ids.push_back(item.id);
    });
    for (const std::uint32_t id : ids) {
        const KItem* item = list->find(id);
        if (item == nullptr) continue;
        KItem copy = *item;
        if (list->remove(id)) {
            const std::uint32_t back = list->add(copy, room_equipment);
            if (back != 0) {
                item_moved(e.sid, back, 0);
            } else {
                pb::ItemRemove r;
                r.set_id(id);
                emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_REMOVE), r);
                copy.id = 0;
                drop_item(std::move(copy), e.pos(), e.player_id);
                log::warn("zone.trade", "trade box item dropped", {log::kv("entity", e.id), log::kv("item", id)});
            }
        }
    }
    list->set_money(room_trade, 0);
}

// KPlayer 0x080AE380(player, partner): the duplicate check 0x08207DE0, the trade boxes (rooms 2 and 4) back, KTrade::Release,
// the 0x78 {0} to both, both menu states restored (RestoreBackupState 0x080C2ED0)
void KSubWorld::trade_cancel(KNpc& e)
{
    if (e.kind != KNpcKind::player || !e.player.trade.trading) return;
    KNpc* p = team_player(e.player.trade.dest);
    log::info("zone.trade", "trade cancelled", {log::kv("entity", e.id), log::kv("partner", p != nullptr ? p->id : EntityId{})});
    trade_clear_box(e);
    e.player.trade.release();
    pb::TradeEnd end;
    end.set_ok(false);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_END), end);
    restore_menu_state(e);
    if (p != nullptr && p->player.trade.trading && p->player.trade.dest == e.sid) {
        trade_clear_box(*p);
        p->player.trade.release();
        emit({p->sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_END), end);
        restore_menu_state(*p);
    }
}

// the 0x6d packet 0x080B2C70 {byte decision}: trading with a partner that points back, both normal, the same map, neither
// security-locked; 0 -> the cancel; 2 -> the lock (the ok flags of both cleared, mine lock set) + sync; 3 / 4 -> sync; 1 ->
// both locked: the partner not ok yet -> my ok + sync, the partner ok -> the exchange
bool KSubWorld::trade_decision(KNpc& e, int decision)
{
    e.player.trade.apply = 0;   // 0x080B2C8F: m_nApplyIdx = -1
    if (!trading(e)) return false;
    KNpc* p = trade_partner(e);
    if (p == nullptr || !e.alive() || !p->alive()) return false;
    switch (decision) {
    case 0:
        trade_cancel(e);
        return true;
    case 2:
        if (!e.player.trade.locked) {   // 0x080B2DC9
            e.player.trade.ok = false;
            e.player.trade.locked = true;
            p->player.trade.ok = false;
        }
        trade_sync(e);
        return true;
    case 3:
    case 4:
        trade_sync(e);
        return true;
    case 1:
        if (!e.player.trade.locked || !p->player.trade.locked) {   // 0x080B2E49..0x080B2E6C
            trade_sync(e);
            return true;
        }
        if (!p->player.trade.ok) {   // 0x080B2EAD
            e.player.trade.ok = true;
            trade_sync(e);
            return true;
        }
        return trade_exchange(e, *p);
    default:
        return false;
    }
}

// 0x080B2EC7..0x080B44CC - the second ok: the boxes are checked (0x081FF5D0), the money of both must hold (0 <= box <= bag,
// no overflow of bag + the other's box: else the cancel), the other's items must fit each bag (0x081FA250: else the 0x86
// 0xb / 0xc, the partner's ok cleared, a sync); then every item crosses (logged), the money follows, the 0x78 {1} to both,
// both menu states NORMAL, KTrade::Release
bool KSubWorld::trade_exchange(KNpc& e, KNpc& p)
{
    KItemList* mine = items_of(e.sid);
    KItemList* theirs = items_of(p.sid);
    if (mine == nullptr || theirs == nullptr) return false;
    const std::int64_t my_box = mine->money(room_trade), their_box = theirs->money(room_trade);
    const std::int64_t my_bag = mine->money(room_equipment), their_bag = theirs->money(room_equipment);
    const auto money_ok = [](std::int64_t bag, std::int64_t box, std::int64_t incoming) {
        return box >= 0 && bag >= box && bag - box + incoming <= 0x7fffffff && incoming >= 0;
    };
    if (!money_ok(my_bag, my_box, their_box) || !money_ok(their_bag, their_box, my_box)) {   // 0x080B2F43..0x080B2F9C
        log::warn("zone.trade", "trade money error", {log::kv("entity", e.id), log::kv("partner", p.id), log::kv("money", static_cast<int>(my_box)),
                                                    log::kv("partner_money", static_cast<int>(their_box))});
        trade_cancel(e);
        return false;
    }
    std::vector<KItem> from_me, from_them;
    mine->each([&](const KItem& item, const KItemPlace& place) { if (place.room == room_trade) from_me.push_back(item); });
    theirs->each([&](const KItem& item, const KItemPlace& place) { if (place.room == room_trade) from_them.push_back(item); });
    const auto fits = [](const KItemList& list, const std::vector<KItem>& incoming) {
        KItemList copy = list;
        for (const KItem& it : incoming) {
            KItem c = it;
            c.id = 0;
            int stacked = 0;
            if (copy.add_or_stack(c, room_equipment, &stacked) == 0) return false;
        }
        return true;
    };
    // 0x081FC410 / 0x081FA250 on both: the one whose bag cannot take the other's items loses its ok and hears 0xb, the
    // other 0xc (0x080B31E3 for the presser, 0x080B3016 for the partner); then a sync
    const bool room_mine = fits(*mine, from_them), room_theirs = fits(*theirs, from_me);
    if (!room_mine || !room_theirs) {
        if (!room_mine) {
            e.player.trade.ok = false;
            sys_msg(e.sid, kMsgTradeSelfRoomFull, EntityId{});
            sys_msg(p.sid, kMsgTradeDestRoomFull, EntityId{});
        }
        if (!room_theirs) {
            p.player.trade.ok = false;
            sys_msg(p.sid, kMsgTradeSelfRoomFull, EntityId{});
            sys_msg(e.sid, kMsgTradeDestRoomFull, EntityId{});
        }
        trade_sync(e);
        return false;
    }
    // the items cross: out of one list (ItemRemove), into the other's bag (ItemAdd), one log line each (the trade log
    // "%s\t%s\tdate\tNewWorld(map, x, y)\tItemName[%s]..." of 0x080B3759)
    const auto cross = [&](KNpc& giver, KItemList& from, KNpc& taker, const std::vector<KItem>& items) {
        for (const KItem& it : items) {
            const std::uint32_t old_id = it.id;
            KItem copy = it;
            copy.id = 0;
            if (!from.remove(old_id)) continue;
            pb::ItemRemove r;
            r.set_id(old_id);
            emit({giver.sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_REMOVE), r);
            const std::uint32_t new_id = give_item(taker.sid, std::move(copy));
            log::info("zone.trade", "item traded", {log::kv("entity", giver.id), log::kv("partner", taker.id), log::kv("item", old_id),
                                                   log::kv("new_id", new_id), log::kv("name", it.name()), log::kv("count", it.count)});
        }
    };
    cross(e, *mine, p, from_me);
    cross(p, *theirs, e, from_them);
    mine->set_money(room_trade, 0);
    theirs->set_money(room_trade, 0);
    mine->set_money(room_equipment, static_cast<int>(my_bag - my_box + their_box));
    theirs->set_money(room_equipment, static_cast<int>(their_bag - their_box + my_box));
    send_money(e.sid);
    send_money(p.sid);
    log::info("zone.trade", "trade done", {log::kv("entity", e.id), log::kv("partner", p.id), log::kv("items", from_me.size()),
                                          log::kv("partner_items", from_them.size()), log::kv("money", static_cast<int>(my_box)),
                                          log::kv("partner_money", static_cast<int>(their_box))});
    pb::TradeEnd end;
    end.set_ok(true);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_END), end);
    emit({p.sid}, static_cast<std::uint16_t>(pb::G2C_TRADE_END), end);
    e.player.trade.release();
    p.player.trade.release();
    set_menu_state(e, menu_state_normal, {}, EntityId{});   // 0x080B4448
    set_menu_state(p, menu_state_normal, {}, EntityId{});
    return true;
}

}  // namespace jx::zone
