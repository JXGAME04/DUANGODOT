// db_CopyRoleDlg.h : header file
//

#if !defined(AFX_DB_COPYROLEDLG_H__FE66D0AB_7451_4098_AB96_093C19AF589D__INCLUDED_)
#define AFX_DB_COPYROLEDLG_H__FE66D0AB_7451_4098_AB96_093C19AF589D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DBTable.h"
/////////////////////////////////////////////////////////////////////////////
// CDb_CopyRoleDlg dialog

class CDb_CopyRoleDlg : public CDialog
{
// Construction
public:
	CDb_CopyRoleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDb_CopyRoleDlg)
	enum { IDD = IDD_DB_COPYROLE_DIALOG };
	CString	m_DBDescPath;
	CString	m_DBSourcePath;
	CString	m_RoleName;
	CString	m_RoleData;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDb_CopyRoleDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDb_CopyRoleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnDbSource();
	afx_msg void OnBtnDbDesc();
	afx_msg void OnBtnRoleFind();
	afx_msg void OnBtnRoleCopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CDBTableReadOnly* DBSource;
	ZDBTable* DBDesc;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DB_COPYROLEDLG_H__FE66D0AB_7451_4098_AB96_093C19AF589D__INCLUDED_)
