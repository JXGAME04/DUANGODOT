// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mapconvert.h"
#include "ProgressWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog


CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgressDlg)
	m_strFile = _T("");
	//}}AFX_DATA_INIT
}


void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, IDC_STATIC_FILE, m_scFile);
	DDX_Text(pDX, IDC_STATIC_FILE, m_strFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

LRESULT CProgressDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
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
		m_scFile.SetWindowText((LPCSTR)wParam);
		break;
	}
	return CDialog::WindowProc(message, wParam, lParam);
}
