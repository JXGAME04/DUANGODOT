/*****************************************************************************************
//  图形到内存区域的操作
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-11
------------------------------------------------------------------------------------------
*****************************************************************************************/

#pragma once
#include "../../../Engine/Src/KWin32.h"

//UINT g_BlendColor16b(UINT nSrcColor, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE);
//UINT g_BlendColor16b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE, UINT usA = 0);
//UINT g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT usA = 0);
//void g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT& uR, UINT& uG, UINT& uB);

void RIO_Set16BitImageFormat(int b565);

void RIO_CopySprToBuffer(void* pSprite, int nSprWidth, int nSprHeight, void* pPalette,
		void* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY);
void RIO_CopySprToBuffer32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY);
void RIO_CopySprToBufferAlpha(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha = 255);
void RIO_CopySprToBufferAlpha32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha = 255, int bSrcModRemoveBk = 0);
void RIO_CopySprToBufferScreen(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nMode, int nExAlpha = 255, UINT nColor = 0);
void RIO_ConvertToHardScreen32b(int nWidth, int nHeight, unsigned int* pBuffer);
void RIO_CopySprToBufferScreen32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nMode, int nExAlpha = 255, UINT nColor = 0);
void RIO_CopySprToBufferBlendColor(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha, UINT nColor, int nMode);
void RIO_CopySprToBufferBlendColor32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha, UINT nColor, int nMode);
void RIO_CopySprToBuffer3LevelAlpha(void* pSpr, int nSprWidth, int nSprHeight, void* pPalette,
		void* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY);
void RIO_CopyBitmap16ToBuffer(WORD* pBitmap, int nBmpWidth, int nBmpHeight,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha);
void RIO_CopyBitmap16ToBuffer32b(WORD* pBitmap, int nBmpWidth, int nBmpHeight,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha);
void RIO_CopyShadowToBuffer(UINT nSrcColor, int nBmpWidth, int nBmpHeight,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY);
void RIO_CopyShadowToBuffer32b(UINT nSrcColor, int nBmpWidth, int nBmpHeight,
		BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY);
