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
#include "../GMPassGen/Include/PassGen.h"

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
//	m_pGMEventProxy		= NULL;
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
	m_strRoleName = _T("");
	m_strPlayerPos = _T("");
	m_strAllChatMsgs = _T("");
	m_strMsgToBeSent = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

	m_nBadPingCount		= 0;
	m_dwGMTimerTick		= 0;
	m_nInfoIndex		= 0;
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
	DDX_Text(pDX, IDC_ROLE_NAME, m_strRoleName);
	DDX_Text(pDX, IDC_PLAYER_POSITION, m_strPlayerPos);
	DDX_Text(pDX, IDC_CHAT_BOX, m_strAllChatMsgs);
	DDX_Text(pDX, IDC_CHAT_MSG, m_strMsgToBeSent);
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
	ON_MESSAGE(WM_JX_SUB_LOGOUT, OnSubLogout)
	ON_MESSAGE(WM_JX_SYS_DISCONNCECT, OnSystemDisconnect)
	ON_MESSAGE(WM_JX_JAIL_JUDGE, OnJailJudge)
	ON_MESSAGE(WM_JX_DO_SCRIPT, OnDoScript)
	ON_MESSAGE(WM_JX_SYS_PING, OnSystemPing)
	ON_MESSAGE(WM_JX_CHATING, OnChat)
	ON_BN_CLICKED(IDC_PING_SERVER, OnPingServer)
	ON_BN_CLICKED(IDC_GET_GW_NAME, OnGetGwName)
	ON_BN_CLICKED(IDC_GET_GW_IP, OnGetGwIp)
	ON_BN_CLICKED(IDC_GET_TOTAL_ONLINE, OnGetTotalOnline)
	ON_BN_CLICKED(IDC_EXECUTE, OnExecute)
	ON_BN_CLICKED(IDC_FIND_PLAYER2, OnFindPlayer2)
	ON_BN_CLICKED(IDC_KICK_OFF, OnKickOff)
	ON_BN_CLICKED(IDC_IMPRISON, OnImprison)
	ON_BN_CLICKED(IDC_SET_FREE, OnSetFree)
	ON_BN_CLICKED(IDC_SEND_CHAT, OnSendChat)
	ON_BN_CLICKED(IDC_GET_ROLE_LIST, OnGetRoleList)
	ON_BN_CLICKED(IDC_BROADCAST, OnBroadcast)
	ON_BN_CLICKED(IDC_GET_POS_FROM_DB, OnGetPosFromDb)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

extern CString gINI_FILENAME;

/////////////////////////////////////////////////////////////////////////////
// CGMDlg message handlers
#define GM_AUTO_REPLY "(GM自动回复)游戏管理员已经收到您的投诉，请耐心等候处理......"
char g_szGMReturnMessage[256] = GM_AUTO_REPLY;
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

#ifndef _DEBUG
	DisableDialog();
#endif
	GetPrivateProfileString("System", "GMAutoReply",
					  GM_AUTO_REPLY,	//"gm-kingsoft"
					  g_szGMReturnMessage,
					  sizeof(g_szGMReturnMessage),
					  gINI_FILENAME);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGMDlg::DisableDialog()
{
	for (int i = IDC_REGISTER_COUNT; i <= IDC_BROADCAST; i++)
	{
		CWnd* pWnd = GetDlgItem(i);
		if (pWnd)
			pWnd->EnableWindow(FALSE);
	}
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
	m_pGMCoreRef->UnlockPlayerAccount(m_strAccount, m_strPassword);
}

void CGMDlg::OnGetRegister() 
{
	UpdateData();
	ASSERT(m_pGMCoreRef);
	m_nRegisterCount = m_pGMCoreRef->GetRegisterCount();
	UpdateData(FALSE);
}

