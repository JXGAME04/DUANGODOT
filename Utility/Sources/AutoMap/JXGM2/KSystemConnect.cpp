// -------------------------------------------------------------------------
//	文件名		：	KSystemConnect.cpp
//	创建者		：	万里
//	创建时间	：	2003-6-2 20:25:53
//	功能描述	：	
//
// -------------------------------------------------------------------------

#include "stdafx.h"

#include "KSystemConnect.h"

#include "KProtocolDef.h"
#include "../../Sword3PaySys/S3AccServer/AccountLoginDef.h"
#include "KProtocol.h"
#include "crtdbg.h"
#include "malloc.h"

#include "../../../Engine/Src/KWin32.h"
#include "../../../Engine/Src/KSG_MD5_String.h"

#include "WinSock.h"
#include "../../../SwordOnline/Headers/inoutmac.h"

inline const char* _ip2a(DWORD ip) { in_addr ia; ia.s_addr = ip; return inet_ntoa(ia); }

static char* gs_Filter[] = 
{
	"Msg2GM",
	"Msg2IP",
};


KSystemConnect::KSystemConnect(DWORD nSerIP)
{
	m_hLogined = NULL;
	m_bLogined = FALSE;

	m_ProtocolTable[MAKEWORD(pf_normal,s2c_gatewayverify)] = P_Logined;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_gatewayinfo)] = P_ServerInfo;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_gamelogin)] = P_FreezeReturn;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_accountlogout)] = P_FreezeReturn;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_ping)] = P_Ping;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_s2c_loseway)] = P_MissingPlayer;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_c2c_data)] = P_GWReturnMsg;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_c2c_askwaydata)] = P_AskwayProcess;

	memset(m_MacAddress, 0, 6);

	gGetMacAndIPAddress(NULL, NULL, m_MacAddress, NULL);

	m_pEvent = NULL;

	m_nRelyID = -1;

	m_szAccount[0] = 0;

	m_nFlag = 0;
}

KSystemConnect::~KSystemConnect()
{
	if (m_hLogined)
		CloseHandle(m_hLogined);
	m_hLogined = NULL;
}

void KSystemConnect::ConnectClose()
{
	KConnect::ConnectClose();
	KSystemConnect* p = this;

	if (p->m_pEvent)
	{
		p->m_pEvent->ConnectClosed();
	}
}

bool KSystemConnect::CanAction(char* szScript)
{
	if (m_nFlag & UNFILTER)
		return true;

	int nCount = sizeof(gs_Filter) / sizeof(char*);
	for (int i = 0; i < nCount; i++)
	{
		if (strstr(szScript, gs_Filter[i]))
			return false;
	}

	return true;
}

bool KSystemConnect::NeedGMLogEvent(char* szAccount)
{
	if (m_nFlag & GMLOGINOUT)	//当此标志设上是,则
		return true;

	if (szAccount &&
		szAccount[0] == 'g' &&
		szAccount[0] == 'm' &&
		szAccount[0] == '-')
		return false;

	return true;
}

void KSystemConnect::P_Logined(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	KAccountUserReturn* pReturn = (KAccountUserReturn*)(((BYTE*)pData) + 2);
	if (pReturn && dataLength - 2 == sizeof(KAccountUserReturn))
	{
		if (pReturn->nReturn == ACTION_SUCCESS)
			p->m_bLogined = TRUE;
	}

	SetEvent(p->m_hLogined);
}

void KSystemConnect::P_FreezeReturn(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	KAccountUserReturn* pReturn = (KAccountUserReturn*)(((BYTE*)pData) + 2);
	if (pReturn && dataLength - 2 == sizeof(KAccountUserReturn))
	{
		p->m_pEvent->AccountUserReturn(pReturn->Operate, pReturn->Account, pReturn->nReturn);
	}
}

void KSystemConnect::P_UnlockReturn(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	KAccountUserReturn* pReturn = (KAccountUserReturn*)(((BYTE*)pData) + 2);
	if (pReturn && dataLength - 2 == sizeof(KAccountUserReturn))
	{
		p->m_pEvent->AccountUserReturn(pReturn->Operate, pReturn->Account, pReturn->nReturn);
	}
}

