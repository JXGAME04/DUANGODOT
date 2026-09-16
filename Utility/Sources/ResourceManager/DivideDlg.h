#if !defined(AFX_DIVIDEDLG_H__FD61CCAE_4BC6_4AC4_978A_132C2912E335__INCLUDED_)
#define AFX_DIVIDEDLG_H__FD61CCAE_4BC6_4AC4_978A_132C2912E335__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DivideDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDivideDlg dialog
#include "../controls/sprstatic.h"
struct stImageInfo;

class CDivideDlg : public CDialog
{
// Construction
public:
	CDivideDlg(CWnd* pParent = NULL);   // standard constructor

	void UpdateType(int i);

	CString m_strFile;
	void SetFile(const CString& s){m_strFile = s;}
	CString m_strTxtFile;
	void SetTxtFile(const CString& s){m_strTxtFile = s;}
	void GetTxtFile(CString& s){s = m_strTxtFile;}
	CString m_strGroupFile;
	void GetGroupFile(CString& s){s = m_strGroupFile;}
	
	void SaveDivide(stDivide& divide);
	BOOL LoadDivide(stDivide& divide);
	void GetDivide(stDivide& divide);

	void DivideHor(int w, int h);
	void DivideVer(int w, int h);
	void Divide(stDivide& divide);

	void DivideImage(int w, int h, stImageInfo& info);
	
	void FormatPoint(CPoint& pt);
	void OffsetPoint(CPoint pt);
	void UpdatePointArray();
	void OnStaticSpr();
// Dialog Data
	//{{AFX_DATA(CDivideDlg)
	enum { IDD = IDD_DIVIDE };
	CComboBox	m_cbObstacle;
	CEdit	m_editHigh;
	CEdit	m_editWide;
	CSpinButtonCtrl	m_spinHigh;
	CSpinButtonCtrl	m_spinWide;
	CSpinButtonCtrl	m_spinHeight;
	CStatic	m_scHeight;
	CStatic	m_scWidth;
	CSprStatic	m_staticSpr;
	CListBox	m_listPoint;
	CSpinButtonCtrl	m_spinY;
	CSpinButtonCtrl	m_spinX;
	CComboBox	m_cbHeight;
	CComboBox	m_cbGeometry;
	CComboBox	m_cbDoodadType;
	int		m_nX;
	int		m_nY;
	BOOL	m_bDisplayAlpha;
	//}}AFX_DATA

	enum {MAX_COMBOX = 32};
	CComboBox* m_aCB[MAX_COMBOX];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDivideDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDivideDlg)
	afx_msg void OnDeletepoint();
	afx_msg void OnDivide();
	afx_msg void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnAddpoint();
	afx_msg void OnUp();
	afx_msg void OnLeft();
	afx_msg void OnDown();
	afx_msg void OnRight();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSelchangeDoodadtype();
	afx_msg void OnSelchangeGeometry();
	afx_msg void OnEditchangeHeight();
	afx_msg void OnSelchangeHeight();
	afx_msg void OnDivided();
	afx_msg void OnSelchangeObstacle();
	afx_msg void OnDisplayalpha();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIVIDEDLG_H__FD61CCAE_4BC6_4AC4_978A_132C2912E335__INCLUDED_)
