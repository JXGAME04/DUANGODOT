// SpecialDlg.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "SpecialDlg.h"

#include "igraphics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecialDlg dialog


CSpecialDlg::CSpecialDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpecialDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpecialDlg)
	//}}AFX_DATA_INIT
}


void CSpecialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecialDlg)
	DDX_Text(pDX, IDC_VALUE1, m_aValue[0]);
	DDX_Text(pDX, IDC_ITEM1, m_aItem[0]);
	DDX_Text(pDX, IDC_DESCRIPTION1, m_aDescription[0]);
	DDX_Text(pDX, IDC_VALUE2, m_aValue[1]);
	DDX_Text(pDX, IDC_ITEM2, m_aItem[1]);
	DDX_Text(pDX, IDC_DESCRIPTION2, m_aDescription[1]);
	DDX_Text(pDX, IDC_VALUE3, m_aValue[2]);
	DDX_Text(pDX, IDC_ITEM3, m_aItem[2]);
	DDX_Text(pDX, IDC_DESCRIPTION3, m_aDescription[2]);
	DDX_Text(pDX, IDC_VALUE4, m_aValue[3]);
	DDX_Text(pDX, IDC_ITEM4, m_aItem[3]);
	DDX_Text(pDX, IDC_DESCRIPTION4, m_aDescription[3]);
	DDX_Text(pDX, IDC_VALUE5, m_aValue[4]);
	DDX_Text(pDX, IDC_ITEM5, m_aItem[4]);
	DDX_Text(pDX, IDC_DESCRIPTION5, m_aDescription[4]);
	DDX_Text(pDX, IDC_VALUE6, m_aValue[5]);
	DDX_Text(pDX, IDC_ITEM6, m_aItem[5]);
	DDX_Text(pDX, IDC_DESCRIPTION6, m_aDescription[5]);
	DDX_Text(pDX, IDC_VALUE7, m_aValue[6]);
	DDX_Text(pDX, IDC_ITEM7, m_aItem[6]);
	DDX_Text(pDX, IDC_DESCRIPTION7, m_aDescription[6]);
	DDX_Text(pDX, IDC_VALUE8, m_aValue[7]);
	DDX_Text(pDX, IDC_ITEM8, m_aItem[7]);
	DDX_Text(pDX, IDC_DESCRIPTION8, m_aDescription[7]);
	DDX_Text(pDX, IDC_VALUE9, m_aValue[8]);
	DDX_Text(pDX, IDC_ITEM9, m_aItem[8]);
	DDX_Text(pDX, IDC_DESCRIPTION9, m_aDescription[8]);
	DDX_Text(pDX, IDC_VALUE10, m_aValue[9]);
	DDX_Text(pDX, IDC_ITEM10, m_aItem[9]);
	DDX_Text(pDX, IDC_DESCRIPTION10, m_aDescription[9]);
	DDX_Text(pDX, IDC_VALUE11, m_aValue[10]);
	DDX_Text(pDX, IDC_ITEM11, m_aItem[10]);
	DDX_Text(pDX, IDC_DESCRIPTION11, m_aDescription[10]);
	DDX_Text(pDX, IDC_VALUE12, m_aValue[11]);
	DDX_Text(pDX, IDC_ITEM12, m_aItem[11]);
	DDX_Text(pDX, IDC_DESCRIPTION12, m_aDescription[11]);
	DDX_Text(pDX, IDC_VALUE13, m_aValue[12]);
	DDX_Text(pDX, IDC_ITEM13, m_aItem[12]);
	DDX_Text(pDX, IDC_DESCRIPTION13, m_aDescription[12]);
	DDX_Text(pDX, IDC_VALUE14, m_aValue[13]);
	DDX_Text(pDX, IDC_ITEM14, m_aItem[13]);
	DDX_Text(pDX, IDC_DESCRIPTION14, m_aDescription[13]);
	DDX_Text(pDX, IDC_VALUE15, m_aValue[14]);
	DDX_Text(pDX, IDC_ITEM15, m_aItem[14]);
	DDX_Text(pDX, IDC_DESCRIPTION15, m_aDescription[14]);
	DDX_Text(pDX, IDC_VALUE16, m_aValue[15]);
	DDX_Text(pDX, IDC_ITEM16, m_aItem[15]);
	DDX_Text(pDX, IDC_DESCRIPTION16, m_aDescription[15]);
