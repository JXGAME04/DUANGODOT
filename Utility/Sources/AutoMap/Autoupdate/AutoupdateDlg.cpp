// AutoupdateDlg.cpp : implementation file
//
/*
  作者：WPS万里，常齐 
  时间：2003年6 -7月
  版本：1.0
  功能：自动更新界面，通过对话框界面调用动态库UpdateDLL,并通过函数AutoUpdateDLL
  调用接口，动态库通过回调函数RefreshStatus,  来表明自己的状态，界面通过g_dlgInfo 
  来取得自动更新内核的状态。并表现在界面上

*/
#include "stdafx.h"
#include "Autoupdate.h"
#include "AutoupdateDlg.h"

#include "StartDlg.h"
char url[MAX_PATH];

#include "../UPDATEDLL/src/UpdateExport.h"

#include "../engine/Src/KWin32.h"
#include "../engine/Src/KIniFile.h"
#include "../engine/Src/KFilePath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEvent UpdateFinish(FALSE, TRUE, NULL, NULL);
CEvent UpdateCancel(FALSE, TRUE, NULL, NULL);

KUpdateStatusInfo g_dlgInfo;


void DisplayErrorInfo(string& ErrorInfo)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	MessageBox( NULL, ErrorInfo.c_str(),(LPCTSTR)lpMsgBuf , MB_OK |MB_ICONWARNING );
	// Free the buffer.
	LocalFree( lpMsgBuf );
	
	
	
}



KUpdateStatusInfo::KUpdateStatusInfo()
{
	memset((void*)&m_info, 0, sizeof(STATUSINFO));
	m_info.nFinish = -1;
	m_plock = new CSingleLock(&m_csUpdateLock);
}

KUpdateStatusInfo::~KUpdateStatusInfo()
{
	if (m_plock)
	{
		delete m_plock;
	}
}

BOOL KUpdateStatusInfo::Write(const LPSTATUSINFO p)
{
	if (!m_plock->IsLocked())
	{
		m_plock->Lock();
		ASSERT(p);
		memcpy((void*)&m_info, (void*)p, sizeof(STATUSINFO));
		m_plock->Unlock();
		return TRUE;
	}
	
	return FALSE;
}

BOOL KUpdateStatusInfo::Read(LPSTATUSINFO p)
{
	if (!m_plock->IsLocked())
	{
		m_plock->Lock();
		ASSERT(p);
		memcpy((void*)p, (void*)&m_info, sizeof(STATUSINFO));
		m_plock->Unlock();
		return TRUE;
	}
	
	return FALSE;
}

int __stdcall RefreshStatus(int nCurrentStatus, long lParam)
{
	//初始化状态信息Info 
	STATUSINFO Info;
	Info.nTotalPos = -1;
	Info.szTotalStatusInfo[0] = 0;
	Info.nCurrentPos = -1;
	Info.nFinish = -1;
	Info.szCurrentStatusInfo[0] = 0;
	
	LPDOWNLOADFILESTATUS pFileinfo = NULL;
	
	//根据由UpdateSet传经来的状态值nCurrentStatus 对状态信息Info给与相应的状态值
	switch(nCurrentStatus)
	{
	case defUPDATE_STATUS_INITIALIZING:
		strcpy(Info.szTotalStatusInfo, "正在连接服务器...");
		Info.nTotalPos = ConnectionProgress / 5;
		Info.nCurrentPos = lParam;
		break;
	case defUPDATE_STATUS_VERIFING:
		strcpy(Info.szTotalStatusInfo, "正在进行用户校验...");
		Info.nTotalPos = ConnectionProgress * 2 / 5;
		Info.nCurrentPos = lParam;
		break;
	case defUPDATE_RESULT_VERSION_NOT_ENOUGH:
		Info.nFinish = defUPDATE_RESULT_VERSION_NOT_ENOUGH;              
		strcpy(url, (char *)lParam);
		break;
	case defUPDATE_STATUS_PROCESSING_INDEX:
		strcpy(Info.szTotalStatusInfo, "正在下载更新文件的信息...");
		Info.nTotalPos = ConnectionProgress * 4 / 5;
		Info.nCurrentPos = lParam;
		break;
	case defUPDATE_STATUS_DOWNLOADING:
		strcpy(Info.szTotalStatusInfo, "正在下载更新文件...");
		Info.nTotalPos = ConnectionProgress + ((100 - ConnectionProgress) * (4 / 5)) * lParam / 100;
		Info.nCurrentPos = lParam;
		break;
	case defUPDATE_STATUS_DOWNLOADING_FILE:
		strcpy(Info.szTotalStatusInfo, "正在下载更新文件...");
		Info.nTotalPos = -1;
		pFileinfo = (LPDOWNLOADFILESTATUS)lParam;
		if (pFileinfo)
		{
			CString strFile;
			strFile.Format("%s --- %d KB(%d KB)", pFileinfo->strFileName, pFileinfo->dwFileSize / 1024, pFileinfo->dwFileDownloadedSize / 1024);
			strncpy(Info.szCurrentStatusInfo, strFile, MAX_PATH);
			Info.nCurrentPos = (int)(pFileinfo->dwFileDownloadedSize * 100.0 / pFileinfo->dwFileSize);
		}
		break;
	case defUPDATE_STATUS_UPDATING:
		strcpy(Info.szTotalStatusInfo, "正在更新系统...");
		Info.nTotalPos = 100 - (80 + (1/5) * ConnectionProgress);
		Info.nCurrentPos = lParam;
		break;
	case defUPDATE_RESULT_UPDATE_SUCCESS:
		strcpy(Info.szTotalStatusInfo, "已经是最新的版本...");
		Info.nTotalPos = 100;
		Info.nCurrentPos = 100;
		break;
	default:
		ASSERT(FALSE);
	}
	
	
	BOOL bCancel = FALSE;
	bCancel = (::WaitForSingleObject(UpdateCancel.m_hObject, 0) == WAIT_OBJECT_0);
	g_dlgInfo.Write(&Info);
	
	return bCancel;
}

BOOL g_bLog = FALSE;

typedef int (__stdcall *UPDATEA_INIT)(KUPDATE_SETTING UpdateSetting);
typedef int (__stdcall *UPDATE_UNINIT)();
typedef int (__stdcall *UPDATE_START)();
typedef int (__stdcall *UPDATE_CANCEL)();

