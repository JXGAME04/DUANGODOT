// -------------------------------------------------------------------------
//	UiGetString.h
// -------------------------------------------------------------------------
#ifndef __UiGetString_H__
#define __UiGetString_H__

#include "../Elem/WndButton.h"
#include "../Elem/WndEdit.h"


class KUiGetString : protected KWndImage
{
public:
	static KUiGetString*	OpenWindow(const char* pszTitle,
				const char* pszInitString,				
				KWndWindow* pRequester, unsigned int uParam,
				int nMinLen = 4, int nMaxLen = 16, bool bSpecFocus = false); //guve
	static KUiGetString*	GetIfVisible();
	static void			LoadScheme(const char* pScheme);
	static void			CloseWindow(bool bDestroy);
private:
	KUiGetString();
	~KUiGetString() {}
	void	Initialize();
	void	Show();
	void	Hide();
	int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);
	void	OnCancel();
	void	OnOk();
private:
	static KUiGetString*	m_pSelf;
	KWndText32			m_Title;
	short				m_nMinLen, m_nMaxLen;
	KWndEdit32			m_StringEdit;
	KWndButton			m_OkBtn;
	KWndButton			m_CancelBtn;
	KWndWindow*			m_pRequester;
	unsigned int		m_uRequesterParam;
	bool				m_bSpecFocus;
};


#endif // __UiGetString_H__