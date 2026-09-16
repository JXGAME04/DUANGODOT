// cPlaneHor.cpp: implementation of the cPlaneHor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPlaneHor.h"

#include "cGraphics.h"
#include "cGround.h"

#include "iItem.h"
#include "position.h"
#include "cObstacle.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlaneHor::cPlaneHor(cGraphics* p) : cPlane(p)
{
}

cPlaneHor::~cPlaneHor()
{

}

void cPlaneHor::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneHorParam;		
	cPlane::Init(false);
	stPlaneHorParam& m_param = *GetPlaneHorParam();
	m_param.ePlane = PLANE_HORNORMAL;
	m_param.eSort = SORT_HORBELOWVER;
	m_pParam->eElement = GT_PLANEHORNORMAL;
}

void cPlaneHor::UpdateObstacle(cObstacle* p)
{
	stObstacleParam param;
	param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);
	GetRectPoint(param.aPoint);
	cPoint pt = param.aPoint[3];
	param.aPoint[3] = param.aPoint[2];
	param.aPoint[2] = pt;
	param.aPoint[4] = param.aPoint[0];
	param.nPoint = 5;
	p->setParam(param);
}

/////////////////////////////////////////////////////////////////
//param
/////////////////////////////////////////////////////////////////
HRESULT	cPlaneHor::setParam(stObjParam& pm) 
{
	HRESULT hr = cPlane::setParam(pm);

	stGroundObjParam& param = *((stGroundObjParam* )&pm);
	stGroundObjParam& m_param = *GetGroundObjParam();
	if (param.IsEMask(MASK_GROUNDOBJ) && param.IsSetMask(stGroundObjParam::MASK_GROUNDOBJ_MOVING))
	{
		if (m_param.bMoving == TRUE)
		{
			FindTexture();
		}
		else
		{
			ClearTexture();
		}
	}
	if (!pm.IsEMask(MASK_PLANE))
		return hr;
	return hr;
}

/////////////////////////////////////////////////////////////////
//Draw
/////////////////////////////////////////////////////////////////
//临时的绘制,所以不打光了
HRESULT cPlaneHor::Draw()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
	DWORD value = GetFinalColor().Value();
	stVertexGround* aVG = GetVG(0);
	for (int i=0; i<4; i++)
		aVG[i].color = value;
	GetGround()->SetTexture(GetTexture(0),GetiFrame(0));
	GetGround()->DrawQuadrangle(aVG);
	return S_OK;
}

BOOL cPlaneHor::IsShow() 
{
	if (!IsStatic() && !IsNoSave())
	{
		if (!GetGround()->IsDisplayEvent(GetEvent()))
			SetColor(cColor(255,255,255,128));
		else
			SetColor(cColor(255,255,255,255));
//			return FALSE;
	}
	return 	(GetGraphics()->IsShow(iGraphics::GS_HOR) && IsDisplay());
};

float cPlaneHor::GetArea()
{
	cRect r;
	GetGroundRect(r);
	return fabs(r.Width()*r.Height());
}

void cPlaneHor::GetGroundRect(cRect& r)
{
	ASSERT(GetResNum() == 1);
	stVertexGround v[4];
	GetQuadrangle(v,0);
	r = c_rectMin;
	for (int i=0; i<4; i++)
	{
		r.l = _cpp_min(r.l,v[i].sx);
		r.r = _cpp_max(r.r,v[i].sx);
		r.t = _cpp_min(r.t,v[i].sy);
		r.b = _cpp_max(r.b,v[i].sy);
	}
}

ePlane2Plane cPlaneHor::PositionPlane(cPlane* pPlane)
{
	ePlane2Plane e = cPlane::PositionPlane(pPlane);
	if (e != PP_UNKNOW)
		return e;
	cPlaneHor* p = dynamic_cast<cPlaneHor*>(pPlane);
	if ( p == NULL )
	{
//		ASSERT(0);
		return PP_BACK;
	}
	if (GetLayer() < p->GetLayer())
		e = PP_BACK;
	else if (GetLayer() > p->GetLayer())
		e = PP_FRONT;
	else
	{
		if (GetBottomPoint().y < p->GetBottomPoint().y)
			e = PP_BACK;
		else
			e = PP_FRONT;
	}
	return e;
}

