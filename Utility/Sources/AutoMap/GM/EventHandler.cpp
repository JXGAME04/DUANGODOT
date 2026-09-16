//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	EventHandler.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 14:53:10
//	文件说明		:	同 jxgm 模块挂接的事件相应模块实现，根据系统规则，
//						本文件中的这些代码同 GM.exe 不是运行在同一个线程内
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EventHandler.h"
#include "GMCore.h"
#include "GMMessages.h"

////////////////////////////////////////////////////////////////
//(class KSystemEventHandler Implement

KSystemEventHandler::KSystemEventHandler(EXCHANGEDATA_SYS* pData)
{
	ASSERT(pData);
	m_pExgSys = pData;
}

KSystemEventHandler::~KSystemEventHandler()
{
}

void STDMETHODCALLTYPE KSystemEventHandler::GWLogin(char* szGWName, DWORD nIP)
{
	GAMEWORLD gw(szGWName, nIP);
	m_pExgSys->aryGameWorlds.push_back(gw);

	//post message to dialog
	{
		CString strOut(szGWName);
		::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_GW_LOGIN, 0, (LONG)strOut.AllocSysString());
	}
}

void STDMETHODCALLTYPE KSystemEventHandler::GWSubLogout(DWORD nGWIP, DWORD nSubIP)
{
	for (int i = 0, nSize = (int)m_pExgSys->aryGameWorlds.size(); i < nSize; i++)
	{
		const GAMEWORLD& gw = m_pExgSys->aryGameWorlds[i];
		if (nGWIP == gw.dwIP)
		{
			//GameWorld found
			::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_SUB_LOGOUT, nSubIP, (LONG)gw.strGameWorldName.AllocSysString());
			return;
		}
	}
	ASSERT(0);	//not login ???
}

void STDMETHODCALLTYPE KSystemEventHandler::GWLogout(char* szGWName, DWORD nIP)
{
	for (int i = 0, nSize = (int)m_pExgSys->aryGameWorlds.size(); i < nSize; i++)
	{
		const GAMEWORLD& gw = m_pExgSys->aryGameWorlds[i];
		if (szGWName == gw.strGameWorldName && nIP == gw.dwIP)
		{
			//GameWorld found
			m_pExgSys->aryGameWorlds.erase(m_pExgSys->aryGameWorlds.begin() + i);

			//post message to dialog
			{
				CString strOut(szGWName);
				::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_GW_LOGOFF, 0, (LONG)strOut.AllocSysString());
			}
			return;
		}
	}
	ASSERT(0);	//not login ???
}

void STDMETHODCALLTYPE KSystemEventHandler::OnlinePlayerCount(char* szGWName, DWORD nCount)
{
	if (szGWName && szGWName[0])
	{
		//gameworld count
		for (int i = 0, nSize = (int)m_pExgSys->aryGameWorlds.size(); i < nSize; i++)
		{
			GAMEWORLD& gw = m_pExgSys->aryGameWorlds[i];
			if (szGWName == gw.strGameWorldName)// && nIP == gw.dwIP)
			{
				//GameWorld found
				gw.nOnlinePlayerCount = nCount;
				break;
			}
		}
		ASSERT(i < nSize);	//GameWorld not login ???
	}
	else //total count
		m_pExgSys->nOnlineCount = nCount;
}

void STDMETHODCALLTYPE KSystemEventHandler::RegisterUserCount(DWORD nCount)
{
	m_pExgSys->nRegisterCount = nCount;
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerAccountAtGWID(char* szAccount, DWORD dwRelayID)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetPlayerAtGWDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::PlayerAccountAtGWID(...) called for no reason.\n", __FILE__, __LINE__);
#endif

	if (szAccount == m_pExgSys->strAccount)
	{
		m_pExgSys->dwRelayIDIncludePlayer = dwRelayID;
		return;
	}
	TRACE("%s(%u): PlayerAccountAtGWID info returned for NO USE.\n", __FILE__, __LINE__);
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerRoleList(char* szAccount, WORD wRoleCount, void* p)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetRoleListDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): IGWEvent::PlayerRoleList(...) called for no reason.\n", __FILE__, __LINE__);
#endif

#pragma	pack(push, 1)
typedef struct
{
	char	szName[32];
	BYTE	Sex;
	BYTE	Series;
//	BYTE	HelmType;
//	BYTE	ArmorType;
//	BYTE	WeaponType;
	BYTE	Level;
} RoleBaseInfo/* client */, S3DBI_RoleBaseInfo /* server */;
#pragma pack(pop)

	RoleBaseInfo* pRoleList = (RoleBaseInfo*)p;
	m_pExgSys->lstRoleName.RemoveAll();
	for (int i = 0; i < MAX_PLAYER_PER_ACCOUNT; i++)
	{
		if (pRoleList[i].szName[0])
		{
			m_pExgSys->lstRoleName.AddTail(pRoleList[i].szName);
			if (m_pExgSys->lstRoleName.GetCount() >= wRoleCount)
				break;
		}
	}

	ASSERT(wRoleCount >= m_pExgSys->lstRoleName.GetCount());
	SetEvent(m_pExgSys->hEventGetRoleListDone);
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerAccountAtGW(char* szAccount, DWORD nIP,
															  int nX, int nY, int nSubWorld,
															  char* szCurRole)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetPlayerAtGWDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::PlayerAccountAtGW(...) called for no reason.\n", __FILE__, __LINE__);
