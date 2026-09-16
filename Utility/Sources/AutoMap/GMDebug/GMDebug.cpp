// GMDebug.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "GMDebug.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "GMDebugDoc.h"
#include "GMDebugView.h"

#include "AddressDlg.h"
#include "NameDlg.h"
#include "WinSock2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline const char* _ip2a(DWORD ip) { in_addr ia; ia.s_addr = ip; return inet_ntoa(ia); }

void STDMETHODCALLTYPE CSystemEvent::GWLogin(char* szGWName, DWORD nIP)
{
	if (m_pParent)
	{
		m_pParent->NewRelay(szGWName, nIP);
	}
}

void STDMETHODCALLTYPE CSystemEvent::GWLogout(char* szGWName, DWORD nIP)
{
	if (m_pParent)
	{
		m_pParent->RemoveRelay(szGWName, nIP);
	}
}

void STDMETHODCALLTYPE CSystemEvent::GWRelayID(char* szGWName, DWORD nRelayID)
{
	if (m_pParent)
	{
		m_pParent->RelayID(szGWName, nRelayID);
	}
}

void STDMETHODCALLTYPE CSystemEvent::GWSubLogout(DWORD nGWIP, DWORD nSubIP)
{
	if (m_pParent)
	{
		CString str;
		str.Format("Error(\"GameWorld %s Closed\")", _ip2a(nSubIP));
		m_pParent->NewMessage("", "", str, str.GetLength(), nGWIP);
	}
}

void STDMETHODCALLTYPE CSystemEvent::Ping(DWORD nTick)
{
	CMainFrame* pWnd = (CMainFrame*)AfxGetMainWnd();
	pWnd->SetPingTime(nTick);
}

void STDMETHODCALLTYPE CSystemEvent::ConnectClosed()
{
	if (m_pParent)
	{
		m_pParent->CloseDocs();
	}
}

void STDMETHODCALLTYPE CSystemEvent::PlayerMessage(char* szAccountFrom, char* szRole, char* szMessage, int nMsgLen, DWORD dwGWIP)
{
	if (m_pParent)
	{
		m_pParent->NewMessage(szAccountFrom, szRole, szMessage, nMsgLen, dwGWIP);
	}
}

void STDMETHODCALLTYPE CSystemEvent::LosePlayer(char* szAccount, DWORD dwGWIP)
{
	if (m_pParent)
	{
		CString strError;
		strError.Format("Error(\"%s no found at %s\")", szAccount, _ip2a(dwGWIP));
		m_pParent->ErrorMessage(strError);
	}
}

void STDMETHODCALLTYPE CSystemEvent::AccountUserReturn(DWORD Operate, char* szAccount, int nReturn)
{
	if (m_pParent)
	{
		CString strError;
		strError.Format("AccountUserReturn(\"%s\", %d)", szAccount, nReturn);
		m_pParent->ErrorMessage(strError);
	}
}

void STDMETHODCALLTYPE CSystemEvent::OnlinePlayerCount(DWORD Operate, char* szGWName, DWORD nCount)
{
	if (m_pParent)
	{
		CString strError;
		strError.Format("OnlinePlayerCount(\"%s\", %d)", szGWName, nCount);
		m_pParent->ErrorMessage(strError);
	}
}

