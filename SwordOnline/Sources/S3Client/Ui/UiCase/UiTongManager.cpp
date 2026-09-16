/********************************************************************
File        : UiTongManager.cpp
Creator     : GuveCorp
*********************************************************************/

#include "KWin32.h"
#include "KIniFile.h"
#include "CoreShell.h"
#include "KPlayerDef.h"

#include "../elem/wnds.h"
#include "../elem/wndmessage.h"

#include "../UiBase.h"
#include "../UiSoundSetting.h"

#include "../../../Engine/src/KFilePath.h"
#include "../../../Engine/src/KDebug.h"
#include "../../../Engine/src/Text.h"
#include "UiPopupPasswordQuery.h"
#include "UiTongAssignBox.h"
#include "UiTongManager.h"
#include "UiInformation.h"
//#include "UiTongGetString.h"
#include "UiTongCreateSheet.h"
#include "UiTongDistrib.h"
#include "UiSysMsgCentre.h"
#include "UiGetString.h"
#include "UiFaceSelector.h"
#include "time.h"
#include "../Elem/MouseHover.h"
#include "../Elem/PopupMenu.h"

#pragma warning(disable:4018)
KUiTongManager* KUiTongManager::ms_pSelf = NULL;

//#define TONG_MANAGER_INI "°ï»á¹ÜÀí½çÃæ.ini"
#define TONG_MANAGER_INI "°ï»áÖ÷´°¿Ú.ini"

#define MSG_POPUP_NORIGHT	"Ng­¬i kh«ng cã quyÒn sö dông chøc n¨ng nµy"
#define MSG_POPUP_NOSEL		"Ng­¬i ch­a chän thµnh viªn nµo ®Ó sö dông chøc n¨ng"

extern iCoreShell* g_pCoreShell;
extern KUiInformation g_UiInformation;

void g_GetCurDate(UINT& uYear, UINT& uMonth, UINT& uDay, UINT& uHour, UINT& uMin)
{
	time_t curtm = ::time(NULL);
	struct tm* ptm = localtime(&curtm);
	uYear  = ptm->tm_year + 1900;  //
	uMonth = ptm->tm_mon + 1;      //
	uDay   = ptm->tm_mday;         // 1 -> 31
	uHour  = ptm->tm_hour;         // 0 -> 23
	uMin   = ptm->tm_min;          // 0 -> 59
}

static const UINT daysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
static BOOL IsLeapYear(UINT uYear)
{
    if (uYear % 4 != 0) return FALSE;
    if (uYear % 100 != 0) return TRUE;
    return (uYear % 400 == 0);
}

UINT g_DateMinute(UINT uYear, UINT uMonth, UINT uDay, UINT uHour, UINT uMin)
{
    UINT totalDays = 0;
    UINT y, m;

    for (y = 2026; y < uYear; y++)
    {
        totalDays += IsLeapYear(y) ? 366 : 365;
    }

    for (m = 0; m < uMonth - 1; m++)
    {
        totalDays += daysInMonth[m];
        if (m == 1 && IsLeapYear(uYear))
        {
            totalDays += 1;
        }
    }

    totalDays += (uDay - 1);

    return totalDays * 24 * 60 + uHour * 60 + uMin;
}

void g_Minute2Date(UINT uSrcMin, UINT& uYear, UINT& uMonth, UINT& uDay, UINT& uHour, UINT& uMin)
{
    UINT totalDays    = uSrcMin / (24 * 60);
    UINT remainMinute = uSrcMin % (24 * 60);
    uHour = remainMinute / 60;
    uMin  = remainMinute % 60;
    UINT year = 2026;
    while (TRUE)
    {
        UINT daysInYear = IsLeapYear(year) ? 366 : 365;
        if (totalDays < daysInYear)
        {
            break;
        }
        totalDays -= daysInYear;
        year++;
    }
    uYear = year;
    UINT month = 0;
    while (TRUE)
    {
        UINT daysInThisMonth = daysInMonth[month];
        if (month == 1 && IsLeapYear(year))
        {
            daysInThisMonth += 1;
        }

        if (totalDays < daysInThisMonth)
        {
            break;
        }
        totalDays -= daysInThisMonth;
        month++;
    }
    uMonth = month + 1;
    uDay = totalDays + 1;
}

UINT g_GetCurDateMin()
{
	UINT y, mt, d, h, m;
	g_GetCurDate(y, mt, d, h, m);
	return g_DateMinute(y, mt, d, h, m);
}


KUiTongManager::KUiTongManager()
{
	m_nCurTab = -1;
	m_nNextTimeInfo = 0;
	m_nNextTimeMemPage = 0;
	m_nCurMemPage = 0;
	m_nCurTongPage = 0;
	m_nCurUnionPage = 0;
	m_szAnnounce[0] = 0;
	m_bHideAnnounce = false;
}

KUiTongManager::~KUiTongManager()
{

}


KUiTongManager* KUiTongManager::OpenWindow(char* pszPlayerName)
{
	if(g_pCoreShell)
	{
    	if (ms_pSelf == NULL)
    	{
		    ms_pSelf = new KUiTongManager;
		    if (ms_pSelf)
    			ms_pSelf->Initialize();
    	}
    	if (ms_pSelf)
    	{
		    UiSoundPlay(UI_SI_WND_OPENCLOSE);
			ms_pSelf->BringToTop();
			ms_pSelf->Show();
			ms_pSelf->ArrangeComposition(pszPlayerName);
	    }
	}
	return ms_pSelf;
}

KUiTongManager* KUiTongManager::GetIfVisible()
{
	if (ms_pSelf && ms_pSelf->IsVisible())
		return ms_pSelf;
	return NULL;
}

void KUiTongManager::CloseWindow(bool bDestory)
{
	if (ms_pSelf)
	{
		ms_pSelf->Hide();
		if (bDestory)
		{
			ms_pSelf->Destroy();
			ms_pSelf = NULL;
		}
	}
}


void KUiTongManager::Initialize()
{
	AddChild(&m_TabBtnBaseInfo);
	AddChild(&m_TabBtnRecruit);
	AddChild(&m_TabBtnFunUse);
	AddChild(&m_TabBtnRight);
	AddChild(&m_TabBtnWorkShop);
	AddChild(&m_TabBtnTongRec);
	
	AddChild(&m_BgBaseInfo);
	AddChild(&m_BgRecruitSelf);
	AddChild(&m_BgRecruitOth);
	AddChild(&m_BgFunUse);
	AddChild(&m_BgRight);
	AddChild(&m_BgWorkShop);
	AddChild(&m_BgWorkShopImg);
	AddChild(&m_BgRecord);
	AddChild(&m_P0_TitleTongInfo);
	AddChild(&m_P0_TitlePersonalInfo);
	AddChild(&m_P0_TxtHelpTitle);
	AddChild(&m_P0_TitleTongName);
	AddChild(&m_P0_TitleMaster);
	AddChild(&m_P0_TitleLeague);
	AddChild(&m_P0_TitleCamp);
	AddChild(&m_P0_TitleTongLevel);
	AddChild(&m_P0_TitleMemberNum);
	AddChild(&m_P0_TitleBuildLevel);
	AddChild(&m_P0_TitleTongCapital);
	AddChild(&m_P0_TitleBuildFund);
	AddChild(&m_P0_TitleTotalOffer);
	
	AddChild(&m_P0_TxtTongName);
	AddChild(&m_P0_TxtMaster);
	AddChild(&m_P0_TxtLeague);
	AddChild(&m_P0_TxtCamp);
	AddChild(&m_P0_TxtTongLevel);
	AddChild(&m_P0_TxtMemberNum);
	AddChild(&m_P0_TxtBuildLevel);
	AddChild(&m_P0_TxtTongCapital);
	AddChild(&m_P0_TxtBuildFund);
	AddChild(&m_P0_TxtTotalOffer);
	AddChild(&m_P0_TxtMoneyUnit1);
	AddChild(&m_P0_TxtMoneyUnit2);
	AddChild(&m_P0_TitlePersonalOffer);
	AddChild(&m_P0_TxtPersonalOffer);
	AddChild(&m_P0_TitleWeeklyOffer);
	AddChild(&m_P0_TxtWeeklyOffer);
	AddChild(&m_P0_TxtHelp);
	AddChild(&m_BtnApply);
	AddChild(&m_TxtRank);
	AddChild(&m_TxtTitle);
	AddChild(&m_TxtType);
	AddChild(&m_MemList);
	AddChild(&m_BtnPrevPage);
	AddChild(&m_BtnNextPage);
	AddChild(&m_BtnJump);
	AddChild(&m_BtnOnlinePriority);
	AddChild(&m_BtnEnterMap);
	AddChild(&m_BtnRefresh);
	AddChild(&m_BtnTongList);
	AddChild(&m_BtnClose);
	AddChild(&m_BtnSortMenu);
	AddChild(&m_EditBoxDestPage);
	
	AddChild(&m_P2_TxtPersonalInfo);
	AddChild(&m_P2_TitleTongName);
	AddChild(&m_P2_TitleTongUnion);
	AddChild(&m_P2_TitleBuildLevel);
	AddChild(&m_P2_TitleTotalOffer);
	AddChild(&m_P2_TitleTongCapital);
	AddChild(&m_P2_TitleBuildFund);
	AddChild(&m_P2_TitleExp);
	AddChild(&m_P2_ImgExp);
	AddChild(&m_P2_TxtTongName);
	AddChild(&m_P2_TxtTongUnion);
	AddChild(&m_P2_TxtBuildLevel);
	AddChild(&m_P2_TxtTotalOffer);
	AddChild(&m_P2_TxtTongCapital);
	AddChild(&m_P2_TxtBuildFund);
	AddChild(&m_P2_TxtMoneyUnit);
	AddChild(&m_P2_TxtExpPercent);
	AddChild(&m_P2_TitlePersonalOffer);
	AddChild(&m_P2_TxtPersonalOffer);
	AddChild(&m_P2_TitleTongCapital2);
	AddChild(&m_P2_TxtTongCapital2);
	AddChild(&m_P2_TitleBuildFund2);
	AddChild(&m_P2_TxtBuildFund2);
	AddChild(&m_BtnLeaveTong);
	AddChild(&m_BtnUpgradeBuildLevel);
	AddChild(&m_BtnAssignTongOffer);
	AddChild(&m_BtnGetTongMoney);
	AddChild(&m_BtnAssignTongMoney);
	AddChild(&m_BtnTransformMoney);
	AddChild(&m_BtnStorePersonalOffer);
	AddChild(&m_BtnStoreTongMoney);
	AddChild(&m_BtnStoreBuildFund);
	AddChild(&m_BtnSubPage[0]);
	AddChild(&m_BtnSubPage[1]);
	AddChild(&m_BtnSubPage[2]);
	AddChild(&m_BtnDemise);
	AddChild(&m_BtnForceToRetire);
	AddChild(&m_BtnKickOut);
	AddChild(&m_BtnDepose);
	AddChild(&m_Btn_DispenseOffer);
	AddChild(&m_BtnChangeMaleTitle);
	AddChild(&m_BtnChangeFemaleTitle);
	AddChild(&m_BtnChangeTitle);
	AddChild(&m_BtnRecruit);
	AddChild(&m_BtnChangeCamp);
	AddChild(&m_BtnCreateTongMap);
	AddChild(&m_BtnConfigureTongMap);
	AddChild(&m_BtnTongChallenge);
	AddChild(&m_BtnCreateUnion);
	AddChild(&m_BtnApplyJionUnion);
	AddChild(&m_BtnLeaveUnion);
	AddChild(&m_BtnKickUnionTong);
	AddChild(&m_BtnAcceptUnionReq);
	AddChild(&m_P3_BtnDepose);
	AddChild(&m_P3_BtnChangeCamp);
	AddChild(&m_P3_BtnChangeTitle);
	AddChild(&m_P3_BtnKickOut);
	AddChild(&m_BtnRecordEvent);
	AddChild(&m_BtnLeagueManage);
	AddChild(&m_P3_BtnUpgradeBuildLevel);
	AddChild(&m_P3_BtnForceToRetire);
	AddChild(&m_BtnMapManagement);
	AddChild(&m_BtnWorkshopManagement);
	AddChild(&m_BtnTongClaimWar);
	AddChild(&m_BtnFundManagement);
	AddChild(&m_BtnWeekGoalManagement);
	AddChild(&m_P3_BtnRecruit);
	AddChild(&m_BtnSelectAll);
	AddChild(&m_BtnDistribute);
	AddChild(&m_BtnWeekDaily);
	AddChild(&m_BtnAnnounce);
	AddChild(&m_BtnTongAffair);
	AddChild(&m_BtnTongHistory);
	AddChild(&m_RecordList);
	AddChild(&m_AnnounceEditor);
	AddChild(&m_BtnLeaveWord);
	AddChild(&m_BtnEditAnnounce);

	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	Wnd_AddWindow(this);
}


