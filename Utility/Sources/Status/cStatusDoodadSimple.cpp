// cStatusDoodadSimple.cpp: implementation of the cStatusDoodadSimple class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cStatusDoodadSimple.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iImage.h"
#include "fileop.h"
#include "iItem.h"
#include "objutility.h"

#include "cclipboard.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strAutoLinkHor = "自动连接平行的面.txt";
const CString c_strAutoLinkVer = "自动连接垂直的面.txt";
const CString c_strAutoLinkTile = "自动对齐格子.txt";

cStatusDoodadSimple::cStatusDoodadSimple()
{
	m_pPlaneLeft = NULL;
	m_pPlaneRight = NULL;
	m_bTrace = false;

//	m_stSelect.eType = SelectType_Default;
//	m_stSelect.eFilter = SelectFilter_OneByDisplay;
//	m_stSelect.eFilterParam = SelectFilterParam_Default;
//	m_stSelect.eKind = KIND_NORMAL;
//	m_stSelect.eSort = SORT_VER;
}

void cStatusDoodadSimple::Clear()
{
	cStatus::Clear();
	SetPlaneLeft(NULL);
	SetPlaneRight(NULL);
}

cStatusDoodadSimple::~cStatusDoodadSimple()
{
	Clear();
}

void cStatusDoodadSimple::RemovePlaneRight()
{
	if (m_pPlaneRight)
	{
		SetObjSelect(m_pPlaneRight,false);
		SetObjTrace(m_pPlaneRight,false);
		RemoveObj(m_pPlaneRight);
		RELEASE(m_pPlaneRight);
	}
}

void cStatusDoodadSimple::RemovePlaneLeft()
{
	if (m_pPlaneLeft)
	{
		SetObjSelect(m_pPlaneLeft,false);
		SetObjTrace(m_pPlaneLeft,false);
		RemoveObj(m_pPlaneLeft);
		RELEASE(m_pPlaneLeft);
	}
}

void cStatusDoodadSimple::SetPlaneLeft(iObj* p)
{
	if (p == NULL)
	{
		SetObjSelect(m_pPlaneLeft,false);
		SetObjTrace(m_pPlaneLeft,false);
		RELEASE(m_pPlaneLeft);
	}
	else
	{
		m_pPlaneLeft= p;
		SetObjSelect(m_pPlaneLeft,true);
		SetObjTrace(m_pPlaneLeft,m_bTrace);
	}	
}

void cStatusDoodadSimple::SetPlaneRight(iObj* p)
{
	if (p == NULL)
	{
		SetObjSelect(m_pPlaneRight,false);
		SetObjTrace(m_pPlaneRight,false);
		RELEASE(m_pPlaneRight);
	}
	else
	{
		m_pPlaneRight = p;
		SetObjSelect(m_pPlaneRight,true);
		SetObjTrace(m_pPlaneRight,m_bTrace);
	}
}

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////

BOOL cStatusDoodadSimple::Create()
{
	cStatus::Create();

	CString sFolder = "游戏资源\\美术图素\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

	m_bLinkHor = false;
	m_bLinkVer = false;
	m_bLinkTile = false;
	if (AppFindFile(c_strAutoLinkHor,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_BOOL);
		m_bLinkHor = set.b;
	}
	if (AppFindFile(c_strAutoLinkVer,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_BOOL);
		m_bLinkVer= set.b;
	}
	if (AppFindFile(c_strAutoLinkTile,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_BOOL);
		m_bLinkTile = set.b;
	}

	return TRUE;
}

BOOL cStatusDoodadSimple::OnSetFocus()
{
	GU_SetSelectSize(GetSelect(),0.1f,0.2f);
	GU_ChangeSelectColor(GetSelect(), cColor(0,255,0));
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////
void cStatusDoodadSimple::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strAutoLinkHor)
		m_bLinkHor = m_stSetting.b ;
	if (sName == c_strAutoLinkVer)
		m_bLinkVer = m_stSetting.b ;
	if (sName == c_strAutoLinkTile)
		m_bLinkTile = m_stSetting.b ;
}

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////
BOOL cStatusDoodadSimple::MoveSelect(CPoint point)
{
	if (m_pPlaneLeft == NULL)
	{
		if (!IsSelected())
		{
			ClearSelect();
			return FALSE;
		}
		MoveiSelect(SELECT_SQUARE,point);
	}
	else if (m_pPlaneRight == NULL)
	{
		cPoint ptGround;
		m_pPlaneLeft->GetOffset(point,ptGround);
		
		stGroundObjParam param;
		param.ClearMask();
		m_pPlaneLeft->GetParam(param);
		cPoint p = param.ptOffset - ptGround;

		int eLink = LINK_NULL;
		if (m_bLinkVer)
			eLink |= LINK_VER;
		if (m_bLinkHor)
			eLink |= LINK_HOR;
		if (m_bLinkTile)
			eLink |= LINK_TILE;
		if (fabs(p.x) + fabs(p.y) < 2.f && eLink != LINK_NULL)
		{
			cPoint pt;
			BOOL b = GetGround()->GetLinkOffset(pt,m_pPlaneLeft,eLink);
			if (b)
				ptGround = pt;
		}
		
		GetOffsetOfPlane(m_pPlaneLeft,ptGround);
		OffsetObjTo(m_pPlaneLeft,ptGround);
	}
	return TRUE;
}

