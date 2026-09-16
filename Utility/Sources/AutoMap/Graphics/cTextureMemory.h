// cTextureMemory.h: interface for the cTextureMemory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTUREMEMORY_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_)
#define AFX_CTEXTUREMEMORY_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

//texture memory 用于临时生成的贴图
//例如组合的地图，显示文字等
//所以，一张贴图对应一个资源

class cTextureMemory : public cTexture  
{
public:
	cTextureMemory(cGraphics* pGraphics);
	~cTextureMemory();

	virtual ULONG AddRef() ;
	virtual ULONG Release() ;
//load
	bool IsUpload(MediaEnum type);
	long Upload(MediaEnum type);
	ULONG Download(MediaEnum type);
	ULONG GetSize(MediaEnum type);

//picture info
	void GetImageSize(int frame,ULONG& width, ULONG& height);
	void GetTextureSize(int frame,ULONG& width, ULONG& height);
	void GetTextureRect(int frame,cRect& rc);

//render
	HRESULT Update(RECT* pDirtyRect);

//device
	HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();                     
	bool m_bInvalidateVideo;
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();	

//texture
	HRESULT Create(const stResourceA& st, int width, int height);
	LPDIRECT3DTEXTURE8 m_pTextureMemory;
	HRESULT CreateTextureVideo();
	LPDIRECT3DTEXTURE8 m_pTextureVideo;
	HRESULT Lock(int frame,D3DLOCKED_RECT& lr);
	HRESULT Unlock(int frame);

	virtual IDirect3DTexture8* GetD3DTexture(int frame ) ;
//utility
	virtual void	Clear();
	virtual void	ClearRect(RECT& rc);	
	virtual HRESULT ChangeFormat(D3DFORMAT format) ;
};

#endif // !defined(AFX_CTEXTUREMEMORY_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_)
