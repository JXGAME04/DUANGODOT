/*****************************************************************************************
//  表现模块的对外接口的二维版本实现。
//	Copyright : Kingsoft 2002
//	Author	:   Spe(huyi)
//	CreateTime:	2002-11-11
*****************************************************************************************/
//#define _REPRESENT_INTERNAL_SIGNATURE_
#include "KRepresentShell2.h"
#include "../iRepresent/KRepresentUnit.h"
#include "../../Engine/src/KColors.h"
#include "../iRepresent/Font/KFont2.h"
#include "../iRepresent/Image/ImageOperation.h"
#include "..\iRepresent\RepresentUtility.h"
#include "..\..\engine\src\KWin32Wnd.h"
#include "..\..\engine\src\KBmpFile24.h"
#include "..\..\engine\src\KTgaFile32.h"
#include "../../Engine/Src/Text.h"
#include <assert.h>
#include <algorithm>

#pragma warning(disable:4244)
//根据SPR头指针，获取调色版缓冲区指针
#define GET_SPR_PALETTE(pHeader)	( ((char*)pHeader) + sizeof(SPRHEAD))


void FillSprData(UINT* pDestData, int nWidth, int nHeight, void* lpSprite, void* lpPalette)
{
	BYTE* pPalette	= (BYTE*)lpPalette;// palette pointer
	BYTE* pSprite = (BYTE*)lpSprite;	// sprite pointer
	int h = nHeight;
	while(h)
	{
		int w = nWidth;
		while(w > 0)
		{
			int nPixelBatch = *(pSprite++);
			UINT nAlpha = *(pSprite++);
			w -= nPixelBatch;
			if(nAlpha)
			{
				while(nPixelBatch)
				{
					UINT nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					UINT nDstColor = nSrcColor | (nAlpha << 24);
					*(pDestData++) = nDstColor;
					nPixelBatch--;
				}
			}
			else
				pDestData += nPixelBatch;
		}
		h--;
	}
}

void FillSprData32b(UINT* pDestData, int nWidth, int nHeight, void* lpSprite, void* lpPalette)
{
	BYTE* pPalette	= (BYTE*)lpPalette;// palette pointer
	BYTE* pSprite = (BYTE*)lpSprite;	// sprite pointer
	int h = nHeight;
	while(h)
	{
		int w = nWidth;
		while(w > 0)
		{
			int nPixelBatch = *(pSprite++);
			UINT nAlpha = *(pSprite++);
			w -= nPixelBatch;
			if(nAlpha)
			{
				while(nPixelBatch)
				{
					UINT R = *(pPalette + 3*(*pSprite));
					UINT G = *(pPalette + 3*(*pSprite) + 1);
					UINT B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					UINT nDstColor = (R << 16) | (G << 8) | B | (nAlpha << 24);
					*(pDestData++) = nDstColor;
					nPixelBatch--;
				}
			}
			else
				pDestData += nPixelBatch;
		}
		h--;
	}
}
//=========创建一个iRepresentShell接口的实例===============
#ifdef REP_STATIC
extern "C" //__declspec(dllexport)
iRepresentShell* CreateRepresentShell()
{
	return (new KRepresentShell2);
}
#else
extern "C" __declspec(dllexport)
iRepresentShell* CreateRepresentShell()
{
	return (new KRepresentShell2);
}
#endif

IInlinePicEngineSink* g_pIInlinePicSinkRP = NULL;	//嵌入式图片的处理接口[wxb 2003-6-20]

inline void SWAP(int &x, int &y)
{
	int t = x; x = y; y = t;
}

inline unsigned int SampleColour(BYTE* pTex, int nWidth, int nHeight, float x, float y)
{
	/*int sx = (int)(x * (float)nWidth);
	int sy = (int)(y * (float)nHeight);
	if(sx == nWidth) sx = nWidth - 1;
	if(sy == nHeight) sy = nHeight - 1;
	if (sx < 0 || sx >= nWidth || sy < 0 || sy >= nHeight)
		return 0xff000000;
	else
		return *(((unsigned int*)pTex) + sy * nWidth + sx);*/
	//clamp x,y into [0,1]
	if (x < 0.0f) x = 0.0f; else if (x > 1.0f) x = 1.0f;
    if (y < 0.0f) y = 0.0f; else if (y > 1.0f) y = 1.0f;
    // map v祇 t﹎ texel: [0..1] -> [0..(W-1)] r錳 + 0.5 璾 ti猲 t﹎
    float fx = x * (nWidth  - 1);
    float fy = y * (nHeight - 1);

    int sx = (int)(fx + 0.5f);
    int sy = (int)(fy + 0.5f);

    if (sx < 0) sx = 0; else if (sx >= nWidth)  sx = nWidth  - 1;
    if (sy < 0) sy = 0; else if (sy >= nHeight) sy = nHeight - 1;

    return *(((unsigned int*)pTex) + sy * nWidth + sx);
}

void KRepresentShell2::DrawPointLine(int sx, int ex, int ny, unsigned int uColor, BOOL bScreenMode, bool bCheckDupl)
{
	KRColor c;
	c.Color_dw = uColor;
	int nColor = uColor & 0xffffff;
	for (int i = sx; i <= ex; i++)
	{
		m_Canvas.DrawPixelAlpha(i, ny, nColor, c.Color_b.a, bScreenMode);
	}
}

void KRepresentShell2::FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int uColor, bool bCheckDupl)
{
	int t1x, t2x, y, minx, maxx, t1xp, t2xp;
	bool changed1 = false;
	bool changed2 = false;
	int signx1, signx2, dx1, dy1, dx2, dy2;
	int e1, e2;
	// Sort vertices
	if (y1>y2) { SWAP(y1, y2); SWAP(x1, x2); }
	if (y1>y3) { SWAP(y1, y3); SWAP(x1, x3); }
	if (y2>y3) { SWAP(y2, y3); SWAP(x2, x3); }

	t1x = t2x = x1; y = y1;   // Starting points
	dx1 = (int)(x2 - x1); if (dx1<0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y2 - y1);

	dx2 = (int)(x3 - x1); if (dx2<0) { dx2 = -dx2; signx2 = -1; }
	else signx2 = 1;
	dy2 = (int)(y3 - y1);

	if (dy1 > dx1) {   // swap values
		SWAP(dx1, dy1);
		changed1 = true;
	}
	if (dy2 > dx2) {   // swap values
		SWAP(dy2, dx2);
		changed2 = true;
	}

	e2 = (int)(dx2 >> 1);
	// Flat top, just process the second half
	int i;
	if (y1 == y2) goto next;
	e1 = (int)(dx1 >> 1);

	for ( i = 0; i < dx1;) {
		t1xp = 0; t2xp = 0;
		if (t1x<t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }
		// process first line until y value is about to change
		while (i<dx1) {
			i++;
			e1 += dy1;
			while (e1 >= dx1) {
				e1 -= dx1;
				if (changed1) t1xp = signx1;//t1x += signx1;
				else          goto next1;
			}
			if (changed1) break;
			else t1x += signx1;
		}
		// Move line
	next1:
		// process second line until y value is about to change
		while (1) {
			e2 += dy2;
			while (e2 >= dx2) {
				e2 -= dx2;
				if (changed2) t2xp = signx2;//t2x += signx2;
				else          goto next2;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}
	next2:
		if (minx>t1x) minx = t1x; if (minx>t2x) minx = t2x;
		if (maxx<t1x) maxx = t1x; if (maxx<t2x) maxx = t2x;
		DrawPointLine(minx, maxx, y, uColor, FALSE, bCheckDupl);    // Draw line from min to max points found on the y
									 // Now increase y
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y == y2) break;

	}
next:
	// Second half
	dx1 = (int)(x3 - x2); if (dx1<0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y3 - y2);
	t1x = x2;

	if (dy1 > dx1) {   // swap values
		SWAP(dy1, dx1);
		changed1 = true;
	}
	else changed1 = false;

	e1 = (int)(dx1 >> 1);

	for ( i = 0; i <= dx1; i++) {
		t1xp = 0; t2xp = 0;
		if (t1x<t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }
		// process first line until y value is about to change
		while (i<dx1) {
			e1 += dy1;
			while (e1 >= dx1) {
				e1 -= dx1;
				if (changed1) { t1xp = signx1; break; }//t1x += signx1;
				else          goto next3;
			}
			if (changed1) break;
			else   	   	  t1x += signx1;
			if (i<dx1) i++;
		}
	next3:
		// process second line until y value is about to change
		while (t2x != x3) {
			e2 += dy2;
			while (e2 >= dx2) {
				e2 -= dx2;
				if (changed2) t2xp = signx2;
				else          goto next4;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}
	next4:

		if (minx>t1x) minx = t1x; if (minx>t2x) minx = t2x;
		if (maxx<t1x) maxx = t1x; if (maxx<t2x) maxx = t2x;
		DrawPointLine(minx, maxx, y, uColor, FALSE, bCheckDupl);   										
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y>y3) return;
	}
}

void KRepresentShell2::FillColorTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int uColor)
{
	FillTriangle(x1, y1, x2, y2, x3, y3, uColor, FALSE);
}

HRESULT KRepresentShell2::AdviseRepresent(IInlinePicEngineSink* pSink)
{
	assert(NULL == g_pIInlinePicSinkRP);	//一般不会挂接两次
	g_pIInlinePicSinkRP = pSink;
	return S_OK;
}

HRESULT KRepresentShell2::UnAdviseRepresent(IInlinePicEngineSink* pSink)
{
	if (pSink == g_pIInlinePicSinkRP)
		g_pIInlinePicSinkRP = NULL;
	return S_OK;
}

//##ModelId=3DD20C90004D
KRepresentShell2::KRepresentShell2()
{
	m_nLeft = 0;
	m_nTop = 0;
	memset(m_FontTable, 0, sizeof(m_FontTable));
}

//##ModelId=3DD20C900089
KRepresentShell2::~KRepresentShell2()
{
	m_UFont.Release();
	m_DirectDraw.Exit();
	m_ImageStore.Free();
	for (int i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].pFontObj)
		{
			m_FontTable[i].pFontObj->Release();
			m_FontTable[i].pFontObj = NULL;
		}
	}
}

//设置偏色列表
unsigned int KRepresentShell2::SetAdjustColorList(unsigned int* puColorList, unsigned int uCount)
{
	return m_ImageStore.SetAdjustColorList(puColorList, uCount);
}



int KRepresentShell2::Create(int nWidth, int nHeight, bool bFullScreen)
{
	m_UFont.Init();
	m_DirectDraw.Mode(bFullScreen, nWidth, nHeight);
	if (m_DirectDraw.Init())
	{
		m_Canvas.Init(nWidth, nHeight);
		m_ImageStore.Init();
		RIO_Set16BitImageFormat(m_DirectDraw.GetRGBBitMask16() == RGB_565);
		// 初始化Gdi+
		InitGdiplus();
		return 0;
	}
	return 1;
}

void KRepresentShell2::DrawUFont(const char* psFontName, unsigned int uSize, const char* psText, 
        int nCount, int nX, int nY, unsigned int Color, int nLineWidth, int nZ, unsigned int BorderColor)
{
	if (nCount <= 0)
		nCount = strlen(psText);
	if (nCount <= 0)
		return;
	int nFontId = m_UFont.FindFont(psFontName);
	if(nFontId < 0)
		return;
	if(!m_UFont.SetSize(nFontId, uSize))
		return;
	if (nZ != TEXT_IN_SINGLE_PLANE_COORD)
		CoordinateTransform(nX, nY, nZ);
	if (nLineWidth < (int)(uSize*2))
		nLineWidth = 0;
	UINT ab = (BorderColor & 0xff000000) >> 24;
	UINT al = (Color & 0xff000000) >> 24;
	if(!al)
		return;
	UINT clr = Color & 0xffffff;
	UINT bclr = BorderColor & 0xffffff;
	//KRColor clr;
	//KRColor bclr;
	//clr.Color_dw = Color;
	//bclr.Color_dw = BorderColor;
	int h = 0;
	int nCharIndex = 0;
	while(nCharIndex < nCount)
	{
		UINT uChar = psText[nCharIndex++];
		if(uChar == 0x0d || uChar == 0x0a)
		{
			if(uChar == 0x0d)
				nCharIndex++;
			h = 0;
			nY += uSize;
			continue;
		}
		if(uChar == 0x0b && nCharIndex < nCount)
		{
			uChar = g_ConvertOne2UCEChar(uChar, psText[nCharIndex++]);
		}
		else
			uChar = g_ConvertOne2UCEChar(uChar, 0);
		UCharPIXInfo* pPXInfo = m_UFont.LoadChar(nFontId, uChar, uSize);
		if(!pPXInfo)
			continue;
		if(ab && pPXInfo->str_buffer)
		{
			int start_x = nX + h + pPXInfo->str_bitmap_left;
			int start_y = nY + pPXInfo->baseline - pPXInfo->str_bitmap_top;
			for (int y = 0; y < (int)pPXInfo->str_rows; ++y)
			{
				for (int x = 0; x < (int)pPXInfo->str_width; ++x)
				{
					unsigned char pixel = pPXInfo->str_buffer[y * pPXInfo->str_pitch + x];
					if (pixel > 0)
					{
						UINT a = ab*pixel/255;
						//UINT c = (BorderColor & 0xffffff);
						//c = c | (a << 24);
						//DrawPointLine(start_x + x, start_x + x, start_y + y, c);
						//a = 31 - a/8;
						//m_Canvas.ClearAlpha(start_x + x, start_y + y, 1, 1, g_RGB(bclr.Color_b.r, bclr.Color_b.g, bclr.Color_b.b), a);
						m_Canvas.ClearAlpha(start_x + x, start_y + y, 1, 1, bclr, a);
					}
				}
			}
		}
		int y = nY + pPXInfo->baseline - pPXInfo->bitmap_top;
		for (int i = 0; i < pPXInfo->rows; ++i,++y)
		{
			int x = nX + h + pPXInfo->bitmap_left;
			for (int j = 0; j < pPXInfo->width; ++j,++x)
			{
				unsigned char pixel = pPXInfo->buffer[i * pPXInfo->pitch + j];  // pixel grayscale
				if(pixel > 0)
				{
					if(uSize <= 20)
					{
						UINT np = pixel + 96;
						if(np > 255)
							np = 255;
						pixel = np;
					}
					else if(uSize <= 30)
					{
						UINT np = pixel + 48;
						if(np > 255)
							np = 255;
						pixel = np;
					}
					else if(uSize <= 40)
					{
						UINT np = pixel + 24;
						if(np > 255)
							np = 255;
						pixel = np;
					}
					UINT a = al*pixel/255;
					//UINT c = (Color & 0xffffff);
					//c = c | (a << 24);
					//DrawPointLine(x,x,y,c);
					//a = 31 - a/8;
					//m_Canvas.ClearAlpha(x, y, 1, 1, g_RGB(clr.Color_b.r, clr.Color_b.g, clr.Color_b.b), a);
					m_Canvas.ClearAlpha(x, y, 1, 1, clr, a);
				}
			}
		}
		h += pPXInfo->glyph_advance;
		if(nLineWidth)
		{
			if(nCharIndex < nCount && (BYTE)psText[nCharIndex] != 0x0d && (BYTE)psText[nCharIndex] != 0x0a)
			{
				uChar = psText[nCharIndex];
				bool bSpecAdd = false;
				if(uChar == 0x0b && nCharIndex+1 < nCount)
				{
					uChar = g_ConvertOne2UCEChar(uChar, psText[nCharIndex+1]);
					bSpecAdd = true;
				}
				else
					uChar = g_ConvertOne2UCEChar(uChar, 0);
				pPXInfo = m_UFont.LoadChar(nFontId, uChar, uSize);
				if(!pPXInfo)
				{
					nCharIndex++;
					if(bSpecAdd)
						nCharIndex++;
					continue;
				}
				int nNextWidth = pPXInfo->glyph_advance;
				if (h + nNextWidth > nLineWidth)
				{
					h = 0;
					nY += uSize;
				}
			}
		}
	}
}

