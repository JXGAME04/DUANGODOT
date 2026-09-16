// cCmdObjRemove.cpp: implementation of the cCmdObjRemove class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdObjRemove.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdObjRemove::cCmdObjRemove(iCommandMachine* p):cCommand(p)
{

}

cCmdObjRemove::~cCmdObjRemove()
{
	stCommand_AddObj& param = GetParam();
	RELEASE(param.pObj);
}

eDoType cCmdObjRemove::Redo()
{
	stCommand_AddObj& param = GetParam();
	GetGround()->RemoveiObj(param.pObj);
	return DO_CANUNDO;
}

eDoType cCmdObjRemove::Undo()
{
	stCommand_AddObj& param = GetParam();
	GetGround()->AddiObj(param.pObj);
	return DO_CANUNDO;
}

eDoType cCmdObjRemove::Do()
{
	return Redo();
}
