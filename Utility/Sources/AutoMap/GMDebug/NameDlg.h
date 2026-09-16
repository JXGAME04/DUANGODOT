#if !defined(AFX_NAMEDLG_H__466D7A46_F278_48DC_97B6_EC0D01965B25__INCLUDED_)
#define AFX_NAMEDLG_H__466D7A46_F278_48DC_97B6_EC0D01965B25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NameDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNameDlg dialog

class CNameDlg : public CDialog
{
// Construction
public:
	CNameDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNameDlg)
	enum { IDD = IDD_DIALOG_PASSWORD };
	CString	m_strName;
	CString	m_strPassword;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNameDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NAMEDLG_H__466D7A46_F278_48DC_97B6_EC0D01965B25__INCLUDED_)
