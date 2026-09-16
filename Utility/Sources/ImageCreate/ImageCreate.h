// ImageCreate.h : main header file for the IMAGECREATE application
//

#if !defined(AFX_IMAGECREATE_H__DD0CD61B_E40A_472D_B6EA_3A73BA25C226__INCLUDED_)
#define AFX_IMAGECREATE_H__DD0CD61B_E40A_472D_B6EA_3A73BA25C226__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CImageCreateApp:
// See ImageCreate.cpp for the implementation of this class
//

class CImageCreateApp : public CWinApp
{
public:
	CImageCreateApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageCreateApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CImageCreateApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGECREATE_H__DD0CD61B_E40A_472D_B6EA_3A73BA25C226__INCLUDED_)
