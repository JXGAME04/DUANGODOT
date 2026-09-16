// DBBackupLoadDlg.h : header file
//

#if !defined(AFX_DBBACKUPLOADDLG_H__59C7E446_5FA9_4570_A627_BBDD7170B19C__INCLUDED_)
#define AFX_DBBACKUPLOADDLG_H__59C7E446_5FA9_4570_A627_BBDD7170B19C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDBBackupLoadDlg dialog

class CDBBackupLoadDlg : public CDialog
{
// Construction
public:
	CDBBackupLoadDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDBBackupLoadDlg)
	enum { IDD = IDD_DBBACKUPLOAD_DIALOG };
	CListBox	m_listStatus;
	CString	m_txtBackupFile;
	CString	m_txtDBName;
	CString	m_txtDBPath;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBBackupLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDBBackupLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnBackupfile();
	afx_msg void OnBtnDbload();
	afx_msg void OnBtnDbpath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString m_DBSavePath;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBBACKUPLOADDLG_H__59C7E446_5FA9_4570_A627_BBDD7170B19C__INCLUDED_)