void KUiTongManager::LoadScheme(const char* pScheme)
{
	if(ms_pSelf)
	{
		char		Buff[128];
		KIniFile	Ini;
		sprintf(Buff, "%s\\%s", pScheme, TONG_MANAGER_INI);

		if(Ini.Load(Buff))
		{
			ms_pSelf->Init(&Ini, "Main");			
			ms_pSelf->m_BtnRefresh.Init(&Ini, "BtnRefresh");
			ms_pSelf->m_BtnClose.Init(&Ini, "BtnClose");
			ms_pSelf->m_BtnEnterMap.Init(&Ini, "BtnEnterMap");
			ms_pSelf->m_BtnTongList.Init(&Ini, "BtnTongList");
			ms_pSelf->m_TabBtnBaseInfo.Init(&Ini, "BtnBaseInfo");
			ms_pSelf->m_TabBtnRecruit.Init(&Ini, "BtnZhaoMu");
			ms_pSelf->m_TabBtnFunUse.Init(&Ini, "BtnFunUse");
			ms_pSelf->m_TabBtnRight.Init(&Ini, "BtnRightManage");
			ms_pSelf->m_TabBtnWorkShop.Init(&Ini, "BtnWorkShop");
			ms_pSelf->m_TabBtnTongRec.Init(&Ini, "BtnTongRecord");
			
			ms_pSelf->m_BgBaseInfo.Init(&Ini, "BgBaseInfo");
			ms_pSelf->m_BgRecruitSelf.Init(&Ini, "BgRecruitSelf");
			ms_pSelf->m_BgRecruitOth.Init(&Ini, "BgRecruitOth");
			ms_pSelf->m_BgFunUse.Init(&Ini, "BgFunUse");
			ms_pSelf->m_BgRight.Init(&Ini, "BgRight");
			ms_pSelf->m_BgWorkShop.Init(&Ini, "BgWorkShop");
			ms_pSelf->m_BgWorkShopImg.Init(&Ini, "BgWorkShopImg");
			ms_pSelf->m_BgRecord.Init(&Ini, "BgRecord");
			ms_pSelf->m_P0_TitleTongInfo.Init(&Ini, "P0_TitleTongInfo");
			ms_pSelf->m_P0_TitlePersonalInfo.Init(&Ini, "P0_TitlePersonalInfo");
			ms_pSelf->m_P0_TxtHelpTitle.Init(&Ini, "P0_TxtHelpTitle");
			
			ms_pSelf->m_P0_TitleTongName.Init(&Ini, "P0_TitleTongName");
			ms_pSelf->m_P0_TitleMaster.Init(&Ini, "P0_TitleMaster");
			ms_pSelf->m_P0_TitleLeague.Init(&Ini, "P0_TitleLeague");
			ms_pSelf->m_P0_TitleCamp.Init(&Ini, "P0_TitleCamp");
			ms_pSelf->m_P0_TitleTongLevel.Init(&Ini, "P0_TitleTongLevel");
			ms_pSelf->m_P0_TitleMemberNum.Init(&Ini, "P0_TitleMemberNum");
			ms_pSelf->m_P0_TitleBuildLevel.Init(&Ini, "P0_TitleBuildLevel");
			ms_pSelf->m_P0_TitleTongCapital.Init(&Ini, "P0_TitleTongCapital");
			ms_pSelf->m_P0_TitleBuildFund.Init(&Ini, "P0_TitleBuildFund");
			ms_pSelf->m_P0_TitleTotalOffer.Init(&Ini, "P0_TitleTotalOffer");
			Ini.GetString("P0_TitleTongName", "HelpInfo", "", ms_pSelf->m_szHelp[0], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleMaster", "HelpInfo", "", ms_pSelf->m_szHelp[1], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleLeague", "HelpInfo", "", ms_pSelf->m_szHelp[2], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleCamp", "HelpInfo", "", ms_pSelf->m_szHelp[3], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleTongLevel", "HelpInfo", "", ms_pSelf->m_szHelp[4], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleMemberNum", "HelpInfo", "", ms_pSelf->m_szHelp[5], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleBuildLevel", "HelpInfo", "", ms_pSelf->m_szHelp[6], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleTongCapital", "HelpInfo", "", ms_pSelf->m_szHelp[7], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleBuildFund", "HelpInfo", "", ms_pSelf->m_szHelp[8], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleTotalOffer", "HelpInfo", "", ms_pSelf->m_szHelp[9], sizeof(ms_pSelf->m_szHelp[0]));
			
			ms_pSelf->m_P0_TxtTongName.Init(&Ini, "P0_TxtTongName");
			ms_pSelf->m_P0_TxtMaster.Init(&Ini, "P0_TxtMaster");
			ms_pSelf->m_P0_TxtLeague.Init(&Ini, "P0_TxtLeague");
			ms_pSelf->m_P0_TxtCamp.Init(&Ini, "P0_TxtCamp");
			ms_pSelf->m_P0_TxtTongLevel.Init(&Ini, "P0_TxtTongLevel");
			ms_pSelf->m_P0_TxtMemberNum.Init(&Ini, "P0_TxtMemberNum");
			ms_pSelf->m_P0_TxtBuildLevel.Init(&Ini, "P0_TxtBuildLevel");
			ms_pSelf->m_P0_TxtTongCapital.Init(&Ini, "P0_TxtTongCapital");
			ms_pSelf->m_P0_TxtBuildFund.Init(&Ini, "P0_TxtBuildFund");
			ms_pSelf->m_P0_TxtTotalOffer.Init(&Ini, "P0_TxtTotalOffer");
			ms_pSelf->m_P0_TxtMoneyUnit1.Init(&Ini, "P0_TxtMoneyUnit1");
			ms_pSelf->m_P0_TxtMoneyUnit2.Init(&Ini, "P0_TxtMoneyUnit2");
			ms_pSelf->m_P0_TitlePersonalOffer.Init(&Ini, "P0_TitlePersonalOffer");
			ms_pSelf->m_P0_TxtPersonalOffer.Init(&Ini, "P0_TxtPersonalOffer");
			ms_pSelf->m_P0_TitleWeeklyOffer.Init(&Ini, "P0_TitleWeeklyOffer");
			ms_pSelf->m_P0_TxtWeeklyOffer.Init(&Ini, "P0_TxtWeeklyOffer");
			Ini.GetString("P0_TitlePersonalOffer", "HelpInfo", "", ms_pSelf->m_szHelp[10], sizeof(ms_pSelf->m_szHelp[0]));
			Ini.GetString("P0_TitleWeeklyOffer", "HelpInfo", "", ms_pSelf->m_szHelp[11], sizeof(ms_pSelf->m_szHelp[0]));
			ms_pSelf->m_P0_TxtHelp.Init(&Ini, "P0_TxtHelp");
			ms_pSelf->m_BtnApply.Init(&Ini, "BtnApply");
			ms_pSelf->m_TxtRank.Init(&Ini, "TxtRank");
			ms_pSelf->m_TxtTitle.Init(&Ini, "TxtTitle");
			ms_pSelf->m_TxtType.Init(&Ini, "TxtType");
			ms_pSelf->m_MemList.Init(&Ini, "MemberList");
			Ini.GetString("MemberList", "OnlineColor", "", Buff, sizeof(Buff));
			ms_pSelf->m_OnlineColor = GetColor(Buff);
			Ini.GetString("MemberList", "OfflineColor", "", Buff, sizeof(Buff));
			ms_pSelf->m_OfflineColor = GetColor(Buff);
			ms_pSelf->m_MemList.SetMsgColor(ms_pSelf->m_OnlineColor);
			ms_pSelf->m_BtnPrevPage.Init(&Ini, "BtnPrevPage");
			ms_pSelf->m_BtnNextPage.Init(&Ini, "BtnNextPage");
			ms_pSelf->m_BtnJump.Init(&Ini, "BtnJump");
			ms_pSelf->m_BtnOnlinePriority.Init(&Ini, "BtnOnlinePriority");
			ms_pSelf->m_BtnSortMenu.Init(&Ini, "BtnSortMenu");
			ms_pSelf->m_EditBoxDestPage.Init(&Ini, "EditBoxDestPage");
			
			ms_pSelf->m_P2_TxtPersonalInfo.Init(&Ini, "P2_TxtPersonalInfo");
			ms_pSelf->m_P2_TitleTongName.Init(&Ini, "P2_TitleTongName");
			ms_pSelf->m_P2_TitleTongUnion.Init(&Ini, "P2_TitleTongUnion");
			ms_pSelf->m_P2_TitleBuildLevel.Init(&Ini, "P2_TitleBuildLevel");
			ms_pSelf->m_P2_TxtTongName.Init(&Ini, "P2_TxtTongName");
			ms_pSelf->m_P2_TxtTongUnion.Init(&Ini, "P2_TxtTongUnion");
			ms_pSelf->m_P2_TxtBuildLevel.Init(&Ini, "P2_TxtBuildLevel");
			ms_pSelf->m_P2_TitleTotalOffer.Init(&Ini, "P2_TitleTotalOffer");
			ms_pSelf->m_P2_TxtTotalOffer.Init(&Ini, "P2_TxtTotalOffer");
			ms_pSelf->m_P2_TitleTongCapital.Init(&Ini, "P2_TitleTongCapital");
			ms_pSelf->m_P2_TxtTongCapital.Init(&Ini, "P2_TxtTongCapital");
			ms_pSelf->m_P2_TitleBuildFund.Init(&Ini, "P2_TitleBuildFund");
			ms_pSelf->m_P2_TxtBuildFund.Init(&Ini, "P2_TxtBuildFund");
			ms_pSelf->m_P2_TxtMoneyUnit.Init(&Ini, "P2_TxtMoneyUnit");
			ms_pSelf->m_P2_TxtExpPercent.Init(&Ini, "P2_TxtExpPercent");
			ms_pSelf->m_P2_TitleExp.Init(&Ini, "P2_TitleExp");
			ms_pSelf->m_P2_ImgExp.Init(&Ini, "P2_ImgExp");
			ms_pSelf->m_P2_TitlePersonalOffer.Init(&Ini, "P2_TitlePersonalOffer");
			ms_pSelf->m_P2_TxtPersonalOffer.Init(&Ini, "P2_TxtPersonalOffer");
			ms_pSelf->m_P2_TitleTongCapital2.Init(&Ini, "P2_TitleTongCapital2");
			ms_pSelf->m_P2_TxtTongCapital2.Init(&Ini, "P2_TxtTongCapital2");
			ms_pSelf->m_P2_TitleBuildFund2.Init(&Ini, "P2_TitleBuildFund2");
			ms_pSelf->m_P2_TxtBuildFund2.Init(&Ini, "P2_TxtBuildFund2");
			ms_pSelf->m_BtnLeaveTong.Init(&Ini, "BtnLeaveTong");
			ms_pSelf->m_BtnUpgradeBuildLevel.Init(&Ini, "BtnUpgradeBuildLevel");
			ms_pSelf->m_BtnAssignTongOffer.Init(&Ini, "BtnAssignTongOffer");
			ms_pSelf->m_BtnGetTongMoney.Init(&Ini, "BtnGetTongMoney");
			ms_pSelf->m_BtnAssignTongMoney.Init(&Ini, "BtnAssignTongMoney");
			ms_pSelf->m_BtnTransformMoney.Init(&Ini, "BtnTransformMoney");
			ms_pSelf->m_BtnStorePersonalOffer.Init(&Ini, "BtnStorePersonalOffer");
			ms_pSelf->m_BtnStoreTongMoney.Init(&Ini, "BtnStoreTongMoney");
			ms_pSelf->m_BtnStoreBuildFund.Init(&Ini, "BtnStoreBuildFund");
			ms_pSelf->m_BtnSubPage[0].Init(&Ini, "BtnSubPage1");
			ms_pSelf->m_BtnSubPage[1].Init(&Ini, "BtnSubPage2");
			ms_pSelf->m_BtnSubPage[2].Init(&Ini, "BtnSubPage3");
			ms_pSelf->m_BtnSubPage[0].CheckButton(1);
			ms_pSelf->m_nCurFunUseTab = 0;
			ms_pSelf->m_BtnDemise.Init(&Ini, "BtnDemise");
			ms_pSelf->m_BtnForceToRetire.Init(&Ini, "BtnForceToRetire");
			ms_pSelf->m_BtnKickOut.Init(&Ini, "BtnKickOut");
			ms_pSelf->m_BtnDepose.Init(&Ini, "BtnDepose");
			ms_pSelf->m_Btn_DispenseOffer.Init(&Ini, "Btn_DispenseOffer");
			ms_pSelf->m_BtnChangeMaleTitle.Init(&Ini, "BtnChangeMaleTitle");
			ms_pSelf->m_BtnChangeFemaleTitle.Init(&Ini, "BtnChangeFemaleTitle");
			ms_pSelf->m_BtnChangeTitle.Init(&Ini, "BtnChangeTitle");
			ms_pSelf->m_BtnRecruit.Init(&Ini, "BtnRecruit");
			ms_pSelf->m_BtnChangeCamp.Init(&Ini, "BtnChangeCamp");
			ms_pSelf->m_BtnCreateTongMap.Init(&Ini, "BtnCreateTongMap");
			ms_pSelf->m_BtnConfigureTongMap.Init(&Ini, "BtnConfigureTongMap");
			ms_pSelf->m_BtnTongChallenge.Init(&Ini, "BtnTongChallenge");
			ms_pSelf->m_BtnCreateUnion.Init(&Ini, "BtnCreateUnion");
			ms_pSelf->m_BtnApplyJionUnion.Init(&Ini, "BtnApplyJionUnion");
			ms_pSelf->m_BtnLeaveUnion.Init(&Ini, "BtnLeaveUnion");
			ms_pSelf->m_BtnKickUnionTong.Init(&Ini, "BtnKickUnionTong");
			ms_pSelf->m_BtnAcceptUnionReq.Init(&Ini, "BtnAcceptUnionReq");
			ms_pSelf->m_P3_BtnDepose.Init(&Ini, "P3_BtnDepose");
			ms_pSelf->m_P3_BtnChangeCamp.Init(&Ini, "P3_BtnChangeCamp");
			ms_pSelf->m_P3_BtnChangeTitle.Init(&Ini, "P3_BtnChangeTitle");
			ms_pSelf->m_P3_BtnKickOut.Init(&Ini, "P3_BtnKickOut");
			ms_pSelf->m_BtnRecordEvent.Init(&Ini, "BtnRecordEvent");
			ms_pSelf->m_BtnLeagueManage.Init(&Ini, "BtnLeagueManage");
			ms_pSelf->m_P3_BtnUpgradeBuildLevel.Init(&Ini, "P3_BtnUpgradeBuildLevel");
			ms_pSelf->m_P3_BtnForceToRetire.Init(&Ini, "P3_BtnForceToRetire");
			ms_pSelf->m_BtnMapManagement.Init(&Ini, "BtnMapManagement");
			ms_pSelf->m_BtnWorkshopManagement.Init(&Ini, "BtnWorkshopManagement");
			ms_pSelf->m_BtnTongClaimWar.Init(&Ini, "BtnTongClaimWar");
			ms_pSelf->m_BtnFundManagement.Init(&Ini, "BtnFundManagement");
			ms_pSelf->m_BtnWeekGoalManagement.Init(&Ini, "BtnWeekGoalManagement");
			ms_pSelf->m_P3_BtnRecruit.Init(&Ini, "P3_BtnRecruit");
			ms_pSelf->m_BtnSelectAll.Init(&Ini, "BtnSelectAll");
			ms_pSelf->m_BtnDistribute.Init(&Ini, "BtnDistribute");
			ms_pSelf->m_BtnWeekDaily.Init(&Ini, "BtnWeekDaily");
			ms_pSelf->m_BtnAnnounce.Init(&Ini, "BtnAnnounce");
			ms_pSelf->m_BtnTongAffair.Init(&Ini, "BtnTongAffair");
			ms_pSelf->m_BtnTongHistory.Init(&Ini, "BtnTongHistory");
			ms_pSelf->m_BtnWeekDaily.CheckButton(1);
			ms_pSelf->m_nCurRecordTab = 0;
			ms_pSelf->m_RecordList.Init(&Ini, "RecordList");
			ms_pSelf->m_AnnounceEditor.Init(&Ini, "AnnounceEditor");
			ms_pSelf->m_BtnLeaveWord.Init(&Ini, "BtnLeaveWord");
			ms_pSelf->m_BtnEditAnnounce.Init(&Ini, "BtnEditAnnounce");
		}
	}
}

void KUiTongManager::Breathe()
{
	char szBuffer[32];
	int nValue = g_pCoreShell->TongOperation(GTOI_GET_WEEKOFFER, 0, 0);
	sprintf(szBuffer, "%d", nValue);
	m_P0_TxtWeeklyOffer.SetLabel(szBuffer);
	nValue = g_pCoreShell->TongOperation(GTOI_GET_PERSONALOFFER, 0, 0);
	sprintf(szBuffer, "%d", nValue);
	m_P0_TxtPersonalOffer.SetLabel(szBuffer);
	m_P2_TxtPersonalOffer.SetLabel(szBuffer);
	if(m_bHideAnnounce)
	{
		m_bHideAnnounce = false;
		m_AnnounceEditor.Hide();
	}
}

void KUiTongManager::ShowTab(int nTab)
{
	if(nTab == 0)
	{
		m_TabBtnBaseInfo.CheckButton(1);
		m_TabBtnRecruit.CheckButton(0);
		m_TabBtnFunUse.CheckButton(0);
		m_TabBtnRight.CheckButton(0);
		m_TabBtnWorkShop.CheckButton(0);
		m_TabBtnTongRec.CheckButton(0);
	}
	else if(nTab == 1)
	{
		m_TabBtnBaseInfo.CheckButton(0);
		m_TabBtnRecruit.CheckButton(1);
		m_TabBtnFunUse.CheckButton(0);
		m_TabBtnRight.CheckButton(0);
		m_TabBtnWorkShop.CheckButton(0);
		m_TabBtnTongRec.CheckButton(0);
	}
	else if(nTab == 2)
	{
		m_TabBtnBaseInfo.CheckButton(0);
		m_TabBtnRecruit.CheckButton(0);
		m_TabBtnFunUse.CheckButton(1);
		m_TabBtnRight.CheckButton(0);
		m_TabBtnWorkShop.CheckButton(0);
		m_TabBtnTongRec.CheckButton(0);
	}
	else if(nTab == 3)
	{
		m_TabBtnBaseInfo.CheckButton(0);
		m_TabBtnRecruit.CheckButton(0);
		m_TabBtnFunUse.CheckButton(0);
		m_TabBtnRight.CheckButton(1);
		m_TabBtnWorkShop.CheckButton(0);
		m_TabBtnTongRec.CheckButton(0);
	}
	else if(nTab == 4)
	{
		m_TabBtnBaseInfo.CheckButton(0);
		m_TabBtnRecruit.CheckButton(0);
		m_TabBtnFunUse.CheckButton(0);
		m_TabBtnRight.CheckButton(0);
		m_TabBtnWorkShop.CheckButton(1);
		m_TabBtnTongRec.CheckButton(0);
	}
	else if(nTab == 5)
	{
		m_TabBtnBaseInfo.CheckButton(0);
		m_TabBtnRecruit.CheckButton(0);
		m_TabBtnFunUse.CheckButton(0);
		m_TabBtnRight.CheckButton(0);
		m_TabBtnWorkShop.CheckButton(0);
		m_TabBtnTongRec.CheckButton(1);
	}
	if(m_nCurTab == nTab)
		return;
	m_nCurTab = nTab;
	m_TabBtnBaseInfo.Enable(1);
	if(m_uMeTongID)
	{
		m_TabBtnFunUse.Enable(1);
		m_TabBtnRight.Enable(1);
		m_TabBtnWorkShop.Enable(1);
		m_TabBtnTongRec.Enable(1);
	}
	else
	{
		m_TabBtnFunUse.Enable(0);
		m_TabBtnRight.Enable(0);
		m_TabBtnWorkShop.Enable(0);
		m_TabBtnTongRec.Enable(0);
	}
	m_BgBaseInfo.Hide();
	m_BgRecruitSelf.Hide();
	m_BgRecruitOth.Hide();
	m_BgFunUse.Hide();
	m_BgRight.Hide();
	m_BgWorkShop.Hide();
	m_BgWorkShopImg.Hide();
	m_BgRecord.Hide();
	m_P0_TitleTongInfo.Hide();
	m_P0_TitlePersonalInfo.Hide();
	m_P0_TxtHelpTitle.Hide();
	m_P0_TitleTongName.Hide();
	m_P0_TitleMaster.Hide();
	m_P0_TitleLeague.Hide();
	m_P0_TitleCamp.Hide();
	m_P0_TitleTongLevel.Hide();
	m_P0_TitleMemberNum.Hide();
	m_P0_TitleBuildLevel.Hide();
	m_P0_TitleTongCapital.Hide();
	m_P0_TitleBuildFund.Hide();
	m_P0_TitleTotalOffer.Hide();
	m_P0_TxtMoneyUnit1.Hide();
	m_P0_TxtMoneyUnit2.Hide();
	m_P0_TxtTongName.Hide();
	m_P0_TxtMaster.Hide();
	m_P0_TxtLeague.Hide();
	m_P0_TxtCamp.Hide();
	m_P0_TxtTongLevel.Hide();
	m_P0_TxtMemberNum.Hide();
	m_P0_TxtBuildLevel.Hide();
	m_P0_TxtTongCapital.Hide();
	m_P0_TxtBuildFund.Hide();
	m_P0_TxtTotalOffer.Hide();
	
	m_P0_TitlePersonalOffer.Hide();
	m_P0_TxtPersonalOffer.Hide();
	m_P0_TitleWeeklyOffer.Hide();
	m_P0_TxtWeeklyOffer.Hide();
	m_BtnApply.Hide();
	m_P0_TxtHelp.Hide();
	m_TxtRank.Hide();
	m_TxtTitle.Hide();
	m_TxtType.Hide();
	m_MemList.Hide();
	m_BtnPrevPage.Hide();
	m_BtnNextPage.Hide();
	m_BtnJump.Hide();
	m_BtnOnlinePriority.CheckButton(0);
	m_BtnOnlinePriority.Hide();
	m_BtnSortMenu.Hide();
	m_EditBoxDestPage.Hide();
	
	m_P2_TxtPersonalInfo.Hide();
	m_P2_TitleTongName.Hide();
	m_P2_TitleTongUnion.Hide();
	m_P2_TxtTongName.Hide();
	m_P2_TxtTongUnion.Hide();
	m_P2_TitleBuildLevel.Hide();
	m_P2_TxtBuildLevel.Hide();
	m_P2_TitleTotalOffer.Hide();
	m_P2_TxtTotalOffer.Hide();
	m_P2_TitleTongCapital.Hide();
	m_P2_TxtTongCapital.Hide();
	m_P2_TitleBuildFund.Hide();
	m_P2_TxtBuildFund.Hide();
	m_P2_TxtMoneyUnit.Hide();
	m_P2_TitleExp.Hide();
	m_P2_ImgExp.Hide();
	m_P2_TxtExpPercent.Hide();
	m_P2_TitlePersonalOffer.Hide();
	m_P2_TxtPersonalOffer.Hide();
	m_P2_TitleTongCapital2.Hide();
	m_P2_TxtTongCapital2.Hide();
	m_P2_TitleBuildFund2.Hide();
	m_P2_TxtBuildFund2.Hide();
	m_BtnLeaveTong.Hide();
	m_BtnUpgradeBuildLevel.Hide();
	m_BtnAssignTongOffer.Hide();
	m_BtnGetTongMoney.Hide();
	m_BtnAssignTongMoney.Hide();
	m_BtnTransformMoney.Hide();
	m_BtnStorePersonalOffer.Hide();
	m_BtnStoreTongMoney.Hide();
	m_BtnStoreBuildFund.Hide();
	m_BtnSubPage[0].Hide();
	m_BtnSubPage[1].Hide();
	m_BtnSubPage[2].Hide();
	m_BtnDemise.Hide();
	m_BtnForceToRetire.Hide();
	m_BtnKickOut.Hide();
	m_BtnDepose.Hide();
	m_Btn_DispenseOffer.Hide();
	m_BtnChangeMaleTitle.Hide();
	m_BtnChangeFemaleTitle.Hide();
	m_BtnChangeTitle.Hide();
	m_BtnRecruit.Hide();
	m_BtnChangeCamp.Hide();
	m_BtnCreateTongMap.Hide();
	m_BtnConfigureTongMap.Hide();
	m_BtnTongChallenge.Hide();
	m_BtnCreateUnion.Hide();
	m_BtnApplyJionUnion.Hide();
	m_BtnLeaveUnion.Hide();
	m_BtnKickUnionTong.Hide();
	m_BtnAcceptUnionReq.Hide();
	m_P3_BtnDepose.Hide();
	m_P3_BtnChangeCamp.Hide();
	m_P3_BtnChangeTitle.Hide();
	m_P3_BtnKickOut.Hide();
	m_BtnRecordEvent.Hide();
	m_BtnLeagueManage.Hide();
	m_P3_BtnUpgradeBuildLevel.Hide();
	m_P3_BtnForceToRetire.Hide();
	m_BtnMapManagement.Hide();
	m_BtnWorkshopManagement.Hide();
	m_BtnTongClaimWar.Hide();
	m_BtnFundManagement.Hide();
	m_BtnWeekGoalManagement.Hide();
	m_P3_BtnRecruit.Hide();
	m_BtnSelectAll.Hide();
	m_BtnDistribute.Hide();
	m_BtnWeekDaily.Hide();
	m_BtnAnnounce.Hide();
	m_BtnTongAffair.Hide();
	m_BtnTongHistory.Hide();
	m_RecordList.Hide();
	m_AnnounceEditor.Hide();
	m_BtnLeaveWord.Hide();
	m_BtnEditAnnounce.Hide();
	
	int nCurTime = g_pCoreShell->GetGameData(GDI_GET_GAME_TIME, 0, 0);
	if(nTab == 0)
	{
		m_TxtTitle.SetText("Thµnh viªn");
		m_TxtType.SetText("Chøc vô");
		m_BgBaseInfo.Show();
		m_P0_TitleTongInfo.Show();
		m_P0_TitlePersonalInfo.Show();
		m_P0_TxtHelpTitle.Show();
		
		m_P0_TitleTongName.Show();
		m_P0_TitleMaster.Show();
		m_P0_TitleLeague.Show();
		m_P0_TitleCamp.Show();
		m_P0_TitleTongLevel.Show();
		m_P0_TitleMemberNum.Show();
		m_P0_TitleBuildLevel.Show();
		m_P0_TitleTongCapital.Show();
		m_P0_TitleBuildFund.Show();
		m_P0_TitleTotalOffer.Show();
		m_P0_TxtMoneyUnit1.Show();
		m_P0_TxtMoneyUnit2.Show();
		m_P0_TxtTongName.Show();
		m_P0_TxtMaster.Show();
		m_P0_TxtLeague.Show();
		m_P0_TxtCamp.Show();
		m_P0_TxtTongLevel.Show();
		m_P0_TxtMemberNum.Show();
		m_P0_TxtBuildLevel.Show();
		m_P0_TxtTongCapital.Show();
		m_P0_TxtBuildFund.Show();
		m_P0_TxtTotalOffer.Show();
		m_P0_TitlePersonalOffer.Show();
		m_P0_TxtPersonalOffer.Show();
		m_P0_TitleWeeklyOffer.Show();
		m_P0_TxtWeeklyOffer.Show();
		
		m_P0_TxtHelp.Show();
		m_TxtRank.Show();
		m_TxtTitle.Show();
		m_TxtType.Show();
		m_MemList.Show();
		m_BtnPrevPage.Show();
		m_BtnNextPage.Show();
		m_BtnJump.Show();
		m_BtnOnlinePriority.Show();
		m_BtnSortMenu.Show();
		m_EditBoxDestPage.Show();
		if(m_szDestName[0] && !m_uMeTongID)
			m_BtnApply.Show();
		if(m_nNextTimeInfo < nCurTime)
		{
			KUiPlayerItem Player;
			memset(&Player, 0, sizeof(KUiPlayerItem));
			m_nNextTimeInfo = nCurTime + 4;
			Player.uId = m_uDestTongID?m_uDestTongID:m_uMeTongID;
			g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_ID_TONG_HEAD);
			if(!m_uDestTongID && m_uMeTongID)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				Player.uId = m_uMeTongID;
				Player.nIndex = m_nCurMemPage; //page
				g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
			}
		}
	}
	else if(nTab == 1)
	{
		if(m_uMeTongID)
		{
			m_BgRecruitSelf.Show();
		}
		else
		{
			if(!m_szDestName[0])
				m_TabBtnBaseInfo.Enable(0);
			m_BgRecruitOth.Show();
		}
	}
	else if(nTab == 2)
	{
		m_BgFunUse.Show();
		m_P0_TitleTongInfo.Show();
		m_TxtRank.Show();
		m_TxtTitle.Show();
		m_TxtType.Show();
		m_MemList.Show();
		m_BtnPrevPage.Show();
		m_BtnNextPage.Show();
		m_BtnJump.Show();
		m_BtnOnlinePriority.Show();
		m_BtnSortMenu.Show();
		m_EditBoxDestPage.Show();
		
		m_P2_TxtPersonalInfo.Show();
		m_P2_TitleTongName.Show();
		m_P2_TitleTongUnion.Show();
		m_P2_TxtTongName.Show();
		m_P2_TxtTongUnion.Show();
		m_P2_TitleBuildLevel.Show();
		m_P2_TxtBuildLevel.Show();
		m_P2_TitleTotalOffer.Show();
		m_P2_TxtTotalOffer.Show();
		m_P2_TitleTongCapital.Show();
		m_P2_TxtTongCapital.Show();
		m_P2_TitleBuildFund.Show();
		m_P2_TxtBuildFund.Show();
		m_P2_TxtMoneyUnit.Show();
		m_P2_TitleExp.Show();
		m_P2_ImgExp.Show();
		m_P2_TxtExpPercent.Show();
		m_P2_TitlePersonalOffer.Show();
		m_P2_TxtPersonalOffer.Show();
		m_P2_TitleTongCapital2.Show();
		m_P2_TxtTongCapital2.Show();
		m_P2_TitleBuildFund2.Show();
		m_P2_TxtBuildFund2.Show();
		m_BtnLeaveTong.Show();
		m_BtnUpgradeBuildLevel.Show();
		m_BtnAssignTongOffer.Show();
		m_BtnGetTongMoney.Show();
		m_BtnAssignTongMoney.Show();
		m_BtnTransformMoney.Show();
		m_BtnStorePersonalOffer.Show();
		m_BtnStoreTongMoney.Show();
		m_BtnStoreBuildFund.Show();
		m_BtnSubPage[0].Show();
		m_BtnSubPage[1].Show();
		m_BtnSubPage[2].Show();
		if(m_nNextTimeInfo < nCurTime)
		{
			m_nNextTimeInfo = nCurTime + 4;
			KUiPlayerItem Player;
			memset(&Player, 0, sizeof(KUiPlayerItem));
			Player.uId = m_uMeTongID;
			g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_ID_TONG_HEAD);
		}
		if(m_BtnSubPage[0].IsButtonChecked())
		{
			m_TxtTitle.SetText("Thµnh viªn");
			m_TxtType.SetText("Chøc vô");
			m_BtnDemise.Show();
			m_BtnForceToRetire.Show();
			m_BtnKickOut.Show();
			m_BtnDepose.Show();
			m_Btn_DispenseOffer.Show();
			m_BtnChangeMaleTitle.Show();
			m_BtnChangeFemaleTitle.Show();
			m_BtnChangeTitle.Show();
			m_BtnRecruit.Show();
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				KUiPlayerItem Player;
				memset(&Player, 0, sizeof(KUiPlayerItem));
				Player.uId = m_uMeTongID;
				Player.nIndex = m_nCurMemPage; //page
				g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
			}
		}
		else if(m_BtnSubPage[1].IsButtonChecked())
		{
			m_TxtTitle.SetText("Bang héi");
			m_TxtType.SetText("§¼ng cÊp");
			m_BtnChangeCamp.Show();
			m_BtnCreateTongMap.Show();
			m_BtnConfigureTongMap.Show();
			m_BtnTongChallenge.Show();
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				KUiPlayerItem Player;
				memset(&Player, 0, sizeof(KUiPlayerItem));
				Player.uId = 0;
				Player.nIndex = m_nCurTongPage; //page
				g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
			}
		}
		else if(m_BtnSubPage[2].IsButtonChecked())
		{
			m_TxtTitle.SetText("Liªn minh");
			m_TxtType.SetText("Sè bang");
			m_BtnCreateUnion.Show();
			m_BtnApplyJionUnion.Show();
			m_BtnLeaveUnion.Show();
			m_BtnKickUnionTong.Show();
			m_BtnAcceptUnionReq.Show();
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				KUiPlayerItem Player;
				memset(&Player, 0, sizeof(KUiPlayerItem));
				Player.uId = 0;
				Player.nIndex = m_nCurUnionPage; //page
				g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
			}
		}
	}
	else if(nTab == 3)
	{
		m_TxtTitle.SetText("Thµnh viªn");
		m_TxtType.SetText("Chøc vô");
		m_BgRight.Show();
		m_TxtRank.Show();
		m_TxtTitle.Show();
		m_TxtType.Show();
		m_MemList.Show();
		m_BtnPrevPage.Show();
		m_BtnNextPage.Show();
		m_BtnJump.Show();
		m_BtnOnlinePriority.Show();
		m_BtnSortMenu.Show();
		m_EditBoxDestPage.Show();
		
		m_P3_BtnDepose.Show();
		m_P3_BtnChangeCamp.Show();
		m_P3_BtnChangeTitle.Show();
		m_P3_BtnKickOut.Show();
		m_BtnRecordEvent.Show();
		m_BtnLeagueManage.Show();
		m_P3_BtnUpgradeBuildLevel.Show();
		m_P3_BtnForceToRetire.Show();
		m_BtnMapManagement.Show();
		m_BtnWorkshopManagement.Show();
		m_BtnTongClaimWar.Show();
		m_BtnFundManagement.Show();
		m_BtnWeekGoalManagement.Show();
		m_P3_BtnRecruit.Show();
		m_BtnSelectAll.Show();
		m_BtnDistribute.Show();
		if(m_nNextTimeInfo < nCurTime)
		{
			m_nNextTimeInfo = nCurTime + 4;
			KUiPlayerItem Player;
			memset(&Player, 0, sizeof(KUiPlayerItem));
			Player.uId = m_uMeTongID;
			g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_ID_TONG_HEAD);
			
			m_nNextTimeMemPage = nCurTime + 4;
			Player.uId = m_uMeTongID;
			Player.nIndex = m_nCurMemPage; //page
			g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
		}
	}
	else if(nTab == 4)
	{
		m_BgWorkShop.Show();
		m_BgWorkShopImg.Show();
	}
	else if(nTab == 5)
	{
		m_BgRecord.Show();
		m_BtnWeekDaily.Show();
		m_BtnAnnounce.Show();
		m_BtnTongAffair.Show();
		m_BtnTongHistory.Show();
		m_RecordList.Show();
		m_RecordList.GetMessageListBox()->Clear();
		if(m_BtnAnnounce.IsButtonChecked())
		{
			m_BtnLeaveWord.Show();
			m_BtnEditAnnounce.Show();
		}
		if(m_nNextTimeMemPage < nCurTime)
		{
			m_nNextTimeMemPage = nCurTime + 4;
			KUiPlayerItem Player;
			memset(&Player, 0, sizeof(KUiPlayerItem));
			Player.uId = 0;
			Player.nIndex = m_nCurRecordTab; //id page nhat ky
			g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
		}
	}
}

