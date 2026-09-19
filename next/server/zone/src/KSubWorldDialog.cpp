// KSubWorldDialog.cpp - a player talks to a npc: the npc's script main(), Say / Talk to the client, the answer back
// into the script (docs/LINUX-SERVER.md §20).
//
// jx_linux_y KPlayer::DialogNpc 0x080B1300 (the 0x6e packet): not trading (+0x5700 == 3 or +0x5920), the sync code
// (0x080A79B0, not ported), the npc by id (0x080B12C0), the distance of the two (0x0809F370, pixels), a dialoger
// (kind 3) or a npc the player is at peace with (0x0809EE50 == 1), within twice the npc's m_DialogRadius (+0x1634 =
// 124), the anti-addiction gate (0x08176160 == 2 and npc+0x18e4 bit 0 -> L_PLAYER_TIRE_MSG11; not ported), a npc with
// a script (+0x1538); then Player+8 = +0xc = the npc, the script event 15 (eventsys, not ported), task_function.lua
// OnEventTalkNpc(npc) and the npc's script main(npc+0x158c) through 0x080AD300 (CallFunction(fn, 0, "d", param)).
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KText.h"
#include "jx/zone/ScriptFuns.h"

namespace jx::zone {

namespace {

constexpr const char* kTaskFunctionScript = R"(\script\task\system\task_function.lua)";   // 0x08256720

// a script call with the context of this player; the script's game path goes into the context so Say / Talk
// remember where the answers return to
std::optional<double> call_in_script(KSubWorld& world, KLuaScript& script, const std::string& game_path, const char* fn, KNpc& player,
                                     const std::vector<KLuaScript::Arg>& args)
{
    KScriptContext& ctx = g_ScriptContext();
    const KScriptContext saved = ctx;
    ctx.world = &world;
    ctx.player = &player;
    ctx.sid = player.sid;
    ctx.script_path = game_path;
    const std::optional<double> r = script.call_number(fn, args);
    ctx = saved;
    return r;
}

}   // namespace

bool KSubWorld::dialog_npc_request(std::uint64_t sid, EntityId npc_id)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    KNpc& e = entities_.at(pit->second);
    log::ScopedContext lctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    if (trading(e)) {   // 0x080B1315 / 0x080B1322
        log::debug("zone.dialog", "dialog refused", {log::kv("entity", e.id), log::kv("npc", npc_id), log::kv("reason", "trading")});
        return false;
    }
    KNpc* npc = entities_.find(npc_id);   // 0x080B1389: FindNpcById > 0
    if (npc == nullptr || npc == &e) {
        log::debug("zone.dialog", "dialog refused", {log::kv("entity", e.id), log::kv("npc", npc_id), log::kv("reason", "no npc")});
        return false;
    }
    // 0x0809F370(npc, player): the distance in pixels (cells x the cell width + the fine offset)
    const double dx = static_cast<double>(e.pos().x - npc->pos().x);
    const double dy = static_cast<double>(e.pos().y - npc->pos().y);
    const int dist = static_cast<int>(std::sqrt(dx * dx + dy * dy));
    // 0x080B13BB: a dialoger (kind 3) talks to anyone; another npc only when the relation is 1 (0x080B14A8 -> 0x0809EE50)
    if (npc->npc_kind != 3 && relation(e, *npc) != 1) {
        log::debug("zone.dialog", "dialog refused", {log::kv("entity", e.id), log::kv("npc", npc_id), log::kv("reason", "relation"),
                                                     log::kv("relation", relation(e, *npc))});
        return false;
    }
    if (dist > 2 * npc->base.dialog_radius) {   // 0x080B13C7..0x080B13D1: dist <= 2 * m_DialogRadius
        log::debug("zone.dialog", "dialog refused", {log::kv("entity", e.id), log::kv("npc", npc_id), log::kv("reason", "too far"),
                                                     log::kv("distance", dist), log::kv("radius", npc->base.dialog_radius)});
        return false;
    }
    if (npc->script.empty()) {   // 0x080B13EE: npc+0x1538 == 0 - nothing to say
        log::debug("zone.dialog", "dialog refused", {log::kv("entity", e.id), log::kv("npc", npc_id), log::kv("reason", "no script")});
        return false;
    }
    e.player.dialog.npc = npc->id;   // Player+8 / +0xc (0x080B140B)
    log::info("zone.dialog", "dialog started", {log::kv("entity", e.id), log::kv("npc", npc->id), log::kv("name", npc->name),
                                                 log::kv("script", npc->script), log::kv("distance", dist)});
    // 0x080B141B: the task system first - OnEventTalkNpc(npc) of \script\task\system\task_function.lua; the binary takes
    // its answer only together with Player+0x78ec (the task dialog flag no script api of the zone sets), so main follows
    if (cfg_.scripts) {
        if (KLuaScript* tf = cfg_.scripts->get(kTaskFunctionScript); tf != nullptr && tf->has_function("OnEventTalkNpc")) {
            call_in_script(*this, *tf, kTaskFunctionScript, "OnEventTalkNpc", e, {static_cast<double>(npc->id.value)});
        }
    }
    // 0x080B1457: main(npc+0x158c) of the npc's script - the placement carries no parameter here, so 0
    const bool ok = execute_script(npc->script, "main", e, 0);
    if (!ok) log::warn("zone.dialog", "dialog script failed", {log::kv("entity", e.id), log::kv("npc", npc->id), log::kv("script", npc->script)});
    return ok;
}

