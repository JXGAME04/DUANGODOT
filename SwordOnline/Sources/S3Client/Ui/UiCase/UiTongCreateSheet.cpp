/********************************************************************
File        : UiTongCreateSheet.cpp
*********************************************************************/

#include "KWin32.h"
#include "KIniFile.h"
#include "CoreShell.h"
#include "GameDataDef.h"

#include "../elem/wnds.h"
#include "../Elem/WndMessage.h"

#include "../UiBase.h"
#include "../UiSoundSetting.h"

#include "UiTongCreateSheet.h"
#include "UiTongManager.h"

#define TONG_CREATE_SHEET_SCHEME "帮会创建单.ini"

KUiTongCreateSheet* KUiTongCreateSheet::ms_pSelf = NULL;


extern iCoreShell *g_pCoreShell;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

KUiTongCreateSheet::KUiTongCreateSheet()
{
	m_nSelectSide = -1;
}

KUiTongCreateSheet::~KUiTongCreateSheet()
{

}


/*********************************************************************
* 功能：打开窗口
**********************************************************************/
KUiTongCreateSheet* KUiTongCreateSheet::OpenWindow(bool bChangeCamp)
{
	if (ms_pSelf == NULL)
	{
		ms_pSelf = new KUiTongCreateSheet;
		if (ms_pSelf)
			ms_pSelf->Initialize();
	}
	if (ms_pSelf)
	{
		ms_pSelf->SetChangeCamp(bChangeCamp);
		UiSoundPlay(UI_SI_WND_OPENCLOSE);
		ms_pSelf->BringToTop();
		ms_pSelf->Show();
	}
	return ms_pSelf;
}

KUiTongCreateSheet* KUiTongCreateSheet::GetIfVisible()
{
	if (ms_pSelf && ms_pSelf->IsVisible())
		return ms_pSelf;
	return NULL;
}

void KUiTongCreateSheet::CloseWindow(bool bDestory)
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

void KUiTongCreateSheet::SetChangeCamp(bool bChangeCamp)
{
	m_bChangeCamp = bChangeCamp;
	if(m_bChangeCamp)
	{
		SetImage(ISI_T_SPR, m_szImageChangeCamp);
		m_TxtTongName.Hide();
		m_EditTongName.Hide();
		m_TextError.SetText(m_szChangeFaction, strlen(m_szChangeFaction));
	}
}

void KUiTongCreateSheet::Initialize()
{
	AddChild(&m_TxtTongName);
	AddChild(&m_EditTongName);
	AddChild(&m_BtnOrder);
	AddChild(&m_BtnNatural);
	AddChild(&m_BtnChaos);
	AddChild(&m_BtnDone);
	AddChild(&m_BtnCancel);
	AddChild(&m_TextError);
	
	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	Wnd_AddWindow(this);
}

void KUiTongCreateSheet::LoadScheme(const char* pScheme)
{
	if(ms_pSelf)
	{
    	char		Buff[128];
    	KIniFile	Ini;
    	sprintf(Buff, "%s\\%s", pScheme, TONG_CREATE_SHEET_SCHEME);
    	if (Ini.Load(Buff))
    	{
		    ms_pSelf->Init(&Ini, "Main");

		    ms_pSelf->m_BtnOrder.Init(&Ini, "BtnOrder");
			ms_pSelf->m_BtnChaos.Init(&Ini, "BtnChaos");
			ms_pSelf->m_BtnNatural.Init(&Ini, "BtnNatural");

			ms_pSelf->m_TextError.Init(&Ini, "ErrorBox");
			ms_pSelf->m_EditTongName.Init(&Ini, "EditTongName");

			ms_pSelf->m_BtnDone.Init(&Ini, "BtnDone");
			ms_pSelf->m_BtnCancel.Init(&Ini, "BtnCancel");
			ms_pSelf->m_TxtTongName.Init(&Ini, "TxtTongName");
			
			Ini.GetString("ErrorBox", "WithoutName", "",
				ms_pSelf->m_szNameNullString, sizeof(ms_pSelf->m_szNameNullString));
			Ini.GetString("ErrorBox", "WithoutFaction", "",
				ms_pSelf->m_szAlignmentNullString, sizeof(ms_pSelf->m_szAlignmentNullString));
			Ini.GetString("ErrorBox", "ChangeFaction", "",
				ms_pSelf->m_szChangeFaction, sizeof(ms_pSelf->m_szChangeFaction));
			Ini.GetString("ImgTitle", "ImageChangeCamp", "",
				ms_pSelf->m_szImageChangeCamp, sizeof(ms_pSelf->m_szImageChangeCamp));
				
		}
	}
}

int KUiTongCreateSheet::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	switch(uMsg)
	{
	case WND_N_BUTTON_CLICK:
		if(uParam == (unsigned int)&m_BtnOrder )
		{
			AlignmentButtonCheck(camp_justice);
		}
		else if(uParam == (unsigned int)&m_BtnChaos)
		{
			AlignmentButtonCheck( camp_evil  );
		}
		else if(uParam == (unsigned int)&m_BtnNatural  )
		{
			AlignmentButtonCheck(camp_balance);
		}
		/*----------------------------------------------------------*/
		else if(uParam == (unsigned int)&m_BtnCancel)
		{
			CloseWindow();
		}
		else if(uParam == (unsigned int)&m_BtnDone)
		{
			OnDone();
		}
		break;

	default:
		return KWndImage::WndProc(uMsg, uParam, nParam);
		break;
	}
    return 1;
}

void KUiTongCreateSheet::AlignmentButtonCheck(NPCCAMP eSide)
{
	m_BtnOrder.CheckButton(FALSE);
	m_BtnChaos.CheckButton(FALSE);
	m_BtnNatural.CheckButton(FALSE);

	switch(eSide)
	{
	case camp_justice:
		m_BtnOrder.CheckButton(TRUE);
		break;

	case camp_evil:
		m_BtnChaos.CheckButton(TRUE);
		break;

	case camp_balance:
		m_BtnNatural.CheckButton(TRUE);
		break;
	}
	m_nSelectSide = eSide;
}

void KUiTongCreateSheet::OnDone()
{
	if(m_bChangeCamp)
	{
		if(m_nSelectSide != -1)
		{
			KUiTongManager* pTMgr = KUiTongManager::GetIfVisible();
			if(pTMgr)
				pTMgr->WndProc(WND_M_OTHER_WORK_RESULT, TONG_ACTION_CHANGECAMP, m_nSelectSide);
			CloseWindow();
		}
		else
		{
			m_TextError.SetText(m_szAlignmentNullString, strlen(m_szAlignmentNullString));
		}
	}
	else
	{
		char Buff[16];
		m_EditTongName.GetText(Buff, sizeof(Buff), TRUE);
		if(Buff[0] && g_pCoreShell)
		{
			if(m_nSelectSide != -1)
			{
				g_pCoreShell->TongOperation(GTOI_TONG_CREATE, (unsigned int)Buff, (int)m_nSelectSide);
				CloseWindow();
			}
			else
			{
				m_TextError.SetText(m_szAlignmentNullString, strlen(m_szAlignmentNullString));
			}
		}
		else
		{
			m_TextError.SetText(m_szNameNullString, strlen(m_szNameNullString));
		}
	}
}
