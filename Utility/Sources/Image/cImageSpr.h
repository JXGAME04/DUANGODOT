// cImageSpr.h: interface for the cImageSpr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMAGESPR_H__27F9E036_AA37_4437_9D7C_DC8B75F90317__INCLUDED_)
#define AFX_CIMAGESPR_H__27F9E036_AA37_4437_9D7C_DC8B75F90317__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cImage.h"

class cImageSpr : public cImage  
{
typedef struct
{
	BYTE	Comment[4];	// 注释文字(SPR\0)
	WORD	Width;		// 图片宽度
	WORD	Height;		// 图片高度
	WORD	CenterX;	// 重心的水平位移
	WORD	CenterY;	// 重心的垂直位移
	WORD	Frames;		// 总帧数
	WORD	Colors;		// 颜色数
	WORD	Directions;	// 方向数
	WORD	Interval;	// 每帧间隔（以游戏帧为单位）
	WORD	Reserved[6];// 保留字段（到以后使用）
} SPRHEAD;
//---------------------------------------------------------------------------
typedef struct
{
	DWORD	Offset;		// 每一帧的偏移
	DWORD	Length;		// 每一帧的长度
} SPROFFS;
//---------------------------------------------------------------------------
typedef struct
{
	WORD	Width;		// 帧最小宽度
	WORD	Height;		// 帧最小高度
	WORD	OffsetX;	// 水平位移（相对于原图左上角）
	WORD	OffsetY;	// 垂直位移（相对于原图左上角）
	BYTE	Sprite[1];	// RLE压缩图形数据
} SPRFRAME;


typedef struct {
	BYTE		Blue;		// 兰色分量
	BYTE		Green;		// 绿色分量
	BYTE		Red;		// 红色分量
	BYTE		Alpha;		// 透明分量
} KPAL32;					// 32位色盘
//---------------------------------------------------------------------------
typedef struct {
	BYTE		Red;		// 红色分量
	BYTE		Green;		// 绿色分量
	BYTE		Blue;		// 兰色分量
} KPAL24;					// 24位色盘

typedef WORD	KPAL16;		// 16位色盘

public:
	cImageSpr();
	virtual ~cImageSpr();

//////////////////////////////////////////////////////////////////////
//load and save
	CString m_strName;
	HRESULT LoadFile(const char *filename) ;
	HRESULT LoadFileInfo(const char *filename) ;
	enum {MAX_IMAGE = 256,};
//	stImageInfo m_aInfo[MAX_IMAGE];
	stImageInfo m_info;
	int m_nFrameInfo;
	stFrameInfo m_aFrameInfo[MAX_IMAGE];
	ULONG AddImage(iImage* p);
	HRESULT SaveFile(const char *filename, stImageInfo* pInfo) ;
	HRESULT SaveFile888(const char *filename, stImageInfo* pInfo) ;
	HRESULT SaveFile8888(const char *filename, stImageInfo* pInfo) ;

//设置重心

//////////////////////////////////////////////////////////////////////
//info of a image
protected:	
	ULONG	m_size;
	ULONG		GetSize() ; // get memory size;
	HRESULT		GetOffset(ULONG nFrame, int& x, int &y) ;
	HRESULT		SetOffset(int nFrame,int x,int y) ;
	void		GetSize(ULONG nFrame, ULONG& width, ULONG& height);
public:
	void		GetFrameInfo(stFrameInfo& info,ULONG nFrame) ;
	void		GetImageInfo(stImageInfo& info) ;
	HRESULT		GetAlphaBottom(int nFrame, int&x, int& y) ;
	HRESULT		GetCenterToUpleft(int nFrame,int& x,int& y) ;
	HRESULT		SetCenterToUpleft(int nFrame,int x,int y) ;

//////////////////////////////////////////////////////////////////////
//render
protected:
	HRESULT RenderToA4R4G4B4(stImageRender& render);
	HRESULT RenderToA4R4G4B4Alpha(stImageRender& render);
	HRESULT RenderToA4R4G4B4Alpha2(stImageRender& render);
	HRESULT RenderToR5G6B5(stImageRender& render);
	HRESULT RenderToR5G6B5Alpha(stImageRender& render);
public:
	CBitmap* ImageToBitmap(ULONG nFrame,int nDepth);
	HRESULT	Render(stImageRender& render);

private:
	BYTE* 		m_Buffer;
	BYTE*	 	m_Palette;
	KPAL24*		m_pPal24;
	KPAL16*		m_pPal16;
	SPROFFS* 	m_pOffset;
	PBYTE		m_pSprite;
	int			m_nWidth;
	int			m_nHeight;
	int			m_nCenterX;
	int			m_nCenterY;
	ULONG		m_nFrames;
	int			m_nColors;
	ULONG		m_nDirections;
	int			m_nInterval;
	int			m_nColorStyle;
	int			m_nLum;

public:
	BOOL		Load(LPCSTR  FileName);
	BOOL		LoadInfo(LPCSTR  FileName);
	BOOL		Save();
//	BOOL		LoadFor3D(LPCSTR  FileName);
	void		Free();
	void		MakePalette();
	void		Make4444Palette();
	void		MakePaletteLum32();
	void		SetColorStyle(int nStyle, int nLum);
	void		SetStyle(int nStyle);
	UINT		NextFrame(UINT nFrame);
	int			GetWidth(){ return m_nWidth; };
	int			GetHeight(){ return m_nHeight; };
	int			GetCenterX(){ return m_nCenterX; };
	int			GetCenterY(){ return m_nCenterY; };
	ULONG		GetFrames(){ return m_nFrames; };
	PVOID		GetFrame(UINT nFrame);
	ULONG		GetDirections(){ return m_nDirections; };
	int			GetInterval(){ return m_nInterval; };
	int			GetPixelAlpha(UINT nFrame, int x, int y);
	PVOID		GetPalette(){return m_Palette;}	;
	PVOID		Get24Palette(){return m_pPal24;};
	BOOL		IsAlphaBad();

public:
//	BOOL		SaveIt();
//	HRESULT		RemoveShadow();
};

#endif // !defined(AFX_CIMAGESPR_H__27F9E036_AA37_4437_9D7C_DC8B75F90317__INCLUDED_)
