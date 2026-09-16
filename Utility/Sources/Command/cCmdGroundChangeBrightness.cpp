// cCmdGroundChangeBrightness.cpp: implementation of the cCmdGroundChangeBrightness class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdGroundChangeBrightness.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdGroundChangeBrightness::cCmdGroundChangeBrightness(iCommandMachine* p) : cCommand(p)
{

}

cCmdGroundChangeBrightness::~cCmdGroundChangeBrightness()
{

}

eDoType cCmdGroundChangeBrightness::Redo()
{
	SetGroundBrightness(GetParam().crColor);
	return DO_CANUNDO;
}

eDoType cCmdGroundChangeBrightness::Undo()
{
	SetGroundBrightness(m_crColor);
	return DO_CANUNDO;
}

eDoType cCmdGroundChangeBrightness::Do()
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	m_crColor = param.crColor;
	return Redo();	
}

void cCmdGroundChangeBrightness::SetGroundBrightness(cColor crColor)
{
	crColor.Format();
	iGround::stParam param;
	param.mask = iGround::MASK_COLOR;
	param.crColor = crColor;
	GetGround()->SetParam(param);
}

BOOL cCmdGroundChangeBrightness::Merge(iCommand* p)
{
	ASSERT(p->GetType() == GetType());
	cCmdGroundChangeBrightness* pCmd = dynamic_cast<cCmdGroundChangeBrightness*>(p);
	ASSERT(pCmd);
	GetParam().crColor = pCmd->GetParam().crColor;
	return TRUE;	
}
