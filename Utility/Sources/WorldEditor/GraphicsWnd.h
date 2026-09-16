#if !defined(AFX_GRAPHICSWND_H__B8D47E33_8573_49CE_8CC6_8D8A9062B1B4__INCLUDED_)
#define AFX_GRAPHICSWND_H__B8D47E33_8573_49CE_8CC6_8D8A9062B1B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphicsWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd window
class iGraphics;
class iGround;
class iCamera;
class iStatusMachine;
class CMainFrame;


class CGraphicsWnd : public CWnd
{
// Construction
public:
	CGraphicsWnd();

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about window operator
// Create
	BOOL Create(CMainFrame* m_pWnd);
	CRITICAL_SECTION m_csTextureCache;
	CMainFrame* m_pWnd;

// Full screen
	CRect m_rectWindow;
	LONG m_dwWindowStyle;
	void ToggleFullscreen() ;

//	Grid
	iGround* GetGround();
	void ToggleGrid();

//	Camera
	iCamera* GetCamera();
	void OffsetCamera(int x,int y);
	void OffsetCameraTo(float x,float y,float z);

// Mouse
//	CPoint GetMousePos();
// Keyboard
	bool IsKeyDown(UINT vk);
	bool IsCtrlDown();
	bool IsShiftDown();
	bool IsAltDown();
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//CGraphicsWnd about Document
	CString m_strDocument;
	BOOL OnNewDocument();
	BOOL OnOpenDocument(LPCTSTR lpszPathName);
	BOOL OnSaveDocument(LPCTSTR lpszPathName);
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//redo and undo
	BOOL Redo();
	BOOL Undo();
	BOOL CanRedo();
	BOOL CanUndo();
	BOOL RedoToEnd();
	BOOL UndoToBegin();

/////////////////////////////////////////////////////////////////////////////
//CGraphicsWnd about other window operator
	BOOL OnLeftSelectedFile(LPCTSTR lpszPathName,int nFrame);
	BOOL OnRightSelectedFile(LPCTSTR lpszPathName, int nFrame);
	
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about graphics engine operator
// Create
	iGraphics* m_pGraphics;
	iGraphics* GetGraphics(){return m_pGraphics;}
	int BeginGraphics();
	int EndGraphics();
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGraphicsWnd about statusMachine
	iStatusMachine* m_pMachine;
	iStatusMachine* GetStatusMachine(){return m_pMachine;}

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphicsWnd)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	void PrintScreen();
	virtual ~CGraphicsWnd();
	friend class CMainFrame;
	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphicsWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	BOOL OnIdle(int lCount);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHICSWND_H__B8D47E33_8573_49CE_8CC6_8D8A9062B1B4__INCLUDED_)
