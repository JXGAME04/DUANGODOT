//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMDlg.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 14:59:49
//	文件说明		:	测试对话框类的实现
//////////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "GM.h"
#include "GMDlg.h"
#include "GMCore.h"
#include "GMConsts.h"
#include "WinSock.h"
#include "GMMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

/////////////////////////////////////////////////////////////////////////////
// CGMDlg dialog

CGMDlg::CGMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGMDlg::IDD, pParent)
{
	m_pGMCoreRef		= NULL;
	m_pGMEventProxy		= NULL;
	//{{AFX_DATA_INIT(CGMDlg)
	m_nOnlineCount = 0;
	m_nRegisterCount = 0;
	m_strIPAddress = _T("");
	m_strGWName = _T("");
	m_strAccount = _T("");
	m_strPassword = _T("");
	m_nPingOfSystem = 0;
	m_nTotalOnline = 0;
	m_strScript = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CGMDlg::~CGMDlg()
{
}

void CGMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGMDlg)
	DDX_Control(pDX, IDC_GAME_WORLD_LIST, m_lstboxGameWorlds);
	DDX_Text(pDX, IDC_ONLINE_COUNT, m_nOnlineCount);
	DDX_Text(pDX, IDC_REGISTER_COUNT, m_nRegisterCount);
	DDX_Text(pDX, IDC_IP_ADDRESS, m_strIPAddress);
	DDX_Text(pDX, IDC_GW_NAME, m_strGWName);
	DDX_Text(pDX, IDC_ACCOUNT, m_strAccount);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_SYS_PING, m_nPingOfSystem);
	DDX_Text(pDX, IDC_TOTAL_ONLINE_COUNT, m_nTotalOnline);
	DDX_Text(pDX, IDC_SCRIPT, m_strScript);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGMDlg, CDialog)
	//{{AFX_MSG_MAP(CGMDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_UNLOCK_PLAYER, OnUnlockPlayer)
	ON_BN_CLICKED(IDC_GET_REGISTER, OnGetRegister)
	ON_BN_CLICKED(IDC_GET_ONLINE, OnGetOnline)
	ON_BN_CLICKED(IDC_FIND_PLAYER, OnFindPlayer)
	ON_WM_TIMER()
	ON_MESSAGE(WM_JX_GW_LOGIN, OnGWLoginIn)
	ON_MESSAGE(WM_JX_GW_LOGOFF, OnGWLoginOut)
	ON_BN_CLICKED(IDC_PING_SERVER, OnPingServer)
	ON_BN_CLICKED(IDC_GET_GW_NAME, OnGetGwName)
	ON_BN_CLICKED(IDC_GET_GW_IP, OnGetGwIp)
	ON_BN_CLICKED(IDC_GET_TOTAL_ONLINE, OnGetTotalOnline)
	ON_BN_CLICKED(IDC_EXECUTE, OnExecute)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMDlg message handlers

