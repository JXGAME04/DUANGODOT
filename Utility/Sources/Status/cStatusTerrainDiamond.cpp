// cStatusTerrainDiamond.cpp: implementation of the cStatusTerrainDiamond class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusTerrainDiamond.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "fileop.h"
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

cStatusTerrainDiamond::cStatusTerrainDiamond()
{
	m_bMouseStart = false;
	m_nRadius = 8;
}

cStatusTerrainDiamond::~cStatusTerrainDiamond()
{

}

const CString c_strFillGround = "填充矩形.txt";
const CString c_strFillRadius = "填充半径.txt";

BOOL cStatusTerrainDiamond::Create()
{
	cStatus::Create();

	CString sFolder= "游戏资源\\室内地表\\setting";
	AppGetFullPath(sFolder);

	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strFillGround,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_RECT);
		m_rcFillGround = set.rect;	
	}
	if (AppFindFile(c_strFillRadius,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_nRadius = set.n;	
	}

	return TRUE;
}

BOOL cStatusTerrainDiamond::OnSetFocus()
{
	stSelectParam param;
	GU_SetSelectSize(GetSelect(),1.f,1.f);
	GU_ChangeSelectColor(GetSelect(),cColor(0,255,0));
	return TRUE;
}

BOOL cStatusTerrainDiamond::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	cStatus::OnSelectedFile(szFile,nFrame);
	m_bMouseStart = false;
	return TRUE;
}

void cStatusTerrainDiamond::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strFillGround)
		m_rcFillGround = m_stSetting.rect;
	if (sName == c_strFillRadius)
		m_nRadius = m_stSetting.n;
}

//////////////////////////////////////////////////////////////////////
// select
//////////////////////////////////////////////////////////////////////
void cStatusTerrainDiamond::GetSelectByPoint(CPoint point, cPoint& ptTile)
{
	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GroundToTile(pt,pt);
	TileToDiamondSmall(pt,pt);
	ptTile.x = floor(pt.x);
	ptTile.y = floor(pt.y);
	DiamondSmallToTile(ptTile,ptTile);
	TileToGround(ptTile,ptTile);
	ptTile.y += 1;
}

void cStatusTerrainDiamond::MoveSelect(CPoint point)
{
	if (!IsSelected())
	{
		ClearSelect();
		return ;
	}
	if (m_bMouseStart)
	{
		cPoint ptStart = m_ptMouseStart;
		cPoint ptEnd;
		GetSelectByPoint(point,ptEnd);
		GU_SetSelectType(GetSelect(),SELECT_PARALLELOGRAM);
		GU_SetSelectOffsetEnd(GetSelect(),ptEnd);
		GU_OffsetObjTo(GetSelect(),ptStart);
	}
	else
	{
		cPoint ptSelect;
		GetSelectByPoint(point,ptSelect);
		MoveiSelect(SELECT_DIAMOND,ptSelect);
	}
}

BOOL cStatusTerrainDiamond::OnCameraMove(CPoint point)
{
	MoveSelect(point);
	return TRUE;
}

BOOL cStatusTerrainDiamond::OnMouseMove(UINT nFlags, CPoint point)
{
	BOOL b = cStatus::OnMouseMove(nFlags,point);
	MoveSelect(point);
	return TRUE;
}

