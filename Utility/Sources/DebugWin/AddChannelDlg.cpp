// AddChannelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "debugwin.h"
#include "AddChannelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddChannelDlg dialog


CAddChannelDlg::CAddChannelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddChannelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddChannelDlg)
	m_strChannel = _T("");
	//}}AFX_DATA_INIT
}


void CAddChannelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddChannelDlg)
	DDX_Text(pDX, IDC_EDIT1, m_strChannel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddChannelDlg, CDialog)
	//{{AFX_MSG_MAP(CAddChannelDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddChannelDlg message handlers

void CAddChannelDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData();
	CDialog::OnOK();
}

void CAddChannelDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	m_strChannel = "";
	UpdateData(FALSE);
	CDialog::OnCancel();
}
