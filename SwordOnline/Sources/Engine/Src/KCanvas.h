//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KCanvas.h
// Date:	2000.08.08
// Code:	WangWei(Daphnis)
// Desc:	Header File
//---------------------------------------------------------------------------
#ifndef KCanvas_H
#define KCanvas_H
//---------------------------------------------------------------------------
#include "KList.h"
#include "KMemClass.h"
#include "Colors.h"
//---------------------------------------------------------------------------
//typedef void (* DRAW_FUNC)(void* pNode, void* pCanvas);
//---------------------------------------------------------------------------
class KDrawNode : public KNode
{
public:
//	DRAW_FUNC	m_pDrawFunc;
	BOOL		m_bChanged;
	int			m_nX;
	int			m_nY;
	int			m_nWidth;
	int			m_nHeight;
	int			m_nColor;
	int			m_nAlpha;
	void*		m_pBitmap;
	void*		m_pPalette;
};
//---------------------------------------------------------------------------
typedef struct {
	int		x;			// 裁减后的X坐标
	int		y;			// 裁减后的Y坐标
	int		width;		// 裁减后的宽度
	int		height;		// 裁减后的高度
	int		left;		// 上边界裁剪量
	int		top;		// 左边界裁剪量
	int		right;		// 右边界裁剪量
} KClipper;
/*/---------------------------------------------------------------------------
typedef struct {
	long		num;		// 当前的对象数
	long		changed;	// 重绘标志
	KDrawNode*	node[10];	// 绘制对象指针
} KSubCanvas;*/
//---------------------------------------------------------------------------
class ENGINE_API KCanvas
{
private:
//	KList		m_DrawList;// 绘制列表
//	KMemClass	m_Canvas;// 画布内存对象
//	KMemClass	m_SubCanvas;//画布子区域
	LPDIRECTDRAWSURFACE	m_pSurface;
	int			m_nWidth;// 画布宽度
	int			m_nHeight;// 画布高度

public:
//	void*		m_pCanvasAlloc;// 分配画布内存
//	void*		m_pCanvas;// 画布内存   // 对齐256字节
	int			m_nMask16;// 16bit RGB Mask
	int			m_nMask32;// 32bit RGB Mask
	int			m_nScreenWidth;// 屏幕宽度
	int			m_nScreenHeight;// 屏幕高度
	int			m_nScreenPitch;// 屏幕跨度
//	long		m_nSubRows;// 行数目
//	long		m_nSubCols;// 列数目
	RECT		m_ClipRect;// 裁减区域
	BOOL		m_bChanged;// 改变标志
    BOOL        m_dwScreenMode;
private:
//	void		SubAddNode(int nX,int nY,int nW,int nH,KDrawNode* pNode);
//	void		SubAdd(int nCol, int nRow, KDrawNode* pNode);
//	void		SubChangedNode(int nX,int nY,int nW,int nH,KDrawNode* pNode);
//	void		SubChanged(int nCol, int nRow);
//	void		SubMarkChanged();
	void		UpdateBackBuffer(LPRECT lpRect);
	void		UpdateFrontBuffer(LPRECT lpRect);
	bool		GetUpdateRect(LPRECT lpSrc, LPRECT lpDest);
public:
	KCanvas();
	~KCanvas();
	bool		Init(int nWidth, int nHeight);
	void		Terminate();
	void		Changed(BOOL bChanged){m_bChanged = bChanged;};
	void		FillCanvas(UINT wColor);
	void		UpdateCanvas();
	void		UpdateScreen();
	int			GetWidth() { return m_nWidth; }
	int			GetHeight() { return m_nHeight; }
	void*		LockCanvas(int& nPitch);
	void		UnlockCanvas();
	void		BltSurface(LPDIRECTDRAWSURFACE pSurface, RECT* pDestRect);
//	void		UpdateScreen(LPRECT lpRect);
	BOOL		RectClip(LPRECT pRect);
	BOOL		MakeClip(int nX, int nY, int nWidth, int nHeight, KClipper* pClipper);
	void		SetClipRect(LPRECT pRect){m_ClipRect = *pRect;};
	void		GetClipRect(LPRECT pRect){*pRect = m_ClipRect;};
	void		DrawPixel(int nX, int nY, int nColor);
	void		DrawPixelAlpha(int nX, int nY, int nColor, int nAlpha, int bScreenMode = 0);
	void		DrawLine(int nX1, int nY1, int nX2, int nY2, int nColor);
	void		DrawLineAlpha(int nX1, int nY1, int nX2, int nY2, int nColor, int nAlpha);
	void		DrawSprite(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette);
	void		DrawSpriteBlendColor(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette,
					int nAlpha, UINT nColor, int nMode = CBM_MULTIPLY, BOOL bScreenMode = FALSE);
    void		DrawSpriteMixColor(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette,int nColor,int nAlpha);
    void		DrawSpriteWithColor(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette,int nColor,int nAlpha);
	void		DrawSpriteAlpha(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette, int nExAlpha);
	void		DrawSpriteBorder(int nX, int nY, int nWidth, int nHeight, int nColor, void* lpSprite);
	void		DrawSprite3LevelAlpha(int nX, int nY, int nWidth, int nHeight, void* lpSprite, void* lpPalette);
	void		DrawFont(int nX, int nY, int nWidth, int nHeight, int nColor, int nAlpha,void* lpFont);
	void		DrawFontWithBorder(int nX, int nY, int nWidth, int nHeight, int nColor, int nAlpha,void* lpFont, int nBorderColor);
	void		DrawFontSolid(int nX, int nY, int nWidth, int nHeight, int nColor, int nAlpha,void* lpFont);
	void		DrawBitmap(int nX, int nY, int nWidth, int nHeight, void* lpBitmap, void* lpPalette);
	void		DrawBitmap16(int nX, int nY, int nWidth, int nHeight, void* lpBitmap);
	void		DrawAlphaRecImage(int nX, int nY, int nWidth, int nHeight, void* lpBitmap, int nAlpha, int bOpa, int bScrMode);
	void		DrawBitmap16mmx(int nX, int nY, int nWidth, int nHeight, void* lpBitmap);
	void		DrawBitmap16win(int nX, int nY, int nWidth, int nHeight, void* lpBitmap);
	void		DrawFade(int nX, int nY, int nWidth, int nHeight, int nAlpha);
    void        Clear(int nX, int nY, int nWidth, int nHeight, int nColor);
    void        DotClear(int nX, int nY, int nWidth, int nHeight, int nColor);
    void        ClearAlpha(int nX, int nY, int nWidth, int nHeight, int nColor,int nAlpha);
};


extern ENGINE_API KCanvas* g_pCanvas;
//---------------------------------------------------------------------------
#endif
