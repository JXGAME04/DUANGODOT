// cTextureVideoDirection.h: interface for the cTextureVideoDirection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTEXTUREVIDEODIRECTION_H__D5E3901E_C62F_4832_9F25_FA23CFAC0798__INCLUDED_)
#define AFX_CTEXTUREVIDEODIRECTION_H__D5E3901E_C62F_4832_9F25_FA23CFAC0798__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cTextureVideo.h"

class cTextureVideoDirection : public cTextureVideo  
{
public:
	cTextureVideoDirection(cGraphics* p);
	virtual ~cTextureVideoDirection();

	int m_nDirection;
	IDirect3DTexture8* GetD3DTexture(int frame);
	void GetImageSize(int frame,ULONG& width, ULONG& height);
	void GetTextureSize(int frame,ULONG& width, ULONG& height);
};

#endif // !defined(AFX_CTEXTUREVIDEODIRECTION_H__D5E3901E_C62F_4832_9F25_FA23CFAC0798__INCLUDED_)
