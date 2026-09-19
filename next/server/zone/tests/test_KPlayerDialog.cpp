// The npc dialog of the JX2 server (docs/LINUX-SERVER.md §20): KPlayer::DialogNpc 0x080B1300 (the 0x6e click: a dialoger
// within twice its dialog radius runs its script's main), Lua Say 0x08123C90 / Talk 0x08116930 (the 0x63 packet and the
// answer functions kept on the player), the 0x5f answer 0x080AC5D0 (the function of the answer, or the code after '#').
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/role.pb.h"
#include "jx/zone/KLuaScript.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KPlayer.h"
#include "jx/zone/KPlayerDialog.h"
#include "jx/zone/KPlayerSet.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"
#include "jx/zone/KText.h"

using jx::zone::KNpc;
using jx::zone::KNpcKind;
using jx::zone::KScriptCache;

namespace {

constexpr const char* kNpcScript = R"(
g_last = -1
function main(param)
    g_param = param
    Say("Xin chao", 3, "Mot/OnOne", "Hai/#g_last = 200", "Ba")
end
function OnOne(i)
    g_last = 100 + i
end
function talk()
    Talk(2, "OnTalkDone", "Trang mot", "Trang hai")
end
function talk_plain()
    Talk(1, "", 42)
end
function say_table()
    Say(7, 2, {"Bon/OnFour", "Nam"})
end
function say_none()
    Say("Chao", 0)
end
function OnTalkDone(i)
    g_last = 300 + i
end
function OnFour(i)
    g_last = 400 + i
end
function describe()
    Describe("Mo ta", 3, "Mot/OnOne", "Hai", "Ba/#g_last = 500")
end
function describe_table()
    Describe(9, 2, {"Bon/OnFour", "Nam"})
end
function describe_nocount()
    Describe("Mo ta", "x", "Mot")
end
function describe_count_only()
    Describe("Mo ta", 2)
end
function describe_none()
    Describe("Mo ta", 0)
end
function describe_long()
    local a = string.rep("x", 250)
    Describe("Mo ta", 3, a .. "/OnOne", a, a)
end
function describe_short()
    Describe("Mo ta", 5, "Mot/OnOne", "Hai")
end
function notes()
    AddNote("Dai hiep da thu thap du Hong Moc.", 7)
    AddNote(1234)
    AddNote("Khong so")
    AddNote()
    AddNote({})
end
function GetLast()
    return g_last
end
function GetParam()
    return g_param
end
)";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_dialog_test";
    std::filesystem::create_directories(root / "script" / "test");
    std::ofstream(root / "script" / "test" / "npc.lua") << kNpcScript;
    return root.string();
}

jx::zone::KSubWorldConfig dialog_world()
{
    jx::zone::KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.view_cells = 1;
    c.interest_period = 1;
    c.view_slack = 0;
    c.far_period = 1;
    c.spawn_point = jx::zone::Pos{2000, 2000};
    c.seed = 7;
    c.player_set = std::make_shared<jx::zone::KPlayerSet>();
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    return c;
}

jx::pb::RoleData role_of(std::uint64_t player_id, const char* name, int x, int y)
{
    jx::pb::RoleData r;
    r.set_player_id(player_id);
    r.set_name(name);
    r.set_level(10);
    auto* s = r.mutable_stats();
    s->set_strength(35);
    s->set_dexterity(25);
    s->set_vitality(25);
    s->set_energy(15);
    s->set_hp_max(204);
    s->set_hp(204);
    s->set_mp_max(100);
    s->set_mp(100);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(x);
    r.mutable_position()->mutable_pos()->set_y(y);
    return r;
}

std::vector<jx::pb::ScriptAction> actions(const std::vector<jx::zone::Packet>& all, std::uint64_t sid)
{
    std::vector<jx::pb::ScriptAction> out;
    for (const auto& p : all) {
        if (p.msg_id != jx::pb::G2C_SCRIPT_ACTION || std::find(p.sids.begin(), p.sids.end(), sid) == p.sids.end()) continue;
        jx::pb::ScriptAction a;
        REQUIRE(a.ParseFromString(p.payload));
        out.push_back(a);
    }
    return out;
}

