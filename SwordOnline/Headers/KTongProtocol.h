// -------------------------------------------------------------------------
//	文件名		：	KTongProtocol.h
//	创建者		：	谢茂培 (Hsie)
//	创建时间	：	2003-08-13 15:12:19
//	功能描述	：	
//
// -------------------------------------------------------------------------
#ifndef __KTONGPROTOCOL_H__
#define __KTONGPROTOCOL_H__
#ifdef _STANDALONE //guve
#include "GameDataDef.h"
#else
#include "../Sources/Core/src/GameDataDef.h"
#endif

#pragma pack(push, 1)

#define		defTONG_PROTOCOL_SERVER_NUM		255
#define		defTONG_PROTOCOL_CLIENT_NUM		255



//---------------------------- tong protocol ----------------------------
// relay server 收到的 game server 的协议
enum 
{
	enumC2S_TONG_CREATE = 0,		 
	enumC2S_TONG_ADD_MEMBER,		 
	enumC2S_TONG_GET_HEAD_INFO,		 
	enumC2S_TONG_GET_MEMBER_INFO,	 
	enumC2S_TONG_GET_TONGPAGE_INFO,	 
	enumC2S_TONG_GET_RECORD_INFO,	 
	enumC2S_TONG_INSTATE,			 
	enumC2S_TONG_KICK,				 
	enumC2S_TONG_LEAVE,				 
	enumC2S_TONG_ACCEPT_MASTER,		 
	enumC2S_TONG_GET_LOGIN_DATA,	 
	enumC2S_TONG_DISTRIB_RIGHT,		
	enumC2S_TONG_CHANGERECRUIT,		
	enumC2S_TONG_CONTRIBMONEY,		
	enumC2S_TONG_WITHDRAWMONEY,		
	enumC2S_TONG_STOREOFFER,		
	enumC2S_TONG_DISPENSEOFFER,		
	enumC2S_TONG_ASSIGNMONEY,		
	enumC2S_TONG_ASSIGNOFFER,		
	enumC2S_TONG_TRANSMONEY,		
	enumC2S_TONG_STOREBUILDFUND,	
	enumC2S_TONG_ANNOUNCE,
	enumC2S_TONG_RESETTASK,
	enumC2S_TONG_ADDWEEKGOAL,
	enumC2S_TONG_TASKLEVEL,
	enumC2S_TONG_RECVPRICE,
	enumC2S_TONG_PLAYERPRICE,
	enumC2S_TONG_CHANGETITLE,
	enumC2S_TONG_CHANGETITLEALL,
	enumC2S_TONG_CHANGECAMP,
	enumC2S_TONG_NUM,
};

// relay server 发给 game server 的协议
enum
{
	enumS2C_TONG_CREATE_SUCCESS = 0,
	enumS2C_TONG_CREATE_FAIL,		
	enumS2C_TONG_ADD_MEMBER_SUCCESS,
	enumS2C_TONG_ACCEPTMEMBER_FAIL,	
	enumS2C_TONG_HEAD_INFO,			
	enumS2C_TONG_MEMBER_INFO,		
	enumS2C_TONG_PAGE_INFO,
	enumS2C_TONG_RECORD_AFFAIRHISTORY,
	enumS2C_TONG_RECORD_ANNOUNCE,
	enumS2C_TONG_RECORD_WEEKTASK,
	enumS2C_TONG_BE_INSTATED,		
	enumS2C_TONG_INSTATE,			
	enumS2C_TONG_KICK,				
	enumS2C_TONG_BE_KICKED,			
	enumS2C_TONG_LEAVE,				
	enumS2C_TONG_CHANGE_AS,			
	enumS2C_TONG_CHANGE_MASTER,		
	enumS2C_TONG_LOGIN_DATA,		
	enumS2C_TONG_DISTRIB_RIGHT_RET,	
	enumS2C_TONG_BE_RIGHT,			
	enumS2C_TONG_CHANGERECRUIT,		
	enumS2C_TONG_DELETE,			
	enumS2C_TONG_CONTRIBMONEY,		
	enumS2C_TONG_WITHDRAWMONEY,		
	enumS2C_TONG_STOREOFFER,		
	enumS2C_BE_DISPENSEOFFER,		
	enumS2C_ASSIGN_MONEY_RET,		
	enumS2C_ASSIGN_OFFER_RET,		
	enumS2C_TONG_TRANSMONEY,		
	enumS2C_TONG_STOREBUILDFUND,	
	enumS2C_TONG_WEEKLYRESET,	
	enumS2C_TONG_RECEIVEPRICE_RET,
	enumS2C_TONG_PLAYERPRICE_RET,
	enumS2C_TONG_CHANGETITLE_UI,
	enumS2C_TONG_BE_CHANGETITLE,
	enumS2C_TONG_ADDWEEKGOAL_RET,
	enumS2C_TONG_CHANGETITLEALL,
	enumS2C_TONG_CHANGECAMP,
	enumS2C_TONG_NUM,						// total
};
//-------------------------- tong protocol end --------------------------

