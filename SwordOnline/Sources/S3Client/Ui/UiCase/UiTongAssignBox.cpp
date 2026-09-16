/*******************************************************************************
File        : UiTongAssignBox.h
********************************************************************************/

#include "KWin32.h"
#include "KIniFile.h"

#include "../elem/wnds.h"
#include "../elem/wndmessage.h"

#include "../UiBase.h"
#include "../UiSoundSetting.h"

#include "../../../Engine/src/KFilePath.h"
#include "../../Core/Src/GameDataDef.h"

#include "UiTongAssignBox.h"


#define TONG_ASSIGN_BOX_INI "°ï»áÖ°Î»ÈÎÃü.ini"

KUiTongAssignBox* KUiTongAssignBox::ms_pSelf = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

KUiTongAssignBox::KUiTongAssignBox()
{
	m_pMain = NULL;
}

KUiTongAssignBox::~KUiTongAssignBox()
{
	m_pMain = NULL;
}

KUiTongAssignBox* KUiTongAssignBox::OpenWindow(int nCurFigure, const char* pName, KWndWindow* pCaller, UINT uParam)
{
    if (ms_pSelf == NULL)
    {
	    ms_pSelf = new KUiTongAssignBox;
	    if (ms_pSelf)
    		ms_pSelf->Initialize();
    }
    if (ms_pSelf)
    {
	    UiSoundPlay(UI_SI_WND_OPENCLOSE);
		ms_pSelf->m_pMain = pCaller;
		ms_pSelf->m_nCurrentFigure = nCurFigure;
		ms_pSelf->m_nSelectFigure = enumTONG_FIGURE_DIRECTOR;
		ms_pSelf->m_uParam = uParam;
		ms_pSelf->m_BtnElder.CheckButton(1);
		ms_pSelf->m_BtnCaptain.CheckButton(0);
		ms_pSelf->m_BtnMember.CheckButton(0);
		ms_pSelf->m_TargetName.SetText(pName);
		ms_pSelf->BringToTop();
		ms_pSelf->Show();
		Wnd_SetExclusive((KWndWindow*)ms_pSelf);
	}
	return ms_pSelf;
}

KUiTongAssignBox* KUiTongAssignBox::GetIfVisible()
{
	if (ms_pSelf && ms_pSelf->IsVisible())
		return ms_pSelf;
	return NULL;
}

void KUiTongAssignBox::CloseWindow(bool bDestory)
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

void KUiTongAssignBox::Initialize()
{
	AddChild(&m_TitlePlayerName);
	AddChild(&m_TargetName);
	AddChild(&m_TitlePositionName);
	AddChild(&m_BtnElder);
	AddChild(&m_BtnCaptain);
	AddChild(&m_BtnMember);
	AddChild(&m_BtnConfirm);
	AddChild(&m_BtnCancel);
	AddChild(&m_TextError);

	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	Wnd_AddWindow(this);
}

void KUiTongAssignBox::LoadScheme(const char* pScheme)
{
	if(ms_pSelf)
	{
		char		Buff[128];
		KIniFile	Ini;
		sprintf(Buff, "%s\\%s", pScheme, TONG_ASSIGN_BOX_INI);

		if(Ini.Load(Buff))
		{
			ms_pSelf->Init(&Ini, "Main");
			ms_pSelf->m_TitlePlayerName.Init(&Ini, "TitlePlayerName");
			ms_pSelf->m_TargetName.Init(&Ini, "PlayerName");
			ms_pSelf->m_TitlePositionName.Init(&Ini, "TitlePositionName");
			ms_pSelf->m_BtnConfirm.Init(&Ini, "BtnConfirm");
			ms_pSelf->m_BtnCancel.Init(&Ini, "BtnCancel");
			ms_pSelf->m_BtnMember.Init(&Ini, "BtnMember");
			ms_pSelf->m_BtnCaptain.Init(&Ini, "BtnManager");
			ms_pSelf->m_BtnElder.Init(&Ini, "BtnDirector");
			ms_pSelf->m_TextError.Init(&Ini, "ErrorBox");
			ms_pSelf->m_BtnElder.SetText("Tr­ëng l·o");
			ms_pSelf->m_BtnCaptain.SetText("§éi tr­ëng");
			ms_pSelf->m_BtnMember.SetText("M«n ®Ö");
		}
	}
}

int KUiTongAssignBox::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
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
		else if(uParam == (unsigned int)&m_BtnElder)
		{
			m_BtnElder.CheckButton(1);
			m_BtnCaptain.CheckButton(0);
			m_BtnMember.CheckButton(0);
			m_nSelectFigure = enumTONG_FIGURE_DIRECTOR;
		}
		else if(uParam == (unsigned int)&m_BtnCaptain)
		{
			m_BtnElder.CheckButton(0);
			m_BtnCaptain.CheckButton(1);
			m_BtnMember.CheckButton(0);
			m_nSelectFigure = enumTONG_FIGURE_MANAGER;
		}
		else if(uParam == (unsigned int)&m_BtnMember)
		{
			m_BtnElder.CheckButton(0);
			m_BtnCaptain.CheckButton(0);
			m_BtnMember.CheckButton(1);
			m_nSelectFigure = enumTONG_FIGURE_MEMBER;
		}
		break;
		
	default:
		return KWndImage::WndProc(uMsg, uParam, nParam);
	}

	return 1;
}

void KUiTongAssignBox::OnConfirm()
{
	if(m_nSelectFigure == m_nCurrentFigure)
	{
		m_TextError.SetText("Chøc vÞ bæ nhiÖm trïng chøc vÞ hiÖn t¹i. H·y chän chøc vÞ kh¸c ®Ó bæ nhiÖm.");
		return;
	}
	if(m_pMain)
		m_pMain->WndProc(WND_M_OTHER_WORK_RESULT, m_uParam, m_nSelectFigure);
	CloseWindow();
}
