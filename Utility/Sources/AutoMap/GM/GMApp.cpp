//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GM.cpp
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 14:56:38
//	文件说明		:	DLL的加载，系统初始化等
//////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GM.h"
#include "GMDlg.h"
#include "GMCore.h"
#include "GMMessages.h"
#include <initguid.h>
#include "GameMaster_i.c"
#include "S3PDBConnectionPool.h"
#include "GMConsts.h"
#include "../GMPassGen/Include/PassGen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMApp construction

CGMApp::CGMApp()
{
	m_hDllJXGM = NULL;
	m_pISystemOnline = NULL;
	m_pGameMaster = NULL;
}

CGMApp::~CGMApp()
{
	if (m_pGameMaster)
		{ delete m_pGameMaster; m_pGameMaster = NULL; }
	if (m_hDllJXGM)
	{
		typedef HRESULT (STDAPICALLTYPE *DESTROYSYSTEMONLINE)();
		DESTROYSYSTEMONLINE fnDestroySystemOnline;
		HRESULT hr = S_OK;

		fnDestroySystemOnline = (DESTROYSYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("DestroySystemOnline"));
		VERIFY(fnDestroySystemOnline && SUCCEEDED(fnDestroySystemOnline()));
		::FreeLibrary(m_hDllJXGM); m_hDllJXGM = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGMApp object

CGMModule _Module;
CString gINI_FILENAME;

/////////////////////////////////////////////////////////////////////////////
// CGMApp initialization

BOOL CGMApp::InitInstance()
{
	VERIFY(::AfxInitRichEdit());

	if (false == _Module.m_bATLInited)
		return FALSE;

	_Module.UpdateRegistryFromResource(IDR_GMAPP, TRUE);
	_Module.RegisterServer(TRUE);

	char szModulePath[MAX_PATH + 1];
	::GetModuleFileName(NULL, szModulePath, (MAX_PATH + 1));

	char* pszOffset = strrchr(szModulePath, '\\');
    ASSERT(pszOffset);
    pszOffset[1] = '\0';
	
	CString strPath = szModulePath;

	gINI_FILENAME = strPath + "GMConn.ini";

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CGMDlg dlg;
	CCommandLineInfo cmdInfo;
	int nResponse = 0;

	typedef HRESULT (STDAPICALLTYPE *CREATESYSTEMONLINE)(ISystemOnline**);
	CREATESYSTEMONLINE fnCreateSystemOnline;
	HRESULT hr = S_OK;
	BOOL br = TRUE;

	S3PDBSQLServerConnectionPool *pSQLConnectPool = NULL;
	S3PDBAccessConnectionPool *pConnectPool = S3PDBAccessConnectionPool::Instance();
	KS_CHECK_BOOLEX(pConnectPool, hr = E_FAIL);


	br = pConnectPool->Init(strPath + GM_DB_NAME, 10);
	if (!br)
	{
		CString str;
		str.Format("在当前位置 \"%s\" 找不到运行所需文件 \"%s\" !!!", szModulePath, GM_DB_NAME);
		::AfxMessageBox(str);
		goto KS_EXIT;
	}

	pSQLConnectPool = S3PDBSQLServerConnectionPool::Instance();
	KS_CHECK_BOOLEX(pSQLConnectPool, hr = E_FAIL);

	{
		char szBuffer[128], szBufDec[128];
		_DATABASEINFO database;

		GetPrivateProfileString("DataBase", "DBAddress", "192.168.20.84", szBuffer, sizeof(szBuffer), gINI_FILENAME);
		database.strServer = szBuffer;
		GetPrivateProfileString("DataBase", "DBName", "cs", szBuffer, sizeof(szBuffer), gINI_FILENAME);
		database.strDataBase = szBuffer;
		GetPrivateProfileString("DataBase", "UserName", "8UGFSDUHGxWCA~TH1jkNrx84Zz080AF7", szBuffer, sizeof(szBuffer), gINI_FILENAME);	//blackbay
		::SimplyDecryptPassword(szBufDec, szBuffer);
		database.strUser = szBufDec;
		GetPrivateProfileString("DataBase", "Password", "bLdZsppHf_ij_HzMJAUnrb_VVdaZ4OAY", szBuffer, sizeof(szBuffer), gINI_FILENAME);	//thekingofworld
		::SimplyDecryptPassword(szBufDec, szBuffer);
		database.strPassword = szBufDec;

		br = pSQLConnectPool->Init(&database, 10);
		if (!br)
		{
			CString str;
			str.Format("在地址 \"%s\" 无法打开 SQL 数据库 \"%s\" !!!", database.strServer.c_str(), database.strDataBase.c_str());
			::AfxMessageBox(str);
		}
	}
	m_hDllJXGM = ::LoadLibrary(strPath + _T("JXGM.dll"));
	KS_CHECK_BOOLEX(m_hDllJXGM, hr = E_FAIL);

	fnCreateSystemOnline = (CREATESYSTEMONLINE)::GetProcAddress(m_hDllJXGM, _T("CreateSystemOnline"));
	KS_CHECK_BOOLEX(fnCreateSystemOnline, hr = E_FAIL);

	hr = fnCreateSystemOnline(&m_pISystemOnline);
	KS_CHECK(hr);
	KS_CHECK_BOOLEX(m_pISystemOnline, hr = E_FAIL);
	
	m_pGameMaster = new KGameMasterCore(m_pISystemOnline);
	KS_CHECK_BOOLEX(m_pGameMaster, hr = E_FAIL);

	// Parse command line for standard shell commands, DDE, file open
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		m_pMainWnd = &dlg;
		dlg.m_pGMCoreRef	= m_pGameMaster;

		// Dispatch commands specified on the command line
		if (!ProcessShellCommand(cmdInfo))
			goto KS_EXIT;

		//dlg.m_pGMEventProxy	= this;
		if (m_pGameMaster->IsConnectionOK())
			dlg.DoModal2(FALSE);
		goto KS_EXIT;
	}

	m_pMainWnd = &dlg;

	// The main window has been initialized, so show and update it.
	dlg.m_pGMCoreRef		= m_pGameMaster;
	//dlg.m_pGMEventProxy		= this;

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		goto KS_EXIT;

	if (m_pGameMaster->IsConnectionOK())
		nResponse = dlg.DoModal2(TRUE);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
KS_EXIT:
	S3PDBAccessConnectionPool::ReleaseInstance();
	S3PDBSQLServerConnectionPool::ReleaseInstance();
	m_pMainWnd = NULL;
	return FALSE;
}


BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_GameMaster, CGMApp)
END_OBJECT_MAP()

