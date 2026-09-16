// cMapSmall.cpp: implementation of the cMapSmall class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "graphics.h"
#include "graphicspublic.h"
#include "cgraphics.h"
#include "cScreen.h"
#include "cMapSmall.h"
#include "cCamera.h"
#include "position.h"

#include "iItem.h"
#include "iImage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const int c_nTextureWidth = 512;
const int c_nTextureHeight = 512;
const int c_nTextureWidthRegion = 32;
const int c_nTextureHeightRegion = 32;

cMapSmall::cMapSmall(cGraphics* p) : cScreenObj(p)
{
	m_pTexture = NULL;
	m_pImage = NULL;
}

cMapSmall::~cMapSmall()
{
	RELEASE(m_pImage);
}

void cMapSmall::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stMapSmallParam;
	cScreenObj::Init(false);
	stMapSmallParam& m_param = *GetMapParam();
	m_param.bFull = FALSE;
	int w,h;
	GetGraphics()->GetScreen(w,h);
	m_param.wScreen = w / 4;
	m_param.hScreen = h / 4;
	m_param.hScreen = m_param.wScreen;
	m_param.ptOffset.x = 0;
	m_param.ptOffset.y = 0;
	InitVB();
}

void cMapSmall::InitVB()
{
	stMapSmallParam& m_param = *GetMapParam();
	cColor cr(255,255,255,255);
	for (int i=0; i<4; i++)
	{
		m_aV[i].rhw = 1.f;
		m_aV[i].color = cr.Value();
		m_aV[i].sz = 0;
	}
	m_aV[0].sx = 0;
	m_aV[0].sy = 0;
	m_aV[0].tu = 0;
	m_aV[0].tv = 0;
	m_aV[1].sx = m_param.wScreen;
	m_aV[1].sy = 0;
	m_aV[1].tu = 1;
	m_aV[1].tv = 0;
	m_aV[2].sx = 0;
	m_aV[2].sy = m_param.hScreen;
	m_aV[2].tu = 0;
	m_aV[2].tv = 1;
	m_aV[3].sx = m_param.wScreen;
	m_aV[3].sy = m_param.hScreen;
	m_aV[3].tu = 1;
	m_aV[3].tv = 1;
	for (i=0; i<4; i++)
	{
		AddPoint(m_aV[i],m_param.ptOffset);
	}
}

BOOL cMapSmall::IsShow()
{
	return GetGraphics()->IsShow(iGraphics::GS_SMALLMAP);
}

void cMapSmall::RenderTexture()
{
	D3DLOCKED_RECT lrDes;
	HRESULT hr = m_pTexture->LockRect(0,&lrDes,NULL,0);

	stImageRender rd;
	rd.nFrame = 0;
	rd.buffer = lrDes.pBits;
	rd.nPitch = lrDes.Pitch;
	rd.rcSrc = CRect(m_ptLoad.x,m_ptLoad.y,m_ptLoad.x+512,m_ptLoad.y+512);
	rd.ptDes.x = 0;
	rd.ptDes.y = 0;
	rd.format = IMAGE_565;
	rd.eOp = Render_Copy;

	m_pImage->Render(rd);

	m_pTexture->UnlockRect(0);
}

