// AddressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GMDebug.h"
#include "AddressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddressDlg dialog


CAddressDlg::CAddressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddressDlg)
	m_strAddress = _T("");
	m_strPort = _T("");
	//}}AFX_DATA_INIT
}


void CAddressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddressDlg)
	DDX_Text(pDX, IDC_EDIT_ADDRESS, m_strAddress);
	DDX_Text(pDX, IDC_EDIT_PORT, m_strPort);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddressDlg, CDialog)
	//{{AFX_MSG_MAP(CAddressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddressDlg message handlers

void CAddressDlg::OnOK() 
{
	UpdateData(TRUE);
	CDialog::OnOK();
}
