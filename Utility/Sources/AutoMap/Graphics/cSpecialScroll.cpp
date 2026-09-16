// cSpecialScroll.cpp: implementation of the cSpecialScroll class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cSpecialScroll.h"
#include "stringvalue.h"
#include "cGraphics.h"
#include "cCamera.h"
#include "cGround.h"
#include "cScreen.h"
#include "Position.h"
#include "cPicture.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cSpecialScroll::cSpecialScroll(cGraphics* p) : cSpecial(p)
{
	m_param.eElement = GT_SPECIAL_SCROLL;
	m_param.ptSpeed = cPoint(0,0,0);
	m_param.bLoop = FALSE;
	m_param.vSpeedMove = cPoint(0,0,0);
	m_param.bBackGround = TRUE;

	for (int i=0; i<MAX_IMAGE; i++)
	{
		m_param.aImage[i][0] = 0;
		m_param.aImageFrame[i] = 0;
		m_aPicture[i] = NULL;
	}
	m_nFrame = 0;
}

cSpecialScroll::~cSpecialScroll()
{
	Clear();
}

void cSpecialScroll::Clear()
{
	for (int i=0; i<MAX_IMAGE; i++)
	{
		if (m_aPicture[i] != NULL)
			GetScreen()->RemoveiObj(m_aPicture[i]);
		RELEASE(m_aPicture[i]);
	}
}

//////////////////////////////////////////////////////////////////////
//load binary
//////////////////////////////////////////////////////////////////////
HRESULT	cSpecialScroll::LoadBinary(CFile& f)
{
	f.Read(&m_param,sizeof(m_param));
	return S_OK;
}

