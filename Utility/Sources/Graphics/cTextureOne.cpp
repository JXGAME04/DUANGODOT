// cTextureOne.cpp: implementation of the cTextureOne class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureOne.h"

#include "ctimer.h"

#include "cGraphics.h"
#include "template.h"
#include <numeric>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureOne::cTextureOne(cGraphics* pGraphics): cTexture(pGraphics)
{
	m_pTexture = NULL;
	m_pInfo = NULL;
}

cTextureOne::~cTextureOne()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////
bool cTextureOne::IsResource(const stResourceA* st)
{
	ASSERT(st->stRes.idResource == 0);
	return cTexture::IsResource(st);
}

bool cTextureOne::IsUpload(MediaEnum type)
{
	if (type != MEDIA_VIDEOONE)
		return 0;
	return m_bUpload;
}

long cTextureOne::Upload(MediaEnum type)
{
	if (type != MEDIA_VIDEOONE)
		return 0;
	m_size = 0;

	m_bUpload = true;
	return m_size;
}

iTexture* cTextureOne::GetTextureSpr()
{
	if (m_pTexture == NULL)
	{
		stResourceA posRes;
		posRes.stRes = m_stResA.stRes;
		posRes.stDevice.eUsage = m_stResA.stDevice.eUsage;
		posRes.stDevice.eMedia = MEDIA_SPR;
		m_pTexture = GetGraphics()->FindTexture(posRes,true);
	}
	return m_pTexture;
}

ULONG cTextureOne::Download(MediaEnum type)
{
	if (type != MEDIA_VIDEOONE || m_pInfo == NULL)
		return 0;
	m_pInfo->Download();
	m_pInfo->Release();
	m_pInfo = NULL;

		///////////////////debug
#ifdef _DEBUG_texture_load			
	CString strMsg;
	strMsg.Format("download: texture video %s\n", m_stResA.stRes.szResource);
	OutputDebugString(strMsg);
#endif
		
	ULONG sz = m_size;
	m_size = 0;
	m_bUpload = false;
	return sz;
}

