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

/////////////////////////////////////////////////////////////////////////////
// CGMApp initialization

BOOL CGMApp::InitInstance()
{
	if (false == _Module.m_bATLInited)
		return FALSE;

	_Module.UpdateRegistryFromResource(IDR_GMAPP, TRUE);
	_Module.RegisterServer(TRUE);

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

	m_hDllJXGM = ::LoadLibrary(_T("JXGM.dll"));
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
		dlg.m_pGMEventProxy	= this;
		dlg.DoModal2(FALSE);
		return TRUE;
	}

	m_pMainWnd = &dlg;

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	dlg.m_pGMCoreRef		= m_pGameMaster;
	dlg.m_pGMEventProxy		= this;
	nResponse = dlg.DoModal2(TRUE);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
KS_EXIT:
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
