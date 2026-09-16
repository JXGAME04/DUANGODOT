// cMapRegion.cpp: implementation of the cMapRegion class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cMapRegion.h"

#include "stLine.h"

#include "cGraphics.h"
#include "GraphicsPublic.h"
#include "template.h"
#include "cObstacle.h"
#include "cMap.h"
#include "cScreen.h"
#include "iItem.h"

#include "cGround.h"
#include "cRegion.h"
#include "position.h"
#include "cplanever.h"
#include "cplanehor.h"

#include "planedoodad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const long c_lMapColor = 0x80ffffff;

cMapRegion::cMapRegion(cGraphics* p) : m_pGraphics(p)
{
	m_bUseRegion = true;
	m_ulCount = 1;
	m_pRegion = NULL;
}

cMapRegion::~cMapRegion()
{
	Clear();
}

void cMapRegion::Clear()
{
	delete m_pRegion;
	m_pRegion = NULL;
}

////////////////////////////////////////////////////////////////////
//com
////////////////////////////////////////////////////////////////////
ULONG cMapRegion::AddRef() 
{
	return ++m_ulCount;
}

ULONG cMapRegion::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

///////////////////////////////////////////////////////////////
//graphics
///////////////////////////////////////////////////////////////
cScreen* cMapRegion::GetScreen()
{
	return GetGraphics()->GetcScreen();
}

cMap* cMapRegion::GetMap()
{
	return GetScreen()->GetcMap();
}

cGround* cMapRegion::GetGround()
{
	return GetGraphics()->GetcGround();
}

cRegion* cMapRegion::GetRegion(int x,int y)
{
	return GetGround()->GetcRegionByRegion(x,y);
}

///////////////////////////////////////////////////////////////
//paint
///////////////////////////////////////////////////////////////
void cMapRegion::DrawSeam(cPoint p0, cPoint p1)
{
	const float width = 1.5f;
	stVertexGround v[4];		
	SetPoint(v[0],p0);
	SetPoint(v[1],p1);
	SetPoint(v[2],p0);
	SetPoint(v[3],p1);
	v[0].sy -= width;
	v[2].sy += width;
	v[1].sy -= width;
	v[3].sy += width;
	cRect rc(0,0,1,1);
	FillRectUV(v,rc);
	for (int i=0; i<4; i++)
		v[i].color = c_lMapColor;
	GetMap()->Draw(v,4);
}

void cMapRegion::DrawSeam(cPlaneVerList& plist)
{
	if (plist.empty())
		return;
	GetMap()->SetCommonTexture(cMap::TEX_HILL,0);
	for (itcPlaneVer it = plist.begin(); it != plist.end(); ++it)
	{
		cPlaneVer* pVer = (*it);
		if (pVer->GetKind() == KIND_HILL)
		{
			cPoint p1,p2;
			pVer->GetBottomSeam(p1,p2);
			DrawSeam(p1,p2);
		}
	}
}

void cMapRegion::DrawTree(cPlaneVerList& plist)
{
	if (plist.empty())
		return;
	LineArray al;
	for (itcPlaneVer it = plist.begin(); it != plist.end(); ++it)
	{
		AddPlaneVerToLineArray(al,(*it));
	}
	GetMap()->SetCommonTexture(cMap::TEX_WALL,0);
	for (itALine ita = al.begin(); ita != al.end(); ++ita)
	{
		stLine* pLine = (*ita);
		DrawSeam(pLine->ptStart,pLine->ptEnd);
		delete pLine;
	}
}

void cMapRegion::DrawGround(cPlaneHorList& plist)
{
	if (plist.empty())
		return;
	GetMap()->SetCommonTexture(cMap::TEX_ROAD,0);
	for (itcPlaneHor it = plist.begin(); it != plist.end(); ++it)
	{
		cPlaneHor* p = (*it);
		if (p->GetKind() != KIND_ROAD)
			continue;
		stVertexGround av[5];
		p->GetQuadrangle(av,0);
		stVertexGround vtemp = av[2];
		av[2] = av[3];
		av[3] = vtemp;
		av[4] = av[0];
		stVertexGround v[4];
		for (int i=0; i<4; i++)
		{
			v[i].sx = (av[i].sx + av[i+1].sx)/2;
			v[i].tu = (av[i].tu + av[i+1].tu)/2;
			v[i].sy = (av[i].sy + av[i+1].sy)/2;
			v[i].tv = (av[i].tv + av[i+1].tv)/2;
			v[i].sz = 0;
		}
		GetMap()->DrawFan(v,4);
	}	
}

