#pragma once

#ifdef _SERVER
class iServer;
iServer* AppGetServer();
#else
class iClient;
iClient* AppGetClient();

enum NetStatus{NetStatus_Connect, NetStatus_Verify, NetStatus_Send, NetStatus_Quit,};
//void DisableAll();
void EnableNetStatus(NetStatus e, BOOL b);
#endif

class cWorld;
cWorld* AppGetWorld();

void OutputString(LPCSTR szAccount,LPCSTR szMessage);