//friend protocol
enum 
{
	friend_c2c_askaddfriend,	//请求加为好友
	friend_c2c_repaddfriend,	//同意/拒绝加为好友
	friend_c2s_groupfriend,		//将好友分组
	friend_c2s_erasefriend,		//删除好友

	friend_c2s_asksyncfriendlist,	//请求同步好友列表
	friend_s2c_repsyncfriendlist,	//同步好友列表

	friend_s2c_friendstate,		//好友状态通知

	friend_c2s_associate,		//GS到Relay，自动组合2个人（有方向）
	friend_c2s_associatebevy,	//GS到Relay，自动组合n个人
	friend_s2c_syncassociate,	//Relay到Client，通知组合
};

//extend protocol
enum
{
	extend_s2c_passtosomeone,
	extend_s2c_passtobevy,
};



/////////////////////////////////////////////////////////////////
//friend struct

const int _GROUP_NAME_LEN = _NAME_LEN * 2;


struct ASK_ADDFRIEND_CMD : EXTEND_HEADER
{
	BYTE pckgid;
	char dstrole[_NAME_LEN];
};
struct ASK_ADDFRIEND_SYNC : EXTEND_HEADER
{
	BYTE pckgid;
	char srcrole[_NAME_LEN];
};

enum {answerAgree, answerDisagree, answerUnable};
struct REP_ADDFRIEND_CMD : EXTEND_HEADER
{
	BYTE pckgid;
	char dstrole[_NAME_LEN];
	BYTE answer;	//agree/disagree/unable
};
struct REP_ADDFRIEND_SYNC : EXTEND_HEADER
{
	BYTE pckgid;
	char srcrole[_NAME_LEN];
	BYTE answer;	//agree/disagree/unable
};



//used by GROUP_FRIEND & REP_SYNCFRIENDLIST
enum {specOver = 0x00, specGroup = 0x01, specRole = 0x02};

struct GROUP_FRIEND : EXTEND_HEADER
{
	//format: char seq
	//specGroup标记组，其后接该组好友列表，以\0间隔，specRole标记角色名
	//最后以双\0结束
};


struct ERASE_FRIEND : EXTEND_HEADER
{
	char friendrole[_NAME_LEN];
};


struct ASK_SYNCFRIENDLIST : EXTEND_HEADER
{
	BYTE pckgid;
	BYTE full;
};


struct REP_SYNCFRIENDLIST : EXTEND_HEADER
{
	BYTE pckgid;
	//format: char seq (same as GROUP_FRIEND)
	//specGroup标记组，其后接该组好友列表，以\0间隔，specRole标记角色名
	//最后以双\0结束
};


enum {stateOffline, stateOnline};

struct FRIEND_STATE : EXTEND_HEADER
{
	BYTE state;
	//format: char seq, \0间隔，双\0结束
};

struct FRIEND_ASSOCIATE : EXTEND_HEADER
{
	BYTE bidir;
	//format: string * 3
	//组名
	//角色名 * 2
};

struct FRIEND_ASSOCIATEBEVY : EXTEND_HEADER
{
	//format: char seq, \0间隔，双\0结束
	//组名
	//角色名列表
};

struct FRIEND_SYNCASSOCIATE : EXTEND_HEADER
{
	//format: char seq, \0间隔，双\0结束
	//组名
	//角色名列表
};


/////////////////////////////////////////////////////////////////
//extend struct

struct EXTEND_PASSTOSOMEONE : EXTEND_HEADER
{
	UINT			nameid;
	unsigned long	lnID;
	WORD			datasize;
};

struct EXTEND_PASSTOBEVY : EXTEND_HEADER
{
	WORD	datasize;
	WORD	playercount;
	//data
	//tagPlusSrcInfo vector
};

//----------------------------- tong struct -----------------------------

