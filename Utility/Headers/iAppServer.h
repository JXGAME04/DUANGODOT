#pragma once

#include "iPublic.h"

class iServer;
class iDatabase;
class iServerLink;
struct stMsg;

typedef void (*OutputStringFunc) (LPCSTR szAccount,LPCSTR szMessage);

class iAppServer
{
public:
	//create it after client connected and entergameworld
	virtual BOOL Create(iServer* pServer,iDatabase* pDatabase) = 0;
	virtual BOOL RunThread() = 0;
	virtual BOOL EndThread() = 0;

	virtual void PlayerDisconnect(void* p) = 0;

	virtual void RegistOutputStringFunc(OutputStringFunc f) = 0;
};

_declspec(dllexport) iAppServer* AppServer_GetAppServer();