int	KRepresentShell2::GetEncodedTextLineCount(const char* pFontName, char* pBuffer, int nCount, int nWrapCharaNum,
					int& nMaxLineLen, int nFontSize, int nSkipLine /*= 0*/, int nNumLineLimit /*= 0*/,
					int bPicSingleLine /*= false*/, int* nAddEnterLen /*= NULL*/)
{
	nMaxLineLen = 0;
	if (nCount <= 0)
		nCount = strlen(pBuffer);
	if (nCount <= 0)
		return 0;
	int nFontId = m_UFont.FindFont(pFontName);
	if(nFontId < 0)
		return 0;
	if(!m_UFont.SetSize(nFontId, nFontSize))
		return 0;
	float fMaxLineLen = 0;
	float fNumChars = 0;
	int nNumLine = 0;
	int nPos = 0;
	unsigned char	cCode;

	if (nWrapCharaNum <= 0)
		nWrapCharaNum = 0x7fffffff;
	if (nSkipLine < 0)
		nSkipLine = 0;
	if (nNumLineLimit <= 0)
		nNumLineLimit = 0x7fffffff;

	bool bNextLine = false;
	float fNumNextLineChar = 0;
	int  nExtraLineForInlinePic = 0;
	while(nPos < nCount)
	{
		cCode = pBuffer[nPos];
		if (cCode == KTC_COLOR || cCode == KTC_BORDER_COLOR)
			nPos += 4;
		else if (cCode == KTC_INLINE_PIC)
		{
			WORD wPicIndex = *((WORD*)(pBuffer + nPos + 1));
			nPos += 1 + sizeof(WORD);
			if (g_pIInlinePicSinkRP)
			{
				int nWidth, nHeight;
				if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(wPicIndex, nWidth, nHeight)))
				{
					if (nHeight > nFontSize)
					{
						int nExtraLines = nHeight - nFontSize;
						nExtraLines = nExtraLines / nFontSize + ((nExtraLines % nFontSize) ? 1 : 0);
						if (nExtraLines > nExtraLineForInlinePic && !bPicSingleLine)
							nExtraLineForInlinePic = nExtraLines;
					}
					if (fNumChars + nWidth < nWrapCharaNum)
						fNumChars += nWidth;
					else if (fNumChars + nWidth == nWrapCharaNum || fNumChars == 0)
					{
						bNextLine = true;
						fNumChars += nWidth;
						bool bFoundEnter = false;
						int nNewPos = nPos;
						while(nNewPos < nCount)
						{
							BYTE bChar = (BYTE)pBuffer[nNewPos];
							if(bChar == KTC_ENTER)
							{
								bFoundEnter = true;
								break;
							}
							else if(bChar == KTC_COLOR || bChar == KTC_BORDER_COLOR)
							{
								nNewPos += 4;
							}
							else if(bChar == KTC_COLOR_RESTORE || bChar == KTC_BORDER_RESTORE)
							{
								nNewPos++;
							}
							else
								break;
						}
						if(nAddEnterLen)
						{
							if(nPos < nCount)
							{
								if(!bFoundEnter)
								{
									memmove(pBuffer+nPos+1, pBuffer+nPos, nCount-nPos);
									*nAddEnterLen = *nAddEnterLen + 1;
									pBuffer[nPos] = KTC_ENTER;
									nCount++;
									nPos++;
								}
								else
									nPos = nNewPos+1;
							}
						}
						else if(bFoundEnter)
							nPos = nNewPos+1;
					}
					else
					{
						bNextLine = true;
						fNumNextLineChar = nWidth;
						bool bFoundEnter = false;
						int nNewPos = nPos;
						while(nNewPos < nCount)
						{
							BYTE bChar = (BYTE)pBuffer[nNewPos];
							if(bChar == KTC_ENTER)
							{
								bFoundEnter = true;
								break;
							}
							else if(bChar == KTC_COLOR || bChar == KTC_BORDER_COLOR)
							{
								nNewPos += 4;
							}
							else if(bChar == KTC_COLOR_RESTORE || bChar == KTC_BORDER_RESTORE)
							{
								nNewPos++;
							}
							else
								break;
						}
						if(nAddEnterLen)
						{
							if(nPos < nCount)
							{
								if(!bFoundEnter)
								{
									memmove(pBuffer+nPos+1, pBuffer+nPos, nCount-nPos);
									*nAddEnterLen = *nAddEnterLen + 1;
									pBuffer[nPos] = KTC_ENTER;
									nCount++;
									nPos++;
								}
								else
									nPos = nNewPos+1;
							}
						}
						else if(bFoundEnter)
							nPos = nNewPos+1;
					}
				}
			}
		}
		else if (cCode == KTC_ENTER)
		{
			nPos ++;
			bNextLine = true;
		}
		else if (cCode != KTC_COLOR_RESTORE && cCode != KTC_BORDER_RESTORE)
		{
			UINT uChar = cCode;
			nPos ++;
			if(uChar == 0x0b && nPos < nCount)
			{
				uChar = g_ConvertOne2UCEChar(uChar, pBuffer[nPos++]);
			}
			else
				uChar = g_ConvertOne2UCEChar(uChar, 0);
			UCharPIXInfo* pPXInfo = m_UFont.LoadChar(nFontId, uChar, nFontSize);
			if(!pPXInfo)
				continue;
			fNumChars += pPXInfo->glyph_advance;
			if (fNumChars >= nWrapCharaNum)
			{
				bNextLine = true;
				bool bFoundEnter = false;
				int nNewPos = nPos;
				while(nNewPos < nCount)
				{
					BYTE bChar = (BYTE)pBuffer[nNewPos];
					if(bChar == KTC_ENTER)
					{
						bFoundEnter = true;
						break;
					}
					else if(bChar == KTC_COLOR || bChar == KTC_BORDER_COLOR)
					{
						nNewPos += 4;
					}
					else if(bChar == KTC_COLOR_RESTORE || bChar == KTC_BORDER_RESTORE)
					{
						nNewPos++;
					}
					else
						break;
				}
				if(nAddEnterLen)
				{
					if(nPos < nCount)
					{
						if(!bFoundEnter)
						{
							memmove(pBuffer+nPos+1, pBuffer+nPos, nCount-nPos);
							*nAddEnterLen = *nAddEnterLen + 1;
							pBuffer[nPos] = KTC_ENTER;
							nCount++;
							nPos++;
						}
						else
							nPos = nNewPos+1;
					}
				}
				else if(bFoundEnter)
					nPos = nNewPos+1;
			}
		}
		else
		{
			nPos++;
		}

		if (bNextLine)
		{
			if (nSkipLine > 0)
			{
				nSkipLine -= 1 + nExtraLineForInlinePic;
				if (nSkipLine < 0)
				{
					if (fMaxLineLen < fNumChars)
						fMaxLineLen = fNumChars;
					nNumLine += (-nSkipLine);
					if (nNumLine >= nNumLineLimit)
						break;
				}
			}
			else
			{
				if (fMaxLineLen < fNumChars)
					fMaxLineLen = fNumChars;
				nNumLine += 1 + nExtraLineForInlinePic;
				if (nNumLine >= nNumLineLimit)
					break;
			}
			nExtraLineForInlinePic = 0;
			fNumChars = (float)fNumNextLineChar;
			fNumNextLineChar = 0;
			bNextLine = false;
		}
	}
	if (nNumLine < nNumLineLimit && fNumChars && nSkipLine == 0)
	{
		if (fMaxLineLen < fNumChars)
			fMaxLineLen = fNumChars;
		nNumLine += 1 + nExtraLineForInlinePic;
	}

	nMaxLineLen = (int)(fMaxLineLen + (float)0.9999);
	return nNumLine;
}