BOOL CGMDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	SetTimer(GM_TIMER, GM_TIMER_INTERVAL, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGMDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGMDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGMDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CGMDlg::OnUnlockPlayer() 
{
	ASSERT(m_pGMCoreRef);
	UpdateData();
	m_pGMCoreRef->UnlockPlayerAccount(m_strAccount, m_strPassword);
}

void CGMDlg::OnGetRegister() 
{
	ASSERT(m_pGMCoreRef);
	m_nRegisterCount = m_pGMCoreRef->GetRegisterCount();
	UpdateData(FALSE);
}

void CGMDlg::OnGetOnline() 
{
	ASSERT(m_pGMCoreRef);
	OnGetGwName();
	if (LB_ERR != m_lstboxGameWorlds.GetCurSel())
	{
		m_nOnlineCount = m_pGMCoreRef->GetOnlineCount(m_strGWName);
		UpdateData(FALSE);
	}
}

void CGMDlg::OnFindPlayer()
{
	ShowWindow(SW_HIDE);
	HRESULT hr = S_OK;
	GAMEWORLD gw;

	ASSERT(m_pGMCoreRef);
	UpdateData();
	hr = m_pGMCoreRef->WhichGWIsPlayerIn(gw, m_strAccount);
	KS_CHECK(hr);

	m_lstboxGameWorlds.SelectString(0, gw.strGameWorldName);

KS_EXIT:
	UpdateData(FALSE);
}

void CGMDlg::OnTimer(UINT nIDEvent) 
{
/*	//测试 VBA Event 链接
	static nCount = 0;
	if (m_pGMEventProxy)
	{
		CString str;
		str.Format("%d", nCount++);
		HRESULT hr = m_pGMEventProxy->Fire_Alert(CComBSTR(str));
		ASSERT(!SUCCEEDED(hr));
	}*/
	
	CDialog::OnTimer(nIDEvent);
}


BOOL AFXAPI AfxUnhookWindowCreate();
void AFXAPI AfxHookWindowCreate(CWnd* pWnd);


int CGMDlg::DoModal2(BOOL bShow) 
{
	// can be constructed with a resource template or InitModalIndirect
	ASSERT(m_lpszTemplateName != NULL || m_hDialogTemplate != NULL ||
		m_lpDialogTemplate != NULL);

	// load resource as necessary
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();
	if (m_lpszTemplateName != NULL)
	{
		hInst = AfxFindResourceHandle(m_lpszTemplateName, RT_DIALOG);
		HRSRC hResource = ::FindResource(hInst, m_lpszTemplateName, RT_DIALOG);
		hDialogTemplate = LoadResource(hInst, hResource);
	}
	if (hDialogTemplate != NULL)
		lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);

	// return -1 in case of failure to load the dialog template resource
	if (lpDialogTemplate == NULL)
		return -1;

	// disable parent (before creating dialog)
	HWND hWndParent = PreModal();
	AfxUnhookWindowCreate();
	BOOL bEnableParent = FALSE;
	if (hWndParent != NULL && ::IsWindowEnabled(hWndParent))
	{
		::EnableWindow(hWndParent, FALSE);
		bEnableParent = TRUE;
	}

	TRY
	{
		// create modeless dialog
		AfxHookWindowCreate(this);
		if (CreateDlgIndirect(lpDialogTemplate,
						CWnd::FromHandle(hWndParent), hInst))
		{
			if (m_nFlags & WF_CONTINUEMODAL)
			{
				// enter modal loop
				DWORD dwFlags = bShow ? MLF_SHOWONIDLE : 0;
				if (GetStyle() & DS_NOIDLEMSG)
					dwFlags |= MLF_NOIDLEMSG;
				ShowWindow(bShow ? SW_SHOW : SW_HIDE);
				VERIFY(RunModalLoop(dwFlags) == m_nModalResult);
			}

			// hide the window before enabling the parent, etc.
			if (m_hWnd != NULL)
				SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
					SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
		}
	}
	CATCH_ALL(e)
	{
		DELETE_EXCEPTION(e);
		m_nModalResult = -1;
	}
	END_CATCH_ALL

	if (bEnableParent)
		::EnableWindow(hWndParent, TRUE);
	if (hWndParent != NULL && ::GetActiveWindow() == m_hWnd)
		::SetActiveWindow(hWndParent);

	// destroy modal window
	DestroyWindow();
	PostModal();

	// unlock/free resources as necessary
	if (m_lpszTemplateName != NULL || m_hDialogTemplate != NULL)
		UnlockResource(hDialogTemplate);
	if (m_lpszTemplateName != NULL)
		FreeResource(hDialogTemplate);

	return m_nModalResult;
}


void CGMDlg::OnGWLoginOut(WPARAM wParam, LPARAM lParam)
{
	ASSERT(wParam == 0);
	BSTR bstrGWName = (BSTR) lParam;
	CString strGW = bstrGWName;
	::SysFreeString(bstrGWName);

	int i = m_lstboxGameWorlds.FindString(0, strGW);
	if (LB_ERR != i)
		m_lstboxGameWorlds.DeleteString(i);
}

void CGMDlg::OnGWLoginIn(WPARAM wParam, LPARAM lParam)
{
	ASSERT(wParam == 0);
	BSTR bstrGWName = (BSTR) lParam;
	CString strGW = bstrGWName;
	::SysFreeString(bstrGWName);

	VERIFY(m_pGMCoreRef->InitGameWorld(strGW));
	m_lstboxGameWorlds.AddString(strGW);
}

void CGMDlg::OnPingServer() 
{
	m_nPingOfSystem = m_pGMCoreRef->PingSystem();
	UpdateData(NULL);
}

void CGMDlg::OnGetGwName()
{
	int nSel = m_lstboxGameWorlds.GetCurSel();
	if (LB_ERR != nSel)
		m_lstboxGameWorlds.GetText(nSel, m_strGWName);
	else
		m_strGWName = "please select a GameWorld";
	UpdateData(FALSE);
}

void CGMDlg::OnGetGwIp() 
{
	ASSERT(m_pGMCoreRef);
	OnGetGwName();
	if (LB_ERR != m_lstboxGameWorlds.GetCurSel())
	{
		GAMEWORLD gw;
		if (m_pGMCoreRef->GetGWByName(gw, m_strGWName))
		{
			in_addr addr;
			addr.S_un.S_addr = gw.dwIP;
			m_strIPAddress = inet_ntoa(addr);
		}
		else
			m_strGWName = "GW Not found ?!?>>!@$!@$";
		UpdateData(FALSE);
	}
}

void CGMDlg::OnGetTotalOnline() 
{
	m_nTotalOnline = m_pGMCoreRef->GetOnlineCount("");
	UpdateData(FALSE);
}

void CGMDlg::OnExecute() 
{
	UpdateData();
	m_pGMCoreRef->ExecutePlayer(m_strAccount, m_strScript);	
}
