// SprStatic.cpp : implementation file
//

#include "stdafx.h"
#include "SprStatic.h"

#include "iImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprStatic

CSprStatic::CSprStatic()
{
	m_pImage = NULL;
	m_pBmp = NULL;
	m_bShowCenter = false;
	m_nIndex = 0;

	m_bDisplayAlpha = false;

	m_stDivide.nPoint = 0;
}

CSprStatic::~CSprStatic()
{
	if (m_pImage)
		m_pImage->Release();
	if (m_pBmp)
	{
		m_pBmp->DeleteObject();
		delete m_pBmp;
	}
}


BEGIN_MESSAGE_MAP(CSprStatic, CStatic)
	//{{AFX_MSG_MAP(CSprStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprStatic message handlers
void CSprStatic::DrawPoint(CPoint ptCenter,COLORREF cr)
{
	CClientDC dc(this);
	int n = 3;
	CRect rc(ptCenter.x - n, ptCenter.y - n, ptCenter.x + n, ptCenter.y + n);
	CBrush brush;
	brush.CreateSolidBrush(cr);
	dc.SelectObject(&brush);	
	dc.Rectangle(rc);
}

void CSprStatic::DrawDivide(int divide, int xoff, int yoff)
{
	CPoint ptOld;
	COLORREF cr = RGB(0,0,255);
	CClientDC dc(this);
	CPen pen;
	pen.CreatePen(PS_SOLID,2,cr);
	dc.SelectObject(pen);

	if (m_stDivide.eDoodadType == 2 || m_stDivide.eDoodadType == 3)
	{
		CRect rc;
		GetClientRect(rc);

		dc.MoveTo(xoff,yoff);
		dc.LineTo(rc.right - xoff,yoff);
		dc.LineTo(rc.right - xoff,rc.bottom - yoff);
		dc.LineTo(xoff,rc.bottom - yoff);
		dc.LineTo(xoff,yoff);

		for (int i=1; i<m_stDivide.nWide+1; i++)
		{
			int off = (rc.Width()-2*xoff) / (m_stDivide.nWide+1)*i ;
			dc.MoveTo(xoff+off,yoff);
			dc.LineTo(xoff+off,rc.bottom-yoff);
		}
		for (int j=1; j<m_stDivide.nHigh+1; j++)
		{
			int off = (rc.Height()-2*yoff) / (m_stDivide.nHigh+1)*j ;
			dc.MoveTo(xoff,yoff+off);
			dc.LineTo(rc.right - xoff,yoff+off);
		}
		return;
	}
	for (int y=0; y<=m_stDivide.nHigh; y++)
	for (int i=0; i<m_stDivide.nPoint; i++)
	{
		CPoint pt;
		pt.x = m_stDivide.aPoint[i].x / divide;
		pt.y = (m_stDivide.aPoint[i].y-y * m_stDivide.nHeight) / divide; 
		pt.x += xoff;
		pt.y += yoff;

//		if (pt.y < 0)
//			pt.y = 0;
		DrawPoint(pt,cr);
		if (i != 0)
		{
			dc.MoveTo(ptOld);
			dc.LineTo(pt);
			if (m_stDivide.eDoodadType == 1) //draw hor
			{
			#pragma message (ATTENTION("不再支持了"))
				ptOld.y -= m_stDivide.nHeight / divide;
				CPoint point = pt;
				point.y -= m_stDivide.nHeight / divide;
				dc.MoveTo(ptOld);
				dc.LineTo(point);
			}
		}
		ptOld = pt;
	}
}

void CSprStatic::SetDivide(stDivide& Divide)
{
	m_stDivide = Divide;
	if (Divide.eGeometryType == 0)
	{
		//沿着45 x:y = 2:1
		//保持x值改变y值
		int nMul = 1;
		for (int i=1; i<m_stDivide.nPoint; i++)
		{
			int yOff = (m_stDivide.aPoint[i].x - m_stDivide.aPoint[i-1].x) * 0.5f;
			yOff *= nMul;
			m_stDivide.aPoint[i].y = m_stDivide.aPoint[i-1].y + yOff;
			nMul = -nMul;
		}
	}
	InvalidateRect(NULL,FALSE);	
}

