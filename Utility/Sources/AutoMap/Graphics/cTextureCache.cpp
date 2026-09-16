// cTextureCache.cpp: implementation of the cTextureCache class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Graphics.h"
#include "cTextureCache.h"

#include "ctimer.h"

#include "cUsed.h"
#include "cGraphics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cTextureCache::cTextureCache()
{
	for (int i=0; i<MEDIA_NUM; i++)
	{
		m_size[i].nTotalSize = 0;
		m_size[i].nMaxCount = 100;
		m_size[i].nCurrentSize = 0;
	}
	m_bDownloadFrames = FALSE;
}

cTextureCache::~cTextureCache()
{
	if (m_mapTexture.size() > 0)
	{
		OutputDebugString("\nwarning the cache not clear !\n\n");
	}
}

void cTextureCache::Create(cGraphics* pGraphics, UsageEnum texType)
{
	m_pGraphics = pGraphics;
	m_eUsage = texType;
}

iTexture* cTextureCache::FindTexture(const stResourceA& resPos, bool bAutoCreate)
{
	MediaEnum type = resPos.stDevice.eMedia;
	ASSERT(type > MEDIA_NULL && type < MEDIA_NUM);

	ASSERT(m_size[type].nTotalSize != 0);
	stTexture* pTex = FindTexture(resPos);
	
	iTexture* pTexture;
	if (pTex != NULL)
	{
		pTexture = pTex->aTexture[type];
		if (NULL != pTexture)
		{
			if (!pTexture->IsUpload(type))
			{
				ULONG sz = pTexture->Upload(type);
				IncreaseSize(type,sz);
			}
			pTexture->AddRef();
		}
		else if (bAutoCreate)
		{
			pTexture = CreateTexture(type);
			if (FAILED(AddTexture(resPos,pTexture)))
			{
				pTexture->Release();
				return NULL;
			}

			pTexture->AddRef();
			pTex->aTexture[type] = pTexture;
		}
		return pTexture;
	}
	else if (bAutoCreate)
	{
		pTexture = CreateTexture(type);
		if (FAILED(AddTexture(resPos,pTexture)))
		{
			pTexture->Release();
			return NULL;
		}

		pTexture->AddRef();
		stTexture* pTex = new stTexture;
		pTex->resPos = resPos;
		pTex->aTexture[resPos.stDevice.eMedia] = pTexture;
		
		m_mapTexture[resPos.stRes.szResource] = pTex;

		return pTexture;
	}
	return NULL;
}

stTexture* cTextureCache::FindTexture(const stResourceA& resPos)
{
	int media = resPos.stDevice.eMedia;
	ASSERT(media > MEDIA_NULL && media < MEDIA_NUM);
	itTexture it = m_mapTexture.find(resPos.stRes.szResource);
	if (it == m_mapTexture.end())
		return NULL;
	return (*it).second;
}

iTexture* cTextureCache::CreateTexture(MediaEnum eMedia)
{
	iTexture* p;
	ElementEnum type;
	switch (eMedia)
	{
	case MEDIA_SPR:
		type = GT_TEXTURESPR;
		break;
	case MEDIA_MEMORY:
		type = GT_TEXTUREMEMORY;
		break;
	case MEDIA_VIDEOOTHER:
		type = GT_TEXTUREVIDEOOTHER;
		break;
	case MEDIA_VIDEO:
		type = GT_TEXTUREVIDEO;
		break;
	case MEDIA_VIDEOONE:
		type = GT_TEXTUREONE;
		break;
	}
	GetGraphics()->CreateGraphicsElement(type,&p);
	return p;
}

HRESULT cTextureCache::ChildUpload(MediaEnum type,ULONG size)
{
	IncreaseSize(type,size);
	return S_OK;
}

//#define _DEBUG_cTextureCache

