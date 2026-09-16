// ProgressWnd.cpp: implementation of the CProgressWnd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mapconvert.h"
#include "ProgressWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProgressWnd::CProgressWnd()
{

}

CProgressWnd::~CProgressWnd()
{

}

BOOL CProgressWnd::Create()
{
	CPoint ptCenter = (1024/2, 768/2);
	int width = 100;
	int height = 100;
	CRect r;
	r.right = ptCenter.x + width;
	r.top = ptCenter.y - height;
	r.left = ptCenter.x - width;
	r.bottom = ptCenter.y + height;
	
	DWORD style = WS_VISIBLE|WS_POPUP;
	BOOL b=CWnd::CreateEx(NULL,AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS),
		"edit Window",style,
		r,NULL,0);
	
	ASSERT(b);

	return b;
}

LRESULT CProgressWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	switch (message)
	{
	case WM_USER_SHOWWINDOW:
		if (wParam == 0)
			ShowWindow(SW_HIDE);
		else
			ShowWindow(SW_SHOW);
		break;
	case WM_USER_SETTEXT:
		SetWindowText((LPCSTR)wParam);
		break;
	}
	return CWnd::WindowProc(message, wParam, lParam);
}
