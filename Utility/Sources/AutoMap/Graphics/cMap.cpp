// cMap.cpp: implementation of the cMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cMap.h"

#include "cGraphics.h"
#include "cScreen.h"
#include "cGround.h"
#include "cMapRegion.h"

#include "cGroundObj.h"
#include "Position.h"

#include "coordinate.h"
#include "stringvalue.h"

#include "cSpecial.h"
#include "cCamera.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cMap::cMap(cGraphics* p) : cScreenObj(p)
{
	for (int x = 0; x<MAX_MAP_REGION_WIDTH; x++)
	for (int y = 0; y<MAX_MAP_REGION_HEIGHT; y++)
		m_aRegion[x][y] = NULL;
}

cMap::~cMap()
{
	ClearRegion();
}

void cMap::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stMapParam;
	cScreenObj::Init(false);
	stMapParam& m_param = *GetMapParam();
	int w,h;
	GetGraphics()->GetScreen(w,h);
	m_param.wScreen = w / 4;
	m_param.hScreen = h / 4;
	m_param.hScreen = m_param.wScreen;
	m_param.ptOffset.x = w - m_param.wScreen;
	m_param.ptOffset.y = 0;
	m_param.wRegion = 7;		//there are 7*7 = 49 region on map!
	m_param.hRegion = 7;
}

void cMap::ClearRegion()
{
	for (int x = 0; x<MAX_MAP_REGION_WIDTH; x++)
	for (int y = 0; y<MAX_MAP_REGION_HEIGHT; y++)
	{
		RELEASE(m_aRegion[x][y]);
	}
}

//////////////////////////////////////////////////////////////////////
//Graphics
//////////////////////////////////////////////////////////////////////

cGround* cMap::GetGround()
{
	return GetGraphics()->GetcGround();
}

//////////////////////////////////////////////////////////////////////
//坐标变换
//////////////////////////////////////////////////////////////////////
void cMap::ScreenToGround(CPoint ptScreen ,cPoint& ptGround)
{
	stMapParam& m_param = *GetMapParam();
	float offx = m_param.wScreen / 2 + m_param.ptOffset.x;
	float offy = m_param.hScreen / 2 + m_param.ptOffset.y;
	
	float w = m_param.wScreen / (m_param.wRegion);
	float h = m_param.hScreen / (m_param.hRegion);

	ptGround.x = (ptScreen.x - offx) / w;
	ptGround.y = (ptScreen.y - offy) / h;

	ptGround.x += m_param.xRegion + 0.5f;
	ptGround.y += m_param.yRegion + 0.5f;
	RegionToTile(ptGround);
	TileToGround(ptGround);
}

void cMap::GroundToScreen(const stVertexGround& vG, stVertexScreen& vS)
{
	long nx,ny;
	GroundToScreen(vG.sx,vG.sy,nx,ny);
	vS.sx = nx;
	vS.sy = ny;
}

void cMap::GroundToScreen(float fx,float fy,long& nx, long& ny)
{
	GroundToTile(fx,fy,fx,fy);
	TileToRegion(fx,fy,fx,fy);
	RegionToScreen(fx,fy,nx,ny);
}

void cMap::GroundToScreen(cPoint ptGround, CPoint& ptScreen)
{
	GroundToScreen(ptGround.x,ptGround.y,ptScreen.x,ptScreen.y);
}

void cMap::RegionToScreen(cPoint ptGround, CPoint& ptScreen)
{
	RegionToScreen(ptGround.x,ptGround.y,ptScreen.x,ptScreen.y);
}

//void cMap::RegionToScreen(cPoint ptGround, CPoint& ptScreen)
void cMap::RegionToScreen(float fx,float fy, long& nx, long& ny)
{
	stMapParam& m_param = *GetMapParam();
	float offx = m_param.wScreen / 2 + m_param.ptOffset.x;
	float offy = m_param.hScreen / 2 + m_param.ptOffset.y;
	
	float w = m_param.wScreen / (m_param.wRegion);
	float h = m_param.hScreen / (m_param.hRegion);

	fx -= m_param.xRegion + 0.5f;
	fy -= m_param.yRegion + 0.5f;

	nx = fx * w + offx;
	ny = fy * h + offy;
}

////////////////////////////////////////////////////////////
//param
////////////////////////////////////////////////////////////

