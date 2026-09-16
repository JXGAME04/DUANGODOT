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

#include <nb30.h>
#include "../../../Engine/Src/KWin32.h"
#include "../../../Engine/Src/KSG_MD5_String.h"

extern DWORD g_dwLastOpenIP;
typedef struct _ASTAT_
{
	ADAPTER_STATUS adapt;
	NAME_BUFFER    NameBuff [30];
}ASTAT, * PASTAT;

// 输入参数：lana_num为网卡编号，一般地，从0开始，但在Windows 2000中并不一定是连续分配的 
void getmac_one(int lana_num, ASTAT& Adapter)
{
    NCB ncb;
    UCHAR uRetCode;

    memset( &ncb, 0, sizeof(ncb) );
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lana_num = lana_num;   
    // 指定网卡号

	// 首先对选定的网卡发送一个NCBRESET命令，以便进行初始化 
    uRetCode = Netbios( &ncb );

    memset( &ncb, 0, sizeof(ncb) );
    ncb.ncb_command = NCBASTAT;
    ncb.ncb_lana_num = lana_num;     // 指定网卡号

    strcpy( (char *)ncb.ncb_callname, "*               ");
    ncb.ncb_buffer = (unsigned char *) &Adapter;

	// 指定返回的信息存放的变量 
    ncb.ncb_length = sizeof(Adapter);

	// 接着，可以发送NCBASTAT命令以获取网卡的信息 
    uRetCode = Netbios( &ncb );

    if ( uRetCode == 0 )
    {
		// 把网卡MAC地址格式化成常用的16进制形式，如0010-A4E4-5802 
//		lana_num
//		  Adapter.adapt.adapter_address[0],
//		  Adapter.adapt.adapter_address[1],
//		  Adapter.adapt.adapter_address[2],
//		  Adapter.adapt.adapter_address[3],
//		  Adapter.adapt.adapter_address[4],
//		  Adapter.adapt.adapter_address[5];
	}
}

int getmac_all(ASTAT** ppBuffer)
{
	NCB ncb;
    UCHAR uRetCode;
    LANA_ENUM lana_enum;

	lana_enum.length = 0;

    memset( &ncb, 0, sizeof(ncb) );
    ncb.ncb_command = NCBENUM;

    ncb.ncb_buffer = (unsigned char *) &lana_enum;
    ncb.ncb_length = sizeof(lana_enum);

	// 向网卡发送NCBENUM命令，以获取当前机器的网卡信息，如有多少个网卡、每张网卡的编号等 
	uRetCode = Netbios( &ncb );
	if ( uRetCode == 0 && lana_enum.length > 0)
	{
		*ppBuffer = new ASTAT[lana_enum.length];
		// 对每一张网卡，以其网卡编号为输入编号，获取其MAC地址 
		for ( int i = 0; i < lana_enum.length; ++i)
		{
			getmac_one(lana_enum.lana[i], (*ppBuffer)[i]);
		}
	}

	return lana_enum.length;
}

static int _GetCpuID(BYTE byaryCpuID[12])
{
	DWORD dwaryCPUID[3];

	dwaryCPUID[0] = 0;
	dwaryCPUID[1] = 0;
	dwaryCPUID[2] = 0;
	memcpy(byaryCpuID, dwaryCPUID, 12);

	__try
	{
		__asm mov eax, 0x01
		__asm cpuid
		__asm test edx, (0x01 << 18)
		__asm jz NoSerial
		__asm mov dwaryCPUID[2], eax
		__asm mov eax, 0x03
		__asm cpuid
		__asm mov dwaryCPUID[1], edx
		__asm mov dwaryCPUID[0], ecx
		__asm jmp HaveSerial

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	NoSerial:
	return FALSE;

	HaveSerial:
	memcpy(byaryCpuID, dwaryCPUID, 12);

	return true;
}

KSystemConnect::KSystemConnect()
{
	m_hLogined = NULL;
	m_bLogined = FALSE;

	m_ProtocolTable[MAKEWORD(pf_normal,s2c_gatewayverify)] = P_Logined;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_gatewayinfo)] = P_ServerInfo;
	m_ProtocolTable[MAKEWORD(pf_normal,s2c_ping)] = P_Ping;
	m_ProtocolTable[MAKEWORD(pf_gamemaster,gm_s2c_tracking)] = P_PlayerTrackingData;
	m_ProtocolTable[MAKEWORD(pf_gamemaster,gm_s2c_getrole)] = P_PlayerInfo;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_s2c_loseway)] = P_MissingPlayer;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_c2c_data)] = P_GWReturnMsg;
	m_ProtocolTable[MAKEWORD(pf_relay,relay_c2c_askwaydata)] = P_AskwayProcess;

	memset(m_MacAddress, 0, 6);
	ASTAT* pBuffer = NULL;
	int n = getmac_all(&pBuffer);
	if (n > 0)
	{
		memcpy(m_MacAddress, pBuffer->adapt.adapter_address, 6);
		delete [] pBuffer;
	}

	//_GetCpuID(m_CPU);
}

