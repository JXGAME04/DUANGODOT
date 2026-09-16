//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMCore.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 15:02:31
//	文件说明		:	GM.exe 核心类 KGameMasterCore 的实现
//						在这一层实现多线程以及 JXGM.dll 提供模块的完全封装
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMCore.h"
#include "winsock.h"
#include "EventHandler.h"
#include "GMConsts.h"
#include "../GMPassGen/Include/PassGen.h"
#include "KIniFile.h"
#include "../../../SwordOnline/Headers/ServerPort.h"

KGameMasterCore::KGameMasterCore(ISystemOnline* pISO)
{
	m_pISystemOnline	= pISO;
	m_pISystemConnect	= NULL;
	m_pSysHandler		= NULL;

	//(exchange data init
	m_ExgSYS.dwIPGWIncludePlayer		= 0;
	m_ExgSYS.nPing						= MAX_PING;
	m_ExgSYS.nRegisterCount				= 0;
	m_ExgSYS.nOnlineCount				= 0;
	//)

	HRESULT hr = S_OK;
	BOOL br = TRUE;
	DWORD dwSystemIP = 0;
	int nSystemPort = 0;
	int nOnlineRegisterAutoCheckInterval = 0;
	CString strTmp;
	CString strAccount, strPassword;

	//register signals and critical-section
	::InitializeCriticalSection(&m_CS);
	m_ExgSYS.hEventGetPlayerAtGWDone	= ::CreateEvent(NULL, TRUE, TRUE, NULL);
	m_ExgSYS.hEventPingDone				= ::CreateEvent(NULL, TRUE, TRUE, NULL);
	m_ExgSYS.hEventGetPlayerInfoDone	= ::CreateEvent(NULL, TRUE, TRUE, NULL);

	KIniFile ini;
	br = ini.Load(INI_FILENAME);
	TRACE(br ? "" : "No GMConn.ini file....Load default settings.\n");
	//get Pay System connection settings from ini file
	br = ini.GetString("System", "IP",
				  "192.168.20.15",
				  strTmp.GetBuffer(STR_BUFFER_LEN),
				  STR_BUFFER_LEN);
	//KS_CHECK_BOOLEX(br, hr = E_FAIL);
	strTmp.ReleaseBuffer();
	dwSystemIP = inet_addr(strTmp);
	ASSERT(dwSystemIP != INADDR_NONE);

	br = ini.GetInteger("System", "Port", RELAY_ROUTE_PORT, &nSystemPort);
	//KS_CHECK_BOOLEX(br, hr = E_FAIL);

	//open connection for Pay System
	hr = pISO->OpenSystemConnect(dwSystemIP, nSystemPort, &m_pISystemConnect);
	KS_CHECK(hr);

	m_pSysHandler = new KSystemEventHandler(&m_ExgSYS);
	KS_CHECK_BOOLEX(m_pSysHandler, hr = E_FAIL);

	//Get Account and Password from ini file
	br = ini.GetString("Account", "Name",
					   "n2tbsltAwyZyn2hzgABlIR__VQTk9Arc",	//"gm-kingsoft"
					   strTmp.GetBuffer(STR_BUFFER_LEN),
					   STR_BUFFER_LEN);
	//KS_CHECK_BOOLEX(br, hr = E_FAIL);
	strTmp.ReleaseBuffer();
	::SimplyDecryptPassword(strAccount.GetBuffer(PG_MAXBUFFER), strTmp);
	strAccount.ReleaseBuffer();

	br = ini.GetString("Account", "Password",
					   "Rrd0cpp_k1IAxBA3Z_OKA4gtFAmjlmZt",	//"gm-kingsoft"
					   strTmp.GetBuffer(STR_BUFFER_LEN),
					   STR_BUFFER_LEN);
	//KS_CHECK_BOOLEX(br, hr = E_FAIL);
	strTmp.ReleaseBuffer();
	::SimplyDecryptPassword(strPassword.GetBuffer(PG_MAXBUFFER), strTmp);
	strPassword.ReleaseBuffer();

	//GameMaster Login and Event Handlers Junction
	hr = m_pISystemConnect->Login((char*)(const char*)strAccount, (char*)(const char*)strPassword, m_pSysHandler);
	KS_CHECK(hr);

	//PaySystem option setup
	br = ini.GetInteger("System", "AutoCheckInterval", 10000, &nOnlineRegisterAutoCheckInterval);
	//KS_CHECK_BOOLEX(br, hr = E_FAIL);
	m_pISystemConnect->SetOption(TRUE, TRUE, nOnlineRegisterAutoCheckInterval);

	//First get online count
	m_pISystemConnect->GetOnlinePlayerCount("");
	m_pISystemConnect->GetRegisterUserCount();
KS_EXIT:
	;
}

KGameMasterCore::~KGameMasterCore()
{
	if (m_pISystemConnect)
		m_pISystemConnect->Close();
	if (m_ExgSYS.hEventGetPlayerInfoDone)
	{
		::CloseHandle(m_ExgSYS.hEventGetPlayerInfoDone);
		::CloseHandle(m_ExgSYS.hEventGetPlayerAtGWDone);
		::CloseHandle(m_ExgSYS.hEventPingDone);
	}

	if (m_pSysHandler)
		{ delete m_pSysHandler; m_pSysHandler = NULL; } 
	::DeleteCriticalSection(&m_CS);
}

