#pragma once

#include "stMsg.h"
#include "../appCommon/appPublic.h"

enum {
	Msg_App2Db_VerifyPassword,
	Msg_Db2App_VerifyPassword,
	
	Msg_Db2App_AccountInfo,
	Msg_App2Db_AccountInfo,

	Msg_Db2App_PlayerInfo,
	Msg_App2Db_PlayerInfo,
};

class iServerLink;
class cPlayer;

struct stMsg_App2Db_VerifyPassword : public stMsg
{
	stMsg_App2Db_VerifyPassword() : stMsg(Msg_App2Db_VerifyPassword){}
	iServerLink* pLink;
	char szAccount[MAX_ACCOUNT];
	char szPassword[MAX_PASSWORD];
};

struct stMsg_Db2App_VerifyPassword : public stMsg
{
	stMsg_Db2App_VerifyPassword() : stMsg(Msg_Db2App_VerifyPassword){}
	HRESULT hr;
	iServerLink* pLink;
	char szAccount[MAX_ACCOUNT];
	char szPassword[MAX_PASSWORD];	
};

struct stMsg_App2Db_AccountInfo : public stMsg
{
	stMsg_App2Db_AccountInfo() : stMsg(Msg_App2Db_AccountInfo){}
	iServerLink* pLink;
	char szAccount[MAX_ACCOUNT];
};

struct stMsg_Db2App_AccountInfo : public stMsg
{
	stMsg_Db2App_AccountInfo() : stMsg(Msg_Db2App_AccountInfo){}
	HRESULT hr;
	iServerLink* pLink;
	char szAccount[MAX_ACCOUNT];
	int nPlayer;
	stAccountPlayerInfo aInfo[MAX_PLAYEROFACCOUNT];
};

struct stMsg_App2Db_PlayerInfo : public stMsg
{
	stMsg_App2Db_PlayerInfo () : stMsg(Msg_App2Db_PlayerInfo){}
	iServerLink* pLink;
	char szAccount[MAX_ACCOUNT];
	char szName[MAX_NAME];
};

struct stMsg_Db2App_PlayerInfo : public stMsg
{
	stMsg_Db2App_PlayerInfo () : stMsg(Msg_Db2App_PlayerInfo){}
	HRESULT hr;
	iServerLink* pLink;
	stPlayerInfo info;
};

typedef UINT (*HandleDatabaseMessageFunc)(stMsg* pMsg, DWORD msgSize, void* pPlayer);

class iDatabase
{
public:
	virtual DWORD AddRef() = 0;
	virtual DWORD Release() = 0;
	
	virtual HRESULT Open(LPCSTR szDatabase, LPCSTR szAccount, LPCSTR szPassword) = 0;
	virtual HRESULT Close() = 0;

	virtual BOOL RunThread() = 0;
	virtual BOOL EndThread() = 0;
	
	virtual void BindPlayer(void* p) = 0;
	virtual void RegistHandle(HandleDatabaseMessageFunc func) = 0;

	virtual BOOL VerifyPassword(LPCSTR szAccount, LPCSTR szPassword, iServerLink* pLink)	= 0;
	virtual BOOL QueryAccountInfo(LPCSTR szAccount, iServerLink* pPlayer)	= 0;
	virtual BOOL QueryPlayerInfo(LPCSTR szAccount, LPCSTR szName, iServerLink* pPlayer)	= 0;
};


_declspec(dllexport) iDatabase* Database_CreateDatabase();