// GMPassGen.h : main header file for the GMPASSGEN application
//

#if !defined(AFX_GMPASSGEN_H__CE077276_FA38_44E6_A5D8_216A798F0969__INCLUDED_)
#define AFX_GMPASSGEN_H__CE077276_FA38_44E6_A5D8_216A798F0969__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGMPassGenApp:
// See GMPassGen.cpp for the implementation of this class
//

class CGMPassGenApp : public CWinApp
{
public:
	CGMPassGenApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMPassGenApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGMPassGenApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMPASSGEN_H__CE077276_FA38_44E6_A5D8_216A798F0969__INCLUDED_)