void CGMDlg::OnGetOnline() 
{
	UpdateData();
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
	HRESULT hr = S_OK;
	GAMEWORLD gw;

	ASSERT(m_pGMCoreRef);
	UpdateData();
	if (0 == m_strAccount.GetLength())
		return;
	hr = m_pGMCoreRef->WhichGWIsPlayerIn(gw, m_strAccount);
	if (FAILED(hr))
	{
		ShowInfoString("在游戏世界里寻找玩家失败 !!!");
		goto KS_EXIT;
	}

	if (gw.strGameWorldName.GetLength())
	{
		m_lstboxGameWorlds.SelectString(0, gw.strGameWorldName);
		CString str;
		str.Format("使用账号\"%s\"的玩家正在游戏世界\"%s\"里.\n(注：使用的是快速查找，不能保证完全正确)", m_strAccount, gw.strGameWorldName);
		ShowInfoString(str);
	}
	else
	{
		if (gw.dwIP == 0)
			ShowInfoString("该用户不在线 ！！");
		else
			ShowInfoString("该账号不存在 ！！！");
	}

KS_EXIT:
	UpdateData(FALSE);
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


void CGMDlg::OnSystemDisconnect(WPARAM wParam, LPARAM lParam)
{
	m_pGMCoreRef->DisconnectionOccur();
	ShowInfoString("GM系统和中转服务器失去联系 ！！！\n请检查线路后重新启动GM.exe.");
#ifdef _DEBUG
	DisableDialog();
#endif
}

void CGMDlg::OnJailJudge(WPARAM wParam, LPARAM lParam)
{
	BSTR bstrAccountName = (BSTR) lParam;
	CString strAccount = bstrAccountName;
	::SysFreeString(bstrAccountName);
	int nSplit = strAccount.Find('\n');
	int nSplit1 = strAccount.Find('\n', nSplit + 1);
	if (nSplit >= 0 && nSplit1 > nSplit)
		m_pGMCoreRef->JailJudge(strAccount.Left(nSplit), strAccount.Mid(nSplit + 1, nSplit1 - nSplit - 1), strAccount.Mid(nSplit1 + 1));
}

void CGMDlg::OnDoScript(WPARAM wParam, LPARAM lParam)
{
	BSTR bstrAccountName = (BSTR)lParam;
	CString strAccount = bstrAccountName;
	::SysFreeString(bstrAccountName);
	int nSplit = strAccount.Find('\n');
	if (-1 != nSplit)
		m_pGMCoreRef->ExecutePlayer(strAccount.Left(nSplit), strAccount.Mid(nSplit + 1));
}

void CGMDlg::OnSystemPing(WPARAM wParam, LPARAM lParam)
{
	m_nBadPingCount = 0;	//ping 失败计数清零
}

void CGMDlg::OnChat(WPARAM wParam, LPARAM lParam)
{
	UpdateData();
	ASSERT(wParam == 0);
	if (!lParam)
		return;
	BSTR bstrChat = (BSTR) lParam;
	CString strChat = bstrChat;
	::SysFreeString(bstrChat);
	int nSplit = strChat.Find('\n');
	int	nSplit2 = strChat.Find('\n', nSplit + 1);
	int	nSplit3 = strChat.Find('\n', nSplit2 + 1);
	if (nSplit >= 0 && nSplit2 > nSplit && nSplit3 > nSplit2)
	{
		LPSTR lpChat = (LPSTR)(LPCSTR)strChat;
		lpChat[nSplit] = 0;
		lpChat[nSplit2] = 0;
		lpChat[nSplit3] = 0;

		//执行GM指令
#ifdef _DEBUG
		if (strChat.Find("?gm ", nSplit3 + 1) == nSplit3 + 1)
		{
			m_pGMCoreRef->ExecutePlayer(strChat, ((LPCSTR)strChat) + nSplit3 + 1 + 4);
			return;
		}
#endif

		m_pGMCoreRef->SaveMessageToDB(strChat, ((LPCSTR)strChat) + nSplit + 1,
							((LPCSTR)strChat) + nSplit2 + 1, ((LPCSTR)strChat) + nSplit3 + 1);
#ifdef _DEBUG
		if (m_strAllChatMsgs.GetLength())
			m_strAllChatMsgs += "\r\n";
		m_strAllChatMsgs += "[";
		m_strAllChatMsgs += lpChat + nSplit2 + 1;	//gameworld
		m_strAllChatMsgs += "] ";
		m_strAllChatMsgs += lpChat;					//accountname
		m_strAllChatMsgs += "(";
		m_strAllChatMsgs += lpChat + nSplit + 1;	//rolename
		m_strAllChatMsgs += "): ";
		m_strAllChatMsgs += lpChat + nSplit3 + 1;	//chat message
#endif
		//自动回复给玩家
		if (strlen(g_szGMReturnMessage))
			m_pGMCoreRef->SendMessage((LPCSTR)strChat, (LPCSTR)(lpChat + nSplit + 1), g_szGMReturnMessage);

		strChat.SetAt(nSplit, '\n');
		strChat.SetAt(nSplit2, '\n');
		strChat.SetAt(nSplit3, '\n');
	}
	UpdateData(FALSE);
}

inline char* _ip2a(DWORD ip) { in_addr ia; ia.s_addr = ip; return inet_ntoa(ia); }

void CGMDlg::OnSubLogout(WPARAM wParam, LPARAM lParam)
{
	if (!lParam || !wParam)
	{
		ASSERT(0);
		return;
	}
	BSTR bstrGWName = (BSTR) lParam;
	CString strGWName = bstrGWName;
	::SysFreeString(bstrGWName);
	m_pGMCoreRef->SaveDisconnectionToDB(strGWName, _ip2a(wParam));

	CString strOut;
	strOut.Format("游戏世界\"%s\"的服务器组中IP地址为\"%s\"的机器断开连接。\n有可能是出现故障，请注意！！", strGWName, _ip2a(wParam));
	ShowInfoString(strOut);
}

void CGMDlg::OnGWLoginOut(WPARAM wParam, LPARAM lParam)
{
	ASSERT(wParam == 0);
	if (!lParam)
		return;
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
	if (!lParam)
		return;
	BSTR bstrGWName = (BSTR) lParam;
	CString strGW = bstrGWName;
	::SysFreeString(bstrGWName);

	VERIFY(m_pGMCoreRef->InitGameWorld(strGW));
	m_lstboxGameWorlds.AddString(strGW);
	if (m_lstboxGameWorlds.GetCount() == 1)
		m_lstboxGameWorlds.SelectString(0,strGW);
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
		ShowInfoString("请选择一个游戏世界.....");
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
			ShowInfoString("指定的游戏世界不存在?@!#%!#");
		UpdateData(FALSE);
	}
}