HRESULT	cMap::OnCommand(long Command,stObjParam& pm)
{
	cScreenObj::OnCommand(Command,pm);

	stMapParam& param = *((stMapParam* )&pm);
	stMapParam& m_param = *GetMapParam();

	switch (Command)
	{
	case stMapParam::COM_MAP_GETCLICKPOS:
		{
			CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
				m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
			if (!rc.PtInRect(param.ptScreenClick))
				return -1;
			ScreenToGround(param.ptScreenClick,param.ptGroundClick);
		}
		break;
	}
	return S_OK;
}

HRESULT	cMap::setParam(stObjParam& pm)
{
	HRESULT hr = cScreenObj::setParam(pm);
	
	if (!pm.IsEMask(MASK_MAP))
		return hr;

	stMapParam& param = *((stMapParam* )&pm);
	stMapParam& m_param = *GetMapParam();

	if (param.IsSetMask(stMapParam::MASK_MAP_DISPLAY))
		BitOP(m_param.lDisplay,param.eDisplayOp,param.lDisplay);
	if (param.IsSetMask(stMapParam::MASK_MAP_REGIONSIZE))
	{
		if (m_param.wRegion >= 3 && m_param.hRegion >= 3)
		{
			m_param.wRegion = param.wRegion;
			m_param.hRegion = param.hRegion;
		}
	}
	return S_OK;
}

////////////////////////////////////////////////////////////
//paint
////////////////////////////////////////////////////////////
HRESULT cMap::OnFileOpen(LPCSTR szNewFile)
{
	m_strFile = szNewFile;
	ClearRegion();
	return S_OK;
}

HRESULT cMap::OnRegionUpdate(int xRegion, int yRegion)//当前的region变成xregion,yregion!
{
	stMapParam& m_param = *GetMapParam();
	CString strFolder = m_strFile.Left(m_strFile.GetLength() - 4);

//	if (xRegion == m_param.xRegion && yRegion == m_param.yRegion)
//		return S_OK;

	cMapRegion*	aRegion[MAX_MAP_REGION_WIDTH][MAX_MAP_REGION_HEIGHT];
	int x,y;
	for (x = 0; x < MAX_MAP_REGION_WIDTH; x++)
	for (y = 0; y < MAX_MAP_REGION_HEIGHT; y++)
	{
		aRegion[x][y] = m_aRegion[x][y];
		m_aRegion[x][y] = NULL;
	}

	int xoff = m_param.xRegion - xRegion;
	int yoff = m_param.yRegion - yRegion;
	for (x = 0; x < MAX_MAP_REGION_WIDTH; x++)
	for (y = 0; y < MAX_MAP_REGION_HEIGHT; y++)
	{
		if (x+xoff >=0 && y+yoff >= 0 && x+xoff < MAX_MAP_REGION_WIDTH && y+yoff < MAX_MAP_REGION_HEIGHT)
			m_aRegion[x+xoff][y+yoff] = aRegion[x][y];
		else
		{			
			RELEASE(aRegion[x][y]);
		}
	}

	for (x = 0; x < m_param.wRegion; x++)
	for (y = 0; y < m_param.hRegion; y++)
	{
		if (m_aRegion[x][y] == NULL)
		{
			m_aRegion[x][y] = new cMapRegion(GetGraphics());
			cMapRegion& region = *m_aRegion[x][y];
			region.SetPos(	xRegion + 0.5f - (float)m_param.wRegion/2.f + x, 
							yRegion + 0.5f - (float)m_param.hRegion/2.f + y);
			region.Load(strFolder,cMapRegion::LOAD_OBSTACLE);
		}
		int tx,ty;
		m_aRegion[x][y]->GetPos(tx,ty);
	}

	m_param.xRegion = xRegion;
	m_param.yRegion = yRegion;
	return S_OK;
}

////////////////////////////////////////////////////////////
//paint
////////////////////////////////////////////////////////////
BOOL cMap::IsShow()
{
	return GetGraphics()->IsShow(iGraphics::GS_MAP);
}

CRect cMap::GetRect()
{
	stMapParam& m_param = *GetMapParam();
	CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
		m_param.ptOffset.x + m_param.wScreen, m_param.ptOffset.y + m_param.hScreen);
	return rc;
}

BOOL cMap::IsPointInMap(CPoint pt)
{
	CRect rc = GetRect();
	return rc.PtInRect(pt);
}

BOOL cMap::IsPointInMap(cPoint pt)
{
	CPoint p;
	GroundToScreen(pt,p);
	return IsPointInMap(p);
}

