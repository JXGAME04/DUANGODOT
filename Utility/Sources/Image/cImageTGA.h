// cImageTga.h: interface for the cImageTga class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMAGETga_H__A3E40227_BEE3_4FD8_B449_9C117FC288E2__INCLUDED_)
#define AFX_CIMAGETga_H__A3E40227_BEE3_4FD8_B449_9C117FC288E2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cImage.h"

#pragma pack(1)

class cImageTga : public cImage  
{
public:
	struct stTgaHeader
	{
		byte m_id_length;
		byte m_color_map_type;
		byte m_type;
		short m_cm_index;
		short m_cm_length;
		byte m_cm_entry_size;
		short m_x_org;
		short m_y_org;
		short dwWidth;
		short dwHeight;
		byte bits_per_pixel;
		byte m_desc;
	};

public:
	cImageTga();
	virtual ~cImageTga();

private:
//	ulGraphicsFileData m_tgadata;
	stImageInfo m_info;
	unsigned char	m_id[256];
	unsigned char	m_color_palette[256 * 3];
//Load and Save
	HRESULT LoadFile(const char *filename) ;
	ULONG AddImage(iImage* p) ;
	HRESULT SaveFile(const char *filename, stImageInfo* info = NULL) ;
//info of a image
	void GetImageInfo(stImageInfo& info) ;
	void GetFrameInfo(stFrameInfo& info,ULONG nFrame = 0) ;
	ULONG SetAlpha1();
	HRESULT MakeSmallestImage(BOOL bRemoveShadow);
//render to target
	HRESULT Render(stImageRender& render) ;
	CBitmap* ImageToBitmap(ULONG nFrame,int nDepth ) ;
	CBitmap* ImageAlphaToBitmap(int nFrame,int nDepth );

	BOOL	DivideTo(LPCSTR szFile, UINT wDivide, UINT hDivide);
	BOOL	DivideTo(LPCSTR szFile,stDivide& divide);
	void	DivideRect(LPCSTR szFile,const CRect& rc,stDivide* pDivide = NULL);
	HRESULT	AlphaFormat();
	HRESULT	CreateGrid(LPCSTR szFile,int width,int height,int wTile, int hTile) ;
	HRESULT	RemoveShadow();
};

void SaveTGAFile(const char *filename, const stImageInfo& m_tgadata);

#endif // !defined(AFX_CIMAGETga_H__A3E40227_BEE3_4FD8_B449_9C117FC288E2__INCLUDED_)
