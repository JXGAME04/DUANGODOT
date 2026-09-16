// cPlaneVer.cpp: implementation of the cPlaneVer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPlaneVer.h"
#include "position.h"
#include "cLight.h"
#include "cGraphics.h"
#include "cGround.h"
#include "cCamera.h"
#include "cMesh.h"
#include "Font/KFontD3D.h"

#include "position.h"
#include "iitem.h"
#include "cObstacle.h"
#include "iImage.h"

#include "fileop.h"
#include "planedoodad.h"
#include "iTemplate.h"

#include "stLine.h"
#include "GraphicsPublic.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlaneVer::cPlaneVer(cGraphics* p) : cPlane(p)
{
	m_fK = 0;
	m_fB = 0;
}

cPlaneVer::~cPlaneVer()
{
}

void cPlaneVer::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneVerParam;
	cPlane::Init(false);
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	m_param.ePlane = PLANE_VERMESH;
	m_param.eSort = SORT_VER;
	m_param.eVersort = VERSORT_SEAM;
	m_param.fSortOffY = 0.f;
}

//////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////
bool cPlaneVer::IsSortPoint()
{
	return GetVersortType() == VERSORT_POINT;
}

bool cPlaneVer::IsLightingPoint()
{
	return GetLightingType() == LIGHTING_POINT;
}

float cPlaneVer::GetSortSeamLengthSquare()
{
	cPoint p1,p2;
	GetSortSeam(p1,p2);
	cPoint p = p1 - p2;
	return (p.x*p.x + p.y*p.y);
}

BOOL cPlaneVer::ComputeLineFunction(float& k, float& b)
{
	cPoint p1,p2;
	GetSortSeam(p1,p2);
	ASSERT(p1.x != p2.x);
	k = (p1.y - p2.y) / (p1.x - p2.x);
	b = p1.y - k * p1.x ;
	return TRUE;
}

void cPlaneVer::GetBottomSeam(cPoint& p1, cPoint& p2)
{
	stVertexGround* aVG = GetVG(0);
	//z value is no use!
	p1 = GetPoint(aVG[2]);
	p1.z = 0;
	p2 = GetPoint(aVG[3]);
	p2.z = 0;
}

void cPlaneVer::GetLightedSeam(cPoint& p1, cPoint& p2)
{
	GetBottomSeam(p1,p2);
	p1.y -= 1.f;
	p2.y -= 1.f;
}

void cPlaneVer::GetLightedPoint(cPoint& p)
{
	p = GetOffset();
}

void cPlaneVer::GetLightingSeam(cPoint& p1, cPoint& p2)
{
	GetBottomSeam(p1,p2);
}

void cPlaneVer::GetLightingPoint(cPoint& p)
{
	p = GetOffset();
}

void cPlaneVer::GetSortSeam(cPoint& p1, cPoint& p2)
{
	GetBottomSeam(p1,p2);
}

void cPlaneVer::GetSortPoint(cPoint& p)
{
	p = GetOffset();
	p.y += GetSortOffY();
	p.z = 0.f;
}

void cPlaneVer::GetSubQuadrangle(stVertexGround v[4],cPoint p1,cPoint p2)
{
	stVertexGround* m_aVG = GetVG(0);
	cPoint pt1,pt2;
	GetBottomSeam(pt1,pt2);
	float w = pt2.x - pt1.x;
	ASSERT(w > 0);
	if (w == 0)
		return ;
	float h = pt2.y - pt1.y;

	float dz = (m_aVG[2].sz - m_aVG[3].sz) / (m_aVG[2].sx - m_aVG[3].sx);

	float uu = (m_aVG[1].tu - m_aVG[0].tu);
	float vv = (m_aVG[1].tv - m_aVG[0].tv);
	float du = uu / w;

	float dv = 0;
	if (h != 0 ) 
		dv = vv / h;
	
	v[0].sx = p1.x;
	v[0].sy = p1.y;
	v[0].sz = m_aVG[0].sz + (p1.x - m_aVG[0].sx) * dz;
	v[0].tu = m_aVG[0].tu + (p1.x - pt1.x) * du;
	v[0].tv = m_aVG[0].tv + (p1.y - pt1.y) * dv;

	v[1].sx = p2.x;
	v[1].sy = p2.y;
	v[1].sz = m_aVG[0].sz + (p2.x - m_aVG[0].sx) * dz;
	v[1].tu = m_aVG[0].tu + (p2.x - pt1.x) * du;
	v[1].tv = m_aVG[0].tv + (p2.y - pt1.y) * dv;

	v[2].sx = p1.x;
	v[2].sy = p1.y;
	v[2].sz = m_aVG[2].sz + (p1.x - m_aVG[0].sx) * dz;
	v[2].tu = m_aVG[2].tu + (p1.x - pt1.x) * du;
	v[2].tv = m_aVG[2].tv + (p1.y - pt1.y) * dv;

	v[3].sx = p2.x;
	v[3].sy = p2.y;
	v[3].sz = m_aVG[2].sz + (p2.x - m_aVG[0].sx) * dz;
	v[3].tu = m_aVG[2].tu + (p2.x - pt1.x) * du;
	v[3].tv = m_aVG[2].tv + (p2.y - pt1.y) * dv;

}

