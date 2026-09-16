// KTongSet.h: interface for the CTongSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KTONGSET_H__B42782F1_FA08_4D1C_A209_1ED1F5E0BAA3__INCLUDED_)
#define AFX_KTONGSET_H__B42782F1_FA08_4D1C_A209_1ED1F5E0BAA3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include	"KTongControl.h"

#define		defTONG_SET_INIT_POINT_NUM		16

class CTongSet
{
public:
	CTongSet();
	virtual ~CTongSet();

private:
	CTongControl**	m_pcTong;
	int				m_nTongPointSize;		// 指针 m_pcTong 当前分配内存的大小(多少个)
	CLockMRSW		m_lockTArray;
public:
	void			Init();
	void			DeleteAll();
	BOOL			InitFromDB();

	int				Create(int nCamp, char *lpszPlayerName, char *lpszTongName, BYTE nSex = 0);
	
	int				AddMember(char *lpszPlayerName, UINT dwTongNameID, char *lpszTongName,
						BYTE btFigure, BYTE nSex, BOOL& bForce);
	
	int				GetTongCamp(int nTongIdx);

	BOOL			GetMasterName(int nTongIdx, char *lpszName);

	BOOL			GetTongHeadInfo(UINT dwTongNameID, STONG_HEAD_INFO_SYNC *pInfo);

	BOOL			GetTongMemberInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, STONG_MEMBER_INFO_SYNC *pInfo, CTongConnect* pConn);
	
	BOOL			GetTongPageInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	
	BOOL			GetRecordInfo(STONG_GET_MEMBER_INFO_COMMAND *pApply, CTongConnect* pConn);
	
	BOOL			Instate(STONG_INSTATE_COMMAND *pInstate, STONG_INSTATE_SYNC *pSync);

	BOOL			Kick(STONG_KICK_COMMAND *pKick, STONG_KICK_SYNC *pSync);

	BOOL			Leave(STONG_LEAVE_COMMAND *pLeave, STONG_LEAVE_SYNC *pSync);

	BOOL			AcceptMaster(STONG_ACCEPT_MASTER_COMMAND *pAccept, CTongConnect* pConn);
	
	BOOL			GetLoginData(STONG_GET_LOGIN_DATA_COMMAND *pLogin, STONG_LOGIN_DATA_SYNC *pSync);
	
	void			SaveMember(const char* szTongName, STONG_MEMBER& sCtrlMem);
	
	BOOL			DistribRight(UINT dwTongNameID, UINT uSenderID, UINT uDestID, UINT& uRight);
	BOOL			ChangeRecruit(UINT dwTongNameID, UINT uSenderID, int& nLockRecruit);
	int				Update(int nTongIdx);
	BOOL			AddMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney);
	BOOL			WithDrawMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney);
	BOOL			AddOffer(UINT dwTongNameID, UINT uSenderID, int& nOffer);
	BOOL			DispenseOffer(UINT dwTongNameID, UINT uSenderID, UINT uDestID, int& nOffer);
	void			AssignMoney(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn);
	void			AssignOffer(STONG_ASSIGNFUND_COMMAND* pInfo, CTongConnect* pConn);
	BOOL			TransMoney(UINT dwTongNameID, UINT uSenderID, int& nMoney, int& nBuildFund);
	BOOL			AddBuildFund(STONG_CONTRIBMONEY_COMMAND	*pInfo, CTongConnect* pConn);
	BOOL			SetAnnounce(STONG_ANNOUNCE_COMMAND* pInfo);
	void			ResetTask(UINT dwTongNameID);
	void			AddWeekGoal(UINT dwTongNameID, UINT dwPlayerNameID, int nValue);
	void			SetTaskLevel(UINT dwTongNameID, UINT dwPlayerNameID, int nValue);
	void			ReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn);
	void			PlayerReceivePrice(STONG_ADDWEEKGOAL_COMMAND *pInfo, CTongConnect* pConn);
	void			ChangeTitle(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn);
	void			ChangeTitleAll(STONG_CHANGETITLE_COMMAND *pInfo, CTongConnect* pConn);
	void			ChangeCamp(STONG_CHANGECAMP_COMMAND *pInfo, CTongConnect* pConn);
};

#endif // !defined(AFX_KTONGSET_H__B42782F1_FA08_4D1C_A209_1ED1F5E0BAA3__INCLUDED_)