HRESULT	cSpecialScroll::SaveBinary(CFile& f)
{
	f.Write(&m_param,sizeof(m_param));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//param
//////////////////////////////////////////////////////////////////////
const CString c_aStrItem[] = 
{
	"elementtype",
	"image0","image1","image","image3",
	"image0frame","image1frame","image2frame","image3frame",
	"卷轴的起始点","卷轴的范围","卷轴的速度","是否循环卷轴","camera的起始点","自身运动的速度",
	"背景图还是前景图",
	"",
};

const CString* cSpecialScroll::GetItemStringArray()
{
	return c_aStrItem;
}

const CString c_aStrDescription[] = 
{
	"不能修改",
	"输入相对文件名称","输入相对文件名称","输入相对文件名称","输入相对文件名称",
	"int","int","int","int",
	"cPoint","以地图格子为单位","以像素点为单位(32*16移动跟地图一致)","不能修改","不能修改","用于云朵",
	"bool(1 背景)",
	"",
};

const CString* cSpecialScroll::GetDescriptionStringArray()
{
	return c_aStrDescription;
}

HRESULT cSpecialScroll::GetValue(int e, CString& strValue)
{
	int n = GT_SPECIAL_SCROLL;
	switch (e)
	{
	case PARAM_ELEMENTTYPE:
		ValueToString(n,strValue);
		break;
	case PARAM_IMAGE0:
	case PARAM_IMAGE1:
	case PARAM_IMAGE2:
	case PARAM_IMAGE3:
		{
			ValueToString(m_param.aImage[e - PARAM_IMAGE0],strValue);
			Clear();
		}
		break;
	case PARAM_IMAGE0FRAME:
	case PARAM_IMAGE1FRAME:
	case PARAM_IMAGE2FRAME:
	case PARAM_IMAGE3FRAME:
		ValueToString(m_param.aImageFrame[e - PARAM_IMAGE0FRAME],strValue);		
		break;
	case PARAM_START:
		ValueToString(m_param.ptStart,strValue);
		break;
	case PARAM_BACKGROUND:
		ValueToString(m_param.bBackGround,strValue);
		break;
	case PARAM_SIZE:
		ValueToString(m_param.ptSize,strValue);
		break;
	case PARAM_SPEED:
		ValueToString(m_param.ptSpeed,strValue);
		break;
	case PARAM_LOOP:
		ValueToString(m_param.bLoop,strValue);
		break;
	case PARAM_CAMERASTART:
		ValueToString(m_param.ptCameraInit,strValue);
		break;
	case PARAM_SPEEDMOVE:
		ValueToString(m_param.vSpeedMove,strValue);
		break;
	}	
	return S_OK;
}

HRESULT cSpecialScroll::SetParam(int e, LPCSTR szValue)
{
	CString strValue = szValue;
	switch (e)
	{
	case PARAM_IMAGE0:
	case PARAM_IMAGE1:
	case PARAM_IMAGE2:
	case PARAM_IMAGE3:
		StringToValue(szValue,m_param.aImage[e - PARAM_IMAGE0]);
		break;
	case PARAM_IMAGE0FRAME:
	case PARAM_IMAGE1FRAME:
	case PARAM_IMAGE2FRAME:
	case PARAM_IMAGE3FRAME:
		StringToValue(szValue,m_param.aImageFrame[e - PARAM_IMAGE0FRAME]);
		break;
	case PARAM_START:
		StringToValue(szValue,m_param.ptStart);
		break;
	case PARAM_BACKGROUND:
		StringToValue(szValue,m_param.bBackGround);
		break;
	case PARAM_SIZE:
		StringToValue(szValue,m_param.ptSize);
		break;
	case PARAM_SPEED:
		StringToValue(szValue,m_param.ptSpeed);
		break;
	case PARAM_LOOP:
		StringToValue(szValue,m_param.bLoop);
		break;
	case PARAM_CAMERASTART:
		StringToValue(szValue,m_param.ptCameraInit);
		break;		
	case PARAM_SPEEDMOVE:
		StringToValue(szValue,m_param.vSpeedMove);
		break;		
	}	
	return S_OK;
}


//////////////////////////////////////////////////////////////////////
//run
//////////////////////////////////////////////////////////////////////
cRect cSpecialScroll::GetScrollRect()
{
	cRect rect(m_param.ptStart.x,m_param.ptStart.y,
		m_param.ptStart.x+m_param.ptSize.x,m_param.ptStart.y+m_param.ptSize.y);
	return rect;
}

BOOL cSpecialScroll::Active()	
{
	//是否进入scroll的范围
	cRect rc = GetGround()->GetRectGround();
	cRect rect = GetScrollRect();
	cRect r = rect & rc;
	if (!r.IsEmpty())
	{
		m_bActive = TRUE;
	}
	else
	{
		for (int i=0; i<MAX_IMAGE; i++)
		{
			if (m_aPicture[i])
				m_aPicture[i]->SetHide(TRUE);
		}
		m_bActive = FALSE;
	}
	return m_bActive;
}

BOOL SetPictureOffset(cPicture* p, CPoint ptOffset)
{
	stScreenObjParam param;
	param.SetMask(MASK_SCREENOBJ,stScreenObjParam::MASK_SCREENOBJ_OFFSET);
	param.ptOffset = ptOffset;
	p->setParam(param);
	param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COMMAND);
	param.eCommand = stPictureParam::COM_PICTURE_UPDATEVERTEX;
	p->setParam(param);
	return TRUE;
}

void cSpecialScroll::CreatePicture()
{
	for (int i=0; i<MAX_IMAGE; i++)
	{
		if (m_param.aImage[i][0] == 0)
			break;
		if (m_aPicture[i] != NULL)
			continue;
		GetGraphics()->CreateGraphicsElement(GT_PICTURE,&m_aPicture[i]);
		ASSERT(m_aPicture[i]);
		stPictureParam param;
		param.SetMask(MASK_PICTURE,0);
		param.AddMask(stPictureParam::MASK_PICTURE_LAYER|stPictureParam::MASK_PICTURE_RESOURCE);
		stResourceA& stRes = param.stResA;
		CString strSpr = m_param.aImage[i];

		stRes.stDevice.format = D3DFMT_R5G6B5;
		if (strSpr.Right(4) == ".spr")
			stRes.stDevice.eMedia = MEDIA_VIDEO;
		else 
		{
			stRes.stDevice.eMedia = MEDIA_VIDEOOTHER;
			if (strSpr.Right(4) == ".tga")
				stRes.stDevice.format = D3DFMT_A4R4G4B4;
		}
		stRes.stDevice.eUsage = USAGE_MENU;
		stRes.stDevice.pool = D3DPOOL_MANAGED;
		stRes.stRes.iFrame = m_param.aImageFrame[i];
		stRes.stRes.idResource = 0;
		strcpy(stRes.stRes.szResource,m_param.aImage[i]);
		if (m_param.bBackGround)
			param.nLayer = -1;
		else
			param.nLayer = 1;
		m_aPicture[i]->setParam(param);

		param.SetMask(MASK_OBJ,stObjParam::MASK_OBJ_COMMAND);
		param.eCommand = stPictureParam::COM_PICTURE_UPDATEVERTEX;
		m_aPicture[i]->setParam(param);

		GetScreen()->AddiObj(m_aPicture[i]);
	}
}

