// cTextureVideoDirection.cpp: implementation of the cTextureVideoDirection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cTextureVideoDirection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureVideoDirection::cTextureVideoDirection(cGraphics* p) : cTextureVideo(p)
{
	m_nDirection = -1;
	m_eMedia = MEDIA_VIDEOONE;
}

cTextureVideoDirection::~cTextureVideoDirection()
{

}

IDirect3DTexture8* cTextureVideoDirection::GetD3DTexture(int frame)
{
	int nFrame = GetFrames();
	int nDir = GetDirections();
	int framedir = nFrame / nDir;
	int dir = frame / framedir;
	if (m_nDirection != dir)
	{
		ULONG sz = Download(MEDIA_VIDEOONE);
		InfoTextureCacheUpload(MEDIA_VIDEOONE,-sz);	
	}
	m_nDirection = dir;
	return cTextureVideo::GetD3DTexture(frame);
}

void cTextureVideoDirection::GetImageSize(int frame,ULONG& width, ULONG& height)
{
	if (m_listInfo[frame] != NULL)
	{
		width = m_listInfo[frame]->wImage;
		height = m_listInfo[frame]->hImage;
	}
	else 
	{
		ASSERT(m_pTexture);
		m_pTexture->GetImageSize(frame,width,height);
	}	
}

void cTextureVideoDirection::GetTextureSize(int frame,ULONG& width, ULONG& height)
{
	if (m_listInfo[frame] != NULL)
	{
		width = m_listInfo[frame]->wTexture;
		height = m_listInfo[frame]->hTexture;
	}
	else 
	{
		ASSERT(m_pTexture);
		m_pTexture->GetTextureSize(frame,width,height);
	}	
}