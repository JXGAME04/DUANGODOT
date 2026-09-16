// SelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "SelectDlg.h"

#include "iItem.h"
#include "planedoodad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg dialog


CSelectDlg::CSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectDlg)
	m_nType = -1;
	m_nSort = -1;
	m_nKind = -1;
	m_nFilter = -1;
	m_nFilterParam = -1;
	//}}AFX_DATA_INIT
}


void CSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDlg)
	DDX_Control(pDX, IDC_COMBO_FILTERPARAM, m_cbFilterParam);
	DDX_Control(pDX, IDC_COMBO_FILTER, m_cbFilter);
	DDX_Control(pDX, IDC_COMBO_KIND, m_cbKind);
	DDX_Control(pDX, IDC_COMBO_SORT, m_cbSort);
	DDX_Radio(pDX, IDC_TYPE, m_nType);
	DDX_CBIndex(pDX, IDC_COMBO_SORT, m_nSort);
	DDX_CBIndex(pDX, IDC_COMBO_KIND, m_nKind);
	DDX_CBIndex(pDX, IDC_COMBO_FILTER, m_nFilter);
	DDX_CBIndex(pDX, IDC_COMBO_FILTERPARAM, m_nFilterParam);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(ID_RESET, OnReset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg message handlers

CString g_aStrFilterType[] = 
	{"全部","默认","Y","面积","偏移","Z","前后",""};
CString g_aStrFilterParamType[] = 
	{"默认","最小","最大","前面","后面","中间",""};


BOOL CSelectDlg::UpdateSelect() 
{
	m_nType = m_stSelect.eType;
	m_nSort = m_stSelect.eSort;
	m_nKind = m_stSelect.eKind;
	m_nFilter = m_stSelect.eFilter;
	m_nFilterParam = m_stSelect.eFilterParam;
	UpdateData(FALSE);
	return TRUE;
}

BOOL CSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int j = 0;
	CString* aStr = GetTablePlaneValue(TP_SORT);
	while(!aStr[j].IsEmpty())
		m_cbSort.AddString(aStr[j++]);

	j = 0;
	aStr = GetTablePlaneValue(TP_KIND);
	while(!aStr[j].IsEmpty())
		m_cbKind.AddString(aStr[j++]);

	j = 0;
	aStr = g_aStrFilterType;
	while(!aStr[j].IsEmpty())
		m_cbFilter.AddString(aStr[j++]);

	j = 0;
	aStr = g_aStrFilterParamType;
	while(!aStr[j].IsEmpty())
		m_cbFilterParam.AddString(aStr[j++]);

	UpdateSelect();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectDlg::OnOk() 
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
	m_stSelect.eType = (SelectTypeEnum)m_nType;
	m_stSelect.eSort = (SortEnum)m_nSort;
	m_stSelect.eKind = (KindEnum)m_nKind;
	m_stSelect.eFilter = (SelectFilterEnum)m_nFilter;
	m_stSelect.eFilterParam = (SelectFilterParamEnum)m_nFilterParam;
}

void CSelectDlg::OnCancel() 
{
	CDialog::OnCancel();
}

void CSelectDlg::OnReset() 
{
	m_stSelect.eType = SelectType_Default;
	m_stSelect.eFilter = SelectFilter_OneByDisplay;
	m_stSelect.eFilterParam = SelectFilterParam_Default;
	m_stSelect.eKind = KIND_NORMAL;
	m_stSelect.eSort = SORT_VER;		
	UpdateSelect();
}
