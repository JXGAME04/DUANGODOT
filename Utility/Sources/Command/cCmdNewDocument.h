// cCmdNewDocument.h: interface for the cCmdNewDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDNEWDOCUMENT_H__C83583B9_9C9C_4AF3_B3C3_C8175D3260B1__INCLUDED_)
#define AFX_CCMDNEWDOCUMENT_H__C83583B9_9C9C_4AF3_B3C3_C8175D3260B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdNewDocument : public cCommand  
{
public:
	cCmdNewDocument(iCommandMachine* p);
	virtual ~cCmdNewDocument();

	stCommand_Document& GetParam();

	eDoType	Do() ;
};

#endif // !defined(AFX_CCMDNEWDOCUMENT_H__C83583B9_9C9C_4AF3_B3C3_C8175D3260B1__INCLUDED_)
