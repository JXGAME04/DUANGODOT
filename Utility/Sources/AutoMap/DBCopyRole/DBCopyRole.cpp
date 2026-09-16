// DBCopyRole.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DBCopyRole.h"
#include "DBCopyRoleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDBCopyRoleApp

BEGIN_MESSAGE_MAP(CDBCopyRoleApp, CWinApp)
	//{{AFX_MSG_MAP(CDBCopyRoleApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBCopyRoleApp construction

CDBCopyRoleApp::CDBCopyRoleApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDBCopyRoleApp object

CDBCopyRoleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDBCopyRoleApp initialization

BOOL CDBCopyRoleApp::InitInstance()
{
	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CDBCopyRoleDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