CGMModule::CGMModule()
{
	m_bATLInited = TRUE;

	HRESULT hRes = CoInitialize(NULL);
	if (FAILED(hRes))
		exit(1);

	_Module.Init(ObjectMap, GetModuleHandle(NULL));
	_Module.dwThreadID = GetCurrentThreadId();

	LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	TCHAR szTokens[] = _T("-/");

	BOOL bRun = TRUE;
	LPCTSTR lpszToken = _Module.FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_GMAPP, FALSE);
			_Module.UnregisterServer(TRUE); //TRUE means typelib is unreg'd
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_GMAPP, TRUE);
			_Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
		lpszToken = _Module.FindOneOf(lpszToken, szTokens);
	}

	if (!bRun)
	{
		_Module.Term();
		CoUninitialize();
		exit(0);
	}

	hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);
	if (FAILED(hRes))
	{
		CoUninitialize();
		exit(1);
	}
	IRunningObjectTable *pIROT = NULL;
	//hRes = GetRunningObjectTable(0, &pIROT); //todo...ROT register
	if (FAILED(hRes))
	{
		CoUninitialize();
		exit(1);
	}
	//pIROT->Register(
}

CGMModule::~CGMModule()
{
	if (m_bATLInited)
	{
		_Module.RevokeClassObjects();
		_Module.Term();
		CoUninitialize();
	}
}

LONG CGMModule::Unlock()
{
	AfxOleUnlockApp();
	return 0;
}

LONG CGMModule::Lock()
{
	AfxOleLockApp();
	return 1;
}

LPCTSTR CGMModule::FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p)
				return CharNext(p1);
			p = CharNext(p);
		}
		p1++;
	}
	return NULL;
}

BOOL CGMApp::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message >= WM_JX_MSG_FIRST && pMsg->message <= WM_JX_MSG_LAST &&
		m_pMainWnd)
	{
		if (m_pMainWnd->m_hWnd != pMsg->hwnd &&
			m_pMainWnd->PostMessage(pMsg->message, pMsg->wParam, pMsg->lParam))
			return TRUE;
	}
	
	return CWinApp::PreTranslateMessage(pMsg);
}
