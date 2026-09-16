// Status.h : main header file for the STATUS DLL
//

#if !defined(AFX_STATUS_H__06A269C1_4079_4DE4_B704_9EA15DEB095A__INCLUDED_)
#define AFX_STATUS_H__06A269C1_4079_4DE4_B704_9EA15DEB095A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CStatusApp
// See Status.cpp for the implementation of this class
//

class CStatusApp : public CWinApp
{
public:
	CStatusApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CStatusApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUS_H__06A269C1_4079_4DE4_B704_9EA15DEB095A__INCLUDED_)
