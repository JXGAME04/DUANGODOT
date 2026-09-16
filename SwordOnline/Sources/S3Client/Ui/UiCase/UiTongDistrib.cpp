/*******************************************************************************
File        : UiTongDistrib.h
********************************************************************************/

#include "KWin32.h"
#include "KIniFile.h"

#include "../elem/wnds.h"
#include "../elem/wndmessage.h"

#include "../UiBase.h"
#include "../UiSoundSetting.h"

#include "../../../Engine/src/KFilePath.h"
#include "../../Core/Src/GameDataDef.h"

#include "UiTongDistrib.h"


#define TONG_DISTRIBUTION_BOX_INI "°ï»á·¢Ç®½çÃæ.ini"

KUiTongDistrib* KUiTongDistrib::ms_pSelf = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

KUiTongDistrib::KUiTongDistrib()
{
	m_pMain = NULL;
}

KUiTongDistrib::~KUiTongDistrib()
{
	m_pMain = NULL;
}

KUiTongDistrib* KUiTongDistrib::OpenWindow(int nUiType, KWndWindow* pCaller, UINT uParam)
{
    if (ms_pSelf == NULL)
    {
	    ms_pSelf = new KUiTongDistrib;
	    if (ms_pSelf)
    		ms_pSelf->Initialize();
    }
    if (ms_pSelf)
    {
	    UiSoundPlay(UI_SI_WND_OPENCLOSE);
		ms_pSelf->m_pMain = pCaller;
		ms_pSelf->m_nUiType = nUiType;
		ms_pSelf->m_uParam = uParam;
		ms_pSelf->m_ImgTitleMoney.Hide();
		ms_pSelf->m_ImgTitleOffer.Hide();
		if(!nUiType)
		{
			ms_pSelf->m_ImgTitleMoney.Show();
			ms_pSelf->m_UnitDirector.SetText("v¹n");
			ms_pSelf->m_UnitManager.SetText("v¹n");
			ms_pSelf->m_UnitMember.SetText("v¹n");
		}
		else
		{
			ms_pSelf->m_ImgTitleOffer.Show();
			ms_pSelf->m_UnitDirector.SetText("®iÓm");
			ms_pSelf->m_UnitManager.SetText("®iÓm");
			ms_pSelf->m_UnitMember.SetText("®iÓm");
		}
		ms_pSelf->BringToTop();
		ms_pSelf->Show();
		Wnd_SetExclusive((KWndWindow*)ms_pSelf);
	}
	return ms_pSelf;
}

KUiTongDistrib* KUiTongDistrib::GetIfVisible()
{
	if (ms_pSelf && ms_pSelf->IsVisible())
		return ms_pSelf;
	return NULL;
}

void KUiTongDistrib::CloseWindow(bool bDestory)
{
	if (ms_pSelf)
	{
		ms_pSelf->m_pMain = NULL;
		Wnd_ReleaseExclusive((KWndWindow*)ms_pSelf);
		ms_pSelf->Hide();
		if (bDestory)
		{
			ms_pSelf->Destroy();
			ms_pSelf = NULL;
		}
	}
}

void KUiTongDistrib::Initialize()
{
	AddChild(&m_ImgTitleMoney);
	AddChild(&m_ImgTitleOffer);
	AddChild(&m_TxtDirector);
	AddChild(&m_TxtManager);
	AddChild(&m_TxtMember);
	AddChild(&m_EditDirector);
	AddChild(&m_EditManager);
	AddChild(&m_EditMember);
	AddChild(&m_UnitDirector);
	AddChild(&m_UnitManager);
	AddChild(&m_UnitMember);
	AddChild(&m_TextError);
	AddChild(&m_BtnConfirm);
	AddChild(&m_BtnCancel);

	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	Wnd_AddWindow(this);
}

