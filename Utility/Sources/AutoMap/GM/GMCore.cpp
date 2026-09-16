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
#include "S3PDBConnectionPool.h"
#include "S3PResultVBC.h"

int  KGameMasterCore::s_nJailWorldID = 111;
int  KGameMasterCore::s_nJailX = 1598;
int  KGameMasterCore::s_nJailY = 3198;
extern CString gINI_FILENAME;

KGameMasterCore::KGameMasterCore(ISystemOnline* pISO)
{
	m_pISystemOnline	= pISO;
	m_pISystemConnect	= NULL;
	m_pSysHandler		= NULL;
	m_pConn				= NULL;
	m_pConnSQL			= NULL;
	m_dwLastUseDBTime	= 0;
	m_dwLastUseDBTimeSQL= 0;

	//(exchange data init
	m_ExgSYS.dwIPGWIncludePlayer		= 0;
	m_ExgSYS.dwRelayIDIncludePlayer		= 0;
	m_ExgSYS.dwRelayIDOfGameWorld		= 0;
	m_ExgSYS.nPing						= MAX_PING;
	m_ExgSYS.nRegisterCount				= 0;
	m_ExgSYS.nOnlineCount				= 0;
	memset(&m_ExgSYS.PlayerInfo, 0, sizeof(m_ExgSYS.PlayerInfo));
	memset(&m_ExgSYS.posInfo, 0, sizeof(m_ExgSYS.posInfo));
	//)

	m_bConnected					= FALSE;
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
	m_ExgSYS.hEventGetRoleListDone		= ::CreateEvent(NULL, TRUE, TRUE, NULL);
	m_ExgSYS.hEventGetGWRelayIDDone		= ::CreateEvent(NULL, TRUE, TRUE, NULL);

	//get jail postion
	s_nJailX = GetPrivateProfileInt("Jail", "PosX", 1598, gINI_FILENAME);
	s_nJailY = GetPrivateProfileInt("Jail", "PosY", 3198, gINI_FILENAME);
	s_nJailWorldID = GetPrivateProfileInt("Jail", "WorldID", 111, gINI_FILENAME);

	//get Pay System connection settings from ini file
	GetPrivateProfileString("System", "IP",
				  "61.55.138.19",
				  strTmp.GetBuffer(STR_BUFFER_LEN),
				  STR_BUFFER_LEN,
				  gINI_FILENAME);
	strTmp.ReleaseBuffer();
	dwSystemIP = inet_addr(strTmp);
	ASSERT(dwSystemIP != INADDR_NONE);

	nSystemPort = GetPrivateProfileInt("System", "Port", 7777, gINI_FILENAME);

	//open connection for Pay System
	hr = pISO->OpenSystemConnect(dwSystemIP, nSystemPort, &m_pISystemConnect);
	if (FAILED(hr))
	{
		CString str;
		str.Format("在地址 %s 找不到 S3ReleyServer !!!", strTmp);
		::AfxMessageBox(str);
		goto KS_EXIT;
	}

	m_pSysHandler = new KSystemEventHandler(&m_ExgSYS);
	KS_CHECK_BOOLEX(m_pSysHandler, hr = E_FAIL);

	//Get Account and Password from ini file
	GetPrivateProfileString("Account", "Name",
					   "n2tbsltAwyZyn2hzgABlIR__VQTk9Arc",	//"gm-kingsoft"
					   strTmp.GetBuffer(STR_BUFFER_LEN),
					   STR_BUFFER_LEN,
					   gINI_FILENAME);
	strTmp.ReleaseBuffer();
	::SimplyDecryptPassword(strAccount.GetBuffer(PG_MAXBUFFER), strTmp);
	strAccount.ReleaseBuffer();

	GetPrivateProfileString("Account", "Password",
					   "Rrd0cpp_k1IAxBA3Z_OKA4gtFAmjlmZt",	//"gm-kingsoft"
					   strTmp.GetBuffer(STR_BUFFER_LEN),
					   STR_BUFFER_LEN,
					   gINI_FILENAME);
	strTmp.ReleaseBuffer();
	::SimplyDecryptPassword(strPassword.GetBuffer(PG_MAXBUFFER), strTmp);
	strPassword.ReleaseBuffer();

	//GameMaster Login and Event Handlers Junction
	hr = m_pISystemConnect->Login((char*)(const char*)strAccount, (char*)(const char*)strPassword, m_pSysHandler);
	if (FAILED(hr))
	{
		::AfxMessageBox("GM.exe登入系统失败!!!");
		goto KS_EXIT;
	}

	//First get online count
	m_pISystemConnect->GetOnlinePlayerCount("");
	m_pISystemConnect->GetRegisterUserCount();
	m_bConnected = TRUE;

KS_EXIT:
	;
}

