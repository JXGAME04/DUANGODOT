// cCmdCameraSet.cpp: implementation of the cCmdCameraSet class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdCameraSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdCameraSet::cCmdCameraSet(iCommandMachine* p) : cCommand(p)
{

}

cCmdCameraSet::~cCmdCameraSet()
{

}

stCommand_SetCamera& cCmdCameraSet::GetParam()
{
	return *((stCommand_SetCamera*)GetBuffer());
}

eDoType cCmdCameraSet::Undo()
{
	SetCamera(m_param);
	return DO_CANUNDO;
}

void cCmdCameraSet::SetCamera(iCamera::stParam param)
{
	param.mask = iCamera::MASK_EYE | iCamera::MASK_TO | iCamera::MASK_SCALE;
	GetCamera()->SetParam(param);
	
	param.mask = iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	GetCamera()->SetParam(param);
}

eDoType	cCmdCameraSet::Do()
{
	m_param.mask = 0;
	GetCamera()->GetParam(m_param);

	return Redo();
}

eDoType	cCmdCameraSet::Redo()
{
	stCommand_SetCamera& m_param = GetParam();
	iCamera::stParam param;
	param.ptEye = m_param.ptEye;
	param.ptTo = m_param.ptTo;
	param.fScale = m_param.fScale;
	SetCamera(param);

	return DO_BREAKUNDO;
}
