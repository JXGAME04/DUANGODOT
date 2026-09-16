// DBCopyRole.h : main header file for the DBCOPYROLE application
//

#if !defined(AFX_DBCOPYROLE_H__BCB0F8A8_0992_4F96_9566_A47F4845B9C9__INCLUDED_)
#define AFX_DBCOPYROLE_H__BCB0F8A8_0992_4F96_9566_A47F4845B9C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDBCopyRoleApp:
// See DBCopyRole.cpp for the implementation of this class
//

class CDBCopyRoleApp : public CWinApp
{
public:
	CDBCopyRoleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBCopyRoleApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDBCopyRoleApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBCOPYROLE_H__BCB0F8A8_0992_4F96_9566_A47F4845B9C9__INCLUDED_)
