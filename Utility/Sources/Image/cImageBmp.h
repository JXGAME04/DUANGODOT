// cImageBmp.h: interface for the cImageBmp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMAGEBMP_H__5E9FA1F3_E222_414A_A49A_1AF44F2342FF__INCLUDED_)
#define AFX_CIMAGEBMP_H__5E9FA1F3_E222_414A_A49A_1AF44F2342FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cImage.h"

class cImageBmp : public cImage  
{
public:
	cImageBmp();
	virtual ~cImageBmp();

//Load and Save
	stImageInfo m_info;
//	stFrameInfo m_infoFrame;
	enum {MAX_FRAME = 64};
	UINT m_nFrameInfo;
	stFrameInfo m_aFrameInfo[MAX_FRAME];

	HRESULT LoadFile(const char *filename) ;
	ULONG AddImage(iImage* p) ;
	HRESULT SaveFile(const char *filename, stImageInfo* info = NULL) ;
	HRESULT CreateFile(const char *filename,  const stImageInfo& m_info);

	BOOL DivideTo(UINT wDivide, UINT hDivide);
	void SaveFileDivide(const char *filename, UINT wDivide, UINT hDivide, int& i);

//info of a image
	void GetImageInfo(stImageInfo& info) ;
	void GetFrameInfo(stFrameInfo& info,ULONG nFrame = 0) ;
//render to target
	HRESULT Render(stImageRender& render) ;
	CBitmap* ImageToBitmap(ULONG nFrame ,int nDepth ) ;
};

#endif // !defined(AFX_CIMAGEBMP_H__5E9FA1F3_E222_414A_A49A_1AF44F2342FF__INCLUDED_)