BOOL cMapSmall::VerifyTexture(BOOL bForceUpdate)
{
	stMapSmallParam& m_param = *GetMapParam();

	CRect rcGroundNow;
	ScreenToGround(0,0,rcGroundNow.left,rcGroundNow.top);
	ScreenToGround(m_param.wScreen,m_param.hScreen,rcGroundNow.right,rcGroundNow.bottom);

	CRect rcGround = CRect(m_rcRegion.left*c_nWidthRegion,m_rcRegion.top*c_nHeightRegion,
		m_rcRegion.right*c_nWidthRegion,m_rcRegion.bottom*c_nHeightRegion);
	CRect rcImage = CRect(m_rcRegion.left*32,m_rcRegion.top*32,
		m_rcRegion.right*32,m_rcRegion.bottom*32);

	rcGroundNow &= rcGround;
	if (rcGroundNow.IsRectEmpty())
		return FALSE;

	GroundToScreen(rcGroundNow.left,rcGroundNow.top,m_rcMap.left,m_rcMap.top);
	GroundToScreen(rcGroundNow.right,rcGroundNow.bottom,m_rcMap.right,m_rcMap.bottom);

	BOOL bUpdate = m_bUpdate;
	m_bUpdate = FALSE;

	m_rcImage.left = (rcGroundNow.left - rcGround.left) * 32 / c_nWidthRegion ;
	m_rcImage.top = (rcGroundNow.top - rcGround.top) * 32 / c_nHeightRegion;		
	m_rcImage.right = m_rcImage.left + (rcGroundNow.Width()) * 32 / c_nWidthRegion ;
	m_rcImage.bottom = m_rcImage.top + (rcGroundNow.Height()) * 32 / c_nHeightRegion ;
	if (!bUpdate)
	{
		if (m_rcImage.left < m_ptLoad.x || m_rcImage.right > m_ptLoad.x + c_nTextureWidth
			|| m_rcImage.top < m_ptLoad.y || m_rcImage.bottom > m_ptLoad.y + c_nTextureHeight)
		{
			bUpdate = TRUE;
		}
	}

	if (bUpdate || bForceUpdate)
	{
		//compute the ptload and ptTexture!
		//center
		stMapSmallParam& m_param = *GetMapParam();
		CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
			m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
		CPoint pt = rc.CenterPoint();
		cPoint ptGround;
		ScreenToGround(pt,ptGround);
		int dx = (ptGround.x - rcGround.left) * 32 / c_nWidthRegion;
		int dy = (ptGround.y - rcGround.top) * 32 / c_nHeightRegion;
		m_ptLoad.x = dx - c_nTextureWidth / 2;
		m_ptLoad.y = dy - c_nTextureHeight / 2;

		if (m_ptLoad.x + c_nTextureWidth > rcImage.Width())
		{
			m_ptLoad.x = rcImage.Width() - c_nTextureWidth;
		}
		if (m_ptLoad.x < 0)
		{
			m_ptLoad.x = 0;
		}
		
		if (m_ptLoad.y + c_nTextureHeight > rcImage.Height())
		{
			m_ptLoad.y = rcImage.Height() - c_nTextureHeight;
		}
		if (m_ptLoad.y < 0)
		{
			m_ptLoad.y = 0;
		}
		//render the texture!
		RenderTexture();
		//
	}

	m_ptTexture.x = m_rcImage.left - m_ptLoad.x;
	m_ptTexture.y = m_rcImage.top - m_ptLoad.y;
///*
	m_aV[0].sx = m_rcMap.left;
	m_aV[0].tu = (float)m_ptTexture.x / c_nTextureWidth;
	m_aV[0].sy = m_rcMap.top;
	m_aV[0].tv = (float)m_ptTexture.y / c_nTextureHeight;
	m_aV[3].sx = m_rcMap.right;
	m_aV[3].tu = m_aV[0].tu + ((float)m_rcMap.Width())/c_nTextureWidth;
	m_aV[3].sy = m_rcMap.bottom;
	m_aV[3].tv = m_aV[0].tv + ((float)m_rcMap.Height())/c_nTextureHeight;

	m_aV[1].sx = m_aV[3].sx;
	m_aV[1].tu = m_aV[3].tu;
	m_aV[1].sy = m_aV[0].sy;
	m_aV[1].tv = m_aV[0].tv;
	m_aV[2].sx = m_aV[0].sx;
	m_aV[2].tu = m_aV[0].tu;
	m_aV[2].sy = m_aV[3].sy;
	m_aV[2].tv = m_aV[3].tv;
//*/
	return TRUE;
}

HRESULT	cMapSmall::DrawTexture()
{
	if (m_strMap.IsEmpty())
		return S_OK;
	if (m_pImage == NULL)
		return -1;
	if (VerifyTexture())
	{
		HRESULT hr;
		hr = GetGraphics()->GetDevice3D()->SetTexture(0,m_pTexture);
		GetScreen()->Draw(m_aV);
	}

	return S_OK;
}

HRESULT	cMapSmall::Draw()
{
	//use a texture of 512*512
	if (!IsShow())
		return S_OK;

	DrawTexture();

	stMapSmallParam& m_param = *GetMapParam();
	if (m_pImage != NULL)
	{
		cRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
			m_param.ptOffset.x+m_param.wScreen,
			m_param.ptOffset.y+m_param.hScreen);
		cPoint ptCenter = rc.CenterPoint();
		GetScreen()->DrawRect(rc,cColor(0,0,255));
//draw button
		rc = cRect(m_param.ptOffset.x,m_param.ptOffset.y,m_param.ptOffset.x+8,m_param.ptOffset.y+8);
		GetScreen()->DrawRect(rc,cColor(0,255,0));
		int dx = 800.f/512.f*c_nTextureWidthRegion/2;
		int dy = 600.f/512.f*c_nTextureHeightRegion/2;
		rc = cRect(ptCenter.x - dx,ptCenter.y - dy,
			ptCenter.x + dx,ptCenter.y + dy);
		GetScreen()->DrawRect(rc,cColor(0,255,0));
	}

	return S_OK;
}

HRESULT	cMapSmall::setParam(stObjParam& pm)
{
	HRESULT hr = cScreenObj::setParam(pm);
	
	if (!pm.IsEMask(MASK_MAPSMALL))
		return hr;

	stMapSmallParam& param = *((stMapSmallParam* )&pm);
	stMapSmallParam& m_param = *GetMapParam();
	if (pm.IsMask(stMapSmallParam::MASK_MAP_FULL))
	{
		m_param.bFull = param.bFull;
	}
	return S_OK;
}

void cMapSmall::GroundToScreen(int x,int y, long & nx,long & ny)
{
	stMapSmallParam& m_param = *GetMapParam();

	cPoint ptGroundFocus = GetGraphics()->GetcCamera()->GetFocus();
	int offx = x - ptGroundFocus.x;
	int offy = y - ptGroundFocus.y;

	CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
		m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
	CPoint pt = rc.CenterPoint();

	nx = pt.x + offx * 32 / c_nWidthRegion;
	ny = pt.y + offy * 32 / c_nHeightRegion;
}

