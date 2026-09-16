// WorldEditorDoc.cpp : implementation of the CWorldEditorDoc class
//

#include "stdafx.h"
#include "WorldEditor.h"

#include "Mainfrm.h"

#include "WorldEditorDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc

IMPLEMENT_DYNCREATE(CWorldEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CWorldEditorDoc, CDocument)
	//{{AFX_MSG_MAP(CWorldEditorDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc construction/destruction

CWorldEditorDoc::CWorldEditorDoc()
{
	// TODO: add one-time construction code here

}

CWorldEditorDoc::~CWorldEditorDoc()
{
}

BOOL CWorldEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	return theApp.GetMainFrame()->OnNewDocument();
	
}



/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc serialization

void CWorldEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc diagnostics

#ifdef _DEBUG
void CWorldEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWorldEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc commands


BOOL CWorldEditorDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	// TODO: Add your specialized creation code here
	return theApp.GetMainFrame()->OnOpenDocument(lpszPathName);
	
}

BOOL CWorldEditorDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// TODO: Add your specialized code here and/or call the base class
	return theApp.GetMainFrame()->OnSaveDocument(lpszPathName);

	return TRUE;
//	return CDocument::OnSaveDocument(lpszPathName);
}