BOOL cStatusDoodadSimple::OnCameraMove(CPoint point)
{
	return MoveSelect(point);
}

BOOL cStatusDoodadSimple::OnMouseMove(UINT nFlags, CPoint point)
{
	cStatus::OnMouseMove(nFlags,point);

	MoveSelect(point);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
void cStatusDoodadSimple::TogglePlaneTrace()
{
	m_bTrace = !m_bTrace;
	SetObjTrace(m_pPlaneLeft,m_bTrace);
	SetObjTrace(m_pPlaneRight,m_bTrace);
}

/////////////////////////////////////////////////////////////////////
//Plane
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//virtual function
/////////////////////////////////////////////////////////////////////
void cStatusDoodadSimple::SelectPlane(CPoint point,iObj* & pPlane)
{
	ASSERT(pPlane == NULL);
	iObjList llist;
	m_stSelect.ePoint = SelectPoint_OneByScreen;
	m_stSelect.ptScreen.x = point.x;
	m_stSelect.ptScreen.y = point.y;
	GetGround()->GetiPlane(m_stSelect,llist);
	if (llist.size() == 0)
		return;
	pPlane = llist.front();
 	pPlane->AddRef();
	for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
}

void cStatusDoodadSimple::RefreshSpr()
{
	CString s =GetSelectFile();
	iItem* pItem = Common_CreateItem(s,iItem::FLAG_RELOAD);
	if (pItem)
	{
		wrap_ptr<iItem> ptr(pItem);
		CString strFile;
		if (GetItemValue(pItem,"file",strFile))
		{
			int pos = s.ReverseFind('\\');
			if (pos != -1)
			{
				strFile = s.Left(pos) + "\\" + strFile;
				AppGetShortPath(strFile);
				GetGraphics()->RefreshSpr(strFile);
			}
		}
	}
}

void cStatusDoodadSimple::CreatePlane(CPoint point,iObj* & pPlane, int nFrame)
{
	if (!IsSelected())
		return;

	if (IsShiftDown()) //更新文件
	{
		RefreshSpr();
	}

	::CreatePlane(GetGraphics(), pPlane, GetSelectFile(), nFrame);
	GU_OffsetObjTo(pPlane,point);

	AddObj(pPlane);

	cPoint ptGround;
	GetCamera()->ScreenToGround(point,ptGround);
	if (GetOffsetOfPlane(pPlane,ptGround))
		OffsetObjTo(pPlane,ptGround);

	ClearSelect();
}

/////////////////////////////////////////////////////////////////////
//property function
/////////////////////////////////////////////////////////////////////

void cStatusDoodadSimple::AddPlaneSortOffY(iObj* pPlane,float fOff)
{
	if (pPlane == NULL)
		return;
	stPlaneVerParam param;
	param.ClearMask();
	pPlane->GetParam(param);
	if (param.ePlane != PLANE_VERNORMAL)
		return;
	param.fSortOffY += fOff;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_SORTOFFY);
	pPlane->setParam(param);
	//修正Undo，当一个操作的undo可以轻易完成，不提供undo.
	//SetObjParam(pPlane,param); no need undo
}

void cStatusDoodadSimple::AddPlaneLayer(iObj* pPlane,int layer)
{
	if (pPlane == NULL)
		return;
	stPlaneParam param;
	param.ClearMask();
	pPlane->GetParam(param);
	param.nLayer += layer;
	param.SetMask(MASK_PLANE,stPlaneParam::MASK_PLANE_LAYER);
	pPlane->setParam(param);
//	SetObjParam(pPlane,param);	no need undo
}

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////
BOOL cStatusDoodadSimple::OnSelectedFile(LPCTSTR lpszPathName, int frame)
{
	m_strCopyDataFile.Empty();
	return cStatus::OnSelectedFile(lpszPathName,frame);
}

BOOL cStatusDoodadSimple::Copy()
{
	iObj* p;
	p = m_pPlaneRight;
	if (p != NULL)
	{
		stPlaneParam param;
		param.ClearMask();
		p->GetParam(param);
		m_strSelectFile = param.szTemplate;
		m_nFrame = param.stResA.stRes.iFrame;

		GetObjIniFile(m_pPlaneRight,m_strCopyDataFile);
		
		Clipboard_Copy(m_strSelectFile,m_pWnd->GetSafeHwnd());
		return TRUE;
	}
	return FALSE;
}

