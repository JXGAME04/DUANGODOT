// cTextureMemory.cpp: implementation of the cTextureMemory class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureMemory.h"

#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureMemory::cTextureMemory(cGraphics* pGraphics) : cTexture(pGraphics)
{
	m_pTextureMemory = NULL;
	m_pTextureVideo = NULL;

	m_bInvalidateVideo = false;
}

cTextureMemory::~cTextureMemory()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// COM
//////////////////////////////////////////////////////////////////////

ULONG cTextureMemory::AddRef() 
{
	return ++m_ulCount;
}

//should delete it in cache!
ULONG cTextureMemory::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////

bool cTextureMemory::IsUpload(MediaEnum type)
{
	switch (type)
	{
	case MEDIA_VIDEO:
		return m_pTextureVideo != NULL;
	case MEDIA_MEMORY:
		return m_pTextureMemory != NULL;
	}
	//ASSERT(0);
	return false;
}

long cTextureMemory::Upload(MediaEnum type)
{
	switch (type)
	{
	case MEDIA_VIDEO:
		{
			ASSERT(m_pTextureVideo == NULL);
			CreateTextureVideo();

			///////////////////debug
#ifdef _DEBUG_texture_load			
			CString strMsg;
			if (m_stResA.stRes.idResource == 0) 
				strMsg.Format("upload: texture memory of video %s\n", m_stResA.stRes.szResource);
			else
				strMsg.Format("upload: texture memory of video %d\n", m_stResA.stRes.idResource);
			OutputDebugString(strMsg);
#endif
			return GetSize(type);
		}
	case MEDIA_MEMORY:
		{
			ASSERT(m_pTextureMemory == NULL);

			///////////////////debug
#ifdef _DEBUG_texture_load			
			CString strMsg;
			if (m_stResA.stRes.idResource == 0) 
				strMsg.Format("upload: texture memory of sys memory %s\n", m_stResA.stRes.szResource);
			else
				strMsg.Format("upload: texture memory of sys memory %d\n", m_stResA.stRes.idResource);
			OutputDebugString(strMsg);
#endif

			return GetSize(type);
		}
		break;
	default:
		ASSERT(0);
		break;
	}
	return 0;
}

ULONG cTextureMemory::Download(MediaEnum type)
{
	int size = GetSize(type);
	switch (type)
	{
	case MEDIA_VIDEO:
		{
			ASSERT(m_pTextureVideo);
			RELEASE(m_pTextureVideo);
			///////////////////debug
			CString strMsg;
			if (m_stResA.stRes.idResource == 0) 
				strMsg.Format("download: texture memory of video %s\n", m_stResA.stRes.szResource);
			else
				strMsg.Format("download: texture memory of video %d\n", m_stResA.stRes.idResource);
			OutputDebugString(strMsg);
		}
		break;
	case MEDIA_MEMORY:
		{
			ASSERT(m_pTextureMemory);
			RELEASE(m_pTextureMemory);
			///////////////////debug
			CString strMsg;
			if (m_stResA.stRes.idResource == 0) 
				strMsg.Format("download: texture memory of sys memory %s\n", m_stResA.stRes.szResource);
			else
				strMsg.Format("download: texture memory of sys memory %d\n", m_stResA.stRes.idResource);
			OutputDebugString(strMsg);

			SetInvalid();
		}
		break;
	}
	return size;
}

