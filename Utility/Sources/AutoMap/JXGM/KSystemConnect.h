// -------------------------------------------------------------------------
//	文件名		：	KSystemConnect.h
//	创建者		：	万里
//	创建时间	：	2003-6-2 20:25:48
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __KSYSTEMCONNECT_H__
#define __KSYSTEMCONNECT_H__

#include "Headers\jxgm_i.h"
#include "KConnect.h"
#include "KGmProtocol.h"

class KSystemConnect : public ISystemConnect, public KConnect
{
public:
	KSystemConnect();
	virtual ~KSystemConnect();

protected:
	ISystemEvent* m_pEvent;
	HANDLE m_hLogined;
	BOOL m_bLogined;
	BYTE m_MacAddress[6];

	static void P_AskwayProcess(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerTrackingData(void* pThis, const void *pData, size_t dataLength);
	static void P_PlayerInfo(void* pThis, const void *pData, size_t dataLength);
	static void P_Logined(void* pThis, const void *pData, size_t dataLength);
	static void P_ServerInfo(void* pThis, const void *pData, size_t dataLength);
	static void P_Ping(void* pThis, const void *pData, size_t dataLength);
	static void P_MissingPlayer(void* pThis, const void *pData, size_t dataLength);
	static void P_GWReturnMsg(void* pThis, const void *pData, size_t dataLength);
	
	virtual HRESULT STDMETHODCALLTYPE Login(char* szGMAccount, char* szPassword, ISystemEvent* pEvent);
	virtual void STDMETHODCALLTYPE Close();
	virtual void STDMETHODCALLTYPE GetOnlinePlayerCount(char* szGWName);
	virtual void STDMETHODCALLTYPE GetRegisterUserCount();
	virtual void STDMETHODCALLTYPE GetPlayerAccountAtGW(char* szAccount);
	virtual void STDMETHODCALLTYPE GetGWRelayID(char* szGWName);
	virtual void STDMETHODCALLTYPE GetPlayerAccountAtGWID(char* szAccount);
	virtual void STDMETHODCALLTYPE GetPlayerAccountAtGWRealTime(char* szAccount);
	virtual void STDMETHODCALLTYPE UnlockPlayerAccount(char* szAccount, char* szPassword);
	virtual void STDMETHODCALLTYPE SendPing();

	virtual void STDMETHODCALLTYPE ExecuteAction(char* szAccount, char* szScript);
	virtual void STDMETHODCALLTYPE EnableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE DisableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE TrackingPlayer(char* szAccount, bool bTrack);
	virtual void STDMETHODCALLTYPE GetRoleListFromDB(char* szAccount, DWORD dwRelayID);
	virtual void STDMETHODCALLTYPE GetPlayerInfo(char* szAccount, WORD ninfo);
	virtual void STDMETHODCALLTYPE GetPlayerInfoFromDB(char* szRole, DWORD dwRelayID, WORD ninfo);
	virtual void STDMETHODCALLTYPE SetPlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE SetPlayerInfoToDB(char* szRole,  DWORD dwRelayID, WORD ninfo, void* pData, size_t dataLength);
	virtual void STDMETHODCALLTYPE SendMessage(char* szAccount, char* szRole, char* szMessage);
	virtual void STDMETHODCALLTYPE ListenChat(bool bOpen);
	virtual void STDMETHODCALLTYPE BroadCastChat(DWORD dwRelayID, char* szCharMsg);
};

#endif // __KSYSTEMCONNECT_H__