// the 0x63 packet (0x080A8510: +7 must be 1, the protocol byte 0x63, the size = +0xd + 0x10) to the player
void KSubWorld::send_script_action(const KNpc& e, int ui_id, std::string_view text, int text_id, const std::vector<std::string>& options,
                                   int param, bool interactive, bool notify)
{
    pb::ScriptAction a;
    a.set_notify_changes(notify);
    a.set_operate(static_cast<std::uint32_t>(script_action_ui_show));
    a.set_ui_id(static_cast<std::uint32_t>(ui_id));
    a.set_text(std::string(text));
    a.set_text_id(text_id);
    a.set_interactive(interactive);
    a.set_param(param);
    for (const std::string& o : options) a.add_options(o);
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_SCRIPT_ACTION), a);
}

// Lua Say (0x08123C90): the sentence (a string, or a number = a string-table id: m_bParam1 1), the count of answers,
// the answers as more strings or as a table (0x08123D8B / 0x08124120); at most 50 (0x08123DBC); an answer is
// "text/function" - the function name (0x7f bytes) goes to m_szTaskAnswerFun[i], "main" when there is no '/'
// (0x08123EA7); a missing answer is an empty string with "main" (0x08123F88); the packet holds at most 0x384 bytes of
// sentence + answers, the rest of the answers are dropped (0x08123EC1 -> 0x08123FE0); m_nAvailableAnswerNum = the
// count, m_bWaitingPlayerFeedBack = count != 0 (0x08124019).  A number sentence is sent as 4 bytes (0x08123E01).
void KSubWorld::dialog_say(KNpc& e, std::string_view text, int text_id, const std::vector<std::string>& answers)
{
    KPlayerDialog& d = e.player.dialog;
    d.waiting = false;   // 0x08123CDB
    d.script = g_ScriptContext().script_path;   // +0x5f9c: the script the answers go back to
    // the bytes of the script (TCVN3 Vietnamese / GBK) measure the packet as the binary does; the client gets UTF-8
    std::string raw(text);
    if (text_id == 0 && raw.size() > kDialogSentenceMax) raw.resize(kDialogSentenceMax);   // 0x081240E0
    std::size_t used = text_id != 0 ? 4 : raw.size() + 1;
    const std::string sentence = text::decode_mixed(raw);
    std::vector<std::string> shown;
    d.clear_answers();
    const int count = std::min<int>(static_cast<int>(answers.size()), kDialogAnswers);
    int kept = 0;
    for (int i = 0; i < count; ++i) {
        std::string display = answers[static_cast<std::size_t>(i)];
        std::string fun = "main";
        if (const std::size_t slash = display.find('/'); slash != std::string::npos) {   // 0x0805CF90(answer, "/")
            fun = display.substr(slash + 1);
            display.resize(slash);
        }
        if (used + display.size() + 1 > kDialogContentMax) break;   // 0x08123EC1: this answer and the rest are dropped
        used += display.size() + 1;
        if (fun.size() > kDialogAnswerFunMax) fun.resize(kDialogAnswerFunMax);
        d.answer_fun[static_cast<std::size_t>(i)] = fun;
        shown.push_back(text::decode_mixed(display));
        ++kept;
    }
    d.available_answers = kept;   // 0x08124019
    d.waiting = kept != 0;        // 0x0812401F
    log::debug("zone.dialog", "dialog say", {log::kv("entity", e.id), log::kv("len", sentence.size()), log::kv("answers", kept),
                                             log::kv("script", d.script)});
    send_script_action(e, ui_select_dialog, sentence, text_id, shown, -1, true);   // +9 = -1 (0x08124035), +7 = 1
}

