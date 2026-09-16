// DebugWinDlg.h : header file
//

#if !defined(AFX_DEBUGWINDLG_H__8B93B4BF_F3B2_4680_8013_0D2F24C5C2B0__INCLUDED_)
#define AFX_DEBUGWINDLG_H__8B93B4BF_F3B2_4680_8013_0D2F24C5C2B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDebugWinDlg dialog

class CDebugWinDlg : public CDialog
{
// Construction
public:
	CDebugWinDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL	m_nState;
// Dialog Data
	//{{AFX_DATA(CDebugWinDlg)
	enum { IDD = IDD_DEBUGWIN_DIALOG };
	CListBox	m_ChannelList;
	CListBox	m_List;
	CString	m_szWinInfo;
	BOOL	m_bWriteTime;
	BOOL	m_bViewString;
	BOOL	m_bAllChannel;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugWinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDebugWinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnClearList();
	afx_msg void OnStop();
	afx_msg void OnSaveFile();
	afx_msg void OnClearAll();
	afx_msg void OnAddChannel();
	afx_msg void OnDelChannel();
	afx_msg void OnClearChannel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGWINDLG_H__8B93B4BF_F3B2_4680_8013_0D2F24C5C2B0__INCLUDED_)
