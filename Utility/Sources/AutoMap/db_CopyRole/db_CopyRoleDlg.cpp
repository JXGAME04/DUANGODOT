// db_CopyRoleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RoleStruct.h"
#include "db_CopyRole.h"
#include "db_CopyRoleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDb_CopyRoleDlg dialog

CDb_CopyRoleDlg::CDb_CopyRoleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDb_CopyRoleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDb_CopyRoleDlg)
	m_DBDescPath = _T("");
	m_DBSourcePath = _T("");
	m_RoleName = _T("");
	m_RoleData = _T("");
	DBSource = NULL;
	DBDesc = NULL;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDb_CopyRoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDb_CopyRoleDlg)
	DDX_Text(pDX, IDC_TXT_DB_DESC, m_DBDescPath);
	DDX_Text(pDX, IDC_TXT_DB_SOURCE, m_DBSourcePath);
	DDX_Text(pDX, IDC_TXT_ROLENAME, m_RoleName);
	DDX_Text(pDX, IDC_MEMO_ROLEDATA, m_RoleData);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDb_CopyRoleDlg, CDialog)
	//{{AFX_MSG_MAP(CDb_CopyRoleDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_DB_SOURCE, OnBtnDbSource)
	ON_BN_CLICKED(IDC_BTN_DB_DESC, OnBtnDbDesc)
	ON_BN_CLICKED(IDC_BTN_ROLE_FIND, OnBtnRoleFind)
	ON_BN_CLICKED(IDC_BTN_ROLE_COPY, OnBtnRoleCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDb_CopyRoleDlg message handlers

BOOL CDb_CopyRoleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDb_CopyRoleDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDb_CopyRoleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CDb_CopyRoleDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	if(DBDesc)
	{
		delete DBDesc;
		DBDesc = NULL;
	}
	if(DBSource)
	{
		delete DBSource;
		DBSource = NULL;
	}
	return CDialog::DestroyWindow();
}

void CDb_CopyRoleDlg::OnBtnDbSource() 
{
	UpdateData();
	if(DBSource)return;
	if(m_DBSourcePath == "")
	{
		AfxMessageBox("请输入源数据库路径！");
		return;
	}
	DBSource = new CDBTableReadOnly(m_DBSourcePath.GetBuffer(1),"roledb");
	if(DBSource->open())
	{
		::EnableWindow( ::GetDlgItem( this->m_hWnd, IDC_BTN_DB_SOURCE ), FALSE );
		::EnableWindow( ::GetDlgItem( this->m_hWnd, IDC_TXT_DB_SOURCE ), FALSE );
	}
	else
	{
		AfxMessageBox("源数据库打开错误！");
		delete DBSource;
		DBSource = NULL;
	}
}

void CDb_CopyRoleDlg::OnBtnDbDesc() 
{
	UpdateData();
	if(DBDesc)return;
	if(m_DBDescPath == "")
	{
		AfxMessageBox("请输入目标数据库路径！");
		return;
	}
	DBDesc = new ZDBTable(m_DBDescPath.GetBuffer(1),"roledb");
	if(DBDesc->open())
	{
		::EnableWindow( ::GetDlgItem( this->m_hWnd, IDC_BTN_DB_DESC ), FALSE );
		::EnableWindow( ::GetDlgItem( this->m_hWnd, IDC_TXT_DB_DESC ), FALSE );
	}
	else
	{
		AfxMessageBox("目标数据库打开错误");
		delete DBDesc;
		DBDesc = NULL;
	}
}



