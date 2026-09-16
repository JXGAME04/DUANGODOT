#pragma once

#include "iNet.h"
#include "iServer.h"

struct stServerInfo
{
	CHAR strServerIP[64];
	unsigned short wServerPort;
};

class cNetMsg;
class iEncode;
class iPack;

//typedef UINT (*HandleServerMessageFunc)(stMsg* pMsg,DWORD size);

class iClient : public iServerLink
{
public:
//	virtual DWORD AddRef() = 0;
//	virtual DWORD Release() = 0;

	//call it once but should beforce any create!
	//if return false please close the server!
	virtual BOOL InitSocket() = 0;
	//call it after close all server
	virtual BOOL CleanSocket() = 0;

	virtual void Bind(iEncode* pEncode,iPack* pPack) = 0;
	virtual BOOL SendMessage(const char* buffer, DWORD size) = 0;
//	virtual BOOL SendMessageWithoutSize(char* buffer, DWORD size) = 0;

//	virtual BOOL Create() = 0;
	virtual BOOL Connect(stServerInfo& info) = 0;
//	virtual BOOL IsConnect() = 0;
	//dont use close avoid the name conflict
//	virtual BOOL CloseClient() = 0;

//	virtual BOOL SendMessage(stMsg* pMsg,DWORD size) = 0;
//	virtual stMsg* PopMessage() = 0;
//	virtual void RegisterHandle(HandleMessageFunc func) = 0;

	//server的消息
	virtual void RegisterServerHandle(HandleMessageFunc func) = 0;
	//自身的消息
	virtual void RegisterNetHandle(HandleMessageFunc func) = 0;
};


enum ClientType{
	//match the iserver provide
	Server_Client,
	//mud client
	Mud_Client,
};

_declspec(dllexport) iClient* Net_CreateClient(ClientType verson = Server_Client);