//control
	DDX_Control(pDX, IDC_VALUE1, m_aValueControl[0]);
	DDX_Control(pDX, IDC_ITEM1, m_aItemControl[0]);
	DDX_Control(pDX, IDC_DESCRIPTION1, m_aDescriptionControl[0]);
	DDX_Control(pDX, IDC_VALUE2, m_aValueControl[1]);
	DDX_Control(pDX, IDC_ITEM2, m_aItemControl[1]);
	DDX_Control(pDX, IDC_DESCRIPTION2, m_aDescriptionControl[1]);
	DDX_Control(pDX, IDC_VALUE3, m_aValueControl[2]);
	DDX_Control(pDX, IDC_ITEM3, m_aItemControl[2]);
	DDX_Control(pDX, IDC_DESCRIPTION3, m_aDescriptionControl[2]);
	DDX_Control(pDX, IDC_VALUE4, m_aValueControl[3]);
	DDX_Control(pDX, IDC_ITEM4, m_aItemControl[3]);
	DDX_Control(pDX, IDC_DESCRIPTION4, m_aDescriptionControl[3]);
	DDX_Control(pDX, IDC_VALUE5, m_aValueControl[4]);
	DDX_Control(pDX, IDC_ITEM5, m_aItemControl[4]);
	DDX_Control(pDX, IDC_DESCRIPTION5, m_aDescriptionControl[4]);
	DDX_Control(pDX, IDC_VALUE6, m_aValueControl[5]);
	DDX_Control(pDX, IDC_ITEM6, m_aItemControl[5]);
	DDX_Control(pDX, IDC_DESCRIPTION6, m_aDescriptionControl[5]);
	DDX_Control(pDX, IDC_VALUE7, m_aValueControl[6]);
	DDX_Control(pDX, IDC_ITEM7, m_aItemControl[6]);
	DDX_Control(pDX, IDC_DESCRIPTION7, m_aDescriptionControl[6]);
	DDX_Control(pDX, IDC_VALUE8, m_aValueControl[7]);
	DDX_Control(pDX, IDC_ITEM8, m_aItemControl[7]);
	DDX_Control(pDX, IDC_DESCRIPTION8, m_aDescriptionControl[7]);
	DDX_Control(pDX, IDC_VALUE9, m_aValueControl[8]);
	DDX_Control(pDX, IDC_ITEM9, m_aItemControl[8]);
	DDX_Control(pDX, IDC_DESCRIPTION9, m_aDescriptionControl[8]);
	DDX_Control(pDX, IDC_VALUE10, m_aValueControl[9]);
	DDX_Control(pDX, IDC_ITEM10, m_aItemControl[9]);
	DDX_Control(pDX, IDC_DESCRIPTION10, m_aDescriptionControl[9]);
	DDX_Control(pDX, IDC_VALUE11, m_aValueControl[10]);
	DDX_Control(pDX, IDC_ITEM11, m_aItemControl[10]);
	DDX_Control(pDX, IDC_DESCRIPTION11, m_aDescriptionControl[10]);
	DDX_Control(pDX, IDC_VALUE12, m_aValueControl[11]);
	DDX_Control(pDX, IDC_ITEM12, m_aItemControl[11]);
	DDX_Control(pDX, IDC_DESCRIPTION12, m_aDescriptionControl[11]);
	DDX_Control(pDX, IDC_VALUE13, m_aValueControl[12]);
	DDX_Control(pDX, IDC_ITEM13, m_aItemControl[12]);
	DDX_Control(pDX, IDC_DESCRIPTION13, m_aDescriptionControl[12]);
	DDX_Control(pDX, IDC_VALUE14, m_aValueControl[13]);
	DDX_Control(pDX, IDC_ITEM14, m_aItemControl[13]);
	DDX_Control(pDX, IDC_DESCRIPTION14, m_aDescriptionControl[13]);
	DDX_Control(pDX, IDC_VALUE15, m_aValueControl[14]);
	DDX_Control(pDX, IDC_ITEM15, m_aItemControl[14]);
	DDX_Control(pDX, IDC_DESCRIPTION15, m_aDescriptionControl[14]);
	DDX_Control(pDX, IDC_VALUE16, m_aValueControl[15]);
	DDX_Control(pDX, IDC_ITEM16, m_aItemControl[15]);
	DDX_Control(pDX, IDC_DESCRIPTION16, m_aDescriptionControl[15]);

	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpecialDlg, CDialog)
	//{{AFX_MSG_MAP(CSpecialDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecialDlg message handlers

void CSpecialDlg::OnOk() 
{
	// TODO: Add your control notification handler code here
	UpdateData();

	int count  = m_pSpecial->GetItemCount();
	ASSERT(count <= MAX_ITEM);
	for (int i=0; i<count; i++)
	{
		m_pSpecial->SetParam(i,m_aValue[i]);
	}
	
	OnOK();
}

BOOL CSpecialDlg::OnInitDialog() 
{
	// TODO: Add extra initialization here
	ASSERT(m_pSpecial);

	int count  = m_pSpecial->GetItemCount();
	ASSERT(count <= MAX_ITEM);
	
	std::string strItem,strValue,strDescription;
	for (int i=0; i<count; i++)
	{
		m_pSpecial->GetParam(i,strItem,strValue,strDescription);
		m_aItem[i] = strItem.c_str();
		m_aValue[i] = strValue.c_str();
		m_aDescription[i] = strDescription.c_str();
	}
	
	CDialog::OnInitDialog();

	for (i=0; i<count; i++)
	{
		if (m_aDescription[i] == "²»ÄÜÐÞ¸Ä")
		{
			m_aValueControl[i].EnableWindow(FALSE);
		}
	}

	for (;i<MAX_ITEM; i++)
	{
		m_aValueControl[i].ShowWindow(SW_HIDE);
		m_aItemControl[i].ShowWindow(SW_HIDE);
		m_aDescriptionControl[i].ShowWindow(SW_HIDE);
//hide others
	}	


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
