#pragma once
// KPlayerMenuState (Core/Src/KPlayerMenuState.h, 2002; jx_linux_y Player+0x5700) and KTrade (Core/Src/KPlayerTrade.h, 2003;
// Player+0x5910): the sign over a player's head and the trade with another player.  docs/LINUX-SERVER.md §18.
#include <cstddef>
#include <cstdint>
#include <string>

namespace jx::zone {

// PLAYER_MENU_STATE_* of KPlayerMenuState.h
enum KMenuState : int {
    menu_state_normal = 0,
    menu_state_team_open = 1,    // KTeam::SetTeamOpen 0x080CD960: looking for team mates
    menu_state_trade_open = 2,   // KPlayer::TradeApplyOpen 0x080AE590: open for a trade, with a sentence
    menu_state_trading = 3,      // c2sTradeReplyStart 0x080BAFD0: in a trade
    menu_state_idle = 4,
    menu_state_num = 5,
};

inline constexpr std::size_t kMenuSentenceMax = 255;   // MAX_SENTENCE_LENGTH - 1 (0x080AE605: longer sentences are cut)
inline constexpr std::size_t kMenuSyncSentenceMax = 0x1e;   // the full sync 0x4c copies that much of the sentence (0x0807FE32)

// what the others see over the head (m_cMenuState; SetState 0x080C29D0 keeps the state before as the backup, which a
// cancelled trade restores through 0x080C2ED0)
struct KPlayerMenuState {
    int state = menu_state_normal;   // +0x5700 m_nState
    int back_state = menu_state_normal;
    std::string sentence;            // +0x570c.. m_szSentence (state 2: what the player wrote)
    std::string back_sentence;

    // KPlayerMenuState::Release
    void release()
    {
        state = menu_state_normal;
        back_state = menu_state_normal;
        sentence.clear();
        back_sentence.clear();
    }
};

// KTrade of the server (Player+0x5910; KTrade::Release 0x080D82D0 / StartTrade 0x080D8300): who one trades with, whether the
// ok button and the lock were pressed, who applied; players are their session ids here (the binary keeps player indices)
struct KTrade {
    std::uint64_t dest = 0;   // +0 m_nTradeDest (0x5910): the partner (0 = none; the binary keeps -1)
    bool ok = false;          // +4 m_nTradeState (0x5914): the ok button pressed
    bool locked = false;      // +8 m_nTradeLock (0x5918): the table locked
    std::uint64_t apply = 0;  // +0xc m_nApplyIdx (0x591c): who asked to trade with me (TradeApplyStart 0x080B4DE0 sets it on the target)
    bool trading = false;     // +0x10 m_nIsTrading (0x5920): CheckTrading 0x080A7E90 = menu state 3 or this

    // KTrade::Release 0x080D82D0: everything but the applicant
    void release() noexcept
    {
        dest = 0;
        ok = false;
        locked = false;
        trading = false;
    }
    // KTrade::StartTrade 0x080D8300: only when not trading yet
    bool start(std::uint64_t partner) noexcept
    {
        if (trading || partner == 0) return false;
        trading = true;
        dest = partner;
        ok = false;
        locked = false;
        return true;
    }
};

}  // namespace jx::zone