//input
BOOL cStatusTerrainDiamond::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (IsDocumentEmpty())
		return FALSE;
	if (!IsSelected())
		return FALSE;

	if (IsCtrlDown())
	{
		m_bMouseStart = true;
		GetSelectByPoint(point,m_ptMouseStart);
		return TRUE;
	}

	CWaitCursor cursor;
	if (m_bMouseStart)
	{
		cPoint ptMouseEnd ;
		GetSelectByPoint(point,ptMouseEnd);
		//create all diamond in a rect !

		cPoint ptStart,ptEnd;
		GroundToTile(m_ptMouseStart,ptStart);
		TileToDiamondSmall(ptStart,ptStart);
		GroundToTile(ptMouseEnd,ptEnd);
		TileToDiamondSmall(ptEnd,ptEnd);

		CRect r;
		r.left = floor(min(ptStart.x,ptEnd.x));
		r.right = floor(max(ptStart.x,ptEnd.x));
		r.top = floor(min(ptStart.y,ptEnd.y));
		r.bottom = floor(max(ptStart.y,ptEnd.y));

		BeginMacroCommand();
		SetUpdateRegionTexture(FALSE);
		FillTile(r,!IsShiftDown());
		SetUpdateRegionTexture(TRUE);
		UpdateRegionTexture();
		EndMacroCommand();

		m_bMouseStart = false;
		ClearSelect();
		return TRUE;
	}
	else if(IsAltDown())
	{
		BeginMacroCommand();
		SetUpdateRegionTexture(FALSE);

		int size = m_nRadius;
		cPoint pt;
		GetGround()->ScreenToGround(point,pt);
		GroundToTile(pt,pt);
		TileToDiamondSmall(pt,pt);
		int x = pt.x;
		x = x / size * size;
		if (pt.x < 0)
			x -= size;
		int y = pt.y;
		y = y / size * size;
		if (pt.y < 0)
			y -= size;
		FillTile(CRect(x,y,x+size-1,y+size-1), TRUE);
		SetUpdateRegionTexture(TRUE);
		UpdateRegionTexture();
		EndMacroCommand();
	}
	else
	{
		BeginMacroCommand();
		CreateDiamond(point,!IsShiftDown());
		EndMacroCommand();
	}
	return TRUE;
}

HRESULT cStatusTerrainDiamond::FillTile(CRect rect, bool bCreate)
{
	for (float x = rect.left ; x <= rect.right ; x++)
	for (float y = rect.top ; y <= rect.bottom; y++)
	{
		cPoint ptTile = cPoint(x,y);
		DiamondSmallToTile(ptTile,ptTile);
		TileToGround(ptTile,ptTile);
		ptTile.y += 1;
		CreateDiamond(ptTile, bCreate);
	}
	return TRUE;
}

HRESULT cStatusTerrainDiamond::CreateDiamond(cPoint ptTile, bool bCreate)
{
	CPoint point;
	GetCamera()->GroundToScreen(ptTile,point);

	iObjList ilist;
	stSelect st;
	st.ePoint = SelectPoint_OneByScreen;
	st.eType = SelectType_Kind;
	st.eKind = KIND_DIAMOND;
	st.eFilter = SelectFilter_OneByOffset;
	st.eFilterParam = SelectFilterParam_Default;
	st.ptScreen.x = point.x;
	st.ptScreen.y = point.y;
	GetGround()->GetiPlane(st,ilist);

	ASSERT(ilist.size() <= 1);

	int n = GetSelectFileFrames();
	int id = rand() % n;

	iObj* pObj;
	if (ilist.size() == 0)
	{
	}
	else
	{
		iObj* pObj = ilist.front();
		pObj->AddRef();
		RemoveObj(pObj);
		RELEASE(pObj);
	}
	for_all(ilist,mem_fun<ULONG,iObj>(&iObj::Release));

	if (bCreate == FALSE)
		return S_OK;

	CreatePlane(GetGraphics(),pObj,GetSelectFile(),id);

	GU_SetObjKind(pObj,KIND_DIAMOND);
	GU_OffsetObjTo(pObj,ptTile);
	
	AddObj(pObj);

	pObj->Release();

	return S_OK;
}

HRESULT cStatusTerrainDiamond::CreateDiamond(CPoint point, bool bCreate)
{
	//first to see if there is already a terrain on the ground
	//random select a file 
	cPoint ptTile;
	GetSelectByPoint(point, ptTile);
	return CreateDiamond(ptTile,bCreate);
}

