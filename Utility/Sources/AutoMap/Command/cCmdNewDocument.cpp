// cCmdNewDocument.cpp: implementation of the cCmdNewDocument class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdNewDocument.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdNewDocument::cCmdNewDocument(iCommandMachine* p) : cCommand(p)
{
	
}

cCmdNewDocument::~cCmdNewDocument()
{

}

eDoType	cCmdNewDocument::Do()
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;
	param.eCommand = iGround::GCOM_NEW;
	strcpy(param.szFile,GetParam().strFile);
	if (GetGround())
		GetGround()->SetParam(param);

	return DO_BREAKUNDO;
}

stCommand_Document& cCmdNewDocument::GetParam()
{
	return *((stCommand_Document*)GetBuffer());
}

