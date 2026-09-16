#if !defined(AFX_PROCESSDLG_H__D96869DF_B678_45DE_8A1E_7C11199EF672__INCLUDED_)
#define AFX_PROCESSDLG_H__D96869DF_B678_45DE_8A1E_7C11199EF672__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProcessDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProcessDlg dialog

class CProcessDlg : public CDialog
{
// Construction
public:
	CProcessDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProcessDlg)
	enum { IDD = IDD_DIALOG_PROCESS };
	CListBox	m_RelayList;
	CListBox	m_ErrorList;
	CString	m_strAccount;
	CString	m_strAction;
	CString	m_strRole;
	CString	m_strIP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProcessDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProcessDlg)
	afx_msg void OnButtonFreeze();
	afx_msg void OnButtonCount();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonSend();
	afx_msg void OnButtonUnlock();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonReturn();
	afx_msg void OnRadioBroad();
	afx_msg void OnRadioIp();
	afx_msg void OnRadioRelay();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	DWORD m_nSendIP;
	void Send(DWORD nReturn);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROCESSDLG_H__D96869DF_B678_45DE_8A1E_7C11199EF672__INCLUDED_)
