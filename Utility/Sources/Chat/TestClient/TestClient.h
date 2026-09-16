// TestClient.h : main header file for the TESTCLIENT application
//

#if !defined(AFX_TESTCLIENT_H__C3781B6D_A1D7_4A8F_A1EB_05811E6642D2__INCLUDED_)
#define AFX_TESTCLIENT_H__C3781B6D_A1D7_4A8F_A1EB_05811E6642D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

const int c_nLink = c_nMaxLink;
/////////////////////////////////////////////////////////////////////////////
// CTestClientApp:
// See TestClient.cpp for the implementation of this class
//

class cPlayer;
class iClient;
class CTestClientApp : public CWinApp
{
public:
	cPlayer* aPlayer[c_nLink];
	iClient* aClient[c_nLink];
	void OnTimer();
	CTestClientApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestClientApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTestClientApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CTestClientApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTCLIENT_H__C3781B6D_A1D7_4A8F_A1EB_05811E6642D2__INCLUDED_)
