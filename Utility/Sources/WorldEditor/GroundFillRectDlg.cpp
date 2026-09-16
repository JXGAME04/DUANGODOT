// GroundFillRectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "worldeditor.h"
#include "GroundFillRectDlg.h"

#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroundFillRectDlg dialog


CGroundFillRectDlg::CGroundFillRectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroundFillRectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroundFillRectDlg)
	m_nLeft = 0;
	m_nTop = 0;
	m_nRight = 2;
	m_nBottom = 2;
	m_strType = _T("");
	//}}AFX_DATA_INIT
}


void CGroundFillRectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroundFillRectDlg)
	DDX_Control(pDX, IDC_TYPE, m_cbType);
	DDX_Text(pDX, IDC_LEFT, m_nLeft);
	DDX_Text(pDX, IDC_TOP, m_nTop);
	DDX_Text(pDX, IDC_RIGHT, m_nRight);
	DDX_Text(pDX, IDC_BOTTOM, m_nBottom);
	DDX_CBString(pDX, IDC_TYPE, m_strType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroundFillRectDlg, CDialog)
	//{{AFX_MSG_MAP(CGroundFillRectDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroundFillRectDlg message handlers
void CGroundFillRectDlg::SetPathName(LPCSTR sz)
{
	m_strPathName = sz;
	CString sPath,sName,sExt;
	AppParsePathNameExt(sz,sPath,sName,sExt);
	m_strType = sName;
}

void CGroundFillRectDlg::OnOK() 
{
	// TODO: Add extra validation here
	if (m_nLeft >= m_nRight || m_nTop >= m_nBottom)
	{
		AfxMessageBox("请保证是个正常的矩形");
		return ;
	}
	if (m_nRight >= 255 || m_nLeft >= 255)
	{
		AfxMessageBox("数值不能超过255");
		return ;
	}

	
	CDialog::OnOK();
}
