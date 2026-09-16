// cCmdCameraOffsetTo.cpp: implementation of the cCmdCameraOffsetTo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdCameraOffsetTo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdCameraOffsetTo::cCmdCameraOffsetTo(iCommandMachine* p) : cCommand(p)
{

}

cCmdCameraOffsetTo::~cCmdCameraOffsetTo()
{

}

void cCmdCameraOffsetTo::OffsetCameraTo(cPoint pt)
{
	iCamera::stParam param;
	param.mask = iCamera::MASK_TO | iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	param.ptTo = pt;
	GetCamera()->SetParam(param);
}

eDoType cCmdCameraOffsetTo::Do()
{
	iCamera::stParam m_param;
	m_param.mask = 0;
	GetCamera()->GetParam(m_param);
	m_ptOldCamera = m_param.ptTo;
	return Redo();
}

eDoType	cCmdCameraOffsetTo::Redo()
{
	stCommand_OffsetCameraTo& m_param = GetParam();
	OffsetCameraTo(m_param.ptTo);
	return DO_BREAKUNDO;
}

eDoType	cCmdCameraOffsetTo::Undo()
{
	OffsetCameraTo(m_ptOldCamera);
	return DO_CANUNDO;
}
