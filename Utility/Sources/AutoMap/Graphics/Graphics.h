// Graphics.h : main header file for the GRAPHICS DLL
//

#if !defined(AFX_GRAPHICS_H__97C4FF0A_5145_4736_B32E_859B8E96F0C6__INCLUDED_)
#define AFX_GRAPHICS_H__97C4FF0A_5145_4736_B32E_859B8E96F0C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


class CGraphicsApp : public CWinApp
{
public:
	CGraphicsApp();
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphicsApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CGraphicsApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHICS_H__97C4FF0A_5145_4736_B32E_859B8E96F0C6__INCLUDED_)
