// NetClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NetClient.h"
#include "NetClientDlg.h"
#include "..\cWorld.h"

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
// CNetClientDlg dialog

CNetClientDlg::CNetClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNetClientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetClientDlg)
	m_strChat = _T("");
	m_nPort = c_nPort;
	m_strServer = _T("yujiang");
	m_strName = _T("yujiang");
	m_strPassword = _T("1111");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNetClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetClientDlg)
	DDX_Control(pDX, IDC_LIST_CHAT, m_listChat);
	DDX_Text(pDX, IDC_EDIT_CHAT, m_strChat);
	DDV_MaxChars(pDX, m_strChat, 127);
	DDX_Text(pDX, IDC_SERVERPORT, m_nPort);
	DDX_Text(pDX, IDC_SERVERNAME, m_strServer);
	DDX_Text(pDX, IDC_NAME, m_strName);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNetClientDlg, CDialog)
	//{{AFX_MSG_MAP(CNetClientDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_QUIT, OnQuit)
	ON_BN_CLICKED(IDC_VERIFY, OnVerify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetClientDlg message handlers
void CNetClientDlg::EnableNetStatus(NetStatus e,BOOL b)
{
	switch (e)
	{
	case NetStatus_Connect:
		GetDlgItem(IDC_CONNECT)->EnableWindow(b);
		break;
	case NetStatus_Send:
		GetDlgItem(IDC_SEND)->EnableWindow(b);
		break;
	case NetStatus_Verify:
		GetDlgItem(IDC_VERIFY)->EnableWindow(b);
		break;
	case NetStatus_Quit:
		GetDlgItem(IDC_QUIT)->EnableWindow(b);
		break;
	}
}

BOOL CNetClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.
	CString strItem1= _T("Player");
	CString strItem2= _T("Message");
	
	int statusWidth = 80;
	CRect rect;
	GetWindowRect(&rect);
	int rt = m_listChat.InsertColumn(0, strItem1, LVCFMT_LEFT,
		statusWidth,0);
	ASSERT(rt!=-1);
	rt = m_listChat.InsertColumn(1, strItem2, LVCFMT_LEFT,
		rect.Width() - statusWidth - 10,1);
	ASSERT(rt!=-1);

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
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNetClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CNetClientDlg::OnPaint() 
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
HCURSOR CNetClientDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CNetClientDlg::AddMessage(LPCSTR szMessage)
{
	int n = m_listChat.GetItemCount();
	
	LVITEM item;
	item.iItem = 0;
	item.iSubItem = 0;
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	char buffer[128];
	strcpy(buffer,szMessage);
	item.pszText = buffer;
	item.iImage = 0;
	item.state = LVIS_SELECTED | LVIS_FOCUSED;
	int iA;
	iA = m_listChat.InsertItem(&item);	
	ASSERT(iA != - 1);
}

void CNetClientDlg::AddString(LPCSTR szAccount,LPCSTR szChat)
{
	int n = m_listChat.GetItemCount();
	
	LVITEM item;
	item.iItem = 0;
	item.iSubItem = 0;
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	char buffer[128];
	strcpy(buffer,szAccount);
	item.pszText = buffer;
	item.iImage = 0;
	item.state = LVIS_SELECTED | LVIS_FOCUSED;
	int iA;
	iA = m_listChat.InsertItem(&item);	
	//SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ASSERT(iA != - 1);
	strcpy(buffer,szChat);
	item.iItem = iA;
	item.iSubItem = 1;
	item.mask = LVIF_TEXT;
	BOOL b = m_listChat.SetItem(&item);	
	ASSERT(b != 0);
}

void CNetClientDlg::OnConnect() 
{
	// TODO: Add your control notification handler code here
	EnableNetStatus(NetStatus_Connect,FALSE);
	EnableNetStatus(NetStatus_Verify,FALSE);
	EnableNetStatus(NetStatus_Send,FALSE);
	EnableNetStatus(NetStatus_Quit,FALSE);

	UpdateData();
	if (m_nPort == 0 || m_strServer.IsEmpty())
	{
		return;
	}
	stServerInfo info;
	strcpy(info.strServerIP , m_strServer);
	info.wServerPort = m_nPort;
	AppGetClient()->Connect(info);
}

void CNetClientDlg::OnQuit() 
{
	EnableNetStatus(NetStatus_Connect,FALSE);
	EnableNetStatus(NetStatus_Verify,FALSE);
	EnableNetStatus(NetStatus_Send,FALSE);
	EnableNetStatus(NetStatus_Quit,FALSE);

	theApp.OnQuit();
}

void CNetClientDlg::OnVerify() 
{
	EnableNetStatus(NetStatus_Connect,FALSE);
	EnableNetStatus(NetStatus_Verify,FALSE);
	EnableNetStatus(NetStatus_Send,FALSE);
	EnableNetStatus(NetStatus_Quit,FALSE);

	if (m_strName.IsEmpty() || m_strPassword.IsEmpty())
		return;
	stMsg_c2s_Account msg;
	strcpy(msg.szAccount,m_strName);
	strcpy(msg.szPassword,m_strPassword);
	AppGetClient()->SendMessage(&msg,sizeof(msg));

	AppGetWorld()->SetWaitingAnswer(TRUE);
}

void CNetClientDlg::OnSend() 
{
	UpdateData();
	if (m_strChat.IsEmpty())
		return;
	if (m_strChat.GetLength() >= MAX_WORD-1)
		return;
	stMsg_c2s_Chat msg;
	msg.szAccount[0] = 0;
	strcpy(msg.szChat,m_strChat);
	AppGetClient()->SendMessage(&msg,sizeof(msg));
}

void CNetClientDlg::OnOK() 
{
	// TODO: Add extra validation here
	OnSend();
	//CDialog::OnOK();
}
