// cStatusLightStatic.cpp: implementation of the cStatusLightStatic class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cStatusLightStatic.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "fileop.h"
#include "iitem.h"
#include "objutility.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strLight = "灯光颜色.txt";
const CString c_strRadius = "灯光半径.txt";
const CString c_strHeight = "灯光高度.txt";
const CString c_strLightCapture = "光源文件名称.txt";

const CString c_strfRangeVarMax		= "光源最大半径.txt";
const CString c_strfRangeVarSpeed	= "光源半径变化速度.txt";
const CString c_strfRangeVarFrame	= "光源半径变化帧数.txt";

const CString c_strfPosJumpFrame	= "光源位置变化帧数.txt";
const CString c_strptPosJumpSpeed	= "光源位置变化速度.txt";
const CString c_strfPosJumpMax		= "光源位置变化距离.txt";

const CString c_strfFlashFrame	= "光源闪烁帧数.txt";
const CString c_strnFlashDarkFrame	= "光源闪烁变暗帧数.txt";

cStatusLightStatic::cStatusLightStatic()
{
	m_pLightLeft = NULL;
	m_pLightRight = NULL;
	m_bTrace = false;
}

cStatusLightStatic::~cStatusLightStatic()
{
	Clear();
}

void cStatusLightStatic::Clear()
{
	cStatus::Clear();
	SetObjSelect(m_pLightLeft,false);
	SetObjTrace(m_pLightLeft,false);
	SetObjSelect(m_pLightRight,false);
	SetObjTrace(m_pLightLeft,false);
	RELEASE(m_pLightLeft);
	RELEASE(m_pLightRight);
}

/////////////////////////////////////////////////////////////////////
//create
/////////////////////////////////////////////////////////////////////