void KUiTongManager::ArrangeComposition(char* pszPlayerName)
{
	m_nCurTab = -1;
	int nIndex = 0;
	m_szDestName[0] = 0;
	m_uDestTongID = 0;
	KUiPlayerItem Player;
	int nKind;
    memset(&Player, 0, sizeof(KUiPlayerItem));
    if(pszPlayerName && pszPlayerName[0])
    {
		if(g_pCoreShell->FindSpecialNPC(pszPlayerName, &Player, nKind))
		{
			strcpy(m_szDestName, Player.Name);
			nIndex = Player.nIndex;
			m_uDestTongID = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONGID_NPC, nIndex, 0);
		}
	}
	else
	{
    	KUiPlayerBaseInfo Me;
	    g_pCoreShell->GetGameData(GDI_PLAYER_BASE_INFO, (unsigned int)&Me, 0);
		if(g_pCoreShell->FindSpecialNPC(Me.Name, &Player, nKind))
			nIndex = Player.nIndex;
	}
	if(nIndex <= 0)
	{
		CloseWindow(false);
		return;
	}
	m_uMeTongID = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONGID_NPC, 0, 0);
	if(!m_uMeTongID)
	{
		m_nCurTongPage = 0;
		m_nCurUnionPage = 0;
	}
	else if(m_uDestTongID == m_uMeTongID)
	{
		m_uDestTongID = 0;
		m_szDestName[0] = 0;
	}
	if(m_szDestName[0] || m_uMeTongID)
	{
		if(m_szDestName[0])
		m_nCurMemPage = 0;
		ShowTab(0);
	}
	else
	{
		m_nCurMemPage = 0;
		ShowTab(1);
	}
}

