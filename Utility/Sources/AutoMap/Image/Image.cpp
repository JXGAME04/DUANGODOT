// Image.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Image.h"

#include "cImageSpr.h"
#include "cImageTga.h"
#include "cImagePsd.h"
#include "cImageBmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
// CImageApp

BEGIN_MESSAGE_MAP(CImageApp, CWinApp)
	//{{AFX_MSG_MAP(CImageApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageApp construction
void InitAlpha4444();

CImageApp::CImageApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	char str[128];
	GetModuleFileName(GetModuleHandle(NULL),str,128);
	CString sPath = str;	
	int pos = sPath.ReverseFind('\\');
	m_strPath = sPath.Left(pos);

	InitAlpha4444();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CImageApp object

CImageApp theApp;

iImage* Image_CreateImage(eImage e)
{
	switch (e)
	{
	case IMAGE_SPR:
		return new cImageSpr;
	case IMAGE_TGA:
		return new cImageTga;
	case IMAGE_PSD:
		return new cImagePsd;
	case IMAGE_BMP:
		return new cImageBmp;
	}
	return NULL;
}

iImage* Image_CreateImage(LPCSTR sz)
{
	CString s = sz;
	CString sExt = s.Right(3);
	sExt.MakeLower();
	eImage e = IMAGE_NULL;
	if (sExt == "spr")
		e = IMAGE_SPR;
	else if (sExt == "tga")
		e = IMAGE_TGA;
	else if (sExt == "psd")
		e = IMAGE_PSD;
	else if (sExt == "bmp")
		e = IMAGE_BMP;
	else if (sExt == "tgb")
		e = IMAGE_TGA;
	return Image_CreateImage(e);
}
