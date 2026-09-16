// DBBackupDumpDlg.h : header file
//

#if !defined(AFX_DBBACKUPDUMPDLG_H__AF3320CE_BC06_4FF4_A2E9_E16836521492__INCLUDED_)
#define AFX_DBBACKUPDUMPDLG_H__AF3320CE_BC06_4FF4_A2E9_E16836521492__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDBBackupDumpDlg dialog

class CDBBackupDumpDlg : public CDialog
{
// Construction
public:
	CDBBackupDumpDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDBBackupDumpDlg)
	enum { IDD = IDD_DBBACKUPDUMP_DIALOG };
	CListBox	m_listStatus;
	CString	m_txtDBPath;
	CString	m_txtDBName;
	CString	m_txtBackupFile;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBBackupDumpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDBBackupDumpDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnBackupfile();
	afx_msg void OnBtnDbpath();
	afx_msg void OnBtnDbdump();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBBACKUPDUMPDLG_H__AF3320CE_BC06_4FF4_A2E9_E16836521492__INCLUDED_)
