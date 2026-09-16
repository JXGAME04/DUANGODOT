// cCmdGroundDynamic.cpp: implementation of the cCmdGroundDynamic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdGroundDynamic.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdGroundDynamic::cCmdGroundDynamic(iCommandMachine* p) : cCommand(p)
{

}

cCmdGroundDynamic::~cCmdGroundDynamic()
{

}

eDoType	cCmdGroundDynamic::Do()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	if (GetParam().nOperator == stCommand_Dynamic::OP_NEW)
	{
		param.eCommand = iGround::GCOM_NEWDYNAMIC;
	}
	else if (GetParam().nOperator == stCommand_Dynamic::OP_OPEN)
	{
		param.eCommand = iGround::GCOM_OPENDYNAMIC;
		strcpy(param.szDynamic,GetParam().strFile);
	}
	else if (GetParam().nOperator == stCommand_Dynamic::OP_SAVE)
	{
		param.eCommand = iGround::GCOM_SAVEDYNAMIC;
		strcpy(param.szDynamic,GetParam().strFile);
	}
	else
		return DO_NOTHING;
	GetGround()->SetParam(param);
	return DO_BREAKUNDO;
}