BOOL cStatusDoodadSimple::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);
	float off = 0.1f;
	if (IsShiftDown())
	{
		off *= 0.1f;
	}
	else if (IsCtrlDown())
	{
		off *= 10.f;
	}
	
	float angle = 1.f;
	float fHeight = 0.1f;
	int nLayer = 1;
	if (IsShiftDown())
	{
		angle *= -1;
		fHeight *= -1;
		nLayer *= -1;
	}

	iObj* pPlane = m_pPlaneLeft;
	if (pPlane == NULL)
		pPlane = m_pPlaneRight;

	switch (nChar)
	{
	case VK_ESCAPE:
	case VK_DELETE:
		RemovePlaneLeft();
		RemovePlaneRight();
		break;
	case VK_SPACE:
		Clear();
		break;
	case 'W':
		OffsetObj(m_pPlaneRight,cPoint(0,-off));
		break;
	case 'X':
	case 'S':
		OffsetObj(m_pPlaneRight,cPoint(0,off));
		break;
	case 'A':
		OffsetObj(m_pPlaneRight,cPoint(-off,0));
		break;
	case 'D':
		OffsetObj(m_pPlaneRight,cPoint(off,0));
		break;
	case 'Q':
		OffsetObj(m_pPlaneRight,cPoint(-off,-off));
		break;
	case 'E':
		OffsetObj(m_pPlaneRight,cPoint(off,-off));
		break;
	case 'Z':
		OffsetObj(m_pPlaneRight,cPoint(-off,off));
		break;
	case 'C':
		OffsetObj(m_pPlaneRight,cPoint(off,off));
		break;
	case 'G':
		OffsetObj(pPlane,cPoint(0,0,-off));
		break;
	case 'H':
		OffsetObj(pPlane,cPoint(0,0,off));
		break;
//	case 'O':
//		ObjProperty(m_pPlaneRight);
//		break;
	case 'I':
		OnSelectSetting();
		break;
	case 'L':
		AddPlaneLayer(pPlane,nLayer);
		break;
	case 'Y':
		AddPlaneSortOffY(pPlane,off);
		break;
	case 'U':
		AddPlaneSortOffY(pPlane,-off);
		break;
	case VK_F6:
		TogglePlaneTrace();
		break;
	}
	return TRUE;
}

BOOL cStatusDoodadSimple::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_pPlaneLeft != NULL)	
	{
		if (!IsSelected())
			return TRUE;

		stPlaneParam param;
		param.ClearMask();
		m_pPlaneLeft ->GetParam(param);
		
		int frame = param.stResA.stRes.iFrame ;
		if (GetSelectFileFrames() == 1)
			return FALSE;
		if (!IsShiftDown())
		{
			frame ++;
			if (frame >= GetSelectFileFrames())
				frame = 0;
		}
		else
		{
			frame --;
			if (frame < 0)
				frame = GetSelectFileFrames() - 1;
		}

		RemovePlaneLeft();

		iObj* pPlane;
		CreatePlane(point,pPlane,frame);
		SetPlaneLeft(pPlane);
		
		return TRUE;
	}
	if (m_pPlaneRight)
		SetPlaneRight(NULL);
	iObj* pSelect = NULL;
	SelectPlane(point,pSelect);
	if (pSelect != NULL)
		SetPlaneRight(pSelect);

	if (m_pPlaneRight)
		ClearSelect();
	
	iObj* pPlane = m_pPlaneLeft;
	if (pPlane == NULL)
		pPlane = m_pPlaneRight;	

	return TRUE;
}

BOOL cStatusDoodadSimple::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pPlaneLeft)
	{
		SetPlaneLeft(NULL);
		ASSERT(m_pPlaneRight == NULL);
		return TRUE;
	}
	if (m_pPlaneRight)
		SetPlaneRight(NULL);
	
	if (!IsSelected())
	{
		iObj* pPlane = NULL;
		SelectPlane(point,pPlane);
		if (pPlane != NULL)
		{
			if (IsCtrlDown())
				SetPlaneLeft(pPlane);
			else
				SetPlaneRight(pPlane);
		}
	}
	else
	{
		iObj* pPlane = NULL;
		CreatePlane(point,pPlane,GetFrame());
		SetPlaneLeft(pPlane);
	}

	return TRUE;
}

BOOL cStatusDoodadSimple::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pPlaneRight)
	{
		stPlaneParam param;
		param.ClearMask();
		m_pPlaneRight->GetParam(param);

		CString s = param.szTemplate;
		AppGetFullPath(s);
		ShellExecute(NULL,"open",(LPCSTR)s,NULL,NULL,SW_SHOW);
		
		return TRUE;
	}
	return FALSE;
}

#include "selectdlg.h"
void cStatusDoodadSimple::OnSelectSetting()
{
	CSelectDlg dlg;
	dlg.SetSelect(m_stSelect);
	if (dlg.DoModal() == IDOK)
		dlg.GetSelect(m_stSelect);
}

