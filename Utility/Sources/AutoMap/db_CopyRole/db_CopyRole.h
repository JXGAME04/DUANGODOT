// db_CopyRole.h : main header file for the DB_COPYROLE application
//

#if !defined(AFX_DB_COPYROLE_H__70B7BFE6_A232_4EA6_807B_99F6450A8BA6__INCLUDED_)
#define AFX_DB_COPYROLE_H__70B7BFE6_A232_4EA6_807B_99F6450A8BA6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDb_CopyRoleApp:
// See db_CopyRole.cpp for the implementation of this class
//

class CDb_CopyRoleApp : public CWinApp
{
public:
	CDb_CopyRoleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDb_CopyRoleApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDb_CopyRoleApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DB_COPYROLE_H__70B7BFE6_A232_4EA6_807B_99F6450A8BA6__INCLUDED_)