BOOL cStatusLightStatic::Create()
{
	cStatus::Create();

	CString sFolder = "游戏资源\\灯光\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

	if (AppFindFile(c_strLight,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_COLOR);
		m_param.crColor = set.cr ;	
	}
	if (AppFindFile(c_strHeight,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_fHeight = set.f ;	
	}
	if (AppFindFile(c_strLightCapture,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		m_strLightCapture = set.str.c_str() ;	
	}
	if (AppFindFile(c_strRadius,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fRange = set.f ;	
	}
	if (AppFindFile(c_strfRangeVarMax,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fRangeVarMax = set.f ;	
	}
	if (AppFindFile(c_strfRangeVarSpeed ,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fRangeVarSpeed = set.f ;	
	}
	if (AppFindFile(c_strfRangeVarFrame,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fRangeVarFrame = set.f ;	
	}
	if (AppFindFile(c_strfPosJumpMax ,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fPosJumpMax = set.f ;	
	}
	if (AppFindFile(c_strfPosJumpFrame,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fPosJumpFrame = set.f ;	
	}
	if (AppFindFile(c_strptPosJumpSpeed,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_CPOINT);
		m_param.ptPosJumpSpeed = set.point;	
	}
	if (AppFindFile(c_strnFlashDarkFrame,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_param.nFlashDarkFrame = set.f ;	
	}
	if (AppFindFile(c_strfFlashFrame,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_FLOAT);
		m_param.fFlashFrame = set.f ;	
	}

	return TRUE;
}


BOOL cStatusLightStatic::OnSetFocus()
{
	GU_SetSelectSize(GetSelect(),0.1f,0.2f);
	GU_ChangeSelectColor(GetSelect(),cColor(0,255,0));
	
	m_bShowLight = SetShow(iGraphics::GS_LIGHT);

	return TRUE;
}

BOOL cStatusLightStatic::OnLostFocus()
{
	if (m_bShowLight == FALSE)
		ClearShow(iGraphics::GS_LIGHT);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//on select
/////////////////////////////////////////////////////////////////////

void cStatusLightStatic::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strLight)
		m_param.crColor = m_stSetting.cr ;
	else if (sName == c_strHeight)
		m_fHeight = m_stSetting.f;
	else if (sName == c_strLightCapture)
		m_strLightCapture = m_stSetting.str.c_str();
	else if (sName == c_strRadius)
		m_param.fRange = m_stSetting.f;
	else if (sName == c_strfRangeVarMax)
		m_param.fRangeVarMax = m_stSetting.f;
	else if (sName == c_strfRangeVarSpeed)
		m_param.fRangeVarSpeed = m_stSetting.f;
	else if (sName == c_strfRangeVarFrame)
		m_param.fRangeVarFrame = m_stSetting.f;
	else if (sName == c_strfPosJumpFrame)
		m_param.fPosJumpFrame = m_stSetting.f;
	else if (sName == c_strptPosJumpSpeed)
		m_param.ptPosJumpSpeed = m_stSetting.point;
	else if (sName == c_strfPosJumpMax)
		m_param.fPosJumpMax = m_stSetting.f;
	else if (sName == c_strfFlashFrame)
		m_param.fFlashFrame = m_stSetting.f;
	else if (sName == c_strnFlashDarkFrame)
		m_param.nFlashDarkFrame = m_stSetting.n;
}

BOOL cStatusLightStatic::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	iItem* pItem = Common_CreateItem(szFile,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return FALSE;
	wrap_ptr<iItem> ptr(pItem);
	CString sValue;
	if (GetItemValue(pItem,"lighttype",sValue))
	{
		SetSelectFile(szFile,nFrame);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////
//select
/////////////////////////////////////////////////////////////////////

BOOL cStatusLightStatic::MoveSelect(CPoint point)
{
	if (m_pLightLeft == NULL)
	{
		if (!IsSelected())
		{
			ClearSelect();
			return FALSE;
		}

		MoveiSelect(SELECT_SQUARE,point);
	}
	else if (m_pLightRight == NULL)
	{
		cPoint ptGround;
		m_pLightLeft->GetOffset(point,ptGround);
		OffsetObjTo(m_pLightLeft,ptGround);
	}
	return TRUE;
}

BOOL cStatusLightStatic::OnCameraMove(CPoint point)
{
	return MoveSelect(point);
}

BOOL cStatusLightStatic::OnMouseMove(UINT nFlags, CPoint point)
{
	cStatus::OnMouseMove(nFlags,point);

	MoveSelect(point);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//trace
/////////////////////////////////////////////////////////////////////
void cStatusLightStatic::ToggleLightTrace()
{
	m_bTrace = !m_bTrace;
	SetObjTrace(m_pLightLeft,m_bTrace);
	SetObjTrace(m_pLightRight,m_bTrace);
}

/////////////////////////////////////////////////////////////////////
//light
/////////////////////////////////////////////////////////////////////

void cStatusLightStatic::SelectLight(CPoint point, iObj*& pLight)
{
	ASSERT(pLight == NULL);
//	cPoint pt = ScreenToGround(point);
	iObjList llist;
	POINT ptScreen = {point.x,point.y};
	GetGround()->GetiLightByPoint(ptScreen,llist);
	ASSERT(llist.size() <= 1);
	if (llist.size() > 0)
	{
		pLight = llist.front();
		pLight->AddRef();
	}
	for_all(llist,mem_fun<ULONG,iObj>(&iObj::Release));
}

void cStatusLightStatic::CreateLight(CPoint point,iObj*& pLight)
{
	stLightParam param = m_param;
	iItem* pItem = Common_CreateItem(GetSelectFile(),iItem::FLAG_LOAD);
	wrap_ptr<iItem> ptr(pItem);
	GetItemValue(pItem,"lightcolor",param.crColor);
	GetItemValue(pItem,"lightradius",param.fRange);

	GetItemValue(pItem,"varrangemax",param.fRangeVarMax);
	GetItemValue(pItem,"varrangespeed",param.fRangeVarSpeed);
	GetItemValue(pItem,"varrangeframe",param.fRangeVarFrame);

	GetItemValue(pItem,"posjumpmax",param.fPosJumpMax);
	GetItemValue(pItem,"posjumpspeed",param.ptPosJumpSpeed);
	GetItemValue(pItem,"posjumpframe",param.fPosJumpFrame);

	GetItemValue(pItem,"frameflash",param.fFlashFrame);
	GetItemValue(pItem,"framedark",param.nFlashDarkFrame);
	
	float fHeight = m_fHeight;
	GetItemValue(pItem,"height",fHeight);

	::CreateLight(GetGraphics(),pLight,param);

	pLight->GetOffset(point,param.ptOffset);
	param.ptOffset.z = fHeight;
	GU_OffsetLightTo(pLight,param.ptOffset);

	AddObj(pLight);
	
	ClearSelect();
}

void cStatusLightStatic::AddLightRange(iObj* pLight,float fRange)
{
	if (pLight == NULL)
		return ;
	stLightParam param;
	param.ClearMask();
	pLight->GetParam(param);
	float fDis = param.fRange + fRange;
	if (fDis > 4.f && fDis < 16.f)
		SetLightRange(pLight,fDis);
}

void cStatusLightStatic::SetLightRange(iObj* pLight,float fRange)
{
	if (pLight == NULL)
		return ;
//需要redo和undo的
	GU_SetLightRange(pLight,fRange);
}

void cStatusLightStatic::AddLightColor(iObj* pLight,cColor cr)
{
	if (pLight == NULL)
		return;

	stLightParam param;
	param.ClearMask();
	pLight->GetParam(param);
	cColor c = param.crColor + cr;
	c.Format();
	if (c == param.crColor)
		return;
	SetLightColor(pLight,c);	
}

void cStatusLightStatic::SetLightColor(iObj* pLight,cColor cr)
{
	if (pLight == NULL)
		return ;
	GU_ChangeObjColor(pLight,cr);
}

void cStatusLightStatic::CaptureLight()
{
	if (m_pLightRight == NULL)
		return;
	stLightParam param;
	param.ClearMask();
	m_pLightRight->GetParam(param);

	CString sWork;
	sWork = "\\游戏资源\\灯光\\静态灯光\\";
	if (m_strLightCapture.IsEmpty())
		sWork += "LightQuick.txt";
	else
		sWork += m_strLightCapture+ ".txt";
	AppGetFullPath(sWork);
	AppBuildPath(sWork);

	CFileFind f;
	bool bNew = true;
	if (f.FindFile(sWork))
		bNew = false;

	iItem* pItem = Common_CreateItem(sWork,iItem::FLAG_CREATE);
	wrap_ptr<iItem> ptr(pItem);
	SetItemValue(pItem,"icon","light");
	SetItemValue(pItem,"type","light_static");
	SetItemValue(pItem,"lighttype","static");

	SetItemValue(pItem,"height",param.ptOffset.z);
	
	SetItemValue(pItem,"lightcolor",param.crColor);
	SetItemValue(pItem,"lightradius",param.fRange);

//	SetItemValue(pItem,"range",m_param.fRange);
//	SetItemValue(pItem,"mainlight",m_param.bMainlight);

	SetItemValue(pItem,"varrangemax",param.fRangeVarMax);
	SetItemValue(pItem,"varrangespeed",param.fRangeVarSpeed);
	SetItemValue(pItem,"varrangeframe",param.fRangeVarFrame);

	SetItemValue(pItem,"posjumpmax",param.fPosJumpMax);
	SetItemValue(pItem,"posjumpspeed",param.ptPosJumpSpeed);
	SetItemValue(pItem,"posjumpframe",param.fPosJumpFrame);

	SetItemValue(pItem,"frameflash",param.fFlashFrame);
	SetItemValue(pItem,"framedark",param.nFlashDarkFrame);

	pItem->Save();

	if (bNew)
		SetNewFile(sWork);
}

/////////////////////////////////////////////////////////////////////
//input
/////////////////////////////////////////////////////////////////////

BOOL cStatusLightStatic::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar,nRepCnt,nFlags);
	const float off = 0.1f;

	int nColor = 1;
	float fDis = 0.2f;
	float fHeight = 0.1f;
	if (IsShiftDown())
	{
		nColor *= -1;
		fDis *= -1;
		fHeight *= -1;
	}
	
	switch (nChar)
	{
	case VK_DELETE:
	case VK_ESCAPE:
		RemoveAndReleaseObj(m_pLightLeft);
		RemoveAndReleaseObj(m_pLightRight);
		break;
	case 'R':
		AddLightColor(m_pLightLeft,cColor(nColor,0,0,0));
		AddLightColor(m_pLightRight,cColor(nColor,0,0,0));
		break;
	case 'G':
		AddLightColor(m_pLightLeft,cColor(0,nColor,0,0));
		AddLightColor(m_pLightRight,cColor(0,nColor,0,0));
		break;
	case 'B':
		AddLightColor(m_pLightLeft,cColor(0,0,nColor,0));
		AddLightColor(m_pLightRight,cColor(0,0,nColor,0));
		break;
	case 'N': //+
		AddLightRange(m_pLightRight,fDis);
		AddLightRange(m_pLightLeft,fDis);
		break;
	case 'H':
		if (IsShiftDown())
			OffsetObj(m_pLightRight,cPoint(0,0,-off));
		else
			OffsetObj(m_pLightRight,cPoint(0,0,off));
		break;
/*	
	case 'H':
		{
			CPoint pt;
			GetMousePos(pt);
			AddObjHeight(m_pLightLeft,fHeight);
			OffsetObjTo(m_pLightLeft,pt);
			break;
		}
*/
	case 'W':
		OffsetObj(m_pLightRight,cPoint(0,-off));
		break;
	case 'S':
		OffsetObj(m_pLightRight,cPoint(0,off));
		break;
	case 'A':
		OffsetObj(m_pLightRight,cPoint(-off,0));
		break;
	case 'D':
		OffsetObj(m_pLightRight,cPoint(off,0));
		break;
	case 'Q':
		OffsetObj(m_pLightRight,cPoint(-off,-off));
		break;
	case 'E':
		OffsetObj(m_pLightRight,cPoint(off,-off));
		break;
	case 'Z':
		OffsetObj(m_pLightRight,cPoint(-off,off));
		break;
	case 'C':
		OffsetObj(m_pLightRight,cPoint(off,off));
		break;
	case VK_F5:
		if (IsCtrlDown())
			ToggleLightTrace();
		break;
	case 'J':
		if (IsCtrlDown())
			CaptureLight();
		break;
	}
	return TRUE;
}

BOOL cStatusLightStatic::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pLightLeft)
	{
		SetObjSelect(m_pLightLeft,false);
		SetObjTrace(m_pLightLeft,false);
		RELEASE(m_pLightLeft);
		ASSERT(m_pLightRight == NULL);
		return TRUE;
	}

	if (m_pLightRight)
	{
		SetObjSelect(m_pLightRight,false);
		SetObjTrace(m_pLightRight,false);
		RELEASE(m_pLightRight);
	}

	if (!IsSelected())
		SelectLight(point,m_pLightLeft);
	else
		CreateLight(point,m_pLightLeft);
	SetObjSelect(m_pLightLeft,true);
	SetObjTrace(m_pLightLeft,m_bTrace);

	return TRUE;
}

BOOL cStatusLightStatic::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_pLightLeft != NULL)	
		return TRUE;

	if (m_pLightRight)
	{
		SetObjSelect(m_pLightRight,false);
		SetObjTrace(m_pLightRight,false);
		RELEASE(m_pLightRight);
	}
	SelectLight(point,m_pLightRight);
	SetObjSelect(m_pLightRight,true);
	SetObjTrace(m_pLightRight,m_bTrace);
	if (m_pLightRight)
		ClearSelect();
	return TRUE;
}