float cPlaneVer::GetArea()
{
	float fArea = 0;
	stVertexGround v[4];
	for (int j=0; j<GetResNum(); j++)
	{
		GetSelectVG(v,j);
		float h = fabs(v[0].sz - v[2].sz);
		float ww = (v[0].sx - v[1].sx)*(v[0].sx - v[1].sx)+(v[0].sy - v[1].sy)*(v[0].sy - v[1].sy);
		float w = sqrtf(ww);
		fArea +=  w*h;
	}
	return fArea;
}

ePlane2Plane cPlaneVer::PositionPlane(cPlane* p)
{
	ePlane2Plane e = cPlane::PositionPlane(p);
	if (e != PP_UNKNOW)
		return e;
	cPlaneVer* pVer = dynamic_cast<cPlaneVer*>(p);
	if (pVer == NULL)
		return PP_FRONT;
	cPoint p1,p2;
	GetBottomSeam(p1, p2);
	cPoint p3,p4;
	pVer->GetBottomSeam(p3, p4);
	eSeam2Seam	e1 = Seam2Seam(p1,p2,p3,p4);
	if (e1 == SS_Cross)
	{
		float f1 = PointToLineSign(p1,p3,p4);
		float f2 = PointToLineSign(p2,p3,p4);
		float f;
		if (abs(f1) > abs(f2))
			f = f1;
		else
			f = f2;
		if (f < 0)
			e1 = SS_Up;
		else 
			e1 = SS_Down;
	}
	if (e1 == SS_Up)
		return PP_BACK;
	else if (e1 == SS_Down)
		return PP_FRONT;
	return PP_FRONT;
}

void cPlaneVer::GetSelectVG(stVertexGround v[4], int i)
{
	GetQuadrangle(v,i);
	v[0].sz = _cpp_min(v[0].sz,v[1].sz);
	v[1].sz = v[0].sz;
	v[2].sz = _cpp_max(v[2].sz,v[3].sz);
	v[3].sz = v[2].sz;
}

//////////////////////////////////////////////////////////////
//set param
HRESULT cPlaneVer::setParam(stObjParam& pm)
{
	HRESULT hr = cPlane::setParam(pm);
	if (!pm.IsEMask(MASK_PLANE))
		return hr;

	stPlaneVerParam& param = *((stPlaneVerParam* )&pm);
	stPlaneVerParam& m_param = *GetPlaneVerParam();

	if (param.IsSetMask(stPlaneParam::MASK_PLANE_LIGHTED))
		m_param.eLighted = param.eLighted;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_LIGHTEDRADIUS))
		m_param.fLightedRadius = param.fLightedRadius;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_LIGHTING))
		m_param.eLighting = param.eLighting;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_ANGLEXY))
		m_param.fAngleXY = param.fAngleXY;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_VERSORT))
		m_param.eVersort = param.eVersort;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_TEXTUREUSE))
		m_param.eTextureUse = param.eTextureUse;
	if (param.IsSetMask(stPlaneParam::MASK_PLANE_SORTOFFY))
		m_param.fSortOffY = param.fSortOffY;
	return 0;
}

