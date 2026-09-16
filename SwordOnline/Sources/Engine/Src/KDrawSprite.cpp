//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KDrawSprite.cpp
// Date:	2000.08.08
// Code:	WangWei(Daphnis), Wooy(Wu yue)
// Desc:	Sprite Drawing Function
//---------------------------------------------------------------------------
#include "KWin32.h"
#include "KCanvas.h"
#include "KDrawSprite.h"
#include "DrawSpriteMP.h"

//---------------------------------------------------------------------------
// ∫Ø ˝:	DrawSprite
// π¶ƒ‹:	ªÊ÷∆256…´SpriteŒªÕº
// ≤Œ ˝:	KDrawNode*, KCanvas* 
// ∑µªÿ:	void
//---------------------------------------------------------------------------
void g_DrawSprite(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	// pBuffer÷∏œÚ∆¡ƒªªÊ÷∆––µƒÕ∑“ª∏ˆœÒµ„¥¶ 
	int nPitch;
	void* pBuffer = pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer = (char*)(pBuffer) + Clipper.y * nPitch;
	void* pPalette	= pNode->m_pPalette;// palette pointer
	void* pSprite = pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;

	__asm
	{
		// πedi÷∏œÚbufferªÊ÷∆∆µ„,	(“‘◊÷Ω⁄º∆)	
		mov		edi, pBuffer
		mov		eax, Clipper.x
		add		edi, eax
		add		edi, eax

		// πesi÷∏œÚÕºøÈ ˝æ›∆µ„,(Ã¯π˝nSprSkip∏ˆœÒµ„µƒÕº–Œ ˝æ›)
		mov		esi, pSprite
		//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
			or		edx, edx
			jz		_SkipSpriteAheadContentEnd_

			_SkipSpriteAheadContentLocalStart_:
			{
				read_alpha_2_ebx_run_length_2_eax
				or		ebx, ebx
				jnz		_SkipSpriteAheadContentLocalAlpha_
				sub		edx, eax
				jg		_SkipSpriteAheadContentLocalStart_
				neg		edx
				jmp		_SkipSpriteAheadContentEnd_

				_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					add		esi, edx
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
		_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
		or		eax, eax
		jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

		//_DrawFullLineSection_:
		{
			//“ÚŒ™sprite≤ªª·øÁ––—πÀı£¨‘Ú‘À––µΩ¥À¥¶edx±ÿŒ™0£¨»Áspriteª·øÁ––—πÀı‘Ú_DrawFullLineSection_–Ë∏ƒ			
			_DrawFullLineSection_Line_:
			{
				mov		edx, Clipper.width
				_DrawFullLineSection_LineLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.height
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_
				
					_DrawFullLineSection_LineLocal_Alpha_:
					{
						sub		edx, eax
						mov		ecx, eax
						mov     ebx, pPalette
						_DrawFullLineSection_CopyPixel_:
						{
							copy_pixel_use_eax
							loop	_DrawFullLineSection_CopyPixel_
						}
						or		edx, edx
						jnz		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		Clipper.height
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_
					}
				}
			}
		}

		_DrawPartLineSection_:
		{
			mov		eax, Clipper.left
			or		eax, eax
			jz		_DrawPartLineSection_SkipRight_Line_

			mov		eax, Clipper.right
			or		eax, eax
			jz		_DrawPartLineSection_SkipLeft_Line_
		}

		_DrawPartLineSection_Line_:
		{
			mov		eax, edx
			mov		edx, Clipper.width
			or		eax, eax
			jnz		_DrawPartLineSection_LineLocal_CheckAlpha_
			_DrawPartLineSection_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				_DrawPartLineSection_LineLocal_CheckAlpha_:
				or		ebx, ebx
				jnz		_DrawPartLineSection_LineLocal_Alpha_
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		_DrawPartLineSection_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_

				add		edi, edx
				add		edi, edx
				neg		edx
			}
			
			_DrawPartLineSection_LineSkip_:
			{
				add		edi, nBuffNextLine
				//Ã¯π˝nSprSkipPerLineœÒÀÿµƒspriteƒ⁄»›
				mov		eax, edx
				mov		edx, nSprSkipPerLine
				or		eax, eax
				jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_
				_DrawPartLineSection_LineSkipLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax
					
					_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
					or		ebx, ebx
					jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
					sub		edx, eax
					jg		_DrawPartLineSection_LineSkipLocal_
					neg		edx
					jmp		_DrawPartLineSection_Line_
					_DrawPartLineSection_LineSkipLocal_Alpha_:
					{
						add		esi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_LineSkipLocal_
						add		esi, edx
						neg		edx
						jmp		_DrawPartLineSection_Line_
					}
				}
			}
			_DrawPartLineSection_LineLocal_Alpha_:
			{
				sub		edx, eax
				jle		_DrawPartLineSection_LineLocal_Alpha_Part_		//≤ªƒ‹»´ª≠’‚eax∏ˆœ‡Õ¨alpha÷µµƒœÒµ„£¨∫Û√Ê”–µ„“—æ≠≥¨≥ˆ«¯”Ú

				mov		ecx, eax
				mov     ebx, pPalette

				_DrawPartLineSection_CopyPixel_:
				{
					copy_pixel_use_eax
					loop	_DrawPartLineSection_CopyPixel_
				}
				jmp		_DrawPartLineSection_LineLocal_
			}
			_DrawPartLineSection_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				mov     ebx, pPalette
				_DrawPartLineSection_CopyPixel_Part_:
				{
					copy_pixel_use_eax
					loop	_DrawPartLineSection_CopyPixel_Part_
				}
			
				dec		Clipper.height
				jz		_EXIT_WAY_
				neg		edx
				mov		ebx, 255	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
				jmp		_DrawPartLineSection_LineSkip_
			}
		}

		_DrawPartLineSection_SkipLeft_Line_:
		{
			mov		eax, edx
			mov		edx, Clipper.width
			or		eax, eax
			jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
			_DrawPartLineSection_SkipLeft_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
				or		ebx, ebx
				jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		_DrawPartLineSection_SkipLeft_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_
			}
			
			_DrawPartLineSection_SkipLeft_LineSkip_:
			{
				add		edi, nBuffNextLine
				//Ã¯π˝nSprSkipPerLineœÒÀÿµƒspriteƒ⁄»›
				mov		edx, nSprSkipPerLine
				_DrawPartLineSection_SkipLeft_LineSkipLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax
					or		ebx, ebx
					jnz		_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_
					sub		edx, eax
					jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
					neg		edx
					jmp		_DrawPartLineSection_SkipLeft_Line_
					_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
					{
						add		esi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
						add		esi, edx
						neg		edx
						jmp		_DrawPartLineSection_SkipLeft_Line_
					}
				}
			}
			_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
			{
				sub		edx, eax
				mov		ecx, eax						
				mov     ebx, pPalette
				_DrawPartLineSection_SkipLeft_CopyPixel_:
				{
					copy_pixel_use_eax
					loop	_DrawPartLineSection_SkipLeft_CopyPixel_
				}
				or		edx, edx
				jnz		_DrawPartLineSection_SkipLeft_LineLocal_
				dec		Clipper.height
				jg		_DrawPartLineSection_SkipLeft_LineSkip_
				jmp		_EXIT_WAY_
			}
		}

		_DrawPartLineSection_SkipRight_Line_:
		{
			mov		edx, Clipper.width
			_DrawPartLineSection_SkipRight_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				or		ebx, ebx
				jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		_DrawPartLineSection_SkipRight_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_

				add		edi, edx
				add		edi, edx
				neg		edx
			}
			
			_DrawPartLineSection_SkipRight_LineSkip_:
			{
				add		edi, nBuffNextLine
				//Ã¯π˝nSprSkipPerLineœÒÀÿµƒspriteƒ⁄»›
				mov		eax, edx
				mov		edx, nSprSkipPerLine
				or		eax, eax
				jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_
				_DrawPartLineSection_SkipRight_LineSkipLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax
					
					_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
					or		ebx, ebx
					jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_
					sub		edx, eax
					jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
					jmp		_DrawPartLineSection_SkipRight_Line_
					_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
					{
						add		esi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
						jmp		_DrawPartLineSection_SkipRight_Line_
					}
				}
			}
			_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
			{
				sub		edx, eax
				jle		_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_		//≤ªƒ‹»´ª≠’‚eax∏ˆœ‡Õ¨alpha÷µµƒœÒµ„£¨∫Û√Ê”–µ„“—æ≠≥¨≥ˆ«¯”Ú

				mov		ecx, eax				
				mov     ebx, pPalette
				_DrawPartLineSection_SkipRight_CopyPixel_:
				{
					copy_pixel_use_eax
					loop	_DrawPartLineSection_SkipRight_CopyPixel_
				}
				jmp		_DrawPartLineSection_SkipRight_LineLocal_
			}
			_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				mov     ebx, pPalette
				_DrawPartLineSection_SkipRight_CopyPixel_Part_:
				{
					copy_pixel_use_eax
					loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
				}
				neg		edx
				mov		ebx, 255	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
				dec		Clipper.height
				jg		_DrawPartLineSection_SkipRight_LineSkip_
				jmp		_EXIT_WAY_
			}
		}
		_EXIT_WAY_:
	}
	pCanvas->UnlockCanvas();
}

