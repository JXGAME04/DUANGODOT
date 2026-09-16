/*****************************************************************************************
//	外界访问服务版Core的接口方法定义
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-12-20
------------------------------------------------------------------------------------------
	外界（如界面系统）通过此接口从Core获取游戏世界数据。
*****************************************************************************************/

#ifndef CORESERVERSHELL_H
#define CORESERVERSHELL_H

#include "CoreServerDataDef.h"


//=========================================================
// Core外部客户对core的操作请求的索引定义
//=========================================================
enum SERVER_SHELL_OPERATION_INDEX
{
	SSOI_LAUNCH = 1,			//启动服务
	SSOI_SHUTDOWN,				//关闭服务
	SSOI_BROADCASTING,			//boardcasting
	//uParam = (char*)pMessage
	//nParam = (int)nMsgLen

	SSOI_RELOAD_WELCOME_MSG,	//reload the server's welcome msg

	SSOI_TONG_CREATE,			// relay 帮会创建成功，通知 core 进行相应的处理
	SSOI_TONG_REFUSE_ADD,		// 拒绝帮会加入申请
	SSOI_TONG_ADD,				// relay 帮会成员添加成功，通知 core 进行相应的处理
	SSOI_TONG_CREATE_FAIL,	//guve
};

//=========================================================
// Core外部客户向core获取游戏数据的数据项内容索引定义
//=========================================================
//各数据项索引的相关参数uParam与nParam如果在注释中未提及，则传递定值0。
//如果特别指明返回值含义，则成功获取数据返回1，未成功返回0。
enum GAMEDATA_INDEX
{
	SGDI_CHARACTER_NAME,
	SGDI_LOADEDMAP_ID,
	SGDI_CHARACTER_ACCOUNT,
	SGDI_CHARACTER_ID,
	SGDI_CHARACTER_NETID,
	SGDI_CHARACTER_EXTPOINT,
	SGDI_CHARACTER_EXTPOINTCHANGED,
	SGDI_CHARACTER_FIND,
	SGDI_CHARACTER_SEX,
	SGDI_TONG_APPLY_CREATE,
	SGDI_TONG_APPLY_ADD,
	SGDI_TONG_CHECK_ADD_CONDITION,
	SGDI_TONG_GET_INFO,
	SGDI_TONG_INSTATE_POWER,
	SGDI_TONG_BE_INSTATED,
	SGDI_TONG_KICK_POWER,
	SGDI_TONG_BE_KICKED,
	SGDI_TONG_LEAVE_POWER,
	SGDI_TONG_LEAVE,
	SGDI_TONG_CHANGE_MASTER_POWER,
	SGDI_TONG_CHANGE_AS,
	SGDI_TONG_CHANGE_MASTER,
	SGDI_TONG_GET_TONG_NAMEID,
	SGDI_TONG_LOGIN,
	SGDI_TONG_SEND_SELF_INFO,
	SGDI_TONG_APPLY_RIGHT,
	SGDI_TONG_RIGHT_SENDER_RET,
	SGDI_TONG_BE_RIGHT_RET,
	SGDI_TONG_CHANGERECRUIT_POWER,
	SGDI_TONG_CHANGERECRUIT_RET,
	SGDI_TONG_DELETE,
	SGDI_TONG_CONTRIBMONEY_POWER,
	SGDI_TONG_WITHDRAWMONEY_POWER,
	SGDI_TONG_WITHDRAWMONEY_SUCC,
	SGDI_TONG_STOREOFFER_POWER,
	SGDI_TONG_DISPENSEOFFER_POWER,
	SGDI_TONG_BE_DISPENSEOFFER,
	SGDI_TONG_ASSIGNMONEY_POWER,
	SGDI_TONG_ASSIGNMONEY_RET,
	SGDI_TONG_ASSIGNOFFER_RET,
	SGDI_TONG_TRANSMONEY_POWER,
	SGDI_TONG_STOREBUILDFUND_CHECK,
	SGDI_TONG_STOREBUILDFUND_RET,
	SGDI_TONG_WEEKLYRESET,
	SGDI_TONG_ANNOUNCE_CHECK,
	SGDI_TONG_RECEIVEPRICE_RET,
	SGDI_TONG_PLAYERPRICE_RET,
	SGDI_TONG_CHANGETITLE_CHECK,
	SGDI_TONG_BE_CHANGETITLE,
	SGDI_TONG_ADDWEEKGOAL_RET,
	SGDI_TONG_CHANGETITLEALL_CHECK,
	SGDI_TONG_CHANGETITLEALL_RET,
	SGDI_TONG_CHANGECAMP_CHECK,
	SGDI_TONG_CHANGECAMP_RET,
	SGDI_TONG_ACTION,
};