//////////////////////////////////////////////////////////////
//contain
BOOL cPlaneVer::ContainedDiamond(cRect rc)
{
	if (!IsShow())
		return FALSE;
	cPoint p1,p2;
	GetBottomSeam(p1,p2);
	GroundToTile(p1);
	TileToDiamond(p1);
	if (!IsPointInRect(p1,rc))
		return FALSE;
	GroundToTile(p2);
	TileToDiamond(p2);
	if (!IsPointInRect(p2,rc))
		return FALSE;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//draw
//////////////////////////////////////////////////////////////////////
BOOL cPlaneVer::IsShow()
{
	if (!IsStatic() && !IsNoSave())
	{
		if (!GetGround()->IsDisplayEvent(GetEvent()))
			SetColor(cColor(255,255,255,128));
		else
			SetColor(cColor(255,255,255,255));
//			return FALSE;
	}
	return 	(GetGraphics()->IsShow(iGraphics::GS_VER) && IsDisplay());	
}

HRESULT cPlaneVer::DrawSub(cPoint p1,cPoint p2)
{
	GetGround()->GetPlaneDrawNum();
	
	cPoint _p1,_p2;
	GetBottomSeam(_p1,_p2);
	if (_p1 == p1 && _p2 == p2)
		return Draw();

	stPlaneParam& m_param = *GetPlaneParam();

	DWORD value = GetFinalColor().Value();
	stVertexGround v[4];
	GetSubQuadrangle(v,p1,p2);
	for (int i=0; i<4; i++)
		v[i].color = value;

	GetGround()->SetTexture(GetTexture(0),GetiFrame(0));
	GetGround()->DrawQuadrangle(v);

	return S_OK;
}

HRESULT cPlaneVer::DrawNum(cPoint p1,cPoint p2)
{
	if (GetVersortType() == VERSORT_SEAM)
	{
		CString s;
		s.Format("%d",GetGround()->GetPlaneDrawNum());
		cPoint pt = p1;
		pt += p2;
		pt.x /= 2;
		pt.y /= 2;
		pt.z /= 2;
		GetCamera()->GroundToScreen(pt);
		GetGraphics()->GetDebugFont()->TextOut(s,s.GetLength(),pt.x, pt.y,0xff00ffff);
	}
	return S_OK;
}

HRESULT cPlaneVer::DrawTrace()
{
	cPlane::DrawTrace();
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	if (GetMapedit() & Mapedit_Trace)
	{
		char sMsg[128];
		sprintf(sMsg,"(F6)Plane 角度(%.1f) Height(%.1f) Versort(%d) SortOffY(%1.f),k(%.2f),b(%.2f)",
			m_param.fAngleXY,GetHeight(),GetVersortType(),GetSortOffY(),m_fK,m_fB);
		GetGraphics()->Trace(sMsg);
	}
	return S_OK;
}

HRESULT cPlaneVer::DrawSelect()
{
	if (!IsShow())
		return S_OK;
	if (!IsDisplay())
		return S_OK;
	if (GetMapedit() & Mapedit_Select)
	{
		stVertexGround v[4];
		GetSelectVG(v,0);
//		GetQuadrangle(v,0);
		GetGround()->DrawRect(v,cColor(0,0,255));
		if (GetVersortType() == VERSORT_SEAM || GetVersortType() == VERSORT_TREE )
		{
			if (v[2].sz != 0)
			{
				v[2].sz = 0;
				v[3].sz = 0;
				v[2].color = cColor(0,255,0).Value();
				v[3].color = cColor(0,255,0).Value();
				GetGround()->DrawLine(&v[2],2);
			}
		}
		else if (GetVersortType() == VERSORT_POINT)
		{
			cPoint pt;
			GetSortPoint(pt);
			GetGround()->DrawPoint(pt,cColor(0,255,0));
			DrawOffset();
//			pt = GetOffset();
//			pt.z = 0;
//			GetGround()->DrawPoint(pt,cColor(255,0,0));
		}
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////
//light
////////////////////////////////////////////////////////////////
/*
BOOL cPlaneVer::GetObstacle(cLight* pLight,cPoint2& pt)
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	switch (GetLightedType())
	{
	case LIGHTED_BOTTOMLINE:
		{
			GetLightingSeam(pt.p1,pt.p2);
			return TRUE;
		}
	//重心在中点? 
	case LIGHTED_CENTERCIRCLE:
		{
//			ASSERT(m_param.fLightedRadius != 0);
			if (m_param.fLightedRadius <= 0)
				m_param.fLightedRadius = 0.2f;
			//根据光源，垂点和半径来得到线段
			GetSeam(pLight->GetOffset(),GetOffset(),m_param.fLightedRadius,pt.p1,pt.p2);
			return TRUE;
		}
	}
	return FALSE;	
}
*/

/////////////////////////////////////////////////////////////////////////
//vertex info
/////////////////////////////////////////////////////////////////////////
HRESULT cPlaneVer::FillVertexInfoNotCallGraphics()
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	if (m_param.fAngleXY == 0)
		FillVertexInfoVerAngle0(m_info,0);
	else
		FillVertexInfoVerFull(m_info,0);

	//与后者不同的是，需要偏移
	//OffsetVB(GetOffset());

	return S_OK;
}

HRESULT cPlaneVer::FillVertexInfo()
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();

	stTexInfo info;
	HRESULT hr = GetTexInfo(info,0);
	if (FAILED(hr))
		return hr;

	if (m_param.fAngleXY == 0)
		FillVertexInfoVerAngle0(info,0);
	else
		FillVertexInfoVerFull(info,0);
	return S_OK;
}

HRESULT cPlaneVer::FillTextureUV()
{
	stTexInfo info;
	HRESULT hr = GetTexInfo(info,0);
	stVertexGround* v = GetVG(0);
	FillRectUV(v,info.rcTex);
	return S_OK;
}

BOOL cPlaneVer::FillVertexInfoVerAngle0(const stTexInfo& info,int iResource )
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	//it is not right when it is a ver and the angle != 0
	ASSERT(m_param.fAngleXY == 0);

	cRect rc = info.rcTex;

	ULONG width = info.width;
	ULONG height = info.height;

	stVertexGround* v = GetVG(iResource);
	FillRectUV(v,rc);

	v[0].sx = - 0 ;
	v[1].sx = (int)width -0;
	v[2].sx = v[0].sx;
	v[3].sx = v[1].sx;
	v[0].sz = 0;
	v[1].sz = v[0].sz;
	v[2].sz = 0 - (int)height;	
	v[3].sz = v[2].sz;

	for (int i=0; i<4; i++)
		v[i].sy = 0;

	int x = info.x;
	int y = info.y;
	for (i=0; i<4; i++)
	{
		v[i].sx -= x;
		v[i].sz += y;
		v[i].sx /= c_ptScale.x;
		v[i].sy /= c_ptScale.y;
		v[i].sz /= c_ptScale.z;
		v[i].color = 0;
	}

	return TRUE;
}

