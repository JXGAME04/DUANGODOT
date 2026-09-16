// ProgressWnd.h: interface for the CProgressWnd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROGRESSWND_H__43948613_3D6E_40F8_B350_0DD56249D076__INCLUDED_)
#define AFX_PROGRESSWND_H__43948613_3D6E_40F8_B350_0DD56249D076__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WM_USER_SHOWWINDOW WM_USER+1
#define WM_USER_SETTEXT WM_USER+2

class CProgressWnd : public CWnd  
{
public:
	CProgressWnd();
	virtual ~CProgressWnd();

	BOOL Create();

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif // !defined(AFX_PROGRESSWND_H__43948613_3D6E_40F8_B350_0DD56249D076__INCLUDED_)
