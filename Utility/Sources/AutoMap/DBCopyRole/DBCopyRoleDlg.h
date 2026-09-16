// DBCopyRoleDlg.h : header file
//

#if !defined(AFX_DBCOPYROLEDLG_H__90487149_0EBB_49BC_B734_B89B4BAF3371__INCLUDED_)
#define AFX_DBCOPYROLEDLG_H__90487149_0EBB_49BC_B734_B89B4BAF3371__INCLUDED_

#include "DBTable.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDBCopyRoleDlg dialog

class CDBCopyRoleDlg : public CDialog
{
// Construction
public:
	CDBCopyRoleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDBCopyRoleDlg)
	enum { IDD = IDD_DBCOPYROLE_DIALOG };
	CString	m_txtBackupFile;
	CString	m_txtDBPath;
	CString	m_txtDBName;
	CString	m_txtStatus;
	CString	m_txtRoleName;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBCopyRoleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDBCopyRoleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnBackupfile();
	afx_msg void OnBtnDbpath();
	afx_msg void OnBtnDbopen();
	afx_msg void OnClose();
	afx_msg void OnBtnRolecopy();
	afx_msg void OnBtnSearchrole();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	ZDBTable* m_DescTable;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBCOPYROLEDLG_H__90487149_0EBB_49BC_B734_B89B4BAF3371__INCLUDED_)