// Lua Talk (0x08116930): the number of pages, the function called when the last page is confirmed ("" = none), the
// pages (strings; a number is printed with "%d", 0x08116B7B); the count is clamped to the arguments (0x08116A01);
// the pages are joined with "| |" up to 0x384 bytes (0x08116AF8), the rest dropped; with a callback
// m_szTaskAnswerFun[0] = it, m_nAvailableAnswerNum = 1, m_bWaitingPlayerFeedBack = 1 and the packet's +9 = 1 (the
// client's "confirm notify"), else 0 / 0 / 0 (0x08116C35..0x08116CD8).
void KSubWorld::dialog_talk(KNpc& e, std::string_view callback, const std::vector<std::string>& pages)
{
    KPlayerDialog& d = e.player.dialog;
    d.waiting = false;   // 0x08116982
    d.script = g_ScriptContext().script_path;
    std::vector<std::string> shown;
    std::size_t used = 0;
    for (const std::string& p : pages) {
        if (used + p.size() + 3 > kDialogContentMax) break;   // 0x08116AF8
        used += p.size() + 3;
        shown.push_back(text::decode_mixed(p));
    }
    d.clear_answers();
    int param = 0;
    if (!callback.empty()) {   // 0x08116C86
        std::string fun(callback);
        if (fun.size() > kDialogAnswerFunMax) fun.resize(kDialogAnswerFunMax);
        d.answer_fun[0] = fun;
        d.available_answers = 1;
        d.waiting = true;
        param = 1;
    }
    log::debug("zone.dialog", "dialog talk", {log::kv("entity", e.id), log::kv("pages", shown.size()), log::kv("function", callback),
                                              log::kv("script", d.script)});
    send_script_action(e, ui_talk_dialog, {}, 0, shown, param, true);
}

// Lua Describe (0x081242A0): Say's shape with the ui id 12 (0x08124496): the sentence as given (a string sent whole,
// 0x081247E0; or a string-table id: +6 = 1 and 4 bytes, 0x08124780), the answers "text/function" (0x0805CF90 with "/":
// the name into m_szTaskAnswerFun, 0x7f bytes, "main" without a '/', 0x08124593; a missing one is empty, 0x081246F0);
// an answer's text longer than 0xc8 bytes is cut to 0xc8 (0x081245A3 -> 0x0822A0A0, by whole characters of the code
// page 0x97b2b68 - bytes here); the answers alone are budgeted, the sentence is not: an answer costs its bytes + 3 for
// the "| |" written before it (0x081246D4) - the first after a string sentence costs its bytes only (0x08124616 /
// 0x0812462C) - and the one that would pass 0x1f4 stops the list (0x081245BB -> 0x08124700); m_nAvailableAnswerNum
// and m_bWaitingPlayerFeedBack as Say (0x08124726 / 0x0812472C); +9 = -1 (0x08124749), +7 = 1 (0x081244C0).
void KSubWorld::dialog_describe(KNpc& e, std::string_view text, int text_id, const std::vector<std::string>& answers)
{
    KPlayerDialog& d = e.player.dialog;
    d.waiting = false;   // 0x081242E0
    d.script = g_ScriptContext().script_path;   // 0x081242F5
    const std::string sentence = text::decode_mixed(std::string(text));
    std::vector<std::string> shown;
    d.clear_answers();
    const int count = std::min<int>(static_cast<int>(answers.size()), kDialogAnswers);   // 0x08124478
    std::size_t used = 0;
    int kept = 0;
    for (int i = 0; i < count; ++i) {
        std::string display = answers[static_cast<std::size_t>(i)];
        std::string fun = "main";
        if (const std::size_t slash = display.find('/'); slash != std::string::npos) {
            fun = display.substr(slash + 1);
            display.resize(slash);
        }
        if (display.size() > kDescribeAnswerMax) display.resize(kDescribeAnswerMax);   // 0x081245A3
        if (used + 3 + display.size() > kDescribeContentMax) break;                 // 0x081245BB: this and the rest dropped
        const bool separated = i > 0 || text_id != 0;                                // 0x08124616 / 0x0812462C
        used += display.size() + (separated ? 3 : 0);
        if (fun.size() > kDialogAnswerFunMax) fun.resize(kDialogAnswerFunMax);
        d.answer_fun[static_cast<std::size_t>(i)] = fun;
        shown.push_back(text::decode_mixed(display));
        ++kept;
    }
    d.available_answers = kept;   // 0x08124726
    d.waiting = kept != 0;        // 0x0812472C
    log::debug("zone.dialog", "dialog describe", {log::kv("entity", e.id), log::kv("len", sentence.size()), log::kv("answers", kept),
                                                  log::kv("script", d.script)});
    send_script_action(e, ui_describe_dialog, sentence, text_id, shown, -1, true);
}

