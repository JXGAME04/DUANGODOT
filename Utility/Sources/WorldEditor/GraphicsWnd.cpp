// GraphicsWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "GraphicsWnd.h"

#include "WorldEditorView.h"
#include "WorldEditorDoc.h"

#include "Mainfrm.h"

#include "istatus.h"
#include "iGraphics.h"
#include "fileop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd
const int c_nWndWidth = 800;
const int c_nWndHeight = 600;

const int ID_STATUS_TIMER = 1001;

CGraphicsWnd::CGraphicsWnd()
{
	m_pMachine = NULL;
	m_pGraphics = NULL;
	InitializeCriticalSection(&m_csTextureCache);
}

CGraphicsWnd::~CGraphicsWnd()
{
	EndGraphics();
	DeleteCriticalSection(&m_csTextureCache);
}


BEGIN_MESSAGE_MAP(CGraphicsWnd, CWnd)
	//{{AFX_MSG_MAP(CGraphicsWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
	ON_WM_KEYDOWN()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd message handlers

int CGraphicsWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	BeginGraphics();
	SetTimer(ID_STATUS_TIMER,30,NULL);
	
	return 0;
}

BOOL CGraphicsWnd::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
	//Do not erase!
	return CWnd::OnEraseBkgnd(pDC);
}

void CGraphicsWnd::OnClose() 
{
	return;
	CWnd::OnClose();
}

void CGraphicsWnd::OnDestroy() 
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnClose();
	KillTimer(ID_STATUS_TIMER);
	EndGraphics();
	CWnd::OnDestroy();
}

void CGraphicsWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
}

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about input

void CGraphicsWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnLButtonDown(nFlags,point);

	CWnd::OnLButtonDown(nFlags,point);
}

void CGraphicsWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnLButtonUp(nFlags,point);
	CWnd::OnLButtonUp(nFlags,point);
}

void CGraphicsWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnLButtonDblClk(nFlags,point);
	CWnd::OnLButtonDblClk(nFlags,point);
}

void CGraphicsWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnRButtonDown(nFlags,point);
	CWnd::OnRButtonDown(nFlags,point);
}

void CGraphicsWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnRButtonUp(nFlags,point);
	CWnd::OnRButtonUp(nFlags,point);
}

void CGraphicsWnd::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnRButtonDblClk(nFlags,point);
	CWnd::OnRButtonDblClk(nFlags,point);
}

int CGraphicsWnd::OnIdle(int lCount)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnTimer(ID_STATUS_TIMER);
	if (m_pGraphics)
		m_pGraphics->GraphicsRun();
	return TRUE;
}

void CGraphicsWnd::OnTimer(UINT nIDEvent)
{
	if (ID_STATUS_TIMER == nIDEvent)
	{
		return;
		if (GetStatusMachine() != NULL)
			GetStatusMachine()->OnTimer(nIDEvent);
		m_pGraphics->GraphicsRun();
	}

	CWnd::OnTimer(nIDEvent);
}

BOOL CGraphicsWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnSetCursor(pWnd,nHitTest,message);
	SetFocus();
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return CWnd::OnSetCursor(pWnd,nHitTest,message);
}

void CGraphicsWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= '0' && nChar <= '9')
	{
		theApp.GetDirTree()->Select(nChar-'0');
	}
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnKeyDown(nChar, nRepCnt, nFlags);
	CWnd::OnKeyDown(nChar,nRepCnt,nFlags);
}

void CGraphicsWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (GetStatusMachine() != NULL)
		GetStatusMachine()->OnMouseMove(nFlags, point);
	CWnd::OnMouseMove(nFlags, point);
}
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//utility

bool CGraphicsWnd::IsKeyDown(UINT vk)
{
	short state = GetKeyState(vk);
	return (HIBYTE(state)!=0 );
}

bool CGraphicsWnd::IsShiftDown()
{
	return IsKeyDown(VK_LSHIFT) || IsKeyDown(VK_RSHIFT);
}

bool CGraphicsWnd::IsCtrlDown()
{
	return IsKeyDown(VK_LCONTROL) || IsKeyDown(VK_RCONTROL);
}

bool CGraphicsWnd::IsAltDown()
{
	return IsKeyDown(VK_LMENU) || IsKeyDown(VK_RMENU);
}

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about window operator
// Create
BOOL CGraphicsWnd::Create(CMainFrame* pParentWnd)
{
	m_pWnd = pParentWnd;

	CRect r;
	r.right = 1024-5;
	r.top = 64;
	r.left = r.right - c_nWndWidth ;
	r.bottom = c_nWndHeight + r.top;
	
	DWORD style = WS_VISIBLE|WS_POPUP;
	BOOL b=CWnd::CreateEx(NULL,AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS),
		"edit Window",style,
		r,pParentWnd,0);
	
	ASSERT(b);
//	m_pGraphics->RunThread();

	return b;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about graphics engine operator
