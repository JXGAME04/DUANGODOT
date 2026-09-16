// cFont3D.h: interface for the cFont3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFONT3D_H__974E803F_04AB_4FF5_B4AD_B7F9E4F6B6B9__INCLUDED_)
#define AFX_CFONT3D_H__974E803F_04AB_4FF5_B4AD_B7F9E4F6B6B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cFont.h"

class cFont3D : public cFont  
{
public:
	cFont3D(cGraphics* );
	virtual ~cFont3D();

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
	//device 
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
	LPDIRECT3DDEVICE8 GetDevice();

	//create	
	CD3DFont* m_pFont;
	char m_strName[64];

	//create	
	virtual HRESULT Create(LPCSTR strFontName, DWORD dwHeight, DWORD dwFlags=0L) ;
	//draw
    HRESULT DrawText( const stParam& param, LPCSTR strText, int nLength) ;
};

#endif // !defined(AFX_CFONT3D_H__974E803F_04AB_4FF5_B4AD_B7F9E4F6B6B9__INCLUDED_)