BOOL CSprStatic::Divide(LPCSTR szFile,int nHigh)
{
	if (m_stDivide.nPoint == 0)
		return	FALSE;
	ASSERT(m_pImage);
	m_stDivide.nHigh = nHigh;
	stDivide st = m_stDivide;
	return m_pImage->DivideTo(szFile,st);
}

/////////////////////////////////////////////////////////////////////////////
// CSprStatic message handlers

void CSprStatic::OnPaint() 
{
	CPaintDC paintDC(this); // device context for painting

	if (m_pBmp == NULL)
		return;

	CRect r;
	GetClientRect(&r);

	CDC dc;
	dc.CreateCompatibleDC(&paintDC);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&paintDC,r.Width(),r.Height());
	CBitmap* pBmp = dc.SelectObject(&bmp);
	CBrush brush;
	WORD cr = 0xc6;
	WORD cg = 0xc3;
	WORD cb = 0xc6;
	brush.CreateSolidBrush(RGB(cr,cg,cb));
	dc.SelectObject(&brush);
	dc.Rectangle(r);

	CDC memDC;
	memDC.CreateCompatibleDC(&paintDC);
	CBitmap* p = memDC.SelectObject(m_pBmp);
	BITMAP bm;
	m_pBmp->GetBitmap(&bm);

	int x = 0;
	int y = 0;

	int divide = 0;
	if (bm.bmWidth <= r.Width() && bm.bmHeight <= r.Height())
	{
		x = (r.Width() - bm.bmWidth)/2;
		y = (r.Height() - bm.bmHeight)/2;
		VERIFY(dc.BitBlt(x,y,bm.bmWidth,bm.bmHeight,&memDC,0,0,SRCCOPY));

		divide = 1;
	}
	else
	{
		int dx = bm.bmWidth / r.Width();
		int dy = bm.bmHeight / r.Height();
		int d = max(dx,dy) + 1;
		int w = bm.bmWidth / d;
		int h = bm.bmHeight / d;
		x = (r.Width() - w)/2;
		y = (r.Height() - h)/2;
		dc.SetStretchBltMode(HALFTONE);
		dc.StretchBlt(x,y,w,h,&memDC,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);

		divide = d;
	}
	
	VERIFY(paintDC.BitBlt(0,0,r.Width(),r.Height(),&dc,0,0,SRCCOPY));

	if (m_bShowCenter)
	{
		CPoint ptCenter;
		ptCenter.x = m_ptCenter.x / divide;
		ptCenter.y = m_ptCenter.y / divide;
		ptCenter.x += x;
		ptCenter.y += y;
		
		if (ptCenter.x >= 0 && ptCenter.y >= 0)
			DrawPoint(ptCenter,RGB(0,255,0));
	}

	memDC.SelectObject(p);
	memDC.DeleteDC();


	dc.SelectObject(pBmp);
	dc.DeleteDC();
	bmp.DeleteObject();

	DrawDivide(divide,x,y);
}	


BOOL CSprStatic::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	
	return CStatic::OnEraseBkgnd(pDC);
}

BOOL CSprStatic::SetSprFile(LPCSTR szFile)
{
	if (m_pImage)
		m_pImage->Release();
	m_pImage = Image_CreateImage(szFile);
	if (m_pImage->LoadFile(szFile) < 0)
	{
		m_pImage->Release();
		m_pImage = NULL;
		return FALSE;
	}
	return TRUE;
}

void CSprStatic::SetShowCenter(BOOL b)
{
	m_bShowCenter = b;
	InvalidateRect(NULL,FALSE);	
}

UINT CSprStatic::GetFrames()
{
	stImageInfo info;
	if (m_pImage)
		m_pImage->GetImageInfo(info);
	else
		return 0;
	return info.nFrames;
}

