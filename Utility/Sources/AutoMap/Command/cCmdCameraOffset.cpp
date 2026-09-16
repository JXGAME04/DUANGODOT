// cCmdCameraOffset.cpp: implementation of the cCmdCameraOffset class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdCameraOffset.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdCameraOffset::cCmdCameraOffset(iCommandMachine* p) : cCommand(p)
{

}

cCmdCameraOffset::~cCmdCameraOffset()
{
}

stCommand_OffsetCamera& cCmdCameraOffset::GetParam()
{
	return *((stCommand_OffsetCamera*)GetBuffer());
}

void cCmdCameraOffset::OffsetCameraTo(cPoint pt)
{
	iCamera::stParam param;
	param.mask = iCamera::MASK_TO | iCamera::MASK_COMMAND;
	param.eCommand = iCamera::CCOM_UPDATE;
	param.ptTo = pt;
	GetCamera()->SetParam(param);
}

eDoType cCmdCameraOffset::Do()
{
	//first time do
	iCamera::stParam param;
	param.mask = 0;
	GetCamera()->GetParam(param);
	m_ptStart = param.ptTo ;

	stCommand_OffsetCamera& m_param = GetParam();

	param.ptTo += m_param.ptTo;

	m_ptEnd = param.ptTo;
	OffsetCameraTo(m_ptEnd);
	return DO_CONTINUE;
}

eDoType cCmdCameraOffset::Redo()
{
	OffsetCameraTo(m_ptEnd);
	return DO_CONTINUE;
}

eDoType cCmdCameraOffset::Undo()
{
	OffsetCameraTo(m_ptStart);
	return DO_CONTINUE;
}

BOOL cCmdCameraOffset::Merge(iCommand* p)
{
	ASSERT(p->GetType() == GetType());
	cCmdCameraOffset* pCmd = dynamic_cast<cCmdCameraOffset*>(p);
	ASSERT(pCmd);
//	if (m_ptEnd == pCmd->m_ptStart)
	{
		cPoint pt = pCmd->m_ptEnd - m_ptStart;
//		if (fabs(pt.x) < c_tWidthRegion * c_ptMul.x / 2 
//			&& fabs(pt.y) < c_tHeightRegion * c_ptMul.y / 2)
		{
			m_ptEnd = pCmd->m_ptEnd;
			return TRUE;
		}
	}
	return FALSE;
}
