// DebugWin.h : main header file for the DEBUGWIN application
//

#if !defined(AFX_DEBUGWIN_H__87837F78_929C_4168_AFB7_944A28A6305B__INCLUDED_)
#define AFX_DEBUGWIN_H__87837F78_929C_4168_AFB7_944A28A6305B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDebugWinApp:
// See DebugWin.cpp for the implementation of this class
//

class CDebugWinApp : public CWinApp
{
public:
	CDebugWinApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugWinApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDebugWinApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGWIN_H__87837F78_929C_4168_AFB7_944A28A6305B__INCLUDED_)