void cMapSmall::ScreenToGround(int x,int y, long & nx,long & ny)
{
	stMapSmallParam& m_param = *GetMapParam();

	cPoint ptGroundFocus = GetGraphics()->GetcCamera()->GetFocus();
	//the center is the current region!	
	CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
		m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
	CPoint pt = rc.CenterPoint();
	CPoint ptOffsetScreen = CPoint(x,y)- pt;
	nx = ptGroundFocus.x + ptOffsetScreen.x * c_nWidthRegion / 32;
	ny = ptGroundFocus.y + ptOffsetScreen.y * c_nHeightRegion / 32;
}

void cMapSmall::ScreenToGround(CPoint ptScreen, cPoint& ptGround)
{
	stMapSmallParam& m_param = *GetMapParam();

	cPoint ptGroundFocus = GetGraphics()->GetcCamera()->GetFocus();
	//the center is the current region!	
	CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
		m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
	CPoint pt = rc.CenterPoint();
	CPoint ptOffsetScreen = ptScreen - pt;
	cPoint ptOffsetGround = cPoint(ptOffsetScreen.x * c_nWidthRegion / 32 , ptOffsetScreen.y * c_nHeightRegion / 32 );
	ptGround = ptGroundFocus;
	ptGround += ptOffsetGround;
}

HRESULT	cMapSmall::OnCommand(long Command,stObjParam& pm)
{
	cScreenObj::OnCommand(Command,pm);

	stMapSmallParam& param = *((stMapSmallParam* )&pm);
	stMapSmallParam& m_param = *GetMapParam();

	switch (Command)
	{
	case stMapSmallParam::COM_MAP_GETCLICKPOS:
		{
			CRect rcButton(m_param.ptOffset.x,m_param.ptOffset.y,
				m_param.ptOffset.x+8,m_param.ptOffset.y+8);
			if (rcButton.PtInRect(param.ptScreenClick))
			{
				m_param.bFull = !m_param.bFull;
				if (m_param.bFull)
				{
					m_param.wScreen = 512;
					m_param.hScreen = 512;
				}
				else
				{
					int w,h;
					GetGraphics()->GetScreen(w,h);
					m_param.wScreen = w / 4;
					m_param.hScreen = h / 4;
					m_param.hScreen = m_param.wScreen;
				}
				InitVB();
				return -1;
			}
			else
			{
				CRect rc(m_param.ptOffset.x,m_param.ptOffset.y,
					m_param.ptOffset.x+m_param.wScreen,m_param.ptOffset.y+m_param.hScreen);
				if (!rc.PtInRect(param.ptScreenClick))
					return -1;
				ScreenToGround(param.ptScreenClick,param.ptGroundClick);
			}
		}
		break;
	}
	return S_OK;
}

HRESULT cMapSmall::LoadBmp()
{
	RELEASE(m_pImage);
	m_pImage = Image_CreateImage(m_strMap);
	if (FAILED(m_pImage->LoadFile(m_strMap)))
	{
		RELEASE(m_pImage);
		return -1;
	}

	if (!m_pTexture)
	{
		stResourceA stRes;
		stRes.stDevice.eMedia = MEDIA_VIDEOOTHER;
		stRes.stDevice.eUsage = USAGE_MENU;
		stRes.stDevice.format = D3DFMT_R5G6B5;
		stRes.stDevice.pool = D3DPOOL_MANAGED;
		stRes.stDevice.usage = D3DX_DEFAULT;
		stRes.stRes.iFrame = 0;
		stRes.stRes.idResource = 0;
		strcpy(stRes.stRes.szResource,m_strMap);

		HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(
			c_nTextureWidth,c_nTextureHeight,1,0,stRes.stDevice.format,D3DPOOL_MANAGED, &(m_pTexture));

		DX_ERROR(hr);
	}

	m_bUpdate = TRUE;
	return TRUE;
}

HRESULT cMapSmall::OnFileOpen(LPCSTR szMap)
{
	m_strMap.Empty();
	RELEASE(m_pTexture);

	iItem* pItem = Common_CreateItem(szMap,iItem::FLAG_LOAD);
	if (pItem == NULL)
		return -1;
	wrap_ptr<iItem> ptr(pItem);
	CRect rc;
	m_rcRegion.SetRect(0,0,0,0);
	if (GetItemValue(pItem,"rect",rc))
	{
		m_rcRegion = rc;
	}
	else
		return S_OK;

	CString sFile = szMap;
	sFile = sFile.Left(sFile.GetLength() - 3) + "bmp";
	CFileFind f;
	if (f.FindFile(sFile))
	{
		m_strMap = sFile;
		LoadBmp();
	}

	return S_OK;
}

HRESULT cMapSmall::OnCameraUpdate()
{
	return S_OK;	
}

/**************************************************************************/
//device
/**************************************************************************/
HRESULT cMapSmall::FinalCleanup()
{
	RELEASE(m_pTexture);
	return S_OK;
}