void STDMETHODCALLTYPE CSystemEvent::RegisterUserCount(DWORD Operate, DWORD nCount)
{
	if (m_pParent)
	{
		CString strError;
		strError.Format("RegisterUserCount(%d)", nCount);
		m_pParent->ErrorMessage(strError);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGMDebugApp

BEGIN_MESSAGE_MAP(CGMDebugApp, CWinApp)
	//{{AFX_MSG_MAP(CGMDebugApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(IDM_SELFINFO, OnSelfinfo)
	ON_COMMAND(IDM_AUTORUN, OnAutorun)
	//}}AFX_MSG_MAP
	// Standard file based document commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMDebugApp construction

CGMDebugApp::CGMDebugApp()
{
	m_hDllJXGM = NULL;
	m_pISystemConnect = NULL;
	
	m_Event.m_pParent = this;

	m_nOperate = 0;

	m_bAuto = false;
}

CGMDebugApp::~CGMDebugApp()
{
	CloseConnect();
	
	if (m_hDllJXGM)
	{
		FreeLibrary(m_hDllJXGM);
		m_hDllJXGM = NULL;
	}
}

void CGMDebugApp::CloseConnect()
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->Close();
		m_pISystemConnect = NULL;
		if (m_hDllJXGM)
		{
			typedef HRESULT (STDAPICALLTYPE *DESTROYSYSTEMONLINE)();
			DESTROYSYSTEMONLINE fnDestroySystemOnline;
			HRESULT hr = S_OK;

			fnDestroySystemOnline = (DESTROYSYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("DestroySystemOnline"));
			VERIFY(fnDestroySystemOnline && SUCCEEDED(fnDestroySystemOnline()));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGMDebugApp object

CGMDebugApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGMDebugApp initialization

BOOL CGMDebugApp::InitInstance()
{
	char szModulePath[MAX_PATH + 1];
	::GetModuleFileName(NULL, szModulePath, (MAX_PATH + 1));

	char* pszOffset = strrchr(szModulePath, '\\');
    ASSERT(pszOffset);
    pszOffset[1] = '\0';
	
	CString strPath = szModulePath;
	ISystemOnline2* pISystemOnline = NULL;
	HRESULT hr = E_FAIL;
	CAddressDlg Adlg;
	CNameDlg Ndlg;
	CString strIni = strPath + "GMDebug.ini";

	m_hDllJXGM = ::LoadLibrary(strPath + _T("JXGM.dll"));
	if (m_hDllJXGM == NULL)
	{
		goto Error1;
	}

	typedef HRESULT (STDAPICALLTYPE *CREATESYSTEMONLINE)(ISystemOnline2**);
	CREATESYSTEMONLINE fnCreateSystemOnline;

	fnCreateSystemOnline = (CREATESYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("CreateSystemOnline2"));
	if (fnCreateSystemOnline == NULL)
	{
		goto Error1;
	}

	hr = fnCreateSystemOnline(&pISystemOnline);
	if (FAILED(hr) || pISystemOnline == NULL)
	{
		goto Error1;
	}

#ifdef _DEBUG
	GetPrivateProfileString("Main", "IP", "192.168.20.15", szModulePath, MAX_PATH, strIni);
#else
	GetPrivateProfileString("Main", "IP", "192.168.", szModulePath, MAX_PATH, strIni);
#endif //_DEBUG

	Adlg.m_strAddress = szModulePath;

	GetPrivateProfileString("Main", "Port", "7777", szModulePath, MAX_PATH, strIni);

	Adlg.m_strPort = szModulePath;

	if (Adlg.DoModal() == IDOK)
	{
		m_dwSystemIP = inet_addr(Adlg.m_strAddress);
		if (m_dwSystemIP == INADDR_NONE)
			goto Error2;
		m_nSystemPort = atoi(Adlg.m_strPort);
		if (m_nSystemPort == 0)
			goto Error2;
		if (FAILED(pISystemOnline->OpenSystemConnect2(m_dwSystemIP, m_nSystemPort, &m_pISystemConnect)) ||
			m_pISystemConnect == NULL)
		{
			goto Error2;
		}
	}
	else
		return FALSE;

	WritePrivateProfileString("Main", "IP", Adlg.m_strAddress, strIni);
	WritePrivateProfileString("Main", "Port", Adlg.m_strPort, strIni);

	GetPrivateProfileString("Main", "Account", "gm-kingsoft", szModulePath, MAX_PATH, strIni);

	Ndlg.m_strName = szModulePath;
	Ndlg.m_strPassword = "gm-kingsoft";
	if (Ndlg.DoModal() == IDOK)
	{
		hr = m_pISystemConnect->Login((char*)(const char *)Ndlg.m_strName, (char*)(const char *)Ndlg.m_strPassword, &m_Event);
		if (FAILED(hr))
		{
			goto Error3;
		}
		m_pISystemConnect->SetFlag(UNFILTER | GMLOGINOUT);
	}
	else
		return FALSE;

	WritePrivateProfileString("Main", "Account", Ndlg.m_strName, strIni);

	m_strAccount = Ndlg.m_strName;
	m_strPassword = Ndlg.m_strPassword;
	m_bConnect = FALSE;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_GMDEBUTYPE,
		RUNTIME_CLASS(CGMDebugDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CGMDebugView));
	AddDocTemplate(pDocTemplate);

	{
		// create main MDI Frame window
		CMainFrame* pMainFrame = new CMainFrame;
		if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
			return FALSE;
		m_pMainWnd = pMainFrame;
		// Parse command line for standard shell commands, DDE, file open
		CCommandLineInfo cmdInfo;
		ParseCommandLine(cmdInfo);

		// Dispatch commands specified on the command line
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;

		// The main window has been initialized, so show and update it.
		pMainFrame->ShowWindow(m_nCmdShow);
		pMainFrame->UpdateWindow();
	}

	return TRUE;

Error1:
	{
		CString str;
		str.Format("找不到运行所需文件 \"%s\" !!!", strPath + _T("JXGM.dll"));
		::AfxMessageBox(str);
	}
	return FALSE;

Error2:
	{
		CString str;
		str.Format("在地址 %s 找不到 S3ReleyServer !!!", Adlg.m_strAddress + ":" + Adlg.m_strPort);
		::AfxMessageBox(str);
	}
	return FALSE;
Error3:
	{
		::AfxMessageBox("帐户密码错, 登入系统失败!!!");
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CGMDebugApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CGMDebugApp::OnFileNew()
{
}

/////////////////////////////////////////////////////////////////////////////
// CGMDebugApp message handlers

void CGMDebugApp::NewRelay(char* szGWName, DWORD nIP)
{
	m_RelayDocCS.Lock();
	RELAYDOC doc;
	doc.strDocName = szGWName;
	doc.docIP = nIP;
	doc.nCreate = 1;
	m_Docs.AddTail(doc);
	m_RelayDocCS.Unlock();
}

void CGMDebugApp::RemoveRelay(char* szGWName, DWORD nIP)
{
	m_RelayDocCS.Lock();
	RELAYDOC doc;
	doc.strDocName = szGWName;
	doc.docIP = nIP;
	doc.nCreate = 0;
	m_Docs.AddTail(doc);
	m_RelayDocCS.Unlock();
}

void CGMDebugApp::RelayID(char* szGWName, DWORD nRelayID)
{
	m_RelayDocCS.Lock();
	RELAYDOC doc;
	doc.strDocName = szGWName;
	doc.docIP = nRelayID;
	doc.nCreate = 2;
	m_Docs.AddTail(doc);
	m_RelayDocCS.Unlock();
}

void CGMDebugApp::CloseDocs()
{
	m_RelayDocCS.Lock();
	m_bCloseDocs = TRUE;
	m_RelayDocCS.Unlock();
}

void CGMDebugApp::NewMessage(const char* szAccountFrom, const char* szRole, const char* szMessage, int nMsgLen, DWORD dwGWIP)
{
	m_RelayDataCS.Lock();
	RELAYDATA data;
	data.strAccount = szAccountFrom;
	data.strRole = szRole;
	data.docIP = dwGWIP;
	data.strMessage = szMessage;
	m_Datas.AddTail(data);
	m_RelayDataCS.Unlock();
}

void CGMDebugApp::ErrorMessage(const char* szError)
{
	m_RelayDataCS.Lock();
	m_Errors.AddTail(szError);
	m_RelayDataCS.Unlock();
}

void CGMDebugApp::SendPing()
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->SendPing();
	}
}

void CGMDebugApp::ExecuteAction(const char* szAccount, const char* szScript, DWORD nReturn)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->ExecuteAction((char*)szAccount, (char*)szScript, nReturn);
	}
}