void CSprStatic::GetSize(int& width,int& height)
{
	stImageInfo info;
	if (m_pImage)
	{
		m_pImage->GetImageInfo(info);
		width = info.width;
		height = info.height;
	}
}

UINT CSprStatic::GetDirections()
{
	stImageInfo info;
	if (m_pImage)
		m_pImage->GetImageInfo(info);
	else
		return 1;
	return info.nDirections;
}

UINT CSprStatic::GetAnimationNum()
{
	return 1;
}

BOOL CSprStatic::GetAnimationName(int n, CString& strName)
{
	strName = "Wait";
	return TRUE;
}

void CSprStatic::GetAlphaBottom(int& x,int& y)
{
	if (m_pImage == NULL)
		return;
	m_pImage->GetAlphaBottom(m_nIndex,x,y);
}

void CSprStatic::SetFrame(int dir, int frame)
{
	if (m_pImage == NULL)
		return;
	
	if (m_pBmp != NULL)
	{
		m_pBmp->DeleteObject();
		delete m_pBmp;
		m_pBmp = NULL;
	}
	int frames = GetFrames();
	int dirs = GetDirections();

	int index = dir * frames / dirs + frame;
	CClientDC dc(this);
	int bits = dc.GetDeviceCaps(BITSPIXEL);
	m_pBmp = m_pImage->ImageToBitmap(index,bits);
//	stFrameInfo info;
//	m_pImage->GetFrameInfo(info,index);
//	m_ptCenter = CPoint(info.x,info.y);
	m_ptCenter = CPoint(0,0);
	m_nIndex = index;

	InvalidateRect(NULL,FALSE);
}

BOOL CSprStatic::GetCursorOnImage(CPoint& pt)
{
	CPoint point;
	if (!GetCursorPos(&point))
		return FALSE;
	if (m_pBmp == NULL)
		return FALSE;

    ScreenToClient( &point );
	
	CRect r;
	GetClientRect(&r);

	BITMAP bm;
	m_pBmp->GetBitmap(&bm);
	int x = 0;
	int y = 0;
	if (bm.bmWidth <= r.Width() && bm.bmHeight <= r.Height())
	{
		x = (r.Width() - bm.bmWidth)/2;
		y = (r.Height() - bm.bmHeight)/2;

		pt.x = point.x - x;
		pt.y = point.y - y;
	}
	else
	{
		int dx = bm.bmWidth / r.Width();
		int dy = bm.bmHeight / r.Height();
		int d = max(dx,dy) + 1;
		int w = bm.bmWidth / d;
		int h = bm.bmHeight / d;
		x = (r.Width() - w)/2;
		y = (r.Height() - h)/2;

		pt.x = point.x - x;
		pt.y = point.y - y;
		pt.x *= d;
		pt.y *= d;
	}

	m_ptCenter = pt;
	InvalidateRect(NULL,FALSE);
	
	return TRUE;
}

void CSprStatic::GetCenter(CPoint& pt)
{
	pt = m_ptCenter;
}

void CSprStatic::SetCenter(CPoint pt)
{
	if (m_ptCenter != pt)
	{
		m_ptCenter = pt;
		InvalidateRect(NULL,FALSE);	
	}
}

void CSprStatic::SetDisplayAlpha(bool bDisplayAlpha)
{
	if (m_bDisplayAlpha == bDisplayAlpha)
		return ;
	m_bDisplayAlpha = bDisplayAlpha;
	if (m_pImage == NULL)
		return;
	if (m_pBmp != NULL)
	{
		m_pBmp->DeleteObject();
		delete m_pBmp;
		m_pBmp = NULL;
	}
	CClientDC dc(this);
	int bits = dc.GetDeviceCaps(BITSPIXEL);
	if (m_bDisplayAlpha == false)
	{
		m_pBmp = m_pImage->ImageToBitmap(m_nIndex,bits);
	}
	else
	{
		m_pBmp = m_pImage->ImageAlphaToBitmap(m_nIndex,bits);
	}
	InvalidateRect(NULL,FALSE);	
}