HRESULT KGameMasterCore::GetRoleList(LPCSTR szAccount, LPCSTR szGWName, CStringList& lstRoleNames)
{
	HRESULT hr = S_OK;
	DWORD dwRelayID = 0xFFFFFFFF;

	KS_CHECK_BOOLEX(m_pISystemConnect && m_bConnected, hr = E_FAIL);

	KS_CHECK_BOOLEX(WhichRelayIsTheGameWorld(dwRelayID, szGWName), hr = E_FAIL);

	//wait for GetRoleList done
	::WaitForSingleObject(m_ExgSYS.hEventGetRoleListDone, INFINITE);

	::ResetEvent(m_ExgSYS.hEventGetRoleListDone);
	m_pISystemConnect->GetRoleListFromDB((char*)szAccount, dwRelayID);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetRoleListDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetRoleListDone);
		TRACE("%s(%u): KGameMasterCore::GetRoleList(..., szAccountName == \"%s\") time out.\n", __FILE__, __LINE__, szAccount);
		hr = E_FAIL;
		goto KS_EXIT;
	}

	lstRoleNames.RemoveAll();
	::EnterCriticalSection(&m_CS);
	lstRoleNames.AddTail(&m_ExgSYS.lstRoleName);
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return hr;
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
		::CloseHandle(m_ExgSYS.hEventGetRoleListDone);
		::CloseHandle(m_ExgSYS.hEventGetGWRelayIDDone);
	}

	if (m_pSysHandler)
		{ delete m_pSysHandler; m_pSysHandler = NULL; } 

	if (m_pConn)
	{
		S3PDBAccessConnectionPool* pDB = S3PDBAccessConnectionPool::Instance();
		if (pDB->ReturnDBCon(m_pConn))
			m_pConn = NULL;
	}

	if (m_pConnSQL)
	{
		S3PDBSQLServerConnectionPool* pDB = S3PDBSQLServerConnectionPool::Instance();
		if (pDB->ReturnDBCon(m_pConnSQL))
			m_pConnSQL = NULL;
	}

	::DeleteCriticalSection(&m_CS);
}

//pay system
int KGameMasterCore::GetOnlineCount(LPCSTR szGWName)
{
	int nOnlineCount = 0, i, nSize;

	if (!m_bConnected || !m_pISystemConnect)
		{ return -1;}

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
	if (!m_pISystemConnect || !m_bConnected)
		{ return -1;}

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

	KS_CHECK_BOOLEX(m_pISystemConnect && m_bConnected, hr = E_FAIL);

	//wait for WhichGWIsPlayerInRealTime()
	::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, INFINITE);

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
	if (0 == m_ExgSYS.dwIPGWIncludePlayer || 0xFFFFFFFF == m_ExgSYS.dwIPGWIncludePlayer)
	{
		gw.dwIP = m_ExgSYS.dwIPGWIncludePlayer;
		gw.nOnlinePlayerCount = -1;
		gw.strGameWorldName.Empty();
	}
	else
	{
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
		TRACE(i < nSize ?  "" : "GameWorld not login ???\n");
	}
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return hr;
}

