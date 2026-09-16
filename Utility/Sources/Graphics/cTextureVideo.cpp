// cTextureVideo.cpp: implementation of the cTextureVideo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureVideo.h"

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

const int MAX_FRAME = 8;
cTextureVideo::cTextureVideo(cGraphics* pGraphics): cTexture(pGraphics),m_listInfo(MAX_FRAME)
{
	m_listInfo.assign(MAX_FRAME,NULL);
	m_pTexture = NULL;
	m_eMedia = MEDIA_VIDEO;
}

cTextureVideo::~cTextureVideo()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////
bool cTextureVideo::IsResource(const stResourceA* st)
{
	ASSERT(st->stRes.idResource == 0);
	return cTexture::IsResource(st);
}

bool cTextureVideo::IsUpload(MediaEnum type)
{
	if (type != m_eMedia)
		return 0;
	return m_bUpload;
}

long cTextureVideo::Upload(MediaEnum type)
{
	if (type != m_eMedia)
		return 0;
	m_size = 0;

	m_bUpload = true;
	return m_size;
}

iTexture* cTextureVideo::GetTextureSpr()
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

ULONG cTextureVideo::DownloadSomeFrame(MediaEnum type)
{
	ResetUsed();
	ULONG sz = Download(type);
	m_bUpload = TRUE;
	return sz;
}

ULONG cTextureVideo::Download(MediaEnum type)
{
	if (type != m_eMedia)
		return 0;
	if (m_listInfo.size() == 0)
		return 0;
	cTimerFunction timer("cTextureVideo::Download");
	
	for (int i=0; i<m_listInfo.size(); i++)
	{
		if (m_listInfo[i])
		{
			m_listInfo[i]->Download();
			m_listInfo[i]->Release();
			m_listInfo[i] = NULL;
		}
	}
//	m_listInfo.clear();
		
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

ULONG cTextureVideo::GetSize(MediaEnum type)
{
	if (type == m_eMedia)
		return m_size;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureVideo::GetTextureRect(int frame,cRect& rc)
{
	ULONG w1,w2,h1,h2;
	GetImageSize(frame,w1,h1);
	GetTextureSize(frame,w2,h2);
	rc.l = 0;
	rc.t = 0;
	rc.r = (float)w1 / (float)w2;
	rc.b = (float)h1 / (float)h2;
}

void cTextureVideo::GetImageSize(int frame,ULONG& width, ULONG& height)
{
	width = m_listInfo[frame]->wImage;
	height = m_listInfo[frame]->hImage;
}

void cTextureVideo::GetTextureSize(int frame,ULONG& width, ULONG& height)
{
	width = m_listInfo[frame]->wTexture;
	height = m_listInfo[frame]->hTexture;
}

void cTextureVideo::GetCenterToUpleft(int frame,int& x, int& y)
{
	GetTextureSpr()->GetCenterToUpleft(frame,x,y);
}

ULONG cTextureVideo::GetFrames()
{
	return GetTextureSpr()->GetFrames();
}

ULONG cTextureVideo::GetDirections()
{
	return GetTextureSpr()->GetDirections();
}

void cTextureVideo::Used()
{
	cTexture::Used();
	if (m_pTexture)
		m_pTexture->Used();
}

//////////////////////////////////////////////////////////////////////
//IDirect3DTexture8
//////////////////////////////////////////////////////////////////////
stD3DTextureInfo* cTextureVideo::CreateD3DTextureInfo(int frame)
{
	cTimerFunction timer("cTextureVideo::CreateD3DTextureInfo");

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
	m_listInfo[frame] = pInfo;

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

	InfoTextureCacheUpload(m_eMedia,pInfo->GetSize());

	return pInfo;
}

IDirect3DTexture8* cTextureVideo::GetD3DTexture(int frame)
{
//	itD3DTextureInfo it = find_if(m_listInfo.begin(), m_listInfo.end(), 
//		bind2nd( mem_fun1<bool,stD3DTextureInfo,int>(&stD3DTextureInfo::IsFrame),frame ));
	
	if (frame >= m_listInfo.size())
		m_listInfo.resize(frame*1.5f);

	stD3DTextureInfo* pInfo = m_listInfo[frame];
	if (pInfo == NULL)
	{
		pInfo = CreateD3DTextureInfo(frame);
		if (pInfo == NULL)
			return NULL;
		m_listInfo[frame] = pInfo;
	}
	Used();
	return pInfo->pTexture;
}

//////////////////////////////////////////////////////////////////////
// create
//////////////////////////////////////////////////////////////////////
/*
HRESULT cTextureVideo::Create(const stResourceA& st, int width, int height)
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
HRESULT cTextureVideo::FinalCleanup()
{
	Download(m_eMedia);
	RELEASE(m_pTexture);
	return S_OK;
};	

//////////////////////////////////////////////////////////////////////
// texture
//////////////////////////////////////////////////////////////////////
HRESULT cTextureVideo::Unlock(int frame)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	p->UnlockRect(0);
	return S_OK;
}

HRESULT cTextureVideo::Lock(int frame,D3DLOCKED_RECT& lr)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	HRESULT hr = p->LockRect(0,&lr,NULL,0);
	GetGraphics()->DXError(hr);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//render
//////////////////////////////////////////////////////////////////////

HRESULT	cTextureVideo::Refresh()
{
	Download(m_eMedia);
	return S_OK;
}