void KRepresentShell2::DrawUFontRich(const char* psFontName, unsigned int uSize, KOutputTextParam* pParam,
		const char* psText, int nCount /*= KRF_ZERO_END*/, int nLineWidth /*= 0*/)
{
	if (nCount <= 0)
		nCount = strlen(psText);
	if (nCount <= 0 || pParam->nNumLine <= 0)
		return;
	int nFontId = m_UFont.FindFont(psFontName);
	if(nFontId < 0)
		return;
	if(!m_UFont.SetSize(nFontId, uSize))
		return;
	int nX = pParam->nX;
	int nY = pParam->nY;
	if (pParam->nZ != TEXT_IN_SINGLE_PLANE_COORD)
		CoordinateTransform(nX, nY, pParam->nZ);
	if (nLineWidth < (int)(uSize*2))
		nLineWidth = 0;
	UINT ab = (pParam->BorderColor & 0xff000000) >> 24;
	UINT al = (pParam->Color & 0xff000000) >> 24;
	if(!al)
		return;
	UINT uOriColor = pParam->Color & 0xffffff;
	UINT uOriBColor = pParam->BorderColor & 0xffffff;
	UINT uCurColor = uOriColor;
	UINT uCurBColor = uOriBColor;
	int nCharIndex = 0;
	int nLineLen = 0;
	if(pParam->nSkipLine > 0)
	{
		short nNextLine = 0;
		while(nCharIndex < nCount)
		{
			UINT uChar = psText[nCharIndex++];
			if(uChar == KTC_ENTER)
			{
				nNextLine++;
				nLineLen = 0;
				if(nNextLine >= pParam->nSkipLine)
					break;
			}
			else if(uChar == KTC_INLINE_PIC)
			{
				if(nCharIndex+1 < nCount)
				{
					WORD wPicIndex = *((WORD*)(psText + nCharIndex));
					nCharIndex += sizeof(WORD);
					if (g_pIInlinePicSinkRP)
					{
						int nWidth, nHeight;
						if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(wPicIndex, nWidth, nHeight)))
						{
							nLineLen += nWidth;
							if(nLineWidth)
							{
								if(nLineLen == nLineWidth)
								{
									if(nCharIndex < nCount && (BYTE)psText[nCharIndex] == KTC_ENTER)
										nCharIndex++;
									nNextLine++;
									nLineLen = 0;
									if(nNextLine >= pParam->nSkipLine)
										break;
								}
								else if(nLineLen > nLineWidth)
								{
									nNextLine++;
									nLineLen = nWidth;
									if(nNextLine >= pParam->nSkipLine)
									{
										nCharIndex -= 1 + sizeof(WORD);
										break;
									}
								}
							}
						}
					}
				}
				else
				{
					return;
				}
			}
			else if(uChar == KTC_COLOR)
			{
				if(nCharIndex+2 < nCount)
				{
					uCurColor = (((BYTE)psText[nCharIndex])<<16)
								| (((BYTE)psText[nCharIndex+1])<<8)
								| ((BYTE)psText[nCharIndex+2]);
					nCharIndex += 3;
				}
				else
				{
					return;
				}
			}
			else if(uChar == KTC_BORDER_COLOR)
			{
				if(nCharIndex+2 < nCount)
				{
					uCurBColor = (((BYTE)psText[nCharIndex])<<16)
								| (((BYTE)psText[nCharIndex+1])<<8)
								| ((BYTE)psText[nCharIndex+2]);
					nCharIndex += 3;
				}
				else
				{
					return;
				}
			}
			else if(uChar == KTC_COLOR_RESTORE)
			{
				uCurColor = uOriColor;
			}
			else if(uChar == KTC_BORDER_RESTORE)
			{
				uCurBColor = uOriBColor;
			}
			else
			{
				int nMoreChar = 0;
				if(uChar == 0x0b && nCharIndex < nCount)
				{
					nMoreChar = 1;
					uChar = g_ConvertOne2UCEChar(uChar, psText[nCharIndex++]);
				}
				else
					uChar = g_ConvertOne2UCEChar(uChar, 0);
				UCharPIXInfo* pPXInfo = m_UFont.LoadChar(nFontId, uChar, uSize);
				if(!pPXInfo)
					continue;
				nLineLen += pPXInfo->glyph_advance;
				if(nLineWidth)
				{
					if(nLineLen == nLineWidth)
					{
						if(nCharIndex < nCount && (BYTE)psText[nCharIndex] == KTC_ENTER)
							nCharIndex++;
						nNextLine++;
						nLineLen = 0;
						if(nNextLine >= pParam->nSkipLine)
							break;
					}
					else if(nLineLen > nLineWidth)
					{
						nNextLine++;
						nLineLen = pPXInfo->glyph_advance;
						if(nNextLine >= pParam->nSkipLine)
						{
							nCharIndex -= 1 + nMoreChar;
							break;
						}
					}
				}
			}
		}
	}
	int h = 0;
	short nLineDrawed = 0;
	nLineLen = 0;
	while(nCharIndex < nCount)
	{
		UINT uChar = psText[nCharIndex++];
		if(uChar == KTC_ENTER)
		{
			nLineDrawed++;
			nLineLen = 0;
			nY += uSize;
			if(nLineDrawed >= pParam->nNumLine)
				break;
		}
		else if(uChar == KTC_INLINE_PIC)
		{
			if(nCharIndex+1 < nCount)
			{
				WORD wPicIndex = *((WORD*)(psText + nCharIndex));
				nCharIndex += sizeof(WORD);
				if (g_pIInlinePicSinkRP)
				{
					int nWidth, nHeight;
					if (SUCCEEDED(g_pIInlinePicSinkRP->GetPicSize(wPicIndex, nWidth, nHeight)))
					{
						nLineLen += nWidth;
						if(nLineWidth)
						{
							if(nLineLen < nLineWidth)
							{
								int nPicY = nY - (nHeight - (int)uSize) / 2;
								g_pIInlinePicSinkRP->DrawPic(wPicIndex, nX+nLineLen-nWidth, nPicY, al);
							}
							else if(nLineLen == nLineWidth)
							{
								if(nCharIndex < nCount && (BYTE)psText[nCharIndex] == KTC_ENTER)
									nCharIndex++;
								nLineDrawed++;
								int nPicY = nY - (nHeight - (int)uSize) / 2;
								g_pIInlinePicSinkRP->DrawPic(wPicIndex, nX+nLineLen-nWidth, nPicY, al);
								nY += uSize;
								nLineLen = 0;
								if(nLineDrawed >= pParam->nNumLine)
									break;
							}
							else //if(nLineLen > nLineWidth)
							{
								nLineDrawed++;
								nY += uSize;
								nLineLen = 0;
								nCharIndex -= 1 + sizeof(WORD);
								if(nLineDrawed >= pParam->nNumLine)
									break;
							}
						}
						else
						{
							int nPicY = nY - (nHeight - (int)uSize) / 2;
							g_pIInlinePicSinkRP->DrawPic(wPicIndex, nX+nLineLen-nWidth, nPicY, al);
						}
					}
				}
			}
			else
			{
				return;
			}
		}
		else if(uChar == KTC_COLOR)
		{
			if(nCharIndex+2 < nCount)
			{
				uCurColor = (((BYTE)psText[nCharIndex])<<16)
							| (((BYTE)psText[nCharIndex+1])<<8)
							| ((BYTE)psText[nCharIndex+2]);
				nCharIndex += 3;
			}
			else
			{
				return;
			}
		}
		else if(uChar == KTC_BORDER_COLOR)
		{
			if(nCharIndex+2 < nCount)
			{
				uCurBColor = (((BYTE)psText[nCharIndex])<<16)
							| (((BYTE)psText[nCharIndex+1])<<8)
							| ((BYTE)psText[nCharIndex+2]);
				nCharIndex += 3;
			}
			else
			{
				return;
			}
		}
		else if(uChar == KTC_COLOR_RESTORE)
		{
			uCurColor = uOriColor;
		}
		else if(uChar == KTC_BORDER_RESTORE)
		{
			uCurBColor = uOriBColor;
		}
		else
		{
			int nMoreChar = 0;
			if(uChar == 0x0b && nCharIndex < nCount)
			{
				nMoreChar = 1;
				uChar = g_ConvertOne2UCEChar(uChar, psText[nCharIndex++]);
			}
			else
				uChar = g_ConvertOne2UCEChar(uChar, 0);
			UCharPIXInfo* pPXInfo = m_UFont.LoadChar(nFontId, uChar, uSize);
			if(!pPXInfo)
				continue;
			int nWidth = pPXInfo->glyph_advance;
			nLineLen += nWidth;
			if(nLineWidth)
			{
				if(nLineLen < nLineWidth)
				{
					DrawUFontChar(pPXInfo, uSize, nX+nLineLen-nWidth, nY, uCurColor, uCurBColor, al, ab);
				}
				else if(nLineLen == nLineWidth)
				{
					if(nCharIndex < nCount && (BYTE)psText[nCharIndex] == KTC_ENTER)
						nCharIndex++;
					nLineDrawed++;
					DrawUFontChar(pPXInfo, uSize, nX+nLineLen-nWidth, nY, uCurColor, uCurBColor, al, ab);
					nY += uSize;
					nLineLen = 0;
					if(nLineDrawed >= pParam->nNumLine)
						break;
				}
				else //if(nLineLen > nLineWidth)
				{
					nLineDrawed++;
					nLineLen = 0;
					nY += uSize;
					nCharIndex -= 1 + nMoreChar;
					if(nLineDrawed >= pParam->nNumLine)
						break;
				}
			}
			else
			{
				DrawUFontChar(pPXInfo, uSize, nX+nLineLen-nWidth, nY, uCurColor, uCurBColor, al, ab);
			}
		}
	}
}

void KRepresentShell2::DrawUFontChar(UCharPIXInfo* pPXInfo, unsigned int uSize, int nX, int nY,
				UINT Color, UINT BorderColor, UINT al, UINT ab)
{
	if(pPXInfo->str_buffer)
	{
		int start_x = nX + pPXInfo->str_bitmap_left;
		int start_y = nY + pPXInfo->baseline - pPXInfo->str_bitmap_top;
		for (int y = 0; y < (int)pPXInfo->str_rows; ++y)
		{
			for (int x = 0; x < (int)pPXInfo->str_width; ++x)
			{
				unsigned char pixel = pPXInfo->str_buffer[y * pPXInfo->str_pitch + x];
				if (pixel > 0)
				{
					UINT a = ab*pixel/255;
					m_Canvas.ClearAlpha(start_x + x, start_y + y, 1, 1, BorderColor, a);
				}
			}
		}
	}
	int y = nY + pPXInfo->baseline - pPXInfo->bitmap_top;
	for (int i = 0; i < pPXInfo->rows; ++i,++y)
	{
		int x = nX + pPXInfo->bitmap_left;
		for (int j = 0; j < pPXInfo->width; ++j,++x)
		{
			unsigned char pixel = pPXInfo->buffer[i * pPXInfo->pitch + j];  // pixel grayscale
			if(pixel > 0)
			{
				if(uSize <= 20)
				{
					UINT np = pixel + 96;
					if(np > 255)
						np = 255;
					pixel = np;
				}
				else if(uSize <= 30)
				{
					UINT np = pixel + 48;
					if(np > 255)
						np = 255;
					pixel = np;
				}
				else if(uSize <= 40)
				{
					UINT np = pixel + 24;
					if(np > 255)
						np = 255;
					pixel = np;
				}
				UINT a = al*pixel/255;
				m_Canvas.ClearAlpha(x, y, 1, 1, Color, a);
			}
		}
	}
}

bool KRepresentShell2::UpdateWindowSize(int nWidth, int nHeight)
{
	if(nWidth == m_Canvas.GetWidth() && nHeight == m_Canvas.GetHeight())
		return true;
	m_DirectDraw.Mode(m_DirectDraw.GetScreenMode() == 0, nWidth, nHeight);
	m_Canvas.Terminate();
	if (m_DirectDraw.Update())
	{
		m_Canvas.Init(nWidth, nHeight);
		return true;
	}
	return false;
}

//##ModelId=3DCA0B230317
bool KRepresentShell2::CreateAFont(const char* pszFontFile, CHARACTER_CODE_SET CharaSet, int nId)
{
	int nFirstFree = -1;
	for (int i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].pFontObj == NULL && nFirstFree == -1)
			nFirstFree = i;
		else if (m_FontTable[i].nId == nId)
		{
			nFirstFree = i;
			break;
		}
	}
	if (nFirstFree == -1 || pszFontFile == NULL || nId == 0)
		return false;

	if (m_FontTable[nFirstFree].pFontObj)
	{
		m_FontTable[nFirstFree].pFontObj->Release();
		m_FontTable[nFirstFree].pFontObj = NULL;
	}

	if (pszFontFile[0] == '#')
	{
		//共享已经打开的字库
		int nShareWithId = atoi(pszFontFile + 1);
		for (int j = 0; j < RS2_MAX_FONT_ITEM_NUM; j++)
		{
			if (nFirstFree != j &&	m_FontTable[j].nId == nShareWithId &&
				m_FontTable[j].pFontObj)
			{
				m_FontTable[nFirstFree].nId = nId;
				m_FontTable[nFirstFree].pFontObj = m_FontTable[j].pFontObj->Clone();
				return true;
			}
		}
		return false;
	}

	if ((m_FontTable[nFirstFree].pFontObj = new KFont2) == NULL)
		return false;

	m_FontTable[nFirstFree].pFontObj->Init(&m_Canvas);
	if (m_FontTable[nFirstFree].pFontObj->Load((LPSTR)pszFontFile/*, CharaSet*/))
	{
		m_FontTable[nFirstFree].nId = nId;
		m_FontTable[nFirstFree].pFontObj->SetOutputSize(nId, nId + 1);
	}
	else
	{
		m_FontTable[nFirstFree].pFontObj->Release();
		m_FontTable[nFirstFree].pFontObj = NULL;
	}
	
	return (m_FontTable[nFirstFree].pFontObj != NULL);
}

//##ModelId=3DCD8DEA01BB
unsigned int KRepresentShell2::CreateImage(const char* pszName, int nWidth, int nHeight, int nType, bool bNodeCache, bool bScrMode)
{
	return m_ImageStore.CreateImage(pszName, nWidth, nHeight, nType, bNodeCache, bScrMode);
}

void KRepresentShell2::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int uColor)
{
	KRULine line;
	line.Color.Color_dw = uColor;
	line.oPosition.nX = x1;
	line.oPosition.nY = y1;
	line.oEndPos.nX = x2;
	line.oEndPos.nY = y2;
	line.oEndPos.nZ = 0;
	DrawPrimitives(1, &line, RU_T_LINE, true);
	line.Color.Color_dw = uColor;
	line.oPosition.nX = x2;
	line.oPosition.nY = y2;
	line.oEndPos.nX = x3;
	line.oEndPos.nY = y3;
	line.oEndPos.nZ = 0;
	DrawPrimitives(1, &line, RU_T_LINE, true);
	line.Color.Color_dw = uColor;
	line.oPosition.nX = x3;
	line.oPosition.nY = y3;
	line.oEndPos.nX = x1;
	line.oEndPos.nY = y1;
	line.oEndPos.nZ = 0;
	DrawPrimitives(1, &line, RU_T_LINE, true);
}