BOOL KGameMasterCore::WhichRelayIsPlayerIn(DWORD& dwRelayID, LPCSTR szAccount)
{
	HRESULT hr = S_OK;

	dwRelayID = 0xFFFFFFFF;

	KS_CHECK_BOOLEX(m_pISystemConnect && m_bConnected, hr = E_FAIL);

	//wait for WhichGWIsPlayerXXXX()
	::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, INFINITE);

	::EnterCriticalSection(&m_CS);
	ASSERT(m_ExgSYS.strAccount.IsEmpty());
	m_ExgSYS.strAccount = szAccount;
	m_ExgSYS.dwRelayIDIncludePlayer = 0xFFFFFFFF;
	::LeaveCriticalSection(&m_CS);

	::ResetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
	m_pISystemConnect->GetPlayerAccountAtGWID((char*)szAccount);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
		TRACE("%s(%u): KGameMasterCore::WhichGWIsPlayerIn(..., szAccountName == \"%s\") time out.\n", __FILE__, __LINE__, szAccount);
		hr = E_FAIL;
	}

	::EnterCriticalSection(&m_CS);
	m_ExgSYS.strAccount.Empty();
	if (SUCCEEDED(hr))
		dwRelayID = m_ExgSYS.dwRelayIDIncludePlayer;
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return SUCCEEDED(hr) && dwRelayID != 0xFFFFFFFF;
}

BOOL KGameMasterCore::WhichRelayIsTheGameWorld(DWORD& dwRelayID, LPCSTR szGWName)
{
	HRESULT hr = S_OK;

	dwRelayID = 0xFFFFFFFF;

	KS_CHECK_BOOLEX(m_pISystemConnect && m_bConnected && szGWName && strlen(szGWName), hr = E_FAIL);

	::WaitForSingleObject(m_ExgSYS.hEventGetGWRelayIDDone, INFINITE);

	::EnterCriticalSection(&m_CS);
	m_ExgSYS.dwRelayIDOfGameWorld = 0xFFFFFFFF;
	::LeaveCriticalSection(&m_CS);

	::ResetEvent(m_ExgSYS.hEventGetGWRelayIDDone);
	m_pISystemConnect->GetGWRelayID((char*)szGWName);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetGWRelayIDDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetGWRelayIDDone);
		TRACE("%s(%u): KGameMasterCore::WhichRelayIsTheGameWorld(..., szGWName == \"%s\") time out.\n", __FILE__, __LINE__, szGWName);
		hr = E_FAIL;
	}

	::EnterCriticalSection(&m_CS);
	if (SUCCEEDED(hr))
		dwRelayID = m_ExgSYS.dwRelayIDOfGameWorld;
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return SUCCEEDED(hr) && dwRelayID != 0xFFFFFFFF;
}

HRESULT KGameMasterCore::WhichGWIsPlayerInRealTime(GAMEWORLD& gw, CString& rstrCurRole,
									  int& rnCurX, int& rnCurY, int& rnCurWorldID,
									  LPCSTR szAccount)
{
	HRESULT hr = S_OK;
	int i, nSize;

	KS_CHECK_BOOLEX(m_pISystemConnect && m_bConnected, hr = E_FAIL);

	//wait for WhichGWIsPlayerIn()
	::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, INFINITE);

	::EnterCriticalSection(&m_CS);
	ASSERT(m_ExgSYS.strAccount.IsEmpty());
	m_ExgSYS.strAccount = szAccount;
	::LeaveCriticalSection(&m_CS);

	::ResetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
	m_pISystemConnect->GetPlayerAccountAtGWRealTime((char*)szAccount);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetPlayerAtGWDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetPlayerAtGWDone);
		m_ExgSYS.strAccount.Empty();
		TRACE("%s(%u): KGameMasterCore::WhichGWIsPlayerInRealTime(..., szAccountName == \"%s\") time out.\n", __FILE__, __LINE__, szAccount);
		hr = E_FAIL;
		goto KS_EXIT;
	}

	::EnterCriticalSection(&m_CS);
	rstrCurRole = m_ExgSYS.strCurrentRole;
	rnCurX = m_ExgSYS.nCurX;
	rnCurY = m_ExgSYS.nCurY;
	rnCurWorldID = m_ExgSYS.nCurWorldID;
	m_ExgSYS.strAccount.Empty();
	if (0 == m_ExgSYS.dwIPGWIncludePlayer || 0xFFFFFFFF == m_ExgSYS.dwIPGWIncludePlayer)
	{
		gw.dwIP = m_ExgSYS.dwIPGWIncludePlayer;
		gw.nOnlinePlayerCount = -1;
		gw.strGameWorldName.Empty();
	}
	else
	{
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
		TRACE(i < nSize ?  "" : "GameWorld not login ???\n");
	}
	::LeaveCriticalSection(&m_CS);

