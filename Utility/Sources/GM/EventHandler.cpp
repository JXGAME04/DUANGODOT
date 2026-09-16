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

void STDMETHODCALLTYPE KSystemEventHandler::PlayerAccountAtGW(char* szAccount, DWORD nIP)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventGetPlayerAtGWDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::PlayerAccountAtGW(...) called for no reason.\n", __FILE__, __LINE__);
#endif

	if (szAccount == m_pExgSys->strAccount)
	{
		m_pExgSys->dwIPGWIncludePlayer = nIP;
		::SetEvent(m_pExgSys->hEventGetPlayerAtGWDone);
		return;
	}
	TRACE("%s(%u): PlayerAccountAtGW info returned for NO USE.\n", __FILE__, __LINE__);
}

void STDMETHODCALLTYPE KSystemEventHandler::Ping(DWORD nTick)
{
#ifdef _DEBUG
	if (::WaitForSingleObject(m_pExgSys->hEventPingDone, 0) == WAIT_OBJECT_0)
		TRACE("%s(%u): ISystemEvent::Ping(DWORD) called for no reason.\n", __FILE__, __LINE__);
#endif

	m_pExgSys->nPing = nTick;
	::SetEvent(m_pExgSys->hEventPingDone);
}

void STDMETHODCALLTYPE KSystemEventHandler::ConnectClosed()
{
	ASSERT(0);
	//not handled yet
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
	ASSERT(0);//todo...
	SetEvent(m_pExgSys->hEventGetPlayerInfoDone);
}

void STDMETHODCALLTYPE KSystemEventHandler::PlayerMessage(char* szAccountFrom, char* szAccountTo, char* szMessage)
{
	ASSERT(0);//todo...chat processer
}

