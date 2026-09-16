//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KDrawSpriteAlpha.h
// Date:	2000.08.08
// Code:	WangWei(Daphnis)
// Desc:	Header File
//---------------------------------------------------------------------------
#ifndef KDrawSpriteAlpha_H
#define KDrawSpriteAlpha_H
//---------------------------------------------------------------------------
//UINT g_BlendColor16b(UINT nSrcColor, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE);
//UINT g_BlendColor16b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE, UINT usA = 0);
//UINT g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT usA = 0);
//void g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT& uR, UINT& uG, UINT& uB);
void g_DrawSpriteAlpha(void* node, void* canvas);
void g_DrawSpriteAlpha(void* node, void* canvas, int nExAlpha);
void g_DrawSpriteBlendColor(void* node, void* canvas, UINT nColor);
void g_DrawSpriteBlendColor32b(void* node, void* canvas, UINT nColor);
void g_DrawSpriteScreen(void* node, void* canvas, UINT nColor);
void g_DrawSpriteScreen32b(void* node, void* canvas, UINT nColor);
void g_DrawSpriteAlpha32b(void* node, void* canvas);
void g_DrawSpriteAlpha32b(void* node, void* canvas, int nExAlpha);
void g_DrawSprite3LevelAlpha(void* node, void* canvas);	//Èý¼¶alpha»æÖÆ
void g_DrawAlphaRecImage(void* node, void* canvas);
void g_DrawAlphaRecImageOpa(void* node, void* canvas);
void g_DrawAlphaRecImage32b(void* node, void* canvas, int bScrMode);
void g_DrawOpaRecImage32b(void* node, void* canvas);
//---------------------------------------------------------------------------
#endif
