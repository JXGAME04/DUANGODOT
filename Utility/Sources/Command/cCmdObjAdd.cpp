// cCmdObjAdd.cpp: implementation of the cCmdObjAdd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdObjAdd.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdObjAdd::cCmdObjAdd(iCommandMachine* p):cCommand(p)
{

}

cCmdObjAdd::~cCmdObjAdd()
{
	stCommand_AddObj& param = GetParam();
	RELEASE(param.pObj);
}

eDoType cCmdObjAdd::Redo()
{
	stCommand_AddObj& param = GetParam();
	GetGround()->AddiObj(param.pObj);
	return DO_CANUNDO;
}

eDoType cCmdObjAdd::Undo()
{
	stCommand_AddObj& param = GetParam();
	GetGround()->RemoveiObj(param.pObj);
	return DO_CANUNDO;
}

eDoType cCmdObjAdd::Do()
{
	return Redo();
}
