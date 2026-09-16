// DBBackupLoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DBBackupLoad.h"
#include "DBBackupLoadDlg.h"
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
// CDBBackupLoadDlg dialog

CDBBackupLoadDlg::CDBBackupLoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDBBackupLoadDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDBBackupLoadDlg)
	m_txtBackupFile = _T("");
	m_txtDBName = _T("");
	m_txtDBPath = _T("");
	m_DBSavePath = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDBBackupLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBBackupLoadDlg)
	DDX_Control(pDX, IDC_LIST_STATUS, m_listStatus);
	DDX_Text(pDX, IDC_TXT_BACKUPFILE, m_txtBackupFile);
	DDX_Text(pDX, IDC_TXT_DBNAME, m_txtDBName);
	DDX_Text(pDX, IDC_TXT_DBPATH, m_txtDBPath);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDBBackupLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CDBBackupLoadDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_BACKUPFILE, OnBtnBackupfile)
	ON_BN_CLICKED(IDC_BTN_DBLOAD, OnBtnDbload)
	ON_BN_CLICKED(IDC_BTN_DBPATH, OnBtnDbpath)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBBackupLoadDlg message handlers

BOOL CDBBackupLoadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDBBackupLoadDlg::OnPaint() 
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

HCURSOR CDBBackupLoadDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDBBackupLoadDlg::OnBtnBackupfile() 
{
	CFileDialog DlgOpenFile(true,NULL,NULL,OFN_HIDEREADONLY,"数据库备份文件(*.bak)|*.bak",
							NULL);
	if(DlgOpenFile.DoModal() != IDOK)return;
	m_txtBackupFile = DlgOpenFile.GetPathName();
	m_DBSavePath = DlgOpenFile.GetFileTitle();
	UpdateData(false);
}

void CDBBackupLoadDlg::OnBtnDbload() 
{
	UpdateData();
	if(m_txtBackupFile.IsEmpty() || m_txtDBPath.IsEmpty() || m_txtDBName.IsEmpty())
	{
		m_listStatus.AddString("参数填写不完整！");
		return;
	}
	
	//导出数据库打开
	chdir(m_txtDBPath);
	ZDBTable DBTable(m_DBSavePath, m_txtDBName);
	DBTable.addIndex(get_account);
	if(!DBTable.open())
	{
		m_listStatus.AddString("导出数据库打开错误！");
		return;
	}
	//数据库备份文件打开
	CDBLoad DBLoad;
	if(!DBLoad.Open(m_txtBackupFile.GetBuffer(1)))
	{
		m_listStatus.AddString("数据库备份文件打开错误！");
		return;
	}

	
	char aBuffer[64 * 1024] = {0};
	char aKeyBuffer[32] = {0};
	size_t size,keysize;
	int aCount = 0;
	int aErrCount = 0;
	//m_listStatus.AddString("初始化成功，开始导出数据库,请稍等...");
	while(DBLoad.ReadData(aKeyBuffer,keysize,aBuffer,size))
	{
		if(DBTable.add(aKeyBuffer,keysize,aBuffer,size))
		{
			aCount++;
		}
		else
		{//添加数据出错
			CString aMsg = "数据[";
			aMsg += aKeyBuffer;
			aMsg += "]导出错误。";
			m_listStatus.AddString(aMsg);
			aErrCount++;
		}
		
	}
	DBTable.removeLog();
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
	DBLoad.Close();
	DBTable.close();
}

void CDBBackupLoadDlg::OnBtnDbpath() 
{
	UpdateData();
	LPITEMIDLIST pidlRoot=NULL; 
	SHGetSpecialFolderLocation(m_hWnd,CSIDL_DESKTOP,&pidlRoot); 

	BROWSEINFO bi;   //必须传入的参数,下面就是这个结构的参数的初始化 
	//CString strDisplayName;   //用来得到,你选择的活页夹路径,相当于提供一个缓冲区 
	bi.hwndOwner=GetSafeHwnd();   //得到父窗口Handle值 
	bi.pidlRoot=pidlRoot;   //这个变量就是我们在上面得到的. 
	bi.pszDisplayName=NULL;//strDisplayName.GetBuffer(MAX_PATH+1);   //得到缓冲区指针, 
	bi.lpszTitle="浏览文件夹";   //设置标题 
	bi.ulFlags=0;   //设置标志 
	bi.lpfn=NULL; 
	bi.lParam=0; 
	bi.iImage=0;   //上面这个是一些无关的参数的设置,最好设置起来, 
	LPITEMIDLIST pIIL =SHBrowseForFolder(&bi);   //打开对话框 
	
	if(pIIL == NULL)return; 

	TCHAR szInitialDir[MAX_PATH];
	BOOL bRet = ::SHGetPathFromIDList(pIIL, (char*)&szInitialDir);

	m_txtDBPath = szInitialDir;
	UpdateData(false);	
}
