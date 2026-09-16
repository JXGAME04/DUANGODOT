// cStatusRegion.cpp: implementation of the cStatusRegion class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "cStatusRegion.h"
#include "iGraphics.h"
#include "iGraphicsUtility.h"
#include "iItem.h"
#include "fileop.h"
#include "iCommand.h"
#include "objutility.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const CString c_strTime = "时间变化.txt";
const CString c_strMinBrightness = "午夜亮度.txt";
const CString c_strMaxBrightness = "正午亮度.txt";
const CString c_strRegionCapture = "场景文件名称.txt";

cStatusRegion::cStatusRegion()
{
	m_pRegion = NULL;
}

cStatusRegion::~cStatusRegion()
{
	Clear();
}

void cStatusRegion::Clear()
{
	if (m_pRegion)
	{
		SelectRegion(m_pRegion,false);
		RELEASE(m_pRegion);
	}
}

//////////////////////////////////////////////////////////////////////
//create
BOOL cStatusRegion::Create()
{
	cStatus::Create();

	CString sFolder = "游戏资源\\区域\\setting";
	AppGetFullPath(sFolder);

	//load attribute and settings!
	CString sPathName;
	stSetting set;
	eMaskSetting e;

//	SaveCurrentRegionColor();

	if (AppFindFile(c_strTime,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_nTimeRunning = set.n ;	
	}
	if (AppFindFile(c_strMinBrightness,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_nMinBrightness = set.n ;	
	}
	if (AppFindFile(c_strMaxBrightness,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_INT);
		m_nMaxBrightness = set.n ;	
	}
	if (AppFindFile(c_strRegionCapture,sFolder,sPathName))
	{
		e = GetFileSetting(sPathName,set);
		ASSERT(e == MASK_STRING);
		m_strRegionCapture = set.str.c_str() ;	
	}

//	m_nCurrentBrightness = GetGroundBrightness();

	return TRUE;
}

BOOL cStatusRegion::OnSetFocus()
{
	GU_SetSelectSize(GetSelect(),8.f,16.f);
	GU_ChangeSelectColor(GetSelect(),cColor(0,0,255));
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//command
BOOL cStatusRegion::GetRegionColor(cColor& cr)
{
/*
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return FALSE;
	iRegion::stParam pm;
	pm.mask = 0;
	pRegion->GetParam(pm);
	cr = pm.crColor;
	return TRUE;
*/
	cr = GetGroundColor();
	return TRUE;
}

iRegion* cStatusRegion::GetCurrentRegion()
{
	return m_pRegion;
}

iRegion* cStatusRegion::GetNowRegion()
{
	CPoint ptMouse;
	BOOL b = GetMousePos(ptMouse);
	if (!b)
		return NULL;
	cPoint ptGround;
	GetCamera()->ScreenToGround(ptMouse,ptGround);
	iRegion* pRegion = GetGround()->GetiRegion(ptGround);
	return pRegion;
}

void cStatusRegion::AddRegionAniSpeed(bool bAdd)
{
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.mask = 0;
	pRegion->GetParam(param);
	if (bAdd)
		param.nSpeed --;
	else
		param.nSpeed ++;
	param.mask = iRegion::MASK_ANIMATIONSPEED;
	pRegion->SetParam(param);
}

void cStatusRegion::AddRegionAniDegree(bool bAdd)
{
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.mask = 0;
	pRegion->GetParam(param);
	if (bAdd)
		param.fDegree -= 1.f;
	else
		param.fDegree += 1.f;
	param.mask = iRegion::MASK_ANIMATIONDEGREE;
	pRegion->SetParam(param);
}

void cStatusRegion::ToggleRegionBlend()
{
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.mask = 0;
	pRegion->GetParam(param);
	if (param.eAnimation == iRegion::ANI_BLEND)
		param.eAnimation = iRegion::ANI_NULL;
	else
		param.eAnimation = iRegion::ANI_BLEND;
	param.mask = iRegion::MASK_ANIMATION;
	pRegion->SetParam(param);
}

void cStatusRegion::ToggleRegionTransparent()
{
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.mask = 0;
	pRegion->GetParam(param);
	param.bTransparent = !param.bTransparent;
	param.mask = iRegion::MASK_TRANSPARENT;
	pRegion->SetParam(param);
}

void cStatusRegion::DeleteRegion()
{
	iRegion* pRegion = GetCurrentRegion();
	iRegion::stParam param;
	param.mask = iRegion::MASK_COMMAND;
	param.eCommand = iRegion::RCOM_DELETE;
	pRegion->SetParam(param);
}

void cStatusRegion::ToggleRegionDraw()
{
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.mask = 0;
	pRegion->GetParam(param);
	param.bDraw = !param.bDraw;
	param.mask = iRegion::MASK_DRAW;
	pRegion->SetParam(param);
}

void cStatusRegion::SetRegionColor(cColor cr)
{
	SetGroundColor(cr);
/*
	iRegion* pRegion = GetCurrentRegion();
	if (pRegion == NULL)
		return;
	iRegion::stParam param;
	param.crColor = cr;
	param.mask = iRegion::MASK_COLOR;
	pRegion->SetParam(param);
*/
}

void cStatusRegion::AddRegionColor(cColor cr)
{
	cColor crRegion;
	BOOL b = GetRegionColor(crRegion);
	if (!b)
		return;
	crRegion += cr;
	crRegion.Format();
	SetRegionColor(crRegion);
}

//////////////////////////////////////////////////////////////////////
//input
void cStatusRegion::SelectRegion(iRegion* pRegion, bool bSelect)
{
	iRegion::stParam param;
	param.mask = iRegion::MASK_MAPEDIT;
	if (bSelect)
	{
		param.lMapeditEnum = Mapedit_Select;
		param.eOPMapedit = Bit_Add;
	}
	else
	{
		param.lMapeditEnum = Mapedit_Select;
		param.eOPMapedit = Bit_Remove;
	}
	pRegion->SetParam(param);
}

void cStatusRegion::MoveSelect(CPoint point)
{
	if (m_pRegion)
	{
		SelectRegion(m_pRegion,false);
		RELEASE(m_pRegion);
	}

	cPoint pt;
	GetGround()->ScreenToGround(point,pt);
	GroundToTile(pt);
	cPoint ptTile;
	//the square tile is 64*64 but the tile of ground is 32*32
	ptTile.z = pt.z;
	ptTile.x =floor(pt.x / 16.f) ;
	ptTile.y =floor(pt.y / 16.f) ;
	ptTile.x *= 16;
	ptTile.x += 8;
	ptTile.y *= 16;
	ptTile.y += 8;
	TileToGround(ptTile);

	MoveiSelect(SELECT_SQUARE,ptTile);

	m_pRegion = GetNowRegion();
	if (m_pRegion)
	{
		SelectRegion(m_pRegion,true);
	}
}

BOOL cStatusRegion::OnCameraMove(CPoint point)
{
	MoveSelect(point);
	return TRUE;
}

BOOL cStatusRegion::OnMouseMove(UINT nFlags, CPoint point)
{
	BOOL b = cStatus::OnMouseMove(nFlags,point);
	MoveSelect(point);
	return S_OK;
}

BOOL cStatusRegion::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!IsSelected())
		return FALSE;
	return S_OK;
}

BOOL cStatusRegion::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	cStatus::OnKeyDown(nChar, nRepCnt, nFlags);
//f (IsCtrlDown())
	{
		int off = 1;
		if (IsShiftDown())
			off = -1;
		switch (nChar)
		{
		case 'R':
			AddRegionColor(cColor(off,0,0,0));
			break;
		case 'G':
			AddRegionColor(cColor(0,off,0,0));
			break;
		case 'B':
			AddRegionColor(cColor(0,0,off,0));
			break;
		case 'A': 
			AddRegionColor(cColor(0,0,0,off));
			break;
/*
		case 'J':
			CaptureRegionColor();
			break;
*/
		case VK_DELETE:
			DeleteRegion();
			break;
		case 'D':
			ToggleRegionDraw();
			break;
		case 'T':
			ToggleRegionTransparent();
			break;
		case 'L':
			ToggleRegionBlend();
			break;
		case 'K':
			AddRegionAniSpeed(!IsShiftDown());
			break;
		case 'J':
			AddRegionAniDegree(!IsShiftDown());
			break;
		}
	}
	return S_OK;
}

