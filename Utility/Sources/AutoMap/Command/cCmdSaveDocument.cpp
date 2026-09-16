// cCmdSaveDocument.cpp: implementation of the cCmdSaveDocument class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdSaveDocument.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdSaveDocument::cCmdSaveDocument(iCommandMachine* p) : cCommand(p)
{
	
}

cCmdSaveDocument::~cCmdSaveDocument()
{

}

eDoType	cCmdSaveDocument::Do()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_SAVE;
	strcpy(param.szFile,GetParam().strFile);
	GetGround()->SetParam(param);

	return DO_CANNOTUNDO;
}

stCommand_Document& cCmdSaveDocument::GetParam()
{
	return *((stCommand_Document*)GetBuffer());
}