// A (sid 7) at the spawn; a dialoger with the script 100 px away, another 400 px away, a monster with a script next to A
struct DialogWorld {
    jx::zone::KSubWorld w;
    jx::EntityId a, near, far, monster;
    DialogWorld() : w(dialog_world())
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
        jx::zone::Pos at;
        REQUIRE(w.spawn_player(7, role_of(1, "A", 2000, 2000), a, at) == jx::pb::RESULT_OK);
        near = w.spawn_npc("Ban", jx::zone::Pos{2100, 2000}, 0, 0, KNpcKind::npc, 1, 0, 0);
        far = w.spawn_npc("Xa", jx::zone::Pos{2400, 2000}, 0, 0, KNpcKind::npc, 1, 0, 0);
        monster = w.spawn_npc("Quai", jx::zone::Pos{2000, 2060}, 0, 0, KNpcKind::monster, 1, 0, 0);
        for (const jx::EntityId id : {near, far, monster}) {
            KNpc* n = w.mutable_entity(id);
            REQUIRE(n != nullptr);
            n->script = R"(\script\test\npc.lua)";
        }
        w.mutable_entity(near)->npc_kind = 3;
        w.mutable_entity(far)->npc_kind = 3;
        w.tick();
        w.tick();
        w.take_outbox();
    }
    KNpc& A() { return *w.mutable_entity(a); }
    double last()
    {
        jx::zone::KLuaScript* s = w.config().scripts->get(R"(\script\test\npc.lua)");
        REQUIRE(s != nullptr);
        return s->call_number("GetLast", {}).value_or(-999.0);
    }
};

} // namespace

TEST_CASE("the constants of the dialog follow the binary", "[dialog]")
{
    CHECK(jx::zone::kDialogAnswers == 50);
    CHECK(jx::zone::kDialogRadius == 124);
    CHECK(jx::zone::kDialogContentMax == 0x384);
    CHECK(jx::zone::kDialogAnswerFunMax == 0x7f);
    KNpc n;
    CHECK(n.base.dialog_radius == 124);   // KNpc::Init 0x0807E02D
}

TEST_CASE("DialogNpc 0x080B1300: a dialoger within twice its radius runs main; too far, a monster or no script refuse", "[dialog][world]")
{
    DialogWorld dw;
    // 400 px > 2 * 124
    CHECK_FALSE(dw.w.dialog_npc_request(7, dw.far));
    // a monster (kind 0) with a script: the relation with the player is not 1
    CHECK_FALSE(dw.w.dialog_npc_request(7, dw.monster));
    // a dialoger without a script: nothing to say (npc+0x1538 == 0)
    dw.w.mutable_entity(dw.near)->script.clear();
    CHECK_FALSE(dw.w.dialog_npc_request(7, dw.near));
    dw.w.mutable_entity(dw.near)->script = R"(\script\test\npc.lua)";
    CHECK(dw.w.take_outbox().empty());
    // a nobody
    CHECK_FALSE(dw.w.dialog_npc_request(7, jx::EntityId{99999}));
    // 100 px: main(0) -> Say("Xin chao", 3, ...) -> the 0x63 packet
    REQUIRE(dw.w.dialog_npc_request(7, dw.near));
    const auto acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].operate() == 0);
    CHECK(acts[0].ui_id() == jx::zone::ui_select_dialog);
    CHECK(acts[0].text() == "Xin chao");
    CHECK(acts[0].text_id() == 0);
    CHECK(acts[0].interactive());
    CHECK(acts[0].param() == -1);
    REQUIRE(acts[0].options_size() == 3);
    CHECK(acts[0].options(0) == "Mot");
    CHECK(acts[0].options(1) == "Hai");
    CHECK(acts[0].options(2) == "Ba");
    const KNpc& A = dw.A();
    CHECK(A.player.dialog.npc == dw.near);
    CHECK(A.player.dialog.available_answers == 3);
    CHECK(A.player.dialog.waiting);
    CHECK(A.player.dialog.answer_fun[0] == "OnOne");
    CHECK(A.player.dialog.answer_fun[1] == "#g_last = 200");
    CHECK(A.player.dialog.answer_fun[2] == "main");   // no '/' -> main (0x08123EA7)
    CHECK(A.player.dialog.script == R"(\script\test\npc.lua)");
    jx::zone::KLuaScript* s = dw.w.config().scripts->get(R"(\script\test\npc.lua)");
    REQUIRE(s != nullptr);
    CHECK(s->call_number("GetParam", {}) == 0.0);   // main(0): the placement has no parameter
    // a talk while trading is refused (0x080B1315)
    dw.A().player.trade.trading = true;
    CHECK_FALSE(dw.w.dialog_npc_request(7, dw.near));
    dw.A().player.trade.trading = false;
}