void CGMDebugApp::ExecuteAction(const char* szRole, DWORD nRelayID, const char* szScript, DWORD nReturn)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->ExecuteAction((char*)szRole, nRelayID, (char*)szScript, nReturn);
	}
}

void CGMDebugApp::ExecuteAction(DWORD nRelayID, const char* szScript, DWORD nReturn, DWORD nSendIP)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->ExecuteAction(nRelayID, (char*)szScript, nReturn, nSendIP);
	}
}

void CGMDebugApp::FreezeAccount(const char* szAccount)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->FreezePlayerAccount(m_nOperate++, (char*)szAccount);
	}
}

void CGMDebugApp::UnlockPlayerAccount(const char* szAccount)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->UnlockPlayerAccount(m_nOperate++, (char*)szAccount);
	}
}

void CGMDebugApp::SendOnlinePlayerCount(const char* szGWName)
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->GetOnlinePlayerCount(m_nOperate++, (char*)szGWName);
	}
}

void CGMDebugApp::SendRegisterUserCount()
{
	if (m_pISystemConnect)
	{
		m_pISystemConnect->GetRegisterUserCount(m_nOperate++);
	}
}

BOOL CGMDebugApp::OnIdle(LONG lCount) 
{
	return CWinApp::OnIdle(lCount);
}

