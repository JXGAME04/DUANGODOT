// WorldEditorView.h : interface of the CWorldEditorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WORLDEDITORVIEW_H__BAADE1B1_48FF_494F_87E8_9BCB7E244613__INCLUDED_)
#define AFX_WORLDEDITORVIEW_H__BAADE1B1_48FF_494F_87E8_9BCB7E244613__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../controls/dirTree.h"
#include "controls/outputlist.h"
#include "../controls/Sprstatic.h"

class CWorldEditorDoc;
class CGraphicsWnd;

class CWorldEditorView : public CFormView
{
protected: // create from serialization only
	CWorldEditorView();
	DECLARE_DYNCREATE(CWorldEditorView)
	bool m_bCreate;
	
public:
	//{{AFX_DATA(CWorldEditorView)
	enum { IDD = IDD_WORLDEDITOR_FORM };
	CSpinButtonCtrl	m_spinSpeed;
	COutputList m_listOutput;
	CSpinButtonCtrl	m_spinFrame;
	CSpinButtonCtrl	m_spinDirection;
	CSpinButtonCtrl	m_spinAnimation;
	CDirTree	m_treeDir;
	CSprStatic	m_staticSpr;
	UINT	m_nFrame;
	CString	m_strAnimation;
	BOOL	m_bAutoPlay;
	UINT	m_nDirection;
	UINT	m_nSpeed;
	//}}AFX_DATA

// Attributes
public:
	COutputList* GetOutput(){return &m_listOutput;}
	CWorldEditorDoc* GetDocument();
	HRESULT OutputMessage(LPCSTR szMessage);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWorldEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL OnRightSelectedFile(LPCSTR szFile);
	BOOL OnSelectedFile(LPCSTR szFile);
	void CreateTree();
	void AfterCreate();
	void SetFrame();
// Generated message map functions
protected:
	//{{AFX_MSG(CWorldEditorView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnChangeFrame();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnChangeDirection();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in WorldEditorView.cpp
inline CWorldEditorDoc* CWorldEditorView::GetDocument()
   { return (CWorldEditorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORLDEDITORVIEW_H__BAADE1B1_48FF_494F_87E8_9BCB7E244613__INCLUDED_)
