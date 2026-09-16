#if !defined(AFX_STRINGDLG_H__852CC05E_254D_4144_AFDB_EA503105FF92__INCLUDED_)
#define AFX_STRINGDLG_H__852CC05E_254D_4144_AFDB_EA503105FF92__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StringDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStringDlg dialog
#include "iStatus.h"

class CStringDlg : public CDialog
{
// Construction
public:
	CStringDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_strTitle;
	void SetTitle(LPCSTR sTitle){m_strTitle = sTitle;};

// Dialog Data
	//{{AFX_DATA(CStringDlg)
	enum { IDD = IDD_STRING };
	CString	m_strString;
	//}}AFX_DATA

	eMaskSetting m_eWant;
	void GetSetting(stSetting& st);
	void SetSetting(const stSetting& st);
	
	void SetString(eMaskSetting eWant, const CString& s);
	void GetString(eMaskSetting eWant, CString& s);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStringDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStringDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STRINGDLG_H__852CC05E_254D_4144_AFDB_EA503105FF92__INCLUDED_)
