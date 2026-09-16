// TestServer.h : main header file for the TESTSERVER application
//

#if !defined(AFX_TESTSERVER_H__7FAC5272_19DE_49E5_81CE_CAB15B193847__INCLUDED_)
#define AFX_TESTSERVER_H__7FAC5272_19DE_49E5_81CE_CAB15B193847__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTestServerApp:
// See TestServer.cpp for the implementation of this class
//
class iServer;
class cWorld;

class CTestServerDlg;
class CTestServerApp : public CWinApp
{
public:
	CTestServerApp();
	iServer* m_pServer;
	iServer* GetServer(){return m_pServer;}
	cWorld* m_pWorld;
	cWorld* GetWorld(){return m_pWorld;}

	CTestServerDlg* m_pDlg;
	void OutputString(LPCSTR szAccount,LPCSTR szMessage);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestServerApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTestServerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTSERVER_H__7FAC5272_19DE_49E5_81CE_CAB15B193847__INCLUDED_)
