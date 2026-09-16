// AutoupdateDlg.h : header file
//
//{{AFX_INCLUDES()
#include "webbrowser2.h"
//}}AFX_INCLUDES

#if !defined(AFX_AUTOUPDATEDLG_H__8ECBCF19_56D6_4317_855A_CF114C5A5647__INCLUDED_)
#define AFX_AUTOUPDATEDLG_H__8ECBCF19_56D6_4317_855A_CF114C5A5647__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAutoupdateDlg dialog
#include "afxhtml.h"

class CAutoupdateDlg : public CDialog
{
// Construction
public:
	CAutoupdateDlg(CWnd* pParent = NULL);	// standard constructor
	~CAutoupdateDlg();
	int m_nDownloaded;
	void Refresh();

	enum eStatus{STATUS_DOWNLOADING,STATUS_EXTRACTING,STATUS_DELETING,};
	eStatus m_eStatus;

// Dialog Data
	//{{AFX_DATA(CAutoupdateDlg)
	enum { IDD = IDD_AUTOUPDATE_DIALOG };
	CProgressCtrl	m_progressCurrent;
	CProgressCtrl	m_progress;
	CString	m_strVerson;
	CString	m_strProgress;
	CString	m_strProgressCurrent;
	CWebBrowser2	m_web;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoupdateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAutoupdateDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOUPDATEDLG_H__8ECBCF19_56D6_4317_855A_CF114C5A5647__INCLUDED_)
