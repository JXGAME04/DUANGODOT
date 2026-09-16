// cTexture.cpp: implementation of the cTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cTexture.h"

#include "cGraphics.h"
#include "cTextureCache.h"

#include "cUsed.h"

#include "ctimer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
ULONG stD3DTextureInfo::Download()
{
	if (pTexture == NULL) 
		return 0; 
	cTimerFunction timer("stD3DTextureInfo::Download!",5);
	ULONG n = pTexture->Release(); 
	pTexture = NULL; 
	return n;
}


cTexture::cTexture(cGraphics* p) : m_used(p)
{
	m_pGraphics = p;
	m_ulCount = 1;
	m_bInvalid = true;
//	m_pUsed = NULL;
//	m_pUsed = new cUsed(p);
	m_bDirty = false;
	m_pTextureCache = NULL;
}

cTexture::~cTexture()
{
//	if(m_pUsed)
//		delete m_pUsed;
}

//////////////////////////////////////////////////////////////////////
// COM
//////////////////////////////////////////////////////////////////////

ULONG cTexture::AddRef() 
{
	return ++m_ulCount;
}

//should delete it in cache!
ULONG cTexture::Release() 
{
	ULONG count = --m_ulCount;
	if (!count)
		delete this;
	return count;
}

ULONG cTexture::GetCount()
{
	return m_ulCount;
}

//////////////////////////////////////////////////////////////////////
// res
//////////////////////////////////////////////////////////////////////

bool cTexture::IsResource(const stResourceA* st)
{
	if (!IsValid())
		return false;
	return m_stResA == (*st);
}

HRESULT cTexture::SetResource(const stResourceA* st)
{
	m_stResA = (*st); 
	if (m_stResA.stRes.idResource == 0)
		m_used.SetName(m_stResA.stRes.szResource);
	return S_OK;
}

UsageEnum cTexture::GetUsage()
{
	return m_stResA.stDevice.eUsage;
}

D3DFORMAT cTexture::GetResD3DFormat()
{
	return m_stResA.stDevice.format;
}

D3DPOOL cTexture::GetResD3DPool()
{
	return m_stResA.stDevice.pool;
}

DWORD cTexture::GetResD3DUsage()
{
	return m_stResA.stDevice.usage;
}

//////////////////////////////////////////////////////////////////////
// use
//////////////////////////////////////////////////////////////////////
void cTexture::ResetUsed()
{
	m_used.Reset();
}

void cTexture::Used()
{
	m_used.Used();
}

UsedEnum cTexture::GetUsed()
{
	return m_used.GetUsed();
}

HRESULT cTexture::InfoTextureCacheUpload(MediaEnum type,ULONG size)
{
	ASSERT(m_pTextureCache);
	m_pTextureCache->ChildUpload(type,size);
	return S_OK;
}

ULONG Get2Mi(ULONG num)
{
	ASSERT(num != 0);
	ULONG old = num;
	ULONG mi = 0;
	int n = 1;
	while(num > 1)
	{
		num >>= 1;
		n <<= 1;
		mi++;
	}
	if (n == old)
		return n;
	return n << 1;
}

ULONG GetPixelBytes(D3DFORMAT format)
{
	switch (format)
	{
	case D3DFMT_R5G6B5:
	case D3DFMT_A4R4G4B4:
		return sizeof(WORD);
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		return sizeof(DWORD);
	}
	return -1;
}

BOOL cTexture::SetInvalid()
{
	m_bInvalid = false;
	return TRUE;
}

BOOL cTexture::IsValid()
{
	return m_bInvalid;
}

void cTexture::Trace()
{
	int size[4];
	for (int i=0; i<MEDIA_NUM; i++)
	{
		size[i] = GetSize((MediaEnum)i);
	}
	CString sMessage;
	sMessage.Format("file %s(%d), used %d,%d,%d,%d\n",m_stResA.stRes.szResource,m_stResA.stRes.idResource,
		size[0],size[1],size[2],size[3]);
	OutputDebugString(sMessage);
}

