// cScreenObj.cpp: implementation of the cScreenObj class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cScreenObj.h"

#include "cGraphics.h"
#include "iItem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cScreenObj::cScreenObj(cGraphics* p) : cObj(p)
{
	m_bInScreen = false;
}

cScreenObj::~cScreenObj()
{

}

void cScreenObj::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stScreenObjParam;
	cObj::Init(false);
}

////////////////////////////////////////////////////////////////////////////
//graphics
cScreen* cScreenObj::GetScreen()
{
	return GetGraphics()->GetcScreen();
}

////////////////////////////////////////////////////////////////////////////
//param
HRESULT	cScreenObj::setParam(stObjParam& pm) 
{
	HRESULT hr = cObj::setParam(pm);
	if (!pm.IsEMask(MASK_SCREENOBJ))
		return hr;
	
	stScreenObjParam& param = *((stScreenObjParam* )&pm);

	if (param.IsSetMask(stScreenObjParam::MASK_SCREENOBJ_OFFSET))
		Offset(param.ptOffset);
	return 0;
}

void cScreenObj::Offset(POINT pt)
{
	SetOffset(pt);
}

void cScreenObj::GetOffset(POINT ptScreen,cPoint& ptGround)
{
	ptGround.x = ptScreen.x;
	ptGround.y = ptScreen.y;
}

////////////////////////////////////////////////////////////////////////////
//save and load	
HRESULT	cScreenObj::Load(const stSection* p)
{
	cObj::Load(p);

	if (p != NULL)
	{
		stScreenObjParam& param = *GetScreenObjParam();
		GetSectionValue(p,"Screenoffset",param.ptOffset);
	}
	return S_OK;
}

HRESULT	cScreenObj::Save(stSection* p)
{
	cObj::Save(p);

	ASSERT(p);

	stScreenObjParam& param = *GetScreenObjParam();
	GetSectionValue(p,"Screenoffset",param.ptOffset);
	return S_OK;
}


