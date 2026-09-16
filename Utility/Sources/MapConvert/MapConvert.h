// MapConvert.h : main header file for the MAPCONVERT application
//

#if !defined(AFX_MAPCONVERT_H__AF81947E_870D_42F5_9425_9791E73679F0__INCLUDED_)
#define AFX_MAPCONVERT_H__AF81947E_870D_42F5_9425_9791E73679F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMapConvertApp:
// See MapConvert.cpp for the implementation of this class
//

class CProgressWnd;
class cAppLog;

class CMapConvertApp : public CWinApp
{
public:
	void SetProgressStart();
	void SetProgressFile(LPCSTR szFile);
	void SetProgressEnd();

	CProgressWnd* m_pDlg;
	CString m_strFile;

	CMapConvertApp();
	virtual ~CMapConvertApp();
	
	cAppLog* m_pLog;
	void WriteLogString(LPCSTR szString);
	void OpenLog();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapConvertApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMapConvertApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CMapConvertApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPCONVERT_H__AF81947E_870D_42F5_9425_9791E73679F0__INCLUDED_)