ULONG cTextureOne::GetSize(MediaEnum type)
{
	if (type == MEDIA_VIDEOONE)
		return m_size;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureOne::GetTextureRect(int frame,cRect& rc)
{
	ULONG w1,w2,h1,h2;
	GetImageSize(frame,w1,h1);
	GetTextureSize(frame,w2,h2);
	rc.l = 0;
	rc.t = 0;
	rc.r = (float)w1 / (float)w2;
	rc.b = (float)h1 / (float)h2;
}

void cTextureOne::GetImageSize(int frame,ULONG& width, ULONG& height)
{
	if (m_pInfo->IsFrame(frame))
	{
		width = m_pInfo->wImage;
		height = m_pInfo->hImage;
	}
	else 
	{
		ASSERT(m_pTexture);
		m_pTexture->GetImageSize(frame,width,height);
	}	
}

void cTextureOne::GetTextureSize(int frame,ULONG& width, ULONG& height)
{
	if (m_pInfo->IsFrame(frame))
	{
		width = m_pInfo->wTexture;
		height = m_pInfo->hTexture;
	}
	else 
	{
		ASSERT(m_pTexture);
		m_pTexture->GetTextureSize(frame,width,height);
	}	
}

void cTextureOne::GetCenterToUpleft(int frame,int& x, int& y)
{
	GetTextureSpr()->GetCenterToUpleft(frame,x,y);
}

ULONG cTextureOne::GetFrames()
{
	return GetTextureSpr()->GetFrames();
}

ULONG cTextureOne::GetDirections()
{
	return GetTextureSpr()->GetDirections();
}

void cTextureOne::Used()
{
	cTexture::Used();
	if (m_pTexture)
		m_pTexture->Used();
}

//////////////////////////////////////////////////////////////////////
//IDirect3DTexture8
//////////////////////////////////////////////////////////////////////
stD3DTextureInfo* cTextureOne::CreateD3DTextureInfo(int frame)
{
	int sz = Download(MEDIA_VIDEOONE);
	InfoTextureCacheUpload(MEDIA_VIDEOONE,-sz);	
	m_bUpload = TRUE;

	if (m_pTexture == NULL)
	{
		GetTextureSpr();
		if (m_pTexture == NULL)
			return NULL;
	}

	stD3DTextureInfo* pInfo = new stD3DTextureInfo;
	pInfo->frame = frame;
	
	ULONG width,height;
	int x,y;
	int Width,Height;

	m_pTexture->GetImageSize(frame,width,height);
	m_pTexture->GetCenterToUpleft(frame,x,y);
	Width = Get2Mi(width);
	Height = Get2Mi(height);
	
#ifdef _DEBUG_texture_load			
	CString strMsg;
	strMsg.Format("upload %s frame %d video\n",m_stResA.stRes.szResource,frame);
	OutputDebugString(strMsg);
#endif
		 
	HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(Width,Height,0,
		0,
		GetResD3DFormat(),
		D3DPOOL_MANAGED,
		&(pInfo->pTexture)
		);
	GetGraphics()->DXError(hr);
	m_pInfo = pInfo;

	stRender render;
	render.pTexDes = this;
	render.ptDes = CPoint(0,0);
	render.nFrameDes = frame;

	render.pTexSrc = m_pTexture;
	render.rcSrc = CRect(0,0,width,height);
	render.nFrameSrc = frame;
	render.format = GetResD3DFormat();
	render.eOp = Render_Copy;

	render.pTexSrc->Render(render);

	pInfo->format = render.format;

	pInfo->x = x;
	pInfo->y = y;
	pInfo->wImage = width;
	pInfo->hImage = height;
	pInfo->wTexture = Width;
	pInfo->hTexture = Height;
	pInfo->byte_per_pixel = GetPixelBytes(render.format);;
	m_size += pInfo->GetSize();

	InfoTextureCacheUpload(MEDIA_VIDEOONE,pInfo->GetSize());

	return pInfo;
}

IDirect3DTexture8* cTextureOne::GetD3DTexture(int frame)
{
//	itD3DTextureInfo it = find_if(m_listInfo.begin(), m_listInfo.end(), 
//		bind2nd( mem_fun1<bool,stD3DTextureInfo,int>(&stD3DTextureInfo::IsFrame),frame ));
	
	if (m_pInfo == NULL || !m_pInfo->IsFrame(frame))
	{
		m_pInfo = CreateD3DTextureInfo(frame);
		if (m_pInfo == NULL)
			return NULL;
	}
	Used();
	return m_pInfo->pTexture;
}

//////////////////////////////////////////////////////////////////////
// create
//////////////////////////////////////////////////////////////////////
/*
HRESULT cTextureOne::Create(const stResourceA& st, int width, int height)
{
	//how to 
	stD3DTextureInfo* pInfo = new stD3DTextureInfo;
	pInfo->format = st.stDevice.format;
	pInfo->frame = 0;
	pInfo->byte_per_pixel = 2;
	pInfo->x = 0;
	pInfo->y = 0;
	pInfo->wTexture = width;
	pInfo->hTexture = height;
	HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(
		width,height,1,0,st.stDevice.format,D3DPOOL_MANAGED, &(pInfo->pTexture));
	//When used default can not use locked function, 
	//can only used updatetexture or copy to changed the texture
	GetGraphics()->DXError(hr);

	m_listInfo[0] = pInfo;
	
	m_stResA = st;

	return S_OK;
}
*/
//////////////////////////////////////////////////////////////////////
// device info
//////////////////////////////////////////////////////////////////////
HRESULT cTextureOne::FinalCleanup()
{
	Download(MEDIA_VIDEOONE);
	RELEASE(m_pTexture);
	return S_OK;
};	

//////////////////////////////////////////////////////////////////////
// texture
//////////////////////////////////////////////////////////////////////
HRESULT cTextureOne::Unlock(int frame)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	p->UnlockRect(0);
	return S_OK;
}

HRESULT cTextureOne::Lock(int frame,D3DLOCKED_RECT& lr)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	HRESULT hr = p->LockRect(0,&lr,NULL,0);
	GetGraphics()->DXError(hr);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//render
//////////////////////////////////////////////////////////////////////

HRESULT	cTextureOne::Refresh()
{
	Download(MEDIA_VIDEOONE);
	return S_OK;
}