void CGMDebugApp::OnProcess()
{
	if (m_pDocManager == NULL)
		return;

	m_RelayDocCS.Lock();
	if (m_Docs.GetCount() > 0)
	{
		RELAYDOC doc = m_Docs.RemoveHead();

		if (doc.nCreate == 1)
			m_pDocManager->OnFileNew();

		POSITION posTemp = m_pDocManager->GetFirstDocTemplatePosition();
		CDocTemplate* pDocTemp = m_pDocManager->GetNextDocTemplate(posTemp);
		POSITION pos = pDocTemp->GetFirstDocPosition();
		CGMDebugDoc* pDoc = NULL;
		while (pos)
		{
			pDoc = (CGMDebugDoc*)pDocTemp->GetNextDoc(pos);
			if (pDoc)
			{
				if (pDoc->m_strDocName.IsEmpty())
				{
					ASSERT(doc.nCreate == 1);
					pDoc->m_strDocName = doc.strDocName;
					pDoc->m_docIP = doc.docIP;
					pDoc->SetTitle(doc.strDocName + ":" + _ip2a(doc.docIP));
					pDoc->Trace("Initialize()");
					break;
				}
				else if (pDoc->m_strDocName == doc.strDocName)
				{
					if (doc.nCreate == 0)
					{
						pDoc->Trace("Close()");
						pDoc->OnCloseDocument();
					}
					else if (doc.nCreate == 2)
					{
						pDoc->m_docRelayID = doc.docIP;
					}
					break;
				}
			}
		}
	}
	else if (m_bCloseDocs)
	{
		CloseAllDocuments(TRUE);
		m_bCloseDocs = FALSE;

		CloseConnect();
		m_bConnect = TRUE;

		if (AfxGetMainWnd())
			SetTimer(AfxGetMainWnd()->GetSafeHwnd(), 3, 30000, NULL);	//30秒后重连
	}
	else	//最后才处理消息到达
	{
		m_RelayDataCS.Lock();
		if (m_Datas.GetCount() > 0)
		{
			RELAYDATA data = m_Datas.RemoveHead();
			CGMDebugDoc*  pDoc = FindDoc(data.docIP);
			CString str;
			if (!data.strAccount.IsEmpty())
				str.Format("Say(\"%s\", \"%s\", \"%s\")", data.strAccount, data.strRole, data.strMessage);
			else
				str = data.strMessage;
			pDoc->Trace(str);
		}

		if (m_Errors.GetCount() > 0)
		{
			if (m_pMainWnd)
			{
				if (((CMainFrame*)m_pMainWnd)->Trace(m_Errors.GetHead()))
					m_Errors.RemoveHead();
				
			}
		}

		m_RelayDataCS.Unlock();
	}
	m_RelayDocCS.Unlock();
}

void CGMDebugApp::GetDocs(CPtrList& List)
{
	if (m_pDocManager == NULL)
		return;

	POSITION posTemp = m_pDocManager->GetFirstDocTemplatePosition();
	CDocTemplate* pDocTemp = m_pDocManager->GetNextDocTemplate(posTemp);
	POSITION pos = pDocTemp->GetFirstDocPosition();
	CGMDebugDoc* pDoc = NULL;
	while (pos)
	{
		pDoc = (CGMDebugDoc*)pDocTemp->GetNextDoc(pos);
		if (pDoc)
		{
			List.AddTail(pDoc);
		}
	}
}


