//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KDrawBase.cpp
// Date:	2000.08.08
// Code:	WangWei(Daphnis)
// Desc:	Graphics Drawing Functions
//---------------------------------------------------------------------------
#include "KWin32.h"
#include "KDDraw.h"
#include "KCanvas.h"
#include "KDrawBase.h"
#include "KColors.h"
//---------------------------------------------------------------------------
#define ABS(a)			((a > 0) ? a : -a)
#define SIGN(a)			((a > 0) ? 1 : -1)
//---------------------------------------------------------------------------
// 函数:	Draw Pixel
// 功能:	绘制一个点
// 参数:	x		X坐标
//			y		Y坐标
//			color	颜色
// 返回:	void
//---------------------------------------------------------------------------
void g_DrawPixel(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	long nX = pNode->m_nX;
	long nY = pNode->m_nY;
	long nColor = pNode->m_nColor;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	RECT ClipRect;
	pCanvas->GetClipRect(&ClipRect);

	__asm
	{
		mov		eax, nY
		cmp		eax, ClipRect.top
		jl		loc_PutPixel_exit
		cmp		eax, ClipRect.bottom
		jge		loc_PutPixel_exit

		mov		ebx, nX
		cmp		ebx, ClipRect.left
		jl		loc_PutPixel_exit
		cmp		ebx, ClipRect.right
		jge		loc_PutPixel_exit

		mov		ecx, nPitch
		mul		ecx
		add		eax, ebx
		add		eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
		mov		eax, nColor
		mov		[edi], ax

	loc_PutPixel_exit:
	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
// 函数:	PutPixelAlpha
// 功能:	绘制带透明度的象素点
// 参数:	x			X坐标
//			y			Y坐标
//			color		颜色值
//			alpha		Alpha值
// 返回:	void
// 公式:	(r1, g1, b1) ~ (r2, g2, b2) = (r3, g3, b3)
//			r3 = (r1 * alpha + r2 * (32 - alpha)) / 32
//			g3 = (g1 * alpha + g2 * (32 - alpha)) / 32
//			b3 = (b1 * alpha + b2 * (32 - alpha)) / 32
//---------------------------------------------------------------------------
void g_DrawPixelAlpha(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nAlpha = pNode->m_nAlpha/8;
	if(nAlpha == 0)
		return;
	int nX = pNode->m_nX;
	int nY = pNode->m_nY;
	int R = (pNode->m_nColor & 0xff0000) >> 16;
	int G = (pNode->m_nColor & 0xff00) >> 8;
	int B = pNode->m_nColor & 0xff;
	UINT nSrcColor = g_RGB(R, G, B);
	UINT nDstColor = nSrcColor & 0xf800;
	if(nDstColor == 0xf800)
		nSrcColor &= 0xf7ff;
	int nMask32 = pCanvas->m_nMask32;
	RECT ClipRect;
	pCanvas->GetClipRect(&ClipRect);
	if(nY < ClipRect.top || nY >= ClipRect.bottom)
		return;
	if(nX < ClipRect.left || nX >= ClipRect.right)
		return;
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += nY * nPitch + nX * 2;
	if(nAlpha >= 31)
	{
		*((WORD*)pBuffer) = nSrcColor;
		pCanvas->UnlockCanvas();
		return;
	}
	nDstColor = *((WORD*)pBuffer);
	nSrcColor |= (nSrcColor << 16);
	nSrcColor &= nMask32;
	nDstColor = *((WORD*)pBuffer);
	nDstColor |= (nDstColor << 16);
	nDstColor &= nMask32;
	nDstColor = (nSrcColor - nDstColor)*nAlpha/32 + nDstColor;
	nDstColor &= nMask32;
	nSrcColor = (nDstColor >> 16) & 0x7e0;
	nDstColor = (nDstColor | nSrcColor) & 0xffff;
	*((WORD*)pBuffer) = nDstColor;
	pCanvas->UnlockCanvas();
}

void g_DrawPixelScreen16b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	UINT nAlpha = pNode->m_nAlpha/8;
	if(nAlpha == 0)
		return;
	int nX = pNode->m_nX;
	int nY = pNode->m_nY;
	RECT ClipRect;
	pCanvas->GetClipRect(&ClipRect);
	if(nY < ClipRect.top || nY >= ClipRect.bottom)
		return;
	if(nX < ClipRect.left || nX >= ClipRect.right)
		return;
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += nY * nPitch + nX * 2;
	UINT nDstColor = *((WORD*)pBuffer);
	UINT R = (pNode->m_nColor & 0xff0000) >> 16;
	UINT G = (pNode->m_nColor & 0xff00) >> 8;
	UINT B = pNode->m_nColor & 0xff;
	nAlpha = pNode->m_nAlpha;
	float sA = (float)nAlpha/255.f;
	float sR = (float)R/255.f;
	float sG = (float)G/255.f;
	float sB = (float)B/255.f;
	float dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
	float dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
	float dB = (float)((nDstColor & 0x001f) << 3)/255.f;
	dR = (sR+dR-sR*dR)*sA + dR*(1.f-sA);
	dG = (sG+dG-sG*dG)*sA + dG*(1.f-sA);
	dB = (sB+dB-sB*dB)*sA + dB*(1.f-sA);
	nDstColor = (((UINT)(dB*255.f)) >> 3);
	nDstColor |= ((((UINT)(dR*255.f)) >> 3) << 11);
	nDstColor |= ((((UINT)(dG*255.f)) >> 2) << 5);
	*((WORD*)pBuffer) = nDstColor;
	pCanvas->UnlockCanvas();
}

void g_DrawPixelAlpha32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nX = pNode->m_nX;
	int nY = pNode->m_nY;
	UINT nSrcColor = pNode->m_nColor;
	UINT nAlpha = pNode->m_nAlpha;
	if(nAlpha == 0)
		return;
	RECT ClipRect;
	pCanvas->GetClipRect(&ClipRect);
	if(nY < ClipRect.top || nY >= ClipRect.bottom)
		return;
	if(nX < ClipRect.left || nX >= ClipRect.right)
		return;
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += nY * nPitch + nX * 4;
	if(nAlpha >= 254)
	{
		nSrcColor |= 0xff000000;
		*((UINT*)pBuffer) = nSrcColor;
		pCanvas->UnlockCanvas();
		return;
	}
	UINT R,G,B,R2,G2,B2;
	UINT nDstColor = *((UINT*)pBuffer);
	R = (nSrcColor & 0xff0000) >> 16;
	G = (nSrcColor & 0xff00) >> 8;
	B = nSrcColor & 0xff;
	R2 = (nDstColor & 0xff0000) >> 16;
	G2 = (nDstColor & 0xff00) >> 8;
	B2 = nDstColor & 0xff;
	R = (nAlpha*R+(255-nAlpha)*R2)/255;
	G = (nAlpha*G+(255-nAlpha)*G2)/255;
	B = (nAlpha*B+(255-nAlpha)*B2)/255;
	nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
	*((UINT*)pBuffer) = nDstColor;
	pCanvas->UnlockCanvas();
}