enum ENUMPROPFRAME
{
	SORTMN_AVERAGEOF,
	SORTMN_WEEKOFFER,
	SORTMN_WGCOMPLETED,
	SORTMN_RETIRED,
	SORTMN_FIGURE,
	SORTMN_LASTTIME,
	SORTMN_COUNT,
};

static char MenuSortMemString[][64] =
{
	"§iÓm cèng hiÕn trung b×nh hµng ngµy",
	"§iÓm cèng hiÕn trong tuÇn",
	"§iÓm hoµn thµnh môc tiªu tuÇn",
	"Tho¸i Èn sü",
	"Chøc vÞ",
	"Thêi gian trªn m¹ng cuèi cïng",
};

void KUiTongManager::Popup_SortMenu()
{
	g_MouseOver.CancelMouseHoverInfo();
	KPopupMenuData* pMenuData = (KPopupMenuData*)malloc(MENU_DATA_SIZE(SORTMN_COUNT));
	if (pMenuData == NULL)
		return;

	KPopupMenu::InitMenuData(pMenuData, SORTMN_COUNT);
	pMenuData->usMenuFlag |= PM_F_AUTO_DEL_WHEN_HIDE;
	pMenuData->usMenuFlag &= ~PM_F_HAVE_HEAD_TAIL_IMG;
	for (int i = 0; i < SORTMN_COUNT; ++i)
	{
		strcpy(pMenuData->Items[i].szData, MenuSortMemString[i]);
		pMenuData->Items[i].uDataLen = strlen(pMenuData->Items[i].szData);
		pMenuData->Items[i].uID = i;
	}
	pMenuData->nNumItem = SORTMN_COUNT;
	int	x, y;
	m_BtnSortMenu.GetAbsolutePos(&x, &y);
	pMenuData->nX = x;
	pMenuData->nY = y;
	KPopupMenu::Popup(pMenuData, this, 0);
}

