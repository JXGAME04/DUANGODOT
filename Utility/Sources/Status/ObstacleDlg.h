#if !defined(AFX_OBSTACLEDLG_H__5E9494B9_6B82_476D_9EE0_1E457697512E__INCLUDED_)
#define AFX_OBSTACLEDLG_H__5E9494B9_6B82_476D_9EE0_1E457697512E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObstacleDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObstacleDlg dialog

class CObstacleDlg : public CDialog
{
// Construction
public:
	CObstacleDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObstacleDlg)
	enum { IDD = IDD_OBSTACLE };
	CComboBox	m_cbKind;
	CComboBox	m_cbType;
	int		m_nType;
	int		m_nKind;
	float	m_fRadius;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObstacleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObstacleDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBSTACLEDLG_H__5E9494B9_6B82_476D_9EE0_1E457697512E__INCLUDED_)
