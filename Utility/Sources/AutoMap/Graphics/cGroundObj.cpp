// cGroundObj.cpp: implementation of the cGroundObj class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGroundObj.h"
#include "cCamera.h"
#include "cGround.h"

#include "cGraphics.h"
#include "iItem.h"

#include "fileop.h"
#include "planedoodad.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cGroundObj::cGroundObj(cGraphics* p) : cObj(p)
{
	m_bInGround = false;
	m_xRegion = -1;
	m_yRegion = -1;

	m_bModify = FALSE;
	m_bOffsetOfRegion = FALSE;
	m_bChild = FALSE;
	m_bDisplay = TRUE;
}

cGroundObj::~cGroundObj()
{

}

void cGroundObj::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stGroundObjParam;
	cObj::Init(false);
	stGroundObjParam& m_param = *GetGroundObjParam();
	m_param.eDynamic = DYNAMIC_STATIC;
	m_param.eKind = KIND_NORMAL;
	m_param.iTemplate = 0;
	m_param.lGroupID = 0;
	m_param.lID = 0;
	m_param.ptOffset = cPoint(0,0);
	m_param.szEvent[0] = 0;
	m_param.szGroup[0] = 0;
	m_param.szTemplate[0] = 0;
}

////////////////////////////////////////////////////////////////////////////
//graphics
cGround* cGroundObj::GetGround()
{
	return GetGraphics()->GetcGround();
}

cCamera* cGroundObj::GetCamera()
{
	return GetGraphics()->GetcCamera();
}


////////////////////////////////////////////////////////////////////////////
//region
void cGroundObj::SetRegion(int x,int y)
{
	m_xRegion = x;
	m_yRegion = y;

	stGroundObjParam& param = *GetGroundObjParam();
	cPoint pt = param.ptOffset;
	if (m_bOffsetOfRegion )
	{
		pt.x += x*c_nWidthRegion;
		pt.y += y*c_nHeightRegion;
		SetOffset(pt);
		m_bOffsetOfRegion = FALSE;
	}
	else
	{
		SetOffset(pt);
	}
}

////////////////////////////////////////////////////////////////////////////
//param
HRESULT	cGroundObj::OnCommand(long eCommand,stObjParam& pm)
{
	stGroundObjParam& param = *((stGroundObjParam* )&pm);
	switch(param.eCommand)
	{
	case stGroundObjParam::COM_GROUNDOBJ_OFFSET:
		{
			cPoint pt = GetOffset();
			param.ptOffset += pt;
			param.SetMask(MASK_GROUNDOBJ,stGroundObjParam::MASK_GROUNDOBJ_OFFSET);
			setParam(param);
		}
		break;
	}
	return 0;
}

HRESULT	cGroundObj::setParam(stObjParam& pm) 
{
	AddModify();

	HRESULT hr = cObj::setParam(pm);

	if (!pm.IsEMask(MASK_GROUNDOBJ))
		return hr;

	stGroundObjParam& param = *((stGroundObjParam* )&pm);
	stGroundObjParam& m_param = *GetGroundObjParam();

	//重载mapedit这个操作	
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_MOVING))
	{
		BOOL b = IsInGround() && (!IsChild());
		if (b)
			GetGround()->ObjNotify(this,NOTIFY_MOVING_CHANGING,m_param.bMoving);
		m_param.bMoving = param.bMoving;
		if (b)
			GetGround()->ObjNotify(this,NOTIFY_MOVING_CHANGED,m_param.bMoving);
	}

	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_OFFSET))
	{
		BOOL b = IsInGround() && (!IsChild());
		if (m_param.ptOffset != param.ptOffset)
		{
			if (b)
				GetGround()->ObjNotify(this,NOTIFY_POSITION_CHANGING);
			Offset(param.ptOffset);
			if (b)
				GetGround()->ObjNotify(this,NOTIFY_POSITION_CHANGED);
		}
	}
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_GROUPID))
		m_param.lGroupID = param.lGroupID;
//静态物体
	if (param.IsSetMask(stPlaneParam::MASK_GROUNDOBJ_KIND))
		m_param.eKind = param.eKind;
	if (param.IsSetMask(stPlaneParam::MASK_GROUNDOBJ_TEMPLATE))
	{
		strcpy(m_param.szTemplate,param.szTemplate);
		m_param.iTemplate = param.iTemplate;
	}
