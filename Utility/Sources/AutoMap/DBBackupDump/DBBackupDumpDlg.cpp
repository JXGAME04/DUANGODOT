// DBBackupDumpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DBBackupDump.h"
#include "DBBackupDumpDlg.h"
#include "DBDumpLoad.h"
#include "DBTable.h"
#include "S3DBInterface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int get_account(DB *db, const DBT *pkey, const DBT *pdata, DBT *ikey) 
{
	//给定一个完整的buffer，得到account作为索引
	memset( ikey, 0, sizeof( DBT ) );
	TRoleData *pRoleData = (TRoleData *)pdata->data;

	ikey->data = pRoleData->BaseInfo.caccname;
	ikey->size = strlen( pRoleData->BaseInfo.caccname ) + 1;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDBBackupDumpDlg dialog

CDBBackupDumpDlg::CDBBackupDumpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDBBackupDumpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDBBackupDumpDlg)
	m_txtDBPath = _T("");
	m_txtDBName = _T("");
	m_txtBackupFile = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDBBackupDumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBBackupDumpDlg)
	DDX_Control(pDX, IDC_LIST_STATUS, m_listStatus);
	DDX_Text(pDX, IDC_TXT_DBPATH, m_txtDBPath);
	DDX_Text(pDX, IDC_TXT_DBNAME, m_txtDBName);
	DDX_Text(pDX, IDC_TXT_BACKUPFILE, m_txtBackupFile);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDBBackupDumpDlg, CDialog)
	//{{AFX_MSG_MAP(CDBBackupDumpDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_BACKUPFILE, OnBtnBackupfile)
	ON_BN_CLICKED(IDC_BTN_DBPATH, OnBtnDbpath)
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_BTN_DBDUMP, OnBtnDbdump)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBBackupDumpDlg message handlers

BOOL CDBBackupDumpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDBBackupDumpDlg::OnPaint() 
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

HCURSOR CDBBackupDumpDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDBBackupDumpDlg::OnBtnBackupfile() 
{
	CFileDialog DlgOpenFile(false,NULL,NULL,OFN_HIDEREADONLY,"数据库备份文件(*.bak)|*.bak",
							NULL);
	if(DlgOpenFile.DoModal() != IDOK)return;
	m_txtBackupFile = DlgOpenFile.GetPathName();
	CString aFileExt = DlgOpenFile.GetFileExt();
	if(aFileExt == "") m_txtBackupFile += ".bak";
	UpdateData(false);	
}

void CDBBackupDumpDlg::OnBtnDbpath() 
{
	CFileDialog DlgOpenFile(true,NULL,NULL,OFN_HIDEREADONLY,"数据库文件(*)|*",
							NULL);
	if(DlgOpenFile.DoModal() != IDOK)return;
	char aPathName[MAX_PATH] = {0};
	char aFileName[MAX_PATH] = {0};
	strcpy(aPathName,DlgOpenFile.GetPathName());
	strcpy(aFileName,DlgOpenFile.GetFileName());
	aPathName[strlen(aPathName) - strlen(aFileName)] = '\0';
	m_txtDBPath = aPathName;
	m_txtDBName = aFileName;
	UpdateData(false);

}

void CDBBackupDumpDlg::OnBtnDbdump() 
{
	UpdateData();
	if(m_txtBackupFile.IsEmpty() || m_txtDBPath.IsEmpty() || m_txtDBName.IsEmpty())
	{
		m_listStatus.AddString("参数填写不完整！");
		return;
	}
	
	//分解出绝对路径和数据库工作路径
	char aDBPath[MAX_PATH] = {0};
	char aEvnPath[MAX_PATH] = {0};
	int aSet = 0;
	strcpy(aDBPath, m_txtDBPath);
	char* apt = aDBPath;
	while(*apt != '\0')
	{
		char tmpEvnPath[MAX_PATH] = {0};
		int tmpSet = 0;
		while((*apt != '\\')&&(*apt != '\0'))
		{
			tmpEvnPath[tmpSet] = *apt;
			tmpSet++;
			apt++;
		}
		tmpSet++;
		apt++;
		if(*apt == '\0')
		{
			strcpy(aEvnPath,tmpEvnPath);
			break;
		}
		if(tmpEvnPath[0] != '\0')
		{
			aSet += tmpSet;
			strcpy(aEvnPath,tmpEvnPath);		
		}
		
	}
	aDBPath[aSet] = '\0';
	//导出数据库打开
	chdir(aDBPath);
	ZDBTable DBTable(aEvnPath, m_txtDBName);
	if(!DBTable.open())
	{
		m_listStatus.AddString("导出数据库打开错误！");
		return;
	}
	//数据库备份文件打开
	CDBDump DBDump;
	if(!DBDump.Open(m_txtBackupFile.GetBuffer(1)))
	{
		m_listStatus.AddString("数据库备份文件打开错误！");
		return;
	}

	int aCount = 0;
	int aErrCount = 0;
	//m_listStatus.AddString("初始化成功，开始导出数据库,请稍等...");
	ZCursor* cursor = DBTable.first();
	while(cursor)
	{
		if(DBDump.AddData(cursor->key,cursor->key_size,cursor->data,cursor->size))
		{
			aCount++;
		}
		else
		{//添加数据出错
			CString aMsg = "数据[";
			aMsg += cursor->key;
			aMsg += "]导出错误。";
			m_listStatus.AddString(aMsg);
			aErrCount++;
		}
		if(!DBTable.next(cursor))break;
	}
	char aNumBuf[32] = {0};
	itoa(aCount,aNumBuf,10);
	CString aMsg = "导出完成，共有 ";
	aMsg += aNumBuf;
	aMsg += " 个记录导出。";
	m_listStatus.AddString(aMsg);
	itoa(aErrCount,aNumBuf,10);
	aMsg = "有 ";
	aMsg += aNumBuf;
	aMsg += " 个记录导出失败。";
	m_listStatus.AddString(aMsg);
	DBDump.Close();
	DBTable.close();
	
}