void KSystemConnect::P_ServerInfo(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;

	if (!p->m_bLogined)
		return;
	if (!p->m_pEvent)
		return;
	KServerInfo* pReturn = (KServerInfo*)(((BYTE*)pData) + 2);
	if (pReturn && dataLength - 2 == sizeof(KServerInfo))
	{
		switch (pReturn->nServerType)
		{
		case server_LoginAlready:
		case server_Login:
			if (p->NeedGMLogEvent(pReturn->Account))
			{
				p->m_pEvent->GWLogin(pReturn->Account, pReturn->nValue);
				p->GetGWRelayID(0, pReturn->Account);
			}
			break;
		case server_Logout:
			if (p->NeedGMLogEvent(pReturn->Account))
			{
				p->m_pEvent->GWLogout(pReturn->Account, pReturn->nValue);
			}
			break;
		case server_RegisterCount:
			p->m_pEvent->RegisterUserCount(pReturn->Operate, pReturn->nValue);
			break;
		case server_OnlinePlayerCount:
			p->m_pEvent->OnlinePlayerCount(pReturn->Operate, pReturn->Account, pReturn->nValue);
			break;
		case server_PlayerWhere:
			//p->m_pEvent->PlayerAccountAtGW(pReturn->Account, pReturn->nValue, -1, -1, -1, NULL);
			break;
		case server_PlayerWhereID:
			//p->m_pEvent->PlayerAccountAtGWID(pReturn->Account, pReturn->nValue);
			break;
		case server_GWRelayID:
			if (strcmp(pReturn->Account, p->m_szAccount) == 0)
			{
				p->m_nRelyID = pReturn->nValue;
			}
			else
			{
				if (p->NeedGMLogEvent(pReturn->Account))
					p->m_pEvent->GWRelayID(pReturn->Account, pReturn->nValue);
			}
			break;
		default:
			_ASSERT(0);
			break;
		}
	}
}

void KSystemConnect::P_Ping(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	PING_COMMAND* pReturn = (PING_COMMAND*)((char*)pData + 1);
	if (pReturn && dataLength - 1 == sizeof(PING_COMMAND))
	{
		p->m_pEvent->Ping(pReturn->m_dwTime);
	}
}