#endif

	if (szAccount == m_pExgSys->strAccount)
	{
		m_pExgSys->dwIPGWIncludePlayer = nIP;
		if (szCurRole)
			m_pExgSys->strCurrentRole = szCurRole;
		m_pExgSys->nCurX = nX;
		m_pExgSys->nCurY = nY;
		m_pExgSys->nCurWorldID = nSubWorld;
		::SetEvent(m_pExgSys->hEventGetPlayerAtGWDone);
		return;
	}

	//天牢处理
	if (szAccount && strlen(szAccount))
	{
		POSITION pos = ::FindMyString(m_pExgSys->lstJailWaitingForFree, szAccount);
		if (pos)
		{
			if (nIP != 0 && nIP != 0xFFFFFFFF)
			{
				CString strOut;
				if (nSubWorld != KGameMasterCore::s_nJailWorldID)
				{
					strOut.Format("%s\nds NewWorld(%d, %d, %d)",
									KGameMasterCore::s_nJailWorldID,
									KGameMasterCore::s_nJailX,
									KGameMasterCore::s_nJailY);
					::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_DO_SCRIPT, 0, (LONG)strOut.AllocSysString());
				}
				else
				{
					strOut = m_pExgSys->lstJailWaitingForFree.GetAt(pos);
					ASSERT(strOut.GetLength());
					int nSplit = strOut.Find('\n');
					int nSplit1 = strOut.Find('\n', nSplit + 1);
					CString strRole = strOut.Mid(nSplit + 1, nSplit1 - nSplit - 1), strGWName;
					for (int i = 0, nSize = (int)m_pExgSys->aryGameWorlds.size(); i < nSize; i++)
					{
						const GAMEWORLD& gw = m_pExgSys->aryGameWorlds[i];
						if (nIP == gw.dwIP)
						{
							//GameWorld found
							strGWName = gw.strGameWorldName;
							break;
						}
					}
					ASSERT(i < nSize);
					
					if (strRole == szCurRole && strGWName == strOut.Mid(nSplit1 + 1))
						::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_JAIL_JUDGE, 0, (LONG)strOut.AllocSysString());
				}
			}
			m_pExgSys->lstJailWaitingForFree.RemoveAt(pos);
			return;
		}
	}

	TRACE("%s(%u): PlayerAccountAtGW info returned for NO USE.\n", __FILE__, __LINE__);
}

void STDMETHODCALLTYPE KSystemEventHandler::Ping(DWORD nTick)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventPingDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::Ping(DWORD) called for no return.\n", __FILE__, __LINE__);
#endif

	m_pExgSys->nPing = nTick;
	::SetEvent(m_pExgSys->hEventPingDone);
	::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_SYS_PING, 0, 0);
}

void STDMETHODCALLTYPE KSystemEventHandler::GameWorldRelayID(char* szGWName, DWORD nRelayID)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetGWRelayIDDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::GameWorldRelayID(\"%s\", ...) called for no return.\n", __FILE__, __LINE__, szGWName);
#endif

	m_pExgSys->dwRelayIDOfGameWorld = nRelayID;
	::SetEvent(m_pExgSys->hEventGetGWRelayIDDone);
}

void STDMETHODCALLTYPE KSystemEventHandler::ConnectClosed()
{
	::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_SYS_DISCONNCECT, 0, 0);
}
//)class KSystemEventHandler Implement End
////////////////////////////////////////////////////////////////



void STDMETHODCALLTYPE KSystemEventHandler::PlayerTrackingData(char* szAccount, void* pData, size_t dataLength)
{
	ASSERT(0);//todo...database link
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetPlayerInfoDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): IGWEvent::PlayerInfo(...) called for no reason.\n", __FILE__, __LINE__);
#endif
	switch (ninfo)
	{
	case gm_role_entergame_position:
		ASSERT(dataLength == sizeof(m_pExgSys->posInfo));
		memcpy(&m_pExgSys->posInfo, pData, sizeof(m_pExgSys->posInfo));
		break;
	default:
		ASSERT(0);
		break;
	}
	SetEvent(m_pExgSys->hEventGetPlayerInfoDone);
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerMessage(char* szAccountFrom, char* szRole, char* szMessage,
														  int nMsgLen, DWORD dwGWIP)
{
	if (nMsgLen <= 0)
		return;
	CString strOK = szAccountFrom;
	strOK += "\n";
	strOK += szRole;
	strOK += "\n";
	for (int i = 0, nSize = (int)m_pExgSys->aryGameWorlds.size(); i < nSize; i++)
	{
		const GAMEWORLD& gw = m_pExgSys->aryGameWorlds[i];
		if (dwGWIP == gw.dwIP)
		{
			//GameWorld found
			strOK += gw.strGameWorldName;
			strOK += "\n";
			break;
		}
	}
	if (i >= nSize)
	{
		ASSERT(0);
		return;
	}

	CString strMsg(szMessage, nMsgLen);
	strOK += strMsg;
	::PostThreadMessage((DWORD)_Module.dwThreadID, WM_JX_CHATING, 0, (LONG)strOK.AllocSysString());
}

