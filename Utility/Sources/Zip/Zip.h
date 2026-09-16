// Zip.h : main header file for the ZIP DLL
//

#if !defined(AFX_ZIP_H__9FD07E14_EA80_420C_ADFE_257D9192DC3F__INCLUDED_)
#define AFX_ZIP_H__9FD07E14_EA80_420C_ADFE_257D9192DC3F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CZipApp
// See Zip.cpp for the implementation of this class
//


class CZipApp : public CWinApp
{
public:
	CZipApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZipApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CZipApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZIP_H__9FD07E14_EA80_420C_ADFE_257D9192DC3F__INCLUDED_)
