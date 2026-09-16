// cTextureSpr.cpp: implementation of the cTextureSpr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureSpr.h"

#include "cGraphics.h"
#include "iImage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureSpr::cTextureSpr(cGraphics* pGraphics) : cTexture(pGraphics)
{
	m_pImage = NULL;
}

cTextureSpr::~cTextureSpr()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////

bool cTextureSpr::IsUpload(MediaEnum type)
{
	if (type == MEDIA_SPR)
		return m_pImage != NULL;
	return FALSE;
}

long cTextureSpr::Upload(MediaEnum type)
{
	if (type != MEDIA_SPR)
		return FALSE;
	ASSERT(type == MEDIA_SPR);
	//load spr into memory!
	ASSERT(m_pImage == NULL);
	m_pImage = Image_CreateImage(m_stResA.stRes.szResource);
	ASSERT(m_pImage);
	HRESULT hr = m_pImage->LoadFile(m_stResA.stRes.szResource);
	if (FAILED(hr))
	{
		RELEASE(m_pImage);
		return -1;
	}

	///////////////////debug
#ifdef _DEBUG_texture_load			
	CString strMsg;
	strMsg.Format("upload: texture spr %s\n", m_stResA.stRes.szResource);
	OutputDebugString(strMsg);
#endif

	return GetSize(type);
}

ULONG cTextureSpr::Download(MediaEnum type)
{
	if (type != MEDIA_SPR)
		return 0;

	if(m_pImage == NULL)
		return 0;

	ULONG sz = GetSize(type);
	ASSERT(type == MEDIA_SPR);
	RELEASE(m_pImage);

	m_bInvalid = true;

	///////////////////debug
#ifdef _DEBUG_texture_load			
	CString strMsg;
	strMsg.Format("download: texture spr %s\n", m_stResA.stRes.szResource);
	OutputDebugString(strMsg);
#endif

	return sz;
}

ULONG cTextureSpr::GetSize(MediaEnum type)
{
	if (type != MEDIA_SPR)
		return 0;	
	if (m_pImage == NULL)
		return 0;
	int size = 0;
	ASSERT(m_pImage != NULL);
	stImageInfo info;
	m_pImage->GetImageInfo(info);
	size += info.size;
	return size;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureSpr::GetTextureRect(int frame,cRect& rc)
{
	ASSERT(0);
}

void cTextureSpr::GetTextureSize(int frame,ULONG& width, ULONG& height)
{
	ASSERT(0);
}

void cTextureSpr::GetImageSize(int frame, ULONG& width, ULONG& height)
{
	stFrameInfo info;
	GetImage()->GetFrameInfo(info,frame);
	width = info.width;
	height = info.height;
//	GetImage()->GetSize(frame,width,height);
}

void cTextureSpr::GetCenterToUpleft(int frame,int& x, int& y)
{
	stFrameInfo info;
	GetImage()->GetFrameInfo(info,frame);
	x = info.xCenterToUpleft;
	y = info.yCenterToUpleft;
}

ULONG cTextureSpr::GetFrames()
{
	stImageInfo info;
	GetImage()->GetImageInfo(info);
	return info.nFrames;
	//return GetImage()->GetFrames();
}

ULONG cTextureSpr::GetDirections()
{
	stImageInfo info;
	GetImage()->GetImageInfo(info);
	return info.nDirections;
//	return GetImage()->GetDirections();
}

//////////////////////////////////////////////////////////////////////
// render
//////////////////////////////////////////////////////////////////////

HRESULT cTextureSpr::Render(stRender& render)
{
	ASSERT(render.pTexDes!= NULL);	
	D3DLOCKED_RECT lrDes;
	render.pTexDes->Lock(render.nFrameDes,lrDes);

	stImageRender rd;
	rd.nFrame = render.nFrameSrc;
	rd.buffer = lrDes.pBits;
	rd.nPitch = lrDes.Pitch;
	rd.rcSrc = render.rcSrc;
	rd.ptDes.x = render.ptDes.x;
	rd.ptDes.y = render.ptDes.y;
	if (render.format == D3DFMT_R5G6B5)
		rd.format = IMAGE_565;
	else if (render.format == D3DFMT_A4R4G4B4)
		rd.format = IMAGE_4444;
	rd.eOp = render.eOp;

	m_pImage->Render(rd);

	render.pTexDes->Unlock(render.nFrameDes);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// device
//////////////////////////////////////////////////////////////////////

HRESULT cTextureSpr::FinalCleanup()
{
	RELEASE(m_pImage);
	return S_OK;
};	

//////////////////////////////////////////////////////////////////////
// create
//////////////////////////////////////////////////////////////////////

HRESULT	cTextureSpr::Refresh()
{
	Download(MEDIA_SPR);
	Upload(MEDIA_SPR);
	return S_OK;
}