BOOL cSpecialScroll::Run()
{
	if (m_param.aImage[0][0] == 0)
		return FALSE;
	if (strlen(m_param.aImage[1]) == 0 )
	{
		strcpy(m_param.aImage[1],m_param.aImage[0]);
		m_param.aImageFrame[1] = m_param.aImageFrame[0];
	}

	Active();
	if (!IsActive())
		return FALSE;

	CreatePicture();

	int xScreen,yScreen;
	GetGraphics()->GetScreen(xScreen,yScreen);

	//when camera at m_param.ptStartCamera
	//the first picture at (0,0);
	int aWidth[MAX_IMAGE];
	int aPictureWidth[MAX_IMAGE];
	int imagewidth = 0;
	int imageheight = m_aPicture[0]->GetPictureHeight();
	for (int i=0; i<MAX_IMAGE; i++)
	{
		if (m_aPicture[i] == NULL)
			break;
		m_aPicture[i]->SetHide(TRUE);
		aPictureWidth[i] = m_aPicture[i]->GetPictureWidth();
		ASSERT(aPictureWidth[i] >= xScreen);
		imagewidth += m_aPicture[i]->GetPictureWidth();
		aWidth[i] = imagewidth;
	}

	cPoint ptFocus = GetCamera()->GetFocus();
	cPoint ptMoved = ptFocus - m_param.ptCameraInit;

	CPoint ptMovedPicture;
	ptMovedPicture.x = ptMoved.x * m_param.ptSpeed.x;
	ptMovedPicture.y = ptMoved.y * m_param.ptSpeed.y;

	if (m_param.vSpeedMove != cPoint(0,0,0))
	{
		m_nFrame++;
		ptMovedPicture.x += m_param.vSpeedMove.x * m_nFrame; 
		ptMovedPicture.y += m_param.vSpeedMove.y * m_nFrame; 
	}
	
	CPoint ptIni = CPoint(0,0);
	CPoint ptNow = ptIni + ptMovedPicture;
	ptNow.x = ptNow.x % imagewidth;
	//算出在哪一个图上使用该图和下一个图来绘制
	for (i=0; i<MAX_IMAGE; i++)
	{
		if (aWidth[i] > ptNow.x)
			break;
	}
	
	int i0 = i;
	CPoint ptStart0,ptEnd0;
	if (i >= 1)
		ptStart0.x = -(ptNow.x - aWidth[i-1]);
	else
		ptStart0.x = -(ptNow.x - 0);
	ptStart0.y = ptNow.y;

	SetPictureOffset(m_aPicture[i0],ptStart0);
	m_aPicture[i0]->SetHide(FALSE);
	
	if (aWidth[i] - ptNow.x < xScreen)
	{
		int i1 = i+1;
		if (i1 >= MAX_IMAGE || m_aPicture[i1] == NULL)
			i1 = 0;
		ptStart0.x = aWidth[i] - ptNow.x;
		ptStart0.y = ptNow.y;

		SetPictureOffset(m_aPicture[i1],ptStart0);
		m_aPicture[i1]->SetHide(FALSE);
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//utility
//////////////////////////////////////////////////////////////////////
cPoint cSpecialScroll::GetDrawPoint()
{
	cPoint pt = GetCamera()->GetFocus();
	cRect m_rect = GetScrollRect();
	if (pt.x < m_rect.l)
		pt.x = m_rect.l;
	if (pt.x > m_rect.r)
		pt.x = m_rect.r;
	if (pt.y < m_rect.t)
		pt.y = m_rect.t;
	if (pt.y > m_rect.b)
		pt.y = m_rect.b;
	return pt;
}

