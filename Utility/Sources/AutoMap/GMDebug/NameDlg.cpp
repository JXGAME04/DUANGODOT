// NameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GMDebug.h"
#include "NameDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNameDlg dialog


CNameDlg::CNameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNameDlg)
	m_strName = _T("");
	m_strPassword = _T("");
	//}}AFX_DATA_INIT
}


void CNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNameDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNameDlg, CDialog)
	//{{AFX_MSG_MAP(CNameDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameDlg message handlers

void CNameDlg::OnOK() 
{
	UpdateData(TRUE);
	CDialog::OnOK();
}