BOOL cStatusRegion::OnTimer(UINT nIDEvent)
{
	cStatus::OnTimer(nIDEvent);
	static int s_nTimer = 0;
	static bool s_bAdd = true;

	if (m_nTimeRunning == 0)
		return TRUE;
	if (++s_nTimer > m_nTimeRunning)
	{
		s_nTimer = 0;		
		int n = GetGroundBrightness();
		if (s_bAdd)
		{
			n+=1; 
			if (n >= m_nMaxBrightness)
			{
				n = m_nMaxBrightness;
				s_bAdd = false;
			}
		}
		else
		{
			n-=1; 
			if (n <= m_nMinBrightness)
			{
				n = m_nMinBrightness;
				s_bAdd = true;
			}
		}
		SetGroundBrightness(n);
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//virtual call back
BOOL cStatusRegion::OnSelectedFile(LPCTSTR szFile,int nFrame)
{
	return FALSE;
}

void cStatusRegion::OnSelectedSetting(LPCTSTR szFile)
{
	CString sPath,sName;
	AppParsePathName(szFile,sPath,sName);
	if (sName == c_strTime)
		m_nTimeRunning = m_stSetting.n;
	else if (sName == c_strRegionCapture)
		m_strRegionCapture = m_stSetting.str.c_str();
	else if (sName == c_strMinBrightness)
		m_nMinBrightness = m_stSetting.n;
	else if (sName == c_strMaxBrightness)
		m_nMaxBrightness = m_stSetting.n;
}


