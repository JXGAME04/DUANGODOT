// KTongControl.h: interface for the CTongControl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KTONGCONTROL_H__62D04F9A_67CD_419B_B475_BF0F8727A91E__INCLUDED_)
#define AFX_KTONGCONTROL_H__62D04F9A_67CD_419B_B475_BF0F8727A91E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../../../headers/KTongProtocol.h"
#include <vector>
using namespace std;

typedef struct
{
	char		szName[defTONG_STR_LENGTH];
}TTongList;

typedef struct
{
	char	szMsg[128];
}TTongMsg;

void	InitTScript();
void	UnInitTScript();
class CTongControl
{
	friend class CTongSet;
	friend class CTongDB;
public:
	CTongControl(int nCamp, char *lpszPlayerName, char *lpszTongName, BYTE nSex);
	CTongControl(TTongList sList);
	virtual ~CTongControl();

private:

	int			m_nCamp;								// phe
	int			m_nMoney;								// tien ngan quy
	int			m_nBuildMoney;							// tien kien thiet
	int			m_nLevel;								// cap
	int			m_nBuilLevel;							// cap kien thiet
	int			m_nOffer;								// cong hien du tru
	UINT		m_dwNameID;								// id bang
	UINT		m_uCreateDate;							// phut tao. bang
	UINT		m_uWeekCount;							// tuan thu'
	int			m_nExp;									// kinh nghiem len cap
	int			m_bLockRecruit;							// khoa tuyen dung
	int			m_bOrdeal;								// thoi gian thu thach
	int			m_nTaskLevel;							// do kho' muc tieu tuan hien tai
	UINT		m_uUnionID;								// id lien minh
	UINT		m_uPromoteID;							// id quang ba' tuyen dung
	char		m_szName[defTONG_STR_LENGTH];			// ten bang
	TTongWeekGoal m_OldWeekGoal;						// muc tieu cu~
	TTongWeekGoal m_NewWeekGoal;						// muc tieu moi' hien tai
	STONG_MEMBER m_Master; //thong tin bang chu
	vector<STONG_MEMBER>	m_Director; //thong tin truong lao
	vector<STONG_MEMBER>	m_Manager; //thong tin doi truong
	vector<STONG_MEMBER>	m_Member; //thong tin mon de
	vector<string>			m_AffairMsg;
	vector<string>			m_HistoryMsg;
	char		m_szAnnounce[128];	//cong cao' bang hoi
	UINT		m_uDemiseNextMin; //phut gian~ cach chuyen vi
	UINT		m_uAssignFundNextMin; //phut gian~ cach phat quy~
	UINT		m_uChangeTitleAllNextMin; //phut gian~ cach doi~ ten
	UINT		m_uChangeCampNextMin; //phut gian~ cach doi~ phe
public:
	int			GetLevel() {return m_nLevel;};
	int			GetExp() {return m_nExp;};
	const char*	GetName() {return &m_szName[0];};
	BOOL		AddMember(char *lpszPlayerName, char *lpszTongName, BYTE btFigure, BYTE nSex, BOOL& bForce);

	BOOL		GetTongHeadInfo(STONG_HEAD_INFO_SYNC *pInfo);

	BOOL		GetTongMemberInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, STONG_MEMBER_INFO_SYNC *pInfo,
				CTongConnect* pConn, CNetConnectDup* pConndup = NULL);

	BOOL		Instate(STONG_INSTATE_COMMAND *pInstate, STONG_INSTATE_SYNC *pSync);

	BOOL		Kick(STONG_KICK_COMMAND *pKick, STONG_KICK_SYNC *pSync);

	BOOL		Leave(STONG_LEAVE_COMMAND *pLeave, STONG_LEAVE_SYNC *pSync);

	BOOL		AcceptMaster(STONG_ACCEPT_MASTER_COMMAND *pAccept, CTongConnect* pConn);

	BOOL		GetLoginData(STONG_GET_LOGIN_DATA_COMMAND *pLogin, STONG_LOGIN_DATA_SYNC *pSync);
	BOOL		DistribRight(UINT uSenderID, UINT uDestID, UINT& uRight);
	int			FindMemPage(int nPos);
	STONG_MEMBER*	FindMember(UINT uNameID);
	BOOL		ChangeRecruit(UINT uSenderID, int& nLockRecruit);
	BOOL		Update();
	BOOL		AddMoney(UINT uSenderID, int& nMoney);
	BOOL		WithDrawMoney(UINT uSenderID, int& nMoney);
	BOOL		AddOffer(UINT uSenderID, int& nOffer);
	BOOL		DispenseOffer(UINT uSenderID, UINT uDestID, int& nOffer);
	void		AssignMoney(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn);
	void		AssignOffer(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn);
	BOOL		TransMoney(UINT uSenderID, int& nMoney, int& nBuildFund);
	BOOL		AddBuildFund(STONG_CONTRIBMONEY_COMMAND	*pInfo, CTongConnect* pConn);
	BOOL		GeneratesTask(int nLevel, UINT nMembers, bool bSync = false);
	BOOL		GetRecordWeekTask(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	BOOL		GetRecordAnnounce(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	BOOL		GetRecordAffair(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	BOOL		GetRecordHistory(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	BOOL		SetAnnounce(STONG_ANNOUNCE_COMMAND* pInfo);
	void		AddAffairMsg(const char* pMsg, bool bDate = true, bool bSave = true);
	void		AddHistoryMsg(const char* pMsg, bool bDate = true, bool bSave = true);
	void		ResetTask(bool bSave = true);
	void		AddWeekGoal(UINT dwPlayerNameID, int nValue);
	void		SetTaskLevel(UINT dwPlayerNameID, int nValue);
	void		ReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn);
	void		PlayerReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn);
	void		ChangeTitle(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn);
	void		ChangeTitleAll(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn);
	void		ChangeCamp(STONG_CHANGECAMP_COMMAND *pInfo, CTongConnect* pConn);
};

#endif // !defined(AFX_KTONGCONTROL_H__62D04F9A_67CD_419B_B475_BF0F8727A91E__INCLUDED_)
