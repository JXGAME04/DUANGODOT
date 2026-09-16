// -------------------------------------------------------------------------
//	文件名		：	jxgm_i.h
//	创建者		：	万里
//	创建时间	：	2003-6-2 18:59:12
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __JXGM_I_H__
#define __JXGM_I_H__

struct IGWConnect;
struct ISystemConnect;

struct ISystemOnline
{
	virtual HRESULT STDMETHODCALLTYPE OpenSystemConnect(DWORD nIP, short nPort, ISystemConnect** ppConnect) = 0;
};

struct ISystemEvent
{
	virtual void STDMETHODCALLTYPE GWLogin(char* szGWName, DWORD nIP) = 0;
	virtual void STDMETHODCALLTYPE GWLogout(char* szGWName, DWORD nIP) = 0;
	virtual void STDMETHODCALLTYPE OnlinePlayerCount(char* szGWName, DWORD nCount) = 0;
	virtual void STDMETHODCALLTYPE RegisterUserCount(DWORD nCount) = 0;
	//nIP = 0 not online, nIP = 0xFFFFFFFF account not exist
	virtual void STDMETHODCALLTYPE PlayerAccountAtGW(char* szAccount, DWORD nIP) = 0;
	virtual void STDMETHODCALLTYPE Ping(DWORD nTick) = 0;
	virtual void STDMETHODCALLTYPE ConnectClosed() = 0;

	virtual void STDMETHODCALLTYPE PlayerTrackingData(char* szAccount, void* pData, size_t dataLength) = 0;
	virtual void STDMETHODCALLTYPE PlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength) = 0;
	virtual void STDMETHODCALLTYPE PlayerMessage(char* szAccountFrom, char* szAccountTo, char* szMessage) = 0;
};

struct ISystemConnect
{
	virtual HRESULT STDMETHODCALLTYPE Login(char* szGMAccount, char* szPassword, ISystemEvent* pEvent) = 0;
	virtual void STDMETHODCALLTYPE Close() = 0;
	virtual void STDMETHODCALLTYPE GetOnlinePlayerCount(char* szGWName) = 0;
	virtual void STDMETHODCALLTYPE GetRegisterUserCount() = 0;
	virtual void STDMETHODCALLTYPE GetPlayerAccountAtGW(char* szAccount) = 0;
	virtual void STDMETHODCALLTYPE UnlockPlayerAccount(char* szAccount, char* szPassword) = 0;
	virtual void STDMETHODCALLTYPE SendPing() = 0;
	virtual void STDMETHODCALLTYPE SetOption(bool bOnlinePlayer, bool bRegisterUser, DWORD nAutoTime) = 0;

	virtual void STDMETHODCALLTYPE ExecuteAction(char* szAccount, char* szScript) = 0;
	virtual void STDMETHODCALLTYPE EnableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength) = 0;
	virtual void STDMETHODCALLTYPE DisableProtocol(char* szAccount, BYTE nProtocol, void* pData, size_t dataLength) = 0;
	virtual void STDMETHODCALLTYPE TrackingPlayer(char* szAccount, bool bTrack) = 0;
	virtual void STDMETHODCALLTYPE GetPlayerInfo(char* szAccount, WORD ninfo) = 0;
	virtual void STDMETHODCALLTYPE SetPlayerInfo(char* szAccount, WORD ninfo, void* pData, size_t dataLength) = 0;
	virtual void STDMETHODCALLTYPE SendMessage(char* szAccount, char* szMessage) = 0;
	virtual void STDMETHODCALLTYPE ListenChat(bool bOpen) = 0;
};

STDAPI CreateSystemOnline(ISystemOnline** ppSystem);
STDAPI DestroySystemOnline();

#endif // __JXGM_I_H__