void cTextureCache::IncreaseSize(MediaEnum e, int size)
{
	if (size == 0)
		return;

	m_size[e].nCurrentSize += size;

#ifdef _DEBUG_cTextureCache
	static CString aDevice[] = 
	{
		"Video","Memory","Spr"
	};
	float f1 = size/1024.f/1024.f;
	float f2 = m_size[e].nCurrentSize/1024.f/1024.f;
	CString sMsg;
	if (size > 0)
		sMsg.Format("Cache %s : %s Size ++++ increase %.2fM to %.2fM\n",
		m_strName, aDevice[e], f1,f2 );
	else 
		sMsg.Format("Cache %s : %s Size ---- decrease %.2fM to %.2fM\n",
		m_strName, aDevice[e], -f1,f2);
	OutputDebugString(sMsg);
#endif
}

void cTextureCache::DecreaseSize(MediaEnum e, int size)
{
	IncreaseSize(e,-size);
}

//不是从findtexture得到
HRESULT cTextureCache::AddTextureOut(const stResourceA& resPos,iTexture* pTexture)
{
	for (int i=0; i<MEDIA_NUM; i++)
	{
		int size = pTexture->GetSize((MediaEnum)i);
		IncreaseSize((MediaEnum)i,size);
	}
	pTexture->AddRef();
	pTexture->SetTextureCache(this);
	stTexture* pTex = new stTexture;
	pTex->resPos = resPos;
	pTex->aTexture[resPos.stDevice.eMedia] = pTexture;
//	m_mapTexture.push_back(pTex);
	
	m_mapTexture[resPos.stRes.szResource] = pTex;
	return S_OK;
}

HRESULT cTextureCache::AddTexture(const stResourceA& resPos,iTexture* pTexture)
{
	pTexture->SetResource(&resPos);
	long sz = pTexture->Upload(resPos.stDevice.eMedia);
	if (sz < 0)
	{
		return -1;
	}
	for (int i=0; i<MEDIA_NUM; i++)
	{
		int size = pTexture->GetSize((MediaEnum)i);
		IncreaseSize((MediaEnum)i,size);
	}
	pTexture->SetTextureCache(this);
	return S_OK;
}

HRESULT cTextureCache::Run()
{
//	if (m_eUsage == USAGE_STATIC)
//		Trace();
	
	for (int i=0; i<MEDIA_NUM; i++)
	{
		if (m_size[i].nCurrentSize > m_size[i].nTotalSize)
		{
			if (m_size[i].nCurrentSize - m_size[i].nTotalSize > 0)
				FreeTexture((MediaEnum)i, m_size[i].nCurrentSize - m_size[i].nTotalSize);
		}
	}
	int n = m_mapTexture.size();
	return S_OK;
}

BOOL cTextureCache::FreeTexture(MediaEnum e, ULONG size)
{
	cTimerFunction timer("cTextureCache::FreeTexture");
	int freeSize = 0;

	itTexture it;
	int u;
	for (u = USED_NEVER; u>= USED_OFTEN; u--)
	{
		for (it = m_mapTexture.begin(); it!= m_mapTexture.end(); ++it)
		{
			bool bErase = false;
			stTexture* pTex = (*it).second;
			for (int i=0; i<MEDIA_NUM; i++)
			{
				iTexture* pTexture = pTex->aTexture[i];
				if (pTexture == NULL)
					continue;
				UsedEnum use = pTexture->GetUsed();
				if (use != u)
					continue;

				if (pTexture->GetCount() == 1)
				{
					int sz = pTexture->GetSize(e);
					for (int im = 0; im < MEDIA_NUM; im++)
					{
						MediaEnum m =(MediaEnum ) im;
						if (pTexture->IsUpload(m))
						{
							int size = pTexture->GetSize(m);
							pTexture->Download(m);
							IncreaseSize(m,-size);
						}
					}
					pTexture->Release();
					pTex->aTexture[i] = NULL;
		
					bErase = true;

					freeSize += sz;
//					if (freeSize >= size)
//						break;
				}
			}
			bool bNull = true;
			for (int j=0; j<MEDIA_NUM; j++)
			{
				if (pTex->aTexture[j] != NULL)
					bNull = false;
			}

			if (bNull)
			{
				delete pTex;
				m_mapTexture.erase(it);
				return TRUE;
			}
//			else //can not release here!
//				++it;//do not use it++ !
		}
		if (freeSize >= size)
			return TRUE;
	}
	if (!m_bDownloadFrames)
		return FALSE;
/*
	for (u = USED_NEVER; u>= USED_OFTEN; u--)
	{
		for (it = m_mapTexture.begin(); it!= m_mapTexture.end(); ++it)
		{
			bool bErase = false;
			stTexture* pTex = (*it).second;
			for (int i=0; i<MEDIA_NUM; i++)
			{
				iTexture* pTexture = pTex->aTexture[i];
				if (pTexture == NULL)
					continue;
				UsedEnum use = pTexture->GetUsed();
				if (use != u)
					continue;

				int sz = 0;
				for (int im = 0; im < MEDIA_NUM; im++)
				{
					MediaEnum m =(MediaEnum ) im;
					if (pTexture->IsUpload(m))
					{
						int size = pTexture->DownloadSomeFrame(m);
						if (e == m)
							sz = size;
						IncreaseSize(m,-size);
					}
				}
				freeSize += sz;
//				if (freeSize >= size)
//					return TRUE;
			}
		}
		if (freeSize >= size)
			return TRUE;
	}
*/
	return FALSE;
}

