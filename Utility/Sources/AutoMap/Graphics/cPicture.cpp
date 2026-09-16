// cPicture.cpp: implementation of the cPicture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "graphics.h"
#include "cgraphics.h"

#include "cPicture.h"
#include "cScreen.h"
#include "position.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPicture::cPicture(cGraphics* p) : cScreenObj(p)
{
	m_pTexture = NULL;
	for (int i=0; i<4; i++)
	{
		m_aVS[i].sz = 0.f;
		m_aVS[i].rhw = 1.f;
	}

	m_aVS[0].tu = 0.f; 
	m_aVS[0].tv = 0.f;
	m_aVS[1].tu = 0.f;
	m_aVS[1].tv = 1.f;
	m_aVS[2].tu = 1.f;
	m_aVS[2].tv = 0.f;
	m_aVS[3].tu = 1.f;
	m_aVS[3].tv = 1.f;
	m_bHide = FALSE;
}

cPicture::~cPicture()
{
	FinalCleanup();
}

void cPicture::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPictureParam;
	cScreenObj::Init(false);
}

/**************************************************************************/
//device
/**************************************************************************/
HRESULT cPicture::FinalCleanup()
{
	RELEASE(m_pTexture);
	return S_OK;
}

/**************************************************************************/
//param	
/**************************************************************************/
HRESULT	cPicture::OnCommand(long eCommand,stObjParam& param)
{
	cScreenObj::OnCommand(eCommand,param);

	switch (eCommand)
	{
	case stPictureParam::COM_PICTURE_UPDATEVERTEX:
		UpdateVertexBuffer();
		break;
	}
	return 0;
}

HRESULT cPicture::setParam(stObjParam& pm)
{
	HRESULT hr = cScreenObj::setParam(pm);
	if (!pm.IsEMask(MASK_PICTURE))
		return hr;

	stPictureParam& param = *((stPictureParam* )&pm);
	stPictureParam& m_param = *GetPictureParam();

	if (param.IsSetMask(stPictureParam::MASK_PICTURE_RESOURCE))
	{
		m_param.stResA = param.stResA;
		iTexture* p = GetGraphics()->FindTexture(m_param.stResA);
		if (!p)
		{
			ASSERT(0);
			return -1;
		}
		RELEASE(m_pTexture);
		m_pTexture = p;
	}
	if (param.IsSetMask(stPictureParam::MASK_PICTURE_SUBRECT))
		m_param.rcSub = param.rcSub;
	if (param.IsSetMask(stPictureParam::MASK_PICTURE_LAYER))
		m_param.nLayer = param.nLayer;
	return S_OK;
}

/**************************************************************************/
//vertex	
/**************************************************************************/
void cPicture::UpdateVertexBuffer()
{
	GetScreen()->SetTexture(GetTexture(),GetiFrame());

	ULONG width,height;
	GetTexture()->GetImageSize(GetiFrame(),width,height);
	ULONG wTex,hTex;
	GetTexture()->GetTextureSize(GetiFrame(),wTex,hTex);

	CRect rc = GetRectSub();
	if (rc.IsRectEmpty())
		rc = CRect(0,0,width,height);

	cRect rScreen(0,0,rc.Width(),rc.Height());
	FillRect(m_aVS, rScreen);
	cRect rUV((float)rc.left / wTex, (float)rc.top / hTex, (float)rc.right / wTex, (float)rc.bottom / hTex);
	FillRectUV(m_aVS,rUV);

	//这个在多线程的运行中出错，因为在setoffset(0,0)的时候，另一个线程取出该点,于是人物跳到了(0,0)
	CPoint ptOffset = GetOffset();
//	SetOffset(CPoint(0,0));
	Offset(ptOffset);
}

void cPicture::Offset(CPoint pt)
{
	for (int i=0; i<4; i++)
		AddPoint(m_aVS[i],pt);
}

int	cPicture::GetPictureWidth()
{
	cRect r;
	GetScreenRect(r);
	return r.Width();
}

int	cPicture::GetPictureHeight()
{
	cRect r;
	GetScreenRect(r);
	return r.Height();
}

void cPicture::GetScreenRect(cRect& r)
{
	//alway a rectangle
	r.l= m_aVS[0].sx;
	r.r= m_aVS[3].sx;
	r.t= m_aVS[0].sy;
	r.b= m_aVS[3].sy;
}

/**************************************************************************/
//texture
/**************************************************************************/
iTexture* cPicture::GetTexture()
{
	ASSERT(m_pTexture); 
	return m_pTexture;
}

/**************************************************************************/
//draw
/**************************************************************************/
BOOL cPicture::IsShow()
{
	return GetGraphics()->IsShow(iGraphics::GS_PICTURE);
}

HRESULT cPicture::Draw()
{
	if (!IsShow() || IsHide())
		return S_OK;

	stPictureParam& m_param = *GetPictureParam();
	int index = GetiFrame();

	CRect rc = m_param.rcSub;
	cColor cr = GetColor();

	for (int i=0; i<4; i++)
	{
		m_aVS[i].color = cr.Value();
	}
	
	GetScreen()->SetTexture(GetTexture(),GetiFrame());
	GetScreen()->Draw(m_aVS);		

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//根据几何信息判断是否显示
////////////////////////////////////////////////////////////////////////////
bool cPicture::CheckDisplay(const CRect* rc)
{
	m_bDisplay = true;
	return m_bDisplay;
}

