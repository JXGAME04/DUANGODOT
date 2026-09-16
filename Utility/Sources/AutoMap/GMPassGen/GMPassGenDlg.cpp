// GMPassGenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GMPassGen.h"
#include "GMPassGenDlg.h"
#include "include/PassGen.h"
#include "md5.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int KSG_StringToMD5String(char szDestMD5String[64], const char cszSrcString[])
{
    int nResult = false;
    md5_state_t md5_state;
    unsigned char MD5Value[16];
    int nSrcStringLen = 0;

    if (!szDestMD5String)
        goto Exit0;

    szDestMD5String[0] = '\0';

    if  (!cszSrcString)
       goto Exit0;

    nSrcStringLen = strlen(cszSrcString);

    if (!nSrcStringLen)
        goto Exit0;

    md5_init(&md5_state);

    md5_append(&md5_state, (unsigned char *)cszSrcString, nSrcStringLen);

    md5_finish(&md5_state, MD5Value);


    sprintf(
        szDestMD5String,
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        MD5Value[ 0], MD5Value[ 1], MD5Value[ 2], MD5Value[ 3],
        MD5Value[ 4], MD5Value[ 5], MD5Value[ 6], MD5Value[ 7],
        MD5Value[ 8], MD5Value[ 9], MD5Value[10], MD5Value[11],
        MD5Value[12], MD5Value[13], MD5Value[14], MD5Value[15]
    );

    nResult = true;
Exit0:
    return nResult;
}

/////////////////////////////////////////////////////////////////////////////
// CGMPassGenDlg dialog

CGMPassGenDlg::CGMPassGenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGMPassGenDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGMPassGenDlg)
	m_strPassword1 = _T("");
	m_strPassword2 = _T("");
	m_strResult = _T("");
	m_strResultMD5 = _T("");
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
	DDX_Text(pDX, IDC_ENCRYPTED_RESULT2, m_strResultMD5);
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
	VERIFY(::KSG_StringToMD5String(m_strResultMD5.GetBuffer(64), m_strPassword1));
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
