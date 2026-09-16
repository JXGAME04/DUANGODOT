//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	EventHandler.h
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 14:55:22
//	文件说明		:	同 jxgm 模块挂接的事件相应模块定义
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __EVENTHANDLER_H__
#define __EVENTHANDLER_H__

#ifndef __JXGM_I_H__
#include "../JXGM/Headers/jxgm_i.h"
#endif

typedef struct tagSystemExchangeData EXCHANGEDATA_SYS;
typedef struct tagGameWorldExchangeData EXCHANGEDATA_GW;

class KSystemEventHandler : public ISystemEvent
{
public:
	KSystemEventHandler(EXCHANGEDATA_SYS*);
	virtual ~KSystemEventHandler();
protected:
	EXCHANGEDATA_SYS* m_pExgSys;
public:
	void STDMETHODCALLTYPE GWLogin(char* szGWName, DWORD nIP);
	void STDMETHODCALLTYPE GWLogout(char* szGWName, DWORD nIP);
	void STDMETHODCALLTYPE OnlinePlayerCount(char* szGWName, DWORD nCount);
	void STDMETHODCALLTYPE RegisterUserCount(DWORD nCount);
	void STDMETHODCALLTYPE PlayerAccountAtGW(char* szAccount, DWORD nIP);
	void STDMETHODCALLTYPE Ping(DWORD nTick);
	void STDMETHODCALLTYPE ConnectClosed();

	void STDMETHODCALLTYPE PlayerTrackingData(char* szAccount,void* pData, size_t dataLength);
	void STDMETHODCALLTYPE PlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength);
	void STDMETHODCALLTYPE PlayerMessage(char* szAccountFrom, char* szAccountTo, char* szMessage);
};

#endif //__EVENTHANDLER_H__
