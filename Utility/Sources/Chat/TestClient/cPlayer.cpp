// cPlayer.cpp: implementation of the cPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestClient.h"
#include "cPlayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlayer::cPlayer()
{
	m_eStatus = GameStatus_Invalid;
}

cPlayer::~cPlayer()
{

}

UINT cPlayer::Run()
{
	if (m_eStatus == GameStatus_Verify)
	{
		if (GetTickCount() - m_nTimer > 1000)
		{
			m_nTimer = GetTickCount();
			stMsg_c2s_Chat msg;
			strcpy(msg.szAccount,"");
			strcpy(msg.szChat,m_sPassword);
			m_pLink->SendMessage(&msg,sizeof(msg));
		}
	}
	return 0;
}

UINT cPlayer::HandleMessage(stMsg* p, DWORD msgSize)
{
	switch (p -> GetType())
	{
	case Msg_Net_ServerConnect:
		{
			stMsg_Net_ServerConnect* pMsg = (stMsg_Net_ServerConnect*)p;
			if (pMsg->bConnect == TRUE)
			{
				m_eStatus = GameStatus_Connect;
				stMsg_c2s_Account msg;
				strcpy(msg.szAccount,m_sAccount);
				strcpy(msg.szPassword,m_sPassword);
				m_pLink->SendMessage(&msg,sizeof(msg));
			}
			break;
		}		
	case Msg_Net_ServerDown:
		{
			m_eStatus = GameStatus_Disconnect;
			break;
		}
	case Msg_s2c_Account:
		{
			stMsg_s2c_Account* pMsg = (stMsg_s2c_Account*)p;
			if (pMsg->lResult >= 0)
			{
				m_eStatus = GameStatus_Verify;
				m_nTimer = GetTickCount() + rand() % 1000;
			}
			break;
		}
	}
	return 0;
}

UINT PlayerHandleMessage(stMsg* pMsg, DWORD msgSize, void* pPlayer,iServerLink* pLink)
{
	cPlayer* p = (cPlayer*)pPlayer;
	return p->HandleMessage(pMsg,msgSize);
}
