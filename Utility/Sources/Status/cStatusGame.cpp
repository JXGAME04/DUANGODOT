`// cStatusGame.cpp: implementation of the cStatusGame class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusGame.h"
#include "fileop.h"

#include "iClient.h"
#include "iInterface.h"
#include "iAppClient.h"

#include "..\AppCommon\commonMsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
UINT g_HandleNetMessage(stMsg* pMsg,DWORD size, void* p,iServerLink* pLink);
UINT g_HandleServerMessage(stMsg* pMsg,DWORD size, void* p,iServerLink* pLink);

//iClient* cStatusGame::m_pClient = NULL;
//iAppClient* cStatusGame::m_pAppClient = NULL;

cStatusGame::cStatusGame()
{
	m_pClient = NULL;
	m_pAppClient = NULL;
	SetGameStatus(GameStatus_Clear);
}

cStatusGame::~cStatusGame()
{
//	QuitGame();
	if (m_pAppClient != NULL)
	{
		if (m_pClient->IsConnect())
		{
			stMsg_c2s_Quit msg;
			m_pClient->SendMessage((char*)&msg,sizeof(msg));
		}
	}
	Clear();
}

BOOL cStatusGame::Create()
{
	if (!AfxSocketInit())
	{
		OutputMsg(OUTPUT_FAILURE,"error init socket");
		return FALSE;
	}
	return Start();
}

BOOL cStatusGame::Start()
{
//	m_pClient = Net_CreateClient();
	VERIFY(m_pClient->InitSocket());
	VERIFY(m_pClient->Create());
	m_pClient->BindPlayer(this);
	m_pClient->RegisterNetHandle(g_HandleNetMessage);
	m_pClient->RegisterServerHandle(g_HandleServerMessage);

//	m_pInterface = Interface_CreateInterface(interface_normal);

	return OnConnect();
}

void cStatusGame::Clear()
{
	if (m_pAppClient)
	{
		m_pAppClient->End();
		m_pAppClient = NULL;
	}

	if (m_pClient)
	{
		m_pClient->CloseClient();
		m_pClient->CleanSocket();
		RELEASE(m_pClient);
	}

	RELEASE(m_pInterface);

	SetGameStatus(GameStatus_Clear);
}

BOOL cStatusGame::OnTimer(UINT nIDEvent)
{
	switch (GetGameStatus())
	{
	case GameStatus_Connect:
	case GameStatus_ConnectFailed:			
	case GameStatus_AccountVerifyBack:
		OnConnect();
		break;
	case GameStatus_AccountVerify:
	case GameStatus_AccountVerifyFailed:
	case GameStatus_ChoosePlayerBack:
		OnInputAccount();
		break;
	case GameStatus_ChoosePlayer:
	case GameStatus_ChoosePlayerFailed:
		OnChoosePlayer();
		break;
	case GameStatus_ServerDown:
	case GameStatus_AccountGrab:
		Clear();
		break;
	}

	if (m_pAppClient)
		m_pAppClient->Run();
	cStatus::OnTimer(nIDEvent);
	return FALSE;
}

BOOL cStatusGame::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (GetGameStatus() == GameStatus_Play)
	{
		if (!m_pInterface->OnLButtonDown(GetFlag(),point))
			m_pAppClient->OnLButtonDown(GetFlag(),point);
	}
	return FALSE;
}

BOOL cStatusGame::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (GetGameStatus() == GameStatus_Play)
	{
		if (!m_pInterface->OnRButtonDown(GetFlag(),point))
			m_pAppClient->OnRButtonDown(GetFlag(),point);
	}
	return FALSE;
}

UINT cStatusGame::GetFlag()
{			
	UINT flag = 0;
	if (IsAltDown())
		flag |= Alt_Bit;
	if (IsCtrlDown())
		flag |= Ctrl_Bit;
	if (IsShiftDown())
		flag |= Shift_Bit;
	return flag;
}

BOOL cStatusGame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (GetGameStatus() == GameStatus_Play)
	{
		if (!m_pInterface->OnKeyDown(nChar,nRepCnt,GetFlag()))
			m_pAppClient->OnKeyDown(nChar,nRepCnt,GetFlag());
	}

	switch (GetGameStatus())
	{
	case GameStatus_NotConnect:
		switch (nChar)
		{
		case 'C':
			OnConnect();
			break;
		}
		break;
	case GameStatus_Clear:
		switch (nChar)
		{
		case 'C':
			Start();
			break;
		}
		break;
	}
	return FALSE;
}


UINT cStatusGame::HandleNetMessage(stMsg* p,DWORD size,iServerLink* pLink)
{
	switch (p->GetType())
	{
	case MsgIdle:
		break;
	case Msg_Net_ServerConnect:
		{
			stMsg_Net_ServerConnect* pMsg = (stMsg_Net_ServerConnect*)p;
			if (pMsg->bConnect == TRUE)
			{
				OutputMsg(OUTPUT_SUCCEED,"system: success connect to server!");
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,TRUE);			
				EnableNetStatus(NetStatus_Quit,FALSE);			
				EnableNetStatus(NetStatus_Send,FALSE);		
				//auto send name and password!
				SetGameStatus(GameStatus_AccountVerify);
			}
			else
			{
				OutputMsg(OUTPUT_FAILURE,"system: can not connect to server!");
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,FALSE);			
				EnableNetStatus(NetStatus_Quit,FALSE);			
				EnableNetStatus(NetStatus_Send,FALSE);
				
				SetGameStatus(GameStatus_ConnectFailed);
			}
		}
		break;
	case Msg_Net_ServerDown:
		{
			OutputMsg(OUTPUT_FAILURE,"system: disconnect from server!");
			EnableNetStatus(NetStatus_Connect,TRUE);			
			EnableNetStatus(NetStatus_Verify,FALSE);			
			EnableNetStatus(NetStatus_Quit,FALSE);			
			EnableNetStatus(NetStatus_Send,FALSE);
			
			SetGameStatus(GameStatus_ServerDown);
		}
		break;
	}
	return 0;
}

UINT cStatusGame::HandleServerMessage(stMsg* p,DWORD size,iServerLink* pLink)
{
	switch (p->GetType())
	{
	case Msg_s2c_ChoosePlayer:
		m_msgChoosePlayer = *((stMsg_s2c_ChoosePlayer*) p);
		SetGameStatus(GameStatus_ChoosePlayer);
		//can not call dlg domodal here!
		break;
	case Msg_s2c_StartGame:
		OnStartGame();		
		break;
	case Msg_s2c_Account:
		{
			stMsg_s2c_Account* pMsg = (stMsg_s2c_Account*)p;
			if(pMsg->lResult >= 0)
			{
				if (pMsg->lResult == 0)
					OutputMsg(OUTPUT_OK,"user: password is ok!");
				else
				{
					OutputMsg(OUTPUT_OK,"user: reconnect to server!");
					OnStartGame();
					SetGameStatus(GameStatus_Reconnected);
				}
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,FALSE);			
				EnableNetStatus(NetStatus_Quit,TRUE);			
				EnableNetStatus(NetStatus_Send,TRUE);			
				//auto ask for map and charactor info!
			}	
			else
			{
				if (pMsg->lResult == -1)
				{
					OutputMsg(OUTPUT_ERROR,"user: password is invalid!");
					SetGameStatus(GameStatus_AccountVerifyFailed);
				}
				else if (pMsg->lResult == -2)
				{
					OutputMsg(OUTPUT_ERROR,"user: grab by another ip!");
					SetGameStatus(GameStatus_AccountGrab);
				}
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,TRUE);
				EnableNetStatus(NetStatus_Quit,FALSE);
				EnableNetStatus(NetStatus_Send,FALSE);
			}
		}
		break;
	case Msg_s2c_Chat_Broadcast:
		{
			stMsg_s2c_Chat_Broadcast* pMsg = (stMsg_s2c_Chat_Broadcast* )p;
			CString sMsg;
			sMsg.Format("%s say: %s",pMsg->szName,pMsg->szChat);
			OutputMsg(OUTPUT_CHAT,sMsg);
		}
		break;
	case Msg_s2c_SystemInfo:
		{
		}
		break;
	case Msg_s2c_Chat:
		{
			stMsg_s2c_Chat* pMsg = (stMsg_s2c_Chat* )p;
			if (pMsg->lResult == -1)
				OutputMsg(OUTPUT_ERROR,"not found the player!");
		}
		break;
	case Msg_s2c_PlayerInfo:
		SetGameStatus(GameStatus_Play);
	default:
		if (m_pAppClient)
			m_pAppClient->PushMessage(p,size,NULL,pLink);
		break;
	}
	return 0;
}

UINT g_HandleNetMessage(stMsg* p,DWORD size, void* pVoid,iServerLink* pLink)
{
	//if the player can solve the msg, solve it!
	//else world solve it!
	ASSERT(pVoid);
	cStatusGame* pStatus = (cStatusGame* )pVoid;
	return pStatus->HandleNetMessage(p,size,pLink);
}

UINT g_HandleServerMessage(stMsg* p,DWORD size, void* pVoid,iServerLink* pLink)
{
	//if the player can solve the msg, solve it!
	//else world solve it!
	ASSERT(pVoid);
	cStatusGame* pStatus = (cStatusGame* )pVoid;
	return pStatus->HandleServerMessage(p,size,pLink);
}

//dlg function

BOOL cStatusGame::OnConnect()
{
	SetGameStatus(GameStatus_NotConnect);

	//dlg 0;
	iConnectDlg* p = NULL;
	m_pInterface->CreateDlg(dlg_connect,&p);
	ASSERT(p);
	wrap_ptr<iConnectDlg> ptr(p);
	p->SetServerIP("qqq");
	p->SetServerPort(c_nPort);

	if (p->domodal() != IDOK)
		return FALSE;

	std::string s;
	p->GetServerIP(s);
	WORD port;
	p->GetServerPort(port);

	stServerInfo info;
	#pragma message (ATTENTION("load a ini for server and port?"))
	strcpy(info.strServerIP,s.c_str());
	info.wServerPort = c_nPort;
	//because it is async so return always true!
	VERIFY(m_pClient->Connect(info));

	SetGameStatus(GameStatus_Connecting);

	return TRUE;	
}

BOOL cStatusGame::OnInputAccount()
{
	//dlg 0;
	iInputAccountDlg* p = NULL;
	m_pInterface->CreateDlg(dlg_inputaccount,&p);
	ASSERT(p);
	wrap_ptr<iInputAccountDlg> ptr(p);

	std::string str;
	CString sName;
	AppGetHostName(sName);
	str = sName;
	p->SetAccount(str);

	if (p->domodal() != IDOK)
	{
		SetGameStatus(GameStatus_Connect);
		return FALSE;
	}

	stMsg_c2s_Account msg;
	p->GetAccount(str);
	strcpy(msg.szAccount,str.c_str());
	p->GetPassword(str);
	strcpy(msg.szPassword,str.c_str());
	
	m_pClient->SendMessage((char*)&msg,sizeof(msg));

	SetGameStatus(GameStatus_AccountVerifying);
	
	return TRUE;
}

BOOL cStatusGame::OnChoosePlayer()
{
	iSelectPlayerDlg* p = NULL;
	m_pInterface->CreateDlg(dlg_selectplayer,&p);
	ASSERT(p);
	wrap_ptr<iSelectPlayerDlg> ptr(p);

	std::string str;
	for (int i=0; i<m_msgChoosePlayer.nPlayer; i++ )
	{
		str = m_msgChoosePlayer.aInfo[i].szName;
		p->AddPlayerName(str);
	}

	if (p->domodal() != IDOK)
	{
		SetGameStatus(GameStatus_ChoosePlayerBack);
		return FALSE;
	}

	p->AddPlayerName(str);

	stMsg_c2s_ChoosePlayer msg;
	strcpy(msg.szAccount,m_msgChoosePlayer.szAccount);
	strcpy(msg.szName,str.c_str());
	
	m_pClient->SendMessage((char*)&msg,sizeof(msg));

	SetGameStatus(GameStatus_ChoosePlayering);

	OutputMsg(OUTPUT_OK,"user: choose player ok, wait for info of the world!");

	//create appclient
	return TRUE;
}

BOOL cStatusGame::OnStartGame()
{
	if (m_pAppClient == NULL)
	{
//		m_pAppClient = AppClient_GetAppClient();
		m_pAppClient->Create(m_pClient,m_pGraphics);
		m_pAppClient->Begin();
	}
	return TRUE;
}
