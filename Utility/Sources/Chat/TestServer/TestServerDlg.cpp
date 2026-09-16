// TestServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestServer.h"
#include "TestServerDlg.h"

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
// CTestServerDlg dialog

CTestServerDlg::CTestServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTestServerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestServerDlg)
	DDX_Control(pDX, IDC_LIST_MESSAGE, m_listMessage);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTestServerDlg, CDialog)
	//{{AFX_MSG_MAP(CTestServerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestServerDlg message handlers

BOOL CTestServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strItem1= _T("Player");
	CString strItem2= _T("Message");
	
	int statusWidth = 80;
	CRect rect;
	GetWindowRect(&rect);
	int rt = m_listMessage.InsertColumn(0, strItem1, LVCFMT_LEFT,
		statusWidth,0);
	ASSERT(rt!=-1);
	rt = m_listMessage.InsertColumn(1, strItem2, LVCFMT_LEFT,
		rect.Width() - statusWidth - 10,1);
	ASSERT(rt!=-1);

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
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTestServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTestServerDlg::OnPaint() 
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
HCURSOR CTestServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTestServerDlg::AddString(LPCSTR szAccount, LPCSTR szChat)
{
	int n = m_listMessage.GetItemCount();
	
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
	iA = m_listMessage.InsertItem(&item);	
	//SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ASSERT(iA != - 1);
	strcpy(buffer,szChat);
	item.iItem = iA;
	item.iSubItem = 1;
	item.mask = LVIF_TEXT;
	BOOL b = m_listMessage.SetItem(&item);	
	ASSERT(b != 0);

}

void CTestServerDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	//CDialog::OnOK();
}