void CGMDlg::OnGetTotalOnline() 
{
	UpdateData();
	m_nTotalOnline = m_pGMCoreRef->GetOnlineCount("");
	UpdateData(FALSE);
}

void CGMDlg::OnExecute() 
{
	UpdateData();
	m_pGMCoreRef->ExecutePlayer(m_strAccount, m_strScript);	
}

void CGMDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	//CDialog::OnOK();
}

void CGMDlg::OnFindPlayer2() 
{
	HRESULT hr = S_OK;
	GAMEWORLD gw;

	ASSERT(m_pGMCoreRef);
	UpdateData();
	if (0 == m_strAccount.GetLength())
		return;
	int nX = -1, nY = -1, nWorld = -1;
	hr = m_pGMCoreRef->WhichGWIsPlayerInRealTime(gw, m_strRoleName, nX, nY, nWorld, m_strAccount);
	m_strPlayerPos.Format("[%d]: %d, %d", nWorld, nX, nY);
	if (FAILED(hr))
	{
		ShowInfoString("在游戏世界里*实时*寻找玩家失败 !!!");
		goto KS_EXIT;
	}

	if (gw.strGameWorldName.GetLength())
	{
		m_lstboxGameWorlds.SelectString(0, gw.strGameWorldName);
		CString str;
		str.Format("使用账号\"%s\"的玩家正在游戏世界\"%s\"里.(实时)", m_strAccount, gw.strGameWorldName);
		ShowInfoString(str);
	}
	else
	{
		if (gw.dwIP == 0)
			ShowInfoString("该用户不在线 ！！");
		else
			ShowInfoString("该账号不存在 ！！！");
	}

KS_EXIT:
	UpdateData(FALSE);

}

void CGMDlg::ShowInfoString(LPCSTR szInfo)
{
	CString str;
	str.Format("[%04d]:", m_nInfoIndex);
	GetDlgItem(IDC_INFO_INDEX)->SetWindowText(str);
	GetDlgItem(IDC_STATUS_INFO)->SetWindowText(szInfo);
	m_nInfoIndex++;
}

