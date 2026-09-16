// GMPassGenDlg.h : header file
//

#if !defined(AFX_GMPASSGENDLG_H__5707CB59_D9AF_4888_9EB2_8142F5A61911__INCLUDED_)
#define AFX_GMPASSGENDLG_H__5707CB59_D9AF_4888_9EB2_8142F5A61911__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGMPassGenDlg dialog

class CGMPassGenDlg : public CDialog
{
// Construction
public:
	CGMPassGenDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGMPassGenDlg)
	enum { IDD = IDD_GMPASSGEN_DIALOG };
	CString	m_strPassword1;
	CString	m_strPassword2;
	CString	m_strResult;
	CString	m_strResultMD5;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMPassGenDlg)
	public:
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGMPassGenDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnGenPass();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMPASSGENDLG_H__5707CB59_D9AF_4888_9EB2_8142F5A61911__INCLUDED_)