HRESULT cTextureCache::InitDeviceObjects()
{
	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
				pTexture->InitDeviceObjects();
		}
	}
	return S_OK;
}

HRESULT cTextureCache::RestoreDeviceObjects()
{
	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
				pTexture->RestoreDeviceObjects();
		}
	}
	return S_OK;
}

HRESULT cTextureCache::InvalidateDeviceObjects()
{
//	for_all(m_mapTexture, mem_fun<HRESULT, iTexture> ( &iTexture::InvalidateDeviceObjects) );
	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
				pTexture->InvalidateDeviceObjects();
		}
	}
	return S_OK;
}

HRESULT cTextureCache::DeleteDeviceObjects()
{
//	for_all(m_mapTexture, mem_fun<HRESULT, iTexture> ( &iTexture::DeleteDeviceObjects) );
	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
				pTexture->DeleteDeviceObjects();
		}
	}
	return S_OK;
}

HRESULT cTextureCache::FinalCleanup()
{
//	for_all(m_mapTexture, mem_fun<HRESULT, iTexture> ( &iTexture::FinalCleanup) );
//	for_all(m_mapTexture, mem_fun<ULONG, iTexture> ( &iTexture::Release) );
	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
			{
				pTexture->FinalCleanup();
				pTexture->Release();
			}
		}
		delete pTex;
	}
	m_mapTexture.clear();
	for (int i=0; i<MEDIA_NUM; i++)
	{
		m_size[i].nCurrentSize = 0;
	}
	return S_OK;
}

void cTextureCache::Trace()
{
	CString sTrace;
	sTrace.Format("cache usage %d trace begin \n",m_eUsage);
//	OutputDebugString(sTrace);

	int size[MEDIA_NUM] = {0,0,0,0};

	for (itTexture it  = m_mapTexture.begin(); it != m_mapTexture.end(); ++it)
	{
		stTexture* pTex = (*it).second;
		for (int i = 0; i< MEDIA_NUM; i++)
		{
			iTexture* pTexture = pTex->aTexture[i];
			if (pTexture)
			{
//				pTexture->Trace();
				for (int i = 0; i< MEDIA_NUM; i++)
				{
					size[i] += pTexture->GetSize((MediaEnum)i);
				}
			}
		}
	}

	sTrace.Format("size(%d/%d/%d) (%d/%d/%d) (%d/%d/%d) (%d/%d/%d)\n",
		size[0]>>20,m_size[0].nCurrentSize>>20,m_size[0].nTotalSize>>20,
		size[1]>>20,m_size[1].nCurrentSize>>20,m_size[1].nTotalSize>>20,
		size[2]>>20,m_size[2].nCurrentSize>>20,m_size[2].nTotalSize>>20,
		size[3]>>20,m_size[3].nCurrentSize>>20,m_size[3].nTotalSize>>20
		);

	for (int i=0; i<MEDIA_NUM; i++)
	{
		if  (fabs(m_size[i].nCurrentSize - size[i]) > 0) 
		{
			ASSERT(0);
		}
	}
	OutputDebugString(sTrace);
	sTrace.Format("cache usage %d trace end \n",m_eUsage);
//	OutputDebugString(sTrace);
}
