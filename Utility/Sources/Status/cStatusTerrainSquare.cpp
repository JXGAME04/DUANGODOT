// cStatusTerrainSquare.cpp: implementation of the cStatusTerrainSquare class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "cStatusTerrainSquare.h"

#include "iAutomap.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iCommon.h"
#include "iItem.h"
#include "iCommand.h"
#include "fileop.h"
#include "objutility.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strAutoFillGround = "自动填充.txt";
const CString c_strFillGround = "填充矩形.txt";

cStatusTerrainSquare::cStatusTerrainSquare()
{
	for (int i=0; i<TYPE_NUM; i++)
		m_aGround[i] = NULL;
	m_bAutoFillGround = false;
	m_eType = TYPE_MIDDLE;
	
	m_bMouseStart = false;
}

void cStatusTerrainSquare::ChangeType(TypeEnum e)
{
	m_eType = e;
	OnSetFocus();
}

cStatusTerrainSquare::~cStatusTerrainSquare()
{
	for (int i=0; i<TYPE_NUM; i++)
		RELEASE(m_aGround[i]);
}

//////////////////////////////////////////////////////////////////////
//create
BOOL cStatusTerrainSquare::Create()
{
	cStatus::Create();

	CString sTerrain[TYPE_NUM];
	CString sConnection[TYPE_NUM];

	sTerrain[TYPE_MIDDLE] = "游戏资源\\室外地表\\中型地表图素";
	sConnection[TYPE_MIDDLE] ="游戏资源\\室外地表\\中型拼接图素";
	m_aSize[TYPE_MIDDLE] = 64;
	sTerrain[TYPE_LARGE] = "游戏资源\\室外地表\\大型地表图素";
	sConnection[TYPE_LARGE] ="游戏资源\\室外地表\\大型拼接图素";
	m_aSize[TYPE_LARGE] = 128;

	for (int i=0; i<TYPE_NUM; i++)
	{
		AppGetFullPath(sTerrain[i]);
		AppGetFullPath(sConnection[i]);
		m_aGround[i] = AutoMap_CreateAutoGround(m_aSize[i],sTerrain[i],sConnection[i],"spr");
		m_aTile[i] = m_aSize[i] / c_nWidthTile;
	}

	//set the connections of the automap.
	CString sFolder= "游戏资源\\室外地表\\setting";
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
	if (AppFindFile(c_strAutoFillGround,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_BOOL);
		m_bAutoFillGround = set.b;	
	}

	return TRUE;
}

