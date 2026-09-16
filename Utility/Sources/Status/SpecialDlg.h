#if !defined(AFX_SPECIALDLG_H__FCD18709_2438_45B5_8EA6_04D539A8E5AA__INCLUDED_)
#define AFX_SPECIALDLG_H__FCD18709_2438_45B5_8EA6_04D539A8E5AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecialDlg.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSpecialDlg dialog
class iSpecial;
class CSpecialDlg : public CDialog
{
// Construction
public:
	CSpecialDlg(CWnd* pParent = NULL);   // standard constructor

	iSpecial* m_pSpecial;
	void SetSpecial(iSpecial* p){m_pSpecial = p;}
	
	enum {MAX_ITEM = 16};
	CString m_aValue[MAX_ITEM];
	CString m_aItem[MAX_ITEM];
	CString m_aDescription[MAX_ITEM];

	CEdit	m_aValueControl[MAX_ITEM];
	CStatic	m_aItemControl[MAX_ITEM];
	CStatic	m_aDescriptionControl[MAX_ITEM];
	
// Dialog Data
	//{{AFX_DATA(CSpecialDlg)
	enum { IDD = IDD_SPECIAL };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpecialDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpecialDlg)
	afx_msg void OnOk();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECIALDLG_H__FCD18709_2438_45B5_8EA6_04D539A8E5AA__INCLUDED_)