void CDb_CopyRoleDlg::OnBtnRoleFind() 
{
	UpdateData();
	if(!DBSource)
	{
		AfxMessageBox("源数据库未打开！");
		return;
	}
	if(m_DBSourcePath == "")
	{
		AfxMessageBox("请输入角色名！");
		return;
	}
	int size;
	char* aBuffer = DBSource->search(m_RoleName.GetBuffer(1), strlen(m_RoleName.GetBuffer(1)) + 1, size);

	if(!aBuffer)
	{
		m_RoleData = "未找到人物：" + m_RoleName;
		UpdateData(false);
		return;
	}
	TRoleData* pRoleData = (TRoleData*)aBuffer;
	char aStr[32];

	m_RoleData = "";

	m_RoleData += "账号名：";
	m_RoleData += pRoleData->BaseInfo.caccname;
	m_RoleData += "\r\n";

	m_RoleData += "角色名：";
	m_RoleData += pRoleData->BaseInfo.szName;
	m_RoleData += "\r\n";

	if(!pRoleData->BaseInfo.bSex)
		strcpy(aStr,"男");
	else
		strcpy(aStr,"女");
	m_RoleData += "性别：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";
	
	switch(pRoleData->BaseInfo.ifiveprop)
	{
		case 0:strcpy(aStr,"金");break;
		case 1:strcpy(aStr,"木");break;
		case 2:strcpy(aStr,"水");break;
		case 3:strcpy(aStr,"火");break;
		case 4:strcpy(aStr,"土");break;
		default:strcpy(aStr,"错误");break;
	}
	m_RoleData += "五行属性：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	switch(pRoleData->BaseInfo.nSect)
	{
		case 0:strcpy(aStr,"少林派");break;
		case 1:strcpy(aStr,"天王帮");break;
		case 2:strcpy(aStr,"唐门");break;
		case 3:strcpy(aStr,"五毒教");break;
		case 4:strcpy(aStr,"峨嵋派");break;
		case 5:strcpy(aStr,"翠烟门");break;
		case 6:strcpy(aStr,"丐帮");break;
		case 7:strcpy(aStr,"天忍教");break;
		case 8:strcpy(aStr,"武当派");break;
		case 9:strcpy(aStr,"昆仑派");break;
		default:strcpy(aStr,"无门派");break;
	}
	m_RoleData += "门派：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	
	itoa(pRoleData->BaseInfo.ifightlevel, aStr, 10);
	m_RoleData += "等级：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.imoney + pRoleData->BaseInfo.isavemoney, aStr, 10);
	m_RoleData += "金钱：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.ientergameid, aStr, 10);
	m_RoleData += "最后所在地图：";
	m_RoleData += aStr;
	m_RoleData += "(";
	itoa(pRoleData->BaseInfo.ientergamex, aStr, 10);
	m_RoleData += aStr;
	m_RoleData += ",";
	itoa(pRoleData->BaseInfo.ientergamey, aStr, 10);
	m_RoleData += aStr;
	m_RoleData += ")";
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.ileadlevel, aStr, 10);
	m_RoleData += "统帅力等级：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	m_RoleData += "-----------------------------\r\n";
	
	itoa(pRoleData->BaseInfo.ipower, aStr, 10);
	m_RoleData += "力量：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.iagility, aStr, 10);
	m_RoleData += "身法：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.iouter, aStr, 10);
	m_RoleData += "外功：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.iinside, aStr, 10);
	m_RoleData += "内功：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.iluck, aStr, 10);
	m_RoleData += "幸运：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.icurlife, aStr, 10);
	m_RoleData += "生命值：";
	m_RoleData += aStr;
	m_RoleData += "/";
	itoa(pRoleData->BaseInfo.imaxlife, aStr, 10);
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.icurinner, aStr, 10);
	m_RoleData += "内力：";
	m_RoleData += aStr;
	m_RoleData += "/";
	itoa(pRoleData->BaseInfo.imaxinner, aStr, 10);
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.icurstamina, aStr, 10);
	m_RoleData += "体力：";
	m_RoleData += aStr;
	m_RoleData += "/";
	itoa(pRoleData->BaseInfo.imaxstamina, aStr, 10);
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.ileftprop, aStr, 10);
	m_RoleData += "剩余属性点：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa(pRoleData->BaseInfo.ileftfight, aStr, 10);
	m_RoleData += "剩余技能点：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	m_RoleData += "-----------------------------\r\n";

	itoa((int)pRoleData->nItemCount, aStr, 10);
	m_RoleData += "物品数：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa((int)pRoleData->nFightSkillCount, aStr, 10);
	m_RoleData += "战斗技能数：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa((int)pRoleData->nTaskCount, aStr, 10);
	m_RoleData += "任务数：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	itoa((int)pRoleData->nFriendCount, aStr, 10);
	m_RoleData += "好友数：";
	m_RoleData += aStr;
	m_RoleData += "\r\n";

	UpdateData(false);
	
	delete [] aBuffer;
	
}

void CDb_CopyRoleDlg::OnBtnRoleCopy() 
{
	UpdateData();
	if(!DBSource)
	{
		AfxMessageBox("源数据库未打开！");
		return;
	}

	if(!DBDesc)
	{
		AfxMessageBox("目标数据库未打开！");
		return;
	}

	if(m_DBSourcePath == "")
	{
		AfxMessageBox("请输入角色名！");
		return;
	}
	int size;
	char* aBuffer = DBSource->search(m_RoleName.GetBuffer(1), strlen(m_RoleName.GetBuffer(1)) + 1, size);

	if(!aBuffer)
	{
		m_RoleData = "未找到人物：" + m_RoleName;
		UpdateData(false);
		return;
	}

	bool aAddResult = DBDesc->add(m_RoleName.GetBuffer(1), strlen(m_RoleName.GetBuffer(1)) + 1,
				aBuffer, size);

	m_RoleData = "复制人物 " + m_RoleName;
	if(aAddResult)
		m_RoleData += " 成功！";
	else
		m_RoleData += " 失败！";

	UpdateData(false);
	
	delete [] aBuffer;
}