//动态物体
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_DYNAMIC))
	{
		m_param.eDynamic = param.eDynamic;
		if (m_param.eDynamic != DYNAMIC_STATIC)
			CreateID();
	}
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_EVENT))
	{
		strcpy(m_param.szEvent,param.szEvent);
//		GetGround()->ObjNotify(this,NOTIFY_EVENT);
	}
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_GROUPFILE))
		strcpy(m_param.szGroup,param.szGroup);
	if (param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_ID))
		m_param.lID = param.lID;

	return 0;
}

void cGroundObj::Offset(cPoint pt)
{
	SetOffset(pt);
}

void cGroundObj::GetOffset(POINT ptScreen,cPoint& ptGround)
{
	cPoint pt(ptScreen.x,ptScreen.y,GetOffset().z);
	GetCamera()->ScreenToGround(pt,ptGround);
}

void cGroundObj::CreateID()
{
//mapedit only
	stGroundObjParam& m_param = *GetGroundObjParam();
//几个策划的ini File必须合理的初始化
//	AppGetDynamicNum("dynamicnum",m_param.lID);
}
/**************************************************************************/
//save and load	
HRESULT	cGroundObj::LoadPack(void** pParam)
{
	HRESULT hr = cObj::LoadPack(pParam);
	if (FAILED(hr))
		return hr;
//因为存盘的点使用中心点，所以offset().x可能超过c_nWidthRegion;
//但是因为长宽小于400，所以不可能 > c_nWidthRegion*2;
//	if (GetOffset().x < c_nWidthRegion*2 && GetOffset().y < c_nHeightRegion*2)
//		m_bOffsetOfRegion = TRUE;
//但是对于有高度的物体，这个条件并不足够，y值相差可能很远
	if (fabs(GetOffset().x) < c_nWidthRegion*2 || fabs(GetOffset().y) < c_nHeightRegion*2)
		m_bOffsetOfRegion = TRUE;
	return S_OK;
}

HRESULT	cGroundObj::Load(const stSection* p)
{
	cObj::Load(p);

	if (p != NULL)
	{
		stGroundObjParam& param = *GetGroundObjParam();

		m_bOffsetOfRegion = TRUE;
		if (!GetSectionValue(p,"regionoffset",param.ptOffset))
		{
			m_bOffsetOfRegion = FALSE;
			GetSectionValue(p,"groundoffset",param.ptOffset);
		}
		GetSectionValue(p,"groupid",param.lGroupID);
		GetSectionValue(p,"event",param.szEvent);
		GetSectionValue(p,"id",param.lID);
		GetSectionValue(p,"groupfile",param.szGroup);
		GetSectionValue(p,"template",param.szTemplate);
		GetSectionValue(p,"templateindex",param.iTemplate);

		CString strValue;
		if (GetSectionValue(p,"kind",strValue))
			param.eKind = (KindEnum)StringToPlaneTableValue(TP_KIND,strValue);
	}
	
//因为存盘的点使用中心点，所以offset().x可能超过c_nWidthRegion;
//但是因为长宽小于400，所以不可能 > c_nWidthRegion*2;
//但是对于有高度的物体，这个条件并不足够，y值相差可能很远
	if (!m_bOffsetOfRegion)
	if (fabs(GetOffset().x) < c_nWidthRegion*2 || fabs(GetOffset().y) < c_nHeightRegion*2)
		m_bOffsetOfRegion = TRUE;

	return S_OK;
}

HRESULT	cGroundObj::Save(stSection* p)
{
	cObj::Save(p);

	ASSERT(p);

	stGroundObjParam& param = *GetGroundObjParam();
	cPoint pt = param.ptOffset;
	if (m_xRegion > 0 && m_yRegion > 0)
	{
		pt.x -= m_xRegion * c_nWidthRegion;
		pt.y -= m_yRegion * c_nHeightRegion;
		SetSectionValue(p,"regionoffset",pt);
	}
	SetSectionValue(p,"groundoffset",param.ptOffset);
	SetSectionValue(p,"groupid",param.lGroupID);
	SetSectionValue(p,"event",param.szEvent);
	SetSectionValue(p,"id",param.lID);
	SetSectionValue(p,"groupfile",param.szGroup);
	SetSectionValue(p,"template",param.szTemplate);
	SetSectionValue(p,"templateindex",param.iTemplate);

	CString sValue = GetTablePlaneValue(TP_KIND,param.eKind);
	SetSectionValue(p,"kind",sValue);

	return S_OK;
}