CGMDebugDoc* CGMDebugApp::FindDoc(DWORD nIP)
{
	if (m_pDocManager)
	{
		POSITION posTemp = m_pDocManager->GetFirstDocTemplatePosition();
		CDocTemplate* pDocTemp = m_pDocManager->GetNextDocTemplate(posTemp);
		POSITION pos = pDocTemp->GetFirstDocPosition();
		CGMDebugDoc* pDoc = NULL;
		while (pos)
		{
			pDoc = (CGMDebugDoc*)pDocTemp->GetNextDoc(pos);
			if (pDoc && pDoc->m_docIP == nIP)
			{
				return pDoc;
			}
		}
	}
	return NULL;
}


void CGMDebugApp::OnSelfinfo() 
{
	if (m_pISystemConnect)
	{
		CString str;
		str.Format("%s:%d", _ip2a(m_pISystemConnect->GetIP()),
			m_pISystemConnect->GetRelayID());
		ErrorMessage(str);
	}
}

void CGMDebugApp::OnAutorun() 
{
	if (m_pISystemConnect)
	{
		m_bAuto = !m_bAuto;
		if (m_bAuto)
			m_pISystemConnect->SetFlag(UNFILTER | GMLOGINOUT | GMAUTORETURN);
		else
			m_pISystemConnect->SetFlag(UNFILTER | GMLOGINOUT);
	}

	CMenu* mmenu = AfxGetMainWnd()->GetMenu();
	CMenu* submenu = mmenu->GetSubMenu(0);
	if (m_bAuto)
		submenu->CheckMenuItem(IDM_AUTORUN, MF_CHECKED | MF_BYCOMMAND);
	else
		submenu->CheckMenuItem(IDM_AUTORUN, MF_UNCHECKED | MF_BYCOMMAND);
}

void CGMDebugApp::ConnectAndLogin()
{
	if (!m_bConnect)
		return;

	if (m_pISystemConnect)
		CloseConnect();

	typedef HRESULT (STDAPICALLTYPE *CREATESYSTEMONLINE)(ISystemOnline2**);
	CREATESYSTEMONLINE fnCreateSystemOnline;
	ISystemOnline2* pISystemOnline = NULL;
	ISystemConnect2* pISystemConnect = NULL;
	HRESULT hr;

	fnCreateSystemOnline = (CREATESYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("CreateSystemOnline2"));
	if (fnCreateSystemOnline == NULL)
	{
		goto ErrorExit;
	}

	hr = fnCreateSystemOnline(&pISystemOnline);
	if (FAILED(hr) || pISystemOnline == NULL)
	{
		goto ErrorExit;
	}

	if (FAILED(pISystemOnline->OpenSystemConnect2(m_dwSystemIP, m_nSystemPort, &pISystemConnect)) ||
		pISystemConnect == NULL)
	{
		goto ErrorExit;
	}

	hr = pISystemConnect->Login((char*)(const char *)m_strAccount, (char*)(const char *)m_strPassword, &m_Event);
	if (FAILED(hr))
	{
		goto ErrorExit;
	}

	m_pISystemConnect = pISystemConnect;

	if (m_bAuto)
		m_pISystemConnect->SetFlag(UNFILTER | GMLOGINOUT | GMAUTORETURN);
	else
		m_pISystemConnect->SetFlag(UNFILTER | GMLOGINOUT);
	m_bConnect = FALSE;
	if (AfxGetMainWnd())
		KillTimer(AfxGetMainWnd()->GetSafeHwnd(), 3);

	return;

ErrorExit:
	if (pISystemConnect)
		pISystemConnect->Close();

	if (m_hDllJXGM)
	{
		typedef HRESULT (STDAPICALLTYPE *DESTROYSYSTEMONLINE)();
		DESTROYSYSTEMONLINE fnDestroySystemOnline;
		HRESULT hr = S_OK;

		fnDestroySystemOnline = (DESTROYSYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("DestroySystemOnline"));
		VERIFY(fnDestroySystemOnline && SUCCEEDED(fnDestroySystemOnline()));
	}
}