KS_EXIT:
	return hr;
}

HRESULT KGameMasterCore::UnlockPlayerAccount(LPCSTR szName, LPCSTR szPassword)
{
	HRESULT hr = S_OK;
	KS_CHECK_BOOLEX(m_bConnected && m_pISystemConnect, hr = E_FAIL);
	m_pISystemConnect->UnlockPlayerAccount((char*)szName, (char*)szPassword);
KS_EXIT:
	return hr;
}

int KGameMasterCore::PingSystem()
{
	int nPing = 0, nStartingCount = ::GetTickCount();

	//wait for previous ping CMD
	::WaitForSingleObject(m_ExgSYS.hEventPingDone, INFINITE);

	if (!m_pISystemConnect || m_bConnected)
		{ return 0;}
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
		nPing = GetTickCount() - nStartingCount;
	}

	return nPing;
}

void KGameMasterCore::JustPingSystem()
{
	if (!m_pISystemConnect || !m_bConnected)
		{ return; }
	m_pISystemConnect->SendPing();
}

BOOL KGameMasterCore::GetGWByName(GAMEWORLD& gw, LPCSTR szGWName)
{
	int i = 0, nSize = 0;

	if (!m_pISystemConnect || !m_bConnected)
		{ return FALSE; }

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

int KGameMasterCore::CountOfGameWorlds()
{
	int nSize = 0;

	if (!m_pISystemConnect || !m_bConnected)
		{ return 0; }

	::EnterCriticalSection(&m_CS);
	nSize = (int)m_ExgSYS.aryGameWorlds.size();
	::LeaveCriticalSection(&m_CS);

	return nSize;
}

BOOL KGameMasterCore::GetGWByIndex(GAMEWORLD& gw, int nIndex)
{
	int nSize = 0;

	if (!m_pISystemConnect || !m_bConnected)
		{ return FALSE; }

	::EnterCriticalSection(&m_CS);
	nSize = (int)m_ExgSYS.aryGameWorlds.size();
	if (nIndex >= 0 && nIndex < nSize)
		gw = m_ExgSYS.aryGameWorlds[nIndex];
	::LeaveCriticalSection(&m_CS);

	ASSERT(nIndex >= 0 && nIndex < nSize);
	return (nIndex >= 0 && nIndex < nSize);
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

HRESULT KGameMasterCore::SendMessage(LPCSTR szAccount, LPCSTR szRole, LPCSTR szMessage)
{
	HRESULT hr = S_OK;
	KS_CHECK_BOOLEX((m_bConnected && m_pISystemConnect), hr = E_FAIL);

	m_pISystemConnect->SendMessage((char*)szAccount, (char*)szRole, (char*)szMessage);
	
KS_EXIT:
	return hr;
}

void KGameMasterCore::DoJailProcess(DWORD dwTickCount)
{
	if (1 != dwTickCount % GM_INTERCOUNT_JAIL)
		return;
//	if (!IS_GM_INTERCOUNT_REACHED(dwTickCount, GM_INTERCOUNT_JAIL))
//		return;

	S3PDBConVBC* pCon = GetDB(10);
	char strSQL[256];
	S3PResultVBC* pResult = NULL;
	CString strTmp;

	sprintf(strSQL, "select * from JXJail where (JailMinutesLeft > 0)");
	if (pCon->QuerySql(strSQL, &pResult))
	{
		RefreshLastUseDBTime();
		//
		if (pResult && pResult->num_rows() > 0)
		{
			EnterCriticalSection(&m_CS);

			//清除过多的无效等待队列
			if (m_ExgSYS.lstJailWaitingForFree.GetCount() > pResult->num_rows() * 2 + 128)
			{
				POSITION pos = m_ExgSYS.lstJailWaitingForFree.GetHeadPosition(),
						 posPrev = pos;
				for (int i = 0; i < 64; i++)
				{
					posPrev = pos;
					m_ExgSYS.lstJailWaitingForFree.GetNext(pos);
					m_ExgSYS.lstJailWaitingForFree.RemoveAt(posPrev);
				}
			}

			_variant_t AccountName, RoleName, GWName;
			for (int i = 0, nSize = pResult->num_rows();
				 i < nSize;
				 i++, pResult->data_seek(1, S3PResultVBC::next))
			{
				if (pResult->get_field_data(1, &AccountName, sizeof(_variant_t)) &&
					pResult->get_field_data(2, &RoleName, sizeof(_variant_t)) &&
					pResult->get_field_data(7, &GWName, sizeof(_variant_t)))
				{
					ASSERT(VT_BSTR == AccountName.vt && VT_BSTR == RoleName.vt && VT_BSTR == GWName.vt);
					//将该用户加入等待处理队列
					strTmp.Format("%s\n%s\n%s",
							CString(((LPCWSTR)(_bstr_t)AccountName)),
							CString((LPCWSTR)(_bstr_t)RoleName),
							CString((LPCWSTR)(_bstr_t)GWName));
					m_ExgSYS.lstJailWaitingForFree.AddTail(strTmp);

					//向游戏世界发送找人命令，一旦找到，在EventHandler中处理之
					strTmp = (char*)(_bstr_t)AccountName;
					m_pISystemConnect->GetPlayerAccountAtGWRealTime((char*)(LPCSTR)strTmp);
				}
			}

			LeaveCriticalSection(&m_CS);
		}
	}

	if (pResult)
		pResult->unuse();
}

void KGameMasterCore::DoShutupProcess(DWORD dwTickCount)
{
	if (!IS_GM_INTERCOUNT_REACHED(dwTickCount, GM_INTERCOUNT_SHUTUP))
		return;

	S3PDBConVBC* pCon = GetDB(10);
	//todo...
}

S3PDBConVBC* KGameMasterCore::GetDB(DWORD nSleep)
{
	if (!m_pConn)
	{
		S3PDBAccessConnectionPool* pDB = S3PDBAccessConnectionPool::Instance();
		if (nSleep)
		{
			while (m_pConn == NULL)
			{
				pDB->RemoveDBCon(&m_pConn);
				Sleep(nSleep);
			}
		}
		else
			pDB->RemoveDBCon(&m_pConn);
	}

	return m_pConn;
}

S3PDBConVBC* KGameMasterCore::GetSQLDB(DWORD nSleep)
{
	if (!m_pConnSQL)
	{
		S3PDBSQLServerConnectionPool* pDB = S3PDBSQLServerConnectionPool::Instance();
		if (nSleep)
		{
			while (m_pConnSQL == NULL)
			{
				pDB->RemoveDBCon(&m_pConnSQL);
				Sleep(nSleep);
			}
		}
		else
			pDB->RemoveDBCon(&m_pConnSQL);
	}

	return m_pConnSQL;
}

void KGameMasterCore::RefreshLastUseDBTime()
{
	m_dwLastUseDBTime = ::GetTickCount();
}

void KGameMasterCore::RefreshLastUseDBTimeForSQL()
{
	m_dwLastUseDBTimeSQL = ::GetTickCount();
}

void KGameMasterCore::Breathe(DWORD dwTickCount)
{
	S3PDBAccessConnectionPool* pDB = S3PDBAccessConnectionPool::Instance();
	S3PDBSQLServerConnectionPool* pDBSQL = S3PDBSQLServerConnectionPool::Instance();

	DoJailProcess(dwTickCount);
	DoShutupProcess(dwTickCount);

	if (!m_dwLastUseDBTime)
		RefreshLastUseDBTime();

	if (!m_dwLastUseDBTimeSQL)
		RefreshLastUseDBTimeForSQL();

	if (GetTickCount() - m_dwLastUseDBTime >= 3000)
	{
		if (m_pConn)
		{
			if (pDB->ReturnDBCon(m_pConn))
				m_pConn = NULL;
		}
	}

	if (GetTickCount() - m_dwLastUseDBTimeSQL >= 3000)
	{
		if (m_pConnSQL)
		{
			if (pDBSQL->ReturnDBCon(m_pConnSQL))
				m_pConnSQL = NULL;
		}
	}
}

HRESULT KGameMasterCore::PutInJail(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName, int nMinutes)
{
	if (nMinutes <= 0 || nMinutes > 120 ||
		strlen(szRole) <= 0 || strlen(szAccount) <= 0 || strlen(szGWName) <= 0)
		return E_FAIL;

	HRESULT hr = S_OK;
	S3PDBConVBC* pCon = GetDB(10);
	S3PResultVBC* pResult = NULL;
	char strSQL[256];
	GAMEWORLD gw;
	CString strRole;
	int nX = -1, nY = -1, nWorld = -1;
	RefreshLastUseDBTime();

	hr = WhichGWIsPlayerInRealTime(gw, strRole, nX, nY, nWorld, szAccount);
	KS_CHECK(hr);

	if (nWorld == KGameMasterCore::s_nJailWorldID || nX < 0 || nY < 0)
	{
		//如果不在线，从数据库获取进入游戏点
		VERIFY(GetPlayerPosInDB(szRole, szGWName, nWorld, nX, nY));
	}

	if (gw.strGameWorldName == szGWName && szRole == strRole)
	{
		//如果该用户在线，立即关入天牢
		strRole.Format("ds NewWorld(%d, %d, %d)",
						KGameMasterCore::s_nJailWorldID,
						KGameMasterCore::s_nJailX,
						KGameMasterCore::s_nJailY);
		hr = ExecutePlayer(szAccount, strRole);
	}
	else
	{
		//用户不在线，更新进入游戏点
		VERIFY(SetPlayerPosInDB(szRole, szGWName, s_nJailWorldID, s_nJailX, s_nJailY));
	}

	sprintf(strSQL, "select count(*) from JXJail where (AccountName='%s' and RoleName='%s' and GameWorldName='%s')", szAccount, szRole, szGWName);
	if (pCon->QuerySql(strSQL, &pResult))
	{
		ASSERT(pResult);
		_variant_t var;
		pResult->get_field_data(0, &var, sizeof(var));

		if ((long)var > 0)
		{
			sprintf(strSQL, "update JXJail set JailMinutesLeft=%d where (AccountName='%s' and RoleName='%s' and GameWorldName='%s')", nMinutes, szAccount, szRole, szGWName);
			KS_CHECK_BOOLEX(pCon->Do(strSQL), hr = E_FAIL);
		}
		else
		{
			sprintf(strSQL, "insert into JXJail (AccountName, RoleName, JailMinutesLeft, OriginX, OriginY, OriginWorld, GameWorldName) values ('%s', '%s', %d, %d, %d, %d, '%s')",
					szAccount, szRole, nMinutes, nX, nY, nWorld, szGWName);
			KS_CHECK_BOOLEX(pCon->Do(strSQL), hr = E_FAIL);
		}
	}
	

KS_EXIT:
	if (pResult)
		pResult->unuse();
	return hr;
}

HRESULT KGameMasterCore::FreeFromJail(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName)
{
	if (strlen(szRole) <= 0 || strlen(szAccount) <= 0 || strlen(szGWName) <= 0)
		return E_FAIL;

	HRESULT hr = S_OK;
	S3PDBConVBC* pCon = GetDB(10);
	S3PResultVBC* pResult = NULL;
	char strSQL[256];
	GAMEWORLD gw;
	CString strRole;
	int nX = -1, nY = -1, nWorld = -1;
	RefreshLastUseDBTime();

	hr = WhichGWIsPlayerInRealTime(gw, strRole, nX, nY, nWorld, szAccount);
	KS_CHECK(hr);

	sprintf(strSQL, "select * from JXJail where (AccountName='%s' and RoleName='%s' and JailMinutesLeft > 0 and GameWorldName='%s')", szAccount, szRole, szGWName);
	if (pCon->QuerySql(strSQL, &pResult))
	{
		ASSERT(pResult);
		_variant_t var;
		if (pResult->num_rows())
		{
			if (pResult->get_field_data(4, &var, sizeof(var)))
				nX = (long)var;
			if (pResult->get_field_data(5, &var, sizeof(var)))
				nY = (long)var;
			if (pResult->get_field_data(6, &var, sizeof(var)))
				nWorld = (long)var;

			if (!(strRole == szRole && gw.strGameWorldName == szGWName))
			{
				//用户不在线，更新进入游戏点
				if (nX > 0 && nY > 0 && nWorld > 0 && nWorld != s_nJailWorldID)
					VERIFY(SetPlayerPosInDB(szRole, szGWName, nWorld, nX, nY));
			}

			if (nX > 0 && nY > 0 && nWorld > 0 && nWorld != s_nJailWorldID)
			{
				//送还player到关天牢之前的位置
				strRole.Format("ds NewWorld(%d, %d, %d)", nWorld, nX, nY);
				hr = ExecutePlayer(szAccount, strRole);
			}
			else
			{
				ASSERT(0);
				//出问题了，调用回城
				strRole.Format("ds UseTownPortal()");
				hr = ExecutePlayer(szAccount, strRole);
			}

			sprintf(strSQL, "delete from JXJail where (AccountName='%s' and RoleName='%s' and GameWorldName='%s')", szAccount, szRole, szGWName);
			KS_CHECK_BOOLEX(pCon->Do(strSQL), hr = E_FAIL);
		}
	}

KS_EXIT:
	if (pResult)
		pResult->unuse();
	return hr;
}

HRESULT KGameMasterCore::JailJudge(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName)
{
	ASSERT(strlen(szRole) > 0 && strlen(szAccount) > 0 && strlen(szGWName));
	HRESULT hr = S_OK;
	S3PDBConVBC* pCon = GetDB(10);
	char strSQL[256];
	S3PResultVBC* pResult = NULL;
	int nMinitesLeft = -1;
	RefreshLastUseDBTime();
	
	sprintf(strSQL, "select * from JXJail where (AccountName='%s' and RoleName='%s' and GameWorldName='%s' and JailMinutesLeft > 0)", szAccount, szRole, szGWName);
	if (pCon->QuerySql(strSQL, &pResult))
	{
		ASSERT(pResult);
		_variant_t var;
		if (pResult->num_rows())
		{
			if (pResult->get_field_data(3, &var, sizeof(var)))
				nMinitesLeft = (long)var;

			nMinitesLeft--;
			if (nMinitesLeft <= 0)
				FreeFromJail(szAccount, szRole, szGWName);
			else
			{
				sprintf(strSQL, "update JXJail set JailMinutesLeft=%d where (AccountName='%s' and RoleName='%s' and GameWorldName='%s' and JailMinutesLeft > 0)", nMinitesLeft, szAccount, szRole, szGWName);
				KS_CHECK_BOOLEX(pCon->Do(strSQL), hr = E_FAIL);
			}
		}
	}

KS_EXIT:
	if (pResult)
		pResult->unuse();
	return hr;
}

HRESULT KGameMasterCore::SaveMessageToDB(LPCSTR szAccount, LPCSTR szRole,
										 LPCSTR szGameWorld, LPCSTR szChat)
{
	HRESULT hr = S_OK;

	S3PDBConVBC* pCon = GetSQLDB(0);
	if (NULL == pCon)
		return E_FAIL;
	char* strSQL = (char*)_alloca(256 + 32 * 3 + strlen(szChat));
	RefreshLastUseDBTimeForSQL();

	COleDateTime myTime = COleDateTime::GetCurrentTime();
	sprintf(strSQL, "insert into question (ddatetime, playerAccount, playerRole, gwcode, question, isAnswer) values ('%s', '%s', '%s', '%s', '%s', 0)",
			myTime.Format(), szAccount, szRole, szGameWorld, szChat);
	KS_CHECK_BOOLEX(pCon->Do(strSQL), hr = E_FAIL);

KS_EXIT:
	return hr;
}

HRESULT KGameMasterCore::SaveDisconnectionToDB(LPCSTR szGameWorld, LPCSTR szSubIP)
{
	HRESULT hr = S_OK;

	//todo...
	
KS_EXIT:
	return hr;
}

HRESULT KGameMasterCore::BroadCastChatToGameWorlds(LPCSTR szGWName, LPCSTR szCharMessage)
{
	HRESULT hr = S_OK;

	int i = 0, nSize = 0;
	CStringList lstGWNames;

	if (NULL == szGWName || 0 == strlen(szGWName))
	{
		::EnterCriticalSection(&m_CS);
		for (i = 0, nSize = (int)m_ExgSYS.aryGameWorlds.size(); i < nSize; i++)
			lstGWNames.AddTail(m_ExgSYS.aryGameWorlds[i].strGameWorldName);
		::LeaveCriticalSection(&m_CS);
	}
	else
	{
		::EnterCriticalSection(&m_CS);
		for (i = 0, nSize = (int)m_ExgSYS.aryGameWorlds.size(); i < nSize; i++)
		{
			if (szGWName == m_ExgSYS.aryGameWorlds[i].strGameWorldName)
			{
				lstGWNames.AddTail(szGWName);
				break;
			}
		}
		::LeaveCriticalSection(&m_CS);
		KS_CHECK_BOOLEX(i < nSize, hr = E_FAIL);
	}

	if (lstGWNames.GetCount())
	{
		DWORD dwRelayID = 0xFFFFFFFF;
		for (POSITION pos = lstGWNames.GetHeadPosition(); pos;)
		{
			KS_CHECK_BOOLEX(WhichRelayIsTheGameWorld(dwRelayID, lstGWNames.GetNext(pos)), hr = E_FAIL);
			m_pISystemConnect->BroadCastChat(dwRelayID, (char*)szCharMessage);
		}
	}
	
KS_EXIT:
	return hr;
}


BOOL KGameMasterCore::SetPlayerPosInDB(LPCSTR szRole, LPCSTR szGWName,
									   int nWorldID, int nX, int nY)
{
	BOOL br = TRUE;

	DWORD dwRelayID = 0xFFFFFFFF;

	KS_CHECK_BOOL(strlen(szRole) && strlen(szGWName));
	KS_CHECK_BOOL(WhichRelayIsTheGameWorld(dwRelayID, szGWName));

	GM_ROLE_DATA_SUB_ENTER_POS data;
	data.nEnterGameX = nX * 32;
	data.nEnterGameY = nY * 32;
	data.nSubWorldID = nWorldID;
	m_pISystemConnect->SetPlayerInfoToDB((char*)szRole, dwRelayID, gm_role_entergame_position,
										&data, sizeof(data));

KS_EXIT:
	return br;
}


BOOL KGameMasterCore::GetPlayerPosInDB(LPCSTR szRole, LPCSTR szGWName,
									   int& nWorldID, int& nX, int& nY)
{
	BOOL br = TRUE;

	DWORD dwRelayID = 0xFFFFFFFF;

	KS_CHECK_BOOL(strlen(szRole) && strlen(szGWName));
	KS_CHECK_BOOL(WhichRelayIsTheGameWorld(dwRelayID, szGWName));

	//wait for GetRoleList done
	::WaitForSingleObject(m_ExgSYS.hEventGetPlayerInfoDone, INFINITE);

	::ResetEvent(m_ExgSYS.hEventGetPlayerInfoDone);
	m_pISystemConnect->GetPlayerInfoFromDB((char*)szRole, dwRelayID, gm_role_entergame_position);

	if (::WaitForSingleObject(m_ExgSYS.hEventGetPlayerInfoDone, SYSTEM_TIMEOUT) != WAIT_OBJECT_0)
	{
		//time out
		::SetEvent(m_ExgSYS.hEventGetPlayerInfoDone);
		TRACE("%s(%u): KGameMasterCore::GetPlayerPosInDB(..., szRoleName == \"%s\") time out.\n", __FILE__, __LINE__, szRole);
		br = FALSE;
		goto KS_EXIT;
	}

	::EnterCriticalSection(&m_CS);
	nWorldID = m_ExgSYS.posInfo.nSubWorldID;
	nX = m_ExgSYS.posInfo.nEnterGameX / 32;
	nY = m_ExgSYS.posInfo.nEnterGameY / 32;
	::LeaveCriticalSection(&m_CS);


KS_EXIT:
	return br;
}