// cCmdOpenDocument.h: interface for the cCmdOpenDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDOPENDOCUMENT_H__D1BA6C09_9587_45CF_A1F8_F6810C10D7EB__INCLUDED_)
#define AFX_CCMDOPENDOCUMENT_H__D1BA6C09_9587_45CF_A1F8_F6810C10D7EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdOpenDocument : public cCommand  
{
public:
	cCmdOpenDocument(iCommandMachine* p);
	virtual ~cCmdOpenDocument();

	stCommand_Document& GetParam();

	eDoType	Do() ;
};

#endif // !defined(AFX_CCMDOPENDOCUMENT_H__D1BA6C09_9587_45CF_A1F8_F6810C10D7EB__INCLUDED_)