struct STONG_CREATE_COMMAND : EXTEND_HEADER
{
	WORD	m_wLength;
	UINT	m_dwPlayerNameID;
	BYTE	m_btCamp;
	BYTE	m_btTongNameLength;
	BYTE	m_btPlayerNameLength;
	BYTE	m_nSex;
	char	m_szBuffer[64];
};

struct STONG_CREATE_SUCCESS_SYNC : EXTEND_HEADER
{
	WORD	m_wLength;
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
	BYTE	m_btCamp;
	BYTE	m_btTongNameLength;
	char	m_szTongName[32];
};

struct STONG_CREATE_FAIL_SYNC : EXTEND_HEADER
{
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
};

struct STONG_ADD_MEMBER_COMMAND : EXTEND_HEADER
{
	WORD	m_wLength;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	BYTE	m_btFigure;
	BYTE	m_btPlayerNameLength;
	BYTE	m_nSex;
	char	m_szBuffer[32];
};

struct STONG_ADD_MEMBER_SUCCESS_SYNC : EXTEND_HEADER
{
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
	BYTE	m_btCamp;
	BYTE	m_btFigure;
	char	m_szTongName[32];
	char	m_szMasterName[32];
};

struct STONG_ACCEPTMEMBER_FAIL_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
};

struct STONG_GET_TONG_HEAD_INFO_COMMAND : EXTEND_HEADER
{
	UINT	m_dwParam;
	UINT	m_dwTongNameID;
	UINT	m_dwPID;
	BYTE	m_bSelf;
};

struct STONG_GET_MEMBER_INFO_COMMAND : EXTEND_HEADER
{
	UINT	m_dwParam;
	UINT	m_dwTongNameID;
	UINT	m_dwPID;
	int		m_nPage;
};

struct STONG_HEAD_INFO_SYNC : EXTEND_HEADER
{
	WORD	m_wLength;
	UINT	m_dwParam;
	UINT	m_dwPID;
	int		m_nMoney;
	int		m_nBuildMoney;
	int		m_nOffer;
	UINT	m_MemberNum;
	BYTE	m_btExpPercent;
	BYTE	m_nBuilLevel;
	BYTE	m_btCamp;
	BYTE	m_btLevel;
	BYTE	m_bLockRecruit;
	BYTE	m_bSelf;
	char	m_szTongName[defTONG_STR_LENGTH];
	char	m_szMaster[defTONG_STR_LENGTH];
	char	m_szUnion[defTONG_STR_LENGTH];
};

struct STONG_MEMBER_INFO_SYNC : EXTEND_HEADER
{
	WORD	m_wLength;
	UINT	m_dwParam;
	UINT	m_dwPID;
	int		m_nPage;
	int		m_nMemNum;
	BYTE	m_bBegin;
	STONG_MEMSUBINFO sMem[defTONG_MAX_MEMINFOSYNC];
};

struct STONG_TONGPAGE_INFO_SYNC : EXTEND_HEADER
{
	WORD	m_wLength;
	UINT	m_dwParam;
	UINT	m_dwPID;
	int		m_nPage;
	int		m_nMemNum;
	BYTE	m_bBegin;
	STONG_PAGEMEM sMem[defTONG_MAX_PAGEINFOSYNC];
};

struct STONG_RECORD_ANNOUNCE_SYNC : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	char	m_szAnnounce[128];
};

struct STONG_RECORD_WEEKTASK_SYNC : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	TTongWeekGoal m_OldWeekGoal;
	TTongWeekGoal m_NewWeekGoal;
	UINT	m_uWeekCount;
	int		m_DayNum;
	int		m_nOldWGCompleted;
	int		m_nNewWGCompleted;
	BYTE	m_bOrdeal;
	BYTE	m_nTaskLevel;
	BYTE	m_bWGType;
	BYTE	m_bGetPrice;
};

struct STONG_RECORD_AFFAIRHISTORY_SYNC : EXTEND_HEADER
{
	WORD	m_wLength;
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	int		m_nMsgCount;
	char	m_szMsg[4][128];
};

struct STONG_INSTATE_COMMAND : EXTEND_HEADER
{
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	UINT	m_dwTongNameID;
	UINT	m_uDestID;
	BYTE	m_btNewFigure;
};

struct STONG_BE_INSTATED_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
	BYTE	m_btFigure;
};

struct STONG_BE_KICKED_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
	
};

struct STONG_INSTATE_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	BYTE	m_btNewFigure;
};

struct STONG_KICK_COMMAND : EXTEND_HEADER
{
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	UINT	m_dwTongNameID;
	UINT	m_uDestID;
};