int KUiTongManager::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	int nCurTime = g_pCoreShell->GetGameData(GDI_GET_GAME_TIME, 0, 0);
	switch(uMsg)
	{

	case WND_N_BUTTON_CLICK:
		if(uParam == (unsigned int)&m_BtnClose)
		{
			CloseWindow(false);
		}
		else if(uParam == (unsigned int)&m_BtnJump)
		{
			if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID))
			{
				char Buffer[32];
				Buffer[0] = 0;
				m_EditBoxDestPage.GetText(Buffer, sizeof(Buffer), true);
				int nPage = atoi(Buffer);
				if(nPage <= 0)
					nPage = 1;
				nPage -= 1;
				if(m_nCurTab == 2 && m_BtnSubPage[1].IsButtonChecked())
				{
					if(nPage != m_nCurTongPage)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = 0;
							Player.nIndex = nPage;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
						}
					}
				}
				else if(m_nCurTab == 2 && m_BtnSubPage[2].IsButtonChecked())
				{
					if(nPage != m_nCurUnionPage)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = 0;
							Player.nIndex = nPage;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
						}
					}
				}
				else
				{
					if(nPage != m_nCurMemPage)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = m_uMeTongID;
							Player.nIndex = nPage;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
						}
					}
				}
			}
			else
				m_EditBoxDestPage.SetIntText(1);
		}
		else if(uParam == (unsigned int)&m_BtnPrevPage)
		{
			if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID))
			{
				if(m_nCurTab == 2 && m_BtnSubPage[1].IsButtonChecked())
				{
					if(m_nCurTongPage > 0)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = 0;
							Player.nIndex = m_nCurTongPage - 1;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
						}
					}
				}
				else if(m_nCurTab == 2 && m_BtnSubPage[2].IsButtonChecked())
				{
					if(m_nCurUnionPage > 0)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = 0;
							Player.nIndex = m_nCurUnionPage - 1;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
						}
					}
				}
				else
				{
					if(m_nCurMemPage > 0)
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));						
							Player.uId = m_uMeTongID;
							Player.nIndex = m_nCurMemPage - 1;
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
						}
					}
				}
			}
			else
				m_EditBoxDestPage.SetIntText(1);
		}
		else if(uParam == (unsigned int)&m_BtnNextPage)
		{
			if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID))
			{
				if(m_nCurTab == 2 && m_BtnSubPage[1].IsButtonChecked())
				{
					if(m_nNextTimeMemPage < nCurTime)
					{
						m_nNextTimeMemPage = nCurTime + 4;
						KUiPlayerItem Player;
						memset(&Player, 0, sizeof(KUiPlayerItem));						
						Player.uId = 0;
						Player.nIndex = m_nCurTongPage + 1;
						g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
					}
				}
				else if(m_nCurTab == 2 && m_BtnSubPage[2].IsButtonChecked())
				{
					if(m_nNextTimeMemPage < nCurTime)
					{
						m_nNextTimeMemPage = nCurTime + 4;
						KUiPlayerItem Player;
						memset(&Player, 0, sizeof(KUiPlayerItem));						
						Player.uId = 0;
						Player.nIndex = m_nCurUnionPage + 1;
						g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
					}
				}
				else
				{
					if(m_nNextTimeMemPage < nCurTime)
					{
						m_nNextTimeMemPage = nCurTime + 4;
						KUiPlayerItem Player;
						memset(&Player, 0, sizeof(KUiPlayerItem));						
						Player.uId = m_uMeTongID;
						Player.nIndex = m_nCurMemPage + 1;
						g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
					}
				}
			}
			else
				m_EditBoxDestPage.SetIntText(1);
		}
		else if(uParam == (unsigned int)&m_BtnOnlinePriority)
		{
			if(!m_uMeTongID || (m_nCurTab == 0 && m_uDestTongID)
			|| (m_nCurTab == 2 && (m_BtnSubPage[1].IsButtonChecked() || m_BtnSubPage[2].IsButtonChecked())))
			{	//xem bang khac -> khong check online offline
				m_BtnOnlinePriority.CheckButton(0);
			}
			else
			{	//xem bang chinh minh
				if(nParam) //chi hien thi tren mang
				{
					for(vector<STONG_MEMSUBINFO>::iterator it = m_vMemberList.begin(); it != m_vMemberList.end();)
					{
						if (!it->btOnline)
						{
							it = m_vMemberList.erase(it); // tra ve phan tu~ ke tiep
						}
						else
						{
							++it;
						}
					}
					m_MemList.Clear();
					for(UINT i=0;i < m_vMemberList.size(); ++i)
					{
						AddMemberList(i, m_vMemberList[i], false);
					}
				}
				else
				{ //hien thi tat ca
					if(m_nNextTimeMemPage < nCurTime)
					{
						m_nNextTimeMemPage = nCurTime + 4;
						KUiPlayerItem Player;
						memset(&Player, 0, sizeof(KUiPlayerItem));						
						Player.uId = m_uMeTongID;
						Player.nIndex = m_nCurMemPage;
						g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
					}
					else
						m_BtnOnlinePriority.CheckButton(1);
				}
			}
		}
		else if(uParam == (unsigned int)&m_BtnSortMenu)
		{
			if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID)
			&& (m_nCurTab != 2 || m_BtnSubPage[0].IsButtonChecked()))
				Popup_SortMenu();
		}
		else if(uParam == (unsigned int)&m_BtnRefresh)
		{
			if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID))
			{
				if(m_nNextTimeInfo < nCurTime)
				{
					m_nNextTimeInfo = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = m_uMeTongID;
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_ID_TONG_HEAD);
				}
				if(m_nCurTab == 0 || m_nCurTab == 2 || m_nCurTab == 3)
				{
					if(m_nCurTab == 2 && m_BtnSubPage[1].IsButtonChecked())
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));
							Player.uId = 0;
							Player.nIndex = m_nCurTongPage; //page
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
						}
					}
					else if(m_nCurTab == 2 && m_BtnSubPage[2].IsButtonChecked())
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));
							Player.uId = 0;
							Player.nIndex = m_nCurUnionPage; //page
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
						}
					}
					else
					{
						if(m_nNextTimeMemPage < nCurTime)
						{
							m_nNextTimeMemPage = nCurTime + 4;
							KUiPlayerItem Player;
							memset(&Player, 0, sizeof(KUiPlayerItem));
							Player.uId = m_uMeTongID;
							Player.nIndex = m_nCurMemPage; //page
							g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
						}
					}
				}
				else if(m_nCurTab == 5)
				{
					if(m_nNextTimeMemPage < nCurTime)
					{
						m_RecordList.GetMessageListBox()->Clear();
						m_nNextTimeMemPage = nCurTime + 4;
						KUiPlayerItem Player;
						memset(&Player, 0, sizeof(KUiPlayerItem));
						Player.uId = 0;
						Player.nIndex = m_nCurRecordTab; //id page nhat ky
						g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
					}
				}
			}
		}
		else if(uParam == (unsigned int)&m_BtnKickOut)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_KICKOUT))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER
			|| m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox("Kh«ng thÓ trôc xuÊt Bang chñ vµ Tr­ëng l·o khái bang héi");
				break;
			}
			char szBuff[256];
			sprintf(szBuff,
				"X¸c nhËn muèn trôc xuÊt %s khái bang héi? SÏ tiªu hao ng©n quü dùa trªn ®iÓm cèng hiÕn cña ng­êi ®ã, ng­¬i ch¾c ch¾c kh«ng?",
				m_vMemberList[nSelMsg].m_szName);
			UIMessageBox(szBuff,
				this, "X¸c nhËn", "Hñy bá", TONG_ACTION_DISMISS);
		}
		else if(uParam == (unsigned int)&m_BtnDepose)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_DEPOSE))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			UINT uID;
			g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
			if(m_vMemberList[nSelMsg].m_dwNameID == uID)
			{
				UIMessageBox("Ng­¬i kh«ng thÓ bæ nhiÖm chÝnh m×nh");
				break;
			}
			if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
			{
				UIMessageBox("Ng­¬i kh«ng cã quyÒn bæ nhiÖm Bang chñ.");
				break;
			}
			KUiTongAssignBox::OpenWindow(m_vMemberList[nSelMsg].m_nFigure, m_vMemberList[nSelMsg].m_szName, (KWndWindow*)this, TONG_ACTION_ASSIGN);
		}
		else if(uParam == (unsigned int)&m_BtnDemise)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			UINT uID;
			g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
			if(m_vMemberList[nSelMsg].m_dwNameID == uID
			|| m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
			{
				UIMessageBox("Ng­¬i kh«ng thÓ chuyÓn vÞ cho chÝnh b¶n th©n m×nh");
				break;
			}
			char szBuff[128];
			sprintf(szBuff,
				"X¸c nhËn muèn chuyÓn ng«i bang chñ l¹i cho %s?",
				m_vMemberList[nSelMsg].m_szName);
			UIMessageBox(szBuff,
				this, "X¸c nhËn", "Hñy bá", TONG_ACTION_DEMISE);
		}
		else if(uParam == (unsigned int)&m_BtnChangeTitle)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure == enumTONG_FIGURE_MEMBER)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_CHANGETITLE))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			UINT uID;
			g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
			if(m_vMemberList[nSelMsg].m_dwNameID == uID)
			{
				UIMessageBox("Ng­¬i kh«ng thÓ ®æi danh hiÖu chÝnh m×nh");
				break;
			}
			if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
			{
				UIMessageBox("Kh«ng ai cã quyÒn ®æi danh hiÖu Bang chñ.");
				break;
			}
			if(nFigure <= (int)m_vMemberList[nSelMsg].m_nFigure)
			{
				UIMessageBox("Chøc vô cña ng­¬i kh«ng ®ñ thÈm quyÒn ®Ó ®æi tªn ng­êi ®· chän.");
				break;
			}
			KUiGetString::OpenWindow(m_vMemberList[nSelMsg].m_szName, "§æi danh hiÖu", (KWndWindow*)this, TONG_ACTION_CHANGETITLE, 1, 15, true);
		}
		else if(uParam == (unsigned int)&m_BtnChangeMaleTitle)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_CHANGETITLE))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiGetString::OpenWindow("§æi tªn nam", "", (KWndWindow*)this, TONG_ACTION_CHANGETITLE_MALE, 1, 15);
		}
		else if(uParam == (unsigned int)&m_BtnChangeFemaleTitle)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_CHANGETITLE))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiGetString::OpenWindow("§æi tªn n÷", "", (KWndWindow*)this, TONG_ACTION_CHANGETITLE_FEMALE, 1, 15);
		}
		else if(uParam == (unsigned int)&m_BtnChangeCamp)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_CHANGECAMP))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiTongCreateSheet::OpenWindow(true);
		}
		else if(uParam == (unsigned int)&m_BtnLeaveTong)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure == enumTONG_FIGURE_MASTER || nFigure == enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox("Bang chñ hoÆc Tr­ëng l·o kh«ng ®­îc quyÒn rêi bá bang héi");
				break;
			}
			char szBuff[256];
			sprintf(szBuff, "Rêi bang cÇn n¹p %d v¹n l­îng, ®ång thêi lÞch sö cèng hiÕn trong bang sÏ bÞ xãa bá, cã ®ång ý hay kh«ng?",
				defTONG_LEAVE_MONEY);
			UIMessageBox(szBuff,
				this, "X¸c nhËn", "Hñy bá", TONG_ACTION_LEAVE);
		}
		else if(uParam == (unsigned int)&m_BtnRecruit)
		{
			m_BtnRecruit.CheckButton(!m_BtnRecruit.IsButtonChecked()); //giu nguyen status
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_RECRUIT))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			if(m_BtnRecruit.IsButtonChecked())
				UIMessageBox("Ng­¬i muèn ®ãng chøc n¨ng kÕt n¹p thµnh viªn míi?",
				this, "X¸c nhËn", "Hñy bá", TONG_ACTION_RECRUIT);
			else
				UIMessageBox("Ng­¬i muèn më l¹i chøc n¨ng kÕt n¹p thµnh viªn míi?",
				this, "X¸c nhËn", "Hñy bá", TONG_ACTION_RECRUIT);
		}
		else if(uParam == (unsigned int)&m_BtnStoreTongMoney)
		{
			KUiGetString::OpenWindow("Göi ng©n quü(v¹n)", "", (KWndWindow*)this, TONG_ACTION_CONTRIBMONEY, 1, 8);
		}
		else if(uParam == (unsigned int)&m_BtnGetTongMoney)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_FUNDMANAGER))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiGetString::OpenWindow("Rót ng©n quü(v¹n)", "", (KWndWindow*)this, TONG_ACTION_WITHDRAWMONEY, 1, 8);
		}
		else if(uParam == (unsigned int)&m_BtnStorePersonalOffer)
		{
			KUiGetString::OpenWindow("Göi cèng hiÕn", "", (KWndWindow*)this, TONG_ACTION_STOREOFFER, 1, 8);
		}
		else if(uParam == (unsigned int)&m_Btn_DispenseOffer)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_FUNDMANAGER))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			KUiGetString::OpenWindow(m_vMemberList[nSelMsg].m_szName, "Ph¸t cèng hiÕn", (KWndWindow*)this, TONG_ACTION_DISPENSEOFFER, 1, 8, true);
		}
		else if(uParam == (unsigned int)&m_BtnAssignTongMoney)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiTongDistrib::OpenWindow(0, (KWndWindow*)this, TONG_ACTION_ASSIGNMONEY);
		}
		else if(uParam == (unsigned int)&m_BtnAssignTongOffer)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiTongDistrib::OpenWindow(1, (KWndWindow*)this, TONG_ACTION_ASSIGNOFFER);
		}
		else if(uParam == (unsigned int)&m_BtnTransformMoney)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_FUNDMANAGER))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			KUiGetString::OpenWindow("ChuyÓn kiÕn thiÕt", "v¹n l­îng", (KWndWindow*)this, TONG_ACTION_TRANSMONEY, 1, 8, true);
		}
		else if(uParam == (unsigned int)&m_BtnStoreBuildFund)
		{
			KUiGetString::OpenWindow("Göi kiÕn thiÕt", "v¹n l­îng", (KWndWindow*)this, TONG_ACTION_STOREBUILDFUND, 1, 8, true);
		}
		else if(uParam == (unsigned int)&m_BtnSelectAll)
		{
			BOOL bCheck = m_BtnSelectAll.IsButtonChecked();
			m_P3_BtnDepose.CheckButton(bCheck);
			m_P3_BtnChangeCamp.CheckButton(bCheck);
			m_P3_BtnChangeTitle.CheckButton(bCheck);
			m_P3_BtnKickOut.CheckButton(bCheck);
			m_BtnRecordEvent.CheckButton(bCheck);
			m_BtnLeagueManage.CheckButton(bCheck);
			m_P3_BtnUpgradeBuildLevel.CheckButton(bCheck);
			m_P3_BtnForceToRetire.CheckButton(bCheck);
			m_BtnMapManagement.CheckButton(bCheck);
			m_BtnWorkshopManagement.CheckButton(bCheck);
			m_BtnTongClaimWar.CheckButton(bCheck);
			m_BtnFundManagement.CheckButton(bCheck);
			m_BtnWeekGoalManagement.CheckButton(bCheck);
			m_P3_BtnRecruit.CheckButton(bCheck);
		}
		else if(uParam == (unsigned int)&m_BtnDistribute)
		{
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
			{
				UIMessageBox(MSG_POPUP_NOSEL);
				break;
			}
			UINT uID;
			g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
			if(m_vMemberList[nSelMsg].m_dwNameID == uID)
			{
				UIMessageBox("Ng­¬i kh«ng thÓ ph©n quyÒn cho chÝnh b¶n th©n m×nh");
				break;
			}
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER)
			{
				UIMessageBox("ChØ cã Bang chñ míi cã quyÒn ph©n phèi quyÒn chøc n¨ng");
				break;
			}
			if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MEMBER)
			{
				UIMessageBox("Kh«ng thÓ ph©n quyÒn cho m«n ®Ö.");
				break;
			}
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				UINT uNewRight = 0;
				if(m_P3_BtnDepose.IsButtonChecked())
					uNewRight |= defRIGHT_DEPOSE;
				if(m_P3_BtnChangeCamp.IsButtonChecked())
					uNewRight |= defRIGHT_CHANGECAMP;
				if(m_P3_BtnChangeTitle.IsButtonChecked())
					uNewRight |= defRIGHT_CHANGETITLE;
				if(m_P3_BtnKickOut.IsButtonChecked())
					uNewRight |= defRIGHT_KICKOUT;
				if(m_BtnRecordEvent.IsButtonChecked())
					uNewRight |= defRIGHT_RECORD;
				if(m_BtnLeagueManage.IsButtonChecked())
					uNewRight |= defRIGHT_LEAGUE;
				if(m_P3_BtnUpgradeBuildLevel.IsButtonChecked())
					uNewRight |= defRIGHT_BUILDLEVEL;
				if(m_P3_BtnForceToRetire.IsButtonChecked())
					uNewRight |= defRIGHT_FORCERETIRE;
				if(m_BtnMapManagement.IsButtonChecked())
					uNewRight |= defRIGHT_TONGMAP;
				if(m_BtnWorkshopManagement.IsButtonChecked())
					uNewRight |= defRIGHT_WORKSHOP;
				if(m_BtnTongClaimWar.IsButtonChecked())
					uNewRight |= defRIGHT_CLAIMWAR;
				if(m_BtnFundManagement.IsButtonChecked())
					uNewRight |= defRIGHT_FUNDMANAGER;
				if(m_BtnWeekGoalManagement.IsButtonChecked())
					uNewRight |= defRIGHT_WEEKGOAL;
				if(m_P3_BtnRecruit.IsButtonChecked())
					uNewRight |= defRIGHT_RECRUIT;
				KTongOperationParam Param;
				Param.nData[0] = (int)m_vMemberList[nSelMsg].m_dwNameID;
				Param.nData[1] = (int)uNewRight;
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_RIGHT);
			}
		}
		else if(uParam == (unsigned int)&m_BtnApply)
		{
			if(!m_uMeTongID && m_szDestName[0] && m_uDestTongID)
			{
				KTongOperationParam Param;
				Param.nData[0] = (int)g_FileName2Id(m_szDestName);
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_APPLY);
			}
		}
		else if(uParam == (unsigned int)&m_TabBtnBaseInfo)
		{
			if(m_uMeTongID) //tro lai thong tin bang chinh minh
			{
				m_szDestName[0] = 0;
				m_uDestTongID = 0;
			}
			ShowTab(0);
		}
		else if(uParam == (unsigned int)&m_TabBtnRecruit)
		{
			ShowTab(1);
		}
		else if(uParam == (unsigned int)&m_TabBtnFunUse)
		{
			m_szDestName[0] = 0;
			m_uDestTongID = 0;
			ShowTab(2);
		}
		else if(uParam == (unsigned int)&m_TabBtnRight)
		{
			m_szDestName[0] = 0;
			m_uDestTongID = 0;
			ShowTab(3);
		}
		else if(uParam == (unsigned int)&m_TabBtnWorkShop)
		{
			m_szDestName[0] = 0;
			m_uDestTongID = 0;
			ShowTab(4);
		}
		else if(uParam == (unsigned int)&m_TabBtnTongRec)
		{
			m_szDestName[0] = 0;
			m_uDestTongID = 0;
			ShowTab(5);
		}
		else if(uParam == (unsigned int)&m_P0_TitleTongName)
		{
			m_P0_TxtHelp.SetText(m_szHelp[0]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleMaster)
		{
			m_P0_TxtHelp.SetText(m_szHelp[1]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleLeague)
		{
			m_P0_TxtHelp.SetText(m_szHelp[2]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleCamp)
		{
			m_P0_TxtHelp.SetText(m_szHelp[3]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleTongLevel)
		{
			m_P0_TxtHelp.SetText(m_szHelp[4]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleMemberNum)
		{
			m_P0_TxtHelp.SetText(m_szHelp[5]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleBuildLevel)
		{
			m_P0_TxtHelp.SetText(m_szHelp[6]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleTongCapital)
		{
			m_P0_TxtHelp.SetText(m_szHelp[7]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleBuildFund)
		{
			m_P0_TxtHelp.SetText(m_szHelp[8]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleTotalOffer)
		{
			m_P0_TxtHelp.SetText(m_szHelp[9]);
		}
		else if(uParam == (unsigned int)&m_P0_TitlePersonalOffer)
		{
			m_P0_TxtHelp.SetText(m_szHelp[10]);
		}
		else if(uParam == (unsigned int)&m_P0_TitleWeeklyOffer)
		{
			m_P0_TxtHelp.SetText(m_szHelp[11]);
		}
		else if(uParam == (unsigned int)&m_BtnSubPage[0])
		{
			if(m_nCurFunUseTab != 0)
			{
				m_nCurFunUseTab = 0;
				m_TxtTitle.SetText("Thµnh viªn");
				m_TxtType.SetText("Chøc vô");
				m_BtnSubPage[0].CheckButton(1);
				m_BtnSubPage[1].CheckButton(0);
				m_BtnSubPage[2].CheckButton(0);
				m_BtnDemise.Show();
				m_BtnForceToRetire.Show();
				m_BtnKickOut.Show();
				m_BtnDepose.Show();
				m_Btn_DispenseOffer.Show();
				m_BtnChangeMaleTitle.Show();
				m_BtnChangeFemaleTitle.Show();
				m_BtnChangeTitle.Show();
				m_BtnRecruit.Show();
				m_BtnChangeCamp.Hide();
				m_BtnCreateTongMap.Hide();
				m_BtnConfigureTongMap.Hide();
				m_BtnTongChallenge.Hide();
				m_BtnCreateUnion.Hide();
				m_BtnApplyJionUnion.Hide();
				m_BtnLeaveUnion.Hide();
				m_BtnKickUnionTong.Hide();
				m_BtnAcceptUnionReq.Hide();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = m_uMeTongID;
					Player.nIndex = m_nCurMemPage; //page
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_MEMBERPAGE);
				}
			}
			else
				m_BtnSubPage[0].CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnSubPage[1])
		{
			if(m_nCurFunUseTab != 1)
			{
				m_nCurFunUseTab = 1;
				m_TxtTitle.SetText("Bang héi");
				m_TxtType.SetText("§¼ng cÊp");
				m_BtnOnlinePriority.CheckButton(0);
				m_BtnSubPage[0].CheckButton(0);
				m_BtnSubPage[1].CheckButton(1);
				m_BtnSubPage[2].CheckButton(0);
				m_BtnDemise.Hide();
				m_BtnForceToRetire.Hide();
				m_BtnKickOut.Hide();
				m_BtnDepose.Hide();
				m_Btn_DispenseOffer.Hide();
				m_BtnChangeMaleTitle.Hide();
				m_BtnChangeFemaleTitle.Hide();
				m_BtnChangeTitle.Hide();
				m_BtnRecruit.Hide();
				m_BtnChangeCamp.Show();
				m_BtnCreateTongMap.Show();
				m_BtnConfigureTongMap.Show();
				m_BtnTongChallenge.Show();
				m_BtnCreateUnion.Hide();
				m_BtnApplyJionUnion.Hide();
				m_BtnLeaveUnion.Hide();
				m_BtnKickUnionTong.Hide();
				m_BtnAcceptUnionReq.Hide();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurTongPage; //page
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_TONGPAGE);
				}
			}
			else
				m_BtnSubPage[1].CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnSubPage[2])
		{
			if(m_nCurFunUseTab != 2)
			{
				m_nCurFunUseTab = 2;
				m_TxtTitle.SetText("Liªn minh");
				m_TxtType.SetText("Sè bang");
				m_BtnOnlinePriority.CheckButton(0);
				m_BtnSubPage[0].CheckButton(0);
				m_BtnSubPage[1].CheckButton(0);
				m_BtnSubPage[2].CheckButton(1);
				m_BtnDemise.Hide();
				m_BtnForceToRetire.Hide();
				m_BtnKickOut.Hide();
				m_BtnDepose.Hide();
				m_Btn_DispenseOffer.Hide();
				m_BtnChangeMaleTitle.Hide();
				m_BtnChangeFemaleTitle.Hide();
				m_BtnChangeTitle.Hide();
				m_BtnRecruit.Hide();
				m_BtnChangeCamp.Hide();
				m_BtnCreateTongMap.Hide();
				m_BtnConfigureTongMap.Hide();
				m_BtnTongChallenge.Hide();
				m_BtnCreateUnion.Show();
				m_BtnApplyJionUnion.Show();
				m_BtnLeaveUnion.Show();
				m_BtnKickUnionTong.Show();
				m_BtnAcceptUnionReq.Show();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurUnionPage; //page
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_UNIONPAGE);
				}
			}
			else
				m_BtnSubPage[2].CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnWeekDaily)
		{
			if(m_nCurRecordTab != 0)
			{
				m_nCurRecordTab = 0;
				m_BtnWeekDaily.CheckButton(1);
				m_BtnAnnounce.CheckButton(0);
				m_BtnTongAffair.CheckButton(0);
				m_BtnTongHistory.CheckButton(0);
				m_RecordList.GetMessageListBox()->Clear();
				m_RecordList.Show();
				m_AnnounceEditor.Hide();
				m_BtnLeaveWord.Hide();
				m_BtnEditAnnounce.Hide();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurRecordTab; //id page nhat ky
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
				}
			}
			else
				m_BtnWeekDaily.CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnAnnounce)
		{
			if(m_nCurRecordTab != 1)
			{
				m_nCurRecordTab = 1;
				m_BtnWeekDaily.CheckButton(0);
				m_BtnAnnounce.CheckButton(1);
				m_BtnTongAffair.CheckButton(0);
				m_BtnTongHistory.CheckButton(0);
				m_RecordList.GetMessageListBox()->Clear();
				m_RecordList.Show();
				m_AnnounceEditor.Hide();
				m_BtnLeaveWord.Show();
				m_BtnEditAnnounce.Show();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurRecordTab; //id page nhat ky
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
				}
			}
			else
				m_BtnAnnounce.CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnTongAffair)
		{
			if(m_nCurRecordTab != 2)
			{
				m_nCurRecordTab = 2;
				m_BtnWeekDaily.CheckButton(0);
				m_BtnAnnounce.CheckButton(0);
				m_BtnTongAffair.CheckButton(1);
				m_BtnTongHistory.CheckButton(0);
				m_RecordList.GetMessageListBox()->Clear();
				m_RecordList.Show();
				m_AnnounceEditor.Hide();
				m_BtnLeaveWord.Hide();
				m_BtnEditAnnounce.Hide();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurRecordTab; //id page nhat ky
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
				}
			}
			else
				m_BtnTongAffair.CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnTongHistory)
		{
			if(m_nCurRecordTab != 3)
			{
				m_nCurRecordTab = 3;
				m_BtnWeekDaily.CheckButton(0);
				m_BtnAnnounce.CheckButton(0);
				m_BtnTongAffair.CheckButton(0);
				m_BtnTongHistory.CheckButton(1);
				m_RecordList.GetMessageListBox()->Clear();
				m_RecordList.Show();
				m_AnnounceEditor.Hide();
				m_BtnLeaveWord.Hide();
				m_BtnEditAnnounce.Hide();
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KUiPlayerItem Player;
					memset(&Player, 0, sizeof(KUiPlayerItem));
					Player.uId = 0;
					Player.nIndex = m_nCurRecordTab; //id page nhat ky
					g_pCoreShell->TongOperation(GTOI_REQUEST_PLAYER_TONG, (unsigned int)&Player, enumTONG_APPLY_INFO_RECORD);
				}
			}
			else
				m_BtnTongHistory.CheckButton(1);
		}
		else if(uParam == (unsigned int)&m_BtnEditAnnounce)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_RECORD))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			m_RecordList.Hide();
			m_AnnounceEditor.Show();
			Wnd_SetFocusWnd(&m_AnnounceEditor);
		}
		else if(uParam == (unsigned int)&m_BtnLeaveWord)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_RECORD))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				m_AnnounceEditor.GetText(m_szAnnounce, sizeof(m_szAnnounce), false);
				char Buffer[152];
				int nMsgLength = KUiFaceSelector::ConvertFaceText(Buffer, m_szAnnounce, strlen(m_szAnnounce));
				nMsgLength = TEncodeText(Buffer, nMsgLength);
				m_RecordList.GetMessageListBox()->Clear();
				m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&m_szAnnounce, TONG_ACTION_ANNOUNCE);
			}
		}
		else if(uParam == (unsigned int)&m_BtnUpgradeBuildLevel)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_BUILDLEVEL))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UIMessageBox("N©ng cÊp ®¼ng cÊp kiÕn thiÕt, cÇn ph¶i tiªu hao ng©n s¸ch kiÕn thiÕt, ng­¬i ch¾c ch¾n chø?",
				this, "N©ng cÊp", "Hñy bá", TONG_ACTION_UPBUILDLEVEL);
		}
		else if(uParam == (unsigned int)&m_BtnEnterMap)
		{
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_ENTERMAP);
			}
		}
		else if(uParam == (unsigned int)&m_BtnCreateTongMap)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_TONGMAP))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_CREATEMAP);
			}
		}
		else if(uParam == (unsigned int)&m_BtnConfigureTongMap)
		{
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_TONGMAP))
			{
				UIMessageBox(MSG_POPUP_NORIGHT);
				break;
			}
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_CONFIGMAP);
			}
		}
		break;
	case WND_M_KILL_FOCUS:
		if ((KWndWindow*)uParam == (KWndWindow*)&m_AnnounceEditor)
		{
			m_RecordList.Show();
			m_bHideAnnounce = true;
		}
		break;
	case WND_N_LIST_ITEM_SEL:
	{
		if(uParam == (unsigned int)&m_MemList)
		{
			if(m_nCurTab == 3)
			{
				if(nParam >= 0 && nParam < (int)m_vMemberList.size())
				{
					m_P3_BtnDepose.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_DEPOSE);
					m_P3_BtnChangeCamp.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_CHANGECAMP);
					m_P3_BtnChangeTitle.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_CHANGETITLE);
					m_P3_BtnKickOut.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_KICKOUT);
					m_BtnRecordEvent.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_RECORD);
					m_BtnLeagueManage.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_LEAGUE);
					m_P3_BtnUpgradeBuildLevel.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_BUILDLEVEL);
					m_P3_BtnForceToRetire.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_FORCERETIRE);
					m_BtnMapManagement.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_TONGMAP);
					m_BtnWorkshopManagement.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_WORKSHOP);
					m_BtnTongClaimWar.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_CLAIMWAR);
					m_BtnFundManagement.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_FUNDMANAGER);
					m_BtnWeekGoalManagement.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_WEEKGOAL);
					m_P3_BtnRecruit.CheckButton(m_vMemberList[nParam].m_uRight & defRIGHT_RECRUIT);
				}
			}
		}
		break;
	}
	case WND_N_LIST_ITEM_HIGHLIGHT:
	{
		if(uParam == (unsigned int)&m_MemList)
		{
			if(nParam < 0)
			{
				g_MouseOver.CancelMouseHoverInfo();
			}
			else if(m_uMeTongID && (m_nCurTab != 0 || !m_uDestTongID))
			{
				if(m_nCurTab == 2 && m_BtnSubPage[1].IsButtonChecked())
				{
					
				}
				else if(m_nCurTab == 2 && m_BtnSubPage[2].IsButtonChecked())
				{
					
				}
				else if(!g_MouseOver.IsMoseHoverWndObj((void*)(KWndWindow*)&m_MemList, nParam)
				&& nParam < (int)m_vMemberList.size())
				{
					int x,yy;
					Wnd_GetCursorPos(&x, &yy);
					g_MouseOver.SetMouseHoverInfo((void*)(KWndWindow*)&m_MemList, nParam,
						x, yy, false, false);
					char Buff[536];
					char szDate[128];
					UINT y, mt, d, h, m;
					g_Minute2Date(m_vMemberList[nParam].m_uOnlineDate, y, mt, d, h, m);
					if(m_vMemberList[nParam].m_bRetired)
						strcpy(szDate, "§· tho¸i Èn");
					else
						sprintf(szDate, "<color=Orange>%02u-%02u-%04u %02u:%02u<color>", d,mt,y,h,m);
					g_Minute2Date(m_vMemberList[nParam].m_uJoinDate, y, mt, d, h, m);
					UINT uNowDate = g_GetCurDateMin();
					if(uNowDate < m_vMemberList[nParam].m_uJoinDate)
						uNowDate = m_vMemberList[nParam].m_uJoinDate;
					float fAvf = m_vMemberList[nParam].m_nTotalOffer/(float)((uNowDate - m_vMemberList[nParam].m_uJoinDate)/1440 + 1);
					sprintf(Buff,
						"<color=Pink>%s<color>\n"
						"Danh hiÖu: <color=earth>%s<color>\n"
						"Ngµy gia nhËp: <color=Orange>%02u-%02u-%04u %02u:%02u<color>\n"
						"Ho¹t ®éng gÇn ®©y: %s\n"
						"Cèng hiÕn tuÇn: <color=Orange>%d<color>\n"
						"§iÓm cèng hiÕn trung b×nh hµng ngµy: <color=Orange>%.2f<color>\n"
						"Hoµn thµnh môc tiªu tuÇn: <color=Orange>%d<color>"
						, m_vMemberList[nParam].m_szName, m_vMemberList[nParam].m_szTitle,
						d,mt,y,h,m, szDate,
						m_vMemberList[nParam].m_nWeekOffer, fAvf,
						m_vMemberList[nParam].m_nNewWGCompleted);
					int nLen = TEncodeText(Buff, strlen(Buff));
					g_MouseOver.SetMouseHoverTitle(Buff, nLen, 0xff00f000);
				}
			}
		}
		break;	
	}
	case WND_M_MENUITEM_SELECTED:
	{
		if (uParam == (unsigned int)(KWndWindow*)this)
		{
			int nSel = (int)(short(LOWORD(nParam)));
			//if (HIWORD(nParam) == 0)
			if(nSel == SORTMN_AVERAGEOF)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				UINT uNowDate = g_GetCurDateMin();
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					UINT uDate = uNowDate;
					if(uDate < it->m_uJoinDate)
						uDate = it->m_uJoinDate;
					float fAvf = it->m_nTotalOffer/(float)((uDate - it->m_uJoinDate)/1440 + 1);
					int n;
					for(n = 0; n < (int)vSort.size();++n)
					{
						uDate = uNowDate;
						if(uDate < vSort[n].m_uJoinDate)
							uDate = vSort[n].m_uJoinDate;
						float fACh = vSort[n].m_nTotalOffer/(float)((uDate - vSort[n].m_uJoinDate)/1440 + 1);
						if(fAvf > fACh)
							break;
					}
					vSort.insert(vSort.begin() + n, *it);
					it = m_vMemberList.erase(it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
			else if(nSel == SORTMN_WEEKOFFER)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					int n;
					for(n = 0; n < (int)vSort.size();++n)
					{
						if(it->m_nWeekOffer > vSort[n].m_nWeekOffer)
							break;
					}
					vSort.insert(vSort.begin() + n, *it);
					it = m_vMemberList.erase(it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
			else if(nSel == SORTMN_WGCOMPLETED)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					int n;
					for(n = 0; n < (int)vSort.size();++n)
					{
						if(it->m_nNewWGCompleted > vSort[n].m_nNewWGCompleted)
							break;
					}
					vSort.insert(vSort.begin() + n, *it);
					it = m_vMemberList.erase(it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
			else if(nSel == SORTMN_RETIRED)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					if(it->m_bRetired)
					{
						vSort.push_back(*it);
						it = m_vMemberList.erase(it);
					}
					else
					{
						++it;
					}
				}
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();++it)
				{
					vSort.push_back(*it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
			else if(nSel == SORTMN_FIGURE)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					if(it->m_nFigure == enumTONG_FIGURE_MASTER)
					{
						vSort.push_back(*it);
						it = m_vMemberList.erase(it);
						break;
					}
					else
					{
						++it;
					}
				}
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					if(it->m_nFigure == enumTONG_FIGURE_DIRECTOR)
					{
						vSort.push_back(*it);
						it = m_vMemberList.erase(it);
					}
					else
					{
						++it;
					}
				}
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					if(it->m_nFigure == enumTONG_FIGURE_MANAGER)
					{
						vSort.push_back(*it);
						it = m_vMemberList.erase(it);
					}
					else
					{
						++it;
					}
				}
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();++it)
				{
					vSort.push_back(*it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
			else if(nSel == SORTMN_LASTTIME)
			{
				vector<STONG_MEMSUBINFO> vSort;
				vector<STONG_MEMSUBINFO>::iterator it;
				for(it = m_vMemberList.begin(); it != m_vMemberList.end();)
				{
					int n;
					for(n = 0; n < (int)vSort.size();++n)
					{
						if(it->m_uOnlineDate > vSort[n].m_uOnlineDate)
							break;
					}
					vSort.insert(vSort.begin() + n, *it);
					it = m_vMemberList.erase(it);
				}
				m_vMemberList.clear();
				m_MemList.Clear();
				for(int i = 0; i < (int)vSort.size();++i)
				{
					AddMemberList(i, vSort[i]);
				}
				vSort.clear();
			}
		}
		break;
	}
	case WND_M_OTHER_WORK_RESULT:
	{
		if(uParam == TONG_ACTION_DISMISS)
		{
			if(nParam == 0) //OK
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_KICKOUT))
					break;
				int nSelMsg = m_MemList.GetCurSel();
				if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
					break;
				if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER
				|| m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_DIRECTOR)
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, m_vMemberList[nSelMsg].m_dwNameID, TONG_ACTION_DISMISS);
				}
			}
		}
		else if(uParam == TONG_ACTION_ASSIGN)
		{
			if(nParam != enumTONG_FIGURE_MEMBER && nParam != enumTONG_FIGURE_MANAGER
			&& nParam != enumTONG_FIGURE_DIRECTOR)
				break;
			int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
			if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
				break;
			UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
			if(!(uRight & defRIGHT_DEPOSE))
				break;
			int nSelMsg = m_MemList.GetCurSel();
			if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
				break;
			UINT uID;
			g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
			if(m_vMemberList[nSelMsg].m_dwNameID == uID)
				break;
			if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
				break;
			if(m_vMemberList[nSelMsg].m_nFigure == nParam)
				break;
			if(nFigure == enumTONG_FIGURE_DIRECTOR && (nParam == enumTONG_FIGURE_DIRECTOR
				|| m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_DIRECTOR))
			{
				UIMessageBox("Tr­ëng l·o kh«ng thÓ nhiÖm - miÔn chøc vÞ cho Tr­ëng l·o");
				break;
			}
			if(m_nNextTimeMemPage < nCurTime)
			{
				m_nNextTimeMemPage = nCurTime + 4;
				KTongOperationParam Param;
				Param.nData[0] = (int)m_vMemberList[nSelMsg].m_dwNameID;
				Param.nData[1] = nParam;
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_ASSIGN);
			}
		}
		else if(uParam == TONG_ACTION_DEMISE)
		{
			if(nParam == 0) //OK
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER)
					break;
				int nSelMsg = m_MemList.GetCurSel();
				if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
					break;
				UINT uID;
				g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
				if(m_vMemberList[nSelMsg].m_dwNameID == uID
				|| m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, m_vMemberList[nSelMsg].m_dwNameID, TONG_ACTION_DEMISE);
				}
			}
		}
		else if(uParam == TONG_ACTION_LEAVE)
		{
			if(nParam == 0) //OK
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure == enumTONG_FIGURE_MASTER || nFigure == enumTONG_FIGURE_DIRECTOR)
					break;
				int nMoney = g_pCoreShell->GetGameData(GDI_PLAYER_HOLD_MONEY, 0, 0)/10000;
				if(nMoney < defTONG_LEAVE_MONEY)
				{
					KSystemMessage	sMsg;
					sMsg.eType = SMT_NORMAL;
					sMsg.byConfirmType = SMCT_NONE;
					sMsg.byPriority = 0;
					sMsg.byParamSize = 0;
					sprintf(sMsg.szMessage, "Rêi bang cÇn n¹p %d v¹n l­îng, ng­¬i kh«ng mang ®ñ tiÒn.", defTONG_LEAVE_MONEY);
					KUiSysMsgCentre::AMessageArrival(&sMsg, 0);
					break;
				}
				g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_LEAVE);
			}
		}
		else if(uParam == TONG_ACTION_RECRUIT)
		{
			if(nParam == 0) //OK
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_RECRUIT))
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_RECRUIT);
				}
			}
		}
		else if(uParam == TONG_ACTION_CONTRIBMONEY)
		{
			if(nParam)
			{
				char* szMoney = (char*)nParam;
				int nSendMoney = atoi(szMoney);
				if(nSendMoney <= 0)
				{
					UIMessageBox("Ng­¬i nhËp sè tiÒn göi kh«ng hîp lÖ.");
					break;
				}
				int nMoney = g_pCoreShell->GetGameData(GDI_PLAYER_HOLD_MONEY, 0, 0)/10000;
				if(nSendMoney > nMoney)
				{
					UIMessageBox("Ng­¬i kh«ng cã ®ñ sè tiÒn ®Ó göi vµo ng©n quü.");
					break;
				}
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, nSendMoney, TONG_ACTION_CONTRIBMONEY);
				}
			}
		}
		else if(uParam == TONG_ACTION_WITHDRAWMONEY)
		{
			if(nParam)
			{
				char* szMoney = (char*)nParam;
				int nMoney = atoi(szMoney);
				if(nMoney <= 0)
				{
					UIMessageBox("Ng­¬i nhËp sè tiÒn rót kh«ng hîp lÖ.");
					break;
				}
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, nMoney, TONG_ACTION_WITHDRAWMONEY);
				}
			}
		}
		else if(uParam == TONG_ACTION_STOREOFFER)
		{
			if(nParam)
			{
				char* szMoney = (char*)nParam;
				int nMoney = atoi(szMoney);
				if(nMoney <= 0)
				{
					UIMessageBox("Ng­¬i nhËp sè ®iÓm cèng hiÕn kh«ng hîp lÖ.");
					break;
				}
				int nCurMoney = g_pCoreShell->TongOperation(GTOI_GET_PERSONALOFFER, 0, 0);
				if(nMoney > nCurMoney)
				{
					UIMessageBox("§iÓm cèng hiÕn c¸ nh©n kh«ng ®ñ.");
					break;
				}
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, nMoney, TONG_ACTION_STOREOFFER);
				}
			}
		}
		else if(uParam == TONG_ACTION_DISPENSEOFFER)
		{
			if(nParam)
			{
				char* szMoney = (char*)nParam;
				int nMoney = atoi(szMoney);
				if(nMoney <= 0 || nMoney > defTONG_MAX_OFFER_DAYLIMIT)
				{
					char szBuff[128];
					sprintf(szBuff, "Sè ®iÓm cèng hiÕn cÇn ph¸t ng­¬i nhËp vµo kh«ng hîp lÖ. Tèi ®a %d ®iÓm.",
						defTONG_MAX_OFFER_DAYLIMIT); 
					UIMessageBox(szBuff);
					break;
				}
				int nSelMsg = m_MemList.GetCurSel();
				if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KTongOperationParam Param;
					Param.nData[0] = (int)m_vMemberList[nSelMsg].m_dwNameID;
					Param.nData[1] = nMoney;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_DISPENSEOFFER);
				}
			}
		}
		else if(uParam == TONG_ACTION_ASSIGNMONEY)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER)
					break;
				int* pPointValue = (int*)nParam;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KTongOperationParam Param;
					Param.nData[0] = *pPointValue;
					Param.nData[1] = *(pPointValue+1);
					Param.nData[2] = *(pPointValue+2);
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_ASSIGNMONEY);
				}
			}
		}
		else if(uParam == TONG_ACTION_ASSIGNOFFER)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER)
					break;
				int* pPointValue = (int*)nParam;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					KTongOperationParam Param;
					Param.nData[0] = *pPointValue;
					Param.nData[1] = *(pPointValue+1);
					Param.nData[2] = *(pPointValue+2);
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_ASSIGNOFFER);
				}
			}
		}
		else if(uParam == TONG_ACTION_TRANSMONEY)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_FUNDMANAGER))
					break;
				char* szMoney = (char*)nParam;
				int nMoney = atoi(szMoney);
				if(nMoney <= 0)
				{
					UIMessageBox("Sè tiÒn ng­¬i nhËp vµo kh«ng hîp lÖ ®Ó chuyÓn ng©n s¸ch kiÕn thiÕt.");
					break;
				}
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, nMoney, TONG_ACTION_TRANSMONEY);
				}
			}
		}
		else if(uParam == TONG_ACTION_STOREBUILDFUND)
		{
			if(nParam)
			{
				char* szMoney = (char*)nParam;
				int nMoney = atoi(szMoney);
				if(nMoney <= 0)
				{
					UIMessageBox("Sè tiÒn ng­¬i nhËp vµo kh«ng hîp lÖ ®Ó göi vµo ng©n s¸ch kiÕn thiÕt.");
					break;
				}
				int nCurMoney = g_pCoreShell->GetGameData(GDI_PLAYER_HOLD_MONEY, 0, 0)/10000;
				if(nMoney > nCurMoney)
				{
					UIMessageBox("Ng­¬i kh«ng cã ®ñ sè tiÒn trong hµnh trang ®Ó göi ng©n s¸ch kiÕn thiÕt");
					break;
				}
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, nMoney, TONG_ACTION_STOREBUILDFUND);
				}
			}
		}
		else if(uParam == TONG_ACTION_CHANGETITLE)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure == enumTONG_FIGURE_MEMBER)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_CHANGETITLE))
					break;
				int nSelMsg = m_MemList.GetCurSel();
				if(nSelMsg < 0 || nSelMsg >= (int)m_vMemberList.size())
					break;
				UINT uID;
				g_pCoreShell->GetGameData(GDI_GET_PLAYERNPC_INDEX, (unsigned int)&uID, 0);
				if(m_vMemberList[nSelMsg].m_dwNameID == uID)
					break;
				if(m_vMemberList[nSelMsg].m_nFigure == enumTONG_FIGURE_MASTER)
					break;
				if(nFigure <= (int)m_vMemberList[nSelMsg].m_nFigure)
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					char* szTitle = (char*)nParam;
					KTongOperationParam Param;
					Param.nData[0] = (int)m_vMemberList[nSelMsg].m_dwNameID;
					Param.nData[1] = m_vMemberList[nSelMsg].m_nFigure;
					strcpy(Param.Name, szTitle);
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_CHANGETITLE);
				}
			}
		}
		else if(uParam == TONG_ACTION_CHANGETITLE_MALE)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_CHANGETITLE))
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					char* szTitle = (char*)nParam;
					KTongOperationParam Param;
					Param.nData[0] = 0;
					strcpy(Param.Name, szTitle);
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_CHANGETITLE_MALE);
				}
			}
		}
		else if(uParam == TONG_ACTION_CHANGETITLE_FEMALE)
		{
			if(nParam)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_CHANGETITLE))
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					char* szTitle = (char*)nParam;
					KTongOperationParam Param;
					Param.nData[0] = 1;
					strcpy(Param.Name, szTitle);
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)&Param, TONG_ACTION_CHANGETITLE_MALE);
				}
			}
		}
		else if(uParam == TONG_ACTION_CHANGECAMP)
		{
			if(nParam >= camp_justice && nParam <= camp_balance)
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_CHANGECAMP))
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, (unsigned int)nParam, TONG_ACTION_CHANGECAMP);
				}
			}
		}
		else if(uParam == TONG_ACTION_UPBUILDLEVEL)
		{
			if(nParam == 0) //OK
			{
				int nFigure = g_pCoreShell->TongOperation(GTOI_GETTONG_FIGURE, 0, 0);
				if(nFigure != enumTONG_FIGURE_MASTER && nFigure != enumTONG_FIGURE_DIRECTOR)
					break;
				UINT uRight = (UINT)g_pCoreShell->TongOperation(GTOI_GETTONG_RIGHT, 0, 0);
				if(!(uRight & defRIGHT_BUILDLEVEL))
					break;
				if(m_nNextTimeMemPage < nCurTime)
				{
					m_nNextTimeMemPage = nCurTime + 4;
					g_pCoreShell->TongOperation(GTOI_TONG_ACTION, 0, TONG_ACTION_UPBUILDLEVEL);
				}
			}
		}
		break;
	}
	default:
		return KWndImage::WndProc(uMsg, uParam, nParam);
	}
	return 1;
}

