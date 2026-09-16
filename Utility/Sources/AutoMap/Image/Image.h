// Image.h : main header file for the IMAGE DLL
//

#if !defined(AFX_IMAGE_H__98AF50DC_B310_43CB_B633_BDFBDA40E827__INCLUDED_)
#define AFX_IMAGE_H__98AF50DC_B310_43CB_B633_BDFBDA40E827__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CImageApp
// See Image.cpp for the implementation of this class
//

class CImageApp : public CWinApp
{
public:
	CImageApp();
	CString m_strPath;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CImageApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CImageApp theApp;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGE_H__98AF50DC_B310_43CB_B633_BDFBDA40E827__INCLUDED_)
