// ResourceManager.h : main header file for the RESOURCEMANAGER application
//

#if !defined(AFX_RESOURCEMANAGER_H__C6CFAF71_176C_4F1F_8D96_B0AB3B6FDBC9__INCLUDED_)
#define AFX_RESOURCEMANAGER_H__C6CFAF71_176C_4F1F_8D96_B0AB3B6FDBC9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "KTabFileCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CResourceManagerApp:
// See ResourceManager.cpp for the implementation of this class
//
extern KTabFileCtrl g_SysTabFile;
class CResourceManagerApp : public CWinApp
{
public:
	CResourceManagerApp();
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResourceManagerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CResourceManagerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESOURCEMANAGER_H__C6CFAF71_176C_4F1F_8D96_B0AB3B6FDBC9__INCLUDED_)