// Lua TaskTip (0x08122730): a 0x40 byte buffer holds a 0x10 byte and the text (strcat, 0x081227A9), the safe copy
// 0x08227030 takes 0x3f bytes of it and a NUL, and the 0x41 byte packet {0xb6, buffer} goes to the player (0x080A8400):
// 0x3e bytes of text.  The client (0x00651390) copies the text, and the leading 0x10 picks the ui message 0x5d (else
// 0x52): the system message pane 0x004C4060 with the type 1, the blink flag 1 and the priority 3 (docs/CLIENT-2.0.md §26).
void KSubWorld::task_tip(KNpc& e, std::string_view text)
{
    const std::string raw(text.substr(0, kTaskTipMax));
    pb::TaskTip m;
    m.set_text(text::decode_mixed(raw));
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_TASK_TIP), m);
    log::debug("zone.dialog", "task tip", {log::kv("entity", e.id), log::kv("len", raw.size())});
}

// Lua GiveItemUI (0x0812BBA0): three arguments at least, a player above 0; +0x5f9c = the script (0x0812BBFB); the packet with
// the ui id 0xb, +5 = 1, +7 = 1 (0x0812C044), +8 = the seventh argument (0x0812C180), +9 = the fifth (0x0812BFE0); the
// content (the second argument) a string copied whole with the title (the first) after its NUL (0x0812BC89..0x0812BCD5),
// or a string-table id as 4 bytes with the title after (0x0812BD30); **no confirm function (the third) -> the answers
// cleared and nothing sent** (0x0812BD0A); else m_szTaskAnswerFun[0] = confirm, [1] = cancel (the fourth; 0x80 bytes each,
// 0x0812BDD1 / 0x0812BE04), m_nAvailableAnswerNum = 2, m_bWaitingPlayerFeedBack = 1; the sixth argument into +0x60a0
// (0x0812C131); the trade block reset into the give mode: +0x52a4 = -3, +0x52a8 = the npc talked to, +0x52ac / +0x52b0
// its position (0x080EF710), +0x52b8 = the fifth argument, the six units of 0x98 bytes cleared (0x0812BE36..0x0812BF98);
// then the packet (0x080A8400, the content's length + 0x11 bytes).
void KSubWorld::dialog_give_item_ui(KNpc& e, std::string_view title, std::string_view content, int text_id, std::string_view confirm_fun,
                                    std::string_view cancel_fun, int param, std::string_view select_fun, bool notify)
{
    KPlayerDialog& d = e.player.dialog;
    KPlayerGiveItem& g = e.player.give;
    d.script = g_ScriptContext().script_path;
    if (confirm_fun.empty()) {   // 0x0812BD0A
        d.clear_answers();
        d.waiting = false;
        log::debug("zone.dialog", "give item ui without a confirm function", {log::kv("entity", e.id), log::kv("script", d.script)});
        return;
    }
    d.clear_answers();
    std::string confirm(confirm_fun), cancel(cancel_fun), select(select_fun);
    if (confirm.size() > kDialogAnswerFunMax) confirm.resize(kDialogAnswerFunMax);
    if (cancel.size() > kDialogAnswerFunMax) cancel.resize(kDialogAnswerFunMax);
    if (select.size() > kDialogAnswerFunMax) select.resize(kDialogAnswerFunMax);
    d.answer_fun[0] = confirm;
    d.answer_fun[1] = cancel;
    d.available_answers = 2;   // 0x0812BDB2
    d.waiting = true;          // 0x0812BE18
    g.active = true;
    g.npc = d.npc;
    g.param = param;
    g.select_fun = select;
    g.clear_units();
    const std::string shown_content = text::decode_mixed(std::string(content));
    const std::vector<std::string> options{text::decode_mixed(std::string(title))};
    log::debug("zone.dialog", "give item ui", {log::kv("entity", e.id), log::kv("function", confirm), log::kv("cancel", cancel),
                                               log::kv("select", select), log::kv("param", param), log::kv("notify", notify), log::kv("script", d.script)});
    send_script_action(e, ui_give_item, shown_content, text_id, options, param, true, notify);
}

