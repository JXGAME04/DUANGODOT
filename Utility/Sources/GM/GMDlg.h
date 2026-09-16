//////////////////////////////////////////////////////////////////////////////////////
//	文件名			:	GMDlg.h
//	创建人			:	王西贝
//	创建时间		:	2003-6-4 14:59:13
//	文件说明		:	测试对话框类的定义
//////////////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_GMDLG_H__E9FAD736_C90E_43CD_90F5_8B91CBC58DF0__INCLUDED_)
#define AFX_GMDLG_H__E9FAD736_C90E_43CD_90F5_8B91CBC58DF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGMDlg dialog

#define GM_TIMER		1

class KGameMasterCore;
class CProxy_IGameMasterEvents;
class CGMApp;
class CGMDlg : public CDialog
{
// Construction
public:
	KGameMasterCore*					m_pGMCoreRef;
	CProxy_IGameMasterEvents<CGMApp>*	m_pGMEventProxy;
	CGMDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CGMDlg();
	int DoModal2(BOOL);

// Dialog Data
	//{{AFX_DATA(CGMDlg)
	enum { IDD = IDD_GM_DIALOG };
	CListBox	m_lstboxGameWorlds;
	int		m_nOnlineCount;
	int		m_nRegisterCount;
	CString	m_strIPAddress;
	CString	m_strGWName;
	CString	m_strAccount;
	CString	m_strPassword;
	int		m_nPingOfSystem;
	int		m_nTotalOnline;
	CString	m_strScript;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMDlg)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGMDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnUnlockPlayer();
	afx_msg void OnGetRegister();
	afx_msg void OnGetOnline();
	afx_msg void OnFindPlayer();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnGWLoginIn(WPARAM wParam, LPARAM lParam);
	afx_msg void OnGWLoginOut(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPingServer();
	afx_msg void OnGetGwName();
	afx_msg void OnGetGwIp();
	afx_msg void OnGetTotalOnline();
	afx_msg void OnExecute();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMDLG_H__E9FAD736_C90E_43CD_90F5_8B91CBC58DF0__INCLUDED_)
