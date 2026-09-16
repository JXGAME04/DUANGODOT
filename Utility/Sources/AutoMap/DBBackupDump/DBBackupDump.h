// DBBackupDump.h : main header file for the DBBACKUPDUMP application
//

#if !defined(AFX_DBBACKUPDUMP_H__800376E8_F041_488D_9CBF_899D7B9CC837__INCLUDED_)
#define AFX_DBBACKUPDUMP_H__800376E8_F041_488D_9CBF_899D7B9CC837__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDBBackupDumpApp:
// See DBBackupDump.cpp for the implementation of this class
//

class CDBBackupDumpApp : public CWinApp
{
public:
	CDBBackupDumpApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBBackupDumpApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDBBackupDumpApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBBACKUPDUMP_H__800376E8_F041_488D_9CBF_899D7B9CC837__INCLUDED_)
