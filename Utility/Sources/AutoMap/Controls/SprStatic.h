#if !defined(AFX_SPRSTATIC_H__92A99CE3_0B09_4478_905E_4A0307923C9A__INCLUDED_)
#define AFX_SPRSTATIC_H__92A99CE3_0B09_4478_905E_4A0307923C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SprStatic.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSprStatic window

class iImage;
#include "iImage.h"

class CSprStatic : public CStatic
{
// Construction
public:
	CSprStatic();

public:
	stDivide m_stDivide;
	void SetDivide(stDivide& Divide);
	void GetDivide(stDivide& Divide){Divide = m_stDivide;}
	BOOL Divide(LPCSTR szFile,int nHigh);
	void DrawDivide(int divide, int xoff, int yoff);
	void DrawPoint(CPoint ptCenter,COLORREF cr);
	void GetAlphaBottom(int& x,int& y);

	void SetDisplayAlpha(bool bDisplayAlpha);
	bool m_bDisplayAlpha;

// Attributes
public:
	iImage* m_pImage;
	CBitmap* m_pBmp;
	BOOL SetSprFile(LPCSTR szFile);
	BOOL m_bShowCenter;
	CPoint m_ptCenter;
	void SetShowCenter(BOOL b);
	UINT GetFrames();
	UINT GetDirections();
	void GetSize(int& width,int& height);
	UINT GetAnimationNum();
	BOOL GetAnimationName(int n, CString& strName);
// Operations
public:
	int m_nIndex;
	int GetiFrame(){return m_nIndex;}
	BOOL GetCursorOnImage(CPoint& pt);
	void SetCenter(CPoint pt);
	void GetCenter(CPoint& pt);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprStatic)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetFrame(int dir, int frame);
	virtual ~CSprStatic();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSprStatic)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPRSTATIC_H__92A99CE3_0B09_4478_905E_4A0307923C9A__INCLUDED_)