void KRepresentShell2::DrawTriangleOnBuffer(int x1, int y1, float u1, float v1,
						int x2, int y2, float u2, float v2,
						int x3, int y3, float u3, float v3,
						int nNewWidth, int nNewHeight, UINT*& pDesData,
						BYTE *tex, int nWidth, int nHeight)
{
	float w1 = 1, w2 = 1, w3 = 1;
	int nTotalPixel = nNewWidth * nNewHeight;
	if (y2 < y1)
	{
		swap(y1, y2);
		swap(x1, x2);
		swap(u1, u2);
		swap(v1, v2);
		swap(w1, w2);
	}

	if (y3 < y1)
	{
		swap(y1, y3);
		swap(x1, x3);
		swap(u1, u3);
		swap(v1, v3);
		swap(w1, w3);
	}

	if (y3 < y2)
	{
		swap(y2, y3);
		swap(x2, x3);
		swap(u2, u3);
		swap(v2, v3);
		swap(w2, w3);
	}

	int dy1 = y2 - y1;
	int dx1 = x2 - x1;
	float dv1 = v2 - v1;
	float du1 = u2 - u1;
	float dw1 = w2 - w1;

	int dy2 = y3 - y1;
	int dx2 = x3 - x1;
	float dv2 = v3 - v1;
	float du2 = u3 - u1;
	float dw2 = w3 - w1;

	float tex_u, tex_v, tex_w;

	float dax_step = 0, dbx_step = 0,
		du1_step = 0, dv1_step = 0,
		du2_step = 0, dv2_step = 0,
		dw1_step=0, dw2_step=0;

	if (dy1) dax_step = dx1 / (float)abs(dy1);
	if (dy2) dbx_step = dx2 / (float)abs(dy2);

	if (dy1) du1_step = du1 / (float)abs(dy1);
	if (dy1) dv1_step = dv1 / (float)abs(dy1);
	if (dy1) dw1_step = dw1 / (float)abs(dy1);

	if (dy2) du2_step = du2 / (float)abs(dy2);
	if (dy2) dv2_step = dv2 / (float)abs(dy2);
	if (dy2) dw2_step = dw2 / (float)abs(dy2);
	if (dy1)
	{
		for (int i = y1; i < y2; i++)
		{
			int ax = x1 + (float)(i - y1) * dax_step;
			int bx = x1 + (float)(i - y1) * dbx_step;

			float tex_su = u1 + (float)(i - y1) * du1_step;
			float tex_sv = v1 + (float)(i - y1) * dv1_step;
			float tex_sw = w1 + (float)(i - y1) * dw1_step;

			float tex_eu = u1 + (float)(i - y1) * du2_step;
			float tex_ev = v1 + (float)(i - y1) * dv2_step;
			float tex_ew = w1 + (float)(i - y1) * dw2_step;

			if (ax > bx)
			{
				swap(ax, bx);
				swap(tex_su, tex_eu);
				swap(tex_sv, tex_ev);
				swap(tex_sw, tex_ew);
			}

			tex_u = tex_su;
			tex_v = tex_sv;
			tex_w = tex_sw;

			int span = bx - ax;
			if (span <= 0) continue;
			float tstep = 1.0f / (float)span;
			float t = 0.0f;

			for (int j = ax; j < bx; j++)
			{
				tex_u = (1.0f - t) * tex_su + t * tex_eu;
				tex_v = (1.0f - t) * tex_sv + t * tex_ev;
				tex_w = (1.0f - t) * tex_sw + t * tex_ew;
				UINT color = SampleColour(tex, nWidth, nHeight, tex_u / tex_w, tex_v / tex_w);
				UINT a = (color & 0xff000000) >> 24;
				if(a >= 8)
				{
					if(i*nNewWidth+j >= 0 && i*nNewWidth+j < nTotalPixel)
						pDesData[i*nNewWidth+j] = color;
				}
				t += tstep;
			}
		}
	}

	dy1 = y3 - y2;
	dx1 = x3 - x2;
	dv1 = v3 - v2;
	du1 = u3 - u2;
	dw1 = w3 - w2;

	if (dy1) dax_step = dx1 / (float)abs(dy1);
	if (dy2) dbx_step = dx2 / (float)abs(dy2);

	du1_step = 0, dv1_step = 0;
	if (dy1) du1_step = du1 / (float)abs(dy1);
	if (dy1) dv1_step = dv1 / (float)abs(dy1);
	if (dy1) dw1_step = dw1 / (float)abs(dy1);

	if (dy1)
	{
		for (int i = y2; i < y3; i++)
		{
			int ax = x2 + (float)(i - y2) * dax_step;
			int bx = x1 + (float)(i - y1) * dbx_step;

			float tex_su = u2 + (float)(i - y2) * du1_step;
			float tex_sv = v2 + (float)(i - y2) * dv1_step;
			float tex_sw = w2 + (float)(i - y2) * dw1_step;

			float tex_eu = u1 + (float)(i - y1) * du2_step;
			float tex_ev = v1 + (float)(i - y1) * dv2_step;
			float tex_ew = w1 + (float)(i - y1) * dw2_step;

			if (ax > bx)
			{
				swap(ax, bx);
				swap(tex_su, tex_eu);
				swap(tex_sv, tex_ev);
				swap(tex_sw, tex_ew);
			}

			tex_u = tex_su;
			tex_v = tex_sv;
			tex_w = tex_sw;

			int span = bx - ax;
			if (span <= 0) continue;
			float tstep = 1.0f / (float)span;
			float t = 0.0f;

			for (int j = ax; j < bx; j++)
			{
				tex_u = (1.0f - t) * tex_su + t * tex_eu;
				tex_v = (1.0f - t) * tex_sv + t * tex_ev;
				tex_w = (1.0f - t) * tex_sw + t * tex_ew;
				UINT color = SampleColour(tex, nWidth, nHeight, tex_u / tex_w, tex_v / tex_w);
				UINT a = (color & 0xff000000) >> 24;
				if(a >= 8)
				{
					if(i*nNewWidth+j >= 0 && i*nNewWidth+j < nTotalPixel)
						pDesData[i*nNewWidth+j] = color;
				}
				t += tstep;
			}
		}
	}
}

UINT* KRepresentShell2::ImageScaleBuffer(int nNewW, int nNewH, AlphaRecContent*& pSprData)
{
	UINT* pDesData = (UINT*)malloc(sizeof(UINT) * nNewW * nNewH);
	if(!pDesData)
		return NULL;
	memset(pDesData, 0, sizeof(UINT) * nNewW * nNewH);
	int x1 = 0;
	int y1 = 0;
	int x2 = nNewW;
	int y2 = 0;
	int x3 = 0;
	int y3 = nNewH;
	int x4 = x2;
	int y4 = y2;
	int x5 = x3;
	int y5 = y3;
	int x6 = nNewW;
	int y6 = nNewH;
	DrawTriangleOnBuffer(	x1, y1, 0, 0,
							x2, y2, 1, 0,
							x3, y3, 0, 1,
							nNewW, nNewH, pDesData,
							(BYTE*)pSprData->Data, pSprData->nWidth, pSprData->nHeight);
	DrawTriangleOnBuffer(	x4, y4, 1, 0,
							x5, y5, 0, 1,
							x6, y6, 1, 1,
							nNewW, nNewH, pDesData,
							(BYTE*)pSprData->Data, pSprData->nWidth, pSprData->nHeight);
	return pDesData;
}

BYTE* KRepresentShell2::ArrangeSpriteData(UINT* pBuffer, BYTE*& pPalette, int nWidth, int nHeight)
{
	BYTE* pDesData = (BYTE*)malloc(nWidth * nHeight * 3);
	if(!pDesData)
		return NULL;
	pPalette = (BYTE*)malloc(256 * 2);
	if(pPalette == NULL)
	{
		free(pDesData);
		pDesData = NULL;
		return NULL;
	}
	BYTE* pDesPtr = pDesData;
	UINT* pSrcPtr = pBuffer;
	int h = 0;
	int alpha = -1;
	int nPalCol = 0;
	UINT uPixelBatch;
	WORD color[256];
	while(h < nHeight)
	{
		int w = 0;
		while(w < nWidth)
		{
			if(alpha == -1)
			{
				UINT uColor = *(pSrcPtr++);
				alpha = (uColor & 0xff000000) >> 24;
				if(alpha < 8)
					alpha = 0;
				else if(alpha >= 248)
					alpha = 255;
				color[0] = uColor & 0xffff;
				uPixelBatch = 1;
				if(w + 1 >= nWidth)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							if(nPalCol == 0)
							{
								*(WORD*)pPalette = color[n];
								++nPalCol;
								*(pDesPtr++) = 0;
							}
							else
							{
								int i=0;
								while(i<nPalCol)
								{
									if(color[n] == *((WORD*)pPalette+i))
										break;
									++i;
								}
								if(i >= nPalCol)
								{
									*((WORD*)pPalette+i) = color[n];
									++nPalCol;
								}
								*(pDesPtr++) = i;
							}
						}
					}
					alpha = -1;
				}
			}
			else
			{
				UINT uColor = *(pSrcPtr++);
				int a = (uColor & 0xff000000) >> 24;
				if(a < 8)
					a = 0;
				else if(a >= 248)
					a = 255;
				if (a != alpha)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							if(nPalCol == 0)
							{
								*(WORD*)pPalette = color[n];
								++nPalCol;
								*(pDesPtr++) = 0;
							}
							else
							{
								int i=0;
								while(i<nPalCol)
								{
									if(color[n] == *((WORD*)pPalette+i))
										break;
									++i;
								}
								if(i >= nPalCol)
								{
									*((WORD*)pPalette+i) = color[n];
									++nPalCol;
								}
								*(pDesPtr++) = i;
							}
						}
					}
					alpha = a;
					color[0] = uColor & 0xffff;
					uPixelBatch = 1;
					if(w + 1 >= nWidth)
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								if(nPalCol == 0)
								{
									*(WORD*)pPalette = color[n];
									++nPalCol;
									*(pDesPtr++) = 0;
								}
								else
								{
									int i=0;
									while(i<nPalCol)
									{
										if(color[n] == *((WORD*)pPalette+i))
											break;
										++i;
									}
									if(i >= nPalCol)
									{
										*((WORD*)pPalette+i) = color[n];
										++nPalCol;
									}
									*(pDesPtr++) = i;
								}
							}
						}
						alpha = -1;
					}
				}
				else
				{
					color[uPixelBatch++] = uColor & 0xffff;
					if((uPixelBatch == 255) || (w + 1 >= nWidth))
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								if(nPalCol == 0)
								{
									*(WORD*)pPalette = color[n];
									++nPalCol;
									*(pDesPtr++) = 0;
								}
								else
								{
									int i=0;
									while(i<nPalCol)
									{
										if(color[n] == *((WORD*)pPalette+i))
											break;
										++i;
									}
									if(i >= nPalCol)
									{
										*((WORD*)pPalette+i) = color[n];
										++nPalCol;
									}
									*(pDesPtr++) = i;
								}
							}
						}
						alpha = -1;
					}
				}
			}
			++w;
		}
		++h;
	}
	UINT uSize = (pDesPtr - pDesData);
	BYTE* pFinalDes = (BYTE*)malloc(uSize);
	if(!pFinalDes)
	{
		free(pDesData);
		pDesData = NULL;
		free(pPalette);
		pPalette = NULL;
		return NULL;
	}
	memcpy(pFinalDes, pDesData, uSize);
	free(pDesData);
	pDesData = NULL;
	return pFinalDes;
}

BYTE* KRepresentShell2::ArrangeSpriteData32b(UINT* pBuffer, BYTE*& pPalette, int nWidth, int nHeight)
{
	BYTE* pDesData = (BYTE*)malloc(nWidth * nHeight * 3);
	if(!pDesData)
		return NULL;
	pPalette = (BYTE*)malloc(256 * 3);
	if(pPalette == NULL)
	{
		free(pDesData);
		pDesData = NULL;
		return NULL;
	}
	BYTE* pDesPtr = pDesData;
	UINT* pSrcPtr = pBuffer;
	int h = 0;
	int alpha = -1;
	int nPalCol = 0;
	UINT uPixelBatch;
	BYTE color[256][3];
	while(h < nHeight)
	{
		int w = 0;
		while(w < nWidth)
		{
			if(alpha == -1)
			{
				UINT uColor = *(pSrcPtr++);
				alpha = (uColor & 0xff000000) >> 24;
				if(alpha < 8)
					alpha = 0;
				else if(alpha >= 248)
					alpha = 255;
				color[0][0] = (uColor & 0xff0000) >> 16;
				color[0][1] = (uColor & 0xff00) >> 8;
				color[0][2] = uColor & 0xff;
				uPixelBatch = 1;
				if(w + 1 >= nWidth)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							if(nPalCol == 0)
							{
								*pPalette = color[n][0];
								*(pPalette+1) = color[n][1];
								*(pPalette+2) = color[n][2];
								++nPalCol;
								*(pDesPtr++) = 0;
							}
							else
							{
								int i=0;
								while(i<nPalCol)
								{
									if(color[n][0] == *(pPalette+i*3)
									&& color[n][1] == *(pPalette+i*3+1)
									&& color[n][2] == *(pPalette+i*3+2))
										break;
									++i;
								}
								if(i >= nPalCol)
								{
									*(pPalette+i*3) = color[n][0];
									*(pPalette+i*3+1) = color[n][1];
									*(pPalette+i*3+2) = color[n][2];
									++nPalCol;
								}
								*(pDesPtr++) = i;
							}
						}
					}
					alpha = -1;
				}
			}
			else
			{
				UINT uColor = *(pSrcPtr++);
				int a = (uColor & 0xff000000) >> 24;
				if(a < 8)
					a = 0;
				else if(a >= 248)
					a = 255;
				if (a != alpha)
				{
					*(pDesPtr++) = uPixelBatch;
					*(pDesPtr++) = alpha;
					if(alpha)
					{
						for(UINT n=0; n<uPixelBatch; ++n)
						{
							if(nPalCol == 0)
							{
								*pPalette = color[n][0];
								*(pPalette+1) = color[n][1];
								*(pPalette+2) = color[n][2];
								++nPalCol;
								*(pDesPtr++) = 0;
							}
							else
							{
								int i=0;
								while(i<nPalCol)
								{
									if(color[n][0] == *(pPalette+i*3)
									&& color[n][1] == *(pPalette+i*3+1)
									&& color[n][2] == *(pPalette+i*3+2))
										break;
									++i;
								}
								if(i >= nPalCol)
								{
									*(pPalette+i*3) = color[n][0];
									*(pPalette+i*3+1) = color[n][1];
									*(pPalette+i*3+2) = color[n][2];
									++nPalCol;
								}
								*(pDesPtr++) = i;
							}
						}
					}
					alpha = a;
					color[0][0] = (uColor & 0xff0000) >> 16;
					color[0][1] = (uColor & 0xff00) >> 8;
					color[0][2] = uColor & 0xff;
					uPixelBatch = 1;
					if(w + 1 >= nWidth)
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								if(nPalCol == 0)
								{
									*pPalette = color[n][0];
									*(pPalette+1) = color[n][1];
									*(pPalette+2) = color[n][2];
									++nPalCol;
									*(pDesPtr++) = 0;
								}
								else
								{
									int i=0;
									while(i<nPalCol)
									{
										if(color[n][0] == *(pPalette+i*3)
										&& color[n][1] == *(pPalette+i*3+1)
										&& color[n][2] == *(pPalette+i*3+2))
											break;
										++i;
									}
									if(i >= nPalCol)
									{
										*(pPalette+i*3) = color[n][0];
										*(pPalette+i*3+1) = color[n][1];
										*(pPalette+i*3+2) = color[n][2];
										++nPalCol;
									}
									*(pDesPtr++) = i;
								}
							}
						}
						alpha = -1;
					}
				}
				else
				{
					color[uPixelBatch][0] = (uColor & 0xff0000) >> 16;
					color[uPixelBatch][1] = (uColor & 0xff00) >> 8;
					color[uPixelBatch][2] = uColor & 0xff;
					++uPixelBatch;
					if((uPixelBatch == 255) || (w + 1 >= nWidth))
					{
						*(pDesPtr++) = uPixelBatch;
						*(pDesPtr++) = alpha;
						if(alpha)
						{
							for(UINT n=0; n<uPixelBatch; ++n)
							{
								if(nPalCol == 0)
								{
									*pPalette = color[n][0];
									*(pPalette+1) = color[n][1];
									*(pPalette+2) = color[n][2];
									++nPalCol;
									*(pDesPtr++) = 0;
								}
								else
								{
									int i=0;
									while(i<nPalCol)
									{
										if(color[n][0] == *(pPalette+i*3)
										&& color[n][1] == *(pPalette+i*3+1)
										&& color[n][2] == *(pPalette+i*3+2))
											break;
										++i;
									}
									if(i >= nPalCol)
									{
										*(pPalette+i*3) = color[n][0];
										*(pPalette+i*3+1) = color[n][1];
										*(pPalette+i*3+2) = color[n][2];
										++nPalCol;
									}
									*(pDesPtr++) = i;
								}
							}
						}
						alpha = -1;
					}
				}
			}
			++w;
		}
		++h;
	}
	UINT uSize = (pDesPtr - pDesData);
	BYTE* pFinalDes = (BYTE*)malloc(uSize);
	if(!pFinalDes)
	{
		free(pDesData);
		pDesData = NULL;
		free(pPalette);
		pPalette = NULL;
		return NULL;
	}
	memcpy(pFinalDes, pDesData, uSize);
	free(pDesData);
	pDesData = NULL;
	return pFinalDes;
}

