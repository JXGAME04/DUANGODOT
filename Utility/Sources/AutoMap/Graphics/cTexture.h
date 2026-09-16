// cTexture.h: interface for the cTexture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTURE_H__48A29536_B5B8_4316_9CAC_473CE3593799__INCLUDED_)
#define AFX_CTEXTURE_H__48A29536_B5B8_4316_9CAC_473CE3593799__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

struct stD3DTextureInfo
{
	LPDIRECT3DTEXTURE8 pTexture;
	int frame;
	D3DFORMAT format;
	int byte_per_pixel;
	ULONG x,y;
//	ULONG width,height;
	ULONG wImage,hImage;
	ULONG wTexture,hTexture;
	bool IsFrame(int frm){return frame == frm;}
	ULONG Download();
	ULONG GetSize(){return wTexture*hTexture*byte_per_pixel;}
	ULONG AccumulateSize(ULONG* sz){*sz += GetSize(); return *sz;}
	ULONG Release(){delete this; return 0;}
};
typedef std::vector<stD3DTextureInfo*> D3DTextureInfoList;
typedef D3DTextureInfoList::iterator itD3DTextureInfo;

#include "cUsed.h"

class cTextureCache;

class cTexture : public iTexture
{
public:
	cTexture(cGraphics*);
	virtual ~cTexture();

	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}

	cTextureCache* m_pTextureCache;
	cTextureCache* GetTextureCache(){return m_pTextureCache;}
	void SetTextureCache(cTextureCache* p){m_pTextureCache = p;}
	HRESULT InfoTextureCacheUpload(MediaEnum type,ULONG size);

//com
	ULONG m_ulCount;
	ULONG AddRef();
	ULONG Release();
	ULONG GetCount();

//res and use
	stResourceA m_stResA;
	virtual bool	IsResource(const stResourceA* st);
	virtual HRESULT SetResource(const stResourceA* st);
	void	GetResource(stResourceA& st){st = m_stResA;};
	
	UsageEnum	GetUsage();
	D3DFORMAT	GetResD3DFormat();
	D3DPOOL		GetResD3DPool();
	DWORD		GetResD3DUsage();
//used
	cUsed m_used;
	virtual void Used();
	virtual UsedEnum GetUsed();
	virtual void ResetUsed();

//invalid
	BOOL  m_bInvalid;
	BOOL  SetInvalid();
	BOOL  IsValid();
//dirty
	BOOL  m_bDirty;
	void  SetDirty(BOOL b){m_bDirty = b;};
	BOOL  IsDirty(){return m_bDirty;}
	
	//每个像素多少bytes
	long	GetTextureBytes(){return 2;}

//大多数的接口不是通用的
	HRESULT Update(RECT* pDirtyRect) {return S_OK;}
	HRESULT	Refresh() {return S_OK;}

	void	Clear(){};
	void	ClearRect(RECT& rc){};	
	HRESULT ChangeFormat(D3DFORMAT format) {return-1;};

	HRESULT Create(const stResourceA& st, int width, int height){return -1;};
	HRESULT Render(stRender& render){return -1;};
	HRESULT Lock(int frame,D3DLOCKED_RECT& lr){return -1;};
	HRESULT Unlock(int frame){return -1;};

	HRESULT InitDeviceObjects(){return S_OK;}
    HRESULT RestoreDeviceObjects(){return S_OK;}                   
    HRESULT InvalidateDeviceObjects(){return S_OK;}
    HRESULT DeleteDeviceObjects(){return S_OK;}
    HRESULT FinalCleanup(){return S_OK;}

	ULONG	GetFrames() {return 1;}
	ULONG	GetDirections() {return 1;}
	void	GetCenterToUpleft(int frame,int& x, int& y){x = 0; y = 0;};
	ULONG	DownloadSomeFrame(MediaEnum type) {return 0;};

	void	Trace();
};

ULONG GetPixelBytes(D3DFORMAT format);
ULONG Get2Mi(ULONG num);

//IDirect3DTexture8* Get3DTexture(cTexture* pTexture, int frame);

#endif // !defined(AFX_CTEXTURE_H__48A29536_B5B8_4316_9CAC_473CE3593799__INCLUDED_)
