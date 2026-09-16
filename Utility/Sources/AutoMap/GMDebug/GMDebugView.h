// GMDebugView.h : interface of the CGMDebugView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMDEBUGVIEW_H__321E283C_15F7_4590_A116_47FF762F8C68__INCLUDED_)
#define AFX_GMDEBUGVIEW_H__321E283C_15F7_4590_A116_47FF762F8C68__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGMDebugView : public CView
{
protected: // create from serialization only
	CGMDebugView();
	DECLARE_DYNCREATE(CGMDebugView)

// Attributes
public:
	CGMDebugDoc* GetDocument();

	void AddMessage(DWORD nMessage, LPCTSTR strMessage);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMDebugView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGMDebugView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CListBox m_ListBox;
// Generated message map functions
protected:
	//{{AFX_MSG(CGMDebugView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in GMDebugView.cpp
inline CGMDebugDoc* CGMDebugView::GetDocument()
   { return (CGMDebugDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMDEBUGVIEW_H__321E283C_15F7_4590_A116_47FF762F8C68__INCLUDED_)