void cMapRegion::DrawObstacle(cObstacle* p)
{
	stObstacleParam param;
	param.ClearMask();
	p->GetParam(param);
	cPoint ptOffset = param.ptOffset;
	cRect rcBound = c_rectMin;
	stVertexGround v[MAX_POINT+1];
	cColor cr = cColor(255,255,0,255);
	DWORD color = cr.Value();
	for (int i=0; i<param.nPoint; i++)
	{
		SetPoint(v[i],ptOffset+param.aPoint[i]);
		rcBound.l = _cpp_min(param.aPoint[i].x,rcBound.l);
		rcBound.r = _cpp_max(param.aPoint[i].x,rcBound.r);
		rcBound.t = _cpp_min(param.aPoint[i].y,rcBound.t);
		rcBound.b = _cpp_max(param.aPoint[i].y,rcBound.b);
	}
	for (i=0; i<param.nPoint; i++)
	{
		v[i].tu = 0;//(param.aPoint[i].x - rcBound.l) / rcBound.Width();
		v[i].tv = 0;//(param.aPoint[i].y - rcBound.t) / rcBound.Height();
		v[i].color = color;
	}
	if (param.nPoint > 1)
		GetMap()->DrawLine(v,param.nPoint);
	else
	{
		cPoint pt =	GetPoint(v[0]);
		GetMap()->DrawPoint(pt,cr);
	}
}

void cMapRegion::DrawObstacle(cObstacleList& olist)
{
	if (olist.empty())
		return;
	stObstacleParam param;
	for (itcObstacle it = olist.begin(); it != olist.end(); ++it)
		DrawObstacle(*it);
}

void cMapRegion::DrawPlaneObstacle(cPlaneList& glist)
{
	for (itcPlane it = glist.begin(); it != glist.end(); ++it)
	{
		cPlane* pObj = (*it);
		if (pObj->m_pObstacle)
			DrawObstacle(pObj->m_pObstacle);
		DrawObstacle(pObj->m_listObstacle);
	}
}

void cMapRegion::DrawDynamic(cPlaneList& glist)
{
	for (itcPlane it = glist.begin(); it != glist.end(); ++it)
	{
		cPlane* pObj = (*it);
		cPoint pt = pObj->GetOffset();
		DynamicEnum e = pObj->GetDynamicType();
		cColor cr;
		switch(e)
		{
		case DYNAMIC_NPC:
			cr = cColor(255,0,0);
			break;
		case DYNAMIC_CLIENTONLYNPC:
		case DYNAMIC_CLIENTONLYBOX:
		case DYNAMIC_CLIENTONLYTRAP:
			cr = cColor(255,125,0);
			break;
		case DYNAMIC_TRAP:
			cr = cColor(255,0,255);
			break;
		case DYNAMIC_BOX:
			cr = cColor(0,0,255);
			break;
		}
		GetMap()->DrawPoint(pt,cr);
	}
}

HRESULT cMapRegion::Paint()
{
//	return S_OK;
	//can not call cObstacle::Draw!
	cObstacleList* olist = NULL;
	cPlaneVerList* pverlist = NULL;
	cPlaneHorList* phorlist = NULL;
	cPlaneList* pnpclist = NULL;
	cRegion* pRegion = GetRegion(m_x,m_y);
	cPlaneVerList verlist;
	cPlaneHorList horlist;
	cPlaneList npclist;

	cRegion* pDraw = NULL;
	if (pRegion != NULL && pRegion->IsPaintPlane())
	{
		pDraw = pRegion;
		m_bUseRegion = true;
	}
	else
	{
		if (m_bUseRegion || m_pRegion == NULL)
		{
			Clear();
			m_pRegion = new cRegion(GetGraphics());
			m_pRegion->SetPos(m_x,m_y);
			m_pRegion->LoadForMap(m_strFile);
			m_bUseRegion = false;
		}
		pRegion = m_pRegion;
	}

	DrawPlaneObstacle(pRegion->m_listPlane);
	DrawObstacle(pRegion->m_listObstacle);
	GetScreen()->SetEnvironment();
	DrawDynamic(pRegion->m_listClientNpc);

	return S_OK;
}

HRESULT cMapRegion::Load(LPCSTR szFolder,long lDisplay)
{
	m_strFile.Format("%s\\%03d_%03d",szFolder,m_x,m_y);
	return S_OK;
}