bool KSubWorld::give_items_request(std::uint64_t sid, int kind, const std::vector<KGiveItemEntry>& entries)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    KNpc& e = entities_.at(pit->second);
    log::ScopedContext lctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    KPlayerGiveItem& g = e.player.give;
    if (trading(e) || !g.active) {   // 0x080AB992..0x080AB9BF
        log::debug("zone.dialog", "give items refused", {log::kv("entity", e.id), log::kv("reason", trading(e) ? "trading" : "no box")});
        return false;
    }
    g.clear_units();   // 0x080AB9C1
    const KItemList* list = items_of(sid);
    const std::size_t n = std::min<std::size_t>(entries.size(), static_cast<std::size_t>(kGiveItemUnits));   // 0x080AB9F7: 24 at most
    for (std::size_t i = 0; i < n; ++i) {
        const KGiveItemEntry& en = entries[i];
        const std::uint32_t id = list != nullptr ? list->item_at(en.room, en.x, en.y) : 0;   // 0x081FB0D0
        if (id == 0 || en.cell_x < 0 || en.cell_x >= kGiveBoxWidth || en.cell_y < 0 || en.cell_y >= kGiveBoxHeight) continue;   // 0x080ABA57..0x080ABA6B: skipped
        // 0x080ABA71: with the fifth argument 0 a bound piece (Item+0x350) refuses the whole list - the zone has no binding yet
        const int cell = en.cell_y * kGiveBoxWidth + en.cell_x + 1;   // 0x080ABA96..0x080ABAA3
        for (int j = 0; j < g.count; ++j) {
            if (g.units[static_cast<std::size_t>(j)] == id || g.cells[static_cast<std::size_t>(j)] == cell) {   // 0x080ABAC8..0x080ABAD8: twice -> nothing
                g.clear_units();
                log::debug("zone.dialog", "give items refused", {log::kv("entity", e.id), log::kv("reason", "duplicate"), log::kv("item", id), log::kv("cell", cell)});
                return false;
            }
        }
        g.units[static_cast<std::size_t>(g.count)] = id;
        g.cells[static_cast<std::size_t>(g.count)] = cell;
        ++g.count;
    }
    KPlayerDialog& d = e.player.dialog;
    if (kind == 0) {   // 0x080AC595 -> 0x080AC400: the sixth argument's function in the npc's script
        const KNpc* npc = entities_.find(g.npc);
        if (npc == nullptr || npc->script.empty() || g.select_fun.empty()) {
            log::debug("zone.dialog", "give items", {log::kv("entity", e.id), log::kv("kind", kind), log::kv("count", g.count), log::kv("function", "")});
            return true;
        }
        const bool ok = execute_script(npc->script, g.select_fun.c_str(), e, g.count);
        log::debug("zone.dialog", "give items", {log::kv("entity", e.id), log::kv("kind", kind), log::kv("count", g.count), log::kv("function", g.select_fun), log::kv("ok", ok)});
        return ok;
    }
    // 0x080AC4B0: the confirm function of the dialog script with the count; every answer cleared first (0x080AC508..0x080AC51A)
    if (d.script.empty() || d.answer_fun[0].empty()) {
        log::debug("zone.dialog", "give items", {log::kv("entity", e.id), log::kv("kind", kind), log::kv("count", g.count), log::kv("function", "")});
        return true;
    }
    const std::string fn = d.answer_fun[0];
    d.clear_answers();
    const bool ok = execute_script(d.script, fn.c_str(), e, g.count);
    log::debug("zone.dialog", "give items", {log::kv("entity", e.id), log::kv("kind", kind), log::kv("count", g.count), log::kv("function", fn), log::kv("ok", ok)});
    return ok;
}

