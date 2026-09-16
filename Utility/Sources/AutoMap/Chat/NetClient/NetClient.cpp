// NetClient.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NetClient.h"
#include "NetClientDlg.h"

#include "..\cPlayer.h"
#include "..\cWorld.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetClientApp

BEGIN_MESSAGE_MAP(CNetClientApp, CWinApp)
	//{{AFX_MSG_MAP(CNetClientApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetClientApp construction

CNetClientApp::CNetClientApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_pDlg = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNetClientApp object

CNetClientApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNetClientApp initialization

BOOL CNetClientApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	m_pClient = Net_CreateClient();
	VERIFY(m_pClient->InitSocket());
	VERIFY(m_pClient->Create());
	m_pClient->RegisterHandle(PlayerHandleMessage);
	#pragma message (ATTENTION("load a ini for server and port?"))
	stServerInfo info;
	strcpy(info.strServerIP,"yujiang");
	info.wServerPort = c_nPort;
	VERIFY(m_pClient->Connect(info));

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

	CNetClientDlg dlg;
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

	m_pClient->CloseClient();
	VERIFY(m_pClient->CleanSocket());
	m_pClient->Release();

	m_pWorld->EndThread();
	delete m_pWorld;
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CNetClientApp::OutputString(LPCSTR szAccount,LPCSTR szMessage)
{
	if (m_pDlg != NULL)
		m_pDlg->AddString(szAccount,szMessage);
}

void CNetClientApp::EnableNetStatus(NetStatus e,BOOL b)
{
	if (m_pDlg != NULL)
		m_pDlg->EnableNetStatus(e,b);
}

void CNetClientApp::OnQuit()
{
	GetWorld()->Client_Quit();
}

iClient* AppGetClient()
{
	return theApp.GetClient();
}

cWorld* AppGetWorld()
{
	return theApp.GetWorld();
}

void OutputString(LPCSTR szAccount,LPCSTR szMessage)
{
	theApp.OutputString(szAccount,szMessage);
}

void EnableNetStatus(NetStatus e, BOOL b)
{
	theApp.EnableNetStatus(e,b);
}