void KRepresentShell2::DrawScaleSprite(BYTE*& pSprite, BYTE*& pPal, int nX, int nY, KRUImage*& pTemp, SPRHEAD*& pSprHeader,
							SPRFRAME*& pFrame, int nWidth, int nHeight)
{
	if (pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_REF_SPOT)
	{
#define CENTERX		160
#define	CENTERY		192
		int nCenterX = pSprHeader->CenterX * pTemp->fScaleX;
		int nCenterY = pSprHeader->CenterY * pTemp->fScaleY;
		if (nCenterX || nCenterY)
		{
			nX -= nCenterX;
			nY -= nCenterY;
		}
		else if (pSprHeader->Width > CENTERX)
		{
			nX -= CENTERX * pTemp->fScaleX;
			nY -= CENTERY * pTemp->fScaleY;
		}
	}
	if ((pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_FRAME_DRAW) == 0)
	{
		nX += pFrame->OffsetX * pTemp->fScaleX;
		nY += pFrame->OffsetY * pTemp->fScaleY;
	}
	switch(pTemp->bRenderStyle)
	{
		case IMAGE_RENDER_STYLE_ALPHA:
		case IMAGE_RENDER_STYLE_ALPHA_NOT_BE_LIT:
		case IMAGE_RENDER_STYLE_3LEVEL:
			if(pSprHeader->Reserved[1])
			{
				UINT nColor = pTemp->Color.Color_dw & 0xffffff;
				m_Canvas.DrawSpriteBlendColor(nX, nY, nWidth, nHeight,
					pSprite, pPal, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
			}
			else
				m_Canvas.DrawSpriteAlpha(nX, nY, nWidth, nHeight,
					pSprite, pPal, pTemp->Color.Color_b.a);
			break;
		case IMAGE_RENDER_STYLE_OPACITY:
			m_Canvas.DrawSprite(nX, nY, nWidth, nHeight, pSprite, pPal);
			break;
		case IMAGE_RENDER_STYLE_ALPHA_COLOR_ADJUST:
			UINT nColor = pTemp->Color.Color_dw & 0xffffff;
			if(pSprHeader->Reserved[1])
			{
				m_Canvas.DrawSpriteBlendColor(nX, nY, nWidth, nHeight,
					pSprite, pPal, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
			}
			else
			{
				if(nColor == 0)
				{
					m_Canvas.DrawSpriteAlpha(nX, nY, nWidth, nHeight,
						pSprite, pPal, pTemp->Color.Color_b.a);
				}
				else
				{
					m_Canvas.DrawSpriteBlendColor(nX, nY, nWidth, nHeight,
						pSprite, pPal, pTemp->Color.Color_b.a, nColor);
				}
			}
			break;
	}
}

//##ModelId=3DB69FE401DA
void KRepresentShell2::DrawPrimitives(int nPrimitiveCount, KRepresentUnit* pPrimitives, unsigned int uGenre, int bSinglePlaneCoord, int bFindById)
{
	int i = 0;
	switch(uGenre)
	{
	case RU_T_IMAGE:
		{
			KRUImage* pTemp = (KRUImage*)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				switch(pTemp->nType)
				{
				case ISI_T_SPR:
					{
						SPRFRAME* pFrame;
						SPRHEAD* pSprHeader = (SPRHEAD*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, (void*&)pFrame, bFindById);
						if (pSprHeader == NULL)
							break;

						int nX = pTemp->oPosition.nX;
						int nY = pTemp->oPosition.nY;
						if (bSinglePlaneCoord == false)
							CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
						if(pTemp->fScaleX > 0.0f && pTemp->fScaleY > 0.0f
						&& (pTemp->fScaleX != 1.0f || pTemp->fScaleY != 1.0f))
						{
							int nRet;
							AlphaRecContent* pSprData = (AlphaRecContent*)m_ImageStore.CreateSprData(
								nRet, pFrame->Width, pFrame->Height, pTemp->uImage,
								pTemp->nISPosition, pTemp->nFrame, pSprHeader->Frames);
							if(pSprData)
							{
								if(!nRet)
								{
									if(g_pDirectDraw->GetRGBBitCount() != 32)
										FillSprData(pSprData->Data, pSprData->nWidth, pSprData->nHeight,
										pFrame->Sprite, GET_SPR_PALETTE(pSprHeader));
									else
										FillSprData32b(pSprData->Data, pSprData->nWidth, pSprData->nHeight,
										pFrame->Sprite, GET_SPR_PALETTE(pSprHeader));
								}
								int nNewW = pSprData->nWidth * pTemp->fScaleX;
								int nNewH = pSprData->nHeight * pTemp->fScaleY;
								BYTE* pPal = NULL;
								BYTE* pSprite = m_ImageStore.FindScaleData(pTemp->uImage, pTemp->nISPosition,
										pTemp->nFrame, nNewW, nNewH, pPal);
								if(!pSprite)
								{
									UINT* pScaleBuff = ImageScaleBuffer(nNewW, nNewH, pSprData);
									if(pScaleBuff)
									{
										if(g_pDirectDraw->GetRGBBitCount() != 32)
										pSprite = ArrangeSpriteData(pScaleBuff, pPal, nNewW, nNewH);
										else
										pSprite = ArrangeSpriteData32b(pScaleBuff, pPal, nNewW, nNewH);
										if(pSprite)
											m_ImageStore.AddScaleData(pTemp->uImage, pTemp->nISPosition,
											pTemp->nFrame, nNewW, nNewH, pSprite, pPal);
										free(pScaleBuff);
										pScaleBuff = NULL;
									}
								}
								if(pSprite)
									DrawScaleSprite(pSprite, pPal, nX, nY, pTemp, pSprHeader, pFrame, nNewW, nNewH);
								break;
							}
						}
						if (pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_REF_SPOT)
						{
					//****to be modify****
#define CENTERX		160
#define	CENTERY		192
							int nCenterX = pSprHeader->CenterX;
							int nCenterY = pSprHeader->CenterY;
							if (nCenterX || nCenterY)
							{
								nX -= nCenterX;
								nY -= nCenterY;
							}
							else if (pSprHeader->Width > CENTERX)
							{
								nX -= CENTERX;
								nY -= CENTERY;
							}
						}
						//****to be modify end****

//						Check Current Draw Device??;
						if ((pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_FRAME_DRAW) == 0)
						{
							nX += pFrame->OffsetX;
							nY += pFrame->OffsetY;
						}

						char* pPalette = GET_SPR_PALETTE(pSprHeader);

						switch(pTemp->bRenderStyle)
						{
						case IMAGE_RENDER_STYLE_ALPHA:
						case IMAGE_RENDER_STYLE_ALPHA_NOT_BE_LIT:
						case IMAGE_RENDER_STYLE_3LEVEL:
							if(pSprHeader->Reserved[1])
							{
								UINT nColor = pTemp->Color.Color_dw & 0xffffff;
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
								m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
							break;
						case IMAGE_RENDER_STYLE_OPACITY:
							m_Canvas.DrawSprite(nX, nY, pFrame->Width, pFrame->Height,
								pFrame->Sprite, pPalette);
							break;
						case IMAGE_RENDER_STYLE_BORDER:
//							m_Canvas.DrawSpriteBorder(nX, nY, pFrame->Width, pFrame->Height,
//								///g_RGB(pTemp->Color.Color_b.r, pTemp->Color.Color_b.g, pTemp->Color.Color_b.b),
//								g_RGB(200, 200, 0),
//								pFrame->Sprite);
							break;
						case IMAGE_RENDER_STYLE_ALPHA_COLOR_ADJUST:
							//pPalette = m_ImageStore.GetAdjustColorPalette(pTemp->nISPosition, pTemp->Color.Color_dw);
							UINT nColor = pTemp->Color.Color_dw & 0xffffff;
							if(pSprHeader->Reserved[1])
							{
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
							{
								if(nColor == 0)
								{
									m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
								}
								else
								{
									m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor);
								}
							}
							break;
						}
					}
					break;
				case ISI_T_BITMAP16:
					{
						void* pFrame;
						KSGImageContent* pBitmap = (KSGImageContent *)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							0, ISI_T_BITMAP16, pFrame, bFindById);
						if (pBitmap)
						{
							int nX = pTemp->oPosition.nX;
							int nY = pTemp->oPosition.nY;
							if (bSinglePlaneCoord == false)
								CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
							m_Canvas.DrawBitmap16(nX, nY,
								pBitmap->nWidth, pBitmap->nHeight, pBitmap->Data);
						}
					}
					break;
				case ISI_T_DRAWINGRC:
					{
						void* pFrame;
						AlphaRecContent* pBitmap = (AlphaRecContent *)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							0, ISI_T_DRAWINGRC, pFrame);
						if (pBitmap)
						{
							int nX = pTemp->oPosition.nX;
							int nY = pTemp->oPosition.nY;
							if (bSinglePlaneCoord == false)
								CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
							BYTE* pData = NULL;
							int nNewW = pBitmap->nWidth;
							int nNewH = pBitmap->nHeight;
							if(pTemp->fScaleX > 0.0f && pTemp->fScaleY > 0.0f
							&& (pTemp->fScaleX != 1.0f || pTemp->fScaleY != 1.0f))
							{
								nNewW = pBitmap->nWidth * pTemp->fScaleX;
								nNewH = pBitmap->nHeight * pTemp->fScaleY;
								BYTE* pPal = NULL;
								BYTE* pSprite = m_ImageStore.FindScaleData(pTemp->uImage, pTemp->nISPosition,
										pTemp->nFrame, nNewW, nNewH, pPal);
								if(!pSprite)
								{
									UINT* pScaleBuff = ImageScaleBuffer(nNewW, nNewH, pBitmap);
									if(pScaleBuff)
									{
										m_ImageStore.AddScaleData(pTemp->uImage, pTemp->nISPosition,
										pTemp->nFrame, nNewW, nNewH, (BYTE*)pScaleBuff, NULL);
										pData = (BYTE*)pScaleBuff;
									}
									else
										break;
								}
								else
									pData = pSprite;
							}
							if(pData)
							m_Canvas.DrawAlphaRecImage(nX, nY, nNewW, nNewH,
								pData, pTemp->Color.Color_b.a, pTemp->bRenderStyle == IMAGE_RENDER_STYLE_OPACITY, pBitmap->bScrMode);
							else
							m_Canvas.DrawAlphaRecImage(nX, nY, pBitmap->nWidth, pBitmap->nHeight,
								pBitmap->Data, pTemp->Color.Color_b.a, pTemp->bRenderStyle == IMAGE_RENDER_STYLE_OPACITY, pBitmap->bScrMode);
						}
					}
					break;
				}
			}
		}
		break;
	case RU_T_IMAGE_4:
		{
			KRUImage4* pTemp = (KRUImage4*)pPrimitives;
			RECT rcOld;
			m_Canvas.GetClipRect(&rcOld);
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				//_ASSERT(pTemp->nType == ISI_T_SPR);
					{
						SPRFRAME* pFrame;
						SPRHEAD* pSprHeader = (SPRHEAD*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, (void*&)pFrame, bFindById);
						if (pSprHeader == NULL)
							break;

						int nX = pTemp->oPosition.nX;
						int nY = pTemp->oPosition.nY;
						if (!bSinglePlaneCoord)
							CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
						if ((pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_FRAME_DRAW) == 0)
						{
							nX += pFrame->OffsetX;
							nY += pFrame->OffsetY;
						}

						RECT	rc;
						rc.left  = nX;
						rc.top   = nY;
						nX -= pTemp->oImgLTPos.nX;
						nY -= pTemp->oImgLTPos.nY;
						rc.right = nX + pTemp->oImgRBPos.nX;
						rc.bottom= nY + pTemp->oImgRBPos.nY;
						if (rc.left < rcOld.left)
							rc.left = rcOld.left;
						if (rc.right > rcOld.right)
							rc.right = rcOld.right;
						if (rc.top < rcOld.top)
							rc.top = rcOld.top;
						if (rc.bottom > rcOld.bottom)
							rc.bottom = rcOld.bottom;
						m_Canvas.SetClipRect(&rc);

						char* pPalette = GET_SPR_PALETTE(pSprHeader);

						switch(pTemp->bRenderStyle)
						{
						case IMAGE_RENDER_STYLE_ALPHA:
						case IMAGE_RENDER_STYLE_ALPHA_NOT_BE_LIT:
						case IMAGE_RENDER_STYLE_3LEVEL:
							if(pSprHeader->Reserved[1])
							{
								UINT nColor = pTemp->Color.Color_dw & 0xffffff;
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
								m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
							break;
						case IMAGE_RENDER_STYLE_OPACITY:
							m_Canvas.DrawSprite(nX, nY, pFrame->Width, pFrame->Height,
								pFrame->Sprite, pPalette);
							break;
						case IMAGE_RENDER_STYLE_BORDER:
//							m_Canvas.DrawSpriteBorder(nX, nY, pFrame->Width, pFrame->Height,
//								///g_RGB(pTemp->Color.Color_b.r, pTemp->Color.Color_b.g, pTemp->Color.Color_b.b),
//								g_RGB(200, 200, 0),
//								pFrame->Sprite);
							break;
						case IMAGE_RENDER_STYLE_ALPHA_COLOR_ADJUST:
							//pPalette = m_ImageStore.GetAdjustColorPalette(pTemp->nISPosition, pTemp->Color.Color_dw);
							UINT nColor = pTemp->Color.Color_dw & 0xffffff;
							if(pSprHeader->Reserved[1])
							{
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
							{
								if(nColor == 0)
								{
									m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
								}
								else
								{
									m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor);
								}
							}
							break;
						}
					}
			}
			m_Canvas.SetClipRect(&rcOld);
		}
		break;
	case RU_T_IMAGE_PART:
		{
			KRUImagePart* pTemp = (KRUImagePart *)pPrimitives;
			RECT rcOld;
			m_Canvas.GetClipRect(&rcOld);
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{				
				switch(pTemp->nType)
				{
				case ISI_T_SPR:
					{
						SPRFRAME* pFrame;
						SPRHEAD* pSprHeader = (SPRHEAD*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, (void*&)pFrame, bFindById);
						if (pSprHeader == NULL)
							break;

						int nX = pTemp->oPosition.nX;
						int nY = pTemp->oPosition.nY;
						if (bSinglePlaneCoord == false)
							CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
						if (pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_REF_SPOT)
						{
							nX -= pSprHeader->CenterX;
							nY -= pSprHeader->CenterY;
						}
//						Check Current Draw Device??;

						// Clipper
						if ((pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_FRAME_DRAW) == 0)
						{
							nX += pFrame->OffsetX;
							nY += pFrame->OffsetY;
						}

						RECT	rc;
						rc.left  = nX;
						rc.top   = nY;
						nX -= pTemp->oImgLTPos.nX;
						nY -= pTemp->oImgLTPos.nY;
						rc.right = nX + pTemp->oImgRBPos.nX;
						rc.bottom= nY + pTemp->oImgRBPos.nY;
						if (rc.left < rcOld.left)
							rc.left = rcOld.left;
						if (rc.right > rcOld.right)
							rc.right = rcOld.right;
						if (rc.top < rcOld.top)
							rc.top = rcOld.top;
						if (rc.bottom > rcOld.bottom)
							rc.bottom = rcOld.bottom;
						m_Canvas.SetClipRect(&rc);

						char* pPalette = GET_SPR_PALETTE(pSprHeader);

						switch(pTemp->bRenderStyle)
						{
						case IMAGE_RENDER_STYLE_ALPHA:
						case IMAGE_RENDER_STYLE_ALPHA_NOT_BE_LIT:
						case IMAGE_RENDER_STYLE_3LEVEL:
							if(pSprHeader->Reserved[1])
							{
								UINT nColor = pTemp->Color.Color_dw & 0xffffff;
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
								m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
							break;
						case IMAGE_RENDER_STYLE_OPACITY:
							m_Canvas.DrawSprite(nX, nY, pFrame->Width, pFrame->Height,
								pFrame->Sprite, pPalette);
							break;
						case IMAGE_RENDER_STYLE_ALPHA_COLOR_ADJUST:
							//pPalette = m_ImageStore.GetAdjustColorPalette(pTemp->nISPosition, pTemp->Color.Color_dw);
							UINT nColor = pTemp->Color.Color_dw & 0xffffff;
							if(pSprHeader->Reserved[1])
							{
								m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
									pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor, CBM_COLOR, TRUE);
							}
							else
							{
								if(nColor == 0)
								{
									m_Canvas.DrawSpriteAlpha(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a);
								}
								else
								{
									m_Canvas.DrawSpriteBlendColor(nX, nY, pFrame->Width, pFrame->Height,
										pFrame->Sprite, pPalette, pTemp->Color.Color_b.a, nColor);
								}
							}
							break;
						}
					}
					break;
				case ISI_T_BITMAP16:
					{
						void* pFrame;
						KSGImageContent* pBitmap = (KSGImageContent*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, pFrame, bFindById);
						if (pBitmap)
						{
							int nX = pTemp->oPosition.nX;
							int nY = pTemp->oPosition.nY;
							if (bSinglePlaneCoord == false)
								CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
	
							RECT	rc;
							rc.left  = nX;
							rc.top   = nY;
							nX -= pTemp->oImgLTPos.nX;
							nY -= pTemp->oImgLTPos.nY;
							rc.right = nX + pTemp->oImgRBPos.nX;
							rc.bottom= nY + pTemp->oImgRBPos.nY;
							if (rc.left < rcOld.left)
								rc.left = rcOld.left;
							if (rc.right > rcOld.right)
								rc.right = rcOld.right;
							if (rc.top < rcOld.top)
								rc.top = rcOld.top;
							if (rc.bottom > rcOld.bottom)
								rc.bottom = rcOld.bottom;
							m_Canvas.SetClipRect(&rc);

							m_Canvas.DrawBitmap16(nX, nY,
								pBitmap->nWidth, pBitmap->nHeight, pBitmap->Data);
						}
					}
					break;
					case ISI_T_DRAWINGRC:
					{
						void* pFrame;
						AlphaRecContent* pBitmap = (AlphaRecContent *)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							0, ISI_T_DRAWINGRC, pFrame);
						if (pBitmap)
						{
							int nX = pTemp->oPosition.nX;
							int nY = pTemp->oPosition.nY;
							if (bSinglePlaneCoord == false)
								CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
							RECT	rc;
							rc.left  = nX;
							rc.top   = nY;
							nX -= pTemp->oImgLTPos.nX;
							nY -= pTemp->oImgLTPos.nY;
							rc.right = nX + pTemp->oImgRBPos.nX;
							rc.bottom= nY + pTemp->oImgRBPos.nY;
							if (rc.left < rcOld.left)
								rc.left = rcOld.left;
							if (rc.right > rcOld.right)
								rc.right = rcOld.right;
							if (rc.top < rcOld.top)
								rc.top = rcOld.top;
							if (rc.bottom > rcOld.bottom)
								rc.bottom = rcOld.bottom;
							m_Canvas.SetClipRect(&rc);
							m_Canvas.DrawAlphaRecImage(nX, nY, pBitmap->nWidth, pBitmap->nHeight,
								pBitmap->Data, pTemp->Color.Color_b.a, pTemp->bRenderStyle == IMAGE_RENDER_STYLE_OPACITY, pBitmap->bScrMode);
						}
					}
					break;
				}
			}
			m_Canvas.SetClipRect(&rcOld);
		}
		break;
	case RU_T_POINT:
		{
			KRUPoint* pTemp = (KRUPoint *)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				int nX = pTemp->oPosition.nX;
				int nY = pTemp->oPosition.nY;
				if (!bSinglePlaneCoord)
					CoordinateTransform(nX, nY, pTemp->oPosition.nZ);
				int nColor = pTemp->Color.Color_dw & 0xffffff;
				m_Canvas.DrawPixelAlpha(nX, nY, nColor, pTemp->Color.Color_b.a);
			}
		}
		break;
	case RU_T_LINE:
		{
			KRULine* pTemp = (KRULine *)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				int	nX1 = pTemp->oPosition.nX;
				int nY1 = pTemp->oPosition.nY;
					
				int nX2 = pTemp->oEndPos.nX;
				int nY2 = pTemp->oEndPos.nY;
				if (!bSinglePlaneCoord)
				{
					CoordinateTransform(nX1, nY1, pTemp->oPosition.nZ);
					CoordinateTransform(nX2, nY2, pTemp->oEndPos.nZ);
				}
				int nColor = pTemp->Color.Color_dw & 0xffffff;
				m_Canvas.DrawLineAlpha(nX1, nY1, nX2, nY2, nColor, pTemp->Color.Color_b.a);
			}
		}
		break;
	case RU_T_RECT:
		{
			KRURect* pTemp = (KRURect *)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				int	nX1 = pTemp->oPosition.nX;
				int nY1 = pTemp->oPosition.nY;
					
				int nX2 = pTemp->oEndPos.nX;
				int nY2 = pTemp->oEndPos.nY;
				if (!bSinglePlaneCoord)
				{
					CoordinateTransform(nX1, nY1, pTemp->oPosition.nZ);
					CoordinateTransform(nX2, nY2, pTemp->oEndPos.nZ);
				}
				int nColor = pTemp->Color.Color_dw & 0xffffff;
				m_Canvas.DrawLineAlpha(nX1, nY1, nX2, nY1, nColor, pTemp->Color.Color_b.a);	//上边
				m_Canvas.DrawLineAlpha(nX1, nY2, nX2, nY2, nColor, pTemp->Color.Color_b.a);	//下边
				m_Canvas.DrawLineAlpha(nX1, nY1, nX1, nY2, nColor, pTemp->Color.Color_b.a);	//左边
				m_Canvas.DrawLineAlpha(nX2, nY1, nX2, nY2, nColor, pTemp->Color.Color_b.a);	//右边
			}
		}
		break;
	case RU_T_SHADOW:
		{
			KRUShadow* pTemp =(KRUShadow *)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{				
				int nX1 = pTemp->oPosition.nX;
				int nY1 = pTemp->oPosition.nY;
				int nX2 = pTemp->oEndPos.nX;
				int	nY2 = pTemp->oEndPos.nY;
				if (!bSinglePlaneCoord)
				{
					CoordinateTransform(nX1, nY1, pTemp->oPosition.nZ);
					CoordinateTransform(nX2, nY2, pTemp->oEndPos.nZ);
				}
				int nColor = pTemp->Color.Color_dw & 0xffffff;
				if(pTemp->bUseNewAlpha)
					m_Canvas.ClearAlpha(nX1, nY1, nX2 - nX1, nY2 - nY1, nColor, pTemp->Color.Color_b.a);
				else
				{
					if(pTemp->Color.Color_b.a < 32)
					{
						int nA = (32 - pTemp->Color.Color_b.a)*8;
						if(nA > 255)
							nA = 255;
						m_Canvas.ClearAlpha(nX1, nY1, nX2 - nX1, nY2 - nY1, nColor, nA);
					}
				}
			}
		}
		break;
	case RU_T_IMAGE_STRETCH:
		if (bSinglePlaneCoord)
		{
			KRUImageStretch* pTemp = (KRUImage*)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				if (pTemp->nType == ISI_T_BITMAP16)
				{
					LPDIRECTDRAWSURFACE pSurface;
					KSGImageContent* pBitmap = (KSGImageContent*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							0, ISI_T_BITMAP16, (void*&)pSurface);

					if (pBitmap)
					{
						if (pSurface)
						{
							RECT	rc;
							rc.left = pTemp->oPosition.nX;
							rc.top = pTemp->oPosition.nY;
							rc.right = pTemp->oEndPos.nX;
							rc.bottom = pTemp->oEndPos.nY;
							m_Canvas.BltSurface(pSurface, &rc);
						}
						else
						{
							m_ImageStore.CreateBitmapSurface(pTemp->szImage, pTemp->uImage, pTemp->nISPosition);
						}
					}
				}
			}
		}
		break;
	}
}