// SetUiGiveItemMsg 0x0810B020 / SetUiGiveItemMoreConfirmMsg 0x0810AF50: a string and a player 1..0x4af; the packet {0xd8 |
// 0xdf, int size = length + 9, int 0, the text} (0x0810B097..0x0810B0D2) - the 2.0 client answers 0xdf with the ui message
// 0xa8 (0x0064FD10, a box asking once more); its 0xd8 handler (0x00654C60) is another feature, so the hint of the window
// comes from this message here
void KSubWorld::give_item_msg(KNpc& e, int kind, std::string_view text)
{
    pb::GiveItemMsg m;
    m.set_kind(kind);
    m.set_text(text::decode_mixed(std::string(text)));
    emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_GIVE_ITEM_MSG), m);
    log::debug("zone.dialog", "give item message", {log::kv("entity", e.id), log::kv("kind", kind), log::kv("len", text.size())});
}

// the 0x5f packet (0x080AC5D0): not trading; m_bWaitingPlayerFeedBack = 0; a negative index becomes 0 (0x080AC609);
// kind 1 is the other selection ui (0x081F68C0, not ported), anything else than 0 is ignored; the index must be below
// m_nAvailableAnswerNum (0x080AC61A) and the player's npc must be there; the function of that answer: empty -> nothing;
// '#...' -> the rest is Lua code run in the dialog's script (0x080AC6E6: PlayerIndex / PlayerId / SubWorld set,
// 0x082222E0 LoadBuffer + 0x08222100 Execute); else the name is copied, every slot cleared, the count zeroed and
// CallFunction(name, 0, "d", index) runs it in the dialog's script (0x080AC680 -> 0x080AC1A0)
bool KSubWorld::dialog_answer(std::uint64_t sid, int index, int kind)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    KNpc& e = entities_.at(pit->second);
    log::ScopedContext lctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    KPlayerDialog& d = e.player.dialog;
    if (trading(e)) return false;   // 0x080AC5E8
    d.waiting = false;              // 0x080AC5FE
    if (index < 0) index = 0;       // 0x080AC609
    if (kind != 0) {                // 0x080AC613 / 0x080AC658
        log::debug("zone.dialog", "dialog answer ignored", {log::kv("entity", e.id), log::kv("answer", index), log::kv("kind", kind)});
        return false;
    }
    if (index >= d.available_answers) {   // 0x080AC61A
        log::debug("zone.dialog", "dialog answer ignored", {log::kv("entity", e.id), log::kv("answer", index), log::kv("answers", d.available_answers)});
        return false;
    }
    const std::string name = d.answer_fun[static_cast<std::size_t>(index)];
    if (name.empty()) return false;   // 0x080AC641
    if (!cfg_.scripts || d.script.empty()) {
        log::warn("zone.dialog", "dialog answer without a script", {log::kv("entity", e.id), log::kv("answer", index), log::kv("function", name)});
        d.clear_answers();
        return false;
    }
    KLuaScript* script = cfg_.scripts->get(d.script);
    if (script == nullptr) {
        d.clear_answers();
        return false;
    }
    d.clear_answers();   // 0x080AC6A0 / 0x080AC7D8: every slot cleared, the count zeroed before the call
    KScriptContext& ctx = g_ScriptContext();
    const KScriptContext saved = ctx;
    ctx.world = this;
    ctx.player = &e;
    ctx.sid = sid;
    ctx.script_path = d.script;
    bool ok = false;
    if (name[0] == '#') {   // 0x080AC6E6: the answer is a piece of code
        std::string error;
        ok = script->do_string(name.substr(1), "dialog answer", &error);
        log::info("zone.dialog", "dialog answer code", {log::kv("entity", e.id), log::kv("answer", index), log::kv("code", name.substr(1)),
                                                        log::kv("ok", ok), log::kv("error", error)});
    } else {
        ok = script->call_number(name.c_str(), {static_cast<double>(index)}).has_value() || script->has_function(name.c_str());
        log::info("zone.dialog", "dialog answer", {log::kv("entity", e.id), log::kv("answer", index), log::kv("function", name),
                                                   log::kv("script", d.script), log::kv("ok", ok)});
    }
    ctx = saved;
    return ok;
}

}   // namespace jx::zone
