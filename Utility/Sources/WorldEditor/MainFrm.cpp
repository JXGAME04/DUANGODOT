// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WorldEditor.h"

#include "MainFrm.h"
#include "GraphicsWnd.h"
#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_COMMAND(ID_TOOLS_RESOURCEEDITOR, OnToolsResourceeditor)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDOTOEND, OnEditRedotoend)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDOTOEND, OnUpdateEditRedotoend)
	ON_COMMAND(ID_EDIT_UNDOTOBEGIN, OnEditUndotobegin)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDOTOBEGIN, OnUpdateEditUndotobegin)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_TOOLS_MAPCONVERT, OnToolsMapconvert)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_TOOLS_IMAGECREATE, OnToolsImagecreate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_pWnd = NULL;	
	theApp.SetMainFrame(this);
}

CMainFrame::~CMainFrame()
{
	SAFE_DELETE(m_pWnd);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
/*
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
*/
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	m_pWnd = new CGraphicsWnd;
	if (!m_pWnd->Create(this))
	{
		AfxMessageBox("error create graphicsWnd!");
		return -1;
	}

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZE | WS_MINIMIZEBOX ;
	//| WS_MINIMIZEBOX | WS_MAXIMIZEBOX 

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	return CFrameWnd::WindowProc(message, wParam, lParam);
}

void CMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();
	
	// TODO: Add your message handler code here
	if (m_pWnd )
		m_pWnd->DestroyWindow();	
	
}

BOOL CMainFrame::OnNewDocument()
{
	return m_pWnd->OnNewDocument();
}

BOOL CMainFrame::OnOpenDocument(LPCTSTR lpszPathName)
{
	return m_pWnd->OnOpenDocument(lpszPathName);
}

BOOL CMainFrame::OnSaveDocument(LPCTSTR lpszPathName)
{
	return m_pWnd->OnSaveDocument(lpszPathName);
}

BOOL CMainFrame::OnRightSelectedFile(LPCTSTR lpszPathName,int nFrame)
{
	return m_pWnd->OnRightSelectedFile(lpszPathName,nFrame);
}

BOOL CMainFrame::OnLeftSelectedFile(LPCTSTR lpszPathName,int nFrame)
{
	return m_pWnd->OnLeftSelectedFile(lpszPathName,nFrame);
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	CFrameWnd::OnTimer(nIDEvent);
}


void CMainFrame::OnEditRedo() 
{
	m_pWnd->Redo();	
}

void CMainFrame::OnEditUndo() 
{
	m_pWnd->Undo();
}

void CMainFrame::OnEditRedotoend() 
{
	m_pWnd->RedoToEnd();	
}

void CMainFrame::OnEditUndotobegin() 
{
	m_pWnd->UndoToBegin();	
}

void CMainFrame::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pWnd->CanRedo());
}

void CMainFrame::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pWnd->CanUndo());	
}

void CMainFrame::OnUpdateEditRedotoend(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pWnd->CanRedo());		
}

void CMainFrame::OnUpdateEditUndotobegin(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pWnd->CanUndo());		
}


void CMainFrame::OnEditCopy() 
{
	// TODO: Add your command handler code here
	m_pWnd->OnEditCopy();		
}

BOOL CMainFrame::OnIdle(int lCount)
{
	m_pWnd->OnIdle(lCount);
	return TRUE;
}

void OnExecute(LPCSTR szFile)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	char buffer[256];

	CString s = szFile;
	AppGetFullPath(s);
	strcpy(buffer,LPCSTR(s));

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
        //ErrorExit( "CreateProcess failed." );
		ASSERT(0);
    }


	// Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

void CMainFrame::OnToolsMapconvert() 
{
	OnExecute("\\MapConvert.exe");		
}

void CMainFrame::OnToolsResourceeditor() 
{
	OnExecute("\\ResourceManager.exe");
}

void CMainFrame::OnFilePrint() 
{
	m_pWnd->PrintScreen();
}

void CMainFrame::OnToolsImagecreate() 
{
	OnExecute("\\ImageCreate.exe");	
}
