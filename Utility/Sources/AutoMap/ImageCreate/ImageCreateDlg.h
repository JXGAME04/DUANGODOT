// ImageCreateDlg.h : header file
//

#if !defined(AFX_IMAGECREATEDLG_H__C446A679_0B01_4115_B1DA_7FB185A9BFA3__INCLUDED_)
#define AFX_IMAGECREATEDLG_H__C446A679_0B01_4115_B1DA_7FB185A9BFA3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CImageCreateDlg dialog
#include "..\Controls\SprStatic.h"

class CImageCreateDlg : public CDialog
{
// Construction
public:
	CImageCreateDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL IsFileNameValid();

// Dialog Data
	//{{AFX_DATA(CImageCreateDlg)
	enum { IDD = IDD_IMAGECREATE_DIALOG };
	CSprStatic	m_staticSpr;
	CString	m_strFile;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageCreateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CImageCreateDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnFile();
	afx_msg void OnGrid();
	afx_msg void OnSpr();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGECREATEDLG_H__C446A679_0B01_4115_B1DA_7FB185A9BFA3__INCLUDED_)
