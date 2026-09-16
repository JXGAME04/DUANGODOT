// -------------------------------------------------------------------------
//	文件名		：	KSystemConnect.h
//	创建者		：	万里
//	创建时间	：	2003-6-2 20:25:48
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __KSYSTEMCONNECT_H__
#define __KSYSTEMCONNECT_H__

#include "Headers\jxgm2_i.h"
#include "KConnect.h"
#include "KGmProtocol.h"

class KSystemConnect : public ISystemConnect2, public KConnect
{
public:
	KSystemConnect(DWORD nSerIP);
	virtual ~KSystemConnect();
	void ConnectClose();

protected:
	DWORD m_nRelyID;
	DWORD m_nFlag;
	ISystemEvent* m_pEvent;
	HANDLE m_hLogined;
	BOOL m_bLogined;
	BYTE m_MacAddress[6];
	char m_szAccount[64];

	static void P_AskwayProcess(void* pThis, const void *pData, size_t dataLength);
	static void P_Logined(void* pThis, const void *pData, size_t dataLength);
	static void P_ServerInfo(void* pThis, const void *pData, size_t dataLength);
	static void P_Ping(void* pThis, const void *pData, size_t dataLength);
	static void P_MissingPlayer(void* pThis, const void *pData, size_t dataLength);
	static void P_GWReturnMsg(void* pThis, const void *pData, size_t dataLength);
	static void P_FreezeReturn(void* pThis, const void *pData, size_t dataLength);
	static void P_UnlockReturn(void* pThis, const void *pData, size_t dataLength);

	bool CanAction(char* szScript);
	bool NeedGMLogEvent(char* szAccount);
	bool AutoReturnAction(char* szAccountName, char* szMessage);
	
	virtual HRESULT STDMETHODCALLTYPE Login(char* szGMAccount, char* szPassword, ISystemEvent* pEvent);
	virtual void STDMETHODCALLTYPE Close();
	virtual void STDMETHODCALLTYPE SendPing();
	virtual void STDMETHODCALLTYPE ExecuteAction(char* szAccount, char* szScript, DWORD nReturn);
	virtual void STDMETHODCALLTYPE ExecuteAction(char* szRole, DWORD nIP, char* szScript, DWORD nReturn);
	virtual void STDMETHODCALLTYPE ExecuteAction(DWORD nIP, char* szScript, DWORD nReturn, DWORD nSendIP);

	virtual void STDMETHODCALLTYPE UnlockPlayerAccount(DWORD Operate, char* szAccount);
	virtual void STDMETHODCALLTYPE FreezePlayerAccount(DWORD Operate, char* szAccount);
	virtual void STDMETHODCALLTYPE GetOnlinePlayerCount(DWORD Operate, char* szGWName);
	virtual void STDMETHODCALLTYPE GetRegisterUserCount(DWORD Operate);
	//virtual void STDMETHODCALLTYPE GetPlayerAccountAtGW(DWORD Operate, char* szAccount);
	virtual void STDMETHODCALLTYPE GetGWRelayID(DWORD Operate, char* szGWName);
	//virtual void STDMETHODCALLTYPE GetPlayerAccountAtGWID(DWORD Operate, char* szAccount);

	//ISystemConnect2
	virtual DWORD STDMETHODCALLTYPE GetIP();
	virtual DWORD STDMETHODCALLTYPE GetRelayID();
	virtual void STDMETHODCALLTYPE SetFlag(int nFlag);
};

#endif // __KSYSTEMCONNECT_H__