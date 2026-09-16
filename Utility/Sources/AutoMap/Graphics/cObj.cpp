// cObj.cpp: implementation of the cObj class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "cGraphics.h"
#include "cObj.h"
#include "cCamera.h"
#include "cGround.h"

#include "iItem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cObj::cObj(cGraphics* p) 
{
	m_ulCount = 1;
	m_pGraphics = p;

	m_pParam = NULL;
}

void cObj::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stObjParam;
	stObjParam& m_param = *GetObjParam();
	m_param.crColor = cColor(255,255,255,255);
	m_param.lMapeditEnum = 0;
}

cObj::~cObj()
{
	ASSERT(m_pParam);
	delete m_pParam;
}

/**************************************************************************/
//com
/**************************************************************************/
ULONG cObj::AddRef() 
{
	return ++m_ulCount;
}

ULONG cObj::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

/**************************************************************************/
//Graphics
/**************************************************************************/
LPDIRECT3DDEVICE8 cObj::GetDevice()
{
	return GetGraphics()->GetDevice3D();
}

/**************************************************************************/
HRESULT	cObj::Load(const stSection* p)
{
	stObjParam& param = *GetObjParam();
//	GetFileValue(pItem,"elementtype",cast_int(param.eElement),index);
	if (p != NULL)
	{
		GetSectionValue(p,"color",param.crColor);
		GetSectionValue(p,"mapedit",param.lMapeditEnum);
	}
	param.lMapeditEnum = 0;
	return S_OK;
}

HRESULT	cObj::Save(stSection* p)
{
	ASSERT(p);
	stObjParam& param = *GetObjParam();
	SetSectionValue(p,"elementtype",cast_int(param.eElement));
	SetSectionValue(p,"color",param.crColor);
	SetSectionValue(p,"mapedit",param.lMapeditEnum);
	return S_OK;
}

HRESULT	cObj::LoadPack(void** pParam)
{
	long size = GetParamSize();
	memcpy(GetObjParam(),*pParam,size);
	*pParam = (void*)( ((char*)(*pParam) )+ size);
	stObjParam& param = *GetObjParam();
	param.lMapeditEnum = 0;
	param.size = size;
	return S_OK;
}

HRESULT	cObj::LoadBinary(CFile& f)
{
	long size = GetParamSize();
	f.Read(GetObjParam(),GetParamSize());
	stObjParam& param = *GetObjParam();
	param.lMapeditEnum = 0;
	if (GetParamSize() != size)
		return -1;
	return S_OK;
}

HRESULT	cObj::SaveBinary(CFile& f)
{
	f.Write(GetObjParam(),GetParamSize());
	return S_OK;
}

/**************************************************************************/
//param 
/**************************************************************************/

HRESULT cObj::GetParam(stObjParam& param) 
{
	if (param.IsSetMask(stObjParam::MASK_OBJ_COMMAND))
		return OnCommand(param.eCommand,param);
	else if (param.IsMaskEmpty() )
	{
		int size = param.size;
		memcpy(&param,GetObjParam(),param.size);
		param.size = size;
	}
	return S_OK;
}

HRESULT	cObj::SetParam(stObjParam& param,bool bRunNow)
{
	if (bRunNow)
		return setParam(param);

	AddRef();
	param.wParam = (long)this;
	param.lParam = 0;
	GetGraphics()->PushMessage(&param,param.size,GT_OBJ);
	return 0;
}

HRESULT	cObj::OnCommand(long eCommand,stObjParam& param)
{
	return 0;
}

HRESULT	cObj::setParam(stObjParam& param) 
{
	if (!param.IsEMask(MASK_OBJ))
		return 0;
		
	if (param.IsSetMask(stObjParam::MASK_OBJ_COMMAND))
		return OnCommand(param.eCommand,param);
	
	stObjParam& m_param = *GetObjParam();

//	if (param.mask & stObjParam::MASK_ELEMENT)
//		m_param.eElement = param.eElement;
	if (param.IsSetMask(stObjParam::MASK_OBJ_COLOR))
		m_param.crColor = param.crColor;
	if (param.IsSetMask(stObjParam::MASK_OBJ_MAPEDIT))
		BitOP(m_param.lMapeditEnum,param.eOPMapedit,param.lMapeditEnum);
	return 0;
}