BOOL cStatusTerrainSquare::OnSetFocus()
{
	GU_SetSelectSize(GetSelect(),GetTile() / 2.f, GetTile() / 2.f* 2.f);
	GU_ChangeSelectColor(GetSelect(),cColor(0,255,0));
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//input
void cStatusTerrainSquare::DeleteTile(CPoint point)
{
	ChangeTile("",point,false,true);
}

void cStatusTerrainSquare::ChangeSelectTile(CPoint point, BOOL bFill, BOOL bIgnore)
{
	CString s = GetSelectFile();
	if (s.IsEmpty())
		return;
	iItem* pItem = Common_CreateItem(s,iItem::FLAG_LOAD);
	ASSERT(pItem);
	wrap_ptr<iItem> ptr(pItem);

	CString sFile;
	VERIFY(GetItemValue(pItem,"file",sFile));
	CString sPath,sName;
	AppParsePathName(s,sPath,sName);
	CString strSpr = sPath +"\\" + sFile;
	AppGetShortPath(strSpr);

	ChangeTile(strSpr,point,bFill,bIgnore);
}

void cStatusTerrainSquare::ChangeTile(LPCSTR strSpr, CPoint point, BOOL bFill, BOOL bIgnore)
{
	if (point.x < 0 || point.y < 0)
		return ;

	GetAutoGround()->Load(GetDocumentName());

	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GroundToTile(pt);

	cPoint ptTile;
	ptTile.x =floor(pt.x / GetTile()) ;
	ptTile.y =floor(pt.y / GetTile()) ;
	pt.x = ptTile.x * GetTile();
	pt.y = ptTile.y * GetTile();

	if (bFill)// || m_bAutoFillGround && GetAutoGround()->IsAllNull(ptTile.x,ptTile.y))
	{
		int x = pt.x / c_tWidthRegion;
		int y = pt.y / c_tHeightRegion;
		FillTile(CRect(x,y,x+1,y+1));
		return;
	}
	//get the coordinate in the autoground's space
	stCommand_ChangeGround param;
	param.size = sizeof(param);
	param.type = COM_GROUND_CHANGETILE;
	param.bIgnore = bIgnore;

	strcpy(param.strFile,strSpr);

//	GetItemValue(pItem,"file",param.strFile);
	strcpy(param.strDocument,GetDocumentName());
	param.xAutoGround = ptTile.x;
	param.yAutoGround = ptTile.y;
	param.xGround = pt.x;
	param.yGround = pt.y;
	param.pAutoGround = GetAutoGround();

	DoCommand(&param);
}

void cStatusTerrainSquare::FillTile(CRect rect)
{
	CString strFile = GetSelectFile();
/*
	CString strPath,strName;
	AppParsePathName(strFile,strPath,strName);
	if (strName.Find("大") != -1)
	{
		AfxMessageBox("暂时不支持大图素填充");
		return;
	}
*/
	stCommand_FillGround param;
	param.size = sizeof(param);
	param.type = COM_GROUND_FILLRECT;
	
	CString s = GetSelectFile();
	iItem* pItem = Common_CreateItem(s,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	CString sFile;
	GetItemValue(pItem,"file",sFile);
	CString sPath,sName;
	AppParsePathName(s,sPath,sName);
	CString strSpr = sPath +"\\" + sFile;
	AppGetShortPath(strSpr);
	strcpy(param.strFile,strSpr);
	
	strcpy(param.strDocument,GetDocumentName());
	param.rect = rect;
	param.pAutoGround = GetAutoGround();	

	DoCommand(&param);
}

void cStatusTerrainSquare::FillDiamondTile(CRect rect)
{
	CPoint ptScreen;
	for (float x = rect.left ; x <= rect.right ; x++)
	for (float y = rect.top ; y <= rect.bottom; y++)
	{
		cPoint ptTile = cPoint(x,y);
		DiamondSmallToTile(ptTile,ptTile);
		TileToGround(ptTile,ptTile);
		ptTile.y += 1;
		GetCamera()->GroundToScreen(ptTile,ptScreen);
		ChangeSelectTile(ptScreen,false,true);
/*
		cPoint pt ;
		pt = ptTile;
		pt.x = ptTile.x - 1;
		GetCamera()->GroundToScreen(pt,ptScreen);
		ChangeSelectTile(ptScreen,false,true);
		pt = ptTile;
		pt.x = ptTile.x + 1;
		GetCamera()->GroundToScreen(pt,ptScreen);
		ChangeSelectTile(ptScreen,false,true);
		pt = ptTile;
		pt.y = ptTile.y - 1;
		GetCamera()->GroundToScreen(pt,ptScreen);
		ChangeSelectTile(ptScreen,false,true);
		pt = ptTile;
		pt.y = ptTile.y + 1;
		GetCamera()->GroundToScreen(pt,ptScreen);
		ChangeSelectTile(ptScreen,false,true);
*/
	}
}	


BOOL cStatusTerrainSquare::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (IsDocumentEmpty())
		return FALSE;
	if (!IsSelected())
		return FALSE;

	if (IsAltDown())
	{
		m_bMouseStart = true;
		GetSelectByPoint(point,m_ptMouseStart);
		return TRUE;
	}
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
//		SetUpdateRegionTexture(FALSE);
		FillDiamondTile(r);
//		SetUpdateRegionTexture(TRUE);
//		UpdateRegionTexture();
		EndMacroCommand();

		m_bMouseStart = false;
		ClearSelect();
		return TRUE;
	}

	CWaitCursor cursor;
	if (!m_rcFillGround.IsRectNull())
	{
		cPoint pt;
		GetGround()->ScreenToGround(point,pt);
		GroundToTile(pt);
		int x = pt.x / c_tWidthRegion;
		int y = pt.y / c_tHeightRegion;
		FillTile(CRect(max(x-m_rcFillGround.left,0),max(y-m_rcFillGround.top,0),x+m_rcFillGround.right+1,y+m_rcFillGround.bottom+1));
	}
	else
	{
		ChangeSelectTile(point,IsCtrlDown(),IsShiftDown());
	}

	return TRUE;
}
/*
BOOL cStatusTerrainSquare::OnLButtonUp(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatusTerrainSquare::OnLButtonDblClk(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatusTerrainSquare::OnRButtonDown(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatusTerrainSquare::OnRButtonUp(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatusTerrainSquare::OnRButtonDblClk(UINT nFlags, CPoint point){return FALSE;}
BOOL cStatusTerrainSquare::OnTimer(UINT nIDEvent){return FALSE;}
BOOL cStatusTerrainSquare::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message){return FALSE;}
*/

