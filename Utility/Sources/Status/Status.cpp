// Status.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Status.h"
#include "fileop.h"
#include "kengine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CStatusApp

BEGIN_MESSAGE_MAP(CStatusApp, CWinApp)
	//{{AFX_MSG_MAP(CStatusApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusApp construction

CStatusApp::CStatusApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	g_SetRootPath(NULL);
	OleInitialize(NULL);

	char pszDllName[200];
	g_GetFullPath(pszDllName, "ActiveIniForm.ocx");
	CString str = "ActiveIniForm.ocx";
	AppGetFullPath(str);
	HINSTANCE hLib = LoadLibrary(str);
	
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
	{
//		AfxMessageBox("无法注册ActiveIniForm.ocx"); //unable to load DLL
		
		//iReturn = FAIL_LOAD;                           //unable to load DLL
	}
	else
	// Find the entry point.
{
	FARPROC lpDllEntryPoint ;
	(FARPROC&)lpDllEntryPoint = GetProcAddress(hLib,   _T("DllRegisterServer"));
	if (lpDllEntryPoint != NULL)
		(*lpDllEntryPoint)();
	else
	{
		AfxMessageBox("无法注册ActiveIniForm.ocx"); //unable to load DL
		// Unable to locate entry point
	}
}	
	OleUninitialize();
	
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CStatusApp object

CStatusApp theApp;