TEST_CASE("the 0x5f answer 0x080AC5D0: the function of the answer with its index, '#' code, the bounds, negative -> 0", "[dialog][world]")
{
    DialogWorld dw;
    REQUIRE(dw.w.dialog_npc_request(7, dw.near));
    dw.w.take_outbox();
    // out of the count (0x080AC61A) and the other ui kind (0x080AC658): ignored, the answers stay
    CHECK_FALSE(dw.w.dialog_answer(7, 3, 0));
    CHECK_FALSE(dw.w.dialog_answer(7, 0, 1));
    CHECK(dw.A().player.dialog.available_answers == 3);
    // answer 0 -> OnOne(0): the table is cleared before the call (0x080AC6A0)
    REQUIRE(dw.w.dialog_answer(7, 0, 0));
    CHECK(dw.last() == 100.0);
    CHECK(dw.A().player.dialog.available_answers == 0);
    CHECK_FALSE(dw.A().player.dialog.waiting);
    CHECK_FALSE(dw.w.dialog_answer(7, 0, 0));   // nothing left to answer
    // answer 1 -> the code after '#'
    REQUIRE(dw.w.dialog_npc_request(7, dw.near));
    dw.w.take_outbox();
    REQUIRE(dw.w.dialog_answer(7, 1, 0));
    CHECK(dw.last() == 200.0);
    // answer 2 -> "main" with 2: Say again, a new packet
    REQUIRE(dw.w.dialog_npc_request(7, dw.near));
    dw.w.take_outbox();
    REQUIRE(dw.w.dialog_answer(7, 2, 0));
    CHECK(actions(dw.w.take_outbox(), 7).size() == 1);
    CHECK(dw.A().player.dialog.available_answers == 3);
    jx::zone::KLuaScript* s = dw.w.config().scripts->get(R"(\script\test\npc.lua)");
    CHECK(s->call_number("GetParam", {}) == 2.0);
    // a negative index is answer 0 (0x080AC609)
    REQUIRE(dw.w.dialog_answer(7, -5, 0));
    CHECK(dw.last() == 100.0);
    // an answer while trading is refused (0x080AC5E8)
    REQUIRE(dw.w.dialog_npc_request(7, dw.near));
    dw.w.take_outbox();
    dw.A().player.trade.trading = true;
    CHECK_FALSE(dw.w.dialog_answer(7, 0, 0));
    dw.A().player.trade.trading = false;
}