KSystemConnect::~KSystemConnect()
{
	if (m_hLogined)
		CloseHandle(m_hLogined);
	m_hLogined = NULL;
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
			p->m_pEvent->GWLogin(pReturn->Account, pReturn->nValue);
			break;
		case server_Logout:
			p->m_pEvent->GWLogout(pReturn->Account, pReturn->nValue);
			break;
		case server_RegisterCount:
			p->m_pEvent->RegisterUserCount(pReturn->nValue);
			break;
		case server_OnlinePlayerCount:
			p->m_pEvent->OnlinePlayerCount(pReturn->Account, pReturn->nValue);
			break;
		case server_PlayerWhere:
			p->m_pEvent->PlayerAccountAtGW(pReturn->Account, pReturn->nValue, -1, -1, -1, NULL);
			break;
		case server_PlayerWhereID:
			p->m_pEvent->PlayerAccountAtGWID(pReturn->Account, pReturn->nValue);
			break;
		case server_GWRelayID:
			p->m_pEvent->GameWorldRelayID(pReturn->Account, pReturn->nValue);
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
				hr = S_OK;
		}
	}
	return hr;
}

void STDMETHODCALLTYPE KSystemConnect::Close()
{
	Stop();
}

void STDMETHODCALLTYPE KSystemConnect::GetOnlinePlayerCount(char* szGWName)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = 0;
	strncpy(user.Account, szGWName, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_OnlinePlayerCount;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetRegisterUserCount()
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = 0;
	user.Account[0] = 0;
	user.nServerType = server_RegisterCount;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerAccountAtGW(char* szAccount)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = 0;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_PlayerWhere;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerAccountAtGWID(char* szAccount)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = 0;
	strncpy(user.Account, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_PlayerWhereID;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerAccountAtGWRealTime(char* szAccount)
{
	GM_GET_PLAYER_AT_GW_CMD exe;
	strncpy(exe.AccountName , szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_findplayer;
	SendRelayData(szAccount, &exe, sizeof(exe));
}

void STDMETHODCALLTYPE KSystemConnect::UnlockPlayerAccount(char* szAccount, char* szPassword)
{
	GM_UNLOCK_ACCOUNT exe;
	strncpy(exe.AccountName , szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_unlock;
	SendData(&exe, sizeof(exe));
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

void KSystemConnect::P_PlayerTrackingData(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	GM_TRACK_SYNC *pNPL = (GM_TRACK_SYNC *)((char*)pData);
	dataLength++;
	if (pNPL && dataLength == sizeof(GM_TRACK_SYNC))
	{
		size_t nOffset = sizeof(pNPL->wLength) + sizeof(pNPL->AccountName) + 1;
		p->m_pEvent->PlayerTrackingData(pNPL->AccountName, ((char*)pData) + nOffset, dataLength - nOffset);
	}
}

void KSystemConnect::P_PlayerInfo(void* pThis, const void *pData, size_t dataLength)
{
	KSystemConnect* p = (KSystemConnect*)(KConnect*)pThis;
	if (!p->m_pEvent)
		return;
	GM_GET_ROLE_DATA_SYNC *pNPL = (GM_GET_ROLE_DATA_SYNC *)((char*)pData);
	p->m_pEvent->PlayerInfo(pNPL->AccountName, pNPL->wGetID, pNPL + 1, pNPL->wLength);
}

void STDMETHODCALLTYPE KSystemConnect::ExecuteAction(char* szAccount, char* szScript)
{
	int nszLen = 0;
	if (szScript)
		nszLen = strlen(szScript);
	GM_EXECUTE_COMMAND exe;
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_execute;
	strncpy(exe.AccountName , szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.wLength = nszLen;
	exe.wExecuteID = 1;

	SendRelayData(szAccount, &exe, sizeof(exe), szScript, nszLen);
}

void STDMETHODCALLTYPE KSystemConnect::EnableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength)
{
	GM_ENABLE_ACTION_COMMAND exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_enable;
	exe.btProtocol = nProtocol;
	exe.wLength = dataLength;

	SendRelayData(szAccount, &exe, sizeof(GM_ENABLE_ACTION_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KSystemConnect::DisableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength)
{
	GM_DISABLE_ACTION_COMMAND exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_disable;
	exe.btProtocol = nProtocol;
	exe.wLength = dataLength;

	SendRelayData(szAccount, &exe, sizeof(GM_DISABLE_ACTION_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KSystemConnect::TrackingPlayer(char* szAccount, bool bTrack)
{
	GM_TRACK_COMMAND exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_tracking;
	exe.btTrackFlag = bTrack;

	SendRelayData(szAccount, &exe, sizeof(GM_TRACK_COMMAND));
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerInfo(char* szAccount, WORD ninfo)
{
	GM_GET_ROLE_DATA_COMMAND exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_getrole;
	exe.wGetID = ninfo;
	exe.wLength = 0;

	SendRelayData(szAccount, &exe, sizeof(GM_GET_ROLE_DATA_COMMAND));
}

void STDMETHODCALLTYPE KSystemConnect::GetPlayerInfoFromDB(char* szRole, DWORD dwRelayID, WORD nInfo)
{
	RELAY_DATA data;
	data.ProtocolFamily = pf_relay;
	data.ProtocolID = relay_c2c_data;
	data.nToIP = g_dwLastOpenIP;
	data.nToRelayID = dwRelayID;
	data.nFromRelayID = 0;
	data.nFromIP = 0;
	data.routeDateLength = sizeof(GM_SET_ROLE_DATA_COMMAND);
	GM_GET_ROLE_DATA_COMMAND exe;
	strncpy(exe.AccountName, szRole, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_getrole;
	exe.wGetID = nInfo;
	switch (nInfo)
	{
	case gm_role_entergame_position:
		exe.wLength = 0;
		break;
	default:
		_ASSERT(0);
		break;
	}
	SendData(&data, sizeof(data), &exe, sizeof(exe));
}

void STDMETHODCALLTYPE KSystemConnect::SetPlayerInfoToDB(char* szRole, DWORD dwRelayID, WORD ninfo, void* pData, size_t dataLength)
{
	RELAY_DATA* pRelayData = (RELAY_DATA*)_alloca(sizeof(RELAY_DATA) + sizeof(GM_SET_ROLE_DATA_COMMAND));
	pRelayData->ProtocolFamily = pf_relay;
	pRelayData->ProtocolID = relay_c2c_data;
	pRelayData->nToIP = g_dwLastOpenIP;
	pRelayData->nToRelayID = dwRelayID;
	pRelayData->nFromRelayID = 0;
	pRelayData->nFromIP = 0;
	pRelayData->routeDateLength = sizeof(GM_SET_ROLE_DATA_COMMAND) + dataLength;
	
	GM_SET_ROLE_DATA_COMMAND* pGMData = (GM_SET_ROLE_DATA_COMMAND*)(pRelayData + 1);
	strncpy(pGMData->AccountName, szRole, LOGIN_USER_ACCOUNT_MAX_LEN);
	pGMData->ProtocolFamily = pf_gamemaster;
	pGMData->ProtocolType = gm_c2s_setrole;
	pGMData->wSetID = ninfo;
	pGMData->wLength = dataLength;

	SendData(pRelayData, sizeof(RELAY_DATA) + sizeof(GM_SET_ROLE_DATA_COMMAND),
			 pData, dataLength);
}

void STDMETHODCALLTYPE KSystemConnect::GetRoleListFromDB(char* szAccount, DWORD dwRelayID)
{
	RELAY_DATA data;
	data.ProtocolFamily = pf_relay;
	data.ProtocolID = relay_c2c_data;
	data.nToIP = g_dwLastOpenIP;
	data.nToRelayID = dwRelayID;
	data.nFromRelayID = 0;
	data.nFromIP = 0;
	data.routeDateLength = sizeof(GM_GET_ROLE_LIST_CMD);
	GM_GET_ROLE_LIST_CMD exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_getrolelist;
	SendData(&data, sizeof(data), &exe, sizeof(exe));
}

void STDMETHODCALLTYPE KSystemConnect::SetPlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength)
{
	GM_SET_ROLE_DATA_COMMAND exe;
	strncpy(exe.AccountName, szAccount, LOGIN_USER_ACCOUNT_MAX_LEN);
	exe.ProtocolFamily = pf_gamemaster;
	exe.ProtocolType = gm_c2s_setrole;
	exe.wSetID = ninfo;
	exe.wLength = dataLength;
		
	SendRelayData(szAccount, &exe, sizeof(GM_SET_ROLE_DATA_COMMAND), pData, dataLength);
}

void STDMETHODCALLTYPE KSystemConnect::SendMessage(char* szAccount, char* szRole, char* szMessage)
{
	size_t sentlen = strlen(szMessage);
	if (sentlen <= 0)
		return;

	size_t chatsize = sizeof(CHAT_SOMEONECHAT_SYNC) + sentlen;
	size_t pckgsize = sizeof(BYTE) + chatsize;

	BYTE* pFamily = (BYTE*)_alloca(pckgsize);
	*pFamily = pf_chat;
	CHAT_SOMEONECHAT_SYNC* pCscSync = (CHAT_SOMEONECHAT_SYNC*)(pFamily + 1);
	pCscSync->ProtocolType = chat_someonechat;
	pCscSync->wSize = chatsize - 1;
	pCscSync->packageID = -1;
	strcpy(pCscSync->someone, "gm-kingsoft");
	pCscSync->sentlen = sentlen;
	memcpy(pCscSync + 1, szMessage, sentlen);

	SendRelayData(szAccount, pFamily, pckgsize);
}

void STDMETHODCALLTYPE KSystemConnect::ListenChat(bool bOpen)
{
	assert(0);	//todo...
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
			p->m_pEvent->PlayerAccountAtGW(((GM_HEADER*)pData)->AccountName, 0, -1, -1, -1, NULL);
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
	GM_HEADER* pHeader = (GM_HEADER*)(((char*)pData) + sizeof(RELAY_DATA));
	_ASSERT(pf_gamemaster == pHeader->ProtocolFamily);
	switch (pHeader->ProtocolType)
	{
	case gm_s2c_findplayer:
		{
			_ASSERT(pRelayData->routeDateLength == sizeof(GM_GET_PLAYER_AT_GW_SYNC));
			GM_GET_PLAYER_AT_GW_SYNC* pGMData = (GM_GET_PLAYER_AT_GW_SYNC*)pHeader;
			p->m_pEvent->PlayerAccountAtGW (pHeader->AccountName,
											pRelayData->nFromIP,
											pGMData->nX, pGMData->nY,
											pGMData->nWorldID,
											pGMData->szCurrentRoleName);
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
	_ASSERT(pAskwayData->seekMethod == rm_gm);
	EXTEND_HEADER* pEH = (EXTEND_HEADER*)(pAskwayData + 1);
	switch (pEH->ProtocolFamily)
	{
	case pf_playercommunity:
		{
			_ASSERT(playercomm_channelchat == pEH->ProtocolID);
			CHAT_MSG_EX* pChatData = (CHAT_MSG_EX*)pEH;
			p->m_pEvent->PlayerMessage(pChatData->m_szAccountName, pChatData->m_szSourceName,
									   (char*)(pChatData + 1), pChatData->SentenceLength, pAskwayData->nFromIP);
		}
		break;
	case pf_gamemaster:
		{
			if (gm_s2c_getrolelist == pEH->ProtocolID)
			{
				_ASSERT(pAskwayData->routeDateLength >= sizeof(GM_GET_ROLE_LIST_SYNC));
				GM_GET_ROLE_LIST_SYNC* pGMData = (GM_GET_ROLE_LIST_SYNC*)pEH;
				p->m_pEvent->PlayerRoleList(pGMData->AccountName, pGMData->wRoleCount, (pGMData + 1));
			}
			else if (gm_s2c_getrole == pEH->ProtocolID)
			{
				_ASSERT(pAskwayData->routeDateLength >= sizeof(GM_GET_ROLE_DATA_SYNC));
				GM_GET_ROLE_DATA_SYNC *pNPL = (GM_GET_ROLE_DATA_SYNC *)pEH;
				p->m_pEvent->PlayerInfo(pNPL->AccountName, pNPL->wGetID, pNPL + 1, pNPL->wLength);
			}
			else
			{
				_ASSERT(gm_s2c_findplayer == pEH->ProtocolID);
				_ASSERT(pAskwayData->routeDateLength == sizeof(GM_GET_PLAYER_AT_GW_SYNC));
				GM_GET_PLAYER_AT_GW_SYNC* pGMData = (GM_GET_PLAYER_AT_GW_SYNC*)pEH;
				p->m_pEvent->PlayerAccountAtGW (pGMData->AccountName,
												pAskwayData->nFromIP,
												pGMData->nX, pGMData->nY,
												pGMData->nWorldID,
												pGMData->szCurrentRoleName);
			}
		}
		break;
	case pf_normal:
		{
			_ASSERT(s2c_gatewayinfo == pEH->ProtocolID);
			KServerInfo* pInfo = (KServerInfo*)pInfo;
			_ASSERT(pInfo->nServerType == server_Logout);
			p->m_pEvent->GWSubLogout(pAskwayData->nFromIP, pInfo->nValue);
		}
		break;
	default:
		_ASSERT(0);
		break;
	};
}

void STDMETHODCALLTYPE KSystemConnect::GetGWRelayID(char* szGWName)
{
	KServerInfo user;
	user.Size = sizeof(KServerInfo);
	user.Type = ServerInfo;
	user.Version = ACCOUNT_CURRENT_VERSION;
	user.Operate = 0;
	strncpy(user.Account, szGWName, LOGIN_USER_ACCOUNT_MAX_LEN);
	user.nServerType = server_GWRelayID;
	user.nValue = 0;
	SendProtocol(c2s_gatewayinfo, &user, user.Size);
}

void STDMETHODCALLTYPE KSystemConnect::BroadCastChat(DWORD dwRelayID, char* szChatMsg)
{
	RELAY_DATA data;
	data.ProtocolFamily = pf_relay;
	data.ProtocolID = relay_c2c_data;
	data.nToIP = g_dwLastOpenIP;
	data.nToRelayID = dwRelayID;
	data.nFromRelayID = 0;
	data.nFromIP = 0;
	data.routeDateLength = strlen(szChatMsg) + 1 + sizeof(GM_BROADCAST_CHAT_CMD);

	GM_BROADCAST_CHAT_CMD* msgChat = (GM_BROADCAST_CHAT_CMD*)_alloca(data.routeDateLength);

	msgChat->ProtocolFamily = pf_gamemaster;
	msgChat->ProtocolType = gm_c2s_broadcast_chat;
	strncpy(msgChat->AccountName, "gm-kingsoft", sizeof(msgChat->AccountName));
	msgChat->wSentenceLen = strlen(szChatMsg);
	strcpy((char*)(msgChat + 1), szChatMsg);

	SendData(&data, sizeof(data), msgChat, data.routeDateLength);
}
