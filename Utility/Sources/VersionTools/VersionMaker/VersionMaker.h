// VersionMaker.h : main header file for the VERSIONMAKER application
//

#if !defined(AFX_VERSIONMAKER_H__CBD358D5_82DA_4A15_A00D_EB52383DC06F__INCLUDED_)
#define AFX_VERSIONMAKER_H__CBD358D5_82DA_4A15_A00D_EB52383DC06F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVersionMakerApp:
// See VersionMaker.cpp for the implementation of this class
//

class CVersionMakerApp : public CWinApp
{
public:
	CVersionMakerApp();

	char	m_szSrcRootPath[KLELNAMELEN];
	char	m_szDesRootPath[KLELNAMELEN];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVersionMakerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVersionMakerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CVersionMakerApp g_theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERSIONMAKER_H__CBD358D5_82DA_4A15_A00D_EB52383DC06F__INCLUDED_)
