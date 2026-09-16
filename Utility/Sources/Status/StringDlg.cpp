// StringDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Status.h"
#include "cStatus.h"
#include "StringDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStringDlg dialog


CStringDlg::CStringDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStringDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStringDlg)
	m_strString = _T("");
	//}}AFX_DATA_INIT
}

void CStringDlg::GetSetting(stSetting& st)
{
	ASSERT(m_eWant == st.mask);
	GetValueSetting(m_strString,st);
}

void CStringDlg::GetString(eMaskSetting eWant, CString& s)
{
	ASSERT(m_eWant == eWant);
	s = m_strString ;
}

void CStringDlg::SetString(eMaskSetting eWant, const CString& s)
{
	m_eWant = eWant;
	m_strString = s;
}

void CStringDlg::SetSetting(const stSetting& st)
{
	m_eWant = (eMaskSetting)st.mask;
	SetValueSetting(m_strString,st);
}

void CStringDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStringDlg)
	DDX_Text(pDX, IDC_STRING, m_strString);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStringDlg, CDialog)
	//{{AFX_MSG_MAP(CStringDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStringDlg message handlers

BOOL CStringDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	if (!m_strTitle.IsEmpty())
		SetWindowText(m_strTitle);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStringDlg::OnOk() 
{
	UpdateData();
	if (m_strString.IsEmpty())
	{
		AfxMessageBox("the string can not be null!");
		return;
	}
	//
	CDialog::OnOK();	
}