#ifdef _STANDALONE
class IClient;
#else
struct IClient;
#endif

#ifndef _STANDALONE
struct _declspec (novtable) iCoreServerShell
#else
struct iCoreServerShell
#endif
{
	virtual int  GetLoopRate() = 0;
	virtual void GetGuid(int nIndex, void* pGuid) = 0;
	virtual DWORD GetExchangeMap(int nIndex) = 0;
	virtual bool IsPlayerLoginTimeOut(int nIndex) = 0;
	virtual void RemovePlayerLoginTimeOut(int nIndex) = 0;
	virtual bool IsPlayerExchangingServer(int nIndex) = 0;
	virtual void ProcessClientMessage(int nIndex, const char* pChar, int nSize) = 0;
	virtual void ProcessNewClientMessage(IClient* pTransfer, DWORD dwFromIP, DWORD dwFromRelayID, int nPlayerIndex, const char* pChar, int nSize) = 0;
	virtual void SendNetMsgToTransfer(IClient* pClient) = 0;
	virtual void SendNetMsgToChat(IClient* pClient) = 0;
	virtual void SendNetMsgToTong(IClient* pClient) = 0;
	virtual void ProcessBroadcastMessage(const char* pChar, int nSize) = 0;
	virtual void ProcessExecuteMessage(const char* pChar, int nSize) = 0;
	virtual void ClientDisconnect(int nIndex) = 0;
	virtual void RemoveQuitingPlayer(int nIndex) = 0;
	virtual void* SavePlayerDataAtOnce(int nIndex) = 0;
	virtual bool IsCharacterQuiting(int nIndex) = 0;
	virtual bool CheckProtocolSize(const char* pChar, int nSize) = 0;
	virtual bool PlayerDbLoading(int nPlayerIndex, int bSyncEnd, int& nStep, unsigned int& nParam) = 0;
	virtual int  AttachPlayer(const unsigned long lnID, GUID* pGuid) = 0;
	virtual void GetPlayerIndexByGuid(GUID* pGuid, int* pnIndex, int* plnID) = 0;
	virtual void AddPlayerToWorld(int nIndex) = 0;
	virtual void* PreparePlayerForExchange(int nIndex) = 0;
	virtual void PreparePlayerForLoginFailed(int nIndex) = 0;
	virtual void RemovePlayerForExchange(int nIndex) = 0;
	virtual void RecoverPlayerExchange(int nIndex) = 0;
	virtual int  AddCharacter(int nExtPoint, int nChangeExtPoint, void* pBuffer, GUID* pGuid) = 0;
	virtual int	 AddTempTaskValue(int nIndex, const char* pData) = 0;
	//向游戏发送操作
	virtual int	 OperationRequest(unsigned int uOper, unsigned int uParam, int nParam) = 0;
	//获取连接状况
	virtual int	 GetConnectInfo(KCoreConnectInfo* pInfo) = 0;
//	virtual	BOOL ValidPingTime(int nIndex) = 0;
	//从游戏世界获取数据
	virtual int	 GetGameData(unsigned int uDataId, unsigned int uParam, int nParam) = 0;
	//日常活动，core如果要寿终正寝则返回0，否则返回非0值
	virtual int  Breathe() = 0;
	//释放接口对象
	virtual void Release() = 0;
	virtual void SetSaveStatus(int nIndex, UINT uStatus) = 0;
	virtual UINT GetSaveStatus(int nIndex) = 0;
	virtual BOOL GroupChat(IClient* pClient, DWORD FromIP, unsigned long FromRelayID, DWORD channid, BYTE tgtcls, DWORD tgtid, const void* pData, size_t size) = 0;
	virtual	void SetLadder(void* pData, size_t uSize) = 0;
	virtual BOOL PayForSpeech(int nIndex, int nType) = 0;
	virtual	void SetServerTrans(LPVOID pServer, unsigned int lnID) = 0;//guve
	virtual	void SetServerChat(LPVOID pServer, unsigned int lnID) = 0;
	virtual	void SetServerTong(LPVOID pServer, unsigned int lnID) = 0;
};
#ifndef CORE_STATIC
#ifndef CORE_EXPORTS

	//获取iCoreShell接口实例的指针
#ifndef __linux
	extern "C" 
#endif
	iCoreServerShell* CoreGetServerShell();

#endif
#else
	extern "C" iCoreServerShell* CoreGetServerShell();
#endif
#endif