ULONG cTextureMemory::GetSize(MediaEnum type)
{
	//return spr memory size
	switch (type)
	{
	case MEDIA_VIDEO:
		{
			if (m_pTextureVideo)
			{
				D3DSURFACE_DESC desc;
				m_pTextureVideo->GetLevelDesc(0,&desc);
				return desc.Size;
			}
			break;
		}
	case MEDIA_MEMORY:
		{
			if (m_pTextureMemory)
			{
				D3DSURFACE_DESC desc;
				m_pTextureMemory->GetLevelDesc(0,&desc);
				return desc.Size;
			}
			break;
		}
		break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureMemory::GetTextureRect(int frame,cRect& rc)
{
	rc = cRect(0,0,1,1);
}

void cTextureMemory::GetImageSize( int frame, ULONG& width, ULONG& height)
{
	GetTextureSize(frame,width,height);
}

void cTextureMemory::GetTextureSize( int frame, ULONG& width, ULONG& height)
{
	ASSERT(frame == 0);
	//get the frame's width and height
	if (m_pTextureVideo)
	{
		D3DSURFACE_DESC desc;
		m_pTextureVideo->GetLevelDesc(0,&desc);
		width = desc.Width;
		height = desc.Height;
	}
	else if (m_pTextureMemory)
	{
		D3DSURFACE_DESC desc;
		m_pTextureMemory->GetLevelDesc(0,&desc);
		width = desc.Width;
		height = desc.Height;
	}
	else
		ASSERT(0);
}

/////////////////////////////////////////////////////////////////////
// render
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// device
//////////////////////////////////////////////////////////////////////

HRESULT cTextureMemory::InitDeviceObjects()
{
	return S_OK;
};

HRESULT cTextureMemory::RestoreDeviceObjects()
{
	if (m_bInvalidateVideo)
	{
		CreateTextureVideo();
		m_bInvalidateVideo = false;
	}
/*
	CRect r = CRect(0,0,512,512);
	HRESULT hr = D3DXSaveTextureToFile("d:\\temp.bmp",D3DXIFF_BMP,m_pTextureVideo,NULL);
	ASSERT(!FAILED(hr));	
*/
	return S_OK;
};                     

HRESULT cTextureMemory::InvalidateDeviceObjects()
{
	if (m_pTextureVideo)
	{
		RELEASE(m_pTextureVideo);
		m_bInvalidateVideo = true;
	}
	return S_OK;
};

HRESULT cTextureMemory::DeleteDeviceObjects()
{
	RELEASE(m_pTextureVideo);
	return S_OK;
};

HRESULT cTextureMemory::FinalCleanup()
{
	RELEASE(m_pTextureVideo);
	RELEASE(m_pTextureMemory);
	return S_OK;
};	

//////////////////////////////////////////////////////////////////////
// texture
//////////////////////////////////////////////////////////////////////
HRESULT cTextureMemory::Create(const stResourceA& st, int width, int height)
{
	m_stResA = st;
	if (m_stResA.stRes.idResource == 0)
		m_used.SetName(m_stResA.stRes.szResource);
		
	HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(
		width,height,1,0,m_stResA.stDevice.format,D3DPOOL_SYSTEMMEM, &(m_pTextureMemory));
	GetGraphics()->DXError(hr);
	
	return S_OK;
}

HRESULT cTextureMemory::CreateTextureVideo() 
{
	ASSERT(m_pTextureMemory != NULL);
	ASSERT(m_pTextureVideo == NULL);
	
	D3DSURFACE_DESC desc;
	m_pTextureMemory->GetLevelDesc(0,&desc);

	HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(desc.Width,desc.Height,
		1,0,desc.Format,D3DPOOL_DEFAULT, &(m_pTextureVideo));
	if (FAILED(hr))
	{
		GetGraphics()->DXError(hr);
		return hr;
	}
	hr = m_pTextureVideo->AddDirtyRect(NULL);
	ASSERT(!FAILED(hr));	
	hr = m_pTextureMemory->AddDirtyRect(NULL);
	ASSERT(!FAILED(hr));	
	hr = GetGraphics()->GetDevice3D()->UpdateTexture(m_pTextureMemory,m_pTextureVideo);
	ASSERT(!FAILED(hr));	

/*
	hr = D3DXSaveTextureToFile("d:\\texture.bmp", D3DXIFF_BMP ,m_pTextureMemory,NULL);
	ASSERT(!FAILED(hr));	
*/	
	return S_OK;
}

IDirect3DTexture8* cTextureMemory::GetD3DTexture(int frame )
{
	//enter 
	HRESULT hr = 0;
	ASSERT(frame == 0);
	if (m_pTextureVideo != NULL)
	{
		Used();
		return m_pTextureVideo;
	}
	else if (m_pTextureMemory != NULL)
	{
		hr = CreateTextureVideo();
		if (FAILED(hr))
			return NULL;
		ResetUsed();
		InfoTextureCacheUpload(MEDIA_VIDEO,GetSize(MEDIA_VIDEO));
	}
	else 
	{
		ASSERT(0);
	}
	return m_pTextureVideo;
}

HRESULT cTextureMemory::Lock(int frame, D3DLOCKED_RECT& lr)
{
	ASSERT(frame == 0);
	HRESULT hr = m_pTextureMemory->LockRect(0,&lr,0,0);
	GetGraphics()->DXError(hr);
	return hr;
};

HRESULT cTextureMemory::Unlock(int frame)
{
	HRESULT hr = m_pTextureMemory->UnlockRect(0);
	GetGraphics()->DXError(hr);
	return hr;
}

HRESULT cTextureMemory::Update(RECT* pDirtyRect)
{
	ASSERT(m_pTextureMemory);
	if (m_pTextureVideo == NULL)
		return S_OK;
	HRESULT hr = m_pTextureVideo->AddDirtyRect(pDirtyRect);
	ASSERT(!FAILED(hr));	
	hr = m_pTextureMemory->AddDirtyRect(pDirtyRect);
	ASSERT(!FAILED(hr));
	if (m_pTextureVideo)
	{
		hr = GetGraphics()->GetDevice3D()->UpdateTexture(m_pTextureMemory,m_pTextureVideo);
		ASSERT(!FAILED(hr));	
	}
	return hr;
}

///////////////////////////////////////////////////////////////
//utility
///////////////////////////////////////////////////////////////
void cTextureMemory::Clear()
{
	ULONG width,height;
	GetTextureSize(0,width,height);
	D3DLOCKED_RECT lr;
	Lock(0,lr);
	char* buffer = (char*)lr.pBits;
	for (int y=0; y<height; y++)
	{
		ZeroMemory(buffer,width*GetTextureBytes());
		buffer += lr.Pitch;
	}
	Unlock(0);
}

void cTextureMemory::ClearRect(RECT& rc)
{
	D3DLOCKED_RECT lr;
	Lock(0,lr);
	ULONG width = rc.right - rc.left;
	ULONG height = rc.bottom - rc.top;
	char* buffer = (char*)lr.pBits + rc.top * lr.Pitch + rc.left * GetTextureBytes();
	for (int y=0; y<height; y++)
	{
		ZeroMemory(buffer,width*GetTextureBytes());
		buffer += lr.Pitch;
	}
	Unlock(0);
}

HRESULT cTextureMemory::ChangeFormat(D3DFORMAT format)
{
	ULONG width,height;
	GetTextureSize(0,width,height);
	m_stResA.stDevice.format = format;
	RELEASE(m_pTextureVideo);
	RELEASE(m_pTextureMemory);
	HRESULT hr = GetGraphics()->GetDevice3D()->CreateTexture(
		width,height,1,0,m_stResA.stDevice.format,D3DPOOL_SYSTEMMEM, &(m_pTextureMemory));
	GetGraphics()->DXError(hr);
	return hr;
}
