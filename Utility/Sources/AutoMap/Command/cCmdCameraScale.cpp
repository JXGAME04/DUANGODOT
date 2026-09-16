// cCmdCameraScale.cpp: implementation of the cCmdCameraScale class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdCameraScale.h"

#include "iGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdCameraScale::cCmdCameraScale(iCommandMachine* p):cCommand(p)
{

}

cCmdCameraScale::~cCmdCameraScale()
{

}

eDoType cCmdCameraScale::Redo()
{
	SetGroundScale(GetParam().fScale);
	return DO_CONTINUE;
}

eDoType cCmdCameraScale::Undo()
{
	SetGroundScale(m_fScale);
	return DO_CANUNDO;
}

eDoType cCmdCameraScale::Do()
{
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	m_fScale = param.fScale;
	Redo();
	return DO_CANUNDO;
}

//const float f_minScale = 8.6f;
const float f_minScale = 6.4f;
void cCmdCameraScale::SetGroundScale(float scale)
{
	if (scale >= 100.f)
		scale = 14.2f;
	if (scale < f_minScale)
		scale = f_minScale;
	iCamera::stParam param;
	param.mask = iCamera::MASK_SCALE | iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	param.fScale = scale;
	GetCamera()->SetParam(param);
}

BOOL cCmdCameraScale::Merge(iCommand* p)
{
	ASSERT(p->GetType() == GetType());
	cCmdCameraScale* pCmd = dynamic_cast<cCmdCameraScale*>(p);
	ASSERT(pCmd);
	GetParam().fScale = pCmd->GetParam().fScale;
	return TRUE;	
}
