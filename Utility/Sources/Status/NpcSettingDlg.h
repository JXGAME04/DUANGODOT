//{{AFX_INCLUDES()
#include "activeiniform.h"
//}}AFX_INCLUDES
#if !defined(AFX_NPCSETTINGDLG_H__112E3C40_5B91_4625_9780_DD907BA1EE9D__INCLUDED_)
#define AFX_NPCSETTINGDLG_H__112E3C40_5B91_4625_9780_DD907BA1EE9D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NpcSettingDlg.h : header file
//
#include "igraphics.h"
/////////////////////////////////////////////////////////////////////////////
// CNpcSettingDlg dialog

class CNpcSettingDlg : public CDialog
{
// Construction
public:
	CNpcSettingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNpcSettingDlg)
	enum { IDD = IDD_NPCSETTING };
	CActiveIniForm	m_ActiveIniForm;
	//}}AFX_DATA
	CString m_TemplateFile;	//绝对路经
	CString m_ExtraFile;	//绝对路经
	CString m_Section;
	BOOL	m_bLoadingTemplate;
	DynamicEnum m_CurSaveType;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNpcSettingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
public:
	// Generated message map functions
	//{{AFX_MSG(CNpcSettingDlg)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDataChangedActiveiniformctrl1(LPCTSTR DataName);
	afx_msg void OnCtrlDoubleClickActiveiniformctrl1(LPCTSTR DataName);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NPCSETTINGDLG_H__112E3C40_5B91_4625_9780_DD907BA1EE9D__INCLUDED_)