BOOL cPlaneVer::FillVertexInfoVerFull(const stTexInfo& info,int iResource )
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();

//	ASSERT(m_param.fAngleXY != 90);
	if (m_param.fAngleXY == 90)
	{
		CString strMsg;
		strMsg.Format("file: %s index %d 的在z=0的角度为90,不可能，请检查这个文件",m_param.szTemplate,iResource);
		AfxMessageBox(strMsg);
		m_param.fAngleXY = 0;
	}

	cRect rc = info.rcTex;
	float width = info.width;
	float height = info.height;

//	float fHeightExtra = m_param.fLeftExtra;
	float fHeightExtra = 0;
	height += fHeightExtra;
	rc.t -=  fHeightExtra * rc.Height() / (float)info.height;

	stVertexGround* v = GetVG(iResource);

	float fAngleXY = m_param.fAngleXY * c_fPI / 180.f; //	b渲染时底边与X轴角度
	float fAngleRenderZ = c_fPI / 6;						 //	a渲染时平面与Z轴角度

	float sina = sin(fAngleRenderZ);
	float tgb = tan(fAngleXY);
	float tgr = fabs(sina * tgb);					// for example: a = 30, b = 45, then tgr = 0.5;

	float fWidth;
	float fHeight;

	fWidth = width / cos(fAngleXY) / c_ptScale.x;
	fHeight = height / cos(fAngleRenderZ) / c_ptScale.x;
	float fZ = width * tgr / cos(fAngleRenderZ) / c_ptScale.x;

	FillRectUV(v,rc);

	v[2].sx = 0;
	v[2].sy = 0;
	v[3].sx = 0 + fWidth * cos(fAngleXY);
	v[3].sy = 0 - fWidth * sin(fAngleXY);
	if (m_param.fAngleXY > 90)
	{
		v[2].sz = -fZ;
		v[3].sz = 0;
	}
	else
	{
		v[2].sz = 0;
		v[3].sz = -fZ;
	}

	v[0].sx = v[2].sx;
	v[0].sy = v[2].sy;
	v[0].sz = v[2].sz + fHeight ;
	v[1].sx = v[3].sx;
	v[1].sy = v[3].sy ;
	v[1].sz = v[3].sz + fHeight ;

	return TRUE;
}

