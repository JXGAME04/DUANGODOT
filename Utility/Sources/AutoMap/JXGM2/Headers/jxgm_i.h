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
	virtual void STDMETHODCALLTYPE GWLogin(char* szGWName, DWORD nIP) = 0;	//当S3Relay启动后的消息
	virtual void STDMETHODCALLTYPE GWLogout(char* szGWName, DWORD nIP) = 0;	//当S3Relay退出后的消息
	virtual void STDMETHODCALLTYPE GWRelayID(char* szGWName, DWORD nRelayID) = 0;		//当S3Relay启动后的Relay总服务器给出的服务器RelayID消息,用于向指定S3Reay发送ExecuteAction中
	virtual void STDMETHODCALLTYPE GWSubLogout(DWORD nGWIP, DWORD nSubIP) = 0;//当S3Relay下的某台GameWorld退出后的消息
	virtual void STDMETHODCALLTYPE Ping(DWORD nTick) = 0;	//Relay总服务器回馈的Ping消息
	virtual void STDMETHODCALLTYPE ConnectClosed() = 0;	//与Relay总服务器断线消息
	virtual void STDMETHODCALLTYPE PlayerMessage(char* szAccountFrom, char* szRole, char* szMessage, int nMsgLen, DWORD dwGWIP) = 0;	//特定玩家从特定位置发送的消息
	virtual void STDMETHODCALLTYPE LosePlayer(char* szAccount, DWORD dwGWIP) = 0;	//当ExecuteAction后没有找到指定目标时的回馈

	//以下消息是直接由ISystemConnect相关调用的返回消息,注意Operate与发出的Operate是一致的
	virtual void STDMETHODCALLTYPE AccountUserReturn(DWORD Operate, char* szAccount, int nReturn) = 0;
	virtual void STDMETHODCALLTYPE OnlinePlayerCount(DWORD Operate, char* szGWName, DWORD nCount) = 0;
	virtual void STDMETHODCALLTYPE RegisterUserCount(DWORD Operate, DWORD nCount) = 0;
};


struct ISystemConnect
{
	virtual HRESULT STDMETHODCALLTYPE Login(char* szGMAccount, char* szPassword, ISystemEvent* pEvent) = 0;	//连接Relay总服务器
	virtual void STDMETHODCALLTYPE Close() = 0;	//程序退出时调用
	virtual void STDMETHODCALLTYPE SendPing() = 0;	//定期发送Ping,否则将失去连接
	//向指定帐号的在线玩家发送指令,未找到将收到LosePlayer消息
	//szScript长度不超过128,szScript中的字符串参数全部用两对中括弧括住[[]]
	//nReturn!=0代表希望有返回消息,PlayerMessage("GM", nReturn, 结果)
	virtual void STDMETHODCALLTYPE ExecuteAction(char* szAccount, char* szScript, DWORD nReturn) = 0;	
	//向指定S3Relay和角色名的在线玩家发送指令,未找到将收到LosePlayer消息
	////szScript长度不超过128,szScript中的字符串参数全部用两对中括弧括住[[]]
	//nReturn!=0代表希望有返回消息,PlayerMessage("GM", nReturn, 结果)
	virtual void STDMETHODCALLTYPE ExecuteAction(char* szRole, DWORD nRelayID, char* szScript, DWORD nReturn) = 0;
	//向指定S3Relay发送指令
	////szScript长度不超过128,szScript中的字符串参数全部用两对中括弧括住[[]]
	//nReturn!=0代表希望有返回消息,PlayerMessage("GM", nReturn, 结果)
	//nSendIP = 0代表直接在S3Relay上执行,nSendIP = INADDR_BROADCAST代表要向整个服务器组广播,其他代表向服务器组中某个game发送
	virtual void STDMETHODCALLTYPE ExecuteAction(DWORD nRelayID, char* szScript, DWORD nReturn, DWORD nSendIP) = 0;

	//以下消息是直接发送给Relay总服务器的,注意Operate将与返回的Operate是一致的
	virtual void STDMETHODCALLTYPE UnlockPlayerAccount(DWORD Operate, char* szAccount) = 0;	//解锁数据库中的玩家锁,注意此函数要慎重使用,一定要确保玩家不在线,否则引起错误由调用者负责
	virtual void STDMETHODCALLTYPE FreezePlayerAccount(DWORD Operate, char* szAccount) = 0;	//冻结数据库中的玩家锁,同时踢玩家下线
	virtual void STDMETHODCALLTYPE GetOnlinePlayerCount(DWORD Operate, char* szGWName) = 0;	//获得指定S3Relay的在线人数
	virtual void STDMETHODCALLTYPE GetRegisterUserCount(DWORD Operate) = 0;	//获得总注册人数
};

STDAPI CreateSystemOnline(ISystemOnline** ppSystem);
STDAPI DestroySystemOnline();

#endif // __JXGM_I_H__