void CGMDlg::RefreshOnlineCount()
{
	GAMEWORLD gw;
	for (int i = 0, nSize = m_pGMCoreRef->CountOfGameWorlds(); i < nSize; i++)
	{
		if (m_pGMCoreRef->GetGWByIndex(gw, i))
			m_pGMCoreRef->GetOnlineCount(gw.strGameWorldName);
	}
	m_pGMCoreRef->GetOnlineCount("");
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

	m_dwGMTimerTick ++;

	if (!m_pGMCoreRef || !m_pGMCoreRef->IsConnectionOK())
		goto KS_EXIT;

	//保持定时 Ping
	if (IS_GM_INTERCOUNT_REACHED(m_dwGMTimerTick, GM_INTERCOUNT_PING))
	{
		m_pGMCoreRef->JustPingSystem();
		//m_nBadPingCount++;
	}

	//发现 ping 失败，追加 ping
	//因为网络底层保证连接，所以这里去掉 [wxb 2003-7-30]
/*	if (IS_GM_INTERCOUNT_REACHED_ADD(m_dwGMTimerTick, GM_INTERCOUNT_PING,
				(PING_TIMEOUT / GM_TIMER_INTERVAL) * m_nBadPingCount) &&
		m_nBadPingCount > 0)
	{
		if (m_nBadPingCount > PING_MAX_RETRY)
		{
			PostMessage(WM_JX_SYS_DISCONNCECT, 0, 0);	//断线了
			goto KS_EXIT;
		}
		else
		{
			m_pGMCoreRef->JustPingSystem();
			m_nBadPingCount++;
		}
	}*/

	//定时更新注册人数
	if (IS_GM_INTERCOUNT_REACHED(m_dwGMTimerTick, GM_INTERCOUNT_REGISTER))
		m_pGMCoreRef->GetRegisterCount();

	//定时更新在线人数
	if (IS_GM_INTERCOUNT_REACHED(m_dwGMTimerTick, GM_INTERCOUNT_ONLINE))
		RefreshOnlineCount();

	//定时操作（天牢/禁言等）
	m_pGMCoreRef->Breathe(m_dwGMTimerTick);
KS_EXIT:
	CDialog::OnTimer(nIDEvent);
}


void CGMDlg::OnKickOff() 
{
	UpdateData();
	m_pGMCoreRef->ExecutePlayer(m_strAccount, "ds KickOutSelf()");
}

void CGMDlg::OnImprison() 
{
	UpdateData();
	m_pGMCoreRef->PutInJail(m_strAccount, m_strRoleName, m_strGWName, 2);
}

void CGMDlg::OnSetFree()
{
	UpdateData();
	m_pGMCoreRef->FreeFromJail(m_strAccount, m_strRoleName, m_strGWName);
}

void CGMDlg::OnSendChat()
{
	UpdateData();
	if (m_strMsgToBeSent.GetLength())
	{
		HRESULT hr = 
			m_pGMCoreRef->SendMessage(m_strAccount, m_strRoleName, m_strMsgToBeSent);
#ifdef _DEBUG
		if (SUCCEEDED(hr))
		{
			if (m_strAllChatMsgs.GetLength())
				m_strAllChatMsgs += "\r\n";
			m_strAllChatMsgs += "{GM}: ";
			m_strAllChatMsgs += m_strMsgToBeSent;
		}
#endif
		if (FAILED(hr))	
			ShowInfoString("广播操作失败！！");
	}
	UpdateData(FALSE);
}

void CGMDlg::OnGetRoleList() 
{
	UpdateData();
	CStringList lstRoles;
	HRESULT hr = m_pGMCoreRef->GetRoleList(m_strAccount, m_strGWName, lstRoles);
	CString str;

	if (FAILED(hr))
	{
		str.Format("在游戏世界\"%s\"查询账号\"%s\"的角色失败 ！！", m_strGWName, m_strAccount);
	}
	else
	{
		str.Format("在游戏世界\"%s\"中属于账号\"%s\"的角色如下：\n", m_strGWName, m_strAccount);
		if (lstRoles.GetCount() == 0)
			str += "该账号从未进入过该游戏世界，没有任何角色。";

		for (POSITION pos = lstRoles.GetHeadPosition(); pos; )
		{
			str += lstRoles.GetNext(pos);
			if (pos)
				str += "       ";
		}
	}
	ShowInfoString(str);
}

void CGMDlg::OnBroadcast() 
{
	UpdateData();

	HRESULT hr = m_pGMCoreRef->BroadCastChatToGameWorlds(m_strGWName, m_strMsgToBeSent);
#ifdef _DEBUG
	if (SUCCEEDED(hr))
	{
		if (m_strAllChatMsgs.GetLength())
			m_strAllChatMsgs += "\r\n";
		if (m_strGWName.GetLength())
			m_strAllChatMsgs += "{系统公告}: ";
		else
		{
			CString str;
			str.Format("{游戏世界<%s>公告}", m_strGWName);
			m_strAllChatMsgs += str;
		}
		m_strAllChatMsgs += m_strMsgToBeSent;
	}
#endif
	if (FAILED(hr))	
		ShowInfoString("广播操作失败！！");
}

void CGMDlg::OnGetPosFromDb() 
{
	UpdateData();
	int nX = -1, nY = -1, nWorld = -1;

	if (m_pGMCoreRef->GetPlayerPosInDB(m_strRoleName, m_strGWName, nWorld, nX, nY))
		m_strPlayerPos.Format("[%d]: %d, %d", nWorld, nX, nY);
	else
		m_strPlayerPos = "获取失败!!!";
	UpdateData(FALSE);
}