BOOL cPlaneVer::FillVertexInfoVer(const stTexInfo& info,int iResource )
{
	stPlaneVerParam& m_param = *GetPlaneVerParam();

	ASSERT(m_param.fAngleXY != 90);

	cRect rc = info.rcTex;
	float width = info.width;
	float height = info.height;

//	float fHeightExtra = m_param.fLeftExtra;
	float fHeightExtra = 0;
	height += fHeightExtra;
	rc.t -=  fHeightExtra * rc.Height() / (float)info.height;

	stVertexGround* v = GetVG(iResource);

	float fAngleXY = m_param.fAngleXY * c_fPI / 180.f;		//	b渲染时底边与X轴角度
	float fAngleRenderZ = c_fPI / 6;						//	a渲染时平面与Z轴角度

	float sina = sin(fAngleRenderZ);
	float tgb = tan(fAngleXY);
	float tgr = fabs(sina * tgb);							// for example: a = 30, b = 45, then tgr = 0.5;

	float fWidth;
	float fHeight;

	fWidth = width / cos(fAngleXY) / c_ptScale.x;
//	fHeight = (height-width*tgr)  / cos(fAngleRenderZ) / c_ptScale.z;
	fHeight = (height-width*tgr)  / cos(fAngleRenderZ) / c_ptScale.x;

	//fHeight ... rc.Height() - delta

	float delta = width * tgr / (float)height * rc.Height();	
	float dh = rc.Height() - delta;
	
	v[2].sx = 0;
	v[2].sy = 0;
	v[2].sz = 0;
	v[3].sx = 0 + fWidth * cos(fAngleXY);
	v[3].sy = 0 - fWidth * sin(fAngleXY);
	v[3].sz = 0;

	v[0].tu = rc.l;
	v[2].tu = rc.l;
	v[1].tu = rc.r;
	v[3].tu = rc.r;
	
	if (m_param.fAngleXY < 90 )
	{
		v[2].tv = rc.b;
		v[3].tv = rc.b - delta;
	}
	else if (m_param.fAngleXY > 90)
	{
		v[2].tv = rc.b - delta;
		v[3].tv = rc.b;			
	}
	v[0].tv = v[2].tv - dh;;
	v[1].tv = v[3].tv - dh;

	v[0].sx = v[2].sx;
	v[0].sy = v[2].sy;
	v[0].sz = v[2].sz + fHeight ;
	v[1].sx = v[3].sx;
	v[1].sy = v[3].sy ;
	v[1].sz = v[3].sz + fHeight ;

	//offset x,y
	if (m_param.fAngleXY == 0)
	{
		int x = info.x;
		int y = info.y;
	}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//global function
/////////////////////////////////////////////////////////////////////////
/*
BOOL RemoveVerFromList(cPlaneVerList& plist, cPlaneVer* p)
{
	int size = plist.size();
	plist.remove(p);
	if (size == plist.size() + 1)
		return TRUE;
	return FALSE;
}

BOOL AddVerToList(cPlaneVerList& plist, cPlaneVer* p)
{
	//hor已经在场景中了
	if (find(plist.begin(),plist.end(),p) != plist.end())
		return FALSE;
	plist.push_back(p);	
	return TRUE;
}
*/

/////////////////////////////////////////////////////////////
//load
/////////////////////////////////////////////////////////////

HRESULT cPlaneVer::SetPlaneProperty(stTemplateOne* pTemplate)
{
	HRESULT hr = cPlane::SetPlaneProperty(pTemplate);
	if (FAILED(hr))
		return hr;

	stPlaneVerParam& param = *GetPlaneVerParam();

	param.eLighting = (LightingEnum)pTemplate->eLighting;
	param.eLighted = (LightedEnum)pTemplate->eLighted;
	param.fLightedRadius = pTemplate->fRadiusLighted;
	param.fAngleXY = pTemplate->fAngleXY;
	param.eVersort = (VersortEnum)pTemplate->eVersort;

	return S_OK;
}

HRESULT cPlaneVer::Load(const stSection* p)
{
	HRESULT hr = cPlane::Load(p);
	if (FAILED(hr))
		return hr;
	if (p != NULL)
	{
		stPlaneVerParam& m_param = *GetPlaneVerParam();
		GetSectionValue(p,"sortoffy",m_param.fSortOffY);
	}
	return 0;
}

HRESULT cPlaneVer::Save(stSection* p)
{
	cPlane::Save(p);
	ASSERT(p);
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	GetSectionValue(p,"sortoffy",m_param.fSortOffY);
	return 0;
}

//////////////////////////////////////////////////////////
//obstacle
//////////////////////////////////////////////////////////

void cPlaneVer::UpdateObstacle(cObstacle* p)
{
	stObstacleParam param;

	if (GetVersortType() == VERSORT_SEAM || GetVersortType() == VERSORT_TREE)
	{
		param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);

		cPoint p1,p2;
		GetBottomSeam(p1,p2);
		float r = 0.2f;
		param.nPoint = 5;

		if (p1.y == p2.y)
		{
			param.aPoint[0] = p1;
			param.aPoint[0].y -= r;
			param.aPoint[1] = p2;
			param.aPoint[1].y -= r;
			param.aPoint[2] = p2;
			param.aPoint[2].y += r;
			param.aPoint[3] = p1;
			param.aPoint[3].y += r;
			param.aPoint[4] = param.aPoint[0];
		}
		else
		{
			float k = -(p1.x - p2.x) / (p1.y - p2.y) ;
			float dx = r / sqrt(k*k + 1);

			param.aPoint[0] = p1;
			param.aPoint[0].x -= dx;
			param.aPoint[0].y -= dx*k;
			param.aPoint[1] = p2;
			param.aPoint[1].x -= dx;
			param.aPoint[1].y -= dx*k;
			param.aPoint[2] = p2;
			param.aPoint[2].x += dx;
			param.aPoint[2].y += dx*k;
			param.aPoint[3] = p1;
			param.aPoint[3].x += dx;
			param.aPoint[3].y += dx*k;
			param.aPoint[4] = param.aPoint[0];
		}
		p->setParam(param);
	}
	else if (GetVersortType() == VERSORT_POINT)
	{
		param.SetMask(MASK_OBSTACLE,stObstacleParam::MASK_OBSTACLE_POINTINFO);
		param.AddMask(stObstacleParam::MASK_OBSTACLE_RADIUS);

		cPoint p1;
		GetLightedPoint(p1);
		p1.z = 0;
		param.nPoint = 1;
		param.aPoint[0] = cPoint(0,0);
		param.fRadius = GetLightedRadius();
		if (param.fRadius == 0)
			param.fRadius = 0.5f;
		p->setParam(param);
	}
}

