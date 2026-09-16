// GMDebugDoc.cpp : implementation of the CGMDebugDoc class
//

#include "stdafx.h"
#include "GMDebug.h"

#include "GMDebugDoc.h"
#include "GMDebugView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMDebugDoc

IMPLEMENT_DYNCREATE(CGMDebugDoc, CDocument)

BEGIN_MESSAGE_MAP(CGMDebugDoc, CDocument)
	//{{AFX_MSG_MAP(CGMDebugDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMDebugDoc construction/destruction

CGMDebugDoc::CGMDebugDoc()
{
	m_docIP = 0;
	m_docRelayID = -1;

	m_nMessage = 0;
}

CGMDebugDoc::~CGMDebugDoc()
{
}

void CGMDebugDoc::Trace(LPCTSTR strMessage)
{
	if (strMessage && strMessage[0])
	{
		m_nMessage++;
		POSITION pos = GetFirstViewPosition();
		if (pos)
		{
			CGMDebugView* pView = (CGMDebugView*)GetNextView(pos);
			if (pView)
			{
				pView->AddMessage(m_nMessage, strMessage);
			}
		}
	}
}


BOOL CGMDebugDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGMDebugDoc serialization

void CGMDebugDoc::Serialize(CArchive& ar)
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
// CGMDebugDoc diagnostics

#ifdef _DEBUG
void CGMDebugDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGMDebugDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGMDebugDoc commands
