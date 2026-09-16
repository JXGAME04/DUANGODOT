// DebugWinDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DebugWin.h"
#include "DebugWinDlg.h"
#include <list>
#include "AddChannelDlg.h"

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
// CDebugWinDlg dialog

CDebugWinDlg::CDebugWinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDebugWinDlg::IDD, pParent)
{
	m_nState = 0;
	//{{AFX_DATA_INIT(CDebugWinDlg)
	m_szWinInfo = _T("");
	m_bWriteTime = FALSE;
	m_bViewString = FALSE;
	m_bAllChannel = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDebugWinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDebugWinDlg)
	DDX_Control(pDX, IDC_CHANNELLIST, m_ChannelList);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Text(pDX, IDC_STATIC1, m_szWinInfo);
	DDX_Check(pDX, IDC_CHECKTIME, m_bWriteTime);
	DDX_Check(pDX, IDC_CHECK1, m_bViewString);
	DDX_Check(pDX, IDC_ALLCANEL, m_bAllChannel);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDebugWinDlg, CDialog)
	//{{AFX_MSG_MAP(CDebugWinDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_BUTTON1, OnClearList)
	ON_BN_CLICKED(IDC_Stop, OnStop)
	ON_BN_CLICKED(IDC_SaveFile, OnSaveFile)
	ON_BN_CLICKED(IDC_CLEARALL, OnClearAll)
	ON_BN_CLICKED(IDC_ADDCHANNEL, OnAddChannel)
	ON_BN_CLICKED(IDC_DELCHANNEL, OnDelChannel)
	ON_BN_CLICKED(IDC_CLEARCHANNEL, OnClearChannel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugWinDlg message handlers

BOOL CDebugWinDlg::OnInitDialog()
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
	char str[100];
	char str1[100];
	CString str2;
	GetClassName(this->GetSafeHwnd(), str, 100);
	GetWindowText(str2);
	m_szWinInfo.Format("ClassName:[%s], WindowName:[%s]", str, str2);
	UpdateData(false);
	
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDebugWinDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDebugWinDlg::OnPaint() 
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
HCURSOR CDebugWinDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CDebugWinDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) 
{
	// TODO: Add your message handler code here and/or call default
	
	if (m_nState)return FALSE;
	if (!pCopyDataStruct->lpData) return FALSE;

    char tmpbuf[128], ampm[] = "AM";
    time_t ltime;
  //  struct _timeb tstruct;
    //struct tm *today, *gmt, xmas = { 0, 0, 12, 25, 11, 93 };
	
    /* Set time zone from TZ environment variable. If TZ is not set,
	* the operating system is queried to obtain the default value 
	* for the variable. 
	*/
    _tzset();
	
    /* Display operating system-style date and time. */
    _strtime( tmpbuf );
    printf( "OS time:\t\t\t\t%s\n", tmpbuf );
    CString str; 
	UpdateData();   
	char * pStartString = (char *)pCopyDataStruct->lpData;
	if (!m_bAllChannel)
	{
		if (*(char *)pCopyDataStruct->lpData == '[')	
		{
			if (NULL == (pStartString = strstr((const char *)pCopyDataStruct->lpData, "]")))	return FALSE;
			char sChannel[200];
			memcpy(sChannel, (char*)(pCopyDataStruct->lpData) + 1, pStartString - pCopyDataStruct->lpData - 1);
			sChannel[pStartString - pCopyDataStruct->lpData -1 ] = 0;			
			
			if (LB_ERR == m_ChannelList.FindString(0, sChannel)) return FALSE;
				else
					pStartString ++;
		}
		else 
			return FALSE;
	}

	if (!m_bWriteTime)
		str.Format("%s  [0x%x]", (char*)pCopyDataStruct->lpData ,pWnd);
	else
		str.Format("[%s] %s  [0x%x]", tmpbuf,(char*)pCopyDataStruct->lpData ,pWnd);

	m_List.AddString(str);
	return TRUE;
	
}

void CDebugWinDlg::OnClearList() 
{
//	std::list<int> iset;
	for(int i = m_List.GetCount(); i >=0 ; i --)
	{
		if (m_List.GetSel(i))
			m_List.DeleteString(i);
		
	}
	
}

void CDebugWinDlg::OnStop() 
{
	// TODO: Add your control notification handler code here
	m_nState = !m_nState;
	if (m_nState)
		SetDlgItemText(IDC_Stop,"暂听中");
		else
		SetDlgItemText(IDC_Stop,"接收中");

}

void CDebugWinDlg::OnSaveFile() 
{
	// TODO: Add your control notification handler code here
	CFileDialog FDlg(FALSE);
	CString str;
	if (FDlg.DoModal() == IDOK)
	{
		str = FDlg.GetPathName();
	}
	else 
		return;
	
	CFile File;
	File.Open(str,CFile::modeCreate | CFile::modeWrite);
	CString Buf;
	
	for(int i = 0; i < m_List.GetCount(); i ++)
	{
		Buf = "";
		m_List.GetText(i,Buf);
		Buf = Buf + "\n";
		File.Write(Buf, Buf.GetLength());
	}		
	File.Close();
	
}

void CDebugWinDlg::OnClearAll() 
{
	// TODO: Add your control notification handler code here
	m_List.ResetContent()	;
}

void CDebugWinDlg::OnAddChannel() 
{
	CAddChannelDlg Dlg;
	Dlg.DoModal();
	if (Dlg.m_strChannel != "")
	{
		if (LB_ERR == m_ChannelList.FindString(0,Dlg.m_strChannel))
			m_ChannelList.InsertString(0, Dlg.m_strChannel);
	}
	
}

void CDebugWinDlg::OnDelChannel() 
{
	CAddChannelDlg Dlg;
	Dlg.DoModal();
	if (Dlg.m_strChannel != "")
	{
		int nIndex = 0;
		if (LB_ERR != (nIndex = m_ChannelList.FindString(-1, Dlg.m_strChannel)))
		{
			m_ChannelList.DeleteString(nIndex);
		}
	}
}

void CDebugWinDlg::OnClearChannel() 
{
	// TODO: Add your control notification handler code here
	m_ChannelList.ResetContent();
}
