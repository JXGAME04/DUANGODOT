// NetClientDlg.h : header file
//

#if !defined(AFX_NETCLIENTDLG_H__5EF0C3B7_2BD4_41A7_A9C6_A227A4F1FB7F__INCLUDED_)
#define AFX_NETCLIENTDLG_H__5EF0C3B7_2BD4_41A7_A9C6_A227A4F1FB7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CNetClientDlg dialog

class CNetClientDlg : public CDialog
{
// Construction
public:
	void AddString(LPCSTR szAccount,LPCSTR szChat);
	void AddMessage(LPCSTR szMessage);
	CNetClientDlg(CWnd* pParent = NULL);	// standard constructor

	void EnableNetStatus(NetStatus e,BOOL b);
// Dialog Data
	//{{AFX_DATA(CNetClientDlg)
	enum { IDD = IDD_NETCLIENT_DIALOG };
	CListCtrl	m_listChat;
	CString	m_strChat;
	UINT	m_nPort;
	CString	m_strServer;
	CString	m_strName;
	CString	m_strPassword;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNetClientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSend();
	afx_msg void OnConnect();
	afx_msg void OnQuit();
	afx_msg void OnVerify();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETCLIENTDLG_H__5EF0C3B7_2BD4_41A7_A9C6_A227A4F1FB7F__INCLUDED_)
