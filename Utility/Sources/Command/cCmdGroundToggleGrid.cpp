// cCmdGroundToggleGrid.cpp: implementation of the cCmdGroundToggleGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Command.h"
#include "cCmdGroundToggleGrid.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdGroundToggleGrid::cCmdGroundToggleGrid(iCommandMachine* p):cCommand(p)
{

}

cCmdGroundToggleGrid::~cCmdGroundToggleGrid()
{

}

void cCmdGroundToggleGrid::SetGroundParam(BitOPEnum e, MapeditParamEnum pm)
{
	iGround::stParam param;
	param.lMapeditEnum = pm;
	param.eOPMapedit = e;
	param.mask = iGround::MASK_MAPEDIT;
	GetGround()->SetParam(param);
}

eDoType cCmdGroundToggleGrid::Undo()
{
	if (m_eParam & Mapedit_Grid)
		SetGroundParam(Bit_Add,Mapedit_Grid);
	else
		SetGroundParam(Bit_Remove,Mapedit_Grid);
	return DO_CANUNDO;
}

eDoType cCmdGroundToggleGrid::Redo()
{
	if (m_eParam & Mapedit_Grid)
		SetGroundParam(Bit_Remove,Mapedit_Grid);
	else
		SetGroundParam(Bit_Add,Mapedit_Grid);
	return DO_CANUNDO;
}

eDoType cCmdGroundToggleGrid::Do()
{
	iGround::stParam param;
	param.mask = 0;
	GetGround()->GetParam(param);
	m_eParam = param.lMapeditEnum;

	Redo();

	return DO_CANUNDO;
}