BOOL cPlaneHor::ContainedDiamond(cRect rc)
{
	if (!IsShow())
		return FALSE;
	ASSERT(GetResNum() == 1);
	stVertexGround v[4];
	GetQuadrangle(v,0);
	cPoint aPoint[4];
	cPoint aP[4];
	for (int i=0; i<4; i++)
	{
		aPoint[i] = GetPoint(v[i]);
		aP[i].z = aPoint[i].z;
	}
	aP[0].x = (aPoint[0].x + aPoint[1].x)/2;
	aP[0].y = (aPoint[0].y);
	aP[1].x = (aPoint[1].x);
	aP[1].y = (aPoint[1].y + aPoint[3].y)/2;
	aP[2].x = (aPoint[2].x + aPoint[3].x)/2;
	aP[2].y = (aPoint[2].y);
	aP[3].x = (aPoint[0].x);
	aP[3].y = (aPoint[0].y + aPoint[2].y)/2;
	
	for (i=0; i<4; i++)
	{
		GroundToTile(aP[i]);
		TileToDiamond(aP[i]);
		if (!IsPointInRect(aP[i],rc))
			return FALSE;
	}
	return TRUE;
}


HRESULT cPlaneHor::FillVertexInfoNotCallGraphics()
{
	//现在小地图暂时不绘制平行的面
	stPlaneHorParam& m_param = *GetPlaneHorParam();

	FillVertexInfoHorAngle0(m_info,0);

	//与后者不同的是，需要偏移
	
	return S_OK;
}

HRESULT cPlaneHor::FillVertexInfo()
{
	stPlaneParam& m_param = *GetPlaneParam();
	stTexInfo info;
	GetTexInfo(info,0);
	FillVertexInfoHorAngle0(info,0);
	return S_OK;
}

HRESULT cPlaneHor::FillTextureUV()
{
	stTexInfo info;
	HRESULT hr = GetTexInfo(info,0);
	stVertexGround* v = GetVG(0);
	FillRectUV(v,info.rcTex);
	return S_OK;
}