struct STONG_KICK_SYNC : EXTEND_HEADER
{
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	UINT	m_dwTongNameID;
	UINT	m_uDestID;
	int		m_nNeedMoney;
};

struct STONG_LEAVE_COMMAND : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
};

struct STONG_LEAVE_SYNC : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	BYTE	m_btSuccessFlag;
};

struct STONG_CHANGE_AS_SYNC : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	char	m_btFigure;
};

struct STONG_ACCEPT_MASTER_COMMAND : EXTEND_HEADER
{
	UINT	m_uDestID;
	UINT	m_nDestPIdx;
	UINT	m_dwTongNameID;
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
};

struct STONG_CHANGE_MASTER_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	char	m_szName[32];
};

struct STONG_GET_LOGIN_DATA_COMMAND : EXTEND_HEADER
{
	UINT	m_dwParam;
	UINT	m_dwTongNameID;
	char	m_szName[32];
};

struct STONG_LOGIN_DATA_SYNC : EXTEND_HEADER
{
	UINT	m_dwParam;
	UINT	m_uRight;
	BYTE	m_btFlag;
	BYTE	m_btCamp;
	BYTE	m_btFigure;
	BYTE	m_bWGType;
	int		m_nWeekOffer; //cong hien tuan
	char	m_szTongName[32];
	char	m_szTitle[32];
	char	m_szMaster[32];
};

struct STONG_DISTRIB_RIGHT_COMMAND : EXTEND_HEADER
{
	UINT	m_uDestID;
	UINT	m_dwTongNameID;
	UINT	m_uSenderID;
	int		m_nSenderPIdx;
	UINT	m_uRight;
};

struct STONG_BE_RIGHT_SYNC : EXTEND_HEADER
{
	int		m_uNetID;
	UINT	m_dwPlayerNameID;
	UINT	m_dwTongNameID;
	UINT	m_uRight;
};

struct STONG_CHANGERECRUIT_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	BYTE	m_bLockRecruit;
};

struct STONG_DELETE_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
};

struct STONG_CONTRIBMONEY_COMMAND : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nMoney;
};

struct STONG_CONTRIBMONEY_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		m_nMoney;
};

struct STONG_WITHDRAWMONEY_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		m_nMoney;
	int		m_nAddMoney;
};

struct STONG_DISPENSEOFFER_COMMAND : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nDestPIdx;
	UINT	m_uDestID;
	int		m_nMoney;
};

struct STONG_ASSIGNFUND_COMMAND : EXTEND_HEADER
{
	int		m_nPlayerIdx;
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		nMemberPoint;
	int		nManagerPoint;
	int		nDirectorPoint;
};

struct STONG_ASSIGNFUND_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		nMemberPoint;
	int		nManagerPoint;
	int		nDirectorPoint;
	int		m_nMoney;
	BYTE	m_bSuccess;
};

struct STONG_STOREBUILDFUND_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		m_nBuildFund;
	int		m_nWeekOffer;
	int		m_nAddOffer;
};

struct STONG_WEEKLYRESET_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	BYTE	m_nWeekGoalType;
};

struct STONG_ANNOUNCE_COMMAND : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	char	m_szAnnounce[128];
};

struct STONG_RESETTASK_COMMAND : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
};

struct STONG_ADDWEEKGOAL_COMMAND : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nValue;
};

struct STONG_ADDWEEKGOAL_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_uNetID;
	int		m_nWeekOffer;
	int		m_nAddOffer;
};

struct STONG_RECEIVEPRICE_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	BYTE	m_btSuccessFlag;
};

struct STONG_PLAYERPRICE_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		m_nPrice;
	BYTE	m_btSuccessFlag;
};

struct STONG_CHANGETITLE_COMMAND : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	UINT	m_uDestID;
	BYTE	m_nDestFigure;
	char	m_szName[32];
};

struct STONG_CHANGETITLE_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	UINT	m_uDestID;
	char	m_szName[32];
};

struct STONG_CHANGECAMP_COMMAND : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	BYTE	m_btCamp;
};

struct STONG_CHANGECAMP_SYNC : EXTEND_HEADER
{
	UINT	m_dwTongNameID;
	UINT	m_dwPlayerNameID;
	int		m_nPlayerIdx;
	int		m_nCamp;
	int		m_nMoney;
};
//--------------------------- tong struct end ---------------------------


#pragma pack(pop)

#endif // __KTONGPROTOCOL_H__
