// CQImageButton.cpp : implementation file
//

#include "stdafx.h"
#include "autoupdate.h"
#include "CQImageButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCQImageButton

CCQImageButton::CCQImageButton()

{
}

CCQImageButton::~CCQImageButton()
{
}
void CCQImageButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
 	//CRegion rgn;
	CRgn rgn;
	CDC* pDC;
	CRect rc;
	HBITMAP hBitmap =NULL;
	UINT state = lpDrawItemStruct->itemState;//按钮当前状态
	pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
 	rc.CopyRect(&lpDrawItemStruct->rcItem);

	
	if (state & ODS_SELECTED)//按下
 	{
		hBitmap = m_csDownBmpID;

		//用位图创建区域
		//BitmapToRgn(pDC, hBitmap, rgn);
 	}
 
	
	if(state & ODS_FOCUS    )
	{
 		hBitmap = m_csFocusBmpID;
        
		
		//用位图创建区域
		//BitmapToRgn(pDC, hBitmap, rgn);
 	}
	if(hBitmap == NULL)
	{
		
		hBitmap = m_csNormalBmpID;
	
		
		
		//用位图创建区域
		//BitmapToRgn(pDC, hBitmap, rgn);
		
	}
	
	
	CDC memDC;
	CBitmap bitmap, *pOldBmp;
	BITMAP bm;
	
	memDC.CreateCompatibleDC(pDC);
	bitmap.Attach(hBitmap);
	bitmap.GetBitmap(&bm);

	pOldBmp = memDC.SelectObject(&bitmap);
	pDC->BitBlt(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		&memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
 	//将区域设置为窗口
 		
	bitmap.Detach();
	//rgn.CreateEllipticRgn(0, 0, bm.bmWidth, bm.bmHeight);
	//SetWindowRgn((HRGN)rgn, TRUE);
}


void CCQImageButton::SetButtonBmp(UINT csPushBmpID, UINT csNormalBmpID,UINT csFocusBmpID,HINSTANCE AppHandle)
{
 	if(!(m_csDownBmpID   = LoadBitmap(AppHandle,MAKEINTRESOURCE(csPushBmpID))))
	{
		DisplayErrorInfo(string("LoadBitmap"));
	}
	
	if(!(m_csNormalBmpID   = LoadBitmap(AppHandle,MAKEINTRESOURCE(csNormalBmpID))))
	{
		DisplayErrorInfo(string("LoadBitmap"));
	}
	
	if(!(m_csFocusBmpID   = LoadBitmap(AppHandle,MAKEINTRESOURCE(csFocusBmpID))))
 	{
		DisplayErrorInfo(string("LoadBitmap"));
	}
	
}


BEGIN_MESSAGE_MAP(CCQImageButton, CButton)
	//{{AFX_MSG_MAP(CCQImageButton)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCQImageButton message handlers

void CCQImageButton::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default

	CButton::OnMouseMove(nFlags, point);

	SetFocus();

	Invalidate();
}
