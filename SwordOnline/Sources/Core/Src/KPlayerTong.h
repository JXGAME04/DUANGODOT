//---------------------------------------------------------------------------
// Sword3 Engine (c) 2003 by Kingsoft
//
// File:	KPlayerTong.h
// Date:	2003.08.12
// Code:	边城浪子
// Desc:	KPlayerTong Class
//---------------------------------------------------------------------------

#ifndef KPLAYERTONG_H
#define KPLAYERTONG_H

class KPlayerTong
{
	friend class KPlayer;
private:
	int			m_nPlayerIndex;
	int			m_nFlag;			// co bang hay khong
	int			m_nFigure;			// chuc vu
	int			m_nCamp;			// phe phai
	UINT		m_dwTongNameID;		// ten bang doi ra ID
	int			m_nWeekOffer;		// cong hien tuan
	char		m_szName[32];		// ten bang
	char		m_szTitle[32];		// danh hieu
	int			m_nApplyTo;			// player xin vao bang
	UINT		m_uRight;			// phan quyen
	BYTE		m_nWeekGoalType;	// nhan nhiem vu loai gi
public:
	char		m_szMasterName[32];	// ten bang chu
	int			m_nNextTimeInfo;
	int			m_nNextTimeMemPage;
private:
	BOOL		CheckAcceptAddApplyCondition();	// 判断是否可以转发别人的加入帮会申请

public:
	// 初始化
	void		Init(int nPlayerIdx);
	void		Clear();
	void		GetTongName(char *lpszGetName);
	void		GetTitle(char *lpszGetName);
	UINT		GetTongNameID();
	void		SetTongNameID(UINT dwID) { m_dwTongNameID = dwID; };
	int			CheckIn() {return m_nFlag;};
	int			GetCamp() {return m_nCamp;};
	int			GetFigure() {return m_nFigure;};
	int			GetWeekOffer() {return m_nWeekOffer;};
	int			GetWeekGoalType() {return m_nWeekGoalType;};
	UINT		GetRight() {return m_uRight;};
#ifndef _SERVER
	BOOL		ApplyCreateTong(int nCamp, char *lpszTongName);
	void		Create(TONG_CREATE_SYNC *psCreate);
	BOOL		ApplyAddTong(UINT uDestID);
	void		AcceptMember(int nPlayerIdx, UINT dwNameID, BOOL bFlag);
	BOOL		AddTong(int nCamp, char *lpszTongName, char *lpszMaster, int nFigure);
	BOOL		ApplyInstate(UINT uDestID, int nNewFigure);
	BOOL		ApplyKick(UINT uDestID);
	BOOL		ApplyChangeMaster(UINT uDestID);
	BOOL		ApplyLeave();
	BOOL		ApplyInfo(int nInfoID, UINT nParam1, int nParam2, int nParam3);
	BOOL		ApplyRight(UINT uDestID, UINT uRight);
	void		SetSelfInfo(TONG_SELF_INFO_SYNC *pInfo);
	void        OpenCreateInterface();
	void		ApplyChangeRecruit();
	void		ApplyContribMoney(int nMoney);
	void		ApplyWithdrawMoney(int nMoney);
	void		ApplyStoreOffer(int nMoney);
	void		ApplyDispenseOffer(UINT uDestID, int nMoney);
	void		ApplyAssignMoney(int nMemberPoint, int nManagerPoint, int nDirectorPoint);
	void		ApplyAssignOffer(int nMemberPoint, int nManagerPoint, int nDirectorPoint);
	void		ApplyTransMoney(int nMoney);
	void		ApplyStoreBuildFund(int nMoney);
	void		ApplyChangeAnnounce(const char* pString);
	void		ApplyChangeTitle(UINT uDestID, BYTE nDestFigure, const char* pString);
	void		ApplyChangeTitleAll(BYTE nSex, const char* pString);
	void		ApplyChangeCamp(BYTE btCamp);
	void		ApplyAction(int nAction);
#endif

#ifdef _SERVER
	// 判断创建帮会条件是否成立 if 成功 return == 0 else return error id
	int			CheckCreateCondition(int nCamp, char *lpszTongName);
	// 得到relay通知，帮会创建成功，处理相应数据
	BOOL		Create(int nCamp, char *lpszTongName);
	// 头上是否需要顶找人标志
	BOOL		GetOpenFlag();
	// 转发加入帮会申请给对方客户端
	BOOL		TransferAddApply(UINT uDestID);
	// 发消息通知拒绝某人申请
	void		SendRefuseMessage(int nPlayerIdx, UINT dwNameID);
	// 判断别人加入自己帮会条件是否成立
	BOOL		CheckAddCondition(int nPlayerIdx);
	// 加入帮会，成为普通帮众
	BOOL		AddTong(int nCamp, char *lpszTongName, char *lpszMasterName, int nFigure);
	// 给客户端发送自己在帮会中的信息
	void		SendSelfInfo();
	// 检测是否有任命权利
	BOOL		CheckInstatePower(TONG_APPLY_INSTATE_COMMAND *pApply);
	// 被任命
	void		BeInstated(STONG_SERVER_TO_CORE_BE_INSTATED *pSync);
	// 检测是否有踢人权利
	BOOL		CheckKickPower(UINT uDestID);
	// 被踢出帮会
	void		BeKicked();
	// 检测是否有离开权利
	BOOL		CheckLeavePower();
	// 离开帮会
	void		Leave(STONG_SERVER_TO_CORE_LEAVE *pLeave);
	// 检测是否有权利换帮主
	int			CheckChangeMasterPower(UINT uDestID);
	// 检测是否有能力接受传位
	BOOL		CheckGetMasterPower(UINT dwTongID);
	// 传位导致身份改变
	void		ChangeAs(STONG_SERVER_TO_CORE_CHANGE_AS *pAs);
	// 换帮主
	void		ChangeMaster(char *lpszMaster);
	// 登陆时候获得帮会信息
	void		Login(STONG_SERVER_TO_CORE_LOGIN *pLogin);

	void		DBSetTongNameID(DWORD dwID) { m_dwTongNameID = dwID; if (dwID) m_nFlag = 1;};
	void		BeRight(UINT uRight);
	BOOL		CheckChangeRecruitPower();
	void		Delete();
	BOOL		CheckContribMoney(int nMoney);
	BOOL		CheckWithDrawMoney(int nMoney);
	BOOL		CheckStoreOffer(int nMoney);
	int			CheckDispenseOffer(UINT uDestID, int nOffer);
	BOOL		CheckAssignMoney(int nMemberPoint, int nManagerPoint, int nDirectorPoint);
	BOOL		CheckTransMoney(int nMoney);
	BOOL		CheckStoreBuildFund(int nMoney);
	void		StoreBuildFundRet(int nWeekOffer, int nAddOffer);
	void		ResetWeekTask(BYTE nWeekGoalType);
	BOOL		CheckAnnouncePower();
	void		SetWeekOffer(int nValue);
	BOOL		CheckChangeTitlePower(BYTE nDestFigure, char* pszTitle);
	BOOL		CheckChangeTitleAllPower(char* pszTitle);
	void		ChangeTitle(const char* pszTitle);
	BOOL		CheckChangeCampPower(BYTE btCamp);
	void		ChangeCamp(int nCamp);
	BOOL		CheckCanAction(int nAction);
#endif
};
#endif
