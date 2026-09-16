// Common.h : main header file for the COMMON DLL
//

#if !defined(AFX_COMMON_H__91F6D877_E2A9_4B21_BC0B_2A7F99930537__INCLUDED_)
#define AFX_COMMON_H__91F6D877_E2A9_4B21_BC0B_2A7F99930537__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCommonApp
// See Common.cpp for the implementation of this class
//
class cItemCache;

class CCommonApp : public CWinApp
{
public:
	CCommonApp();
	virtual ~CCommonApp();
	cItemCache* m_pCache;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommonApp)
	public:
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CCommonApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CCommonApp theApp;
//CCommonApp* GetTheApp();
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMON_H__91F6D877_E2A9_4B21_BC0B_2A7F99930537__INCLUDED_)