TEST_CASE("Talk 0x08116930: the pages, the callback of the last page; Say with a table, with a number, with no answer", "[dialog][world]")
{
    DialogWorld dw;
    KNpc& A = dw.A();
    // Talk(2, "OnTalkDone", "Trang mot", "Trang hai"): ui 2, two pages, +9 = 1, one answer = the callback
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "talk", A, 0));
    {
        const auto acts = actions(dw.w.take_outbox(), 7);
        REQUIRE(acts.size() == 1);
        CHECK(acts[0].ui_id() == jx::zone::ui_talk_dialog);
        CHECK(acts[0].param() == 1);
        REQUIRE(acts[0].options_size() == 2);
        CHECK(acts[0].options(0) == "Trang mot");
        CHECK(acts[0].options(1) == "Trang hai");
        CHECK(A.player.dialog.available_answers == 1);
        CHECK(A.player.dialog.waiting);
        CHECK(A.player.dialog.answer_fun[0] == "OnTalkDone");
    }
    REQUIRE(dw.w.dialog_answer(7, 0, 0));
    CHECK(dw.last() == 300.0);
    // Talk(1, "", 42): a number page printed with "%d", no callback, +9 = 0, nothing to answer
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "talk_plain", A, 0));
    {
        const auto acts = actions(dw.w.take_outbox(), 7);
        REQUIRE(acts.size() == 1);
        CHECK(acts[0].ui_id() == jx::zone::ui_talk_dialog);
        CHECK(acts[0].param() == 0);
        REQUIRE(acts[0].options_size() == 1);
        CHECK(acts[0].options(0) == "42");
        CHECK(A.player.dialog.available_answers == 0);
        CHECK_FALSE(A.player.dialog.waiting);
    }
    CHECK_FALSE(dw.w.dialog_answer(7, 0, 0));
    // Say(7, 2, {..}): a string-table id and the table form
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "say_table", A, 0));
    {
        const auto acts = actions(dw.w.take_outbox(), 7);
        REQUIRE(acts.size() == 1);
        CHECK(acts[0].ui_id() == jx::zone::ui_select_dialog);
        CHECK(acts[0].text().empty());
        CHECK(acts[0].text_id() == 7);
        REQUIRE(acts[0].options_size() == 2);
        CHECK(acts[0].options(0) == "Bon");
        CHECK(acts[0].options(1) == "Nam");
        CHECK(A.player.dialog.answer_fun[0] == "OnFour");
        CHECK(A.player.dialog.answer_fun[1] == "main");
    }
    REQUIRE(dw.w.dialog_answer(7, 0, 0));
    CHECK(dw.last() == 400.0);
    // Say("Chao", 0): a sentence without answers - nothing waits (0x0812401F)
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "say_none", A, 0));
    {
        const auto acts = actions(dw.w.take_outbox(), 7);
        REQUIRE(acts.size() == 1);
        CHECK(acts[0].text() == "Chao");
        CHECK(acts[0].options_size() == 0);
        CHECK(A.player.dialog.available_answers == 0);
        CHECK_FALSE(A.player.dialog.waiting);
    }
}

TEST_CASE("the script bytes reach the client as UTF-8: TCVN3 Vietnamese, GBK Chinese, both on one line", "[dialog][text]")
{
    using namespace jx::zone::text;
    // "Con gái của" in TCVN3 (the bytes of the shipped scripts)
    CHECK(decode_mixed("Con g\xb8i c\xf1" "a") == "Con g\xc3\xa1i c\xe1\xbb\xa7" "a");
    CHECK(decode_mixed("plain ascii / OnCancel") == "plain ascii / OnCancel");
    CHECK(is_tcvn3_loose("Xin h\xb7y ch\xe4n"));
    CHECK_FALSE(is_tcvn3_loose("\xb7\xef\xcf\xe8"));   // GBK 凤翔: 0xef / 0xcf / 0xe8 are letters but 0xb7 0xef 0xcf 0xe8 stacks four
#ifdef _WIN32
    CHECK(decode_mixed("\xb7\xef\xcf\xe8") == "\xe5\x87\xa4\xe7\xbf\x94");   // 凤翔
    // a Vietnamese answer and a Chinese comment on one line, cut at the '-'
    CHECK(decode_mixed("Ph\xb6i - \xb7\xef\xcf\xe8") == "Ph\xe1\xba\xa3i - \xe5\x87\xa4\xe7\xbf\x94");
#endif
    CHECK(tcvn3_rune(0xb8) == U'\u00e1');
    CHECK(tcvn3_rune('a') == U'a');
    CHECK(tcvn3_rune(0xb0) == 0);
}