//////////////////////////////////////////////////////////
//global
//////////////////////////////////////////////////////////

bool sortfun( stLine* p1,stLine* p2)
{
   return p1->GetLengthSquare() > p2->GetLengthSquare();
}

void SortLineArray(LineArray& m_aLine)
{
	std::sort(m_aLine.begin(),m_aLine.end(),sortfun);
}

BOOL AddPlaneVerToLineArray(LineArray& m_aLine, cPlaneVer* p)
{
	float k,b;
	cPoint ptStart,ptEnd;
	p->GetSortSeam(ptStart,ptEnd);
	VERIFY(p->ComputeLineFunction(k,b));
	bool bInsert = false;
	const float c_fMinK = 0.01f;
	const float c_fMinB = 0.5f;
	for (itALine it = m_aLine.begin(); it!= m_aLine.end(); it++)
	{
		stLine& line = *(*it);
		if (fabs(line.k - k)<c_fMinK && fabs(line.b - b)<c_fMinB)
		{
			p->SetLineFunction(line.k,line.b);
			if (line.ptStart.x > ptStart.x)
				line.ptStart = ptStart;
			if (line.ptEnd.x < ptEnd.x)
				line.ptEnd = ptEnd;
			bool bAdd = false;
			//按照x的值排序
			for (itcPlaneVer it = line.listVer.begin(); it != line.listVer.end(); ++it)
			{
				cPlaneVer* pVer = (*it);
				ASSERT(pVer != p);
				cPoint p1,p2;
				pVer->GetSortSeam(p1,p2);
				if(p1.x > ptStart.x)
					{
					bAdd = true;
					line.listVer.insert(it,p);
					break;
				}
			}
			if (bAdd == false)
				line.listVer.push_back(p);
			bInsert = true;
			break;
		}
	}
	if (!bInsert)
	{
		stLine& line = *(new stLine);
		line.k = k;
		line.b = b;
		line.ptStart = ptStart;
		line.ptEnd = ptEnd;			
		p->SetLineFunction(k,b);
		line.listVer.push_back(p);
		m_aLine.push_back(&line);
	}
	return TRUE;
}


