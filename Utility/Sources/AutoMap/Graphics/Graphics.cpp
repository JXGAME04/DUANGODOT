// Graphics.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Graphics.h"
#include "cGraphics3d.h"

#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/*
#pragma comment(lib,"message.lib")
#pragma comment(lib,"common.lib")
#pragma comment(lib,"image.lib")
#pragma comment(lib,"engine.lib")
*/

char g_strError[] = "dx Error!";

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CGraphicsApp

BEGIN_MESSAGE_MAP(CGraphicsApp, CWinApp)
	//{{AFX_MSG_MAP(CGraphicsApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphicsApp construction

CGraphicsApp::CGraphicsApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	AfxSocketInit();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGraphicsApp object

CGraphicsApp theApp;

iGraphics* Graphics_CreateGraphics(eEngineType eType, long verson1 ,long verson2 )
{
	iGraphics*  p = NULL;
	switch (verson1)
	{
	case Graphics_3D:
		p = new cGraphics3D; 
		break;
	}
	return p;
}

float FloatToInt(float a)
{
	float f_0 = 0.000001f;
	float f_1 = 1 - f_0;
	float f = a - floor(a);
	if ( f < f_0)
		a = floor(a);
	else if (f > f_1)
		a = floor(a) + 1.f;
	return a;
}
