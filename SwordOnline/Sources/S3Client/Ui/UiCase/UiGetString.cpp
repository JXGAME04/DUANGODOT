// -------------------------------------------------------------------------
//	UiGetString.cpp
// -------------------------------------------------------------------------
#include "KWin32.h"
#include "KIniFile.h"
#include "../Elem/AutoLocateWnd.h"
#include "../Elem/WndMessage.h"
#include "../Elem/Wnds.h"
#include "../UiBase.h"
#include "UiGetString.h"
#include "UiInformation.h"
#include "../UiSoundSetting.h"


#define	SCHEME_INI 	"ÊäÈë×Ö´®½çÃæ.ini"


KUiGetString* KUiGetString::m_pSelf = NULL;

KUiGetString::KUiGetString()
{
	m_pRequester = NULL;
}

KUiGetString* KUiGetString::OpenWindow(const char* pszTitle,
						const char* pszInitString,
						KWndWindow* pRequester, unsigned int uParam,
 						int nMinLen, int nMaxLen, bool bSpecFocus)
{
	if (m_pSelf == NULL)
	{
		m_pSelf = new KUiGetString;
		if (m_pSelf)
			m_pSelf->Initialize();
	}
	if (m_pSelf)
	{
		UiSoundPlay(UI_SI_WND_OPENCLOSE);
		m_pSelf->m_nMinLen = nMinLen;
		m_pSelf->m_nMaxLen = nMaxLen;
		m_pSelf->m_pRequester = pRequester;
		m_pSelf->m_uRequesterParam = uParam;
		if (pszTitle)
			m_pSelf->m_Title.SetText(pszTitle);
		else
			m_pSelf->m_Title.SetText("");
		if (pszInitString)
			m_pSelf->m_StringEdit.SetText(pszInitString);
		else
			m_pSelf->m_StringEdit.ClearText();
		m_pSelf->m_bSpecFocus = bSpecFocus;
		m_pSelf->BringToTop();
		m_pSelf->Show();
	}
	return m_pSelf;
}

void KUiGetString::CloseWindow(bool bDestroy)
{
	if (m_pSelf)
	{
		m_pSelf->m_pRequester = NULL;
		if (bDestroy)
		{
			m_pSelf->Destroy();
			m_pSelf = NULL;
		}
		else
			m_pSelf->Hide();
	}
}

KUiGetString*	KUiGetString::GetIfVisible()
{
	if (m_pSelf && m_pSelf->IsVisible())
		return m_pSelf;
	return NULL;
}

void KUiGetString::Show()
{
	int Left, Top;
	ALW_GetWndPosition(Left, Top, m_Width, m_Height);
	SetPosition(Left, Top);
	KWndImage::Show();
	if(!m_bSpecFocus)
		Wnd_SetFocusWnd(&m_StringEdit);
	Wnd_SetExclusive((KWndWindow*)this);
}

void KUiGetString::Hide()
{
	Wnd_SetFocusWnd(NULL);
	Wnd_ReleaseExclusive((KWndWindow*)this);
	KWndImage::Hide();
}

void KUiGetString::Initialize()
{
	AddChild(&m_Title);
	AddChild(&m_StringEdit);
	AddChild(&m_OkBtn);
	AddChild(&m_CancelBtn);
	char Scheme[256];
	g_UiBase.GetCurSchemePath(Scheme, 256);
	LoadScheme(Scheme);

	Wnd_AddWindow(this);
}

void KUiGetString::LoadScheme(const char* pScheme)
{
	if (m_pSelf)
	{
		char		Buff[128];
		KIniFile	Ini;
		sprintf(Buff, "%s\\%s", pScheme, SCHEME_INI);
		if (Ini.Load(Buff))
		{
			m_pSelf->Init(&Ini, "Main");
			m_pSelf->m_Title.Init(&Ini, "Title");
			m_pSelf->m_StringEdit.Init(&Ini, "StringInput");
			m_pSelf->m_OkBtn.Init(&Ini, "OkBtn");
			m_pSelf->m_CancelBtn.Init(&Ini, "CancelBtn");
		}
	}
}

int KUiGetString::WndProc(unsigned int uMsg, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uMsg)
	{
	case WND_N_BUTTON_CLICK:
		if (uParam == (unsigned int)(KWndWindow*)&m_OkBtn)
			OnOk();
		else if (uParam == (unsigned int)(KWndWindow*)&m_CancelBtn)
			OnCancel();
		break;
	case WM_KEYDOWN:
		if (uParam == VK_RETURN)
		{
			OnOk();
			nRet = 1;
		}
		else if (uParam == VK_ESCAPE)
		{
			OnCancel();
			nRet = 1;
		}
		break;
	case WND_N_EDIT_SPECIAL_KEY_DOWN:
		if (nParam == VK_RETURN &&
			uParam == (unsigned int)(KWndWindow*)&m_StringEdit)
		{
			OnOk();
			return 1;
		}
		break;
	case WND_M_SET_FOCUS:
		if ((KWndWindow*)uParam == (KWndWindow*)&m_StringEdit)
		{
			if(m_bSpecFocus)
			{
				char szString[32];
				szString[0] = 0;
				m_StringEdit.GetText(szString, sizeof(szString), true);
				if(!(szString[0] >= '0' && szString[0] <= '9'))
					m_StringEdit.SetText("");
			}
		}
		break;
	default:
		return KWndImage::WndProc(uMsg, uParam, nParam);
	}
	return 0;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÏìÓ¦µã»÷È·ÈÏ°´Å¥
//--------------------------------------------------------------------------
void KUiGetString::OnOk()
{
	char	szString[32];

	int nLen = m_StringEdit.GetText(szString, sizeof(szString), true);
	if (nLen >= m_nMinLen && nLen <= m_nMaxLen)
	{
		if (m_pRequester)
			m_pRequester->WndProc(WND_M_OTHER_WORK_RESULT, m_uRequesterParam, (int)&szString);
		CloseWindow(true);
	}
	else
	{
		UIMessageBox(((nLen < m_nMinLen) ?
			"NhËp d÷ liÖu qu¸ ng¾n" : "NhËp d÷ liÖu qu¸ dµi"));
	}
}

void KUiGetString::OnCancel()
{
	if (m_pRequester)
		m_pRequester->WndProc(WND_M_OTHER_WORK_RESULT, m_uRequesterParam, 0);
	CloseWindow(true);
}
