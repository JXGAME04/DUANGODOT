// ImageCreateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ImageCreate.h"
#include "ImageCreateDlg.h"

#include "fileop.h"
#include "iItem.h"
#include "iImage.h"

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
// CImageCreateDlg dialog

CImageCreateDlg::CImageCreateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImageCreateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImageCreateDlg)
	m_strFile = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CImageCreateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImageCreateDlg)
	DDX_Control(pDX, IDC_STATIC_SPR, m_staticSpr);
	DDX_Text(pDX, IDC_FILENAME, m_strFile);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CImageCreateDlg, CDialog)
	//{{AFX_MSG_MAP(CImageCreateDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FILE, OnFile)
	ON_BN_CLICKED(IDC_GRID, OnGrid)
	ON_BN_CLICKED(IDC_SPR, OnSpr)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageCreateDlg message handlers

BOOL CImageCreateDlg::OnInitDialog()
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
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CImageCreateDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CImageCreateDlg::OnPaint() 
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
HCURSOR CImageCreateDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CImageCreateDlg::OnFile() 
{
	UpdateData();
	if (m_strFile.IsEmpty())
	{
		CString strFile;
		CString sWork;
		AppGetWorkingFolder(sWork);
		sWork += "\\system\\spr";
		if (AppGetFileDialog(strFile,sWork,"*.tga",FALSE))
		{
			m_strFile = strFile;
			UpdateData(FALSE);
		}
	}
	else
	{
		CString strFolder;
		AppParsePath(m_strFile,strFolder);
		CString strFile;
		if (AppGetFileDialog(strFile,strFolder,"*.tga",FALSE))
		{
			m_strFile = strFile;
			UpdateData(FALSE);
		}
	}	
}

BOOL CImageCreateDlg::IsFileNameValid()
{
	UpdateData();
	if (m_strFile.IsEmpty())
	{
		AfxMessageBox("请先输入文件名称!");
		return FALSE;
	}
	if (m_strFile.Right(4) != ".tga")
	{
		AfxMessageBox("必须是tga的图片!");
		return FALSE;
	}
	return TRUE;
}

void CImageCreateDlg::OnGrid() 
{
	if(!IsFileNameValid())
		return ;
	iImage* pImage = Image_CreateImage(m_strFile);
	wrap_ptr<iImage> ptr(pImage);
	pImage->CreateGrid(m_strFile,512,512,32,16);//长，宽，格子长宽
}

void CImageCreateDlg::OnSpr() 
{
	if(!IsFileNameValid())
		return ;	
	iImage* pImage = Image_CreateImage(m_strFile);
	pImage->LoadFile(m_strFile);
	wrap_ptr<iImage> ptr(pImage);
	CString strSpr = m_strFile.Left(m_strFile.GetLength() - 4) + ".spr";
	iImage* pImageSpr = Image_CreateImage(strSpr);
	wrap_ptr<iImage> ptr2(pImageSpr);
	pImageSpr->AddImage(pImage);
	pImageSpr->SaveFile(strSpr);
	m_staticSpr.SetSprFile(strSpr);
	m_staticSpr.SetFrame(0,0);
}
