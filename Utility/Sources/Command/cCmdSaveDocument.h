// cCmdSaveDocument.h: interface for the cCmdSaveDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDSAVEDOCUMENT_H__8551460D_76FF_4C6A_B25E_526F233E45B3__INCLUDED_)
#define AFX_CCMDSAVEDOCUMENT_H__8551460D_76FF_4C6A_B25E_526F233E45B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdSaveDocument : public cCommand  
{
public:
	cCmdSaveDocument(iCommandMachine* p);
	virtual ~cCmdSaveDocument();

	stCommand_Document& GetParam();

	eDoType Do() ;
};

#endif // !defined(AFX_CCMDSAVEDOCUMENT_H__8551460D_76FF_4C6A_B25E_526F233E45B3__INCLUDED_)
