// cImagePsd.h: interface for the cImagePsd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMAGEPSD_H__2E658AF8_23A2_4DA4_9187_D807ECE0762A__INCLUDED_)
#define AFX_CIMAGEPSD_H__2E658AF8_23A2_4DA4_9187_D807ECE0762A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cImage.h"

class cImagePsd : public cImage  
{
public:
	cImagePsd();
	virtual ~cImagePsd();

	int m_nLayer;
	enum {MAX_LAYER = 2,};
	stImageInfo m_psdData[MAX_LAYER];

//Load and Save
	HRESULT LoadFile(const char *filename) ;
	ULONG AddImage(iImage* p) ;
	HRESULT SaveFile(const char *filename, stImageInfo* info = NULL) ;
	void SavePSDToTGA(const char *szFolder);
	void SaveImageInfoToTGA(const char *szFile, const stImageInfo& info);
//info of a image
	void GetImageInfo(stImageInfo& info) ;
	void GetFrameInfo(stFrameInfo& info,ULONG nFrame = 0) ;
//render to target
	HRESULT Render(stImageRender& render) ;
	CBitmap* ImageToBitmap(ULONG nFrame,int nDepth) ;
};

#endif // !defined(AFX_CIMAGEPSD_H__2E658AF8_23A2_4DA4_9187_D807ECE0762A__INCLUDED_)