HRESULT	cMap::DrawDynamic()
{
/*
	cGroundObjList& glist = GetGround()->m_listDynamicObj;
	for (itcGroundObj it = glist.begin(); it != glist.end(); ++it)
	{
		cGroundObj* pObj = (*it);
		cPoint pt = pObj->GetOffset();
		if (!IsPointInMap(pt))
			continue;
		DynamicEnum e = pObj->GetDynamicType();
		cColor cr;
		switch(e)
		{
		case DYNAMIC_NPC:
			cr = cColor(255,0,0);
			break;
		case DYNAMIC_CLIENTONLYNPC:
			cr = cColor(255,125,0);
			break;
		case DYNAMIC_TRAP:
			cr = cColor(255,0,255);
			break;
		case DYNAMIC_BOX:
			cr = cColor(255,255,0);
			break;
		}
		DrawPoint(pt,cr);
	}
*/
	return S_OK;
}

HRESULT	cMap::Draw()
{
	if (!IsShow())
		return S_OK;

	stMapParam& m_param = *GetMapParam();
	for (int x = 0; x < m_param.wRegion; x++)
	for (int y = 0; y < m_param.hRegion; y++)
	{
		if (m_aRegion[x][y])
			m_aRegion[x][y]->Paint();
	}

	//draw and drawline 不兼容，所以在最后统一使用drawline!
	
	DrawDynamic();

	cRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
		m_param.ptOffset.x + m_param.wScreen, m_param.ptOffset.y + m_param.hScreen);
	GetScreen()->DrawRect(rc,cColor(0,255,0));

	CPoint p1(0,0);
	CPoint p2(800,600);
	cPoint pp1,pp2;
	GetGround()->GetcCamera()->ScreenToGround(p1,pp1);
	GetGround()->GetcCamera()->ScreenToGround(p2,pp2);
	CPoint ptScreen1;
	CPoint ptScreen2;
	GroundToScreen(pp1,ptScreen1);
	GroundToScreen(pp2,ptScreen2);
	GetScreen()->DrawRect(cRect(ptScreen1.x,ptScreen1.y,ptScreen2.x,ptScreen2.y),cColor(0,255,0));

	//draw focus region!

	//路标
	DrawSpecial();

	return S_OK;
}

void cMap::DrawSpecial()
{
	SpecialList& slist = GetGround()->m_listSpecial;
	for (itSpecial it = slist.begin(); it!= slist.end(); ++it)
	{
		cSpecial* p = (cSpecial*)(*it);
		if (p ->GetElementType() == GT_SPECIAL_SIGN)
		{
			cPoint ptStart;
			cPoint ptEnd;
			cColor crColor;
			CString sValue;
			p->GetParam("sign的起始点",sValue);
			StringToValue(sValue,ptStart);
			p->GetParam("sign的终点",sValue);
			StringToValue(sValue,ptEnd);
			p->GetParam("color",sValue);
			StringToValue(sValue,crColor);
			crColor.a /= 2;

			DrawFormatLine(ptStart,ptEnd,crColor);
		}
	}
}

void cMap::DrawPoint(cPoint p1,cColor cr)
{
	CPoint pp1;
	GroundToScreen(p1,pp1);
	GetScreen()->DrawPoint(pp1,cr);	
}

void Format(CPoint& p,CRect rc)
{
	p.x = _cpp_max(p.x,rc.left);
	p.x = _cpp_min(p.x,rc.right);
	p.y = _cpp_max(p.x,rc.top);
	p.y = _cpp_min(p.x,rc.bottom);
}

