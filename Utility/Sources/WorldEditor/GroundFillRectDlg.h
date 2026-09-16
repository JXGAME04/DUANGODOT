#if !defined(AFX_GROUNDFILLRECTDLG_H__F0557533_CB68_4955_A01B_1DB75D4B40A6__INCLUDED_)
#define AFX_GROUNDFILLRECTDLG_H__F0557533_CB68_4955_A01B_1DB75D4B40A6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroundFillRectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroundFillRectDlg dialog

class CGroundFillRectDlg : public CDialog
{
// Construction
public:
	CGroundFillRectDlg(CWnd* pParent = NULL);   // standard constructor

	CString m_strPathName;
	void SetPathName(LPCSTR sz);
// Dialog Data
	//{{AFX_DATA(CGroundFillRectDlg)
	enum { IDD = IDD_GROUNDFILLRECT };
	CComboBox	m_cbType;
	UINT	m_nLeft;
	UINT	m_nTop;
	UINT	m_nRight;
	UINT	m_nBottom;
	CString	m_strType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroundFillRectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroundFillRectDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUNDFILLRECTDLG_H__F0557533_CB68_4955_A01B_1DB75D4B40A6__INCLUDED_)
