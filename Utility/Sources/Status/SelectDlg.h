#if !defined(AFX_SELECTDLG_H__B3F2CACB_8EDF_47B5_9223_8FBBF7ABEA23__INCLUDED_)
#define AFX_SELECTDLG_H__B3F2CACB_8EDF_47B5_9223_8FBBF7ABEA23__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg dialog
#include "iGraphics.h"

class CSelectDlg : public CDialog
{
// Construction
public:
	CSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectDlg)
	enum { IDD = IDD_SELECT };
	CComboBox	m_cbFilterParam;
	CComboBox	m_cbFilter;
	CComboBox	m_cbKind;
	CComboBox	m_cbSort;
	int		m_nType;
	int		m_nSort;
	int		m_nKind;
	int		m_nFilter;
	int		m_nFilterParam;
	//}}AFX_DATA

	stSelect m_stSelect;
	void SetSelect(const stSelect& st){m_stSelect = st;}
	BOOL UpdateSelect();
	void GetSelect(stSelect& st){st = m_stSelect;}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOk();
	virtual void OnCancel();
	afx_msg void OnReset();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTDLG_H__B3F2CACB_8EDF_47B5_9223_8FBBF7ABEA23__INCLUDED_)