void KRepresentShell2::DrawPrimitivesOnImage(int nPrimitiveCount, KRepresentUnit* pPrimitives, 
        unsigned int uGenre, const char* pszImage, unsigned int uImage, short& nImagePosition, int bFindById)
{
	AlphaRecContent* pDestBitmap = (AlphaRecContent*)m_ImageStore.GetExistedCreateBitmap(
		pszImage, uImage, nImagePosition);

	if (pDestBitmap == NULL)
		return;

	int   i = 0;
	int   nDestWidth  = pDestBitmap->nWidth;
	int   nDestHeight = pDestBitmap->nHeight;
	void* pDestBuffer = pDestBitmap->Data;

	switch(uGenre)
	{
	case RU_T_IMAGE:
		{
		KRUImage* pTemp = (KRUImage*)pPrimitives;
		for (i = 0; i < nPrimitiveCount; i++, pTemp++)
		{
			switch(pTemp->nType)
			{
			case ISI_T_SPR:
				{
					SPRFRAME* pFrame;
					SPRHEAD* pSprHeader = (SPRHEAD*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, (void*&)pFrame, bFindById);
					if (pSprHeader == NULL)
						break;

					int nX = pTemp->oPosition.nX;
					int nY = pTemp->oPosition.nY;

					if ((pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_FRAME_DRAW) == 0)
					{
						nX += pFrame->OffsetX;
						nY += pFrame->OffsetY;
						if (pTemp->bRenderFlag & RUIMAGE_RENDER_FLAG_REF_SPOT)
						{
							nX -= pSprHeader->CenterX;
							nY -= pSprHeader->CenterY;
						}
					}

					char* pPalette = GET_SPR_PALETTE(pSprHeader);

					switch(pTemp->bRenderStyle)
					{
					case IMAGE_RENDER_STYLE_ALPHA:
					case IMAGE_RENDER_STYLE_ALPHA_NOT_BE_LIT:
					case IMAGE_RENDER_STYLE_3LEVEL:
						if(pSprHeader->Reserved[1])
						{
							UINT nColor = pTemp->Color.Color_dw & 0xffffff;
							if(g_pDirectDraw->GetRGBBitCount() == 32)
								RIO_CopySprToBufferScreen32b(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY,
								CBM_COLOR, pTemp->Color.Color_b.a, nColor);
							else
								RIO_CopySprToBufferScreen(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY,
								CBM_COLOR, pTemp->Color.Color_b.a, nColor);
						}
						else
						{
							if(g_pDirectDraw->GetRGBBitCount() == 32)
								RIO_CopySprToBufferAlpha32b(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a);
							else
								RIO_CopySprToBufferAlpha(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a);
						}
						break;
					case IMAGE_RENDER_STYLE_OPACITY:
						if(g_pDirectDraw->GetRGBBitCount() == 32)
							RIO_CopySprToBuffer32b(pFrame->Sprite, pFrame->Width, pFrame->Height,
							(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY);
						else
							RIO_CopySprToBuffer(pFrame->Sprite, pFrame->Width, pFrame->Height,
							pPalette, pDestBuffer, nDestWidth, nDestHeight, nX, nY);
						break;
					case IMAGE_RENDER_STYLE_ALPHA_COLOR_ADJUST:
						//pPalette = m_ImageStore.GetAdjustColorPalette(pTemp->nISPosition, pTemp->Color.Color_dw);
						UINT nColor = pTemp->Color.Color_dw & 0xffffff;
						if(pSprHeader->Reserved[1])
						{
							if(g_pDirectDraw->GetRGBBitCount() == 32)
								RIO_CopySprToBufferScreen32b(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY,
								CBM_COLOR, pTemp->Color.Color_b.a, nColor);
							else
								RIO_CopySprToBufferScreen(pFrame->Sprite, pFrame->Width, pFrame->Height,
								(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY,
								CBM_COLOR, pTemp->Color.Color_b.a, nColor);
						}
						else
						{
							if(nColor == 0)
							{
								if(g_pDirectDraw->GetRGBBitCount() == 32)
									RIO_CopySprToBufferAlpha32b(pFrame->Sprite, pFrame->Width, pFrame->Height,
									(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a);
								else
									RIO_CopySprToBufferAlpha(pFrame->Sprite, pFrame->Width, pFrame->Height,
									(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a);
							}
							else
							{
								if(g_pDirectDraw->GetRGBBitCount() == 32)
									RIO_CopySprToBufferBlendColor32b((BYTE*)pFrame->Sprite, pFrame->Width, pFrame->Height,
									(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a, nColor, CBM_MULTIPLY);
								else
									RIO_CopySprToBufferBlendColor((BYTE*)pFrame->Sprite, pFrame->Width, pFrame->Height,
									(BYTE*)pPalette, (BYTE*)pDestBuffer, nDestWidth, nDestHeight, nX, nY, pTemp->Color.Color_b.a, nColor, CBM_MULTIPLY);
							}
						}
						break;
					}
				}
				break;
			case ISI_T_BITMAP16:
				{
					void* pFrame;
					KSGImageContent* pBitmap = (KSGImageContent*)m_ImageStore.GetImage(
							pTemp->szImage,	pTemp->uImage, pTemp->nISPosition,
							pTemp->nFrame, pTemp->nType, pFrame, bFindById);
					if (pBitmap)
					{
						if(g_pDirectDraw->GetRGBBitCount() == 32)
							RIO_CopyBitmap16ToBuffer32b(pBitmap->Data, pBitmap->nWidth, pBitmap->nHeight, (BYTE*)pDestBuffer,
							nDestWidth, nDestHeight, pTemp->oPosition.nX, pTemp->oPosition.nY, pTemp->Color.Color_b.a);
						else
							RIO_CopyBitmap16ToBuffer(pBitmap->Data, pBitmap->nWidth, pBitmap->nHeight, (BYTE*)pDestBuffer,
							nDestWidth, nDestHeight, pTemp->oPosition.nX, pTemp->oPosition.nY, pTemp->Color.Color_b.a);
					}
				}
				break;
			}
		}}
		break;
	case RU_T_SHADOW:
		{
			KRUShadow* pTemp =(KRUShadow *)pPrimitives;
			for (i = 0; i < nPrimitiveCount; i++, pTemp++)
			{
				int nW = pTemp->oEndPos.nX - pTemp->oPosition.nX;
				int nH = pTemp->oEndPos.nY - pTemp->oPosition.nY;
				UINT nColor = pTemp->Color.Color_dw;
				if(!pTemp->bUseNewAlpha)
				{
					nColor = pTemp->Color.Color_dw & 0xffffff;
					if(pTemp->Color.Color_b.a < 32)
					{
						UINT nA = (32 - pTemp->Color.Color_b.a)*8;
						if(nA > 255)
							nA = 255;
						nColor |= (nA << 24);
					}
				}
				if(g_pDirectDraw->GetRGBBitCount() == 32)
					RIO_CopyShadowToBuffer32b(nColor, nW, nH, (BYTE*)pDestBuffer,
					nDestWidth, nDestHeight, pTemp->oPosition.nX, pTemp->oPosition.nY);
				else
					RIO_CopyShadowToBuffer(nColor, nW, nH, (BYTE*)pDestBuffer,
					nDestWidth, nDestHeight, pTemp->oPosition.nX, pTemp->oPosition.nY);
			}
		}
		break;
	}
}

//##Documentation
//## 清除图形数据
void KRepresentShell2::ClearImageData(const char* pszImage, unsigned int uImage, short nImagePosition)
{
	void* pFrame;
	KSGImageContent* pBitmap = (KSGImageContent*)m_ImageStore.GetImage(
			pszImage,	uImage, nImagePosition, 0, ISI_T_BITMAP16, pFrame);
	if (pBitmap)
		memset(pBitmap->Data, 0, 2 * pBitmap->nWidth * pBitmap->nHeight);
}

void KRepresentShell2::ClearAlphaImgData(const char* pszImage, unsigned int uImage, short nImagePosition,
		unsigned int uColor)
{
	void* pFrame;
	AlphaRecContent* pBitmap = (AlphaRecContent*)m_ImageStore.GetImage(
			pszImage,	uImage, nImagePosition, 0, ISI_T_DRAWINGRC, pFrame);
	if (pBitmap)
	{
		if(uColor)
		{
			UINT* p = &pBitmap->Data[0];
			if(g_pDirectDraw->GetRGBBitCount() != 32)
			{
				UINT R = ((uColor & 0xff0000) >> 19) << 11;
				UINT G = ((uColor & 0xff00) >> 10) << 5;
				UINT B = (uColor & 0xff) >> 3;
				uColor = (uColor & 0xff000000) | R | G | B;
			}
			for(int i=0; i < pBitmap->nWidth * pBitmap->nHeight; i++)
			{
				*(p++) = uColor;
			}
		}
		else
			memset(pBitmap->Data, 0, 4 * pBitmap->nWidth * pBitmap->nHeight);
	}
}

void KRepresentShell2::UpdateAlphaImgData(const char* pszImage, void* pNewData, unsigned int uImage,
						short nImagePosition)
{
	void* pFrame;
	AlphaRecContent* pBitmap = (AlphaRecContent*)m_ImageStore.GetImage(
			pszImage,	uImage, nImagePosition, 0, ISI_T_DRAWINGRC, pFrame);
	if (pBitmap)
	{
		memcpy(pBitmap->Data, pNewData, pBitmap->nWidth * pBitmap->nHeight * 4);
	}
}

void KRepresentShell2::UpdateAlphaImgSrcMode(const char* pszImage, int bScrMode, unsigned int uImage,
						short nImagePosition)
{
	void* pFrame;
	AlphaRecContent* pBitmap = (AlphaRecContent*)m_ImageStore.GetImage(
			pszImage, uImage, nImagePosition, 0, ISI_T_DRAWINGRC, pFrame);
	if (pBitmap)
	{
		pBitmap->bScrMode = bScrMode;
	}
}

void KRepresentShell2::ConvertImgRectToHardSrcMode(
        const char* pszImage, unsigned int uImage, short nImagePosition)
{
	void* pFrame;
	AlphaRecContent* pBitmap = (AlphaRecContent*)m_ImageStore.GetImage(
			pszImage, uImage, nImagePosition, 0, ISI_T_DRAWINGRC, pFrame);
	if (pBitmap && pBitmap->bScrMode)
	{
		pBitmap->bScrMode = FALSE;
		m_ImageStore.FreeScaleData(uImage, nImagePosition);
		RIO_ConvertToHardScreen32b(pBitmap->nWidth, pBitmap->nHeight, pBitmap->Data);
	}
}

void* KRepresentShell2::LoadExternImg(const char* pszImage, unsigned int& uImage,
		short& nImagePosition, int nFrame, int nType, void*& pFrameData, int bFindById)
{
	return m_ImageStore.GetImage(pszImage, uImage, nImagePosition,
								nFrame, nType, pFrameData, bFindById);
}
//##ModelId=3DCD8E9200E8
void KRepresentShell2::FreeAllImage()
{
	m_ImageStore.Free();
}

//##ModelId=3DCD8EF60316
void KRepresentShell2::FreeImage(const char* pszImage)
{
	m_ImageStore.FreeImage(pszImage);
}

//##ModelId=3DCD8FA900EE
void* KRepresentShell2::GetBitmapDataBuffer(const char* pszImage, KBitmapDataBuffInfo* pInfo, int nType /*ISI_T_BITMAP16*/)
{
	unsigned int uImage = 0;
	short		nISPosition = -1;
	void*		pBuffer = NULL;

	LPDIRECTDRAWSURFACE pSurface;
	void* pBitmap = m_ImageStore.GetImage(
					pszImage, uImage, nISPosition, 0, nType, (void*&)pSurface);
	if(!pBitmap)
		return pBuffer;
	int nPitch;
	if(nType == ISI_T_BITMAP16)
	{
		nPitch = ((KSGImageContent*)pBitmap)->nWidth * 2;
		pBuffer = ((KSGImageContent*)pBitmap)->Data;
	}
	else
	{
		nPitch = ((AlphaRecContent*)pBitmap)->nWidth * 4;
		pBuffer = ((AlphaRecContent*)pBitmap)->Data;
	}
	if (pSurface)
	{
		DDSURFACEDESC	desc;
		desc.dwSize = sizeof(desc);
		if (pSurface->Lock(NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK)
		{
			pBuffer = desc.lpSurface;
			nPitch = desc.lPitch;
		}
	}
	if (pInfo)
	{
		if(nType == ISI_T_BITMAP16)
		{
			pInfo->nWidth = ((KSGImageContent*)pBitmap)->nWidth;
			pInfo->nHeight = ((KSGImageContent*)pBitmap)->nHeight;
			pInfo->bScrMode = 0;
		}
		else
		{
			pInfo->nWidth = ((AlphaRecContent*)pBitmap)->nWidth;
			pInfo->nHeight = ((AlphaRecContent*)pBitmap)->nHeight;
			pInfo->bScrMode = ((AlphaRecContent*)pBitmap)->bScrMode;
		}
		pInfo->nPitch = nPitch;
		pInfo->pData = pBuffer;
		pInfo->eFormat = BDBF_16BIT_565;//(m_DirectDraw.GetRGBBitMask16() == RGB_565) ? BDBF_16BIT_565 : BDBF_16BIT_555;
	}
	return pBuffer;
}

//##释放对(通过GetBitmapDataBuffer调用获取得的)图形像点数据缓冲区的控制
void KRepresentShell2::ReleaseBitmapDataBuffer(const char* pszImage, void* pBuffer)
{
	unsigned int uImage = 0;
	short		nISPosition = -1;
	LPDIRECTDRAWSURFACE pSurface;
	KSGImageContent* pDestBitmap = (KSGImageContent *)m_ImageStore.GetImage(
					pszImage, uImage, nISPosition, 0, ISI_T_BITMAP16, (void*&)pSurface);
	if (pSurface && pDestBitmap)
	{
		pSurface->Unlock(NULL);
	}
}

//##ModelId=3DCA6EBC000F
bool KRepresentShell2::GetImageParam(const char* pszImage, KImageParam* pImageData, int nType) 
{
	return m_ImageStore.GetImageParam(pszImage, nType, pImageData);
}

bool KRepresentShell2::GetImageFrameParam(const char* pszImage, int nFrame,
			KRPosition2* pOffset, KRPosition2* pSize, int nType)
{
	return m_ImageStore.GetImageFrameParam(pszImage, nType, nFrame, pOffset, pSize);
}

//##ModelId=3DCA72620157
int KRepresentShell2::GetImagePixelAlpha(const char* pszImage, int nFrame, int nX, int nY, int nType)
{
	return m_ImageStore.GetImagePixelAlpha(pszImage, nType, nFrame, nX, nY);
}

//##ModelId=3DC0A08D0085
void KRepresentShell2::LookAt(int nX, int nY, int nZ)
{
	m_nLeft = nX - m_Canvas.GetWidth() / 2;
	m_nTop  = nY / 2 - ((nZ * 887) >> 10) - m_Canvas.GetHeight() / 2;
}

//##ModelId=3DCA0BAE00E4
void KRepresentShell2::OutputText(int nFontId, const char* psText, int nCount, int nX, int nY, unsigned int Color, int nLineWidth, int nZ, unsigned int BorderColor)
{
	int i;
	for (i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].nId == nFontId)
			break;
	}
	if (i < RS2_MAX_FONT_ITEM_NUM && m_FontTable[i].pFontObj)
	{
		if (nZ != TEXT_IN_SINGLE_PLANE_COORD)
			CoordinateTransform(nX, nY, nZ);
		m_FontTable[i].pFontObj->SetBorderColor(BorderColor);
		m_FontTable[i].pFontObj->SetOutputSize(nFontId, nFontId + 1);
		m_FontTable[i].pFontObj->OutputText(psText, nCount, nX, nY, Color, nLineWidth);
	}
}

//##ModelId=3DB655B2000E
//##Documentation
//## 输出文字。
int KRepresentShell2::OutputRichText(int nFontId, KOutputTextParam* pParam, 
		const char* psText, int nCount, int nLineWidth)
{
	if (pParam == NULL)
		return 0;
	int i;
	for (i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].nId == nFontId)
			break;
	}
	if (i < RS2_MAX_FONT_ITEM_NUM && m_FontTable[i].pFontObj)
	{
		KTextProcess	tp(psText, nCount, nLineWidth * 2 / nFontId);
		if (pParam->nZ != TEXT_IN_SINGLE_PLANE_COORD)
		{
			int x, y, z;
			x = pParam->nX;
			y = pParam->nY;
			z = pParam->nZ;
			CoordinateTransform(x, y, z);
			pParam->nX = x;
			pParam->nY = y;
		}
		m_FontTable[i].pFontObj->SetBorderColor(pParam->BorderColor);
		m_FontTable[i].pFontObj->SetOutputSize(nFontId, nFontId + 1);
		return tp.DrawTextLine(m_FontTable[i].pFontObj, nFontId, pParam);
	}
	return 0;
}

//## 返回指定坐标在字符串中最近的字符偏移
int KRepresentShell2::LocateRichText(int nX, int nY,	//not use
						int nFontId, KOutputTextParam* pParam, 
						const char* psText, int nCount, int nLineWidth)
{
	if (pParam == NULL)
		return -1;
	int i;
	for (i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].nId == nFontId)
			break;
	}
	if (i < RS2_MAX_FONT_ITEM_NUM && m_FontTable[i].pFontObj)
	{
		KTextProcess	tp(psText, nCount, nLineWidth * 2 / nFontId);
		if (pParam->nZ != TEXT_IN_SINGLE_PLANE_COORD)
		{
			int x, y, z;
			x = pParam->nX;
			y = pParam->nY;
			z = pParam->nZ;
			CoordinateTransform(x, y, z);
			pParam->nX = x;
			pParam->nY = y;
		}
		m_FontTable[i].pFontObj->SetBorderColor(pParam->BorderColor);
		m_FontTable[i].pFontObj->SetOutputSize(nFontId, nFontId + 1);
		return tp.TransXYPosToCharOffset(nX, nY, m_FontTable[i].pFontObj, nFontId, pParam);
	}
	return -1;
}