void cStatusTerrainSquare::ModifyTile(CPoint pt, cPoint v)
{
	iGround::stParam param;
	param.mask = iGround::MASK_COMMAND;

	cPoint point;
	GetGround()->ScreenToGround(pt,point);
	GroundToTile(point,point);
	if (point.x - (int)point.x > 0.5f)
		point.x ++;
	if (point.y - (int)point.y > 0.5f)
		point.y ++;
	param.xPos = (int)point.x;
	param.yPos = ((int)point.y)/2 * 2;

	param.eCommand = iGround::GCOM_GETTILE;
	GetGround()->GetParam(param);

	param.tile.v.sx += v.x;
	param.tile.v.sy += v.y;
	param.tile.v.sz += v.z;
	param.eCommand = iGround::GCOM_VERTEXUPDATE;
	GetGround()->SetParam(param);
}

BOOL cStatusTerrainSquare::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);

	CPoint pt;
	GetMousePos(pt);
	float f = 0.1f;
	switch (nChar)
	{
	case VK_DELETE:
		DeleteTile(pt);
		break;
	case 'W':
		ModifyTile(pt,cPoint(0,-f,0));
		break;
	case 'S':
	case 'X':
		ModifyTile(pt,cPoint(0,f,0));
		break;
	case 'A':
		ModifyTile(pt,cPoint(-f,0,0));
		break;
	case 'D':
		ModifyTile(pt,cPoint(f,0,0));
		break;
	case 'H':
		ModifyTile(pt,cPoint(0,0,f));
		break;
	case 'G':
		ModifyTile(pt,cPoint(0,0,-f));
		break;
	}
	return TRUE;	
}

void cStatusTerrainSquare::GetSelectByPoint(CPoint point, cPoint& ptTile)
{
	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GroundToTile(pt);

	//the square tile is 64*64 but the tile of ground is 32*32
	ptTile.x =floor(pt.x / GetTile()) ;
	ptTile.y =floor(pt.y / GetTile()) ;
	ptTile.x *= GetTile();
	ptTile.y *= GetTile();

	ptTile.x += GetTile()/2;
	ptTile.y += GetTile()/2;
	TileToGround(ptTile);
}

//select
void cStatusTerrainSquare::MoveSelect(CPoint point)
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
		MoveiSelect(SELECT_SQUARE,ptSelect);
	}
}

BOOL cStatusTerrainSquare::OnCameraMove(CPoint point)
{
	MoveSelect(point);
	return TRUE;
}

BOOL cStatusTerrainSquare::OnMouseMove(UINT nFlags, CPoint point)
{
	BOOL b = cStatus::OnMouseMove(nFlags,point);
	MoveSelect(point);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//select
//////////////////////////////////////////////////////////////////////

void cStatusTerrainSquare::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strFillGround)
		m_rcFillGround = m_stSetting.rect;
	else if (sName == c_strAutoFillGround)
		m_bAutoFillGround = m_stSetting.b;
}

BOOL cStatusTerrainSquare::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	CString strValue;
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	if (GetItemValue(pItem,"planedefaulttype",strValue))
	{
		if (strValue == "plane_squaremiddle")
			ChangeType(TYPE_MIDDLE);
		else if (strValue == "plane_squarelarge")
			ChangeType(TYPE_LARGE);
		else
			return FALSE;
		SetSelectFile(szFile,nFrame);
		return TRUE;
	}
	return FALSE;
}
