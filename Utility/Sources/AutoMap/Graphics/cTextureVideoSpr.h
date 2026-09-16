// cTextureVideoSprSpr.h: interface for the cTextureVideoSprSpr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTextureVideoSprSPR_H__46E4C6ED_9F2B_4C42_991B_120764F73E12__INCLUDED_)
#define AFX_CTextureVideoSprSPR_H__46E4C6ED_9F2B_4C42_991B_120764F73E12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

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
	bool IsFrame(int frm);
	ULONG Download(){if (pTexture == NULL) return 0; ULONG n = pTexture->Release(); pTexture = NULL; return n;}
	ULONG GetSize(){return wTexture*hTexture*byte_per_pixel;}
	ULONG AccumulateSize(ULONG* sz){*sz += GetSize(); return *sz;}
	ULONG Release(){delete this; return 0;}
};

typedef vector<stD3DTextureInfo*> D3DTextureInfoList;
typedef D3DTextureInfoList::iterator itD3DTextureInfo;

class cTextureVideoSpr : public cTexture  
{
public:
	cTextureVideoSpr(cGraphics* pGraphics);
	virtual ~cTextureVideoSpr();
	
//load
	bool m_bUpload;
	bool IsUpload(MediaEnum type);
	ULONG Upload(MediaEnum type);
	ULONG Download(MediaEnum type);
	ULONG GetSize(MediaEnum type);
	void GetTextureSpr();

//picture info
	void GetImageSize(int frame,ULONG& width, ULONG& height);
	void GetTextureSize(int frame,ULONG& width, ULONG& height);
	void GetTextureRect(int frame,cRect& rc);
	void GetCenterToUpleft(int frame, int& x, int& y);
	ULONG GetFrames();
	ULONG GetDirections();

	void Used();

	iTexture* m_pTexture;

//IDirect3DTexture8
	D3DTextureInfoList m_listInfo;
	ULONG m_size;
protected:
	stD3DTextureInfo* CreateD3DTextureInfo(int frame);
public:
	IDirect3DTexture8* GetD3DTexture(int frame);
	
//device
	HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();                     
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();	

//create
	HRESULT Create(const stResourceA& st, int width, int height);
	virtual HRESULT Unlock(int frame);
	virtual HRESULT Lock(int frame,D3DLOCKED_RECT& lr);
//render
	virtual HRESULT Render(stRender& render) ;
	HRESULT	Refresh();
};

#endif // !defined(AFX_CTextureVideoSprSPR_H__46E4C6ED_9F2B_4C42_991B_120764F73E12__INCLUDED_)
