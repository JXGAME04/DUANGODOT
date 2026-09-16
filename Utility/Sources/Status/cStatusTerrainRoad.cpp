// cStatusTerrainRoad.cpp: implementation of the cStatusTerrainRoad class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusTerrainRoad.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iItem.h"
#include "objutility.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cStatusTerrainRoad::cStatusTerrainRoad()
{
	m_eType = TYPE_SMALL;
}

cStatusTerrainRoad::~cStatusTerrainRoad()
{

}

void cStatusTerrainRoad::ChangeType(TypeEnum e)
{
	m_eType = e;
	OnSetFocus();
}

BOOL cStatusTerrainRoad::Create()
{
	cStatus::Create();
	m_aSize[TYPE_SMALL] = 2;
	m_aSize[TYPE_MIDDLE] = 4;
	m_aSize[TYPE_LARGE] = 8;
	return TRUE;
}

BOOL cStatusTerrainRoad::OnSetFocus()
{
	stSelectParam param;
	GU_SetSelectSize(GetSelect(),GetSize(),GetSize());
	GU_ChangeSelectColor(GetSelect(),cColor(0,255,0));
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// select
//////////////////////////////////////////////////////////////////////
/*
void cStatusTerrainRoad::GetSelectDiamondByPoint(CPoint point, cPoint& ptTile)
{
	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GetGround()->GroundToTile(pt,pt);
	GetGround()->TileToDiamond(pt,pt);
	ptTile.x = floor(pt.x);
	ptTile.y = floor(pt.y);
}
*/

void cStatusTerrainRoad::GetSelectByPoint(CPoint point, cPoint& ptTile)
{
	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GroundToTile(pt,pt);
	TileToDiamond(pt,pt);
	ptTile.x = floor(pt.x/(GetSize()/2));
	ptTile.y = floor(pt.y/(GetSize()/2));
	ptTile.x *= GetSize()/2;
	ptTile.y *= GetSize()/2;
	DiamondToTile(ptTile,ptTile);
	TileToGround(ptTile,ptTile);
	ptTile.y += GetSize();
}

void cStatusTerrainRoad::MoveSelect(CPoint point)
{
	if (!IsSelected())
	{
		ClearSelect();
		return ;
	}
	cPoint ptSelect;
	GetSelectByPoint(point,ptSelect);
	MoveiSelect(SELECT_DIAMOND,ptSelect);
}

BOOL cStatusTerrainRoad::OnCameraMove(CPoint point)
{
	MoveSelect(point);
	return TRUE;
}

BOOL cStatusTerrainRoad::OnMouseMove(UINT nFlags, CPoint point)
{
	BOOL b = cStatus::OnMouseMove(nFlags,point);
	MoveSelect(point);
	return TRUE;
}

//input
BOOL cStatusTerrainRoad::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!IsSelected())
		return FALSE;

	CWaitCursor cursor;

	BeginMacroCommand();
	CreateDiamond(point);
	EndMacroCommand();

	return TRUE;
}

iObj* cStatusTerrainRoad::GetiPlane(CPoint point)
{
	iObjList ilist;
	stSelect st;
	st.ePoint = SelectPoint_OneByScreen;
	st.eType = SelectType_Kind;
	st.eKind = KIND_ROAD;
	st.eFilter = SelectFilter_OneByOffset;
	st.eFilterParam = SelectFilterParam_Default;
	st.ptScreen.x = point.x;
	st.ptScreen.y = point.y;
	GetGround()->GetiPlane(st,ilist);
	ASSERT(ilist.size() <= 1);
	if (ilist.size() == 0)
		return NULL;
	iObj* p = ilist.front();
	p->AddRef();
	for_all(ilist,mem_fun<ULONG,iObj>(&iObj::Release));
	return p;
}

enum {
	ROAD_0000 = 0,ROAD_0001,ROAD_0010,ROAD_0011,
	ROAD_0100,ROAD_0101,ROAD_0110,ROAD_0111,
	ROAD_1000,ROAD_1001,ROAD_1010,ROAD_1011,
	ROAD_1100,ROAD_1101,ROAD_1110,ROAD_1111,
};

int FrameToStyle(int frame)
{
	switch (frame)
	{
	case 0:
		return ROAD_0000;
	case 1:
		return ROAD_1010;
	case 2:
		return ROAD_1011;
	case 3:
		return ROAD_0101;
	case 4: 
		return ROAD_0111;
	case 5:
		return ROAD_0110;
	case 6:
		return ROAD_1110;
	case 7:
		return ROAD_1101;
	case 8:
		return ROAD_0011;
	case 9:
		return ROAD_1100;
	case 10:
		return ROAD_1001;
	}
	return 0;
}

