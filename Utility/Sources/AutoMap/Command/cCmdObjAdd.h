// cCmdObjAdd.h: interface for the cCmdObjAdd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCMDOBJADD_H__5103B4FD_1F98_4635_9B92_BFCFDECE626A__INCLUDED_)
#define AFX_CCMDOBJADD_H__5103B4FD_1F98_4635_9B92_BFCFDECE626A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cCommand.h"

class cCmdObjAdd : public cCommand  
{
public:
	cCmdObjAdd(iCommandMachine* p);
	virtual ~cCmdObjAdd();

	stCommand_AddObj& GetParam()
	{return  *( (stCommand_AddObj*) GetBuffer() );};

	eDoType Redo();
	eDoType Undo();
	eDoType Do() ;
};

#endif // !defined(AFX_CCMDOBJADD_H__5103B4FD_1F98_4635_9B92_BFCFDECE626A__INCLUDED_)
