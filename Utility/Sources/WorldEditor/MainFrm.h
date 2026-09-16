// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__745C4E9A_086E_43EC_8940_3F715D363383__INCLUDED_)
#define AFX_MAINFRM_H__745C4E9A_086E_43EC_8940_3F715D363383__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGraphicsWnd;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes

// graphics
	CGraphicsWnd* m_pWnd;
	BOOL OnNewDocument();
	BOOL OnOpenDocument(LPCTSTR lpszPathName);
	BOOL OnSaveDocument(LPCTSTR lpszPathName);

	BOOL OnRightSelectedFile(LPCTSTR lpszPathName, int nFrame);
	BOOL OnLeftSelectedFile(LPCTSTR lpszPathName,int nFrame);
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnToolsResourceeditor();
	afx_msg void OnEditRedo();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedotoend();
	afx_msg void OnUpdateEditRedotoend(CCmdUI* pCmdUI);
	afx_msg void OnEditUndotobegin();
	afx_msg void OnUpdateEditUndotobegin(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnToolsMapconvert();
	afx_msg void OnFilePrint();
	afx_msg void OnToolsImagecreate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	BOOL OnIdle(int lCount);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__745C4E9A_086E_43EC_8940_3F715D363383__INCLUDED_)
