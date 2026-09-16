// CQStatic.cpp : implementation file
//

#include "stdafx.h"
#include "autoupdate.h"
#include "CQStatic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCQStatic

CCQStatic::CCQStatic()
{
}

CCQStatic::~CCQStatic()
{
}


BEGIN_MESSAGE_MAP(CCQStatic, CStatic)
	//{{AFX_MSG_MAP(CCQStatic)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCQStatic message handlers







void CCQStatic::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	 
     CRect rect; 
     GetClientRect(&rect); 
     
     CString m_strStatic; 
     CWnd* wndStatic = dc.GetWindow(); 
     wndStatic->GetWindowText(m_strStatic); 
     
     CBitmap bitmap; 
     bitmap.LoadBitmap(IDB_BackGround); 
     CDC dcmem; 
     dcmem.CreateCompatibleDC(&dc); 
     CBitmap* poldbitmap = dcmem.SelectObject(&bitmap); 
     this->GetWindowRect 
	 dc.BitBlt(0,0,rect.Width(),rect.Height(),&dcmem,0,0,SRCCOPY); 
     dcmem.SelectObject(poldbitmap); 
     
     //dc.SetBkMode(TRANSPARENT); 
     dc.TextOut(2,2,m_strStatic); 
     // Do not call CStatic::OnPaint() for painting messages 

	// Do not call CStatic::OnPaint() for painting messages
}