UPDATEA_INIT g_Update_Init = NULL;
UPDATE_UNINIT g_Update_UnInit = NULL;
UPDATE_START g_Update_Start = NULL;
UPDATE_START g_Update_Cancel = NULL;

UINT AutoUpdateDLL(char* pSite)
{
    KUPDATE_SETTING UpdateSet;
    int nRetCode = 0;
    CString sText;
	
    HMODULE hModule = LoadLibrary("UpdateDLL.dll");
    if (!hModule)
        return FALSE;
	
    g_Update_Init = (UPDATEA_INIT)GetProcAddress(hModule, "Update_Init");
    if (!g_Update_Init)
        return FALSE;
	
    g_Update_UnInit = (UPDATE_UNINIT)GetProcAddress(hModule, "Update_UnInit");
    if (!g_Update_UnInit)
        return FALSE;
	
    g_Update_Start = (UPDATE_START)GetProcAddress(hModule, "Update_Start");
    if (!g_Update_Start)
        return FALSE;
	
	g_Update_Cancel = (UPDATE_START)GetProcAddress(hModule, "Update_Cancel");
    if (!g_Update_Cancel)
        return FALSE;
	
    UpdateSet.bAutoTryNextHost = false;
    UpdateSet.bUseVerify = false;
    UpdateSet.bUseFastestHost = false;
    UpdateSet.pfnCallBackProc = &RefreshStatus;
    UpdateSet.ProxySetting.bUpdateAuth = true; 
    UpdateSet.ProxySetting.nHostPort = 0;
    UpdateSet.ProxySetting.nProxyMethod = PROXY_METHOD_DIRECT;
    strcpy(UpdateSet.szVerifyInfo, "102400-010999-106075-054738");
    UpdateSet.ProxySetting.szHostAddr[0] = '\0';
    UpdateSet.ProxySetting.szPassword[0] = '\0';
    UpdateSet.ProxySetting.szUserName[0] = '\0';
	
    
	char szModulePath[MAX_PATH + 1];
	::GetModuleFileName(NULL, szModulePath, (MAX_PATH + 1));
	char *pszOffset = NULL;
	pszOffset = strrchr(szModulePath, '\\');
    ASSERT(pszOffset);
	strcpy(UpdateSet.szMainExecute, pszOffset + 1);
    pszOffset[1] = '\0';
    strcpy(UpdateSet.szDownloadPath, szModulePath);
	strcat(UpdateSet.szDownloadPath, "Update\\");
    strcpy(UpdateSet.szUpdatePath, szModulePath);
	
	UpdateSet.nUpdateMode = 0;

	
	strncpy(UpdateSet.szUpdateSite, pSite, MAX_PATH);
	
	CString strVersion = szModulePath;
	UpdateSet.nVersion = ::GetPrivateProfileInt(
		"Version",
		"Version",
		0,
		strVersion + "Version.cfg");
	
	CAutoupdateDlg* pDlg = (CAutoupdateDlg*)(AfxGetApp()->m_pMainWnd);
	if (pDlg)
		pDlg->m_strVerson.Format("本地版本 : %d", UpdateSet.nVersion);
	
	UpdateSet.bLog = g_bLog;
	
	if (g_Update_Init == NULL ||
		g_Update_UnInit == NULL ||
		g_Update_Start == NULL)
		goto Exit0;
	
    nRetCode = g_Update_Init(UpdateSet);
    if (nRetCode == defUPDATE_RESULT_INIT_FAILED)
        goto Exit0;
	
    nRetCode = g_Update_Start();
	
    g_Update_UnInit();
	
Exit0:
	STATUSINFO Info;
	g_dlgInfo.Read(&Info);
	if(Info.nFinish == defUPDATE_RESULT_VERSION_NOT_ENOUGH) {
		UpdateFinish.SetEvent();
		return Info.nFinish;
	}
	Info.szTotalStatusInfo[0] = 0;
	Info.nTotalPos = 100;
	Info.nCurrentPos = 100;
	Info.nFinish = nRetCode;
	g_dlgInfo.Write(&Info);
	
	UpdateSet.nVersion = ::GetPrivateProfileInt(
		"Version",
		"Version",
		0,
		strVersion + "Version.cfg");
	if (pDlg)
		pDlg->m_strVerson.Format("本地版本 : %d", UpdateSet.nVersion);

	UpdateFinish.SetEvent();

	g_Update_Cancel = NULL;
	
	if (hModule)
        FreeLibrary(hModule);
	
    return nRetCode;
}

UINT UpdateThread(LPVOID p)
{
    return AutoUpdateDLL((char*)p);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	
	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA
	
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
//{{AFX_MSG_MAP(CAboutDlg)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateDlg dialog
const CString ResFileName = "AutoUpdateRes.dll";
CAutoupdateDlg::CAutoupdateDlg(CWnd* pParent /*=NULL*/)
: CDialog(CAutoupdateDlg::IDD, pParent)

{
	//{{AFX_DATA_INIT(CAutoupdateDlg)
	m_strVerson = _T("");
	m_strProgress = _T("");
	m_strProgressCurrent = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	
	
	m_nCurrentHost = -1;

	m_ResModule = LoadLibrary(ResFileName);
	

	if(!m_ResModule)
	{
		DisplayErrorInfo(string("LoadLibrary()"));
	}

	m_bEnableRun = FALSE;

}

CAutoupdateDlg::~CAutoupdateDlg()
{
	if(m_ResModule)
	{
		
		if(!FreeLibrary(m_ResModule))
		{                           
			DisplayErrorInfo(string("FreeLibrary()"));
			
		}

        m_ResModule = NULL;
	}
}

void CAutoupdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoupdateDlg)
	DDX_Control(pDX, IDC_Advertisement, m_AdvertisementCtl);
	DDX_Control(pDX, IDC_CONFIG, m_GameOption);
	DDX_Control(pDX, IDC_PROGRESSCURRENT, m_progressCurrent);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Control(pDX, IDC_STATIC_PROGRESS, m_DisplayProgressCtl);
	DDX_Control(pDX, IDC_STATIC_PROGRESSCURRENT, m_DisplayCurrentCtl);
	DDX_Control(pDX, IDC_STATIC_VERSON, m_DisplayVersionCtl);
	DDX_Control(pDX, IDC_REGISTER, m_RegisterCtl);
	DDX_Control(pDX, IDC_INVEST, m_InvestCtl);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDOK, m_EnterGameButton);
	DDX_Text(pDX, IDC_STATIC_VERSON, m_strVerson);
	DDX_Text(pDX, IDC_STATIC_PROGRESS, m_strProgress);
	DDX_Text(pDX, IDC_STATIC_PROGRESSCURRENT, m_strProgressCurrent);
	DDX_Control(pDX, IDC_EXPLORER1, m_web);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAutoupdateDlg, CDialog)
