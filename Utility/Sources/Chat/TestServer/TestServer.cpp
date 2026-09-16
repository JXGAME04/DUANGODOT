// TestServer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TestServer.h"
#include "TestServerDlg.h"

#include "..\cPlayer.h"
#include "..\cWorld.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestServerApp

BEGIN_MESSAGE_MAP(CTestServerApp, CWinApp)
	//{{AFX_MSG_MAP(CTestServerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestServerApp construction

CTestServerApp::CTestServerApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTestServerApp object

CTestServerApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTestServerApp initialization

BOOL CTestServerApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	m_pServer = Net_CreateServer();
	VERIFY(m_pServer->InitSocket());
	VERIFY(m_pServer->Create(c_nPort));
	m_pServer->RegisterHandle(PlayerHandleMessage);
	ASSERT(m_pServer->RunThread());

	m_pWorld = new cWorld;
	m_pWorld->RunThread();

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

	CTestServerDlg dlg;
	m_pMainWnd = &dlg;
	m_pDlg = &dlg;
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
	m_pDlg = NULL;

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	VERIFY(m_pServer->Close());
	VERIFY(m_pServer->CleanSocket());
	m_pServer->Release();

	m_pWorld->EndThread();
	delete m_pWorld;

	return FALSE;
}

void CTestServerApp::OutputString(LPCSTR szAccount,LPCSTR szMessage)
{
	if (m_pDlg != NULL)
		m_pDlg->AddString(szAccount,szMessage);
}

int CTestServerApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWinApp::ExitInstance();
}

void OutputString(LPCSTR szAccount,LPCSTR szMessage)
{
	theApp.OutputString(szAccount,szMessage);
}

iServer* AppGetServer()
{
	return theApp.GetServer();
}

cWorld* AppGetWorld()
{
	return theApp.GetWorld();
}
