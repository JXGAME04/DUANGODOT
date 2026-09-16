// GMPassGenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GMPassGen.h"
#include "GMPassGenDlg.h"
#include "include/PassGen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMPassGenDlg dialog

CGMPassGenDlg::CGMPassGenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGMPassGenDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGMPassGenDlg)
	m_strPassword1 = _T("");
	m_strPassword2 = _T("");
	m_strResult = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGMPassGenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGMPassGenDlg)
	DDX_Text(pDX, IDC_PASSWORD1, m_strPassword1);
	DDV_MaxChars(pDX, m_strPassword1, 20);
	DDX_Text(pDX, IDC_PASSWORD2, m_strPassword2);
	DDV_MaxChars(pDX, m_strPassword2, 20);
	DDX_Text(pDX, IDC_ENCRYPTED_RESULT, m_strResult);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGMPassGenDlg, CDialog)
	//{{AFX_MSG_MAP(CGMPassGenDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_GEN_PASS, OnGenPass)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMPassGenDlg message handlers

BOOL CGMPassGenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGMPassGenDlg::OnPaint() 
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

HCURSOR CGMPassGenDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CGMPassGenDlg::OnGenPass() 
{
	UpdateData();
	if (m_strPassword1 != m_strPassword2)
	{
		::AfxMessageBox("两次输入的密码不相符，请重新输入 ！！");
		return;
	}
	VERIFY(::SimplyEncryptPassword(m_strResult.GetBuffer(PG_MAXBUFFER), m_strPassword1));
	m_strResult.ReleaseBuffer();
	UpdateData(FALSE);
}

void CGMPassGenDlg::WinHelp(DWORD dwData, UINT nCmd) 
{
	CString str;
	UpdateData();
	if (m_strResult.GetLength())
	{
		VERIFY(::SimplyDecryptPassword(str.GetBuffer(PG_MAXBUFFER), m_strResult));
		::AfxMessageBox(str);
	}
}