void g_DrawSprite32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	BYTE* pPalette	= (BYTE*)pNode->m_pPalette;// palette pointer
	BYTE* pSprite = (BYTE*)pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;//left:sË Æi”m m t tı tr∏i qua, right: sË Æi”m m t tı l“ ph∂i qua tr∏i
	UINT alpha, nSrcColor, R,G,B;
	int nPixelBatch, nWidth;
	int nRemainPx = 0;
	UINT prevA;
	if(!nSprSkip)
	goto	_SkipSpriteAheadContentEnd_;

	_SkipSpriteAheadContentLocalStart_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_SkipSpriteAheadContentLocalAlpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_SkipSpriteAheadContentLocalStart_;
		nSprSkip = -nSprSkip;
		goto	_SkipSpriteAheadContentEnd_;
	_SkipSpriteAheadContentLocalAlpha_:
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
		{
			pSprite += nPixelBatch;
			goto	_SkipSpriteAheadContentLocalStart_;
		}
		nSprSkip = -nSprSkip;
		pSprite += nPixelBatch - nSprSkip;
	_SkipSpriteAheadContentEnd_:
		if (nSprSkipPerLine)
			goto	_DrawPartLineSection_;
		
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		if(nRemainPx)
		{
			nPixelBatch = nRemainPx;
			nRemainPx = 0;
			alpha = prevA;
		}
		else
		{
			nPixelBatch = *(pSprite++);
			alpha = *(pSprite++);
		}
		if(alpha)
			goto	_DrawFullLineSection_LineLocal_Alpha_;
		memset(pBuffer, 0, nPixelBatch*sizeof(UINT));
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
		if(nPixelBatch > nWidth)
		{
			nRemainPx = nPixelBatch - nWidth;
			prevA = alpha;
			nPixelBatch = nWidth;
			nWidth = 0;
		}
		else
			nWidth -= nPixelBatch;
		if(nWidth > 0)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	_DrawFullLineSection_LineLocal_Alpha_:
		if(nPixelBatch > nWidth)
		{
			nRemainPx = nPixelBatch - nWidth;
			prevA = alpha;
			nPixelBatch = nWidth;
			nWidth = 0;
		}
		else
			nWidth -= nPixelBatch;
		while(nPixelBatch)
		{
			//vœ 1 Æi”m vÌi alpha = 255
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nSrcColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	//bæt Æ«u vœ mÈt ph«n cÒa sprite
	_DrawPartLineSection_:
	//ph«n sprite m t l“ tr∏i vµ ph∂i
	_DrawPartLineSection_Line_:
		nWidth = Clipper.width;
		if(nSprSkip)
		{
			nPixelBatch = nSprSkip;
			goto	_DrawPartLineSection_LineLocal_CheckAlpha_;
		}
	_DrawPartLineSection_LineLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_LineLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_LineLocal_Alpha_;
		nWidth -= nPixelBatch;
		if(nWidth > 0)
		{
			memset(pBuffer, 0, nPixelBatch*sizeof(UINT));
			pBuffer += nPixelBatch*4;
			goto	_DrawPartLineSection_LineLocal_;
		}
		memset(pBuffer, 0, (nPixelBatch + nWidth)*sizeof(UINT));
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*4;
		nWidth = -nWidth;//sË d≠ nPixelBatch cﬂn lπi v≠Ót qua nWidth, xuËng hµng
	
	_DrawPartLineSection_LineSkip_:
		pBuffer += nBuffNextLine;
		nSprSkip = nSprSkipPerLine;
		if(nWidth)
		{
			nPixelBatch = nWidth;
			goto	_DrawPartLineSection_LineSkipLocal_CheckAlpha_;
		}
	_DrawPartLineSection_LineSkipLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_LineSkipLocal_Alpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_LineSkipLocal_;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_Line_;
	_DrawPartLineSection_LineSkipLocal_Alpha_:
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
		{
			pSprite += nPixelBatch;
			goto	_DrawPartLineSection_LineSkipLocal_;
		}
		pSprite += nPixelBatch + nSprSkip;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_Line_;
	_DrawPartLineSection_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		if(nWidth <= 0)
		{
			nWidth = -nWidth;
			nPixelBatch -= nWidth;
			goto	_DrawPartLineSection_LineLocal_Alpha_Part_;
		}
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nSrcColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nSrcColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		goto	_DrawPartLineSection_LineSkip_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// ∫Ø ˝:	DrawSpriteMixColor
// π¶ƒ‹:	ªÊ÷∆”Îƒ≥“ª—’…´ªÏ∫œµƒ256…´SpriteŒªÕº
// ≤Œ ˝:	KDrawNode*, KCanvas* 
// ∑µªÿ:	void
//---------------------------------------------------------------------------
void g_DrawSpriteMixColor(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of sprite
	int nHeight = pNode->m_nHeight;// height of sprite
	void* lpSprite = pNode->m_pBitmap;// sprite pointer
	void* lpPalette	= pNode->m_pPalette;// palette pointer

	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;
	//µ±«∞¥˙¬ÎÕº–Œ◊Û”“Õ¨ ±±ª≤√ºı ±”–ŒÛ
	if (Clipper.left && Clipper.right)
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	int nNextLine = nPitch - nWidth * 2;// next line add
    int nColor=pNode->m_nColor;
    int nAlpha=pNode->m_nAlpha;
	int nMask32 = pCanvas->m_nMask32;

	// ªÊ÷∆∫Ø ˝µƒª„±‡¥˙¬Î
	__asm
	{
//---------------------------------------------------------------------------
// º∆À„ EDI ÷∏œÚ∆¡ƒª∆µ„µƒ∆´“∆¡ø (“‘◊÷Ω⁄º∆)
// edi = lpBuffer + dwPitch * Clipper.y + nX * 2;
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, nX
		add		ebx, ebx
		add		eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
//---------------------------------------------------------------------------
// ≥ı ºªØ ESI ÷∏œÚÕºøÈ ˝æ›∆µ„ 
// (Ã¯π˝Clipper.top––—πÀıÕº–Œ ˝æ›)
//---------------------------------------------------------------------------
		mov		esi, lpSprite
		mov		ecx, Clipper.top
		or		ecx, ecx
		jz		loc_DrawSpriteMixColor_0011

loc_DrawSpriteMixColor_0008:

		mov		edx, nWidth

loc_DrawSpriteMixColor_0009:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0010
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0009
		dec     ecx
		jnz		loc_DrawSpriteMixColor_0008
		jmp		loc_DrawSpriteMixColor_0011

loc_DrawSpriteMixColor_0010:

		add		esi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0009
		dec     ecx
		jnz		loc_DrawSpriteMixColor_0008
//---------------------------------------------------------------------------
// ∏˘æ› Clipper.left, Clipper.right ∑÷ 4 ÷÷«Èøˆ
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0011:

		mov		eax, Clipper.left
		or		eax, eax
		jnz		loc_DrawSpriteMixColor_0012
		mov		eax, Clipper.right
		or		eax, eax
		jnz		loc_DrawSpriteMixColor_0013
		jmp		loc_DrawSpriteMixColor_0100

loc_DrawSpriteMixColor_0012:

		mov		eax, Clipper.right
		or		eax, eax
		jnz		loc_DrawSpriteMixColor_0014
		jmp		loc_DrawSpriteMixColor_0200

loc_DrawSpriteMixColor_0013:

		jmp		loc_DrawSpriteMixColor_0300

loc_DrawSpriteMixColor_0014:

		jmp		loc_DrawSpriteMixColor_exit
//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø == 0
// ”“±ﬂΩÁ≤√ºÙ¡ø == 0
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0100:

		mov		edx, Clipper.width

loc_DrawSpriteMixColor_0101:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0102

		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0101
		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteMixColor_0100
		jmp		loc_DrawSpriteMixColor_exit

loc_DrawSpriteMixColor_0102:

		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteMixColor_0103:

		movzx	eax, byte ptr[esi]
		inc		esi
		mov		dx, [ebx + eax * 2]
		
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   
		
		inc		edi
		inc		edi





		dec		ecx
		jnz		loc_DrawSpriteMixColor_0103

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0101
		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteMixColor_0100
		jmp		loc_DrawSpriteMixColor_exit

//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø != 0
// ”“±ﬂΩÁ≤√ºÙ¡ø == 0
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0200:

		mov		edx, Clipper.left

loc_DrawSpriteMixColor_0201:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0202
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (◊Û±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0201
		jz		loc_DrawSpriteMixColor_0203
		neg		edx
		mov		eax, edx
		mov		edx, Clipper.width
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteMixColor_0200
		jmp		loc_DrawSpriteMixColor_exit
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (◊Û±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0202:

		add		esi, eax
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0201
		jz		loc_DrawSpriteMixColor_0203
//---------------------------------------------------------------------------
// ∞—∂‡ºıµƒøÌ∂»≤πªÿ¿¥
//---------------------------------------------------------------------------
		neg		edx
		sub		esi, edx
		sub		edi, edx
		sub		edi, edx

		push	eax
		push	edx
		mov		ecx, edx
		mov     ebx, lpPalette

loc_DrawSpriteMixColor_Loop20:

		movzx	eax, byte ptr[esi]
		inc		esi
		mov     dx, [ebx + eax * 2]

		
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteMixColor_Loop20

		pop		edx
		pop		eax
		mov		ecx, edx
		mov		edx, Clipper.width
		sub		edx, ecx
		jg		loc_DrawSpriteMixColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteMixColor_0200
		jmp		loc_DrawSpriteMixColor_exit
//---------------------------------------------------------------------------
// “—¥¶¿ÌÕÍºÙ≤√«¯ œ¬√Êµƒ¥¶¿Ìœ‡∂‘ºÚµ•
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0203:

		mov		edx, Clipper.width

loc_DrawSpriteMixColor_0204:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0206
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (◊Û±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteMixColor_0200
		jmp		loc_DrawSpriteMixColor_exit
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (◊Û±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0206:

		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteMixColor_Loop21:

		movzx	eax, byte ptr[esi]
		inc		esi
		mov     dx, [ebx + eax * 2]

		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg		loc_DrawSpriteMixColor_Loop21

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteMixColor_0200
		jmp		loc_DrawSpriteMixColor_exit

//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø == 0
// ”“±ﬂΩÁ≤√ºÙ¡ø != 0
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0300:

		mov		edx, Clipper.width

loc_DrawSpriteMixColor_0301:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0303
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (”“±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0301
		neg		edx
		jmp		loc_DrawSpriteMixColor_0305
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (”“±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0303:

		cmp		edx, eax
		jl		loc_DrawSpriteMixColor_0304
		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteMixColor_Loop30:

		movzx	eax, byte ptr[esi]
		inc		esi
		mov     dx, [ebx + eax * 2]


		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteMixColor_Loop30

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0301
		neg		edx
		jmp		loc_DrawSpriteMixColor_0305

//---------------------------------------------------------------------------
// ¡¨–¯µ„µƒ∏ˆ ˝ (eax) > ≤√ºı∫ÛµƒøÌ∂» (edx)
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0304:

		push	eax
		push	edx
		mov		ecx, edx
		mov     ebx, lpPalette

loc_DrawSpriteMixColor_Loop31:

		movzx	eax, byte ptr[esi]
		inc		esi
		mov     dx, [ebx + eax * 2]
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteMixColor_Loop31

		pop		edx
		pop		eax
//---------------------------------------------------------------------------
// º∆À„≥¨π˝¡ÀºÙ≤√«¯≥§∂» => edx
//---------------------------------------------------------------------------
		sub		eax, edx
		mov		edx, eax
		add		esi, eax
		add		edi, eax
		add		edi, eax
//---------------------------------------------------------------------------
// ¥¶¿Ì≥¨π˝¡À”“±ﬂΩÁµƒ≤ø∑÷, edx = ≥¨π˝”“±ﬂΩÁ≤ø∑÷µƒ≥§∂»
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0305:

		mov		eax, edx
		mov		edx, Clipper.right
		sub		edx, eax
		jle		loc_DrawSpriteMixColor_0308

loc_DrawSpriteMixColor_0306:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteMixColor_0307
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (”“±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0306
		jmp		loc_DrawSpriteMixColor_0308
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (”“±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
loc_DrawSpriteMixColor_0307:

		add		esi, eax
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteMixColor_0306

loc_DrawSpriteMixColor_0308:

		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteMixColor_0300
		jmp		loc_DrawSpriteMixColor_exit

loc_DrawSpriteMixColor_exit:

	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// ∫Ø ˝:	DrawSpriteWithColor
// π¶ƒ‹:	ªÊ÷∆”Îƒ≥“ª—’…´ªÏ∫œµƒ256…´SpriteŒªÕº
// ≤Œ ˝:	KDrawNode*, KCanvas* 
// ∑µªÿ:	void
//---------------------------------------------------------------------------
void g_DrawSpriteWithColor(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of sprite
	int nHeight = pNode->m_nHeight;// height of sprite
	void* lpSprite = pNode->m_pBitmap;// sprite pointer
	void* lpPalette	= pNode->m_pPalette;// palette pointer
	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;
	//µ±«∞¥˙¬ÎÕº–Œ◊Û”“Õ¨ ±±ª≤√ºı ±”–ŒÛ
	if (Clipper.left && Clipper.right)
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	int nNextLine = nPitch - nWidth * 2;// next line add
    int nColor=pNode->m_nColor;
    int nAlpha=pNode->m_nAlpha;
	int nMask32 = pCanvas->m_nMask32;
	// ªÊ÷∆∫Ø ˝µƒª„±‡¥˙¬Î
	__asm
	{
//---------------------------------------------------------------------------
// º∆À„ EDI ÷∏œÚ∆¡ƒª∆µ„µƒ∆´“∆¡ø (“‘◊÷Ω⁄º∆)
// edi = lpBuffer + dwPitch * Clipper.y + nX * 2;
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, nX
		add		ebx, ebx
		add		eax, ebx
		mov		edi, lpBuffer
		add		edi, eax
//---------------------------------------------------------------------------
// ≥ı ºªØ ESI ÷∏œÚÕºøÈ ˝æ›∆µ„ 
// (Ã¯π˝Clipper.top––—πÀıÕº–Œ ˝æ›)
//---------------------------------------------------------------------------
		mov		esi, lpSprite
		mov		ecx, Clipper.top
		or		ecx, ecx
		jz		loc_DrawSpriteWithColor_0011

loc_DrawSpriteWithColor_0008:

		mov		edx, nWidth

loc_DrawSpriteWithColor_0009:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0010
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0009
		dec     ecx
		jnz		loc_DrawSpriteWithColor_0008
		jmp		loc_DrawSpriteWithColor_0011

loc_DrawSpriteWithColor_0010:

		add		esi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0009
		dec     ecx
		jnz		loc_DrawSpriteWithColor_0008
//---------------------------------------------------------------------------
// ∏˘æ› Clipper.left, Clipper.right ∑÷ 4 ÷÷«Èøˆ
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0011:

		mov		eax, Clipper.left
		or		eax, eax
		jnz		loc_DrawSpriteWithColor_0012
		mov		eax, Clipper.right
		or		eax, eax
		jnz		loc_DrawSpriteWithColor_0013
		jmp		loc_DrawSpriteWithColor_0100

loc_DrawSpriteWithColor_0012:

		mov		eax, Clipper.right
		or		eax, eax
		jnz		loc_DrawSpriteWithColor_0014
		jmp		loc_DrawSpriteWithColor_0200

loc_DrawSpriteWithColor_0013:

		jmp		loc_DrawSpriteWithColor_0300

loc_DrawSpriteWithColor_0014:

		jmp		loc_DrawSpriteWithColor_exit
//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø == 0
// ”“±ﬂΩÁ≤√ºÙ¡ø == 0
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0100:

		mov		edx, Clipper.width

loc_DrawSpriteWithColor_0101:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0102

		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0101
		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteWithColor_0100
		jmp		loc_DrawSpriteWithColor_exit

loc_DrawSpriteWithColor_0102:

		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteWithColor_0103:

		movzx	eax, byte ptr[esi]
		inc		esi
//		mov		dx, [ebx + eax * 2]
        xor     edx,edx 
		
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   
		
		inc		edi
		inc		edi





		dec		ecx
		jnz		loc_DrawSpriteWithColor_0103

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0101
		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteWithColor_0100
		jmp		loc_DrawSpriteWithColor_exit

//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø != 0
// ”“±ﬂΩÁ≤√ºÙ¡ø == 0
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0200:

		mov		edx, Clipper.left

loc_DrawSpriteWithColor_0201:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0202
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (◊Û±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0201
		jz		loc_DrawSpriteWithColor_0203
		neg		edx
		mov		eax, edx
		mov		edx, Clipper.width
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteWithColor_0200
		jmp		loc_DrawSpriteWithColor_exit
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (◊Û±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0202:

		add		esi, eax
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0201
		jz		loc_DrawSpriteWithColor_0203
//---------------------------------------------------------------------------
// ∞—∂‡ºıµƒøÌ∂»≤πªÿ¿¥
//---------------------------------------------------------------------------
		neg		edx
		sub		esi, edx
		sub		edi, edx
		sub		edi, edx

		push	eax
		push	edx
		mov		ecx, edx
		mov     ebx, lpPalette

loc_DrawSpriteWithColor_Loop20:

		movzx	eax, byte ptr[esi]
		inc		esi
//		mov     dx, [ebx + eax * 2]
        xor     edx,edx 

		
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteWithColor_Loop20

		pop		edx
		pop		eax
		mov		ecx, edx
		mov		edx, Clipper.width
		sub		edx, ecx
		jg		loc_DrawSpriteWithColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteWithColor_0200
		jmp		loc_DrawSpriteWithColor_exit
//---------------------------------------------------------------------------
// “—¥¶¿ÌÕÍºÙ≤√«¯ œ¬√Êµƒ¥¶¿Ìœ‡∂‘ºÚµ•
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0203:

		mov		edx, Clipper.width

loc_DrawSpriteWithColor_0204:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0206
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (◊Û±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteWithColor_0200
		jmp		loc_DrawSpriteWithColor_exit
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (◊Û±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0206:

		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteWithColor_Loop21:

		movzx	eax, byte ptr[esi]
		inc		esi
//		mov     dx, [ebx + eax * 2]
        xor     edx,edx

		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg		loc_DrawSpriteWithColor_Loop21

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0204
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawSpriteWithColor_0200
		jmp		loc_DrawSpriteWithColor_exit

//---------------------------------------------------------------------------
// ◊Û±ﬂΩÁ≤√ºÙ¡ø == 0
// ”“±ﬂΩÁ≤√ºÙ¡ø != 0
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0300:

		mov		edx, Clipper.width

loc_DrawSpriteWithColor_0301:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0303
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (”“±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0301
		neg		edx
		jmp		loc_DrawSpriteWithColor_0305
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (”“±ﬂΩÁƒ⁄)
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0303:

		cmp		edx, eax
		jl		loc_DrawSpriteWithColor_0304
		push	eax
		push	edx
		mov		ecx, eax
		mov     ebx, lpPalette

loc_DrawSpriteWithColor_Loop30:

		movzx	eax, byte ptr[esi]
		inc		esi
//		mov     dx, [ebx + eax * 2]
        xor     edx,edx 


		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteWithColor_Loop30

		pop		edx
		pop		eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0301
		neg		edx
		jmp		loc_DrawSpriteWithColor_0305

//---------------------------------------------------------------------------
// ¡¨–¯µ„µƒ∏ˆ ˝ (eax) > ≤√ºı∫ÛµƒøÌ∂» (edx)
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0304:

		push	eax
		push	edx
		mov		ecx, edx
		mov     ebx, lpPalette

loc_DrawSpriteWithColor_Loop31:

		movzx	eax, byte ptr[esi]
		inc		esi
//		mov     dx, [ebx + eax * 2]
        xor     edx, edx
		push    ebx
        push    ecx
		
        mov		ax, dx
		mov		bx, ax
		sal		eax, 16
		mov		ax, bx
		and		eax, nMask32

		mov		ecx, nColor
		mov		bx, cx
		sal		ecx, 16
		mov		cx, bx
		and		ecx, nMask32

		mov		ebx, nAlpha
		mul		ebx
		neg		ebx
		add		ebx, 0x20
		xchg	eax, ecx
		mul		ebx
		add		eax, ecx
		sar		eax, 5

		and		eax, nMask32
		mov		bx, ax
		sar		eax, 16
		or		ax, bx
		mov     [edi], ax
        
		pop     ecx
		pop     ebx   

		inc		edi
		inc		edi
		dec     ecx
		jg      loc_DrawSpriteWithColor_Loop31

		pop		edx
		pop		eax
//---------------------------------------------------------------------------
// º∆À„≥¨π˝¡ÀºÙ≤√«¯≥§∂» => edx
//---------------------------------------------------------------------------
		sub		eax, edx
		mov		edx, eax
		add		esi, eax
		add		edi, eax
		add		edi, eax
//---------------------------------------------------------------------------
// ¥¶¿Ì≥¨π˝¡À”“±ﬂΩÁµƒ≤ø∑÷, edx = ≥¨π˝”“±ﬂΩÁ≤ø∑÷µƒ≥§∂»
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0305:

		mov		eax, edx
		mov		edx, Clipper.right
		sub		edx, eax
		jle		loc_DrawSpriteWithColor_0308

loc_DrawSpriteWithColor_0306:

		movzx	eax, byte ptr[esi]
		inc		esi
		movzx	ebx, byte ptr[esi]
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSpriteWithColor_0307
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha == 0 µƒœÒÀÿ (”“±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0306
		jmp		loc_DrawSpriteWithColor_0308
//---------------------------------------------------------------------------
// ¥¶¿Ì Alpha != 0 µƒœÒÀÿ (”“±ﬂΩÁÕ‚)
//---------------------------------------------------------------------------
loc_DrawSpriteWithColor_0307:

		add		esi, eax
		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawSpriteWithColor_0306

loc_DrawSpriteWithColor_0308:

		add		edi, nNextLine
		dec		Clipper.height
		jnz		loc_DrawSpriteWithColor_0300
		jmp		loc_DrawSpriteWithColor_exit

loc_DrawSpriteWithColor_exit:

	}
	pCanvas->UnlockCanvas();
}

static 	KMemClass	Buffer;
//---------------------------------------------------------------------------
void g_DrawSpriteBorder(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of sprite
	int nHeight = pNode->m_nHeight;// height of sprite
	BYTE* lpSprite = (PBYTE)pNode->m_pBitmap;// sprite pointer
//	int nPitch = pCanvas->m_nPitch;// canvas pitch
//	int nNextLine = nPitch - nWidth * 2;// next line add
    int nColor=pNode->m_nColor;
    int nAlpha=pNode->m_nAlpha;
	int nMask32 = pCanvas->m_nMask32;
	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;
	//µ±«∞¥˙¬ÎÕº–Œ◊Û”“Õ¨ ±±ª≤√ºı ±”–ŒÛ
	if (Clipper.left && Clipper.right)
		return;

	PBYTE	lpBitmap;
	BYTE	byPixel;
	BYTE	byAlpha;
	BYTE	byFlags;
	int		nTop = 0;
	int		nLeft = nWidth;
	int		nRight = 0;
	int		nBottom = nHeight;
	int		dx = 0;
	int		dy = 0;
	
	// alloc bitmap memory
	if (!Buffer.Alloc(nWidth * nHeight))
		return;
	
	// bitmap pointer
	lpBitmap = (PBYTE)Buffer.GetMemPtr();
	
	// draw sprite
	while (dy < nHeight)
	{
		while (dx < nWidth)
		{
			byPixel = *lpSprite++;
			byAlpha = *lpSprite++;
			if (byAlpha < 200)
			{
				memset(lpBitmap, 0, byPixel);
			}
			else
			{
				memset(lpBitmap, 1, byPixel);
			}
			if (byAlpha != 0)
				lpSprite += byPixel;
			lpBitmap += byPixel;
			dx += byPixel;
		}
		dx = 0;
		dy++;
	}

	// draw border line horizontal
	for (dy = 0; dy < nHeight; dy++)
	{
		lpBitmap = (PBYTE)Buffer.GetMemPtr();
		lpBitmap += dy * nWidth;
		byFlags = 0;

		for (dx = 0; dx < nWidth; dx++)
		{
			if (*lpBitmap != byFlags)
			{
				if (byFlags == 0)
				{
					pCanvas->DrawPixel(nX + dx - 1, nY + dy, nColor);
					byFlags = 1;
				}
				else
				{
					pCanvas->DrawPixel(nX + dx, nY + dy, nColor);
					byFlags = 0;
				}
			}
			lpBitmap++;
		}
	}

	// draw border line vertical
	for (dx = 0; dx < nWidth; dx++)
	{
		lpBitmap = (PBYTE)Buffer.GetMemPtr();
		lpBitmap += dx;
		byFlags = 0;

		for (dy = 0; dy < nHeight; dy++)
		{
			if (*lpBitmap != byFlags)
			{
				if (byFlags == 0)
				{
					pCanvas->DrawPixel(nX + dx, nY + dy - 1, nColor);
					byFlags = 1;
				}
				else
				{
					pCanvas->DrawPixel(nX + dx, nY + dy, nColor);
					byFlags = 0;
				}
			}
			lpBitmap += nWidth;
		}
	}
}