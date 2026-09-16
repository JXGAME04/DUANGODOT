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

#define STR_BUFFER_LEN	64
#define INI_FILENAME	"GMConn.ini"

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

typedef struct tagSystemExchangeData
{
	vector<GAMEWORLD>	aryGameWorlds;
	int					nRegisterCount;
	int					nOnlineCount;
	int					nPing;
	DWORD				dwIPGWIncludePlayer;
	CString				strAccount;
	PLAYERINFO			PlayerInfo;

	HANDLE hEventGetPlayerAtGWDone;
	HANDLE hEventPingDone;
	HANDLE hEventGetPlayerInfoDone;
}EXCHANGEDATA_SYS, FAR* LPEXCHANGEDATA_SYS;

class KSystemEventHandler;
class KGameMasterCore
{
public:
	KGameMasterCore(ISystemOnline*);
	virtual ~KGameMasterCore();

protected:
	ISystemOnline*			m_pISystemOnline;
	ISystemConnect*			m_pISystemConnect;
	KSystemEventHandler*	m_pSysHandler;

	EXCHANGEDATA_SYS	m_ExgSYS;	//Critical Data

	CRITICAL_SECTION	m_CS;
public:
	//pay system
	BOOL GetGWByName(GAMEWORLD& gw, LPCSTR szGWName);
	int GetOnlineCount(LPCSTR szGWName);
	int GetRegisterCount();
	int PingSystem();

	HRESULT WhichGWIsPlayerIn(GAMEWORLD& gw, LPCSTR szAccount);
	HRESULT UnlockPlayerAccount(LPCSTR szName, LPCSTR szPassword);

	//todo...
	HRESULT GetPlayerInfo();
	HRESULT SetPlayerInfo();

	//todo...
	HRESULT EnableXXX();
	HRESULT DisableXXX();
	

	//gameworld
	BOOL InitGameWorld(LPCSTR szGWName);
	HRESULT GetPlayerAccount(LPCSTR szPlayerName, BSTR* pbstrAccountArray);
	HRESULT ExecutePlayer(LPCSTR szAccount, LPCSTR szScriptName);
	HRESULT TrackingPlayer(LPCSTR szAccount);
	HRESULT SendMessage(LPCSTR szAccount, LPCSTR szMessage);
};

#endif //__GMCORE_H__