void cMap::DrawFormatLine(cPoint p1,cPoint p2,cColor cr)
{
	CPoint pp1,pp2;
	GroundToScreen(p1,pp1);
	GroundToScreen(p2,pp2);
	if (pp1.x > pp2.x)
	{
		CPoint ptTemp;
		ptTemp = pp1;
		pp1 = pp2;
		pp2 = ptTemp;
	}
	CRect rc = GetRect();
	if (rc.PtInRect(pp1) && rc.PtInRect(pp2))
	{
	}
	else
	{
		int x1,x2,y1,y2;
		if (pp1.x == pp2.x)
		{
			if (pp1.x < rc.left || pp1.x > rc.right)
				return;

			x1 = pp1.x;
			x2 = pp1.x;
			y1 = _cpp_max(rc.top,_cpp_min(pp1.y,pp2.y));
			y2 = _cpp_min(rc.bottom,_cpp_max(pp1.y,pp2.y));
			
			if (y1 > y2)
				return;

			pp1 = CPoint(x1,y1);
			pp2 = CPoint(x2,y2);
		}
		else if (pp1.y == pp2.y)
		{
			if (pp1.y < rc.top || pp1.y > rc.bottom)
				return;

			y1 = pp1.y;
			y2 = pp1.y;
			x1 = _cpp_max(rc.left,_cpp_min(pp1.x,pp2.x));
			x2 = _cpp_min(rc.right,_cpp_max(pp1.x,pp2.x));
			
			if (y1 > y2)
				return;

			pp1 = CPoint(x1,y1);
			pp2 = CPoint(x2,y2);
		}
		else
		{
			CPoint pt[5];
			CPoint pc[4];
			pt[0] = CPoint(rc.left,rc.top);
			pt[1] = CPoint(rc.right,rc.top);
			pt[2] = CPoint(rc.right,rc.bottom);
			pt[3] = CPoint(rc.left,rc.bottom);
			pt[4] = CPoint(rc.left,rc.top);

			int bx1 = 0;
			for (int i=0; i<4; i++)
			{
				pc[i] = LineCrossLineAt(pt[i],pt[i+1],pp1,pp2);
				if (pc[i].x <= rc.right && pc[i].x >= rc.left 
					&& pc[i].y <= rc.bottom && pc[i].y >= rc.top)
//				if (rc.PtInRect(pc[i]))
				{
					if (!bx1)
					{
						x1 = pc[i].x;
						y1 = pc[i].y;
						bx1 = 1;
					}
					else
					{
						x2 = pc[i].x;
						y2 = pc[i].y;
						bx1 = 2;
					}
				}
			}
			if (bx1 != 2)
				return ;

			if (x1 > x2)
			{
				int x,y;
				x = x1;
				y = y1;
				x1 = x2;
				y1 = y2;
				x2 = x;
				y2 = y;
			}

			if (pp1.x < x1)
			{
				pp1 = CPoint(x1,y1);
			}
			if (pp2.x > x2)
			{
				pp2 = CPoint(x2,y2);
			}
			if (pp1.x > pp2.x)
				return;
		}
	}
	GetScreen()->DrawLine(pp1,pp2,cr);
}

void cMap::DrawLine(cPoint p1,cPoint p2,cColor cr)
{
	CPoint pp1,pp2;
	GroundToScreen(p1,pp1);
	GroundToScreen(p2,pp2);
	GetScreen()->DrawLine(pp1,pp2,cr);
}

void cMap::VertexGroundToScreen(stVertexGround* pV,stVertexScreen v[],int n)
{
	for (int i=0; i<n; i++)
	{
		GroundToScreen(pV[i],v[i]);
		v[i].sz = 0;
		v[i].rhw = 1.f;
		v[i].tu = pV[i].tu;
		v[i].tv = pV[i].tv;
		v[i].color = pV[i].color;
	}
}

void cMap::DrawLine(stVertexGround* pV, int n)
{
	stVertexScreen v[MAX_POINT];
	VertexGroundToScreen(pV,v,n);
	GetScreen()->DrawLine(v,n);
}

void cMap::Draw(stVertexGround* pV, int nVertex)
{
	stVertexScreen v[MAX_POINT];
	VertexGroundToScreen(pV,v,nVertex);
	GetScreen()->Draw(v,nVertex);
}


void cMap::DrawFan(stVertexGround* pV, int nVertex)
{
	stVertexScreen v[MAX_POINT];
	VertexGroundToScreen(pV,v,nVertex);
	GetScreen()->DrawFan(v,nVertex);
}

HRESULT cMap::SetCommonTexture(eTexKind kind, int frame)
{
	ASSERT(kind < TEX_NUM);
	iTexture* pTexture = FindCommonTexture(kind);

	ASSERT(pTexture);
	HRESULT hr = GetGraphics()->GetDevice3D()->SetTexture(0,pTexture->GetD3DTexture(frame));
	ASSERT(!FAILED(hr));
	pTexture->Release();
	return hr;
}


iTexture* cMap::FindCommonTexture(eTexKind kind)
{
	ASSERT(kind < TEX_NUM);

	static CString file[] = 
	{
		"system\\spr\\house.spr",
		"system\\spr\\wall.spr",
		"system\\spr\\hill.spr",
		"system\\spr\\forest.spr",
		"system\\spr\\road.spr",
		"",
	};

	stResourceA res;
	res.stDevice.eMedia = MEDIA_VIDEO;
	res.stDevice.eUsage = USAGE_MENU;
	res.stRes.idResource = kind + RESOURCE_MAP_START;
	res.stRes.iFrame = 0;
	strcpy(res.stRes.szResource,file[kind]);
	return GetGraphics()->FindTexture(res,true);
}
