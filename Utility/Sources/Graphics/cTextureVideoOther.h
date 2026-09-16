// cTextureVideoOther.h: interface for the cTextureVideoOther class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTUREVIDEOOTHER_H__E2F18C9F_A871_4030_A755_9AF10996E242__INCLUDED_)
#define AFX_CTEXTUREVIDEOOTHER_H__E2F18C9F_A871_4030_A755_9AF10996E242__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTexture.h"

class cTextureVideoOther : public cTexture  
{
public:
	cTextureVideoOther(cGraphics* p);
	virtual ~cTextureVideoOther();

    HRESULT			FinalCleanup() ;	

	long			Upload(MediaEnum type) ;
	bool			IsUpload(MediaEnum type) ;
	ULONG			Download(MediaEnum type) ;
	ULONG			GetSize(MediaEnum type) ;

	void			GetTextureSize(int frame,ULONG& width, ULONG& height) ;
	void			GetImageSize(int frame,ULONG& width, ULONG& height) ;
	void			GetTextureRect(int frame, cRect& rc) ;
	
	D3DXIMAGE_INFO		m_info;
	IDirect3DTexture8* GetD3DTexture(int frame ) ;

	LPDIRECT3DTEXTURE8 m_pTextureVideo;
};

#endif // !defined(AFX_CTEXTUREVIDEOOTHER_H__E2F18C9F_A871_4030_A755_9AF10996E242__INCLUDED_)
