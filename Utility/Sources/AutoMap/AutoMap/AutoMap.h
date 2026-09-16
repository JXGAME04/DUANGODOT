// AutoMap.h : main header file for the AUTOMAP DLL
//

#if !defined(AFX_AUTOMAP_H__90B6EF58_8AB8_41D6_A874_97CC58405010__INCLUDED_)
#define AFX_AUTOMAP_H__90B6EF58_8AB8_41D6_A874_97CC58405010__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAutoMapApp
// See AutoMap.cpp for the implementation of this class
//

class CAutoMapApp : public CWinApp
{
public:
	CAutoMapApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoMapApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CAutoMapApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOMAP_H__90B6EF58_8AB8_41D6_A874_97CC58405010__INCLUDED_)