void KUiTongDistrib::LoadScheme(const char* pScheme)
{
	if(ms_pSelf)
	{
		char		Buff[128];
		KIniFile	Ini;
		sprintf(Buff, "%s\\%s", pScheme, TONG_DISTRIBUTION_BOX_INI);

		if(Ini.Load(Buff))
		{
			ms_pSelf->Init(&Ini, "Main");
			ms_pSelf->m_ImgTitleMoney.Init(&Ini, "ImgTitleMoney");
			ms_pSelf->m_ImgTitleOffer.Init(&Ini, "ImgTitleOffer");
			ms_pSelf->m_TxtDirector.Init(&Ini, "TxtDirector");
			ms_pSelf->m_TxtManager.Init(&Ini, "TxtManager");
			ms_pSelf->m_TxtMember.Init(&Ini, "TxtMember");
			ms_pSelf->m_EditDirector.Init(&Ini, "EditDirector");
			ms_pSelf->m_EditManager.Init(&Ini, "EditManager");
			ms_pSelf->m_EditMember.Init(&Ini, "EditMember");
			ms_pSelf->m_UnitDirector.Init(&Ini, "UnitDirector");
			ms_pSelf->m_UnitManager.Init(&Ini, "UnitManager");
			ms_pSelf->m_UnitMember.Init(&Ini, "UnitMember");
			ms_pSelf->m_TextError.Init(&Ini, "TextDescription");
			ms_pSelf->m_BtnCancel.Init(&Ini, "BtnCancel");
			ms_pSelf->m_BtnConfirm.Init(&Ini, "BtnConfirm");
		}
	}
}

int KUiTongDistrib::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	switch(uMsg)
	{
	case WND_N_BUTTON_CLICK:
		if(uParam == (unsigned int)&m_BtnConfirm)
		{
			OnConfirm();
		}
		else if(uParam == (unsigned int)&m_BtnCancel)
		{
			CloseWindow();
		}
		break;
		
	default:
		return KWndImage::WndProc(uMsg, uParam, nParam);
	}

	return 1;
}

void KUiTongDistrib::OnConfirm()
{
	char	szString[32];
	int nPoint[3];
	m_EditDirector.GetText(szString, sizeof(szString), true);
	nPoint[enumTONG_FIGURE_DIRECTOR] = atoi(szString);
	m_EditManager.GetText(szString, sizeof(szString), true);
	nPoint[enumTONG_FIGURE_MANAGER] = atoi(szString);
	m_EditMember.GetText(szString, sizeof(szString), true);
	nPoint[enumTONG_FIGURE_MEMBER] = atoi(szString);
	if(nPoint[enumTONG_FIGURE_DIRECTOR] <= 0 &&
	nPoint[enumTONG_FIGURE_MANAGER] <= 0 &&
	nPoint[enumTONG_FIGURE_MEMBER] <= 0)
	{
		if(!m_nUiType)
			m_TextError.SetText("Ph¸t 0 v¹n l­îng cho tÊt c¶ chøc vô thµnh viªn lµ kh«ng hîp lÖ.");
		else
			m_TextError.SetText("Ph¸t 0 ®iÓm cèng hiÕn cho tÊt c¶ chøc vô thµnh viªn lµ kh«ng hîp lÖ.");
		return;
	}
	if(nPoint[enumTONG_FIGURE_DIRECTOR] > defTONG_MAX_OFFER_DAYLIMIT ||
	nPoint[enumTONG_FIGURE_MANAGER] > defTONG_MAX_OFFER_DAYLIMIT ||
	nPoint[enumTONG_FIGURE_MEMBER] > defTONG_MAX_OFFER_DAYLIMIT)
	{
		char szMsg[128];
		if(!m_nUiType)
		{
			sprintf(szMsg, "Mçi lÇn ph¸t ng©n quü tèi ®a chØ ®­îc %d v¹n l­îng", defTONG_MAX_OFFER_DAYLIMIT);
			m_TextError.SetText(szMsg);
		}
		else
		{
			sprintf(szMsg, "Mçi lÇn ph¸t cèng hiÕn tèi ®a chØ ®­îc %d ®iÓm", defTONG_MAX_OFFER_DAYLIMIT);
			m_TextError.SetText(szMsg);
		}
		return;
	}
	if(m_pMain)
		m_pMain->WndProc(WND_M_OTHER_WORK_RESULT, m_uParam, (int)&nPoint);
	CloseWindow();
}
