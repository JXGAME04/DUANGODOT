// GMDebugView.cpp : implementation of the CGMDebugView class
//

#include "stdafx.h"
#include "GMDebug.h"

#include "GMDebugDoc.h"
#include "GMDebugView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMDebugView

IMPLEMENT_DYNCREATE(CGMDebugView, CView)

BEGIN_MESSAGE_MAP(CGMDebugView, CView)
	//{{AFX_MSG_MAP(CGMDebugView)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMDebugView construction/destruction

CGMDebugView::CGMDebugView()
{
	// TODO: add construction code here

}

CGMDebugView::~CGMDebugView()
{
}

BOOL CGMDebugView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CGMDebugView drawing

void CGMDebugView::OnDraw(CDC* pDC)
{
}

void CGMDebugView::AddMessage(DWORD nMessage, LPCTSTR strMessage)
{
	CString str;
	str.Format("%d -- %s", nMessage, strMessage);
	m_ListBox.InsertString(-1, str);
	m_ListBox.SetCurSel(m_ListBox.GetCount() - 1);
}

/////////////////////////////////////////////////////////////////////////////
// CGMDebugView diagnostics

#ifdef _DEBUG
void CGMDebugView::AssertValid() const
{
	CView::AssertValid();
}

void CGMDebugView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGMDebugDoc* CGMDebugView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGMDebugDoc)));
	return (CGMDebugDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGMDebugView message handlers

void CGMDebugView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	RECT rc;
	GetWindowRect(&rc);
	rc.bottom = rc.bottom - rc.top;
	rc.right = rc.right - rc.left;
	rc.top = 0;
	rc.left = 0;
	m_ListBox.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
				rc, this, 0);
}

void CGMDebugView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_ListBox.GetSafeHwnd())
		m_ListBox.MoveWindow(0, 0, cx, cy, TRUE);
}
