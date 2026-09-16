// Command.h : main header file for the COMMAND DLL
//

#if !defined(AFX_COMMAND_H__72366DDB_281C_44FE_BD0E_FAD4A687F2CB__INCLUDED_)
#define AFX_COMMAND_H__72366DDB_281C_44FE_BD0E_FAD4A687F2CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCommandApp
// See Command.cpp for the implementation of this class
//

class CCommandApp : public CWinApp
{
public:
	CCommandApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommandApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CCommandApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMAND_H__72366DDB_281C_44FE_BD0E_FAD4A687F2CB__INCLUDED_)
