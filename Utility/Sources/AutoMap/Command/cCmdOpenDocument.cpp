// cCmdOpenDocument.cpp: implementation of the cCmdOpenDocument class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdOpenDocument.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdOpenDocument::cCmdOpenDocument(iCommandMachine* p) : cCommand(p)
{
	
}

cCmdOpenDocument::~cCmdOpenDocument()
{

}

eDoType cCmdOpenDocument::Do()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_LOAD;
	strcpy(param.szFile,GetParam().strFile);
	GetGround()->SetParam(param);

	return DO_BREAKUNDO;
}

stCommand_Document& cCmdOpenDocument::GetParam()
{
	return *((stCommand_Document*)GetBuffer());
}