void KUiTongManager::AddMemberList(int nIndex, STONG_MEMSUBINFO& sMem, bool bPush)
{
	if(bPush)
		m_vMemberList.push_back(sMem);
	char Buff[128];
	char BuffFinal[160];
	char szBuffer[32];
	int offset = 0;
	sprintf(szBuffer, "%d", nIndex+1);
	int j;
	for(j=0; j<strlen(szBuffer); ++j)
		Buff[offset++] = szBuffer[j];
	if(offset < 5)
	{
		j = 5-offset;
		while(j > 0)
		{
			Buff[offset++] = ' ';
			--j;
		}
	}
	Buff[offset] = 0;
	strcat(Buff, sMem.m_szName);
	offset += strlen(sMem.m_szName);
	if(offset < 27)
	{
		j = 27-offset;
		while(j > 0)
		{
			Buff[offset++] = ' ';
			--j;
		}
	}
	else
		offset = 27;
	Buff[offset] = 0;
	if(sMem.m_nFigure == enumTONG_FIGURE_MEMBER)
		strcat(Buff, defTITLE_MEMBER);
	else if(sMem.m_nFigure == enumTONG_FIGURE_MANAGER)
		strcat(Buff, defTITLE_MANAGER);
	else if(sMem.m_nFigure == enumTONG_FIGURE_DIRECTOR)
		strcat(Buff, defTITLE_DIRECT);
	else //if(sMem.m_nFigure == enumTONG_FIGURE_MASTER)
		strcat(Buff, defTITLE_MASTER);
	offset = 0;
	if(!sMem.btOnline)
	{
		BuffFinal[offset++] = KTC_COLOR;
		BuffFinal[offset++] = (m_OfflineColor >> 16) & 0xff;
		BuffFinal[offset++] = (m_OfflineColor >> 8) & 0xff;
		BuffFinal[offset++] = m_OfflineColor & 0xff;
	}
	memcpy(&BuffFinal[offset], &Buff[0], strlen(Buff));
	offset += strlen(Buff);
	m_MemList.AddOneMessage(BuffFinal, offset);
}

