/*******************************************************************************
File        : UiTongManager.h
Creator     : GuveCorp
********************************************************************************/

#if !defined(AFX_UITONGMANAGER_H__13BA213D_11EC_4F24_BF98_F51C3F414D6D__INCLUDED_)
#define AFX_UITONGMANAGER_H__13BA213D_11EC_4F24_BF98_F51C3F414D6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../elem/wndlabeledbutton.h"
#include "../elem/wndscrollbar.h"
#include "../elem/wndimage.h"
//#include "../elem/wndlist2.h"
#include "../elem/WndMessageListBox.h"
//#include "../../Engine/Src/LinkStruct.h"
#include "KProtocol.h"
#include <vector>
using namespace std;

void g_GetCurDate(UINT& uYear, UINT& uMonth, UINT& uDay, UINT& uHour, UINT& uMin);
UINT g_DateMinute(UINT uYear, UINT uMonth, UINT uDay, UINT uHour, UINT uMin);
void g_Minute2Date(UINT uSrcMin, UINT& uYear, UINT& uMonth, UINT& uDay, UINT& uHour, UINT& uMin);
UINT g_GetCurDateMin();

class KUiTongManager : KWndImage
{
public:
	KUiTongManager();
	virtual ~KUiTongManager();

	static			KUiTongManager* OpenWindow(char* pszPlayerName);
	static			KUiTongManager* GetIfVisible();
	static void		CloseWindow(bool bDestory = true);
	static void		LoadScheme(const char* pScheme);
	static void		Static_TongInfoArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->TongInfoArrive(pInfo);
	}
	static void		Static_MemInfoArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->MemInfoArrive(pInfo);
	}
	static void		Static_TongPageArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->TongPageArrive(pInfo);
	}
	static void		Static_RecordListArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->RecordListArrive(pInfo);
	}
	static void		Static_RecordAnnounceArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->RecordAnnounceArrive(pInfo);
	}
	static void		Static_RecordWeekTaskArrive(void * pInfo)
	{
		if(ms_pSelf)
			ms_pSelf->RecordWeekTaskArrive(pInfo);
	}
	static void   Static_ResponseResult(KUiGameObjectWithName *pResult, int nbIsSucceed)
	{
		if(ms_pSelf)
			ms_pSelf->ResponseResult(pResult, nbIsSucceed);
	}
public:
	virtual int	WndProc(unsigned int uMsg, unsigned int uParam, int nParam);
	void		ArrangeComposition(char* pszPlayerName);
	void		TongInfoArrive(void * pInfo);
	void		MemInfoArrive(void * pInfo);
	void		TongPageArrive(void * pInfo);
	void		RecordListArrive(void * pInfo);
	void		RecordAnnounceArrive(void * pInfo);
	void		RecordWeekTaskArrive(void * pInfo);
	void		ResponseResult(KUiGameObjectWithName *pResult, int nbIsSucceed);
private:
	void		Initialize();
	void		Breathe();
	void		ShowTab(int nTab);
	void		AddMemberList(int nIndex, STONG_MEMSUBINFO& sMem, bool bPush = true);
	void		AddTongList(int nIndex, STONG_PAGEMEM& sMem);
	void		Popup_SortMenu();
