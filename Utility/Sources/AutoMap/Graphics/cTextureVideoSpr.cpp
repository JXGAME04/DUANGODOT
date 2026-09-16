// cTextureVideoSpr.cpp: implementation of the cTextureVideoSpr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureVideoSpr.h"

#include "cGraphics.h"
#include "cTextureMemory.h"
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

bool stD3DTextureInfo::IsFrame(int frm)
{
	return frame == frm;
}

const int MAX_FRAME = 256;
cTextureVideoSpr::cTextureVideoSpr(cGraphics* pGraphics): cTexture(pGraphics),m_listInfo(MAX_FRAME)
{
	m_listInfo.assign(MAX_FRAME,NULL);
	m_pTexture = NULL;
}

cTextureVideoSpr::~cTextureVideoSpr()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////
bool cTextureVideoSpr::IsUpload(MediaEnum type)
{
	if (type != MEDIA_VIDEO)
		return 0;
	return m_bUpload;
}

ULONG cTextureVideoSpr::Upload(MediaEnum type)
{
	if (type != MEDIA_VIDEO)
		return 0;
	m_size = 0;

	m_bUpload = true;
	return m_size;
}

void cTextureVideoSpr::GetTextureSpr()
{
	if (!m_pTexture)
	{
		stResourceA posRes;
		posRes.stRes = m_stResA.stRes;
		posRes.stDevice.eUsage = m_stResA.stDevice.eUsage;
		posRes.stDevice.eMedia = MEDIA_SPR;
		m_pTexture = GetGraphics()->FindTexture(posRes,true);
		ASSERT(m_pTexture);
	}
}

ULONG cTextureVideoSpr::Download(MediaEnum type)
{
	if (type != MEDIA_VIDEO)
		return 0;
	for (int i=0; i<m_listInfo.size(); i++)
	{
		if (m_listInfo[i])
		{
			m_listInfo[i]->Download();
			m_listInfo[i]->Release();
			m_listInfo[i] = NULL;
		}
	}
	m_listInfo.clear();
		
	{
		///////////////////debug
		CString strMsg;
		strMsg.Format("download: texture video %s\n", m_stResA.stRes.szResource);
		OutputDebugString(strMsg);
	}
	
	ULONG sz = m_size;
	m_size = 0;
	m_bUpload = false;
	return sz;
}

ULONG cTextureVideoSpr::GetSize(MediaEnum type)
{
	if (type == MEDIA_VIDEO)
		return m_size;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureVideoSpr::GetTextureRect(int frame,cRect& rc)
{
	ULONG w1,w2,h1,h2;
	GetImageSize(frame,w1,h1);
	GetTextureSize(frame,w2,h2);
	rc.l = 0;
	rc.t = 0;
	rc.r = (float)w1 / (float)w2;
	rc.b = (float)h1 / (float)h2;
}

void cTextureVideoSpr::GetImageSize(int frame,ULONG& width, ULONG& height)
{
	width = m_listInfo[frame]->wImage;
	height = m_listInfo[frame]->hImage;
}

void cTextureVideoSpr::GetTextureSize(int frame,ULONG& width, ULONG& height)
{
	width = m_listInfo[frame]->wTexture;
	height = m_listInfo[frame]->hTexture;
}

void cTextureVideoSpr::GetCenterToUpleft(int frame,int& x, int& y)
{
	ASSERT(m_pTexture != NULL);
	m_pTexture->GetCenterToUpleft(frame,x,y);
}

ULONG cTextureVideoSpr::GetFrames()
{
	ASSERT(m_pTexture != NULL);
	return m_pTexture->GetFrames();
}

ULONG cTextureVideoSpr::GetDirections()
{
	ASSERT(m_pTexture != NULL);
	return m_pTexture->GetDirections();
}

void cTextureVideoSpr::Used()
{
	cTexture::Used();
	if (m_pTexture)
		m_pTexture->Used();
}

//////////////////////////////////////////////////////////////////////
//IDirect3DTexture8
//////////////////////////////////////////////////////////////////////
stD3DTextureInfo* cTextureVideoSpr::CreateD3DTextureInfo(int frame)
{
	ASSERT(m_pTexture != NULL);
	if (!m_pTexture->IsValid())
	{
		RELEASE(m_pTexture);
		GetTextureSpr();
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
	
	{
		CString strMsg;
		strMsg.Format("create file %s frame %d video\n",m_stResA.stRes.szResource,frame);
		OutputDebugString(strMsg);
	}
		 
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

	InfoTextureCacheUpload(MEDIA_VIDEO,pInfo->GetSize());

	return pInfo;
}

IDirect3DTexture8* cTextureVideoSpr::GetD3DTexture(int frame)
{
//	itD3DTextureInfo it = find_if(m_listInfo.begin(), m_listInfo.end(), 
//		bind2nd( mem_fun1<bool,stD3DTextureInfo,int>(&stD3DTextureInfo::IsFrame),frame ));
	
	stD3DTextureInfo* pInfo = m_listInfo[frame];
	if (pInfo == NULL)
	{
		pInfo = CreateD3DTextureInfo(frame);
		m_listInfo[frame] = pInfo;
	}
	Used();
	return pInfo->pTexture;
}

//////////////////////////////////////////////////////////////////////
// create
//////////////////////////////////////////////////////////////////////

HRESULT cTextureVideoSpr::Create(const stResourceA& st, int width, int height)
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

//////////////////////////////////////////////////////////////////////
// device info
//////////////////////////////////////////////////////////////////////
HRESULT cTextureVideoSpr::InitDeviceObjects()
{
	return S_OK;
};

HRESULT cTextureVideoSpr::RestoreDeviceObjects()
{
	return S_OK;
};                     

HRESULT cTextureVideoSpr::InvalidateDeviceObjects()
{
	return S_OK;
};

HRESULT cTextureVideoSpr::DeleteDeviceObjects()
{
	return S_OK;
};

HRESULT cTextureVideoSpr::FinalCleanup()
{
	Download(MEDIA_VIDEO);
	RELEASE(m_pTexture);
	return S_OK;
};	

//////////////////////////////////////////////////////////////////////
// texture
//////////////////////////////////////////////////////////////////////
HRESULT cTextureVideoSpr::Unlock(int frame)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	p->UnlockRect(0);
	return S_OK;
}

HRESULT cTextureVideoSpr::Lock(int frame,D3DLOCKED_RECT& lr)
{
	IDirect3DTexture8* p = GetD3DTexture(frame);
	HRESULT hr = p->LockRect(0,&lr,NULL,0);
	GetGraphics()->DXError(hr);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
//render
//////////////////////////////////////////////////////////////////////

HRESULT cTextureVideoSpr::Render(stRender& render) 
{
	ASSERT(0);
	return S_OK;
}

HRESULT	cTextureVideoSpr::Refresh()
{
	Download(MEDIA_VIDEO);
	return S_OK;
}
