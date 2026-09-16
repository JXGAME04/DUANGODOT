//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMCore.h
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 15:01:12
//	文件说明		:	GM.exe 核心类 KGameMasterCore 定义
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __GMCORE_H__
#define __GMCORE_H__

#ifndef _VECTOR_
#include <vector>
using namespace std;
#endif

#pragma	pack(push, 1)

#define STR_BUFFER_LEN	64

typedef struct tagGameWorld
{
	CString		strGameWorldName;
	DWORD		dwIP;
	int			nOnlinePlayerCount;

	tagGameWorld(){ dwIP = 0; nOnlinePlayerCount = 0; }
	tagGameWorld(LPCTSTR szGWName, DWORD dwIPHere)
	{
		ASSERT(strlen(szGWName) && dwIPHere && dwIPHere != 0xFFFFFFFF);
		strGameWorldName	= szGWName;
		dwIP				= dwIPHere;
		nOnlinePlayerCount	= 0;
	}
	tagGameWorld& operator = (const tagGameWorld& that)
	{
		strGameWorldName	= that.strGameWorldName;
		dwIP				= that.dwIP;
		nOnlinePlayerCount	= that.nOnlinePlayerCount;
		return *this;
	}
}GAMEWORLD;

interface ISystemConnect;
interface ISystemOnline;

typedef struct tagPlayerInfo
{
	DWORD	dwNameID;
	WORD	wInfo;
	void*	pData;	//todo...
}PLAYERINFO;

#ifndef MAX_PLAYER_PER_ACCOUNT
#define MAX_PLAYER_PER_ACCOUNT 3
#endif

enum enumGMGetRoleCmds
{
	gm_role_entergame_position = 0,	//获取玩家进入游戏的位置，返回 GM_GET_ROLE_DATA_SUB_ENTER_POS
};

typedef struct tagGMGetRoleEnterGamePos
{
	int nSubWorldID;
	int	nEnterGameX;
	int nEnterGameY;
}GM_ROLE_DATA_SUB_ENTER_POS;

typedef struct tagSystemExchangeData
{
	vector<GAMEWORLD>	aryGameWorlds;
	int					nRegisterCount;
	int					nOnlineCount;
	int					nPing;
	DWORD				dwIPGWIncludePlayer;
	DWORD				dwRelayIDIncludePlayer;
	DWORD				dwRelayIDOfGameWorld;

	int					nCurX;
	int					nCurY;
	int					nCurWorldID;
	CString				strCurrentRole;
	CString				strAccount;

	PLAYERINFO						PlayerInfo;
	GM_ROLE_DATA_SUB_ENTER_POS posInfo;
	CStringList			lstJailWaitingForFree;	//等待解禁的玩家列表
	CStringList			lstRoleName;

	HANDLE hEventGetPlayerAtGWDone;
	HANDLE hEventPingDone;
	HANDLE hEventGetPlayerInfoDone;
	HANDLE hEventGetRoleListDone;
	HANDLE hEventGetGWRelayIDDone;
}EXCHANGEDATA_SYS, FAR* LPEXCHANGEDATA_SYS;

class KSystemEventHandler;
class S3PDBConVBC;
class KGameMasterCore
{
friend class CGMDlg;
public:
	KGameMasterCore(ISystemOnline*);
	virtual ~KGameMasterCore();

	static int s_nJailWorldID;
	static int s_nJailX;
	static int s_nJailY;
protected:
	INT						m_nPingTimerCount;
	ISystemOnline*			m_pISystemOnline;
	ISystemConnect*			m_pISystemConnect;
	KSystemEventHandler*	m_pSysHandler;

	EXCHANGEDATA_SYS	m_ExgSYS;		//Critical Data

	CRITICAL_SECTION	m_CS;
	BOOL				m_bConnected;
	DWORD				m_dwLastUseDBTime;
	DWORD				m_dwLastUseDBTimeSQL;

	S3PDBConVBC* m_pConn;
	S3PDBConVBC* m_pConnSQL;
	S3PDBConVBC* GetDB(DWORD nSleep);
	S3PDBConVBC* GetSQLDB(DWORD nSleep);
	void RefreshLastUseDBTime();
	void RefreshLastUseDBTimeForSQL();
	void DoJailProcess(DWORD dwTickCount);
	void DoShutupProcess(DWORD dwTickCount);
	BOOL WhichRelayIsPlayerIn(DWORD& dwID, LPCSTR szAccount);
	BOOL WhichRelayIsTheGameWorld(DWORD& dwID, LPCSTR szGWName);
	BOOL SetPlayerPosInDB(LPCSTR szRole, LPCSTR szGWName,
						  int nWorldID, int nX, int nY);
	BOOL GetPlayerPosInDB(LPCSTR szRole, LPCSTR szGWName,
						  int& nWorldID, int& nX, int& nY);

public:
	void Breathe(DWORD dwTickCount);

	BOOL GetGWByName(GAMEWORLD& gw, LPCSTR szGWName);
	BOOL GetGWByIndex(GAMEWORLD& gw, int nIndex);
	int CountOfGameWorlds();
	int GetOnlineCount(LPCSTR szGWName);
	int GetRegisterCount();
	int PingSystem();
	void JustPingSystem();
	void DisconnectionOccur() { m_bConnected = FALSE; }
	BOOL IsConnectionOK() { return m_bConnected; }

	HRESULT BroadCastChatToGameWorlds(LPCSTR szGWName, LPCSTR szCharMessage);
	HRESULT GetRoleList(LPCSTR szAccount, LPCSTR szGWName, CStringList& lstRoleNames);
	HRESULT PutInJail(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName, int nMinutes);
	HRESULT JailJudge(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName);
	HRESULT SaveMessageToDB(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGameWorld, LPCSTR szChat);
	HRESULT SaveDisconnectionToDB(LPCSTR szGameWorld, LPCSTR szSubIP);
	HRESULT FreeFromJail(LPCSTR szAccount, LPCSTR szRole, LPCSTR szGWName);
	HRESULT Shutup(LPCSTR szAccount, int nMinutes);
	HRESULT ShutupJudge(LPCSTR szAccount);
	HRESULT FreeChat(LPCSTR szAccount);

	HRESULT WhichGWIsPlayerIn(GAMEWORLD& gw, LPCSTR szAccount);
	HRESULT WhichGWIsPlayerInRealTime(GAMEWORLD& gw, CString& rstrCurRole,
									  int& rnCurX, int& rnCurY, int& rnCurWorldID,
									  LPCSTR szAccount);
	HRESULT UnlockPlayerAccount(LPCSTR szName, LPCSTR szPassword);

	//todo...
	HRESULT GetPlayerInfo();
	HRESULT SetPlayerInfo();

	//todo...
	HRESULT EnableXXX();
	HRESULT DisableXXX();
	

	//gameworld
	BOOL InitGameWorld(LPCSTR szGWName);
	HRESULT ExecutePlayer(LPCSTR szAccount, LPCSTR szScriptName);
	HRESULT TrackingPlayer(LPCSTR szAccount);
	HRESULT SendMessage(LPCSTR szAccount, LPCSTR szRole, LPCSTR szMessage);
};

#pragma	pack(pop)

#endif //__GMCORE_H__