private:
	static KUiTongManager*	ms_pSelf;
	KWndButton			m_BtnEnterMap, m_BtnRefresh, m_BtnTongList, m_BtnClose;
	KWndButton			m_TabBtnBaseInfo, m_TabBtnRecruit, m_TabBtnFunUse,
							m_TabBtnRight, m_TabBtnWorkShop, m_TabBtnTongRec;
	KWndImage			m_BgBaseInfo, m_BgRecruitSelf, m_BgRecruitOth, m_BgFunUse,
							m_BgRight, m_BgWorkShop, m_BgWorkShopImg, m_BgRecord;
	KWndText32			m_P0_TitleTongInfo, m_P0_TitlePersonalInfo, m_P0_TxtHelpTitle;
	KWndLabeledButton	m_P0_TitleTongName,
						m_P0_TitleMaster,
						m_P0_TitleLeague,
						m_P0_TitleCamp,
						m_P0_TitleTongLevel,
						m_P0_TitleMemberNum,
						m_P0_TitleBuildLevel,
						m_P0_TitleTongCapital,
						m_P0_TitleBuildFund,
						m_P0_TitleTotalOffer;
	KWndLabeledButton	m_P0_TxtTongName,
						m_P0_TxtMaster,
						m_P0_TxtLeague,
						m_P0_TxtCamp,
						m_P0_TxtTongLevel,
						m_P0_TxtMemberNum,
						m_P0_TxtBuildLevel,
						m_P0_TxtTongCapital,
						m_P0_TxtBuildFund,
						m_P0_TxtTotalOffer;
	KWndText32			m_P0_TxtMoneyUnit2, m_P0_TxtMoneyUnit1;
	KWndLabeledButton	m_BtnApply;
	KWndLabeledButton	m_P0_TitlePersonalOffer, m_P0_TxtPersonalOffer, m_P0_TitleWeeklyOffer, m_P0_TxtWeeklyOffer;
	KWndText256			m_P0_TxtHelp;
	char				m_szHelp[12][256];
	KWndText32			m_TxtRank, m_TxtTitle, m_TxtType;
	KWndMessageListBox	m_MemList;
	KWndLabeledButton	m_BtnPrevPage, m_BtnNextPage, m_BtnJump, m_BtnOnlinePriority, m_BtnSortMenu;
	KWndEdit32			m_EditBoxDestPage;
	KWndText32			m_P2_TxtPersonalInfo;
	KWndLabeledButton	m_P2_TitleTongName, m_P2_TxtTongName, m_P2_TitleTongUnion, m_P2_TxtTongUnion,
						m_P2_TitleBuildLevel, m_P2_TxtBuildLevel, m_P2_TitleTotalOffer, m_P2_TxtTotalOffer,
						m_P2_TitleTongCapital, m_P2_TxtTongCapital, m_P2_TitleBuildFund, m_P2_TxtBuildFund,
						m_P2_TitleExp;
	KWndImage			m_P2_ImgExp;
	KWndText32			m_P2_TxtMoneyUnit, m_P2_TxtExpPercent;
	KWndLabeledButton	m_P2_TitlePersonalOffer, m_P2_TxtPersonalOffer, m_P2_TitleTongCapital2, m_P2_TxtTongCapital2,
						m_P2_TitleBuildFund2, m_P2_TxtBuildFund2, m_BtnLeaveTong;
	KWndLabeledButton	m_BtnUpgradeBuildLevel, m_BtnAssignTongOffer, m_BtnGetTongMoney,
						m_BtnAssignTongMoney, m_BtnTransformMoney, m_BtnStorePersonalOffer,
						m_BtnStoreTongMoney, m_BtnStoreBuildFund;
	KWndButton			m_BtnSubPage[3];
	KWndLabeledButton	m_BtnDemise,
						m_BtnForceToRetire,
						m_BtnKickOut,
						m_BtnDepose,
						m_Btn_DispenseOffer,
						m_BtnChangeMaleTitle,
						m_BtnChangeFemaleTitle,
						m_BtnChangeTitle,
						m_BtnRecruit,
						m_BtnChangeCamp,
						m_BtnCreateTongMap,
						m_BtnConfigureTongMap,
						m_BtnTongChallenge,
						m_BtnCreateUnion,
						m_BtnApplyJionUnion,
						m_BtnLeaveUnion,
						m_BtnKickUnionTong,
						m_BtnAcceptUnionReq;
	KWndLabeledButton	m_P3_BtnDepose,
						m_P3_BtnChangeCamp,
						m_P3_BtnChangeTitle,
						m_P3_BtnKickOut,
						m_BtnRecordEvent,
						m_BtnLeagueManage,
						m_P3_BtnUpgradeBuildLevel,
						m_P3_BtnForceToRetire,
						m_BtnMapManagement,
						m_BtnWorkshopManagement,
						m_BtnTongClaimWar,
						m_BtnFundManagement,
						m_BtnWeekGoalManagement,
						m_P3_BtnRecruit;
	KWndLabeledButton	m_BtnSelectAll, m_BtnDistribute;
	KWndLabeledButton		m_BtnWeekDaily,
							m_BtnAnnounce,
							m_BtnTongAffair,
							m_BtnTongHistory;
	KScrollMessageListBox	m_RecordList;
	KWndEdit512				m_AnnounceEditor;
	KWndButton				m_BtnLeaveWord, m_BtnEditAnnounce;
	
	vector<STONG_MEMSUBINFO>	m_vMemberList;
	vector<STONG_PAGEMEM>		m_vTongList;
	char				m_szAnnounce[128];
	int					m_nCurTab;
	int					m_nCurMemPage;
	int					m_nCurTongPage;
	int					m_nCurUnionPage;
	unsigned int		m_OnlineColor;
	unsigned int		m_OfflineColor;
	int					m_nCurFunUseTab;
	int					m_nCurRecordTab;
	UINT				m_uMeTongID;
	UINT				m_uDestTongID;
	char          		m_szDestName[32]; //cur dest player
	int					m_nNextTimeInfo;
	int					m_nNextTimeMemPage;
	bool				m_bHideAnnounce;
};


#endif // !defined(AFX_UITONGMANAGER_H__13BA213D_11EC_4F24_BF98_F51C3F414D6D__INCLUDED_)
