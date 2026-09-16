// -------------------------------------------------------------------------
//	文件名		：	KGWConnect.h
//	创建者		：	万里
//	创建时间	：	2003-6-2 19:49:31
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __KGWCONNECT_H__
#define __KGWCONNECT_H__

#include "Headers\jxgm_i.h"
#include "KConnect.h"

class KGSConnect : public KConnect
{
public:
	KGSConnect(IGWEvent* pEvent);
	virtual ~KGSConnect();

protected:
	virtual void ConnectClose();
	
	IGWEvent* m_pEvent;

	static void P_PlayerLoginorOut(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerTrackingData(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerInfo(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerMessage(void* pThis, const void *pData, size_t dataLength);
};

class GSConnect_CS : public K_CS
{
public:
	KGSConnect* m_RelayConnect;
};

class KGWConnect : public IGWConnect, public KConnect
{
public:
	KGWConnect();
	virtual ~KGWConnect();

protected:
	IGWEvent* m_pEvent;
	HANDLE m_hLogined;
	BOOL m_bLogined;

	GSConnect_CS m_GSConnects;

	static void P_Logined(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerLogin(void* pThis, const void *pData, size_t dataLength);
	static void P_Ping(void* pThis, const void *pData, size_t dataLength);

	virtual HRESULT STDMETHODCALLTYPE Login(char* szGMAccount, char* szPassword, IGWEvent* pEvent);
	virtual void STDMETHODCALLTYPE Close();
	virtual void STDMETHODCALLTYPE ExecuteAction(char* szAccount, char* szScript);
	virtual void STDMETHODCALLTYPE EnableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE DisableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE TrackingPlayer(char* szAccount, bool bTrack);
	virtual void STDMETHODCALLTYPE GetPlayerInfo(char* szAccount, WORD ninfo);
	virtual void STDMETHODCALLTYPE SetPlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE SendMessage(char* szAccount, char* szMessage);
	virtual void STDMETHODCALLTYPE SendPing();
	virtual void STDMETHODCALLTYPE ListenChat(bool bOpen);
	
	KConnect* GetRelayConnect();
	void ReleaseRelayConnect();

	virtual void ConnectClose();
};

#endif // __KGWCONNECT_H__