void KUiTongManager::AddTongList(int nIndex, STONG_PAGEMEM& sMem)
{
	m_vTongList.push_back(sMem);
	char Buff[128];
	char BuffFinal[160];
	char szBuffer[32];
	int offset = 0;
	sprintf(szBuffer, "%d", nIndex+1);
	int j;
	for(j=0; j<strlen(szBuffer); ++j)
		Buff[offset++] = szBuffer[j];
	if(offset < 5)
	{
		j = 5-offset;
		while(j > 0)
		{
			Buff[offset++] = ' ';
			--j;
		}
	}
	Buff[offset] = 0;
	strcat(Buff, sMem.m_szName);
	offset += strlen(sMem.m_szName);
	if(offset < 30)
	{
		j = 30-offset;
		while(j > 0)
		{
			Buff[offset++] = ' ';
			--j;
		}
	}
	else
		offset = 30;
	Buff[offset] = 0;
	sprintf(szBuffer, "%d", sMem.m_nLevel);
	strcat(Buff, szBuffer);
	offset = 0;
	memcpy(&BuffFinal[offset], &Buff[0], strlen(Buff));
	offset += strlen(Buff);
	m_MemList.AddOneMessage(BuffFinal, offset);
}

void KUiTongManager::TongInfoArrive(void * pInfo)
{
	TONG_HEAD_INFO_SYNC	*pData = (TONG_HEAD_INFO_SYNC*)pInfo;
	char szBuffer[64];
	sprintf(szBuffer, "%d", pData->m_nMoney);
	m_P0_TxtTongCapital.SetLabel(szBuffer);
	sprintf(szBuffer, "%u", pData->m_MemberNum);
	m_P0_TxtMemberNum.SetLabel(szBuffer);
	sprintf(szBuffer, "%u", pData->m_btLevel);
	m_P0_TxtTongLevel.SetLabel(szBuffer);
	m_P0_TxtTongName.SetLabel(pData->m_szTongName);
	m_P0_TxtMaster.SetLabel(pData->m_szMaster);
	m_P0_TxtLeague.SetLabel(pData->m_szUnion);
	if(pData->m_btCamp == camp_justice)
		m_P0_TxtCamp.SetLabel("ChÝnh ph¸i");
	else if(pData->m_btCamp == camp_evil)
		m_P0_TxtCamp.SetLabel("Tµ ph¸i");
	else if(pData->m_btCamp == camp_balance)
		m_P0_TxtCamp.SetLabel("Trung lËp");
	m_BtnRecruit.CheckButton(!pData->m_bLockRecruit);
	if(pData->m_bSelf)
	{
		m_P2_TxtTongName.SetLabel(pData->m_szTongName);
		m_P2_TxtTongUnion.SetLabel(pData->m_szUnion);
		sprintf(szBuffer, "%d", pData->m_nMoney);
		m_P2_TxtTongCapital.SetLabel(szBuffer);
		m_P2_TxtTongCapital2.SetLabel(szBuffer);
		sprintf(szBuffer, "%d", pData->m_nBuildMoney);
		m_P0_TxtBuildFund.SetLabel(szBuffer);
		m_P2_TxtBuildFund.SetLabel(szBuffer);
		m_P2_TxtBuildFund2.SetLabel(szBuffer);
		sprintf(szBuffer, "%d", pData->m_nBuilLevel);
		m_P0_TxtBuildLevel.SetLabel(szBuffer);
		m_P2_TxtBuildLevel.SetLabel(szBuffer);
		sprintf(szBuffer, "%d", pData->m_nOffer);
		m_P0_TxtTotalOffer.SetLabel(szBuffer);
		m_P2_TxtTotalOffer.SetLabel(szBuffer);
		int nExpFrame = (7*pData->m_btExpPercent)/100;
		m_P2_ImgExp.SetFrame(nExpFrame);
		sprintf(szBuffer, "%d%%", pData->m_btExpPercent);
		m_P2_TxtExpPercent.SetText(szBuffer);
	}
	else
	{
		m_P0_TxtBuildFund.SetLabel("");
		m_P0_TxtTotalOffer.SetLabel("");
		m_P0_TxtBuildLevel.SetLabel("");
		m_vMemberList.clear();
		m_MemList.Clear();
		STONG_MEMSUBINFO sMem;
		memset(&sMem, 0, sizeof(STONG_MEMSUBINFO));
		strcpy(sMem.m_szName, pData->m_szMaster);
		strcpy(sMem.m_szTitle, defTITLE_MASTER);
		sMem.btOnline = 1;
		AddMemberList(0, sMem);
		m_nCurMemPage = 0;
		m_EditBoxDestPage.SetIntText(m_nCurMemPage+1);
	}
}

void KUiTongManager::MemInfoArrive(void * pInfo)
{
	TONG_MEMBER_INFO_SYNC	*pData = (TONG_MEMBER_INFO_SYNC*)pInfo;
	if(pData->m_bBegin)
	{
		m_vMemberList.clear();
		m_MemList.Clear();
		m_nCurMemPage = pData->m_nPage;
		m_EditBoxDestPage.SetIntText(m_nCurMemPage+1);
		m_BtnOnlinePriority.CheckButton(0);
		m_BtnSelectAll.CheckButton(0);
		m_P3_BtnDepose.CheckButton(0);
		m_P3_BtnChangeCamp.CheckButton(0);
		m_P3_BtnChangeTitle.CheckButton(0);
		m_P3_BtnKickOut.CheckButton(0);
		m_BtnRecordEvent.CheckButton(0);
		m_BtnLeagueManage.CheckButton(0);
		m_P3_BtnUpgradeBuildLevel.CheckButton(0);
		m_P3_BtnForceToRetire.CheckButton(0);
		m_BtnMapManagement.CheckButton(0);
		m_BtnWorkshopManagement.CheckButton(0);
		m_BtnTongClaimWar.CheckButton(0);
		m_BtnFundManagement.CheckButton(0);
		m_BtnWeekGoalManagement.CheckButton(0);
		m_P3_BtnRecruit.CheckButton(0);
	}
	int i = m_vMemberList.size() + pData->m_nPage*defTONG_ONE_PAGE_MAX_NUM;
	for(int n=0;n < pData->m_nMemNum; ++n)
	{
		STONG_MEMSUBINFO& sMem = pData->sMem[n];
		AddMemberList(i, sMem);
		++i;
	}
}