BOOL cPlaneHor::FillVertexInfoHorAngle0(const stTexInfo& info,int iResource )
{
	stPlaneHorParam& m_param = *GetPlaneHorParam();
	//it is not right when it is a ver and the angle != 0

	cRect rc = info.rcTex;

	ULONG width = info.width;
	ULONG height = info.height;

	stVertexGround* v = GetVG(iResource);
	FillRectUV(v,rc);

	v[0].sx = 0;
	v[1].sx = (int)width ;
	v[2].sx = v[0].sx;
	v[3].sx = v[1].sx;
	v[0].sy = 0;
	v[1].sy = v[0].sy;
	v[2].sy = -(0 - (int)height);	
	v[3].sy = v[2].sy;
	for (int i=0; i<4; i++)
		v[i].sz = 0;

	int x = info.x;
	int y = info.y;
	for (i=0; i<4; i++)
	{
		v[i].sx -= x;
		v[i].sy -= y;
		v[i].sx /= c_ptScale.x;
		v[i].sy /= c_ptScale.y;
		v[i].sz /= c_ptScale.z;
		v[i].color = 0;
	}

	return TRUE;
}
/*
BOOL cPlaneHor::FillVertexInfoHor(int iResource)
{
	stPlaneParam& m_param = *GetPlaneParam();

	ASSERT(m_param.fAngleXY != 90);
	ASSERT(m_param.fAngleXY != 0);
	ASSERT(m_param.fAngleXY != 180);
	
	stTexInfo info;
	GetTexInfo(info,iResource);
	//y = info.height;
	cRect rc = info.rcTex;
	int x = info.x;
	int y = info.y;
	ULONG width = info.width;

	float fRightExtra = 0;
	float fLeftExtra = 0;
	ULONG height = info.height;

	//必须给出重心
	//重心可能超出图象!

	stVertexGround* v = GetVG(iResource);

	float fAngleXY = m_param.fAngleXY * c_fPI / 180.f;	//	底边在z = 0的角度
	float fAngleZ = m_param.fAngleZ  * c_fPI / 180.f;	//	斜边与z = 0的角度
	float fAngleRenderZ = c_fPI / 6;					//	a渲染时平面与Z轴角度

	float sina = sin(fAngleRenderZ);
	float tgb1 = tan(fAngleXY);
	float tgr1 = fabs(sina * tgb1);					// for example: a = 30, b = 45, then tgr = 0.5;
	float tgb2 = tan(c_fPI/2 - fAngleXY);
	float tgr2 = fabs(sina * tgb2);					// for example: a = 30, b = 45, then tgr = 0.5;

	float fWidth;
	float fHeight;
	float fZ;

	if (m_param.fAngleXY < 90)
	{
		fWidth = (width + fRightExtra - x) / cos(fAngleXY) / c_ptScale.x;
		fHeight = (x - fLeftExtra) / cos(c_fPI / 2 - fAngleXY) / c_ptScale.x;
		fZ = fHeight * tan(fAngleZ);

		v[2].sx = 0;
		v[2].sy = 0;
		v[2].sz = 0;
		v[3].sx = v[2].sx + fWidth * cos(fAngleXY);
		v[3].sy = v[2].sy - fWidth * sin(fAngleXY);
		v[3].sz = v[2].sz;
		v[0].sx = v[2].sx - fHeight * cos(c_fPI / 2 - fAngleXY);
		v[0].sy = v[2].sy - fHeight * sin(c_fPI / 2 - fAngleXY);
		v[0].sz = v[2].sz + fZ;
		v[1].sx = v[3].sx - fHeight * cos(c_fPI / 2 - fAngleXY);
		v[1].sy = v[3].sy - fHeight * sin(c_fPI / 2 - fAngleXY);
		v[1].sz = v[3].sz + fZ;

		cPoint p[4];
		p[2] = cPoint(x,y);
		p[3].x = width + fRightExtra;
		p[3].y = p[2].y - (p[3].x - p[2].x) * tgr1;
		p[0].x = p[2].x - (x - fLeftExtra);
		p[0].y = p[2].y - ((x - fLeftExtra) * tgr2 + fZ * c_ptScale.z);
		p[1].x = p[3].x - (x - fLeftExtra);
		p[1].y = p[3].y - ((x - fLeftExtra) * tgr2 + fZ * c_ptScale.z);

		for (int i=0; i<4; i++)
		{
			v[i].tu = p[i].x / (width+fRightExtra-fLeftExtra) * rc.Width() + rc.l;
			v[i].tv = p[i].y / height * rc.Height() + rc.t;
		}
	}
	else
	{
		fAngleXY = c_fPI - fAngleXY;

		fWidth = (x - fLeftExtra) / cos(fAngleXY) / c_ptScale.x;
		fHeight = (width + fRightExtra - x) / cos(c_fPI / 2 - fAngleXY) / c_ptScale.x;
		fZ = fHeight * tan(fAngleZ);

		v[2].sx = 0;
		v[2].sy = 0;
		v[2].sz = 0;
		v[3].sx = v[2].sx + fWidth * cos(fAngleXY);
		v[3].sy = v[2].sy + fWidth * sin(fAngleXY);
		v[3].sz = v[2].sz;
		v[0].sx = v[2].sx + fHeight * cos(c_fPI / 2 - fAngleXY);
		v[0].sy = v[2].sy - fHeight * sin(c_fPI / 2 - fAngleXY);
		v[0].sz = v[2].sz + fZ;
		v[1].sx = v[3].sx + fHeight * cos(c_fPI / 2 - fAngleXY);
		v[1].sy = v[3].sy - fHeight * sin(c_fPI / 2 - fAngleXY);
		v[1].sz = v[3].sz + fZ;

		cPoint p[4];
		p[3] = cPoint(x,y);
		p[2].x = fLeftExtra;
		p[2].y = p[3].y - (p[3].x - p[2].x) * tgr1;
		p[0].x = p[2].x + (width+fRightExtra - x);
		p[0].y = p[2].y - ((width+fRightExtra - x) * tgr2 + fZ * c_ptScale.z);
		p[1].x = p[3].x + (width+fRightExtra - x);
		p[1].y = p[3].y - ((width+fRightExtra - x) * tgr2 + fZ * c_ptScale.z);

		//u,v可能<0
		for (int i=0; i<4; i++)
		{
			v[i].sx -= v[3].sx;
			v[i].sy -= v[3].sy;
			v[i].sz -= v[3].sz;
			v[i].tu = p[i].x / (width+fRightExtra-fLeftExtra) * rc.Width() + rc.l;
			v[i].tv = p[i].y / height * rc.Height() + rc.t;
		}
	}

	return FALSE;
}
*/

HRESULT cPlaneHor::DrawTrace()
{
	cPlane::DrawTrace();
	stPlaneHorParam& m_param = *GetPlaneHorParam();
	if (GetMapedit() & Mapedit_Trace)
	{
		char sMsg[128];
		sprintf(sMsg,"(F6)Plane layer(%d) Height(%.1f)",
			m_param.nLayer,GetHeight());
		GetGraphics()->Trace(sMsg);
	}
	return S_OK;
}