//{{AFX_MSG_MAP(CAutoupdateDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_REGISTER, OnRegister)
	ON_BN_CLICKED(IDC_INVEST, OnInvest)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CONFIG, OnConfig)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateDlg message handlers
const WindowWidth = 643;
const WindowHeight = 530;

BOOL CAutoupdateDlg::OnInitDialog()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	CDialog::OnInitDialog();

	
	if (!m_strWebURL.IsEmpty())
	{
//		int result = 
		m_web.Navigate(m_strWebURL, NULL, NULL, NULL, NULL);
//		if(result != S_OK) {
//			result = m_web.Navigate("http://www.jxonline.net", NULL, NULL, NULL, NULL);
//		}
	}
	
	m_progress.SetRange(0, 100);
	m_progressCurrent.SetRange(0, 100);
	
	// Add "About..." menu item to system menu.
	
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	ASSERT_VALID(this);
	
	
	
    CRect WindowRange(0,0,640,480);
	MoveWindow(WindowRange);
	
	
	

	CenterWindow(NULL);
	
	
	
	
	InitButton();
	InitProgress();
	InitStatic();

    


	m_web.MoveWindow(0,129 - 18 ,635,226 + 37);


	LoadBitMapFromInstance(m_bmTitle, m_ResModule, IDB_TITLE);
	if (!((HBITMAP)m_bmTitle))
	{
		DisplayErrorInfo(string("LoadBitMapFromInstance"));
	    PostQuitMessage(1);
	}
	


	

	UpdateFinish.SetEvent();
	UpdateFromCurrentHost();
	
	SetTimer(200, 50, NULL);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CAutoupdateDlg::InitAutoUpdate()
{
	KIniFile	IniFile;
	if (!IniFile.Load("\\settings\\autoupdate.ini"))
		return FALSE;
	
	char szWebURL[MAX_PATH];
	IniFile.GetString("main", "gameurl", "http://www.kingsoft.net/", szWebURL, MAX_PATH);
	m_strWebURL = szWebURL;
	
	char run[256];
	IniFile.GetString("main", "game", "s3client.exe", run, 256);
	
	char szModulePath[MAX_PATH + 1];
	g_GetRootPath(szModulePath);
	CString strPath = szModulePath;
	
	m_strRun = strPath + "\\" + run;
	
	m_strUpdateSelf = strPath + "\\Update\\" + "UpdateSelf.DAT";
	
	char szSite[MAX_PATH];
	int n = 1;
	CString strSite;
	CStringArray strHosts;
	do 
	{
		szSite[0] = 0;
		strSite.Format("ftpsite%d", n);
		IniFile.GetString("main", strSite, "", szSite, MAX_PATH);
		if (szSite[0] != 0)
			strHosts.Add(szSite);
		else
			break;
		n++;
	} while(szSite[0] != 0);
	
	if (strHosts.GetSize() > 0)
	{
		int nStart = rand() * GetTickCount() % strHosts.GetSize();
		int i;
		for (i = nStart; i < strHosts.GetSize(); i++)
		{
			m_strHosts.Add(strHosts[i]);
		}
		for (i = 0; i < nStart; i++)
		{
			m_strHosts.Add(strHosts[i]);
		}
		ASSERT(m_strHosts.GetSize() == strHosts.GetSize());
		m_nCurrentHost = 0;

		m_nConnectionStep = ConnectionProgress/(m_strHosts.GetSize());
	}
	else
		m_nCurrentHost = -1;
	
	IniFile.GetInteger("main", "log", 0, &g_bLog);


	
	
	return (m_nCurrentHost >= 0);
}

void CAutoupdateDlg::UpdateFromCurrentHost()
{
	
	if (m_nCurrentHost >= 0 && m_nCurrentHost < m_strHosts.GetSize() &&
		WaitForSingleObject(UpdateFinish.m_hObject, 0) == WAIT_OBJECT_0)
	{
		UpdateFinish.ResetEvent();
		STATUSINFO Info;
		Info.nTotalPos = -1;
		Info.szTotalStatusInfo[0] = 0;
		Info.nCurrentPos = -1;
		Info.nFinish = -1;
		Info.szCurrentStatusInfo[0] = 0;
		g_dlgInfo.Write(&Info);
		AfxBeginThread(UpdateThread, (void*)(LPCTSTR)m_strHosts[m_nCurrentHost]);
		m_nCurrentHost++;
	}
}

void CAutoupdateDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAutoupdateDlg::OnPaint() 
{

	CPaintDC PaintDC(this); 
	
	if (IsIconic())
	{
		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		
		// Draw the icon
		PaintDC.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialog::OnPaint();
	
		//画标题
        CDC dc;
        CBitmap *pOldBitmap = NULL;
		dc.CreateCompatibleDC(&PaintDC);
		
        
        pOldBitmap = dc.SelectObject(&m_bmTitle);

		PaintDC.BitBlt(0, 0, 640, 107, &dc, 0, 0, SRCCOPY);

        dc.SelectObject(pOldBitmap);

       
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAutoupdateDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CAutoupdateDlg::OnRefresh(LPSTATUSINFO pInfo)
{
	ASSERT(pInfo);
	BOOL bEnableRun = FALSE;
	BOOL bClose = FALSE;
	if (pInfo->nFinish < 0)
	{
		if (pInfo->nCurrentPos >= 0)
		{
			if(m_progressCurrent.GetPos() != pInfo->nCurrentPos)
			{
				m_progressCurrent.SetPos(pInfo->nCurrentPos);
				
			}
		}
		if (pInfo->szCurrentStatusInfo[0] != 0)
			m_strProgressCurrent = pInfo->szCurrentStatusInfo;

		if (pInfo->nTotalPos >= 0)
		{
			int TotalProgress = 0;//总进度
			//如果进度小于连接进度，则此进度为单服务器的连接进度,要转换为总连接
			//进度。如果进度大于连接进度则服务器已连上，进度就为整体进度
			if (pInfo->nTotalPos < ConnectionProgress)
			{
				TotalProgress = 
                    m_nCurrentHost  * 
                    m_nConnectionStep +  
					m_nConnectionStep * 
                    (ConnectionProgress / 100 *  pInfo->nTotalPos / 100); 


			}
			else
			{
				TotalProgress = pInfo->nTotalPos;

			}

			if(m_progress.GetPos() != TotalProgress)
			{
				
				m_progress.SetPos(TotalProgress);
			}
		}	
		if (pInfo->szTotalStatusInfo[0] != 0)
		{
			m_strProgress = pInfo->szTotalStatusInfo;
		}
	}
	else 
		
		if (pInfo->nFinish == defUPDATE_RESULT_CANCEL)
	{
		m_progressCurrent.SetPos(0);
		m_strProgressCurrent.Empty();
		m_progress.SetPos(0);
		m_strProgress = "自动更新被取消 ！";
	}
	else
	{ 
		if (pInfo->nFinish == defUPDATE_RESULT_UPDATE_SUCCESS ||
			pInfo->nFinish == defUPDATE_RESULT_NOT_UPDATE_FILE ||
			pInfo->nFinish == defUPDATE_RESULT_UPDATE_SUCCESS_NEED_REBOOT ||
			pInfo->nFinish == defUPDATE_RESULT_UPDATESELF_SUCCESS)
		{
			m_progressCurrent.SetPos(0);
			m_strProgressCurrent.Empty();
			m_progress.SetPos(100);
			m_strProgress = "自动更新完成 ！";
			
			bClose = TRUE;
			
			if (pInfo->nFinish == defUPDATE_RESULT_UPDATE_SUCCESS ||
				pInfo->nFinish == defUPDATE_RESULT_NOT_UPDATE_FILE)
				bEnableRun = TRUE;
		}
		else
		{
			m_progressCurrent.SetPos(0);
			m_strProgressCurrent.Empty();
			m_progress.SetPos(0);
			if (pInfo->nFinish == defUPDATE_RESULT_INIT_FAILED)
				m_strProgress = "自动更新失败--初始化失败 ！";
			else if (pInfo->nFinish == defUPDATE_RESULT_DOWNLOAD_INDEX_FAILED)
				m_strProgress = "当前服务器，无更新信息";
			else if (pInfo->nFinish == defUPDATE_RESULT_PROCESS_INDEX_FAILED)
				m_strProgress = "当前服务器，更新信息无效！";
			
			else if (pInfo->nFinish == defUPDATE_RESULT_VERSION_NOT_ENOUGH)
			{
				//m_strProgress = "当前服务器，更新信息无效！";
 
				AnnuncePanel NeedDownAnnuncePanel;
				strcpy(NeedDownAnnuncePanel.url, url);
				KillTimer(200);
				NeedDownAnnuncePanel.DoModal();
				CDialog::OnCancel();
				m_web.Stop();
			}
			else if (pInfo->nFinish == defUPDATE_RESULT_CONNECT_SERVER_FAILED)
				m_strProgress = "连接当前服务器失败 ！";
			else if (pInfo->nFinish == defUPDATE_RESULT_UPDATE_FAILED)
				m_strProgress = "从当前服务器，更新文件失败 ！";
			else if (pInfo->nFinish == defUPDATE_RESULT_DOWNLOAD_FAILED)
				m_strProgress = "从当前服务器，下载文件不全 ！";
			else
				m_strProgress = "从当前服务器，自动更新失败 ！";			
			
			
			bClose = TRUE;
		}
	}

	CWnd* p = GetDlgItem(IDCANCEL);
	
	CString ButtonText;
	p->GetWindowText(ButtonText);
	
	
	if (bClose)
	{
		

		

		
		if(ButtonText != CString("关闭"))
		{			
			p->SetWindowText("关闭");

		

			
		}
		
			
	}
	else
	{
		if(ButtonText != CString("取消"))
		{
			p->SetWindowText("取消");
			
		}
		
	}
	
	p = GetDlgItem(IDOK);
	
	if(m_bEnableRun != bEnableRun)
	{
		
		
		if (bEnableRun)
		{
			p->EnableWindow(TRUE);
			
			p->SetFocus();
			
			
		}
		else
		{
			CWnd* p = GetDlgItem(IDCANCEL);
			p->SetFocus();
			
		}
	}
	m_bEnableRun = bEnableRun;
/*	CRect rcText;
	CWnd* pWnd = GetDlgItem(IDC_STATIC_PROGRESS);
	pWnd->GetWindowRect(&rcText);
    
	
	pWnd  = GetDlgItem(IDC_STATIC_VERSON);
	pWnd->GetWindowRect(&rcText);
	ScreenToClient( &rcText );
	//InvalidateRect(&rcText, TRUE);
	
	pWnd = GetDlgItem(IDC_STATIC_PROGRESS);
	pWnd->GetWindowRect(&rcText);
	ScreenToClient( &rcText );
	//InvalidateRect(&rcText, TRUE);
	
	pWnd = GetDlgItem(IDC_STATIC_PROGRESSCURRENT);
	pWnd->GetWindowRect(&rcText);
	ScreenToClient( &rcText );
	//Invalidate();//&rcText, TRUE);
*/
	UpdateData(FALSE);

}

void CAutoupdateDlg::OnCancel()
{
	m_web.Stop();
	
	CWnd* p = GetDlgItem(IDCANCEL);
	CString str;
	p->GetWindowText(str);
	if (str.CompareNoCase("取消") == 0)
	{
		UpdateCancel.SetEvent();
		if (g_Update_Cancel)
			g_Update_Cancel();
	}
	else if (str.CompareNoCase("关闭") == 0)
		CDialog::OnCancel();
	else
		ASSERT(0);
}

void CAutoupdateDlg::OnDestroy()
{
	WaitForSingleObject(UpdateFinish.m_hObject, INFINITE);
	
	CDialog::OnDestroy();
}

BOOL OnExecute(LPCSTR szFile)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
	::GetStartupInfo(&si);
    ZeroMemory( &pi, sizeof(pi) );
	
	char buffer[256];
	
	strcpy(buffer, szFile);
	
	BOOL bRet = TRUE;
	
    // Start the child process. 
    if( !CreateProcess( NULL, // No module name (use command line). 
		buffer,				// Command line. 
		NULL,				// Process handle not inheritable. 
		NULL,				// Thread handle not inheritable. 
		FALSE,				// Set handle inheritance to FALSE. 
		0,					// No creation flags. 
		NULL,				// Use parent's environment block. 
		NULL,				// Use parent's starting directory. 
		&si,				// Pointer to STARTUPINFO structure.
		&pi )				// Pointer to PROCESS_INFORMATION structure.
		) 
    {
		bRet = FALSE;
    }
	
	
	// Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
	
	return bRet;
}

void CAutoupdateDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 200)
	{
		STATUSINFO info;
		if (g_dlgInfo.Read(&info))
		{
			OnRefresh(&info);
			if (info.nFinish >= 0 &&
				(info.nFinish == defUPDATE_RESULT_INIT_FAILED ||
				info.nFinish == defUPDATE_RESULT_DOWNLOAD_INDEX_FAILED ||
				info.nFinish == defUPDATE_RESULT_PROCESS_INDEX_FAILED ||
				info.nFinish == defUPDATE_RESULT_CONNECT_SERVER_FAILED ||
				info.nFinish == defUPDATE_RESULT_DOWNLOAD_FAILED
				)
				)
			{
				UpdateFromCurrentHost();
			}
			else if (info.nFinish == defUPDATE_RESULT_CANCEL)
			{
				m_web.Stop();
				CDialog::OnCancel();
			}
			else if (info.nFinish == defUPDATE_RESULT_UPDATESELF_SUCCESS)
			{
				if (!m_strUpdateSelf.IsEmpty())
					OnExecute(m_strUpdateSelf);
				m_web.Stop();
				CDialog::OnOK();
			}
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

void CAutoupdateDlg::OnOK()
{
	if (!m_strRun.IsEmpty())
		OnExecute(m_strRun);
	
	CDialog::OnOK();
}

//DEL HRGN CAutoupdateDlg::BitmapToRegionFromHSBRange(HBITMAP& hBmp, Color cTransparentColor,float Hrange , float SRange,float Brange)
//DEL {
//DEL 	HRGN hRgn = NULL;
//DEL 	
//DEL 	if (hBmp)
//DEL 	{
//DEL 		// Create a memory DC inside which we will scan the bitmap content
//DEL 		HDC hMemDC = CreateCompatibleDC(NULL);
//DEL 		if (hMemDC)
//DEL 		{
//DEL 			// Get bitmap size
//DEL 			BITMAP bm;
//DEL 			GetObject(hBmp, sizeof(bm), &bm);
//DEL 			
//DEL 			// Create a 32 bits depth bitmap and select it into the memory DC 
//DEL 			BITMAPINFOHEADER RGB32BITSBITMAPINFO = {	
//DEL 				sizeof(BITMAPINFOHEADER),	// biSize 
//DEL 					bm.bmWidth,					// biWidth; 
//DEL 					bm.bmHeight,				// biHeight; 
//DEL 					1,							// biPlanes; 
//DEL 					32,							// biBitCount 
//DEL 					BI_RGB,						// biCompression; 
//DEL 					0,							// biSizeImage; 
//DEL 					0,							// biXPelsPerMeter; 
//DEL 					0,							// biYPelsPerMeter; 
//DEL 					0,							// biClrUsed; 
//DEL 					0							// biClrImportant; 
//DEL 			};
//DEL 			VOID * pbits32; 
//DEL 			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
//DEL 			if (hbm32)
//DEL 			{
//DEL 				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);
//DEL 				
//DEL 				// Create a DC just to copy the bitmap into the memory DC
//DEL 				HDC hDC = CreateCompatibleDC(hMemDC);
//DEL 				if (hDC)
//DEL 				{
//DEL 					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
//DEL 					BITMAP bm32;
//DEL 					GetObject(hbm32, sizeof(bm32), &bm32);
//DEL 					while (bm32.bmWidthBytes % 4)
//DEL 						bm32.bmWidthBytes++;
//DEL 					
//DEL 					// Copy the bitmap into the memory DC
//DEL 					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
//DEL 					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);
//DEL 					
//DEL 					// For better performances, we will use the ExtCreateRegion() function to create the
//DEL 					// region. This function take a RGNDATA structure on entry. We will add rectangles by
//DEL 					// amount of ALLOC_UNIT number in this structure.
//DEL #define ALLOC_UNIT	100
//DEL 					DWORD maxRects = ALLOC_UNIT;
//DEL 					HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
//DEL 					RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
//DEL 					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
//DEL 					pData->rdh.iType = RDH_RECTANGLES;
//DEL 					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
//DEL 					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
//DEL 					
//DEL 					// Keep on hand highest and lowest values for the "transparent" pixels
//DEL 					float lH  = cTransparentColor.GetH() - Hrange/2;
//DEL 					float hH  = cTransparentColor.GetH() + Hrange/2;
//DEL 					
//DEL 					float lS = cTransparentColor.GetS() - SRange/2;
//DEL 					float lB = cTransparentColor.GetB() - Brange/2;
//DEL 					
//DEL 					float hS = cTransparentColor.GetS() + SRange/2;
//DEL 					float hB = cTransparentColor.GetS() + Brange/2;
//DEL 					
//DEL 					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
//DEL 					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
//DEL 					for (int y = 0; y < bm.bmHeight; y++)
//DEL 					{
//DEL 						// Scan each bitmap pixel from left to right
//DEL 						for (int x = 0; x < bm.bmWidth; x++)
//DEL 						{
//DEL 							// Search for a continuous range of "non transparent pixels"
//DEL 							int x0 = x;
//DEL 							LONG *p = (LONG *)p32 + x;
//DEL 							while (x < bm.bmWidth)
//DEL 							{
//DEL 							/*	BYTE b = GetRValue(*p);
//DEL 							if (b >= lr && b <= hr)
//DEL 							{
//DEL 							b = GetGValue(*p);
//DEL 							if (b >= lg && b <= hg)
//DEL 							{
//DEL 							b = GetBValue(*p);
//DEL 							if (b >= lb && b <= hb)
//DEL 							// This pixel is "transparent"
//DEL 							break;
//DEL 							}
//DEL 							
//DEL 							  
//DEL 								}
//DEL 								*/	
//DEL 								BYTE  r = GetRValue(*p);
//DEL 								BYTE  g = GetGValue(*p);
//DEL 								BYTE  b = GetBValue(*p);
//DEL 								
//DEL 								Color color(r,g,b);
//DEL 								
//DEL 								
//DEL 								if(((color.GetH() > lH) && (color.GetH() < hH)) && (color.GetS() > lS && color.GetS() < hS ) && (color.GetB() > lB && color.GetB() < hB ) )
//DEL 								{
//DEL 									//透明的颜色
//DEL 									break;
//DEL 								}
//DEL 								
//DEL 								p++;
//DEL 								x++;
//DEL 							}
//DEL 							
//DEL 							if (x > x0)
//DEL 							{
//DEL 								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
//DEL 								if (pData->rdh.nCount >= maxRects)
//DEL 								{
//DEL 									GlobalUnlock(hData);
//DEL 									maxRects += ALLOC_UNIT;
//DEL 									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
//DEL 									pData = (RGNDATA *)GlobalLock(hData);
//DEL 								}
//DEL 								RECT *pr = (RECT *)&pData->Buffer;
//DEL 								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
//DEL 								if (x0 < pData->rdh.rcBound.left)
//DEL 									pData->rdh.rcBound.left = x0;
//DEL 								if (y < pData->rdh.rcBound.top)
//DEL 									pData->rdh.rcBound.top = y;
//DEL 								if (x > pData->rdh.rcBound.right)
//DEL 									pData->rdh.rcBound.right = x;
//DEL 								if (y+1 > pData->rdh.rcBound.bottom)
//DEL 									pData->rdh.rcBound.bottom = y+1;
//DEL 								pData->rdh.nCount++;
//DEL 								
//DEL 								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
//DEL 								// large (ie: > 4000). Therefore, we have to create the region by multiple steps.
//DEL 								if (pData->rdh.nCount == 2000)
//DEL 								{
//DEL 									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
//DEL 									if (hRgn)
//DEL 									{
//DEL 										CombineRgn(hRgn, hRgn, h, RGN_OR);
//DEL 										DeleteObject(h);
//DEL 									}
//DEL 									else
//DEL 										hRgn = h;
//DEL 									pData->rdh.nCount = 0;
//DEL 									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
//DEL 								}
//DEL 							}
//DEL 						}
//DEL 						
//DEL 						// Go to next row (remember, the bitmap is inverted vertically)
//DEL 						p32 -= bm32.bmWidthBytes;
//DEL 					}
//DEL 					
//DEL 					// Create or extend the region with the remaining rectangles
//DEL 					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
//DEL 					if (hRgn)
//DEL 					{
//DEL 						CombineRgn(hRgn, hRgn, h, RGN_OR);
//DEL 						DeleteObject(h);
//DEL 					}
//DEL 					else
//DEL 						hRgn = h;
//DEL 					
//DEL 					// Clean up
//DEL 					GlobalFree(hData);
//DEL 					SelectObject(hDC, holdBmp);
//DEL 					DeleteDC(hDC);
//DEL 				}
//DEL 				
//DEL 				DeleteObject(SelectObject(hMemDC, holdBmp));
//DEL 			}
//DEL 			
//DEL 			DeleteDC(hMemDC);
//DEL 		}	
//DEL 	}
//DEL 	return hRgn;	
//DEL  }
 
 void CAutoupdateDlg::InitButton()
 {
	 /*m_EnterGameButton.SetButtonBmp(IDB_ENTERD,IDB_ENTERN,IDB_ENTERF,AfxGetApp()->m_hInstance);
	 m_CancelButton.SetButtonBmp(IDB_CANCELD,IDB_CANCELN,IDB_CANCELF,AfxGetApp()->m_hInstance);
	 m_RegisterCtl.SetButtonBmp(IDB_REGISTERD,IDB_REGISTERN,IDB_REGISTERF,AfxGetApp()->m_hInstance);
	 m_InvestCtl.SetButtonBmp(IDB_INVESTD,IDB_INVESTN,IDB_INVESTF,AfxGetApp()->m_hInstance);
	 */
	
	 m_EnterGameButton.MoveWindow(15,417,90,24);
	 m_RegisterCtl.MoveWindow(142,417,90,24);
	 m_InvestCtl.MoveWindow(279,417,90,24);
	 m_CancelButton.MoveWindow(406,417,90,24);
	 m_GameOption.MoveWindow(533,417,90,24);
 

	 
	 
	 
	 
 }
 
//DEL  void CAutoupdateDlg::DrawAText(CDC * DeviceHandle,int nHeight, int nWidth, int nOrientation, int nWeight, DWORD dwItalic, DWORD dwUnderline, DWORD dwStrikeOut, const CString &CSrFace, COLORREF colColor, COLORREF colBackColor, int nBackMode,const CString &CSTextString,CRect& Range)
//DEL  {
//DEL 	 HFONT  TextFont = CreateFont(nHeight , nWidth , nOrientation ,nOrientation , nWeight, dwItalic, dwUnderline, dwStrikeOut, DEFAULT_CHARSET ,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH ,CSrFace);
//DEL 	 
//DEL 	 
//DEL 	 HGDIOBJ  OldFont      = DeviceHandle->SelectObject(TextFont);
//DEL 	 COLORREF OldBkColor   = DeviceHandle->SetBkColor(colBackColor);
//DEL 	 COLORREF OldTextColor = DeviceHandle->SetTextColor(colColor);
//DEL 	 
//DEL 	 int      OldTextMode  = DeviceHandle->SetBkMode(nBackMode);
//DEL 	 
//DEL 	 
//DEL 	 
//DEL 	 
//DEL 	 DeviceHandle->DrawText(CSTextString,Range,DT_LEFT);
//DEL 	 
//DEL 	 DeviceHandle->SetBkMode(OldTextMode); 
//DEL 	 DeviceHandle->SetTextColor(OldTextColor); 
//DEL 	 DeviceHandle->SetBkColor(OldBkColor); 
//DEL 	 DeviceHandle->SelectObject(OldFont);
//DEL 	 
//DEL 	 DeleteObject(TextFont);
//DEL 	 
//DEL  }
 

 
 
 
  void CAutoupdateDlg::InitProgress()
  {
 	// m_progress.SetForegroundBmp(IDB_PROGRESS);
 	// m_progress.SetBackColor(0x00210D0B);
      m_progress.MoveWindow(5,390 ,280,15);
 
 
 	
 	 
 	 
 	 
 	 
 //	 m_progressCurrent.SetForegroundBmp(IDB_PROGRESS);
 //	 m_progressCurrent.SetBackColor(0x00210D0B);
      m_progressCurrent.MoveWindow(345, 390 ,280,15);
 	 
 	 
 	 
 	 
 }
 
//DEL  HRGN CAutoupdateDlg::BitmapToRegionFromAlphaValueRange(HBITMAP &hBmp, unsigned char ColorValueRange)
//DEL  {
//DEL 	 HRGN hRgn = NULL;
//DEL 	 
//DEL 	 if (hBmp)
//DEL 	 {
//DEL 		 // Create a memory DC inside which we will scan the bitmap content
//DEL 		 HDC hMemDC = CreateCompatibleDC(NULL);
//DEL 		 if (hMemDC)
//DEL 		 {
//DEL 			 // Get bitmap size
//DEL 			 BITMAP bm;
//DEL 			 GetObject(hBmp, sizeof(bm), &bm);
//DEL 			 
//DEL 			 // Create a 32 bits depth bitmap and select it into the memory DC 
//DEL 			 BITMAPINFOHEADER RGB32BITSBITMAPINFO = {	
//DEL 				 sizeof(BITMAPINFOHEADER),	// biSize 
//DEL 					 bm.bmWidth,					// biWidth; 
//DEL 					 bm.bmHeight,				// biHeight; 
//DEL 					 1,							// biPlanes; 
//DEL 					 32,							// biBitCount 
//DEL 					 BI_RGB,						// biCompression; 
//DEL 					 0,							// biSizeImage; 
//DEL 					 0,							// biXPelsPerMeter; 
//DEL 					 0,							// biYPelsPerMeter; 
//DEL 					 0,							// biClrUsed; 
//DEL 					 0							// biClrImportant; 
//DEL 			 };
//DEL 			 VOID * pbits32; 
//DEL 			 HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
//DEL 			 if (hbm32)
//DEL 			 {
//DEL 				 HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);
//DEL 				 
//DEL 				 // Create a DC just to copy the bitmap into the memory DC
//DEL 				 HDC hDC = CreateCompatibleDC(hMemDC);
//DEL 				 if (hDC)
//DEL 				 {
//DEL 					 // Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
//DEL 					 BITMAP bm32;
//DEL 					 GetObject(hbm32, sizeof(bm32), &bm32);
//DEL 					 while (bm32.bmWidthBytes % 4)
//DEL 						 bm32.bmWidthBytes++;
//DEL 					 
//DEL 					 // Copy the bitmap into the memory DC
//DEL 					 HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
//DEL 					 BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);
//DEL 					 
//DEL 					 // For better performances, we will use the ExtCreateRegion() function to create the
//DEL 					 // region. This function take a RGNDATA structure on entry. We will add rectangles by
//DEL 					 // amount of ALLOC_UNIT number in this structure.
//DEL #define ALLOC_UNIT	 1
//DEL 					 DWORD maxRects = ALLOC_UNIT;
//DEL 					 HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
//DEL 					 RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
//DEL 					 pData->rdh.dwSize = sizeof(RGNDATAHEADER);
//DEL 					 pData->rdh.iType = RDH_RECTANGLES;
//DEL 					 pData->rdh.nCount = pData->rdh.nRgnSize = 0;
//DEL 					 SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
//DEL 					 
//DEL 					 
//DEL 					 long MaxColor = ColorValueRange;
//DEL 					 // Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
//DEL 					 BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
//DEL 					 for (int y = 0; y < bm.bmHeight; y++)
//DEL 					 {
//DEL 						 // Scan each bitmap pixel from left to right
//DEL 						 for (int x = 0; x < bm.bmWidth; x++)
//DEL 						 {
//DEL 							 // Search for a continuous range of "non transparent pixels"
//DEL 							 int x0 = x;
//DEL 							 LONG *p = (LONG *)p32 + x;
//DEL 							 while (x < bm.bmWidth)
//DEL 							 {
//DEL 								 BYTE b = GetRValue(*p);
//DEL 								 if (b <= MaxColor)
//DEL 								 {
//DEL 									 b = GetGValue(*p);
//DEL 									 if (b <= MaxColor)
//DEL 									 {
//DEL 										 b = GetBValue(*p);
//DEL 										 if (b <= MaxColor)
//DEL 										 {	 // This pixel is "transparent"
//DEL 											 break;
//DEL 										 }
//DEL 									 }
//DEL 									 
//DEL 								 }
//DEL 								 
//DEL 									 p++;
//DEL 									 x++;
//DEL 								 }
//DEL 								 
//DEL 								 if (x > x0)
//DEL 								 {
//DEL 									 // Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
//DEL 									 if (pData->rdh.nCount >= maxRects)
//DEL 									 {
//DEL 										 GlobalUnlock(hData);
//DEL 										 maxRects += ALLOC_UNIT;
//DEL 										 hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
//DEL 										 pData = (RGNDATA *)GlobalLock(hData);
//DEL 									 }
//DEL 									 RECT *pr = (RECT *)&pData->Buffer;
//DEL 									 SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
//DEL 									 if (x0 < pData->rdh.rcBound.left)
//DEL 										 pData->rdh.rcBound.left = x0;
//DEL 									 if (y < pData->rdh.rcBound.top)
//DEL 										 pData->rdh.rcBound.top = y;
//DEL 									 if (x > pData->rdh.rcBound.right)
//DEL 										 pData->rdh.rcBound.right = x;
//DEL 									 if (y+1 > pData->rdh.rcBound.bottom)
//DEL 										 pData->rdh.rcBound.bottom = y+1;
//DEL 									 pData->rdh.nCount++;
//DEL 									 
//DEL 									 // On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
//DEL 									 // large (ie: > 4000). Therefore, we have to create the region by multiple steps.
//DEL 									 if (pData->rdh.nCount == 2000)
//DEL 									 {
//DEL 										 HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
//DEL 										 if (hRgn)
//DEL 										 {
//DEL 											 CombineRgn(hRgn, hRgn, h, RGN_OR);
//DEL 											 DeleteObject(h);
//DEL 										 }
//DEL 										 else
//DEL 											 hRgn = h;
//DEL 										 pData->rdh.nCount = 0;
//DEL 										 SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
//DEL 									 }
//DEL 								 }
//DEL 							 }
//DEL 							 
//DEL 							 // Go to next row (remember, the bitmap is inverted vertically)
//DEL 							 p32 -= bm32.bmWidthBytes;
//DEL 						 }
//DEL 						 
//DEL 						 // Create or extend the region with the remaining rectangles
//DEL 						 HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
//DEL 						 if (hRgn)
//DEL 						 {
//DEL 							 CombineRgn(hRgn, hRgn, h, RGN_OR);
//DEL 							 DeleteObject(h);
//DEL 						 }
//DEL 						 else
//DEL 							 hRgn = h;
//DEL 						 
//DEL 						 // Clean up
//DEL 						 GlobalFree(hData);
//DEL 						 SelectObject(hDC, holdBmp);
//DEL 						 DeleteDC(hDC);
//DEL 					 }
//DEL 					 
//DEL 					 DeleteObject(SelectObject(hMemDC, holdBmp));
//DEL 			}
//DEL 			
//DEL 			DeleteDC(hMemDC);
//DEL 		}	
//DEL 	}
//DEL 	return hRgn;	
//DEL 	
//DEL 
//DEL }




/*void CAutoupdateDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	//如果左键已经按下，开始窗口的移动
	if(nFlags&&MK_MBUTTON)
	{
		SetCapture();
	
		CRect WindowRect;
		GetClientRect(WindowRect);

	
        CPoint WindowMove = point - m_MouseClickPosition ;
		
	

		ClientToScreen(&WindowMove);


		this->MoveWindow(WindowMove.x ,WindowMove.y,WindowRect.right,WindowRect.bottom,TRUE);
	    
	
	
	}	

	CDialog::OnMouseMove(nFlags, point);
}
*/
/*void CAutoupdateDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	//终止对鼠标的捕获
	ReleaseCapture();
	CDialog::OnLButtonUp(nFlags, point);
}
*/


/*void CAutoupdateDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_MouseClickPosition = point;
	CDialog::OnLButtonDown(nFlags, point);
}
*/



BEGIN_EVENTSINK_MAP(CAutoupdateDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CAutoupdateDlg)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()


void CAutoupdateDlg::InitStatic()
{
	m_DisplayVersionCtl.MoveWindow(5, 375 ,145,15);
	m_DisplayProgressCtl.MoveWindow(150 ,375 ,155,15);
	m_DisplayCurrentCtl.MoveWindow(355 ,375 ,290,15);
	m_AdvertisementCtl.MoveWindow(553,443,90,24);
	
	
//	m_DisplayCurrentCtl.GetDC()->SetTextColor(RGB(102,101,98));


	
	
	
	
	
	
	
}

void CAutoupdateDlg::OnRegister() 
{
	// TODO: Add your control notification handler code here
	ShellExecute(NULL,"open","\\\\s\\自动更新\\剑侠情缘3网上交易平台.htm",NULL,NULL,SW_SHOW);
}

void CAutoupdateDlg::OnInvest() 
{
	// TODO: Add your control notification handler code here
	ShellExecute(NULL,"open","\\\\s\\自动更新\\剑侠情缘3网上交易平台.htm",NULL,NULL,SW_SHOW);
}

/*void CAutoupdateDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CAutoupdateDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnLButtonDblClk(nFlags, point);
}
*/




//DEL void CAutoupdateDlg::ClientToInfoRegion(CRect &ClientRegion)
//DEL {
//DEL 	int Height = ClientRegion.bottom - ClientRegion.top;
//DEL 	ClientRegion.top  = 0;
//DEL 	ClientRegion.bottom = ClientRegion.top + Height;
//DEL 
//DEL 
//DEL }

void CAutoupdateDlg::LoadBitMapFromInstance(CBitmap &Dest, HINSTANCE Hinstance, WORD wResourceID)
{
	    HBITMAP TempBitMap = LoadBitmap(Hinstance,MAKEINTRESOURCE(wResourceID));

		if(!TempBitMap)
		{
			DisplayErrorInfo(string("LoadBitmap()"));
			
			
		}
		
		if(!Dest.Attach(TempBitMap))
		{
			DisplayErrorInfo(string("FromHandle()"));
		

		}


}

//DEL void CAutoupdateDlg::DrawInfoOnStaticPositionToDC(CDC * DeviceHandle,WORD CtlID, CString& Text)
//DEL {
//DEL 	    CWnd* pWnd; 
//DEL 		CRect rcText;
//DEL 		pWnd = GetDlgItem(CtlID);
//DEL 		pWnd->GetWindowRect(&rcText);
//DEL 		ScreenToClient( &rcText );
//DEL 		//ClientToInfoRegion(rcText);
//DEL 		DrawAText(DeviceHandle,12,6,0,100,FALSE,FALSE,FALSE,"仿宋",RGB(102,101,98),0,TRANSPARENT,Text,rcText);
//DEL 
//DEL }



HBRUSH CAutoupdateDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	

	if(nCtlColor == CTLCOLOR_STATIC   )
	{
		pDC->SetTextColor(RGB(102,101,98));
		// Set the background mode for text to transparent 
		// so background will show thru.
		pDC->SetBkMode(TRANSPARENT);
		
		
		
		
	}
	
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

void CAutoupdateDlg::OnConfig() 
{
	// TODO: Add your control notification handler code here
	

	
	

		
	    GameOptionPanel OptionInterface;
		OptionInterface.DoModal();
		
			
	
		
		

	

}
