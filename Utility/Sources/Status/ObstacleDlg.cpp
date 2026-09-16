// ObstacleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "ObstacleDlg.h"

#include "iItem.h"
#include "planedoodad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObstacleDlg dialog


CObstacleDlg::CObstacleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObstacleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CObstacleDlg)
	m_nType = -1;
	m_nKind = -1;
	m_fRadius = 0.0f;
	//}}AFX_DATA_INIT
}


void CObstacleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObstacleDlg)
	DDX_Control(pDX, IDC_OBSTACLE_KIND, m_cbKind);
	DDX_Control(pDX, IDC_OBSTACLE_TYPE, m_cbType);
	DDX_CBIndex(pDX, IDC_OBSTACLE_TYPE, m_nType);
	DDX_CBIndex(pDX, IDC_OBSTACLE_KIND, m_nKind);
	DDX_Text(pDX, IDC_OBSTACLE_RADIUS, m_fRadius);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObstacleDlg, CDialog)
	//{{AFX_MSG_MAP(CObstacleDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObstacleDlg message handlers

BOOL CObstacleDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = TP_OBSTACLETYPE;
	int j = 0;
	CString* aStr = GetTablePlaneValue(i);
	while(!aStr[j].IsEmpty())
		m_cbType.AddString(aStr[j++]);
	i = TP_KIND;
	j = 0;
	aStr = GetTablePlaneValue(i);
	while(!aStr[j].IsEmpty())
		m_cbKind.AddString(aStr[j++]);
	UpdateData(FALSE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
