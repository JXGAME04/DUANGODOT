// cTextureVideo.h: interface for the cTextureVideo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTUREVIDEO_H__AA7F9DE9_F1F2_42B1_81C4_326BD3C43DEE__INCLUDED_)
#define AFX_CTEXTUREVIDEO_H__AA7F9DE9_F1F2_42B1_81C4_326BD3C43DEE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

class cTextureVideo : public cTexture  
{
public:
	cTextureVideo(cGraphics* pGraphics);
	virtual ~cTextureVideo();
	
	bool IsResource(const stResourceA* st);
	MediaEnum m_eMedia;
//load
	bool m_bUpload;
	bool IsUpload(MediaEnum type);
	long Upload(MediaEnum type);
	ULONG Download(MediaEnum type);
	ULONG DownloadSomeFrame(MediaEnum type);
	ULONG GetSize(MediaEnum type);
	iTexture* GetTextureSpr();

//picture info
	void GetImageSize(int frame,ULONG& width, ULONG& height);
	void GetTextureSize(int frame,ULONG& width, ULONG& height);
	void GetTextureRect(int frame,cRect& rc);
	void GetCenterToUpleft(int frame, int& x, int& y);
	ULONG GetFrames();
	ULONG GetDirections();

//cTextureMemory
	void Used();

//	stResourceA m_stRes;
	iTexture* m_pTexture;

//IDirect3DTexture8
	D3DTextureInfoList m_listInfo;
	ULONG m_size;
protected:
	stD3DTextureInfo* CreateD3DTextureInfo(int frame);
public:
	virtual IDirect3DTexture8* GetD3DTexture(int frame);
	
//device
    HRESULT FinalCleanup();	

//create
//	HRESULT Create(const stResourceA& st, int width, int height);
	virtual HRESULT Unlock(int frame);
	virtual HRESULT Lock(int frame,D3DLOCKED_RECT& lr);
//render
	HRESULT	Refresh();
};

#endif // !defined(AFX_CTEXTUREVIDEO_H__AA7F9DE9_F1F2_42B1_81C4_326BD3C43DEE__INCLUDED_)
