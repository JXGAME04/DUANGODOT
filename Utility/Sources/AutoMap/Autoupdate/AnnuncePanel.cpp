// AnnuncePanel.cpp : implementation file
//

#include "stdafx.h"
#include "autoupdate.h"
#include "AnnuncePanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AnnuncePanel dialog

//const CString Downloadurl = "http://www.jxonline.net/download/10/50.shtml";
AnnuncePanel::AnnuncePanel(CWnd* pParent /*=NULL*/)
	: CDialog(AnnuncePanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(AnnuncePanel)
	m_NoticeInfo = _T("系统检测到您的游戏版本太旧，以至无法完成自动升级。");
    m_NoticeInfo = m_NoticeInfo + '\n' + _T("您需要下载最新的升级包进行升级。");

	//}}AFX_DATA_INIT
}


void AnnuncePanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AnnuncePanel)
	DDX_Text(pDX, IDC_NoticeInfo, m_NoticeInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AnnuncePanel, CDialog)
	//{{AFX_MSG_MAP(AnnuncePanel)
	ON_BN_CLICKED(IDC_BUTTON1, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AnnuncePanel message handlers

void AnnuncePanel::OnOK() 
{
	// TODO: Add extra validation here
	ShellExecute(NULL,"open", m_NoticeInfoUrl,NULL,NULL,SW_SHOW);
	CDialog::OnOK();
}

void AnnuncePanel::OnCancel() 
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

void AnnuncePanel::SetNoticeInfoUrl(const CString &URL)
{
	m_NoticeInfoUrl = URL;

}
