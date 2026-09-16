// cTextureSpr.h: interface for the cTextureSpr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTextureSpr_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_)
#define AFX_CTextureSpr_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

class iImage;
//texture memory 用于所有的游戏的资源

class cTextureSpr : public cTexture  
{
public:
	cTextureSpr(cGraphics* pGraphics);
	~cTextureSpr();

//load
	bool IsUpload(MediaEnum type);
	long Upload(MediaEnum type);
	ULONG Download(MediaEnum type);
	ULONG GetSize(MediaEnum type);

//picture info
	void GetImageSize(int frame,ULONG& width, ULONG& height);
	void GetTextureSize(int frame,ULONG& width, ULONG& height);
	void GetTextureRect(int frame,cRect& rc);
	void GetCenterToUpleft(int frame, int& x, int& y);
	ULONG GetFrames();
	ULONG GetDirections();

//Render
	HRESULT Render(stRender& render);

//device
    HRESULT FinalCleanup();	

//image info
	iImage* m_pImage;
	iImage* GetImage(){return m_pImage;}

	IDirect3DTexture8* GetD3DTexture(int frame ) {ASSERT(0);return NULL;};
	HRESULT	Refresh();
};

#endif // !defined(AFX_CTextureSpr_H__F1CD39FC_CCFD_423B_B25B_AC63D22C0907__INCLUDED_)