//##ModelId=3DCA72E102FE
void KRepresentShell2::Release()
{
	m_Canvas.Terminate();
	ShutdownGdiplus();
	delete this;
}

//##ModelId=3DCA0B8102F3
void KRepresentShell2::ReleaseAFont(int nId)
{
	for (int i = 0; i < RS2_MAX_FONT_ITEM_NUM; i++)
	{
		if (m_FontTable[i].nId == nId)
		{
			m_FontTable[i].nId = 0;
			if (m_FontTable[i].pFontObj)
			{
				m_FontTable[i].pFontObj->Release();
				m_FontTable[i].pFontObj = NULL;
			}
			break;
		}
	}
}

//##ModelId=3DB69EC0023A
bool KRepresentShell2::Reset(int nWidth, int nHeight, bool bFullScreen)
{
	return (Create(nWidth, nHeight, bFullScreen)==0);
}

//##ModelId=3DCD90910361
bool KRepresentShell2::SaveImage(const char* pszFile, const char* pszImage, int nFileType)
{
	return m_ImageStore.SaveImage(pszFile, pszImage, nFileType);
}

//##ModelId=3DCD90F30011
void KRepresentShell2::SetImageStoreBalanceParam(int nNumImage, unsigned int uCheckPoint)
{
	m_ImageStore.SetBalanceParam(nNumImage, uCheckPoint);
}

