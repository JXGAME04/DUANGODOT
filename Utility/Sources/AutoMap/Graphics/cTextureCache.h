// cTextureCache.h: interface for the cTextureCache class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTURECACHE_H__ED1E8826_1389_4B94_AE1D_306B6D820412__INCLUDED_)
#define AFX_CTEXTURECACHE_H__ED1E8826_1389_4B94_AE1D_306B6D820412__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
class iTexture;

struct stTexture
{
	stResourceA resPos;
	iTexture* aTexture[MEDIA_NUM];
	stTexture(){for (int i=0; i<MEDIA_NUM; i++) aTexture[i] = NULL;}
	bool IsResource(const stResourceA* pos){return resPos == *pos;}
};

//typedef std::list<stTexture*> TextureList;
//typedef TextureList::iterator itTexture;

#include <hash_map>
typedef std::hash_map<std::string, stTexture*, std::hash<std::string>, std::equal_to<std::string> > TextureMap;
typedef TextureMap::iterator itTexture;

class cTextureCache  
{
private:
	TextureMap m_mapTexture;
	BOOL m_bDownloadFrames;
public:
	void SetDownloadFrame(BOOL b){m_bDownloadFrames = b;}

public:
	cTextureCache();
	virtual ~cTextureCache();

	//for debug!
	CString m_strName;
	void SetName(LPCSTR szName){m_strName = szName;}

	cGraphics* m_pGraphics;
	UsageEnum m_eUsage;
	void Create(cGraphics* pGraphics, UsageEnum texType);
	cGraphics* GetGraphics(){return m_pGraphics;}

	struct stSize
	{
		int nMaxCount;
		int nTotalSize;
		int nCurrentSize;
	};

	stSize m_size[MEDIA_NUM];
	void IncreaseSize(MediaEnum e, int size);
	void DecreaseSize(MediaEnum e, int size);

	void SetTotalSize(MediaEnum type,ULONG size){m_size[type].nTotalSize = size;};
	ULONG GetTotalSize(MediaEnum type){return m_size[type].nTotalSize;}
	ULONG GetCurrentSize(MediaEnum type){return m_size[type].nCurrentSize;}
	void SetMaxCount(MediaEnum type,int count){m_size[type].nMaxCount = count;}

	iTexture* FindTexture(const stResourceA& resPos, bool bAutoCreate);
	stTexture* FindTexture(const stResourceA& resPos);
	HRESULT AddTexture(const stResourceA& resPos,iTexture* ptr);
	HRESULT AddTextureOut(const stResourceA& resPos,iTexture* pTexture);
	BOOL	FreeTexture(MediaEnum e,ULONG size);
	HRESULT ChildUpload(MediaEnum type,ULONG size);

	iTexture* CreateTexture(MediaEnum type);

    virtual HRESULT InitDeviceObjects();                        
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT FinalCleanup();

	virtual HRESULT Run();
	virtual void	Trace();
};

#endif // !defined(AFX_CTEXTURECACHE_H__ED1E8826_1389_4B94_AE1D_306B6D820412__INCLUDED_)
