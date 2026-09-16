// WorldEditor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WorldEditor.h"

#include "MainFrm.h"
#include "WorldEditorDoc.h"
#include "WorldEditorView.h"
#include "GraphicsWnd.h"
#include "cLog.h"
#include "fileop.h"


//#pragma comment(lib, "graphics.lib")
//#pragma comment(lib, "status.lib")
//#pragma comment(lib, "image.lib")
//common.lib message.lib image.lib graphics.lib command.lib automap.lib graphicsutility.lib status.lib 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp

BEGIN_MESSAGE_MAP(CWorldEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CWorldEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp construction

CWorldEditorApp::CWorldEditorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_hGMutex = NULL;
	m_pLog = new cAppLog;
}

CWorldEditorApp::~CWorldEditorApp()
{
	if(m_hGMutex)
		CloseHandle(m_hGMutex);	
	delete m_pLog;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWorldEditorApp object

CWorldEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp initialization
CMainFrame* CWorldEditorApp::GetMainFrame()
{
	return m_pMainFrame;
	//return (CMainFrame*)m_pMainWnd;
}

CGraphicsWnd* CWorldEditorApp::GetGraphicsWnd()
{
	return GetMainFrame()->m_pWnd;
	//return (CMainFrame*)m_pMainWnd;
}

CDirTree* CWorldEditorApp::GetDirTree()
{
	return &(GetWorldEditorView()->m_treeDir);
}

iStatusMachine* CWorldEditorApp::GetStatusMachine()
{
	return GetGraphicsWnd()->m_pMachine;
}

iGraphics* CWorldEditorApp::GetGraphics()
{
	return GetGraphicsWnd()->m_pGraphics;
}

CWorldEditorDoc* CWorldEditorApp::GetWorldEditorDoc()
{
	POSITION pos = GetFirstDocTemplatePosition();
	if (pos == NULL)
		return NULL;
	CDocTemplate* p = GetNextDocTemplate(pos);
	pos = p->GetFirstDocPosition();
	if (pos == NULL)
		return NULL;
	CDocument* pDoc = p->GetNextDoc(pos);
	CWorldEditorDoc* pWorldEditorDoc = dynamic_cast<CWorldEditorDoc*>(pDoc);
	return pWorldEditorDoc ;
}

CWorldEditorView* CWorldEditorApp::GetWorldEditorView()
{
	CWorldEditorDoc* pDoc = GetWorldEditorDoc();
	if (pDoc == NULL)
		return NULL;
	POSITION pos = pDoc->GetFirstViewPosition();
	if (pos == NULL)
		return NULL;
	CView* pView = pDoc->GetNextView(pos);
	return dynamic_cast<CWorldEditorView*>(pView);
}

COutputList* CWorldEditorApp::GetOutput()
{
	return GetWorldEditorView()->GetOutput();
}

void CWorldEditorApp::OutputMsg(int nStatus, LPCTSTR strMsg)
{
	GetOutput()->Message(nStatus,strMsg);
}

BOOL CWorldEditorApp::IsTheGameRun()
{
	return FALSE;
	m_hGMutex = CreateMutex(NULL,FALSE,"Worldeditor");
	if(m_hGMutex)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(m_hGMutex);
			return TRUE;
		}
	}
	return FALSE;
}

void CWorldEditorApp::WriteLogString(LPCSTR szString)
{
	m_pLog->write_log(szString);
}

BOOL CWorldEditorApp::InitInstance()
{
	if (IsTheGameRun())
	{
		AfxMessageBox("本程序已经在运行");
		return FALSE;
	}

	if (!m_pLog->create())
	{
		AfxMessageBox("can not create log file!");
		return FALSE;
	}
	else
	{
		CTime time = CTime::GetCurrentTime();
		CString sTime = time.Format( "%H:%M, %B %d, %Y" );
		CString sMsg = "create log file at " + sTime + "\n";
		WriteLogString(sMsg);
	}

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
	
//	g_SetRootPath(NULL);
	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWorldEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CWorldEditorView));
	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
	{
		AfxMessageBox("error ProcessShellCommand");
		return FALSE;
	}


	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	return TRUE;
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
		// No message handlers
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
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWorldEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp message handlers

/*
void AppGetWorkingFolder(CString& strWorkingFolder)
{
	char str[128];
	GetModuleFileName(GetModuleHandle(NULL),str,128);
	AppParsePath(str,strWorkingFolder);
}

void AppGetName(CString& strAppName)
{
	char str[128];
	GetModuleFileName(GetModuleHandle(NULL),str,128);
	CString sPath,sExt;
	AppParsePathNameExt(str,sPath,strAppName,sExt);
}

void AppParsePath(LPCSTR sFullPath, CString& sPath)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
}

void AppParsePathName(LPCSTR sFullPath, CString& sPath, CString& sName)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
	sName = szFullPath.Mid(pos + 1);
}

void AppParsePathNameExt(LPCSTR sFullPath, CString& sPath, CString& sName, CString& sExt)
{
	CString szFullPath = sFullPath;
	int pos = szFullPath.ReverseFind('\\');
	ASSERT(pos != -1);
	sPath = szFullPath.Left(pos);
	sName = szFullPath.Mid(pos + 1);
	pos = sName.ReverseFind('.');
	if (pos != -1)
	{
		sExt = sName.Mid(pos+1);
		sName = sName.Left(pos);
	}
}
*/
void AppOutputMsg(int nStatus, LPCTSTR strMsg)
{
	theApp.OutputMsg(nStatus,strMsg);
}


BOOL CWorldEditorApp::OnIdle(LONG lCount) 
{
	// TODO: Add your specialized code here and/or call the base class
	GetMainFrame()->OnIdle(lCount);
	return TRUE;
}
