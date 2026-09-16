// AutoupdateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Autoupdate.h"
#include "AutoupdateDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
const int TIMER_REFRESH = 1001;

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
// CAutoupdateDlg dialog

CAutoupdateDlg::CAutoupdateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoupdateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutoupdateDlg)
	m_strVerson = _T("");
	m_strProgress = _T("");
	m_strProgressCurrent = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CAutoupdateDlg::~CAutoupdateDlg()
{
}

void CAutoupdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoupdateDlg)
	DDX_Control(pDX, IDC_PROGRESSCURRENT, m_progressCurrent);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Text(pDX, IDC_STATIC_VERSON, m_strVerson);
	DDX_Text(pDX, IDC_STATIC_PROGRESS, m_strProgress);
	DDX_Text(pDX, IDC_STATIC_PROGRESSCURRENT, m_strProgressCurrent);
	DDX_Control(pDX, IDC_EXPLORER1, m_web);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAutoupdateDlg, CDialog)
	//{{AFX_MSG_MAP(CAutoupdateDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateDlg message handlers

BOOL CAutoupdateDlg::OnInitDialog()
{
	CString sVersonInfo;
	char cResume = 'T';
	if (!theApp.m_bResume)
		cResume = 'F';

	sVersonInfo.Format("From Client Verson: %d to Server Verson: %d support resume (%c)",theApp.m_nLocalVerson,theApp.m_nServerVerson,cResume);
	m_strVerson = sVersonInfo;

	CDialog::OnInitDialog();
	m_web.Navigate(_T("http://www.jxqyonline.net/"),NULL,NULL,NULL,NULL);

	m_progress.SetRange(0,100);
	m_progressCurrent.SetRange(0,100);

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


	m_eStatus = STATUS_DOWNLOADING;

	SetTimer(TIMER_REFRESH,200,NULL);
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAutoupdateDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CAutoupdateDlg::OnPaint() 
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
HCURSOR CAutoupdateDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

int GetPercent(DWORD D1,  DWORD D2)
{
	double d1 = D1;
	double d2 = D2;
	return d1 * 100 / d2;
}

void CAutoupdateDlg::Refresh()
{
	switch (m_eStatus)
	{
	case STATUS_DOWNLOADING:
		if (theApp.GetDownloadedNum() == theApp.GetDownloadNum())
		{
			m_eStatus = STATUS_EXTRACTING;
			break;
		}
		m_strProgress.Format("all %d files downloaded %d file all %dK size downloaded %dK size",
			theApp.GetDownloadNum(),theApp.GetDownloadedNum(),theApp.GetDownloadSize()/1024,theApp.GetDownloadedSize()/1024);
		m_strProgressCurrent.Format("current file is %s size %dK downloaded %dK",
			theApp.GetDownloadingFile(),theApp.GetDownloadingSize()/1024,theApp.GetDownloadingDownloadedSize()/1024);
		UpdateData(FALSE);
		
		m_progress.SetPos(GetPercent(theApp.GetDownloadedSize(),theApp.GetDownloadSize()));
		m_progressCurrent.SetPos(GetPercent(theApp.GetDownloadingDownloadedSize(),theApp.GetDownloadingSize()));

		break;
	case STATUS_EXTRACTING:
		if (theApp.GetExtractedNum() == theApp.GetExtractNum())
		{	
			m_strProgress.Empty();
			m_strProgressCurrent.Empty();
			m_progress.ShowWindow(SW_HIDE);
			m_progressCurrent.ShowWindow(SW_HIDE);
			
			m_strVerson = "Deleting temp files ... ";
			m_eStatus = STATUS_DELETING;
			UpdateData(FALSE);
			break;
		}
		m_strProgress.Format("all %d files extracted %d file all %dK size extracted %dK size",
			theApp.GetExtractNum(),theApp.GetExtractedNum(),theApp.GetExtractSize()/1024,theApp.GetExtractedSize()/1024);
		m_strProgressCurrent.Format("current file is %s size %dK extracted %dK",
			theApp.GetExtractingFile(),theApp.GetExtractingSize()/1024,theApp.GetExtractingExtractedSize()/1024);
		UpdateData(FALSE);
		
		m_progress.SetPos(GetPercent(theApp.GetExtractedSize(),theApp.GetExtractSize()));
		m_progressCurrent.SetPos(GetPercent(theApp.GetExtractingExtractedSize(),theApp.GetExtractingSize()));

		break;
	case STATUS_DELETING:
		break;
	}

	if (theApp.m_bFinished > 0)
	{
		theApp.UpdateClientVerson();

		m_strVerson = "Update ok, and you can enter the game... ";
		m_strProgress.Format("all %d files extracted %d file all %dK size extracted %dK size",
			theApp.GetExtractNum(),theApp.GetExtractNum(),theApp.GetExtractSize()/1024,theApp.GetExtractSize()/1024);
		m_strProgressCurrent.Empty();
		UpdateData(FALSE);
		m_progress.SetPos(100);
		m_progressCurrent.SetPos(100);
		m_eStatus = STATUS_DELETING;
		UpdateData(FALSE);

		KillTimer(TIMER_REFRESH);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
		m_progressCurrent.ShowWindow(SW_HIDE);
		//OnOK();
	}
	else if (theApp.m_bFinished < 0)
		OnCancel();
}

void CAutoupdateDlg::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == TIMER_REFRESH)
		Refresh();
	CDialog::OnTimer(nIDEvent);
}

void CAutoupdateDlg::OnOK() 
{
	CDialog::OnOK();
}
