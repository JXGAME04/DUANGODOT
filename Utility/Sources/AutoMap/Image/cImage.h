// cImage.h: interface for the cImage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMAGE_H__F6CCDD3D_0AB0_4476_884A_7F12E34E908D__INCLUDED_)
#define AFX_CIMAGE_H__F6CCDD3D_0AB0_4476_884A_7F12E34E908D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iImage.h"

class cImage : public iImage  
{
public:
	cImage();
	virtual ~cImage();

//com
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;

	virtual BOOL	DivideTo(UINT wDivide, UINT hDivide) {ASSERT(0);return false;};
	virtual HRESULT	SetCenter(int nFrame,int x,int y) {return 0;};
	virtual BOOL	DivideTo(LPCSTR szFile,stDivide& divide) {ASSERT(0);return 0;};
	virtual HRESULT	GetAlphaBottom(int nFrame, int&x, int& y) {return 0;};
	virtual HRESULT	AlphaFormat() {return 0;};
	virtual HRESULT	LoadFileInfo(const char *filename) {return 0;};

	virtual HRESULT		GetCenterToUpleft(int nFrame,int& x,int& y) {return 0;};
	virtual HRESULT		SetCenterToUpleft(int nFrame,int x,int y) {return 0;};
	virtual BOOL		DivideTo(LPCSTR szFile, UINT wDivide, UINT hDivide){return 0;}
	virtual ULONG		SetAlpha1() {return 0;};

	virtual CBitmap*	ImageAlphaToBitmap(int nFrame,int nDepth ){return 0;}
	virtual HRESULT		CreateGrid(LPCSTR szFile,int width,int height,int wTile, int hTile) {return S_OK;};
	virtual HRESULT		CreateFile(const char *filename,  const stImageInfo& info) {return 0;};
	virtual HRESULT		MakeSmallestImage(BOOL bRemoveShadow) {return S_OK;};
	virtual BOOL		IsAlphaBad() {return FALSE;};
	virtual HRESULT		RemoveShadow(){return 0;};
};

#endif // !defined(AFX_CIMAGE_H__F6CCDD3D_0AB0_4476_884A_7F12E34E908D__INCLUDED_)
