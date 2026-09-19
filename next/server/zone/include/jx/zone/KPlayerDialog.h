// KPlayerDialog.h - the dialog a script holds with a player: Say (a sentence and up to fifty answers) and Talk (pages).
//
// jx_linux_y: KPlayer::DialogNpc 0x080B1300 (the 0x6e packet {0x6e, dword npc id, dword sync}) runs the npc's script
// `main` for the player; Lua Say 0x08123C90 / Talk 0x08116930 build the 0x63 packet (PLAYER_SCRIPTACTION_SYNC of the
// 2003 KProtocol.h: byte m_nOperateType +3, m_bUIId +4, m_bOptionNum +5, m_bParam1 +6, m_bParam2 +7, int m_nParam +9,
// int m_nBufferLen +0xd, content +0x11 - the sentence, then the answers NUL-separated, at most 0x384 bytes) through
// 0x080A8510, and remember on the player what each answer runs: m_szTaskAnswerFun[50] of 0x80 bytes at Player+0x5fa0
// (the text after the first '/' of an answer, "main" when there is none), m_nAvailableAnswerNum +0x78e4,
// m_bWaitingPlayerFeedBack +0x78e8, the script at +0x5f9c.  The answer comes back as the 0x5f packet {0x5f, int index,
// int kind, int, int} (17 bytes, KPlayer::OnSelectFromUI of the 2.0 client 0x005FC7D0) -> 0x080AC5D0: index below the
// count -> the function of that answer is called with the index (a name starting with '#' is Lua code run instead).
// docs/LINUX-SERVER.md §20.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "jx/ids.hpp"

namespace jx::zone {

inline constexpr int kDialogAnswers = 50;               // MAX_ANSWERNUM (0x08123DB2: min(count, 0x32))
inline constexpr std::size_t kDialogAnswerFunMax = 0x7f; // strncpy of 0x7f into the 0x80 byte slots (0x08123EF1)
inline constexpr std::size_t kDialogContentMax = 0x384;  // the sentence and the answers together (0x08123EC1)
inline constexpr std::size_t kDialogSentenceMax = 0x257; // a Say sentence (0x081240E0); Talk pages 0x384 each
inline constexpr std::size_t kDescribeContentMax = 0x1f4; // Describe 0x081242A0: the answers alone, each + 3 for the "| |" (0x081245BB)
inline constexpr std::size_t kDescribeAnswerMax = 0xc8;   // an answer of a Describe is cut to 200 bytes (0x081245A3 -> 0x0822A0A0)
inline constexpr std::size_t kTaskTipMax = 0x3e;          // TaskTip 0x08122730: 0x40 bytes hold a 0x10 byte, the text and the NUL
inline constexpr int kDialogRadius = 124;               // m_DialogRadius +0x1634: KNpc::Init 0x0807E02D writes 0x7c; the
                                                        // talk reaches twice that (0x080B13C7: dist <= 2 * radius)

// UIInfo of the 2003 KPlayer.h: m_bUIId of the 0x63 packet
enum KDialogUi : int {
    ui_select_dialog = 0,   // Say: the sentence and the answers (KUiMsgSel, 滚动选择界面.ini)
    ui_trade_dialog = 1,
    ui_talk_dialog = 2,     // Talk: the pages (KUiInformation2, 提示2.ini)
    ui_note_info = 3,
    ui_msg_info = 4,
    ui_news_info = 5,
    ui_play_music = 6,
    ui_open_tong_ui = 7,
    ui_describe_dialog = 12,   // Describe: the sentence and the answers in the npc description window (0x006007FD -> ui message
                               // 0x40 -> KUiNpcDescribe 0x00508410, npc描述界面.ini)
};

// m_nOperateType of the 0x63 packet
enum KScriptActionKind : int {
    script_action_ui_show = 0,
    script_action_exe_script = 1,   // the client runs a script of its own ("OnCall") - never sent by the zone
};

struct KPlayerDialog {
    std::string script;                                   // +0x5f9c: the script the answers go back to (a game path here)
    std::array<std::string, kDialogAnswers> answer_fun{}; // m_szTaskAnswerFun +0x5fa0
    int available_answers = 0;                            // m_nAvailableAnswerNum +0x78e4
    bool waiting = false;                                 // m_bWaitingPlayerFeedBack +0x78e8
    EntityId npc;                                         // Player+0xc: the npc talked to (DialogNpc 0x080B140E)

    void clear_answers() noexcept
    {
        for (std::string& f : answer_fun) f.clear();
        available_answers = 0;
    }
};

}   // namespace jx::zone
