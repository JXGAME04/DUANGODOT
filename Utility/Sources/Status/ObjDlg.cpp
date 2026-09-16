// ObjDlg.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "ObjDlg.h"
#include "iPublic.h"
#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjDlg dialog

CObjDlg::CObjDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObjDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CObjDlg)
	m_strNick = _T("");
	m_strTemplate = _T("");
	m_strSection = _T("");
	m_bAutoPlay = FALSE;
	//}}AFX_DATA_INIT
}


void CObjDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjDlg)
	DDX_Text(pDX, IDC_NICK, m_strNick);
	DDX_Text(pDX, IDC_TEMPLATE, m_strTemplate);
	DDX_Text(pDX, IDC_ANIMATION, m_strSection);
	DDX_Check(pDX, IDC_ANI_AUTOPLAY, m_bAutoPlay);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjDlg, CDialog)
	//{{AFX_MSG_MAP(CObjDlg)
	ON_BN_CLICKED(IDC_BROWSE_TEMPLATE, OnBrowseTemplate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjDlg message handlers

void CObjDlg::OnOK() 
{
	if (m_strNick.GetLength() >= MAX_NAME )
	{
		AfxMessageBox("名字太长");
		return;
	}
	CDialog::OnOK();
}

BOOL CObjDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CObjDlg::OnBrowseTemplate() 
{
	CString strFile;
	CString strWork = "游戏资源\\动态物体";
	AppGetFullPath(strWork);
	if (AppGetFileDialog(strFile,strWork,"*.ini"))
	{
		m_strTemplate = strFile;
		UpdateData(FALSE);
	}
}
/*
void CObjDlg::OnBrowseAnimation() 
{
	CString strFile;
	CString strWork = "游戏资源\\动态物体";
	AppGetFullPath(strWork);
	if (AppGetFileDialog(strFile,strWork,"*.spr"))
	{
		m_strSection = strFile;
		UpdateData(FALSE);
	}	
}
*/