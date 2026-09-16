// WorldEditor.h : main header file for the WORLDEDITOR application
//

#if !defined(AFX_WORLDEDITOR_H__81791FC1_DEBC_4217_8EB3_2745DC8A58D8__INCLUDED_)
#define AFX_WORLDEDITOR_H__81791FC1_DEBC_4217_8EB3_2745DC8A58D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "iCommon.h"

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp:
// See WorldEditor.cpp for the implementation of this class
//
#define SAFE_RELEASE(a) if(a){a->Release();a=NULL;}
#define SAFE_DELETE(a) if(a){delete a;a=NULL;}
const int c_nScreenWidth = 1024;
const int c_nScreenHeight = 768;


class CMainFrame;
class CWorldEditorDoc;
class CWorldEditorView;
class COutputList;
class CGraphicsWnd;
class CDirTree;
class iStatusMachine;
class iGraphics;

class cAppLog;

class CWorldEditorApp : public CWinApp
{
public:
	CWorldEditorApp();
	~CWorldEditorApp();
	HANDLE m_hGMutex;

	cAppLog* m_pLog;
	void WriteLogString(LPCSTR szString);

	BOOL IsTheGameRun();

	CMainFrame* m_pMainFrame;
	void SetMainFrame(CMainFrame* p){m_pMainFrame = p;}
	CMainFrame*			GetMainFrame();
	CWorldEditorDoc*	GetWorldEditorDoc();
	CWorldEditorView*	GetWorldEditorView();
	COutputList*		GetOutput();
	CGraphicsWnd*		GetGraphicsWnd();
	CDirTree*			GetDirTree();
	iStatusMachine*		GetStatusMachine();
	iGraphics*			GetGraphics();

	void OutputMsg(int nStatus, LPCTSTR strMsg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CWorldEditorApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CWorldEditorApp theApp;
/////////////////////////////////////////////////////////////////////////////

void AppOutputMsg(int nStatus, LPCTSTR strMsg);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORLDEDITOR_H__81791FC1_DEBC_4217_8EB3_2745DC8A58D8__INCLUDED_)
