#if !defined(AFX_ADDCHANNELDLG_H__F00813EA_C3C0_468E_97AB_B2E2232D60A3__INCLUDED_)
#define AFX_ADDCHANNELDLG_H__F00813EA_C3C0_468E_97AB_B2E2232D60A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddChannelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddChannelDlg dialog

class CAddChannelDlg : public CDialog
{
// Construction
public:
	CAddChannelDlg(CWnd* pParent = NULL);   // standard constructor
// Dialog Data
	//{{AFX_DATA(CAddChannelDlg)
	enum { IDD = IDD_CHANNELDLG };
	CString	m_strChannel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddChannelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddChannelDlg)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDCHANNELDLG_H__F00813EA_C3C0_468E_97AB_B2E2232D60A3__INCLUDED_)
