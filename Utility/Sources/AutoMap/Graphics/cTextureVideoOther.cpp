// cTextureVideoOther.cpp: implementation of the cTextureVideoOther class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cGraphics.h"
#include "GraphicsPublic.h"
#include "cTextureVideoOther.h"
#include "fileop.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureVideoOther::cTextureVideoOther(cGraphics* p) : cTexture(p)
{
	m_pTextureVideo = NULL;
}

cTextureVideoOther::~cTextureVideoOther()
{
	FinalCleanup();
}

//////////////////////////////////////////////////////////////////////
// upload and download
//////////////////////////////////////////////////////////////////////

bool cTextureVideoOther::IsUpload(MediaEnum type)
{
	switch (type)
	{
	case MEDIA_VIDEOOTHER:
		return m_pTextureVideo != NULL;
	}
	//ASSERT(0);
	return false;
}

long cTextureVideoOther::Upload(MediaEnum type)
{
	switch (type)
	{
	case MEDIA_VIDEOOTHER:
		{
			ASSERT(m_pTextureVideo == NULL);
			
			CString str = m_stResA.stRes.szResource;
			ASSERT(!str.IsEmpty());
			AppGetFullPath(str);
			HRESULT hr = D3DXCreateTextureFromFileEx( GetGraphics()->GetDevice3D(), str, 
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, m_stResA.stDevice.usage, m_stResA.stDevice.format, 
                m_stResA.stDevice.pool, D3DX_FILTER_NONE, D3DX_FILTER_NONE,
                0, &m_info, NULL, &m_pTextureVideo );
			//DX_ERROR(hr);
			if (FAILED(hr))
			{
				CString str = "system\\spr\\background.jpg";
				AppGetFullPath(str);
				HRESULT hr = D3DXCreateTextureFromFileEx( GetGraphics()->GetDevice3D(), str, 
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, m_stResA.stDevice.usage, m_stResA.stDevice.format, 
                m_stResA.stDevice.pool, D3DX_FILTER_NONE, D3DX_FILTER_NONE,
                0, &m_info, NULL, &m_pTextureVideo );
				DX_ERROR(hr);
			}
			return GetSize(type);
		}
	default:
		ASSERT(0);
		break;
	}
	return 0;
}

ULONG cTextureVideoOther::Download(MediaEnum type)
{
	int size = GetSize(type);
	switch (type)
	{
	case MEDIA_VIDEOOTHER:
		{
			ASSERT(m_pTextureVideo);
			RELEASE(m_pTextureVideo);
		}
		break;
	}
	return size;
}

ULONG cTextureVideoOther::GetSize(MediaEnum type)
{
	//return spr memory size
	switch (type)
	{
	case MEDIA_VIDEOOTHER:
		{
			if (m_pTextureVideo)
			{
				D3DSURFACE_DESC desc;
				m_pTextureVideo->GetLevelDesc(0,&desc);
				return desc.Size;
			}
			break;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// picture info
//////////////////////////////////////////////////////////////////////
void cTextureVideoOther::GetTextureRect(int frame,cRect& rc)
{
	ULONG w1,w2,h1,h2;
	GetImageSize(frame,w1,h1);
	GetTextureSize(frame,w2,h2);
	rc.l = 0;
	rc.t = 0;
	rc.r = (float)w1 / (float)w2;
	rc.b = (float)h1 / (float)h2;
}

void cTextureVideoOther::GetImageSize( int frame, ULONG& width, ULONG& height)
{
	width = m_info.Width;
	height = m_info.Height;
}

void cTextureVideoOther::GetTextureSize( int frame, ULONG& width, ULONG& height)
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
	else
		ASSERT(0);
}

//////////////////////////////////////////////////////////////////////
// device
//////////////////////////////////////////////////////////////////////
HRESULT cTextureVideoOther::FinalCleanup()
{
	RELEASE(m_pTextureVideo);
	return S_OK;
};	

IDirect3DTexture8* cTextureVideoOther::GetD3DTexture(int frame )
{
	if (frame == 0)
	{
		Used();
		return m_pTextureVideo;
	}
	return NULL;
}