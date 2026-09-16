// -------------------------------------------------------------------------
//	文件名		：	KGWConnect.cpp
//	创建者		：	万里
//	创建时间	：	2003-6-2 19:49:23
//	功能描述	：	
//
// -------------------------------------------------------------------------

#include "stdafx.h"

#include "KGWConnect.h"

#include "KProtocolDef.h"
#include "KGMProtocol.h"
#include "KProtocol.h"

KGSConnect::KGSConnect(IGWEvent* pEvent)
{
	m_pEvent = pEvent;

	m_ProtocolTable[gm_s2c_rolechange] = P_PlayerLoginorOut;
	m_ProtocolTable[gm_s2c_tracking] = P_PlayerTrackingData;
	m_ProtocolTable[gm_s2c_getrole] = P_PlayerInfo;
}

KGSConnect::~KGSConnect()
{
}

void KGSConnect::P_PlayerLoginorOut(void* pThis, const void *pData, size_t dataLength)
{
	KGSConnect* p = (KGSConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	GM_ROLE_CHANGE_SYNC *pNPL = (GM_ROLE_CHANGE_SYNC *)((char*)pData - 1);
	dataLength++;
	if (pNPL && dataLength == sizeof(GM_ROLE_CHANGE_SYNC))
	{
		if (pNPL->m_btLogFlag)
			p->m_pEvent->PlayerLogin(pNPL->m_szAccount, pNPL->m_szRoleName);
		else
			p->m_pEvent->PlayerLogout(pNPL->m_szAccount, pNPL->m_szRoleName);
	}
}

void KGSConnect::P_PlayerTrackingData(void* pThis, const void *pData, size_t dataLength)
{
	KGSConnect* p = (KGSConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	GM_TRACK_SYNC *pNPL = (GM_TRACK_SYNC *)((char*)pData - 1);
	dataLength++;
	if (pNPL && dataLength == sizeof(GM_TRACK_SYNC))
	{
		size_t nOffset = sizeof(pNPL->m_wLength) + sizeof(pNPL->m_szAccount) + sizeof(pNPL->m_szRoleName) + 1;
		p->m_pEvent->PlayerTrackingData(pNPL->m_szAccount, pNPL->m_szRoleName, ((char*)pData) + nOffset, dataLength - nOffset);
	}
}

void KGSConnect::P_PlayerInfo(void* pThis, const void *pData, size_t dataLength)
{
	KGSConnect* p = (KGSConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	GM_GET_ROLE_DATA_SYNC *pNPL = (GM_GET_ROLE_DATA_SYNC *)((char*)pData - 1);
	dataLength++;
	if (pNPL && dataLength == sizeof(GM_GET_ROLE_DATA_SYNC))
	{
		size_t nOffset = sizeof(pNPL->m_wLength) + sizeof(pNPL->m_szAccount) + sizeof(pNPL->m_szRoleName) + sizeof(pNPL->m_wGetID) + 1;
		p->m_pEvent->PlayerInfo(pNPL->m_szAccount, pNPL->m_szRoleName, pNPL->m_wGetID, ((char*)pData) + nOffset, dataLength - nOffset);
	}
}

void KGSConnect::P_PlayerMessage(void* pThis, const void *pData, size_t dataLength)
{
}

void KGSConnect::ConnectClose()
{
	if (m_pEvent)
		m_pEvent->ConnectClosed();
}

/////////////////////////////////////////////////////////////////////////////////

KGWConnect::KGWConnect()
{
	m_hLogined = NULL;
	m_bLogined = FALSE;

}

KGWConnect::~KGWConnect()
{
	if (m_hLogined)
		CloseHandle(m_hLogined);
	m_hLogined = NULL;

	ReleaseRelayConnect();
}

KConnect* KGWConnect::GetRelayConnect()
{
	KConnect* pFind = NULL;
	if (!m_GSConnects.IsLocked())
	{
		pFind = m_GSConnects.m_RelayConnect;
	}
	return pFind;
}

void KGWConnect::ReleaseRelayConnect()
{
	m_GSConnects.Lock();
	if (m_GSConnects.m_RelayConnect)
	{
		m_GSConnects.m_RelayConnect->Stop();
		delete m_GSConnects.m_RelayConnect;
		m_GSConnects.m_RelayConnect = NULL;
	}
	m_GSConnects.Unlock();
}

void KGWConnect::P_Logined(void* pThis, const void *pData, size_t dataLength)
{
}

void KGWConnect::P_Ping(void* pThis, const void *pData, size_t dataLength)
{
}

void KGWConnect::P_PlayerLogin(void* pThis, const void *pData, size_t dataLength)
{
}

void KGWConnect::ConnectClose()
{
	if (m_pEvent)
		m_pEvent->ConnectClosed();
}


HRESULT STDMETHODCALLTYPE KGWConnect::Login(char* szGMAccount, char* szPassword, IGWEvent* pEvent)
{
	HRESULT hr = E_FAIL;
	m_pEvent = pEvent;
	return hr;
}

void STDMETHODCALLTYPE KGWConnect::Close()
{
	Stop();	//first gateway
	ReleaseRelayConnect();	//then relay
}

void STDMETHODCALLTYPE KGWConnect::ExecuteAction(char* szAccount, char* szScript)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	int nszLen = 0;
	if (szScript)
		nszLen = strlen(szScript);
	GM_EXECUTE_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_execute;
	exe.wLength = nszLen;
	exe.wExecuteID = 1;

	pSend->SendRelayData(szAccount, &exe, sizeof(GM_EXECUTE_COMMAND), szScript, nszLen);
}

void STDMETHODCALLTYPE KGWConnect::EnableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	GM_ENABLE_ACTION_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_enable;
	exe.btProtocol = nProtocol;
	exe.wLength = dataLength;

	pSend->SendRelayData(szAccount, &exe, sizeof(GM_ENABLE_ACTION_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KGWConnect::DisableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	GM_DISABLE_ACTION_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_disable;
	exe.btProtocol = nProtocol;
	exe.wLength = dataLength;

	pSend->SendRelayData(szAccount, &exe, sizeof(GM_DISABLE_ACTION_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KGWConnect::TrackingPlayer(char* szAccount, bool bTrack)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	GM_TRACK_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_tracking;
	exe.btTrackFlag = bTrack;

	pSend->SendRelayData(szAccount, &exe, sizeof(GM_TRACK_COMMAND));
}

void STDMETHODCALLTYPE KGWConnect::GetPlayerInfo(char* szAccount, WORD ninfo)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	GM_GET_ROLE_DATA_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_getrole;
	exe.wGetID = ninfo;
	exe.wLength = 0;

	pSend->SendRelayData(szAccount, &exe, sizeof(GM_GET_ROLE_DATA_COMMAND));
}

void STDMETHODCALLTYPE KGWConnect::SetPlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength)
{
	KConnect* pSend = GetRelayConnect();
	if (!pSend)
		return;
	GM_SET_ROLE_DATA_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_setrole;
	exe.wSetID = ninfo;
	exe.wLength = dataLength;
		
	pSend->SendRelayData(szAccount, &exe, sizeof(GM_SET_ROLE_DATA_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KGWConnect::SendMessage(char* szAccount, char* szRole, char* szMessage)
{
}

void STDMETHODCALLTYPE KGWConnect::SendPing()
{
	typedef struct
	{
		BYTE			nFamily;
		BYTE			ProtocolType;
		DWORD			m_dwTime;
	} PING_COMMAND;
	PING_COMMAND PingCmd;
	PingCmd.nFamily = pf_normal;
	PingCmd.ProtocolType = c2s_ping;
	PingCmd.m_dwTime = GetTickCount();
	SendData(&PingCmd, sizeof(PING_COMMAND));
}

void STDMETHODCALLTYPE KGWConnect::ListenChat(bool bOpen)
{
}