int StyleToFrame(int style)
{
	switch (style)
	{
	case ROAD_0000:
	case ROAD_1111:
		return 0;
	case ROAD_1010:
	case ROAD_0010:
	case ROAD_1000:
		return 1;
	case ROAD_1011:
		return 2;
	case ROAD_0101:
	case ROAD_0001:
	case ROAD_0100:
		return 3;
	case ROAD_0111: 
		return 4;
	case ROAD_0110:
		return 5;
	case ROAD_1110:
		return 6;
	case ROAD_1101:
		return 7;
	case ROAD_0011:
		return 8;
	case ROAD_1100:
		return 9;
	case ROAD_1001:
		return 10;
	}
	return 0;
}

HRESULT cStatusTerrainRoad::CreateDiamond(CPoint point)
{
	//first to see if there is already a terrain on the ground
	//random select a file 
	cPoint ptTile;
	GetSelectByPoint(point, ptTile);
	GetCamera()->GroundToScreen(ptTile,point);

	iObj* pObj = GetiPlane(point);
	if (pObj != NULL)
	{
		//remove it! 
		pObj->Release();
		return S_OK;
	}

	//必须考虑周围的情况来设置road!
	int n = GetSelectFileFrames();
	if (n == 1)
	{
		CreateRoadObj(point,0);
		return 0;
	}
	ASSERT(n == 11);

	iObj* aObj[4];
	int aStyle[4];
	cPoint ptNext[4];
	CPoint aPoint[4];
	ptNext[0].x = ptTile.x - GetSize();
	ptNext[0].y = ptTile.y - GetSize();
	ptNext[1].x = ptTile.x - GetSize();
	ptNext[1].y = ptTile.y + GetSize();
	ptNext[2].x = ptTile.x + GetSize();
	ptNext[2].y = ptTile.y + GetSize();
	ptNext[3].x = ptTile.x + GetSize();
	ptNext[3].y = ptTile.y - GetSize();

	int aStyleAdd[4];
	aStyleAdd[0] = (1<<1); 
	aStyleAdd[1] = 1;
	aStyleAdd[2] = (1<<3);
	aStyleAdd[3] = (1<<2);

	for (int i=0; i<4; i++)
	{
		aStyle[i] = 0;
//		GetGround()->TileToGround(ptNext[i],ptNext[i]);
		GetCamera()->GroundToScreen(ptNext[i],aPoint[i]);
		aObj[i] = GetiPlane(aPoint[i]);
		if (aObj[i] != NULL)
			aStyle[i] = GetRoadStyle(ptNext[i],aStyleAdd[i]);
	}

	int iStyle = 0;
	for (i=0; i<4; i++)
	{
		if (aObj[i] != NULL)
		{
			iStyle |= (1 << (3-i));
		}
	}

	CreateRoad(point,iStyle);
	for (i=0; i<4; i++)
	{
		if (aObj[i] != NULL)
		{
			RemoveObj(aObj[i]);
			RELEASE(aObj[i]);
			CreateRoad(aPoint[i],aStyle[i]);
		}
	}

	return S_OK;
}

int cStatusTerrainRoad::GetRoadStyle(cPoint ptTile, int nStyleAdd)
{
	cPoint ptNext[4];
	CPoint aPoint[4];
	ptNext[0].x = ptTile.x - GetSize();
	ptNext[0].y = ptTile.y - GetSize();
	ptNext[1].x = ptTile.x - GetSize();
	ptNext[1].y = ptTile.y + GetSize();
	ptNext[2].x = ptTile.x + GetSize();
	ptNext[2].y = ptTile.y + GetSize();
	ptNext[3].x = ptTile.x + GetSize();
	ptNext[3].y = ptTile.y - GetSize();
	int iStyle = 0;
	for (int i=0; i<4; i++)
	{
		GetCamera()->GroundToScreen(ptNext[i],aPoint[i]);
		iObj* pObj = GetiPlane(aPoint[i]);
		if (pObj != NULL)
		{
			iStyle |= (1 << (3-i));
			RELEASE(pObj);
		}
	}
	iStyle |= nStyleAdd;
	return iStyle;
}

HRESULT cStatusTerrainRoad::CreateRoadObj(CPoint point, int index)
{
	cPoint ptTile;
	GetSelectByPoint(point, ptTile);

	iObj* pObj;
	CreatePlane(GetGraphics(),pObj,GetSelectFile(),index);
	GU_SetObjKind(pObj,KIND_ROAD);
	GU_OffsetObjTo(pObj,ptTile);	
	AddObj(pObj);
	pObj->Release();

	return S_OK;
}

HRESULT cStatusTerrainRoad::CreateRoad(CPoint point, int nStyle)
{
	int id = StyleToFrame(nStyle);
	return CreateRoadObj(point,id);
}

//call back
BOOL cStatusTerrainRoad::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	CString strValue;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (GetItemValue(pItem,"planedefaulttype",strValue))
	{
		if (strValue == "plane_diamondmiddle")
			ChangeType(TYPE_SMALL);
		else if (strValue == "plane_diamondlarge")
			ChangeType(TYPE_MIDDLE);
		else if (strValue == "plane_diamondlargest")
			ChangeType(TYPE_LARGE);
		else
			return FALSE;
		SetSelectFile(szFile,nFrame);
		return TRUE;
	}
	return FALSE;
}
