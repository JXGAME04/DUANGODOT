#if !defined(AFX_OBJDLG_H__7755349C_7216_49BE_9588_6374A155430D__INCLUDED_)
#define AFX_OBJDLG_H__7755349C_7216_49BE_9588_6374A155430D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjDlg dialog
class iObj;
class iGraphics;

class CObjDlg : public CDialog
{
// Construction
public:
	CObjDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjDlg)
	enum { IDD = IDD_OBJ };
	CString	m_strNick;
	CString	m_strTemplate;
	CString	m_strSection;
	BOOL	m_bAutoPlay;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseTemplate();
//	afx_msg void OnBrowseAnimation();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJDLG_H__7755349C_7216_49BE_9588_6374A155430D__INCLUDED_)
