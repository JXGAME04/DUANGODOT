// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "GMDebug.h"

#include "MainFrm.h"

#include "ProcessDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(IDM_PROCESS, OnUpdateProcess)
	ON_COMMAND(IDM_PROCESS, OnProcess)
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
	m_pdlg = NULL;
}

CMainFrame::~CMainFrame()
{
	if (m_pdlg)
		delete m_pdlg;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	SetTimer(1, 60000, NULL);
	SetTimer(2, 3, NULL);

	m_pdlg = new CProcessDlg(this);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == 1)	//ping
	{
		((CGMDebugApp*)AfxGetApp())->SendPing();
	}
	else if (nIDEvent == 2)	//do Process
		((CGMDebugApp*)AfxGetApp())->OnProcess();
	else if (nIDEvent == 3)	//connect
	{
		((CGMDebugApp*)AfxGetApp())->ConnectAndLogin();
		m_wndStatusBar.SetWindowText("ÖØÁ¬ÖÐ...");
	}

	CMDIFrameWnd::OnTimer(nIDEvent);
}

BOOL CMainFrame::DestroyWindow() 
{
	KillTimer(1);
	KillTimer(2);
	
	return CMDIFrameWnd::DestroyWindow();
}

void CMainFrame::SetPingTime(DWORD nTick)
{
	CString str;
	str.Format("Ping:%d", (GetTickCount() - nTick));
	m_wndStatusBar.SetWindowText(str);
}

bool CMainFrame::Trace(LPCTSTR strMessage)
{
	if (strMessage && strMessage[0] && m_pdlg && m_pdlg->GetSafeHwnd())
	{
		m_pdlg->m_ErrorList.InsertString(-1, strMessage);
		m_pdlg->m_ErrorList.SetCurSel(m_pdlg->m_ErrorList.GetCount() - 1);

		return true;
	}

	return false;
}


void CMainFrame::OnUpdateProcess(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnProcess() 
{
	if (m_pdlg)
		m_pdlg->DoModal();
}