//pay system
int KGameMasterCore::GetOnlineCount(LPCSTR szGWName)
{
	int nOnlineCount = 0, i, nSize;
	if (!m_pISystemConnect)
		{ ASSERT(0); return 0;}

	m_pISystemConnect->GetOnlinePlayerCount((char*)szGWName);

	::EnterCriticalSection(&m_CS);
	if (szGWName && szGWName[0])
	{
		for (i = 0, nSize = (int)m_ExgSYS.aryGameWorlds.size(); i < nSize; i++)
		{
			GAMEWORLD gw = m_ExgSYS.aryGameWorlds[i];
			if (szGWName == gw.strGameWorldName)// && nIP == gw.dwIP)
			{
				//GameWorld found
				nOnlineCount = gw.nOnlinePlayerCount;
				break;
			}
		}
		ASSERT(i < nSize);	//GameWorld not login ???
	}
	else
		nOnlineCount = m_ExgSYS.nOnlineCount;
	::LeaveCriticalSection(&m_CS);

	return nOnlineCount;
}

int KGameMasterCore::GetRegisterCount()
{
	int nRegisterCount = 0;
	if (!m_pISystemConnect)
		{ ASSERT(0); return 0;}

	m_pISystemConnect->GetRegisterUserCount();

	::EnterCriticalSection(&m_CS);
	nRegisterCount = m_ExgSYS.nRegisterCount;
	::LeaveCriticalSection(&m_CS);

	return nRegisterCount;
}

HRESULT KGameMasterCore::WhichGWIsPlayerIn(GAMEWORLD& gw, LPCSTR szAccount)
{
	HRESULT hr = S_OK;
	int i, nSize;

	KS_CHECK_BOOLEX(m_pISystemConnect, hr = E_FAIL);

	::EnterCriticalSection(&m_CS);
	ASSERT(m_ExgSYS.strAccount.IsEmpty());
	m_ExgSYS.strAccount = szAccount;
	::LeaveCriticalSection(&m_CS);

	::ResetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
	m_pISystemConnect->GetPlayerAccountAtGW((char*)szAccount);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
		TRACE("%s(%u): KGameMasterCore::WhichGWIsPlayerIn(..., szAccountName == \"%s\") time out.\n", __FILE__, __LINE__, szAccount);
		hr = E_FAIL;
		goto KS_EXIT;
	}

	::EnterCriticalSection(&m_CS);
	m_ExgSYS.strAccount.Empty();
	for (i = 0, nSize = (int)m_ExgSYS.aryGameWorlds.size(); i < nSize; i++)
	{
		GAMEWORLD gwHere = m_ExgSYS.aryGameWorlds[i];
		if (m_ExgSYS.dwIPGWIncludePlayer == gwHere.dwIP)
		{
			//GameWorld found
			gw = gwHere;
			break;
		}
	}
	ASSERT(i < nSize);	//GameWorld not login ???
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return hr;
}

HRESULT KGameMasterCore::UnlockPlayerAccount(LPCSTR szName, LPCSTR szPassword)
{
	HRESULT hr = S_OK;
	KS_CHECK_BOOLEX(m_pISystemConnect, hr = E_FAIL);
	m_pISystemConnect->UnlockPlayerAccount((char*)szName, (char*)szPassword);
KS_EXIT:
	return hr;
}

int KGameMasterCore::PingSystem()
{
	int nPing = 0;
	if (!m_pISystemConnect)
		{ ASSERT(0); return 0;}
	::ResetEvent(m_ExgSYS.hEventPingDone);
	m_pISystemConnect->SendPing();

	if (::WaitForSingleObject(m_ExgSYS.hEventPingDone, PING_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventPingDone);
		nPing = MAX_PING;
		TRACE("%s(%u): KGameMasterCore::Ping() time out.\n", __FILE__, __LINE__);
	}

	if (MAX_PING != nPing)
	{
		::EnterCriticalSection(&m_CS);
		nPing = m_ExgSYS.nPing;
		::LeaveCriticalSection(&m_CS);
	}

	return nPing;
}

BOOL KGameMasterCore::GetGWByName(GAMEWORLD& gw, LPCSTR szGWName)
{
	int i = 0, nSize = 0;

	::EnterCriticalSection(&m_CS);
	for (i = 0, nSize = (int)m_ExgSYS.aryGameWorlds.size(); i < nSize; i++)
	{
		gw = m_ExgSYS.aryGameWorlds[i];
		if (gw.strGameWorldName == szGWName)
		{
			//GameWorld found
			break;
		}
	}
	::LeaveCriticalSection(&m_CS);
	ASSERT(i < nSize);	//GameWorld not login ???
	return (i < nSize);
}

//called when WM_JX_GW_LOGIN happened
BOOL KGameMasterCore::InitGameWorld(LPCSTR szGWName)
{
	BOOL br = TRUE;

	GAMEWORLD gw;
	br = GetGWByName(gw, szGWName);
	KS_CHECK_BOOL(br);

	m_pISystemConnect->GetOnlinePlayerCount((char*)szGWName);

KS_EXIT:
	return br;
}


//gameworld
HRESULT KGameMasterCore::GetPlayerAccount(LPCSTR szPlayerName, BSTR* pbstrAccountArray)
{
	//todo...
	return E_NOTIMPL;
}

HRESULT KGameMasterCore::ExecutePlayer(LPCSTR szAccount, LPCSTR szScriptName)
{
	m_pISystemConnect->ExecuteAction((char*)szAccount, (char*)szScriptName);
	return S_OK;
}

HRESULT KGameMasterCore::TrackingPlayer(LPCSTR szAccount)
{
	//todo...
	return E_NOTIMPL;
}

HRESULT KGameMasterCore::SendMessage(LPCSTR szAccount, LPCSTR szMessage)
{
	//todo...
	return E_NOTIMPL;
}
