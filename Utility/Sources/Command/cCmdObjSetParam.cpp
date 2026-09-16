// cCmdObjSetParam.cpp: implementation of the cCmdObjSetParam class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cCmdObjSetParam.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cCmdObjSetParam::cCmdObjSetParam(iCommandMachine* p):cCommand(p)
{
	m_bNextMoving = FALSE;
	m_bPrevMoving = FALSE;
	m_pParam = NULL;
}

cCmdObjSetParam::~cCmdObjSetParam()
{
	stCommand_SetObjParam& param = GetParam();
	RELEASE(param.pObj);
	delete param.pParam;
	delete m_pParam;
}

HRESULT	cCmdObjSetParam::SetParam(stCommand* param)
{
	cCommand::SetParam(param);

	stObjParam* p = GetParam().pParam;
	GetParam().pParam = (stObjParam*) new char[p->size];
	m_pParam = (stObjParam*) new char[p->size];

	memcpy(GetParam().pParam,p,p->size);
	m_pParam->size = GetParam().pParam->size;

	return 0;
}

eDoType cCmdObjSetParam::Redo()
{
	stCommand_SetObjParam& param = GetParam();
	SetObjParam(param.pParam);
	if (!GetObj()->IsInGraphics())
		return DO_CANNOTUNDO;
	if (m_bNextMoving)
		return DO_CONTINUE;
	if (m_pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_MOVING))
		return DO_CANUNDO;
	if (param.pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_OFFSET))
		return DO_CANUNDO;
	return DO_CANUNDO;
}

eDoType cCmdObjSetParam::Undo()
{
	SetObjParam(m_pParam);
	if (m_pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_MOVING))
	{
		OutputDebugString("undo moving\n");
	}
	if (m_pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_OFFSET))
	{
		OutputDebugString("undo offset\n");
	}
	if (!GetObj()->IsInGraphics())
		return DO_CANNOTUNDO;
	if (m_bPrevMoving)
		return DO_CONTINUE;
	if (m_pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_MOVING))
		return DO_CANUNDO;
	if (m_pParam->IsMask(stGroundObjParam::MASK_GROUNDOBJ_OFFSET))
		return DO_CANUNDO;
	return DO_CANUNDO;
}

eDoType cCmdObjSetParam::Do()
{
	int size = m_pParam->size;
	m_pParam->ClearMask();
	GetObj()->GetParam(*m_pParam);
	m_pParam->SetMask(GetEMask(),GetMask());
	m_pParam->size = size;
	return Redo();
}

MaskEnum cCmdObjSetParam::GetEMask()
{
	return GetParam().pParam->GetEMask();
}

long cCmdObjSetParam::GetMask()
{
	return GetParam().pParam->GetMask();
}

void cCmdObjSetParam::SetObjParam(stObjParam* p)
{
	GetObj()->setParam(*p);	
}

BOOL cCmdObjSetParam::Merge(iCommand* p)
{
	ASSERT(p->GetType() == GetType());
	cCmdObjSetParam* pCmd = dynamic_cast<cCmdObjSetParam*>(p);
	ASSERT(pCmd);
	if (pCmd->GetObj() != GetObj())
		return false;
	if (pCmd->GetMask() != GetMask())
	{
		if (pCmd->GetMask() == stGroundObjParam::MASK_GROUNDOBJ_MOVING)
		{
			m_bNextMoving = TRUE;
			pCmd->m_bPrevMoving = TRUE;
		}
		if (GetMask() == stGroundObjParam::MASK_GROUNDOBJ_MOVING)
		{
			pCmd->m_bPrevMoving = TRUE;
			m_bNextMoving = TRUE;
		}
		return false;
	}
	{
		//exchange the buffer
		stObjParam* pTemp = GetParam().pParam;
		GetParam().pParam = pCmd->GetParam().pParam;
		pCmd->GetParam().pParam = pTemp;
	}
	return true;
}