HRESULT STDMETHODCALLTYPE KSystemConnect::Login(char* szGMAccount, char* szPassword, ISystemEvent* pEvent)
{
	HRESULT hr = E_FAIL;
	m_pEvent = pEvent;
	if (NULL == m_hLogined)
	{
		m_hLogined = CreateEvent(NULL, TRUE, FALSE, NULL);

		KServerAccountUserLoginInfo acclogin;
		acclogin.Size = sizeof(KServerAccountUserLoginInfo);
		acclogin.Version = ACCOUNT_CURRENT_VERSION;
		acclogin.Type = ServerAccountUserLoginInfo;
		acclogin.Operate = 0;
		strncpy(acclogin.Account, szGMAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
		acclogin.Account[LOGIN_USER_ACCOUNT_MAX_LEN - 1] ='\0';
		
		KSG_StringToMD5String(acclogin.Password, szPassword);
		//strncpy(acclogin.Password, szPassword, LOGIN_USER_PASSWORD_MAX_LEN);
		//acclogin.Password[LOGIN_USER_PASSWORD_MAX_LEN - 1] = '\0';

		memcpy(acclogin.MacAddress, m_MacAddress, sizeof(acclogin.MacAddress));
		SendProtocol(c2s_gatewayverify, &acclogin, acclogin.Size);
		
		if (WaitForSingleObject(m_hLogined, FUNC_TIMEOUT) == WAIT_OBJECT_0)
		{
			if (m_bLogined)
			{
				hr = S_OK;
				strncpy(m_szAccount, szGMAccount, 63);
				m_szAccount[63] = 0;

				GetGWRelayID(0, szGMAccount);
			}
		}
	}
	return hr;
}

void STDMETHODCALLTYPE KSystemConnect::Close()
{
	Stop();
}

void STDMETHODCALLTYPE KSystemConnect::GetOnlinePlayerCount(DWORD Operate, char* szGWName)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	strncpy(user.Account, szGWName, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_OnlinePlayerCount;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetRegisterUserCount(DWORD Operate)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	user.Account[0] = 0;
	user.nServerType = server_RegisterCount;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

/*
void STDMETHODCALLTYPE KSystemConnect::GetPlayerAccountAtGW(DWORD Operate, char* szAccount)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_PlayerWhere;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerAccountAtGWID(DWORD Operate, char* szAccount)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_PlayerWhereID;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}
*/

void STDMETHODCALLTYPE KSystemConnect::GetGWRelayID(DWORD Operate, char* szGWName)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	strncpy(user.Account, szGWName, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_GWRelayID;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::UnlockPlayerAccount(DWORD Operate, char* szAccount)
{
	KAccountUserLogout user;
	user.Size = sizeof(KAccountUserLogout);
	user.Type = AccountUserLogout;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	user.nExtPoint = 0;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	SendProtocol(c2s_accountlogout, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::FreezePlayerAccount(DWORD Operate, char* szAccount)
{
	KAccountUser user;
	user.Size = sizeof(KAccountUser);
	user.Type = AccountUser;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = Operate;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	SendProtocol(c2s_gamelogin, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::SendPing()
{
#pragma pack(1)
	typedef struct tagMyPing{
		BYTE byFamily;
		PING_COMMAND oldPing;
	}MYPING;
#pragma pack()
	MYPING PingCmd;
	PingCmd.byFamily = pf_normal;
	PingCmd.oldPing.ProtocolType = c2s_ping;
	PingCmd.oldPing.m_dwTime = GetTickCount();
	SendData(&PingCmd, sizeof(MYPING));
}

void STDMETHODCALLTYPE KSystemConnect::ExecuteAction(char* szAccount, char* szScript, DWORD nReturn)
{
	int nszLen = 0;
	if (szScript)
		nszLen = strlen(szScript);

	if (nszLen == 0 || nszLen > 128)
		return;

	if (!CanAction(szScript))
		return;

	if (szAccount == NULL || szAccount[0] == 0)
		return;

	char szExe[180];
	if (nReturn == 0)
		nszLen = sprintf(szExe, "ds %s", szScript);
	else
		nszLen = sprintf(szExe, "ds Msg2IP(\"%s\", %d, %s, %d)", _ip2a(m_nIP),
			m_nRelyID, szScript, nReturn);

	GM_EXECUTE_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_execute;
	strncpy(exe.AccountName , szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.wLength = nszLen;
	exe.wExecuteID = 1;

	SendRelayAccountData(szAccount, &exe, sizeof(exe), szExe, nszLen);
}

void STDMETHODCALLTYPE KSystemConnect::ExecuteAction(char* szRole, DWORD nRelayID, char* szScript, DWORD nReturn)
{
	if (m_nRelyID == -1 || nRelayID == -1)
		return;
	int nszLen = 0;
	if (szScript)
		nszLen = strlen(szScript);
	if (nszLen == 0 || nszLen > 128)
		return;

	if (!CanAction(szScript))
		return;

	if (szRole == NULL || szRole[0] == 0)
		return;

	char szExe[180];
	if (nReturn == 0)
		nszLen = sprintf(szExe, "ds %s", szScript);
	else
		nszLen = sprintf(szExe, "ds Msg2IP(\"%s\", %d, %s, %d)", _ip2a(m_nIP),
			m_nRelyID, szScript, nReturn);

	GM_EXECUTE_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_execute;
	strncpy(exe.AccountName , szRole, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.wLength = nszLen;
	exe.wExecuteID = 1;

	SendRelayRoleData(szRole, m_nIP, nRelayID, m_nRelyID, &exe, sizeof(exe), szExe, nszLen);
}

void STDMETHODCALLTYPE KSystemConnect::ExecuteAction(DWORD nRelayID, char* szScript, DWORD nReturn, DWORD nSendIP)
{
	if (m_nRelyID == -1 || nRelayID == -1)
		return;
	int nszLen = 0;
	if (szScript)
		nszLen = strlen(szScript);
	if (nszLen == 0 || nszLen > 128)
		return;

	if (!CanAction(szScript))
		return;

	char szExe2[180];
	if (nReturn == 0)
		nszLen = sprintf(szExe2, "%s", szScript);
	else
		nszLen = sprintf(szExe2, "Msg2IP([[%s]], %d, %s, %d)", _ip2a(m_nIP),
			m_nRelyID, szScript, nReturn);

	char szExe[180];
	if (nSendIP != 0)
	{
		if (nSendIP == INADDR_BROADCAST)
		{
			nszLen = sprintf(szExe, "GlobalExecute(\"dw %s\")", szExe2);
		}
		else
		{
			nszLen = sprintf(szExe, "GameExecute(\"%s\", \"dw %s\")",  _ip2a(nSendIP), szExe2);
		}
	}
	else
		strcpy(szExe, szExe2);

	GM_EXECUTE_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_execute;
	strncpy(exe.AccountName , "GM", LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.wLength = nszLen;
	exe.wExecuteID = 1;

	SendRelayData(m_nIP, nRelayID, &exe, sizeof(exe), szExe, nszLen);
}

void KSystemConnect::P_MissingPlayer(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	RELAY_DATA* pLoseWayData = (RELAY_DATA*)pData;
	RELAY_ASKWAY_DATA* pAskWayData = (RELAY_ASKWAY_DATA*)(pLoseWayData + 1);
	if (pLoseWayData)
	{
		pData = (char*)(pAskWayData + 1) + pAskWayData->wMethodDataLength;
		switch (((char*)(pData))[0])
		{
		case pf_gamemaster:
			p->m_pEvent->LosePlayer(((GM_HEADER*)pData)->AccountName, pLoseWayData->nFromIP);
			break;
		};
	}
}

void KSystemConnect::P_GWReturnMsg(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	RELAY_DATA* pRelayData = (RELAY_DATA*)pData;
	EXTEND_HEADER* pEH = (EXTEND_HEADER*)(pRelayData + 1);
	switch (pEH->ProtocolFamily)
	{
	case pf_playercommunity:
		{
			if (playercomm_channelchat == pEH->ProtocolID)
			{
				CHAT_MSG_EX* pChatData = (CHAT_MSG_EX*)pEH;
				char* szMessage = (char*)(pChatData + 1);
				szMessage[pChatData->SentenceLength] = 0;
				p->m_pEvent->PlayerMessage(pChatData->m_szAccountName, pChatData->m_szSourceName,
									  szMessage, pChatData->SentenceLength, pRelayData->nFromIP);

				p->AutoReturnAction(pChatData->m_szAccountName, szMessage);
			}
		}
		break;
	default:
		_ASSERT(0);
		break;
	};
}

void KSystemConnect::P_AskwayProcess(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	RELAY_ASKWAY_DATA* pAskwayData = (RELAY_ASKWAY_DATA*)pData;
	if (pAskwayData->seekMethod != rm_gm)
		return;
	EXTEND_HEADER* pEH = (EXTEND_HEADER*)(pAskwayData + 1);
	switch (pEH->ProtocolFamily)
	{
	case pf_playercommunity:
		{
			if (playercomm_channelchat == pEH->ProtocolID)
			{
				CHAT_MSG_EX* pChatData = (CHAT_MSG_EX*)pEH;
				char* szMessage = (char*)(pChatData + 1);
				szMessage[pChatData->SentenceLength] = 0;
				p->m_pEvent->PlayerMessage(pChatData->m_szAccountName, pChatData->m_szSourceName,
									  szMessage, pChatData->SentenceLength, pAskwayData->nFromIP);
				
				p->AutoReturnAction(pChatData->m_szAccountName, szMessage);
			}
		}
		break;
	case pf_gamemaster:
		{
		}
		break;
	case pf_normal:
		{
			if (s2c_gatewayinfo == pEH->ProtocolID)
			{
				KServerInfo* pInfo = (KServerInfo*)(pEH + 1);
				if (pInfo->nServerType == server_Logout)
					p->m_pEvent->GWSubLogout(pAskwayData->nFromIP, pInfo->nValue);
			}
		}
		break;
	default:
		_ASSERT(0);
		break;
	};
}

DWORD KSystemConnect::GetIP()
{
	return m_nIP;
}

DWORD KSystemConnect::GetRelayID()
{
	return m_nRelyID;
}

void KSystemConnect::SetFlag(int nFlag)
{
	m_nFlag = nFlag;
}

bool KSystemConnect::AutoReturnAction(char* szAccountName, char* szMessage)
{
	if (m_nFlag & GMAUTORETURN)
	{
		//自动执行GM指令
		if (strstr(szMessage, "?gm ds ") == szMessage)
		{
			ExecuteAction(szAccountName, szMessage + 7, 0);
			return true;
		}
	}

	return false;
}