void g_DrawPixelScreen32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nX = pNode->m_nX;
	int nY = pNode->m_nY;
	UINT nSrcColor = pNode->m_nColor;
	UINT nAlpha = pNode->m_nAlpha;
	if(nAlpha == 0)
		return;
	RECT ClipRect;
	pCanvas->GetClipRect(&ClipRect);
	if(nY < ClipRect.top || nY >= ClipRect.bottom)
		return;
	if(nX < ClipRect.left || nX >= ClipRect.right)
		return;
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += nY * nPitch + nX * 4;
	UINT R,G,B,R2,G2,B2;
	UINT nDstColor = *((UINT*)pBuffer);
	R = (nSrcColor & 0xff0000) >> 16;
	G = (nSrcColor & 0xff00) >> 8;
	B = nSrcColor & 0xff;
	R2 = (nDstColor & 0xff0000) >> 16;
	G2 = (nDstColor & 0xff00) >> 8;
	B2 = nDstColor & 0xff;
	float sA = (float)nAlpha/255.f;
	float sR = (float)R/255.f;
	float sG = (float)G/255.f;
	float sB = (float)B/255.f;
	float dR = (float)R2/255.f;
	float dG = (float)G2/255.f;
	float dB = (float)B2/255.f;
	if(nAlpha)
	{
		dR = (sR+dR-sR*dR)*sA + dR*(1.f-sA);
		dG = (sG+dG-sG*dG)*sA + dG*(1.f-sA);
		dB = (sB+dB-sB*dB)*sA + dB*(1.f-sA);
	}
	else
		gBlendMode_Screen(dR,dG,dB,sR,sG,sB);
	nDstColor = (UINT)(dB*255.f);
	nDstColor |= ((UINT)(dR*255.f)) << 16;
	nDstColor |= ((UINT)(dG*255.f)) << 8;
	*((UINT*)pBuffer) = nDstColor;
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
// 函数:	DrawLine
// 功能:	绘制直线
// 参数:	x1		X1坐标
//			x2		X2坐标
//			y1		Y1坐标
//			y2		Y2坐标
//			color	颜色值
// 返回:	void
//---------------------------------------------------------------------------
void g_DrawLine(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	long x1 = pNode->m_nX;
	long y1 = pNode->m_nY;
	long x2 = pNode->m_nWidth;
	long y2 = pNode->m_nHeight;
	KDrawNode LineNode;
	LineNode.m_nColor = pNode->m_nColor;

	long d, x, y, ax, ay, sx, sy, dx, dy;
	
	dx = x2 - x1;
	ax = abs(dx) << 1;
	sx = SIGN(dx);
	
	dy = y2 - y1;
	ay = abs(dy) << 1;
	sy = SIGN(dy);
	
	x  = x1;
	y  = y1;
	
	if (ax > ay) 
	{
		d = ay - (ax >> 1);
		while (x != x2)
		{
			LineNode.m_nX = x;
			LineNode.m_nY = y;
			g_DrawPixel(&LineNode, pCanvas);
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - (ay >> 1);
		while (y != y2)
		{
			LineNode.m_nX = x;
			LineNode.m_nY = y;
			g_DrawPixel(&LineNode, pCanvas);
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
	LineNode.m_nX = x;
	LineNode.m_nY = y;
	g_DrawPixel(&LineNode, pCanvas);
}
//---------------------------------------------------------------------------
// 函数:	Draw Line Alpha
// 功能:	绘制透明直线
// 参数:	x1		X1坐标
//			x2		X2坐标
//			y1		Y1坐标
//			y2		Y2坐标
//			color	颜色值 
//			alpha	透明值
// 返回:	void
//---------------------------------------------------------------------------
void g_DrawLineAlpha(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int x1 = pNode->m_nX;
	int y1 = pNode->m_nY;
	int x2 = pNode->m_nWidth;
	int y2 = pNode->m_nHeight;
	KDrawNode LineNode;
	LineNode.m_nColor = pNode->m_nColor;
	LineNode.m_nAlpha = pNode->m_nAlpha;
	
	int d, x, y, ax, ay, sx, sy, dx, dy;
	
	dx = x2 - x1;
	ax = abs(dx) << 1;
	sx = SIGN(dx);
	
	dy = y2 - y1;
	ay = abs(dy) << 1;
	sy = SIGN(dy);
	
	x  = x1;
	y  = y1;
	if(g_pDirectDraw->GetRGBBitCount() == 32)
	{
		if (ax > ay) 
		{
			d = ay - (ax >> 1);
			while (x != x2)
			{
				LineNode.m_nX = x;
				LineNode.m_nY = y;
				g_DrawPixelAlpha32b(&LineNode, pCanvas);
				if (d >= 0)
				{
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		}
		else
		{
			d = ax - (ay >> 1);
			while (y != y2)
			{
				LineNode.m_nX = x;
				LineNode.m_nY = y;
				g_DrawPixelAlpha32b(&LineNode, pCanvas);
				if (d >= 0)
				{
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
		LineNode.m_nX = x;
		LineNode.m_nY = y;
		g_DrawPixelAlpha32b(&LineNode, pCanvas);
	}
	else
	{
		if (ax > ay) 
		{
			d = ay - (ax >> 1);
			while (x != x2)
			{
				LineNode.m_nX = x;
				LineNode.m_nY = y;
				g_DrawPixelAlpha(&LineNode, pCanvas);
				if (d >= 0)
				{
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		}
		else
		{
			d = ax - (ay >> 1);
			while (y != y2)
			{
				LineNode.m_nX = x;
				LineNode.m_nY = y;
				g_DrawPixelAlpha(&LineNode, pCanvas);
				if (d >= 0)
				{
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
		LineNode.m_nX = x;
		LineNode.m_nY = y;
		g_DrawPixelAlpha(&LineNode, pCanvas);
	}
}

//---------------------------------------------------------------------------
// 函数:	Clear a region on canvas
// 功能:	用色彩nColor清除区域
// 参数:	nX, nY, nWidth, nHeight, nColor
// 返回:	void
//---------------------------------------------------------------------------
void g_Clear(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	long nX = pNode->m_nX;// x coord
	long nY = pNode->m_nY;// y coord
	long nWidth = pNode->m_nWidth;// width of sprite
	long nHeight = pNode->m_nHeight;// height of sprite
	int  nColor  = pNode->m_nColor;//  color

	// 对绘制区域进行裁剪
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	// 计算屏幕下一行的偏移
	long ScreenOffset = nPitch - Clipper.width * 2;

	// 绘制函数的汇编代码
	__asm
	{
//---------------------------------------------------------------------------
//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, Clipper.x
		add		ebx, ebx
		add     eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
		mov		ecx, Clipper.height
//---------------------------------------------------------------------------
//  eax = nColor
//---------------------------------------------------------------------------
		mov		eax, nColor
		mov     bx, ax
		shl     eax,16
		mov     ax,bx
//---------------------------------------------------------------------------
//  color tranfer
//---------------------------------------------------------------------------

	loc_Clear_0001:
		push	ecx
		mov		edx, Clipper.width
		mov		ecx, edi
		shr		ecx, 1
		and		ecx, 1
		sub		edx, ecx
		rep		stosw
		mov		ecx, edx
		shr		ecx, 1
		rep		stosd
		adc		ecx, ecx
		rep		stosw
		add		edi, ScreenOffset

		pop		ecx
		dec		ecx
		jnz		loc_Clear_0001
	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
// 函数:	Clear a region on canvas
// 功能:	用色彩nColor隔点清除清除区域
// 参数:	nX, nY, nWidth, nHeight, nColor
// 返回:	void
//---------------------------------------------------------------------------
void g_DotClear(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	long nX = pNode->m_nX;// x coord
	long nY = pNode->m_nY;// y coord
	long nWidth = pNode->m_nWidth;// width of sprite
	long nHeight = pNode->m_nHeight;// height of sprite
	int  nColor  = pNode->m_nColor;//  color

	// 对绘制区域进行裁剪
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	// 计算屏幕下一行的偏移
	long ScreenOffset = nPitch - Clipper.width * 2;

	
	// 绘制函数的汇编代码
	__asm
	{
//---------------------------------------------------------------------------
//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, Clipper.x
		add		ebx, ebx
		add     eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
		mov		ecx, Clipper.height
//---------------------------------------------------------------------------
//  eax = nColor
//---------------------------------------------------------------------------
		mov		eax, nColor
//---------------------------------------------------------------------------
//  color tranfer
//---------------------------------------------------------------------------

loc_DotClear_0001:
		push	ecx
		mov		ecx, Clipper.width
loc_DotClear_0002:
        stosw
        dec  ecx
        jz   loc_DotClear_0003
        inc     edi
		inc     edi
        dec  ecx
        jnz   loc_DotClear_0002
loc_DotClear_0003:
		add		edi, ScreenOffset
		pop		ecx
		dec		ecx
        jz      loc_DotClear_0006 

		push	ecx
		mov		ecx, Clipper.width
loc_DotClear_0004:
		inc     edi
        inc     edi
		dec     ecx
		jz      loc_DotClear_0005
        stosw
        dec  ecx
        jnz   loc_DotClear_0004
loc_DotClear_0005:
		add		edi, ScreenOffset
		pop		ecx
		dec		ecx
		jnz		loc_DotClear_0001
loc_DotClear_0006:
	}
	pCanvas->UnlockCanvas();
}

void g_ClearAlpha32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of sprite
	int nHeight = pNode->m_nHeight;// height of sprite
	int nSrcColor = pNode->m_nColor;
	UINT nAlpha = pNode->m_nAlpha;

	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	int ScreenOffset = nPitch - Clipper.width * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	if(nAlpha >= 254)
	{
		nSrcColor |= 0xff000000;
		while(Clipper.height)
		{
			for(int i=0;i<Clipper.width;i++)
			{
				*((UINT*)pBuffer) = nSrcColor;
				pBuffer += 4;
			}
			Clipper.height--;
			if(!Clipper.height)
				break;
			pBuffer += ScreenOffset;
		}
		pCanvas->UnlockCanvas();
		return;
	}
	UINT nDstColor,R,G,B,R2,G2,B2;
	R = (nSrcColor & 0xff0000) >> 16;
	G = (nSrcColor & 0xff00) >> 8;
	B = nSrcColor & 0xff;
	while(Clipper.height)
	{
		for(int i=0;i<Clipper.width;i++)
		{
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor & 0xff0000) >> 16;
			G2 = (nDstColor & 0xff00) >> 8;
			B2 = nDstColor & 0xff;
			R2 = (nAlpha*R+(255-nAlpha)*R2)/255;
			G2 = (nAlpha*G+(255-nAlpha)*G2)/255;
			B2 = (nAlpha*B+(255-nAlpha)*B2)/255;
			nDstColor = 0xff000000 | (R2 << 16) | (G2 << 8) | B2;
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
		}
		Clipper.height--;
		if(!Clipper.height)
			break;
		pBuffer += ScreenOffset;
	}
	pCanvas->UnlockCanvas();
}

//---------------------------------------------------------------------------
// 函数:	Clear a region on canvas with alpha
// 功能:	用色彩nColor带alpha清除区域
// 参数:	nX, nY, nWidth, nHeight, nColor,nAlpha
// 返回:	void
// Modify By Freeway Chen in 2003.7.6, USE MMX Register
//---------------------------------------------------------------------------
void g_ClearAlpha(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nAlpha = pNode->m_nAlpha/8;
	if(nAlpha == 0)
		return;
	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of sprite
	int nHeight = pNode->m_nHeight;// height of sprite
	int R = (pNode->m_nColor & 0xff0000) >> 16;
	int G = (pNode->m_nColor & 0xff00) >> 8;
	int B = pNode->m_nColor & 0xff;
	int nColor = g_RGB(R, G, B);
	UINT nCheckColor = nColor & 0xf800;
	if(nCheckColor == 0xf800)
		nColor &= 0xf7ff;
	nAlpha = 31 - nAlpha;
	int nMask32 = pCanvas->m_nMask32;
	// 对绘制区域进行裁剪
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	// 计算屏幕下一行的偏移
	int ScreenOffset = nPitch - Clipper.width * 2;
	
	// 绘制函数的汇编代码
	__asm
	{
//---------------------------------------------------------------------------
//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, Clipper.x
		add		ebx, ebx
		add     eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
        
        mov     esi, nMask32    // esi:  nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, esi        // esi:  nMask32
        movd    mm1, ecx        // mm1: nMaskColor

        mov     ecx, nAlpha
        mov     eax, 0x20
        movd    mm3, ecx        // mm3: nAlpha
        sub     eax, ecx
        movd    ecx, mm1        // mm1: nMaskColor
        imul    eax, ecx      
        movd    mm1, eax        // mm1: nMaskColor * (32 - nAlpha)

		mov		ecx, Clipper.height

        movd    mm6, Clipper.width

//---------------------------------------------------------------------------
//  color tranfer
//---------------------------------------------------------------------------

loc_ClearAlpha_0001:
		movd    mm7, ecx 
		movd	edx, mm6        // Clipper.width

loc_ClearAlpha_0002:
  	    mov     ax, [edi]
		mov		cx, ax
		sal		eax, 16
		movd	ebx, mm3        // mm3: nAlpha
		mov		ax, cx
		and		eax, esi        // esi:  nMask32 

		imul	eax, ebx
        movd    ecx, mm1          // mm1: nMaskColor * (32 - nAlpha)
		add		eax, ecx
		sar		eax, 5
		and		eax, esi         // esi:  nMask32

		mov		bx, ax
		sar		eax, 16
        add     edi, 2 
		or		ax, bx
        dec		edx
		mov     [edi - 2], ax
		jnz		loc_ClearAlpha_0002
         		

		movd    ecx, mm7
		add		edi, ScreenOffset
		dec		ecx
		jnz		loc_ClearAlpha_0001
        emms
	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------