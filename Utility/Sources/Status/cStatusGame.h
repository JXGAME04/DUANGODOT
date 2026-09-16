// cStatusGame.h: interface for the cStatusGame class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSGAME_H__5EF8C9C3_074D_472D_9B1C_136E2E209BC1__INCLUDED_)
#define AFX_CSTATUSGAME_H__5EF8C9C3_074D_472D_9B1C_136E2E209BC1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cStatus.h"

class iAppClient;
class iClient;
struct stMsg;
class iServerLink;
class iInterface;

#include "..\AppCommon\commonMsg.h"

class cStatusGame  : public cStatus
{
public:
	cStatusGame();
	virtual ~cStatusGame();

	iClient* m_pClient;
	iAppClient* m_pAppClient;
	BOOL OnStartGame();
	iInterface* m_pInterface;

	BOOL OnConnect();
	BOOL OnInputAccount();
	stMsg_s2c_ChoosePlayer m_msgChoosePlayer;
	BOOL OnChoosePlayer();

	void Clear();
	BOOL Create();
	BOOL Start();
	BOOL OnTimer(UINT nIDEvent);

	UINT GetFlag();
	BOOL OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnRButtonDown(UINT nFlags, CPoint point);
	BOOL OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	enum NetStatusEnum{NetStatus_Connect,NetStatus_Verify,NetStatus_Quit,NetStatus_Send,NetStatus_Num,};
	BOOL m_aEnableStatus[NetStatus_Num];
	enum GameStatusEnum{
		GameStatus_NotConnect,
		GameStatus_Connect,GameStatus_Connecting,GameStatus_ConnectFailed,
		GameStatus_AccountVerify,GameStatus_AccountVerifying,GameStatus_AccountVerifyBack,GameStatus_AccountVerifyFailed,
		GameStatus_ChoosePlayer,GameStatus_ChoosePlayering,GameStatus_ChoosePlayerBack,GameStatus_ChoosePlayerFailed,
		GameStatus_Reconnected,
		GameStatus_ServerDown,GameStatus_AccountGrab,
		GameStatus_Play,
		GameStatus_Clear,	
	};
	GameStatusEnum m_eGameStatus;
	void SetGameStatus(GameStatusEnum e){m_eGameStatus = e;}
	GameStatusEnum GetGameStatus(){return m_eGameStatus;}

	void EnableNetStatus(NetStatusEnum e, BOOL b){m_aEnableStatus[e] = b;}
	UINT HandleNetMessage(stMsg* p,DWORD size,iServerLink* pLink);
	UINT HandleServerMessage(stMsg* p,DWORD size,iServerLink* pLink);
};


#endif // !defined(AFX_CSTATUSGAME_H__5EF8C9C3_074D_472D_9B1C_136E2E209BC1__INCLUDED_)