TEST_CASE("Describe 0x081242A0: Say's shape with the ui id 12, the answers budgeted at 0x1f4 bytes and cut to 0xc8", "[dialog][world]")
{
    DialogWorld dw;
    // the string form: three answers, the functions kept, the 0x63 packet with ui 12
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe", dw.A(), 0));
    auto acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].ui_id() == jx::zone::ui_describe_dialog);
    CHECK(acts[0].text() == "Mo ta");
    CHECK(acts[0].text_id() == 0);
    CHECK(acts[0].interactive());
    CHECK(acts[0].param() == -1);
    REQUIRE(acts[0].options_size() == 3);
    CHECK(acts[0].options(0) == "Mot");
    CHECK(acts[0].options(2) == "Ba");
    CHECK(dw.A().player.dialog.available_answers == 3);
    CHECK(dw.A().player.dialog.waiting);
    CHECK(dw.A().player.dialog.answer_fun[0] == "OnOne");
    CHECK(dw.A().player.dialog.answer_fun[1] == "main");
    CHECK(dw.A().player.dialog.answer_fun[2] == "#g_last = 500");
    // the answer comes back the same way as Say's (the 0x5f packet, kind 0)
    CHECK(dw.w.dialog_answer(7, 2, 0));
    CHECK(dw.last() == 500.0);
    CHECK_FALSE(dw.A().player.dialog.waiting);
    // the table form with a string-table id: +6 = 1, the first answer counts its separator too
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_table", dw.A(), 0));
    acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].ui_id() == 12);
    CHECK(acts[0].text_id() == 9);
    REQUIRE(acts[0].options_size() == 2);
    CHECK(acts[0].options(0) == "Bon");
    CHECK(dw.A().player.dialog.answer_fun[0] == "OnFour");
    CHECK(dw.w.dialog_answer(7, 0, 0));
    CHECK(dw.last() == 400.0);
    // a count that is not a number: the binary prints and sends nothing (0x08124315)
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_nocount", dw.A(), 0));
    CHECK(actions(dw.w.take_outbox(), 7).empty());
    // a count above 0 without any answer: nothing (0x08124460)
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_count_only", dw.A(), 0));
    CHECK(actions(dw.w.take_outbox(), 7).empty());
    // no answer wanted: the sentence alone, nothing waits
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_none", dw.A(), 0));
    acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].options_size() == 0);
    CHECK_FALSE(dw.A().player.dialog.waiting);
    CHECK(dw.A().player.dialog.available_answers == 0);
    // three answers of 250 bytes: each cut to 200 (0x081245A3); 200, then 200 + 3 + 200 = 403 fit in 0x1f4, the third
    // (403 + 3 + 200) does not (0x081245BB) - two kept
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_long", dw.A(), 0));
    acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    REQUIRE(acts[0].options_size() == 2);
    CHECK(acts[0].options(0).size() == jx::zone::kDescribeAnswerMax);
    CHECK(acts[0].options(1).size() == jx::zone::kDescribeAnswerMax);
    CHECK(dw.A().player.dialog.available_answers == 2);
    CHECK(dw.A().player.dialog.answer_fun[0] == "OnOne");
    // a count above the answers given: clamped to them (0x08124828: count >= n - 1 -> n - 2)
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "describe_short", dw.A(), 0));
    acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].options_size() == 2);
    CHECK(jx::zone::kDescribeContentMax == 0x1f4);
    CHECK(jx::zone::kDescribeAnswerMax == 0xc8);
    CHECK(jx::zone::kTaskTipMax == 0x3e);
}

TEST_CASE("AddNote 0x08124DC0: the 0x63 packet with the ui id 3, the text or a string-table id and the number after it", "[dialog][world]")
{
    DialogWorld dw;
    REQUIRE(dw.w.execute_script(R"(\script\test\npc.lua)", "notes", dw.A(), 0));
    const auto acts = actions(dw.w.take_outbox(), 7);
    REQUIRE(acts.size() == 3);   // no argument and a table send nothing (0x08124DEF / 0x08124E1A)
    CHECK(acts[0].ui_id() == jx::zone::ui_note_info);
    CHECK(acts[0].text() == "Dai hiep da thu thap du Hong Moc.");
    CHECK(acts[0].text_id() == 0);
    CHECK(acts[0].param() == 7);
    CHECK(acts[0].interactive());
    CHECK(acts[0].options_size() == 0);
    CHECK(acts[1].ui_id() == 3);
    CHECK(acts[1].text_id() == 1234);
    CHECK(acts[1].param() == 0);
    CHECK(acts[2].text() == "Khong so");
    CHECK(acts[2].param() == 0);
    // the journal is the client's: nothing waits on the zone (m_bWaitingPlayerFeedBack untouched, 0x08124DC0 sets none)
    CHECK_FALSE(dw.A().player.dialog.waiting);
}