void KUiTongManager::TongPageArrive(void * pInfo)
{
	TONG_PAGE_INFO_SYNC	*pData = (TONG_PAGE_INFO_SYNC*)pInfo;
	if(pData->m_bBegin)
	{
		m_vTongList.clear();
		m_MemList.Clear();
		m_nCurTongPage = pData->m_nPage;
		m_EditBoxDestPage.SetIntText(m_nCurTongPage+1);
		m_BtnOnlinePriority.CheckButton(0);
	}
	int i = m_vTongList.size() + pData->m_nPage*defTONG_ONE_PAGE_MAX_NUM;
	for(int n=0;n < pData->m_nMemNum; ++n)
	{
		STONG_PAGEMEM& sMem = pData->sMem[n];
		AddTongList(i, sMem);
		++i;
	}
}

void KUiTongManager::RecordListArrive(void * pInfo)
{
	TONG_RECORD_AFFAIRHISTORY_SYNC	*pData = (TONG_RECORD_AFFAIRHISTORY_SYNC*)pInfo;
	for(int i=0;i<pData->m_nMsgCount;++i)
	{
		m_RecordList.GetMessageListBox()->AddOneMessage(pData->m_szMsg[i], strlen(pData->m_szMsg[i]));
	}
	m_RecordList.GetScrollBar()->SetScrollPos(m_RecordList.GetScrollBar()->GetMaxValue());
}

void KUiTongManager::RecordAnnounceArrive(void * pInfo)
{
	TONG_RECORD_ANNOUNCE_SYNC	*pData = (TONG_RECORD_ANNOUNCE_SYNC*)pInfo;
	m_RecordList.GetMessageListBox()->Clear();
	strcpy(m_szAnnounce, pData->m_szAnnounce);
	m_AnnounceEditor.SetText(m_szAnnounce);
	char Buffer[152];
	int nMsgLength = KUiFaceSelector::ConvertFaceText(Buffer, m_szAnnounce, strlen(m_szAnnounce));
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
}

const char g_TTaskName[][64] =
{
	"NhiÖm vô kh«ng x¸c ®Þnh",
	"ChiÕn tr­êng Tèng Kim",
	"NhiÖm vô TÝn Sø",
	"Th¸ch thøc thêi gian",
	"Chuçi nhiÖm vô D· TÈu",
};

void KUiTongManager::RecordWeekTaskArrive(void * pInfo)
{
	TONG_RECORD_WEEKTASK_SYNC	*pData = (TONG_RECORD_WEEKTASK_SYNC*)pInfo;
	m_RecordList.GetMessageListBox()->Clear();
	char Buffer[512];
	char szBuffT[32];
	char szBuffM[32];
	int nMsgLength = sprintf(Buffer, "Bang héi ®ang b­íc vµo tuÇn thø <color=metal>%u<color>, ngµy thø <color=earth>%d", pData->m_uWeekCount, pData->m_DayNum);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	nMsgLength = sprintf(Buffer, "ThiÕt lËp møc ®é khã môc tiªu tuÇn hiÖn t¹i: cÊp <color=green>%u", pData->m_nTaskLevel);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	if(pData->m_bOrdeal)
	{
		nMsgLength = sprintf(Buffer, "Bang héi ®ang trong thêi gian thö th¸ch, ®Ó v­ît qua cÇn ®iÒu kiÖn: Ng©n quü %d v¹n, Nh©n sè %d ng­êi",
						defTONG_CONDITION_MINMONEY, defTONG_CONDITION_MINMEM);
		nMsgLength = TEncodeText(Buffer, nMsgLength);
		m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	}
	m_RecordList.GetMessageListBox()->AddOneMessage(" ", 1);
	
	nMsgLength = sprintf(Buffer, "Môc tiªu tuÇn nµy:--------------------<color=green>%s<color>--------------------",
					g_TTaskName[pData->m_NewWeekGoal.wWeekGoalType]);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " Môc tiªu bang héi: <color=metal>%d<color>    Bang héi ®· hoµn thµnh: <color=fire>%d",
					pData->m_NewWeekGoal.nTWeekGoal, pData->m_NewWeekGoal.nTCompletedWG);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " Môc tiªu c¸ nh©n: <color=metal>%d<color>    C¸ nh©n ®· hoµn thµnh: <color=fire>%d",
					pData->m_NewWeekGoal.nMWeekGoal, pData->m_nNewWGCompleted);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	if(!pData->m_bWGType)
	{
		nMsgLength = sprintf(Buffer, " <color=fire>B¹n ch­a ®­îc tiÕp nhËn nhiÖm vô tuÇn nµy");
		nMsgLength = TEncodeText(Buffer, nMsgLength);
		m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	}
	
	if(pData->m_NewWeekGoal.nTWeekGoal && pData->m_NewWeekGoal.nTCompletedWG >= pData->m_NewWeekGoal.nTWeekGoal)
		strcpy(szBuffT, "®· hoµn thµnh");
	else
		strcpy(szBuffT, "ch­a hoµn thµnh");
	if(pData->m_NewWeekGoal.nMWeekGoal && pData->m_nNewWGCompleted >= pData->m_NewWeekGoal.nMWeekGoal)
		strcpy(szBuffM, "®· hoµn thµnh");
	else
		strcpy(szBuffM, "ch­a hoµn thµnh");
	nMsgLength = sprintf(Buffer, " Sè ngµy cßn l¹i: <color=metal>%d<color>    Bang héi: <%s>  C¸ nh©n: <%s>",
					8-pData->m_DayNum, szBuffT, szBuffM);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " PhÇn th­ëng bang héi: <color=yellow>%d<color> ®iÓm cèng hiÕn    PhÇn th­ëng c¸ nh©n: <color=yellow>%d<color> ®iÓm cèng hiÕn",
					pData->m_NewWeekGoal.nTWeeGoalPrice, pData->m_NewWeekGoal.nMWeeGoalPrice);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	m_RecordList.GetMessageListBox()->AddOneMessage(" ", 1);
	
	nMsgLength = sprintf(Buffer, "Môc tiªu tuÇn tr­íc:--------------------<color=green>%s<color>--------------------",
					g_TTaskName[pData->m_OldWeekGoal.wWeekGoalType]);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " Môc tiªu bang héi: <color=metal>%d<color>    Bang héi ®· hoµn thµnh: <color=fire>%d",
					pData->m_OldWeekGoal.nTWeekGoal, pData->m_OldWeekGoal.nTCompletedWG);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " Môc tiªu c¸ nh©n: <color=metal>%d<color>    C¸ nh©n ®· hoµn thµnh: <color=fire>%d",
					pData->m_OldWeekGoal.nMWeekGoal, pData->m_nOldWGCompleted);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	if(pData->m_OldWeekGoal.nTWeekGoal && pData->m_OldWeekGoal.nTCompletedWG >= pData->m_OldWeekGoal.nTWeekGoal)
		strcpy(szBuffT, "®· hoµn thµnh");
	else
		strcpy(szBuffT, "ch­a hoµn thµnh");
	if(pData->m_OldWeekGoal.nMWeekGoal && pData->m_nOldWGCompleted >= pData->m_OldWeekGoal.nMWeekGoal)
		strcpy(szBuffM, "®· hoµn thµnh");
	else
		strcpy(szBuffM, "ch­a hoµn thµnh");
	nMsgLength = sprintf(Buffer, " Sè ngµy cßn l¹i: <color=metal>0<color>    Bang héi: <%s>  C¸ nh©n: <%s>",
					szBuffT, szBuffM);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);
	
	nMsgLength = sprintf(Buffer, " PhÇn th­ëng bang héi: <color=yellow>%d<color> ®iÓm cèng hiÕn    PhÇn th­ëng c¸ nh©n: <color=yellow>%d<color> ®iÓm cèng hiÕn",
					pData->m_OldWeekGoal.nTWeeGoalPrice, pData->m_OldWeekGoal.nMWeeGoalPrice);
	nMsgLength = TEncodeText(Buffer, nMsgLength);
	m_RecordList.GetMessageListBox()->AddOneMessage(Buffer, nMsgLength);

}

void KUiTongManager::ResponseResult(KUiGameObjectWithName *pResult, int nbIsSucceed)
{
    switch(pResult->nData)
    {
		case TONG_ACTION_DISMISS://khong du tien kich ra bang
		{
			int nMoney = pResult->nParam;
			UINT uDestID = pResult->uParam;
			int i;
			for(i=0;i < (int)m_vMemberList.size(); ++i)
			{
				if(m_vMemberList[i].m_dwNameID == uDestID)
				{
					char szBuff[256];
					sprintf(szBuff, "Ng©n quü kh«ng ®ñ %d v¹n l­îng ®Ó trôc xuÊt %s", nMoney, m_vMemberList[i].m_szName);
					UIMessageBox(szBuff);
					break;
				}
			}
		}
		break;
		case TONG_ACTION_ASSIGN:
		{
			char szBuff[128];
			if(pResult->nParam == enumTONG_FIGURE_DIRECTOR)
				sprintf(szBuff, "Danh s¸ch Tr­ëng l·o tèi ®a %d ng­êi, kh«ng thÓ bæ nhiÖm.", defTONG_MAX_DIRECTOR);
			else
				sprintf(szBuff, "Danh s¸ch §éi tr­ëng tèi ®a %d ng­êi, kh«ng thÓ bæ nhiÖm.", defTONG_MAX_MANAGER);
			UIMessageBox(szBuff);
		}
		break;
		case TONG_ACTION_RIGHT://phan quyen thanh cong
		{
			UINT uDestID = (UINT)pResult->nParam;
			int i;
			for(i=0;i < (int)m_vMemberList.size(); ++i)
			{
				if(m_vMemberList[i].m_dwNameID == uDestID)
				{
					m_vMemberList[i].m_uRight = pResult->uParam;
					break;
				}
			}
			if(m_nCurTab == 3 && i == m_MemList.GetCurSel())
			{
				m_P3_BtnDepose.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_DEPOSE);
				m_P3_BtnChangeCamp.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_CHANGECAMP);
				m_P3_BtnChangeTitle.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_CHANGETITLE);
				m_P3_BtnKickOut.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_KICKOUT);
				m_BtnRecordEvent.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_RECORD);
				m_BtnLeagueManage.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_LEAGUE);
				m_P3_BtnUpgradeBuildLevel.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_BUILDLEVEL);
				m_P3_BtnForceToRetire.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_FORCERETIRE);
				m_BtnMapManagement.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_TONGMAP);
				m_BtnWorkshopManagement.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_WORKSHOP);
				m_BtnTongClaimWar.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_CLAIMWAR);
				m_BtnFundManagement.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_FUNDMANAGER);
				m_BtnWeekGoalManagement.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_WEEKGOAL);
				m_P3_BtnRecruit.CheckButton(m_vMemberList[i].m_uRight & defRIGHT_RECRUIT);
				char szBuff[128];
				sprintf(szBuff, "Thµnh c«ng ph©n phèi quyÒn cho %s", m_vMemberList[i].m_szName);
				UIMessageBox(szBuff);
			}
		}
		break;
		case TONG_ACTION_RECRUIT: //thay doi tuyen dung thanh cong
		{
			m_BtnRecruit.CheckButton(!pResult->nParam);
			if(m_BtnRecruit.IsButtonChecked())
				UIMessageBox("Chøc n¨ng kÕt n¹p thµnh viªn míi ®· ®­îc më ra");
			else
				UIMessageBox("Chøc n¨ng kÕt n¹p thµnh viªn míi ®· ®ãng l¹i");
		}
		break;
		case TONG_ACTION_CONTRIBMONEY: //update so tien ngan quy~
		{
			if(pResult->nParam < 0)	//rut tien that bai
			{
				UIMessageBox("Ng©n quü bang kh«ng ®ñ sè tiÒn ng­¬i cÇn rót");
				break;
			}
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P0_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital2.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_STOREOFFER: //update diem cong hien du tru
		{
			if(pResult->nParam < 0)	//phat cong hien ca nhan that bai
			{
				UIMessageBox("§iÓm cèng hiÕn dù tr÷ bang kh«ng ®ñ ®Ó ph¸t cho thµnh viªn");
				break;
			}
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P2_TxtTotalOffer.SetLabel(szBuffer);
			m_P0_TxtTotalOffer.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_ASSIGNMONEY: //update tien hoac phat' that bai
		{
			if(pResult->nParam < 0)
			{
				UIMessageBox("Kh«ng thÓ ph¸t quü bang liªn tôc, xin ®îi mét thêi gian.");
				break;
			}
			if(!pResult->uParam)
			{
				char szMsg[128];
				sprintf(szMsg, "Vèn cña bang héi kh«ng ®ñ %d v¹n l­îng", pResult->nParam);
				UIMessageBox(szMsg);
				break;
			}
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P0_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital2.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_ASSIGNOFFER: //update cong hien hoac phat' that bai
		{
			if(pResult->nParam < 0)
			{
				UIMessageBox("Kh«ng thÓ ph¸t quü bang liªn tôc, xin ®îi mét thêi gian.");
				break;
			}
			if(!pResult->uParam)
			{
				char szMsg[128];
				sprintf(szMsg, "Cèng hiÕn dù tr÷ bang kh«ng ®ñ %d ®iÓm", pResult->nParam);
				UIMessageBox(szMsg);
				break;
			}
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P2_TxtTotalOffer.SetLabel(szBuffer);
			m_P0_TxtTotalOffer.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_TRANSMONEY:
		{
			if(pResult->nParam < 0)
			{
				UIMessageBox("Ng©n quü kh«ng ®ñ sè tiÒn cÇn chuyÓn vµo ng©n s¸ch kiÕn thiÕt");
				break;
			}
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P0_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital.SetLabel(szBuffer);
			m_P2_TxtTongCapital2.SetLabel(szBuffer);
			sprintf(szBuffer, "%u", pResult->uParam);
			m_P0_TxtBuildFund.SetLabel(szBuffer);
			m_P2_TxtBuildFund.SetLabel(szBuffer);
			m_P2_TxtBuildFund2.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_STOREBUILDFUND:
		{
			if(pResult->nParam < 0)
				break;
			char szBuffer[64];
			sprintf(szBuffer, "%d", pResult->nParam);
			m_P0_TxtBuildFund.SetLabel(szBuffer);
			m_P2_TxtBuildFund.SetLabel(szBuffer);
			m_P2_TxtBuildFund2.SetLabel(szBuffer);
		}
		break;
		case TONG_ACTION_CHANGETITLE:
		{
			for(int i=0;i < (int)m_vMemberList.size(); ++i)
			{
				if(m_vMemberList[i].m_dwNameID == pResult->uParam)
				{
					strcpy(m_vMemberList[i].m_szTitle, pResult->szName);
					break;
				}
			}
		}
		break;
		case TONG_ACTION_CHANGETITLE_MALE:
		{
			for(int i=0;i < (int)m_vMemberList.size(); ++i)
			{
				if(m_vMemberList[i].m_nSex == 0
				&& (m_vMemberList[i].m_nFigure == enumTONG_FIGURE_MEMBER
				|| m_vMemberList[i].m_nFigure == enumTONG_FIGURE_MANAGER))
				{
					strcpy(m_vMemberList[i].m_szTitle, pResult->szName);
				}
			}
		}
		break;
		case TONG_ACTION_CHANGETITLE_FEMALE:
		{
			for(int i=0;i < (int)m_vMemberList.size(); ++i)
			{
				if(m_vMemberList[i].m_nSex
				&& (m_vMemberList[i].m_nFigure == enumTONG_FIGURE_MEMBER
				|| m_vMemberList[i].m_nFigure == enumTONG_FIGURE_MANAGER))
				{
					strcpy(m_vMemberList[i].m_szTitle, pResult->szName);
				}
			}
		}
		break;
	}
}
