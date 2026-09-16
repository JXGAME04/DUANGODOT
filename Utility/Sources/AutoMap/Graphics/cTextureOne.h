// cTextureOne.h: interface for the cTextureOne class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTUREONE_H__81014B0E_28F2_4EBF_A20C_BBE9CFF2CC9C__INCLUDED_)
#define AFX_CTEXTUREONE_H__81014B0E_28F2_4EBF_A20C_BBE9CFF2CC9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

struct stFrame
{
	int nFrame;
	CRect rect;
};

class cTextureOne : public cTexture  
{
public:
	cTextureOne(cGraphics* pGraphics);
	virtual ~cTextureOne();

	stD3DTextureInfo* m_pInfo;
	bool IsResource(const stResourceA* st);
//load
	bool m_bUpload;
	bool IsUpload(MediaEnum type);
	long Upload(MediaEnum type);
	ULONG Download(MediaEnum type);
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
	ULONG m_size;
protected:
	stD3DTextureInfo* CreateD3DTextureInfo(int frame);
public:
	IDirect3DTexture8* GetD3DTexture(int frame);
	
//device
    HRESULT FinalCleanup();	

//create
//	HRESULT Create(const stResourceA& st, int width, int height);
	virtual HRESULT Unlock(int frame);
	virtual HRESULT Lock(int frame,D3DLOCKED_RECT& lr);
//render
	HRESULT	Refresh();
};

#endif // !defined(AFX_CTEXTUREONE_H__81014B0E_28F2_4EBF_A20C_BBE9CFF2CC9C__INCLUDED_)
