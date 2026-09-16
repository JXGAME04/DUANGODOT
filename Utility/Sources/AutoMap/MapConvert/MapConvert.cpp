// MapConvert.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MapConvert.h"
#include "MapConvertDlg.h"

#include "clog.h"
#include "fileop.h"

#include "progressWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapConvertApp

BEGIN_MESSAGE_MAP(CMapConvertApp, CWinApp)
	//{{AFX_MSG_MAP(CMapConvertApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapConvertApp construction


CMapConvertApp::CMapConvertApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_pDlg = NULL;
	m_pLog = new cAppLog;
}

void CMapConvertApp::WriteLogString(LPCSTR szString)
{
//	m_file << szString << "\n";
	m_pLog->write_log(szString);
}

void CMapConvertApp::OpenLog()
{
	m_pLog->save();
	m_pLog->open();
}

CMapConvertApp::~CMapConvertApp()
{
	if (m_pDlg)
	{
		m_pDlg->DestroyWindow();
		delete m_pDlg;
	}
	delete m_pLog;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMapConvertApp object

CMapConvertApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMapConvertApp initialization

BOOL CMapConvertApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	if (!m_pLog->create())
	{
		AfxMessageBox("can not create log file!");
		return FALSE;
	}

	CMapConvertDlg* pDlg = new CMapConvertDlg;
	CMapConvertDlg& dlg = *pDlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	delete pDlg;

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CMapConvertApp::SetProgressStart()
{
/*
	if (m_pDlg == NULL)
	{
		m_pDlg = new CProgressWnd;
		m_pDlg->Create();
		m_pDlg->ShowWindow(SW_SHOW);
	}
	m_pDlg->PostMessage(WM_USER_SHOWWINDOW,TRUE);
*/
}

void CMapConvertApp::SetProgressFile(LPCSTR szFile)
{
/*	
	m_strFile = szFile;
	m_pDlg->PostMessage(WM_USER_SETTEXT,(long)((LPCSTR)m_strFile));
*/
	OutputDebugString(szFile);
	OutputDebugString("\n");
}

void CMapConvertApp::SetProgressEnd()
{
//	m_pDlg->PostMessage(WM_USER_SHOWWINDOW,FALSE);
}

void OutputProgress(LPCSTR sz)
{
	((CMapConvertDlg*)(theApp.GetMainWnd()))->OutputProgress(sz);;
}