//##ModelId=3DD00EEE0149
bool KRepresentShell2::CopyDeviceImageToImage(const char* pszName, int nDeviceX, int nDeviceY, int nImageX, int nImageY, int nWidth, int nHeight)
{
	if (nWidth > m_Canvas.GetWidth() - nDeviceX || nHeight > m_Canvas.GetHeight() - nDeviceY)
		return false;

	short nISPosition = -1;
	KSGImageContent* pBitmap = (KSGImageContent*)m_ImageStore.GetExistedCreateBitmap(
		pszName, 0, nISPosition);

	if (pBitmap)
	{
		if (pBitmap->nWidth >= nImageX + nWidth && pBitmap->nHeight >= nImageY + nHeight)
		{
			int nPitch;
			void* pDevice = m_Canvas.LockCanvas(nPitch);
			if (pDevice)
			{
				unsigned short*	pBuffer = &pBitmap->Data[pBitmap->nWidth * nImageY + nImageX];
				pDevice = (char*)pDevice + nDeviceY * nPitch + nDeviceX * 2;
				int nCopyLen = nWidth * 2;
				for (int i = 0; i < nHeight; i++)
				{
					memcpy(pBuffer, pDevice, nCopyLen);
					pBuffer += pBitmap->nWidth;
					pDevice = (char*)pDevice + nPitch;
				}
				m_Canvas.UnlockCanvas();
				return true;
			}
		}
	}
	return false;
}

//##ModelId=3DCFED410049
void KRepresentShell2::CoordinateTransform(int& nX, int& nY, int nZ)
{
	nX = nX - m_nLeft;
	nY = nY / 2 - m_nTop - ((nZ * 887) >> 10);	// * sqrt(3) / 2
}

//##ModelId=3DD20C45002A
bool KRepresentShell2::RepresentBegin(int bClear, unsigned int Color)
{
	KRColor	c;
	c.Color_dw = Color;
	if(g_pDirectDraw->GetRGBBitCount() == 32)
	{
		if (bClear)
			m_Canvas.FillCanvas(Color);
	}
	else
	{
		if (bClear)
			m_Canvas.FillCanvas(g_RGB(c.Color_b.r, c.Color_b.g, c.Color_b.b));
	}
	return true;
}

//##ModelId=3DD20C450066
void KRepresentShell2::RepresentEnd()
{
	m_Canvas.Changed(true);
	m_Canvas.UpdateScreen();
}

//视图/绘图设备坐标 转化为空间坐标
void KRepresentShell2::ViewPortCoordToSpaceCoord(int& nX,	int& nY, int  nZ)
{
	nX = nX + m_nLeft;
	nY = (nY + m_nTop + ((nZ * 887) >> 10)) * 2;
}
/*
bool KRepresentShell2::SaveScreenToFile(const char* pszName)
{
	if(!pszName || !pszName[0])
		return 0;

	DWORD n = m_Canvas.m_nWidth;

	int nPicWidth, nPicHeight, nDesktopWidth, nDesktopHeight, nPicOffX, nPicOffY;
	{
		//全屏模式参数设定
		nPicOffX = 0;
		nPicOffY = 0;
		nDesktopWidth = nPicWidth = m_Canvas.m_nWidth;
		nDesktopHeight = nPicHeight = m_Canvas.m_nHeight;
	}

	WORD *pSrc;
	BYTE *pDes, *pTemp;

	// 分配r8g8b8缓冲区	
	pTemp = pDes = new BYTE[nPicWidth * nPicHeight * 3];
	if(!pDes)
		return false;

	pSrc = (WORD*)m_Canvas.m_pCanvas;
	
	// 拷贝屏幕数据到缓冲区
	for(int i=0; i<nPicHeight; i++)
	{
		for(int j=0; j<nPicWidth; j++)
		{
			if(m_DirectDraw.GetRGBBitMask16() == RGB_565)
			{
				pDes[2] = ((*pSrc) & 0xf800) >> 8;
				pDes[1] = ((*pSrc) & 0x07e0) >> 3;
				pDes[0] = ((*pSrc) & 0x001f) << 3;
			}
			else
			{
				pDes[2] = ((*pSrc) & 0x7c00) >> 7;
				pDes[1] = ((*pSrc) & 0x03e0) >> 2;
				pDes[0] = ((*pSrc) & 0x001f) << 3;
			}
			pDes += 3;
			pSrc++;
		}
	}

	// 生成24位bmp文件
	if(!KBmpFile24::SaveBuffer24((char*)pszName, pTemp, nPicWidth*3, nPicWidth, nPicHeight))
	{
		delete[] pTemp;
		return false;
	}

	delete[] pTemp;
	return true;
}*/

bool KRepresentShell2::SaveScreenToFile(const char* pszName, ScreenFileType eType, unsigned int nQuality)
{
	if(!pszName || !pszName[0])
		return 0;

	DWORD n = m_Canvas.GetWidth();

	int nPicWidth, nPicHeight, nDesktopWidth, nDesktopHeight, nPicOffX, nPicOffY;
	if(m_DirectDraw.GetScreenMode() == WINDOWMODE)
	{
		// 窗口模式参数设定
		RECT rect;
		POINT ptLT, ptRB;
		HWND hWnd = g_GetMainHWnd();
		GetClientRect(hWnd, &rect);
		ptLT.x = rect.left, ptLT.y = rect.top;
		ptRB.x = rect.right, ptRB.y = rect.bottom;
		ClientToScreen(hWnd, &ptLT);
		ClientToScreen(hWnd, &ptRB);

		nDesktopWidth = m_DirectDraw.GetScreenWidth();
		nDesktopHeight = m_DirectDraw.GetScreenHeight();

		// 如果窗口客户区超出屏幕则返回
		if(ptLT.x >= nDesktopWidth || ptLT.y >= nDesktopHeight || ptRB.x <= 0 || ptRB.y <= 0)
			return false;
		if(ptLT.x < 0)
			ptLT.x = 0;
		if(ptLT.y < 0)
			ptLT.y = 0;
		if(ptRB.x > nDesktopWidth)
			ptRB.x = nDesktopWidth - 1;
		if(ptRB.y > nDesktopHeight)
			ptRB.y = nDesktopHeight - 1;

		nPicOffX = ptLT.x;
		nPicOffY = ptLT.y;
		nPicWidth = ptRB.x - ptLT.x;
		nPicHeight = ptRB.y - ptLT.y;
	}
	else
	{
		//全屏模式参数设定
		nPicOffX = 0;
		nPicOffY = 0;
		nDesktopWidth = nPicWidth = m_Canvas.GetWidth();
		nDesktopHeight = nPicHeight = m_Canvas.GetHeight();
	}

	WORD *pSrc;
	BYTE *pDes, *pTemp;

	// 分配r8g8b8缓冲区	
	pTemp = pDes = new BYTE[nPicWidth * nPicHeight * 3];
	if(!pDes)
		return false;

	if((pSrc = (WORD*)m_DirectDraw.LockPrimaryBuffer()) == NULL)
	{
		delete[] pTemp;
		return false;
	}
	int nPitch = m_DirectDraw.GetScreenPitch();
	pSrc += nPicOffY * nPitch / 2 + nPicOffX;
	int nLineAdd = nPitch / 2 - nPicWidth;
	
	// 拷贝屏幕数据到缓冲区
	for(int i=0; i<nPicHeight; i++)
	{
		for(int j=0; j<nPicWidth; j++)
		{
			if(m_DirectDraw.GetRGBBitMask16() == RGB_565)
			{
				pDes[2] = ((*pSrc) & 0xf800) >> 8;
				pDes[1] = ((*pSrc) & 0x07e0) >> 3;
				pDes[0] = ((*pSrc) & 0x001f) << 3;
			}
			else
			{
				pDes[2] = ((*pSrc) & 0x7c00) >> 7;
				pDes[1] = ((*pSrc) & 0x03e0) >> 2;
				pDes[0] = ((*pSrc) & 0x001f) << 3;
			}
			pDes += 3;
			pSrc++;
		}
		pSrc += nLineAdd;
	}

	m_DirectDraw.UnLockPrimaryBuffer();

	BOOL bRet;
	if(eType == SCRFILETYPE_BMP)
		// 保存24位bmp文件
		bRet = KBmpFile24::SaveBuffer24((char*)pszName, pTemp, nPicWidth*3, nPicWidth, nPicHeight);
	else
		// 保存24位jpg文件
		bRet = SaveBufferToJpgFile24((char*)pszName, pTemp, nPicWidth*3, nPicWidth, nPicHeight, nQuality);
	if(!bRet)
	{
		delete[] pTemp;
		return false;
	}

	delete[] pTemp;
	return true;
}

void KRepresentShell2::GetClipRect(RECT &rClip)
{
	m_Canvas.GetClipRect(&rClip);
}

void KRepresentShell2::SetClipRect(RECT rClip)
{
	m_Canvas.SetClipRect(&rClip);
}
