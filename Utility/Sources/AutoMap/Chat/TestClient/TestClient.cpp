// TestClient.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "cPlayer.h"
#include "TestClient.h"
#include "TestClientDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestClientApp

BEGIN_MESSAGE_MAP(CTestClientApp, CWinApp)
	//{{AFX_MSG_MAP(CTestClientApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestClientApp construction

CTestClientApp::CTestClientApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTestClientApp object

CTestClientApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTestClientApp initialization

BOOL CTestClientApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

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


	bool bInit = false;
	for (int i=0; i<c_nLink; i++)
	{
		aPlayer[i] = new cPlayer;
		aPlayer[i]->Create(i);
		
		aClient[i] = Net_CreateClient(0);
	}

	aClient[0]->InitSocket();
	for (i=0; i<c_nLink; i++)
	{
		aClient[i]->Create();

		aClient[i]->BindPlayer(aPlayer[i]);
		aPlayer[i]->BindLink(aClient[i]);

		aClient[i]->RegisterHandle(PlayerHandleMessage);
		stServerInfo info;
		strcpy(info.strServerIP,"yujiang");
		info.wServerPort = c_nPort;
		aClient[i]->Connect(info);
	}

	CTestClientDlg dlg;
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

	for (i=0; i<c_nLink; i++)
	{
		aClient[i]->CloseClient();
	}
	aClient[0]->CleanSocket();
	for (i=0; i<c_nLink; i++)
	{
		aClient[i]->Release();
		delete aPlayer[i];
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CTestClientApp::OnTimer()
{
	for (int i=0; i<c_nLink; i++)
	{
		aPlayer[i]->Run();
	}
}
