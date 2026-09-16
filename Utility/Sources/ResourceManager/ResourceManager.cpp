// ResourceManager.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ResourceManager.h"
#include "ResourceManagerDlg.h"
#include "kengine.h"
#include "ktabfilectrl.h"
#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

KTabFileCtrl g_SysTabFile;//Npc资源设定文件
/////////////////////////////////////////////////////////////////////////////
// CResourceManagerApp

BEGIN_MESSAGE_MAP(CResourceManagerApp, CWinApp)
	//{{AFX_MSG_MAP(CResourceManagerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceManagerApp construction

CResourceManagerApp::CResourceManagerApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CResourceManagerApp object

CResourceManagerApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CResourceManagerApp initialization

BOOL CResourceManagerApp::InitInstance()
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

	g_SetRootPath(NULL);
	g_SetFilePath("\\Settings\\npcres");
	CString strRes = "\\Settings\\npcres\\人物类型.txt";
	AppGetFullPath(strRes);
	g_FindDebugWindow("#32770","DebugWin");
//	if (!g_SysTabFile.Load(const_cast<LPSTR>((LPCSTR)strRes)))	AfxMessageBox("无法打开settings\\npcres\\人物类型.txt文件！");
	g_DebugLog("Hi Test!");
	CResourceManagerDlg dlg;
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


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