// Create
int CGraphicsWnd::BeginGraphics()
{
	//graphcs
	m_pGraphics = Graphics_CreateGraphics(Graphics_3D);
	m_pGraphics->Begin();
	ASSERT(m_pGraphics );	
	iGraphics::stParam sc;
	sc.bFullScreen = false;
	//attention we can not create a device with the rc size
	sc.fWidth = c_nWndWidth;
	sc.fHeight = c_nWndHeight;
	//	sc.fHeight = rc.Height();
	//	sc.fWidth = rc.Width();
	sc.x = 0;
	sc.y = 0;
	sc.hInst = AfxGetInstanceHandle();
	sc.hWnd = GetSafeHwnd();
	sc.nFrame = 25;
	sc.mask = iGraphics::MASK_COMMAND;
	sc.eCommand = iGraphics::GCOM_INIT;
	m_pGraphics->SetParam(sc,TRUE);
	sc.mask = iGraphics::MASK_PATH | iGraphics::MASK_NAME;
	CString strWorkingFolder;
	AppGetWorkingFolder(strWorkingFolder);
	strcpy(sc.szPath,strWorkingFolder);
	strcpy(sc.szName,"WorldEditor");
	m_pGraphics->SetParam(sc,TRUE);

	//machine
//	m_pMachine = new cStatusMachine();
	m_pMachine = Status_CreateStatusMachine();
	m_pMachine->Create(m_pGraphics,this,AppOutputMsg);//&AppOutputMsg
//	m_pMachine->SetOutputList()
	
	return 0;
}

int CGraphicsWnd::EndGraphics()
{
	RELEASE(m_pMachine);

	if (m_pGraphics)
	{
//		m_pGraphics->EndThread();
		m_pGraphics->End();
		m_pGraphics->Release();
		m_pGraphics = NULL;
	}	
	return 0;
}
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//CGraphicsWnd about Document
#include "shellapi.h"

BOOL CGraphicsWnd::OnNewDocument()
{
	static int n = 0;
	CString sWorkingFolder;
	AppGetWorkingFolder(sWorkingFolder);

	CString strDefault;
	strDefault.Format("%s\\maps\\无标题",(LPCSTR)sWorkingFolder,n);
	AppDeleteFolder(strDefault);
	AppBuildPath(strDefault);
	strDefault+=".wor";

	m_strDocument = strDefault;
	
	return m_pMachine->OnNewDocument(strDefault);
}

BOOL CGraphicsWnd::OnOpenDocument(LPCTSTR lpszPathName)
{
	m_strDocument = lpszPathName;

	return m_pMachine->OnOpenDocument(lpszPathName);
}

BOOL CGraphicsWnd::OnSaveDocument(LPCTSTR lpszPathName)
{
	CWaitCursor cursor;
	if (m_strDocument != lpszPathName)
	{
		AppCopyFolder(m_strDocument,lpszPathName);
		
		int pos = m_strDocument.Find(".");
		CString strFolderSrc = 	m_strDocument.Left(pos);
		CString strPathName = lpszPathName;
		pos = strPathName.Find(".");
		CString strFolderDes = 	strPathName.Left(pos);
		if (m_strDocument.Find("\\maps\\default") != -1)
		{
			AppRenameFolder(strFolderSrc,strFolderDes);
		}
		else
		{
			//如果另存的文件在不同的目录下面,并且使用不同的目录
			AppDeleteFolder(strFolderDes);
			AppCopyFolder(strFolderSrc,strFolderDes);
/*
			int pos = strFolderDes.ReverseFind('\\');
			CString strDesPath = strFolderDes.Left(pos);
			pos = strFolderSrc.ReverseFind('\\');
			CString strSrcPath = strFolderSrc.Left(pos);
			if (strDesPath.CompareNoCase(strSrcPath) == 0)
			{
				//同一目录下面
				AppCopyFolder(strFolderSrc,strFolderDes);
			}
			else
			{
				//不同目录下面
				AppCopyFolder(strFolderSrc,strFolderDes);
			}
*/
		}
		m_strDocument = lpszPathName;
	}
	return m_pMachine->OnSaveDocument(lpszPathName);
}
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//redo and undo

BOOL CGraphicsWnd::Redo()
{
	return m_pMachine->Redo();
}

BOOL CGraphicsWnd::Undo()
{
	return m_pMachine->Undo();
}

BOOL CGraphicsWnd::CanRedo()
{
	return m_pMachine->CanRedo();
}

BOOL CGraphicsWnd::CanUndo()
{
	return m_pMachine->CanUndo();
}

BOOL CGraphicsWnd::RedoToEnd()
{
	return m_pMachine->RedoToEnd();
}

BOOL CGraphicsWnd::UndoToBegin()
{
	return m_pMachine->UndoToBegin();
}

/////////////////////////////////////////////////////////////////////////////
//utility
BOOL CGraphicsWnd::OnRightSelectedFile(LPCTSTR lpszPathName,int nFrame)
{
	CString s = lpszPathName;
	if (s.Right(4) == ".txt")
	{
		m_pMachine->OnRightSelectedFile(lpszPathName,nFrame);
	}
	return FALSE;
}

BOOL CGraphicsWnd::OnLeftSelectedFile(LPCTSTR lpszPathName,int nFrame)
{
	CString s = lpszPathName;
	if (s.Right(4) == ".txt")
	{
		m_pMachine->OnLeftSelectedFile(lpszPathName,nFrame);
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////

void CGraphicsWnd::OnEditCopy() 
{
	// TODO: Add your command handler code here
	m_pMachine->Copy();
}

void CGraphicsWnd::PrintScreen()
{
	CFileFind f;
	CString strFolder = "Screen\\%03d.bmp";
	AppGetFullPath(strFolder);
	AppBuildPath(strFolder);
	CString sFind;
	for (int i=0; i<1000; i++)
	{
		sFind.Format(strFolder,i);
		if (!f.FindFile(sFind))
			break;
	}
	if ( i== 1000)
		return;
	GetGraphics()->PrintScreen(sFind);
}

LRESULT CGraphicsWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (GetGraphics())
		GetGraphics()->WindowProc(GetSafeHwnd(),message,wParam,lParam);	
	return CWnd::WindowProc(message, wParam, lParam);
}

