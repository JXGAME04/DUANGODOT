//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KDrawSpriteAlpha.cpp
// Date:	2000.08.08
// Code:	WangWei(Daphnis), Wooy(Wu yue)
// Desc:	Sprite Alpha Drawing Function
//---------------------------------------------------------------------------
#include "KWin32.h"
#include "KCanvas.h"
#include "KDrawSpriteAlpha.h"
#include "DrawSpriteMP.h"

UINT g_BlendColor16b(UINT nSrcColor, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE)
{
	float sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
	float sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
	float sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
	float dR,dG,dB;
	if(bDstClr16b)
	{
		dR = (float)((nBlendColor & 0xf800) >> 8)/255.f;
		dG = (float)((nBlendColor & 0x07e0) >> 3)/255.f;
		dB = (float)((nBlendColor & 0x001f) << 3)/255.f;
	}
	else
	{
		dR = (float)((nBlendColor & 0xff0000) >> 16)/255.f;
		dG = (float)((nBlendColor & 0xff00) >> 8)/255.f;
		dB = (float)((nBlendColor & 0xff))/255.f;
	}
	switch (nMode)
	{
		case CBM_HUE:
			gBlendMode_Hue(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLOR:
			gBlendMode_Color(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SATURATION:
			gBlendMode_Sat(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LUMINOSITY:
			gBlendMode_Lum(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SCREEN:
			gBlendMode_Screen(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_OVERLAY:
			gBlendMode_OverLay(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DARKEN:
			gBlendMode_Darken(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LIGHTEN:
			gBlendMode_Lighten(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORDODGE:
			gBlendMode_ColorDodge(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORBURN:
			gBlendMode_ColorBurn(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_HARDLIGHT:
			gBlendMode_HardLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SOFTLIGHT:
			gBlendMode_SoftLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DIFFERENCE:
			gBlendMode_Difference(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_EXCLUSION:
			gBlendMode_Exclusion(dR,dG,dB,sR,sG,sB);
			break;
		default: //CBM_MULTIPLY
			dR *= sR;
			dG *= sG;
			dB *= sB;
		break;
	}
	
	UINT nRetColor = (((UINT)(dB*255.f)) >> 3);
	nRetColor |= ((((UINT)(dR*255.f)) >> 3) << 11);
	nRetColor |= ((((UINT)(dG*255.f)) >> 2) << 5);
	return nRetColor;
}

UINT g_BlendColor16b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE, UINT usA = 0)
{
	float sA = (float)usA/255.f;
	float sR = (float)usR/255.f;
	float sG = (float)usG/255.f;
	float sB = (float)usB/255.f;
	float dR,dG,dB;
	if(bDstClr16b)
	{
		dR = (float)((nBlendColor & 0xf800) >> 8)/255.f;
		dG = (float)((nBlendColor & 0x07e0) >> 3)/255.f;
		dB = (float)((nBlendColor & 0x001f) << 3)/255.f;
	}
	else
	{
		dR = (float)((nBlendColor & 0xff0000) >> 16)/255.f;
		dG = (float)((nBlendColor & 0xff00) >> 8)/255.f;
		dB = (float)((nBlendColor & 0xff))/255.f;
	}
	switch (nMode)
	{
		case CBM_HUE:
			gBlendMode_Hue(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLOR:
			gBlendMode_Color(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SATURATION:
			gBlendMode_Sat(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LUMINOSITY:
			gBlendMode_Lum(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SCREEN:
			if(usA)
			{
				dR = (sR+dR-sR*dR)*sA + dR*(1.f-sA);
				dG = (sG+dG-sG*dG)*sA + dG*(1.f-sA);
				dB = (sB+dB-sB*dB)*sA + dB*(1.f-sA);
			}
			else
				gBlendMode_Screen(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_OVERLAY:
			gBlendMode_OverLay(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DARKEN:
			gBlendMode_Darken(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LIGHTEN:
			gBlendMode_Lighten(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORDODGE:
			gBlendMode_ColorDodge(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORBURN:
			gBlendMode_ColorBurn(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_HARDLIGHT:
			gBlendMode_HardLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SOFTLIGHT:
			gBlendMode_SoftLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DIFFERENCE:
			gBlendMode_Difference(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_EXCLUSION:
			gBlendMode_Exclusion(dR,dG,dB,sR,sG,sB);
			break;
		default: //CBM_MULTIPLY
			dR *= sR;
			dG *= sG;
			dB *= sB;
		break;
	}
	
	UINT nRetColor = (((UINT)(dB*255.f)) >> 3);
	nRetColor |= ((((UINT)(dR*255.f)) >> 3) << 11);
	nRetColor |= ((((UINT)(dG*255.f)) >> 2) << 5);
	return nRetColor;
}

UINT g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT usA = 0)
{
	float sA = (float)usA/255.f;
	float sR = (float)usR/255.f;
	float sG = (float)usG/255.f;
	float sB = (float)usB/255.f;
	float dR = (float)((nBlendColor & 0xff0000) >> 16)/255.f;
	float dG = (float)((nBlendColor & 0xff00) >> 8)/255.f;
	float dB = (float)(nBlendColor & 0xff)/255.f;
	
	switch (nMode)
	{
		case CBM_HUE:
			gBlendMode_Hue(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLOR:
			gBlendMode_Color(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SATURATION:
			gBlendMode_Sat(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LUMINOSITY:
			gBlendMode_Lum(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SCREEN:
			if(usA)
			{
				dR = (sR+dR-sR*dR)*sA + dR*(1.f-sA);
				dG = (sG+dG-sG*dG)*sA + dG*(1.f-sA);
				dB = (sB+dB-sB*dB)*sA + dB*(1.f-sA);
			}
			else
				gBlendMode_Screen(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_OVERLAY:
			gBlendMode_OverLay(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DARKEN:
			gBlendMode_Darken(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LIGHTEN:
			gBlendMode_Lighten(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORDODGE:
			gBlendMode_ColorDodge(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORBURN:
			gBlendMode_ColorBurn(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_HARDLIGHT:
			gBlendMode_HardLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SOFTLIGHT:
			gBlendMode_SoftLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DIFFERENCE:
			gBlendMode_Difference(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_EXCLUSION:
			gBlendMode_Exclusion(dR,dG,dB,sR,sG,sB);
			break;
		default: //CBM_MULTIPLY
			dR *= sR;
			dG *= sG;
			dB *= sB;
		break;
	}
	
	UINT nRetColor = (UINT)(dB*255.f);
	nRetColor |= ((UINT)(dR*255.f)) << 16;
	nRetColor |= ((UINT)(dG*255.f)) << 8;
	return nRetColor;
}

void g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode,
				UINT& uR, UINT& uG, UINT& uB)
{
	float sR = (float)usR/255.f;
	float sG = (float)usG/255.f;
	float sB = (float)usB/255.f;
	float dR = (float)((nBlendColor & 0xff0000) >> 16)/255.f;
	float dG = (float)((nBlendColor & 0xff00) >> 8)/255.f;
	float dB = (float)(nBlendColor & 0xff)/255.f;
	
	switch (nMode)
	{
		case CBM_HUE:
			gBlendMode_Hue(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLOR:
			gBlendMode_Color(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SATURATION:
			gBlendMode_Sat(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LUMINOSITY:
			gBlendMode_Lum(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SCREEN:
			gBlendMode_Screen(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_OVERLAY:
			gBlendMode_OverLay(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DARKEN:
			gBlendMode_Darken(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_LIGHTEN:
			gBlendMode_Lighten(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORDODGE:
			gBlendMode_ColorDodge(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_COLORBURN:
			gBlendMode_ColorBurn(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_HARDLIGHT:
			gBlendMode_HardLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_SOFTLIGHT:
			gBlendMode_SoftLight(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_DIFFERENCE:
			gBlendMode_Difference(dR,dG,dB,sR,sG,sB);
			break;
		case CBM_EXCLUSION:
			gBlendMode_Exclusion(dR,dG,dB,sR,sG,sB);
			break;
		default: //CBM_MULTIPLY
			dR *= sR;
			dG *= sG;
			dB *= sB;
		break;
	}
	
	uR = (UINT)(dR*255.f);
	uG = (UINT)(dG*255.f);
	uB = (UINT)(dB*255.f);
}
//---------------------------------------------------------------------------
// ∫Ø ˝:	Draw Sprite nAlpha
// π¶ƒ‹:	ªÊ÷∆256…´SpriteŒªÕº(≤ª¥¯‘§‰÷»æ)
// ≤Œ ˝:	KDrawNode*, KCanvas* 
// ∑µªÿ:	void
//---------------------------------------------------------------------------
void g_DrawSpriteAlpha(void* node, void* canvas)
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
	int nMask32 = pCanvas->m_nMask32;	// rgb mask32
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;

	__asm
	{
        mov     eax, pPalette
        movd    mm0, eax        // mm0: pPalette

        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        mov     eax, nMask32
        movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha

        // mm4: temp use

        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax


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
				movd	edx, mm1    // mm1: Clipper.width
				_DrawFullLineSection_LineLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
                    lea     edi, [edi + eax * 2]
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

						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							movd     ebx, mm0   // mm0: pPalette
                            
						    _DrawFullLineSection_CopyPixel_:
							{
								copy_pixel_use_eax
                                dec     ecx
                                jnz     _DrawFullLineSection_CopyPixel_
							}

							or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}

						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
							shr		ebx, 3
                            movd    mm3, ebx    // mm3: nAlpha
							_DrawFullLineSection_HalfAlphaPixel_:
							{
								mix_2_pixel_color_alpha_use_eabdx
								loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							movd    edx, mm6
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
			movd	edx, mm1    // mm1: Clipper.width
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
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_LineLocal_DirectCopy_:
				{
					movd     ebx, mm0 // mm0: pPalette
					_DrawPartLineSection_CopyPixel_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_CopyPixel_
					}
					jmp		_DrawPartLineSection_LineLocal_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_HalfAlphaPixel_
					}
					movd    edx, mm6
					jmp		_DrawPartLineSection_LineLocal_
				}
			}
			_DrawPartLineSection_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				movd	mm5, ebx
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
				{
					movd    ebx,  mm0   // mm0: pPalette
					_DrawPartLineSection_CopyPixel_Part_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_CopyPixel_Part_
					}
			
					dec		Clipper.height
					jz		_EXIT_WAY_
					neg		edx
					mov		ebx, 255
					jmp		_DrawPartLineSection_LineSkip_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_HalfAlphaPixel_Part_
					}
					movd	ebx, mm5
					movd    edx, mm6
					neg		edx
					dec		Clipper.height
					jg		_DrawPartLineSection_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipLeft_Line_:
		{
			mov		eax, edx
			movd	edx, mm1    // mm1: Clipper.width
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
				sub		edx, eax		//œ»∞—eaxºı¡À£¨’‚—˘··√ÊæÕø…“‘≤ª–Ë“™±£¡Ùeax¡À
				mov		ecx, eax
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_
						
				//_DrawPartLineSection_SkipLeft_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
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

				_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipLeft_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipLeft_HalfAlphaPixel_
					}
					movd    edx, mm6
					or		edx, edx
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipLeft_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipRight_Line_:
		{
			movd	edx, mm1    // mm1: Clipper.width
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
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
					_DrawPartLineSection_SkipRight_CopyPixel_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_SkipRight_CopyPixel_
					}
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_
					}
					movd	edx, mm6
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
			}
			_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				movd	mm5, ebx
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_Part_:
				{
					movd    ebx, mm0 // mm0: pPalette
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
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_
					}
					movd	ebx, mm5
					movd	edx, mm6
					neg		edx
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipRight_LineSkip_//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
					jmp		_EXIT_WAY_
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteAlpha(void* node, void* canvas, int nExAlpha)
{
	if (nExAlpha <= 0)
		return;

	if (nExAlpha >= 254)
	{
		g_DrawSpriteAlpha(node, canvas);
		return;
	}
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
	int nMask32 = pCanvas->m_nMask32;	// rgb mask32
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	__asm
	{
        mov     eax, pPalette
        movd    mm0, eax        // mm0: pPalette

        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        mov     eax, nMask32
        movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha

        // mm4: temp use

        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax


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
				movd	edx, mm1    // mm1: Clipper.width
				_DrawFullLineSection_LineLocal_:
				{
					read_alpha_2_ebx_run_length_2_eax

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
                    lea     edi, [edi + eax * 2]	//ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
						mov		eax, nExAlpha
						imul	ebx, eax
						shr		ebx, 8
						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							movd     ebx, mm0   // mm0: pPalette
                            
                            sub ecx, 4
                            jl  _DrawFullLineSection_CopyPixel_continue
							_DrawFullLineSection_CopyPixel4_:
							{
								copy_4pixel_use_eax
                                
                                sub ecx, 4
                                jg     _DrawFullLineSection_CopyPixel4_
							}
							_DrawFullLineSection_CopyPixel_continue:
                            add ecx, 4
                            jz _DrawFullLineSection_CopyPixel_End 

						    _DrawFullLineSection_CopyPixel_:
							{
								copy_pixel_use_eax
                                dec     ecx
                                jnz     _DrawFullLineSection_CopyPixel_
							}
                            _DrawFullLineSection_CopyPixel_End:

							or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}

						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
							shr		ebx, 3
                            movd    mm3, ebx    // mm3: nAlpha
							_DrawFullLineSection_HalfAlphaPixel_:
							{
								mix_2_pixel_color_alpha_use_eabdx
								loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							movd    edx, mm6
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
			movd	edx, mm1    // mm1: Clipper.width
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
				mov		eax, nExAlpha
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_LineLocal_DirectCopy_:
				{
					movd     ebx, mm0 // mm0: pPalette
					_DrawPartLineSection_CopyPixel_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_CopyPixel_
					}
					jmp		_DrawPartLineSection_LineLocal_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_HalfAlphaPixel_
					}
					movd    edx, mm6
					jmp		_DrawPartLineSection_LineLocal_
				}
			}
			_DrawPartLineSection_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				mov		eax, nExAlpha
				movd	mm5, ebx
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
				{
					movd    ebx,  mm0   // mm0: pPalette
					_DrawPartLineSection_CopyPixel_Part_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_CopyPixel_Part_
					}
			
					dec		Clipper.height
					jz		_EXIT_WAY_
					neg		edx
					mov		ebx, 255
					jmp		_DrawPartLineSection_LineSkip_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_HalfAlphaPixel_Part_
					}
					movd	ebx, mm5
					movd    edx, mm6
					neg		edx
					dec		Clipper.height
					jg		_DrawPartLineSection_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipLeft_Line_:
		{
			mov		eax, edx
			movd	edx, mm1    // mm1: Clipper.width
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
				sub		edx, eax		//œ»∞—eaxºı¡À£¨’‚—˘··√ÊæÕø…“‘≤ª–Ë“™±£¡Ùeax¡À
				mov		ecx, eax
				mov		eax, nExAlpha
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_
						
				//_DrawPartLineSection_SkipLeft_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
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

				_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipLeft_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipLeft_HalfAlphaPixel_
					}
					movd    edx, mm6
					or		edx, edx
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipLeft_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipRight_Line_:
		{
			movd	edx, mm1    // mm1: Clipper.width
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
				mov		eax, nExAlpha
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
					_DrawPartLineSection_SkipRight_CopyPixel_:
					{
						copy_pixel_use_eax
						loop	_DrawPartLineSection_SkipRight_CopyPixel_
					}
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_
					}
					movd	edx, mm6
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
			}
			_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				mov		eax, nExAlpha
				movd	mm5, ebx
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_Part_:
				{
					movd    ebx, mm0 // mm0: pPalette
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
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
					shr		ebx, 3
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx
						loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_
					}
					movd	ebx, mm5
					movd	edx, mm6
					neg		edx
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipRight_LineSkip_//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
					jmp		_EXIT_WAY_
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteBlendColor(void* node, void* canvas, UINT nColor)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if(nExAlpha <= 0)
		return;
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += Clipper.y * nPitch + Clipper.x * 2;
	BYTE* pPalette	= (BYTE*)pNode->m_pPalette;// palette pointer
	BYTE* pSprite = (BYTE*)pNode->m_pBitmap;	// sprite pointer
	int nMode = pNode->m_nColor;
	int nMask32 = pCanvas->m_nMask32;	// rgb mask32
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;//left:sË Æi”m m t tı tr∏i qua, right: sË Æi”m m t tı l“ ph∂i qua tr∏i
	UINT tmpAlpha, alpha, nSrcColor, nDstColor;
	int nPixelBatch, nWidth;
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
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawFullLineSection_LineLocal_Alpha_;
		pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
		nWidth -= nPixelBatch;
		if(nWidth > 0)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	_DrawFullLineSection_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		while(nPixelBatch)
		{
			//vœ 1 Æi”m vÌi alpha = 255
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
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
		if(!Clipper.left)
			goto	_DrawPartLineSection_SkipRight_Line_;
		if(!Clipper.right)
			goto	_DrawPartLineSection_SkipLeft_Line_;
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
			pBuffer += nPixelBatch*2;
			goto	_DrawPartLineSection_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*2;
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
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_HalfAlpha_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_Part_;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		goto	_DrawPartLineSection_LineSkip_;
	_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		alpha = tmpAlpha;
		goto	_DrawPartLineSection_LineSkip_;
	//ph«n sprite bﬁ m t b™n tr∏i
	_DrawPartLineSection_SkipLeft_Line_:
		nWidth = Clipper.width;
		if(nSprSkip)
		{
			nPixelBatch = nSprSkip;
			goto	_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_;
		}
	_DrawPartLineSection_SkipLeft_LineLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_Alpha_;
		pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
		nWidth -= nPixelBatch;
		if(nWidth > 0)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipLeft_LineSkip_:
		pBuffer += nBuffNextLine;
		nSprSkip = nSprSkipPerLine;
	_DrawPartLineSection_SkipLeft_LineSkipLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_SkipLeft_Line_;
	_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
		{
			pSprite += nPixelBatch;
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_;
		}
		pSprite += nPixelBatch + nSprSkip;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_SkipLeft_Line_;
	_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkip_;
		goto	_EXIT_WAY_;
	//ph«n sprite bﬁ m t b™n ph∂i
	_DrawPartLineSection_SkipRight_Line_:
		nWidth = Clipper.width;
	_DrawPartLineSection_SkipRight_LineLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawPartLineSection_SkipRight_LineLocal_Alpha_;
		nWidth -= nPixelBatch;
		if(nWidth > 0)
		{
			pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
			goto	_DrawPartLineSection_SkipRight_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*2;
		nWidth = -nWidth;
	_DrawPartLineSection_SkipRight_LineSkip_:
		pBuffer += nBuffNextLine;
		nSprSkip = nSprSkipPerLine;
		if(nWidth)
		{
			nPixelBatch = nWidth;
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_;
		}
	_DrawPartLineSection_SkipRight_LineSkipLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_;
		goto	_DrawPartLineSection_SkipRight_Line_;
	_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
		pSprite += nPixelBatch;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_;
		goto	_DrawPartLineSection_SkipRight_Line_;
	_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		if(nWidth <= 0)
		{
			nPixelBatch += nWidth;
			goto	_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_;
		}
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_SkipRight_LineLocal_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_SkipRight_LineLocal_;
	_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((WORD*)pBuffer) = nSrcColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
		alpha >>= 3;
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);			
			nSrcColor |= (nSrcColor << 16);
			nSrcColor &= nMask32;
			nDstColor = *((WORD*)pBuffer);
			nDstColor |= (nDstColor << 16);
			nDstColor &= nMask32;
			nDstColor = (nSrcColor - nDstColor)*alpha/32 + nDstColor;
			nDstColor &= nMask32;
			nSrcColor = (nDstColor >> 16) & 0x7e0;
			nDstColor = (nDstColor | nSrcColor) & 0xffff;
			*((WORD*)pBuffer) = nDstColor;
			pBuffer += 2;
			nPixelBatch--;
		}
		alpha = tmpAlpha;
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteBlendColor32b(void* node, void* canvas, UINT nColor)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if(nExAlpha <= 0)
		return;
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
	int nMode = pNode->m_nColor;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;//left:sË Æi”m m t tı tr∏i qua, right: sË Æi”m m t tı l“ ph∂i qua tr∏i
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,R2,G2,B2;
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
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		while(nPixelBatch)
		{
			//vœ 1 Æi”m vÌi alpha = 255
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nSrcColor |= 0xff000000;
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
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			g_BlendColor32b(R,G,B, nColor, nMode, R,G,B);			
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
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
		//if(!Clipper.left)
		//	goto	_DrawPartLineSection_SkipRight_Line_;
		//if(!Clipper.right)
		//	goto	_DrawPartLineSection_SkipLeft_Line_;
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
			pBuffer += nPixelBatch*4;
			goto	_DrawPartLineSection_LineLocal_;
		}
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
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_;
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nSrcColor |= 0xff000000;
			*((UINT*)pBuffer) = nSrcColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			g_BlendColor32b(R,G,B, nColor, nMode, R,G,B);			
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_Part_;
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nSrcColor |= 0xff000000;
			*((UINT*)pBuffer) = nSrcColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		goto	_DrawPartLineSection_LineSkip_;
	_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			g_BlendColor32b(R,G,B, nColor, nMode, R,G,B);			
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		alpha = tmpAlpha;
		goto	_DrawPartLineSection_LineSkip_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteScreen(void* node, void* canvas, UINT nColor)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if(nExAlpha <= 0)
		return;
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += Clipper.y * nPitch + Clipper.x * 2;
	BYTE* pPalette	= (BYTE*)pNode->m_pPalette;// palette pointer
	BYTE* pSprite = (BYTE*)pNode->m_pBitmap;	// sprite pointer
	int nMode = pNode->m_nColor;
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;//left:sË Æi”m m t tı tr∏i qua, right: sË Æi”m m t tı l“ ph∂i qua tr∏i
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B;
	int nPixelBatch, nWidth;
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
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawFullLineSection_LineLocal_Alpha_;
		pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
		nWidth -= nPixelBatch;
		if(nWidth > 0)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	_DrawFullLineSection_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
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
		if(!Clipper.left)
			goto	_DrawPartLineSection_SkipRight_Line_;
		if(!Clipper.right)
			goto	_DrawPartLineSection_SkipLeft_Line_;
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
			pBuffer += nPixelBatch*2;
			goto	_DrawPartLineSection_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*2;
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
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		alpha = tmpAlpha;
		goto	_DrawPartLineSection_LineSkip_;
	//ph«n sprite bﬁ m t b™n tr∏i
	_DrawPartLineSection_SkipLeft_Line_:
		nWidth = Clipper.width;
		if(nSprSkip)
		{
			nPixelBatch = nSprSkip;
			goto	_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_;
		}
	_DrawPartLineSection_SkipLeft_LineLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_Alpha_;
		pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
		nWidth -= nPixelBatch;
		if(nWidth > 0)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipLeft_LineSkip_:
		pBuffer += nBuffNextLine;
		nSprSkip = nSprSkipPerLine;
	_DrawPartLineSection_SkipLeft_LineSkipLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_SkipLeft_Line_;
	_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
		{
			pSprite += nPixelBatch;
			goto	_DrawPartLineSection_SkipLeft_LineSkipLocal_;
		}
		pSprite += nPixelBatch + nSprSkip;
		nSprSkip = -nSprSkip;
		goto	_DrawPartLineSection_SkipLeft_Line_;
	_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
		}
		if(nWidth)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkip_;
		goto	_EXIT_WAY_;
	//ph«n sprite bﬁ m t b™n ph∂i
	_DrawPartLineSection_SkipRight_Line_:
		nWidth = Clipper.width;
	_DrawPartLineSection_SkipRight_LineLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
		if(alpha)
			goto	_DrawPartLineSection_SkipRight_LineLocal_Alpha_;
		nWidth -= nPixelBatch;
		if(nWidth > 0)
		{
			pBuffer += nPixelBatch*2; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
			goto	_DrawPartLineSection_SkipRight_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*2;
		nWidth = -nWidth;
	_DrawPartLineSection_SkipRight_LineSkip_:
		pBuffer += nBuffNextLine;
		nSprSkip = nSprSkipPerLine;
		if(nWidth)
		{
			nPixelBatch = nWidth;
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_;
		}
	_DrawPartLineSection_SkipRight_LineSkipLocal_:
		nPixelBatch = *(pSprite++);
		alpha = *(pSprite++);
	_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
		if(alpha)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_;
		goto	_DrawPartLineSection_SkipRight_Line_;
	_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
		pSprite += nPixelBatch;
		nSprSkip -= nPixelBatch;
		if(nSprSkip > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkipLocal_;
		goto	_DrawPartLineSection_SkipRight_Line_;
	_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
		nWidth -= nPixelBatch;
		if(nWidth <= 0)
		{
			nPixelBatch += nWidth;
			goto	_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_;
		}
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
		}
		goto	_DrawPartLineSection_SkipRight_LineLocal_;
	_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor16b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xf800) >> 8;
						G = (nSrcColor & 0x07e0) >> 3;
						B = (nSrcColor & 0x001f) << 3;
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((WORD*)pBuffer);
						nDstColor = g_BlendColor16b(R,G,B, nDstColor, CBM_SCREEN, TRUE, alpha);
						*((WORD*)pBuffer) = nDstColor;
					}
					pBuffer += 2;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 2;
				nPixelBatch--;
			}
		}
		alpha = tmpAlpha;
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteScreen32b(void* node, void* canvas, UINT nColor)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if(nExAlpha <= 0)
		return;
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
	int nMode = pNode->m_nColor;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;//left:sË Æi”m m t tı tr∏i qua, right: sË Æi”m m t tı l“ ph∂i qua tr∏i
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B;
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
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xff0000) >> 16;
						G = (nSrcColor & 0xff00) >> 8;
						B = (nSrcColor & 0xff);
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 4;
				nPixelBatch--;
			}
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
	//	if(!Clipper.left)
	//		goto	_DrawPartLineSection_SkipRight_Line_;
	//	if(!Clipper.right)
	//		goto	_DrawPartLineSection_SkipLeft_Line_;
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
			pBuffer += nPixelBatch*4;
			goto	_DrawPartLineSection_LineLocal_;
		}
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
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xff0000) >> 16;
						G = (nSrcColor & 0xff00) >> 8;
						B = (nSrcColor & 0xff);
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 4;
				nPixelBatch--;
			}
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha)
		{
			if(nColor)
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
						R = (nSrcColor & 0xff0000) >> 16;
						G = (nSrcColor & 0xff00) >> 8;
						B = (nSrcColor & 0xff);
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
			else
			{
				while(nPixelBatch)
				{
					R = *(pPalette + 3*(*pSprite));
					G = *(pPalette + 3*(*pSprite) + 1);
					B = *(pPalette + 3*(*pSprite) + 2);
					pSprite++;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
						nDstColor |= 0xff000000;
						*((UINT*)pBuffer) = nDstColor;
					}
					pBuffer += 4;
					nPixelBatch--;
				}
			}
		}
		else
		{
			while(nPixelBatch)
			{
				pSprite++;
				pBuffer += 4;
				nPixelBatch--;
			}
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		alpha = tmpAlpha;
		goto	_DrawPartLineSection_LineSkip_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteAlpha32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	// pBuffer lµ khung vœ thµnh ph»m
	int nPitch;
	void* pBuffer = pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer = (char*)(pBuffer) + Clipper.y * nPitch;
	void* pPalette	= pNode->m_pPalette;// palette pointer
	void* pSprite = pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	__asm
	{
        mov     eax, pPalette
        movd    mm0, eax        // mm0: pPalette

        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        //mov     eax, nMask32
        //movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha
        // mm4: temp use
        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax

		//Æi Æ’n pBuffer + Clipper.y * nPitch + Clipper.x*4
		mov		edi, pBuffer
		mov		eax, Clipper.x
		imul	eax, 4
		add		edi, eax

		//d˜ li÷u sprite
		mov		esi, pSprite

		//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
			or		edx, edx
			jz		_SkipSpriteAheadContentEnd_

			_SkipSpriteAheadContentLocalStart_:
			{	//Æ‰c sË l≠Óng Æi”m eax vµ alpha ebx (2 bytes)
				read_alpha_2_ebx_run_length_2_eax
				or		ebx, ebx
				jnz		_SkipSpriteAheadContentLocalAlpha_
				sub		edx, eax
				jg		_SkipSpriteAheadContentLocalStart_
				neg		edx	//edx <= 0 sË l≠Óng v≠Ót qu∏ cÒa nSprSkip
				jmp		_SkipSpriteAheadContentEnd_

				_SkipSpriteAheadContentLocalAlpha_://n’u alpha > 0 th◊ esi+
				{
					add		esi, eax
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					add		esi, edx	//edx <= 0 trı lπi esi ptr 
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
			//vœ full spr kh´ng »n ph«n nµo			
			_DrawFullLineSection_Line_:
			{
				movd	edx, mm1    // mm1: Clipper.width
				_DrawFullLineSection_LineLocal_:
				{	//Æ‰c sË l≠Óng Æi”m eax vµ alpha ebx (2 bytes)
					read_alpha_2_ebx_run_length_2_eax
					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
                    lea     edi, [edi + eax * 4]//nh∂y edi qua sË l≠Óng Æi”m eax
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
						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_
						//vœ Æi”m kÃm theo alpha 0xff000000
						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							movd     ebx, mm0   // mm0: pPalette
                            sub ecx, 4
                            jl  _DrawFullLineSection_CopyPixel_continue
							_DrawFullLineSection_CopyPixel4_:
							{	//vœ 1 l«n 4 Æi”m
								copy_4pixel_use_eax_32b
                                sub ecx, 4
                                jg     _DrawFullLineSection_CopyPixel4_
							}
							_DrawFullLineSection_CopyPixel_continue:
                            add ecx, 4
                            jz _DrawFullLineSection_CopyPixel_End 

						    _DrawFullLineSection_CopyPixel_:
							{	//vœ tıng Æi”m
								copy_pixel_use_eax_32b
                                dec     ecx
                                jnz     _DrawFullLineSection_CopyPixel_
							}
                            _DrawFullLineSection_CopyPixel_End:

							or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}
						//vœ Æi”m kÃm theo alpha < 255
						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
                            movd    mm3, ebx    // mm3: nAlpha
							_DrawFullLineSection_HalfAlphaPixel_:
							{
								mix_2_pixel_color_alpha_use_eabdx_32b
								dec ecx
								jnz	_DrawFullLineSection_HalfAlphaPixel_
							}
							movd    edx, mm6
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
			movd	edx, mm1    // mm1: Clipper.width
			or		eax, eax
			jnz		_DrawPartLineSection_LineLocal_CheckAlpha_
			_DrawPartLineSection_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				_DrawPartLineSection_LineLocal_CheckAlpha_:
				or		ebx, ebx
				jnz		_DrawPartLineSection_LineLocal_Alpha_
				lea     edi, [edi + eax * 4]//nh∂y edi qua sË l≠Óng Æi”m eax
				sub		edx, eax
				jg		_DrawPartLineSection_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_
				
				lea     edi, [edi + edx * 4]//nh∂y edi qua sË l≠Óng Æi”m edx
				neg		edx
			}
			
			_DrawPartLineSection_LineSkip_:
			{
				add		edi, nBuffNextLine
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
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_LineLocal_DirectCopy_:
				{
					movd     ebx, mm0 // mm0: pPalette
					_DrawPartLineSection_CopyPixel_:
					{
						copy_pixel_use_eax_32b
						dec ecx
						jnz	_DrawPartLineSection_CopyPixel_
					}
					jmp		_DrawPartLineSection_LineLocal_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx_32b
						dec ecx
						jnz	_DrawPartLineSection_HalfAlphaPixel_
					}
					movd    edx, mm6
					jmp		_DrawPartLineSection_LineLocal_
				}
			}
			_DrawPartLineSection_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
				{
					movd    ebx,  mm0   // mm0: pPalette
					_DrawPartLineSection_CopyPixel_Part_:
					{
						copy_pixel_use_eax_32b
						dec ecx
						jnz	_DrawPartLineSection_CopyPixel_Part_
					}
			
					dec		Clipper.height
					jz		_EXIT_WAY_
					neg		edx
					mov		ebx, 255
					jmp		_DrawPartLineSection_LineSkip_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx_32b
						dec ecx
						jnz	_DrawPartLineSection_HalfAlphaPixel_Part_
					}
					movd	ebx, mm3
					movd    edx, mm6
					neg		edx
					dec		Clipper.height
					jg		_DrawPartLineSection_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipLeft_Line_:
		{
			mov		eax, edx
			movd	edx, mm1    // mm1: Clipper.width
			or		eax, eax
			jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
			_DrawPartLineSection_SkipLeft_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
				or		ebx, ebx
				jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
				lea     edi, [edi + eax * 4]//nh∂y edi qua sË l≠Óng Æi”m eax
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
				sub		edx, eax		//œ»∞—eaxºı¡À£¨’‚—˘··√ÊæÕø…“‘≤ª–Ë“™±£¡Ùeax¡À
				mov		ecx, eax
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_
						
				//_DrawPartLineSection_SkipLeft_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
					_DrawPartLineSection_SkipLeft_CopyPixel_:
					{
						copy_pixel_use_eax_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipLeft_CopyPixel_
					}
					or		edx, edx
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipLeft_LineSkip_
					jmp		_EXIT_WAY_
				}

				_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
				{
					movd    mm6, edx
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipLeft_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipLeft_HalfAlphaPixel_
					}
					movd    edx, mm6
					or		edx, edx
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipLeft_LineSkip_
					jmp		_EXIT_WAY_
				}
			}
		}

		_DrawPartLineSection_SkipRight_Line_:
		{
			movd	edx, mm1    // mm1: Clipper.width
			_DrawPartLineSection_SkipRight_LineLocal_:
			{
				read_alpha_2_ebx_run_length_2_eax
				or		ebx, ebx
				jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
				lea     edi, [edi + eax * 4]//nh∂y edi qua sË l≠Óng Æi”m eax
				sub		edx, eax
				jg		_DrawPartLineSection_SkipRight_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_

				lea     edi, [edi + edx * 4]//nh∂y edi qua sË l≠Óng Æi”m edx
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
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_:
				{
					movd    ebx, mm0    // mm0: pPalette
					_DrawPartLineSection_SkipRight_CopyPixel_:
					{
						copy_pixel_use_eax_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipRight_CopyPixel_
					}
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_:
					{
						mix_2_pixel_color_alpha_use_eabdx_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipRight_HalfAlphaPixel_
					}
					movd	edx, mm6
					jmp		_DrawPartLineSection_SkipRight_LineLocal_
				}
			}
			_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
			{
				add		eax, edx
				mov		ecx, eax
				cmp		ebx, 255
				jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_
					
				//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_Part_:
				{
					movd    ebx, mm0 // mm0: pPalette
					_DrawPartLineSection_SkipRight_CopyPixel_Part_:
					{
						copy_pixel_use_eax_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipRight_CopyPixel_Part_
					}
					neg		edx
					mov		ebx, 255	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipRight_LineSkip_
					jmp		_EXIT_WAY_
				}
				
				_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
				{
					movd    mm6, edx
                    movd    mm3, ebx    // mm3: nAlpha
					_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_:
					{
						mix_2_pixel_color_alpha_use_eabdx_32b
						dec ecx
						jnz	_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_
					}
					movd	edx, mm6
					neg		edx
					movd	ebx, mm3
					dec		Clipper.height
					jg		_DrawPartLineSection_SkipRight_LineSkip_//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
					jmp		_EXIT_WAY_
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawSpriteAlpha32b(void* node, void* canvas, int nExAlpha)
{
	if (nExAlpha <= 0)
		return;
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	// pBuffer lµ khung vœ thµnh ph»m
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
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,R2,G2,B2;
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
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
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
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
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
		//if(!Clipper.left)
		//	goto	_DrawPartLineSection_SkipRight_Line_;
		//if(!Clipper.right)
		//	goto	_DrawPartLineSection_SkipLeft_Line_;
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
			pBuffer += nPixelBatch*4;
			goto	_DrawPartLineSection_LineLocal_;
		}
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
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_;
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
	_DrawPartLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_Alpha_Part_:
		tmpAlpha = alpha;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawPartLineSection_LineLocal_HalfAlpha_Part_;
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
	_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
		while(nPixelBatch)
		{
			R = *(pPalette + 3*(*pSprite));
			G = *(pPalette + 3*(*pSprite) + 1);
			B = *(pPalette + 3*(*pSprite) + 2);
			pSprite++;
			nDstColor = *((UINT*)pBuffer);
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		alpha = tmpAlpha;
		goto	_DrawPartLineSection_LineSkip_;
	
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}
//»˝º∂alphaªÊ÷∆
void g_DrawSprite3LevelAlpha(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;

	void* pSprite = pNode->m_pBitmap;	// sprite pointer
	void* pPalette	= pNode->m_pPalette;// palette pointer

	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KClipper Clipper;
	if (!pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper))
		return;

	int nPitch;
	void* pBuffer = pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;

	int nMask32 = pCanvas->m_nMask32;	// rgb mask32

	// pBuffer÷∏œÚ∆¡ƒª∆µ„µƒ∆´“∆Œª÷√ (“‘◊÷Ω⁄º∆)
	pBuffer = (char*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;

	__asm
	{
        mov     eax, pPalette
        movd    mm0, eax        // mm0: pPalette

        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        mov     eax, nMask32
        movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha
        // mm4: 32 - nAlpha

        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax

		// πedi÷∏œÚcanvasªÊ÷∆∆µ„, πesi÷∏œÚÕºøÈ ˝æ›∆µ„,(Ã¯π˝nSprSkip∏ˆœÒµ„µƒÕº–Œ ˝æ›)
		mov		edi, pBuffer
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
				movd	edx, mm1    // mm1: Clipper.width
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
						movd    mm5, eax
						mov		ecx, eax

						cmp		ebx, 200
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							movd    ebx, mm0    // mm0: pPalette
							_DrawFullLineSection_CopyPixel_:
							{
								copy_pixel_use_eax
								loop	_DrawFullLineSection_CopyPixel_
							}

							movd    eax, mm5
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}

						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
        					movd    mm6, edx
							_DrawFullLineSection_HalfAlphaPixel_:
							{
								mix_2_pixel_color_use_eabdx
								loop	_DrawFullLineSection_HalfAlphaPixel_
							}
        					movd	edx, mm6
							movd    eax, mm5
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_

							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}
					}
				}
			}
		}

		_DrawPartLineSection_:
		{
			_DrawPartLineSection_Line_:
			{
				mov		eax, edx
				movd	edx, mm1    // mm1: Clipper.width
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
					cmp		eax, edx
					jnl		_DrawPartLineSection_LineLocal_Alpha_Part_		//≤ªƒ‹»´ª≠’‚eax∏ˆœ‡Õ¨alpha÷µµƒœÒµ„£¨∫Û√Ê”–µ„“—æ≠≥¨≥ˆ«¯”Ú

					movd	mm5, eax
					mov		ecx, eax
					cmp		ebx, 200
					jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
					//_DrawPartLineSection_LineLocal_DirectCopy_:
					{
						movd    ebx, mm0    // mm0: pPalette
						_DrawPartLineSection_CopyPixel_:
						{
							copy_pixel_use_eax
							loop	_DrawPartLineSection_CopyPixel_
						}						
						movd    eax, mm5
						sub		edx, eax
						jmp		_DrawPartLineSection_LineLocal_
					}
					
					_DrawPartLineSection_LineLocal_HalfAlpha_:
					{
    					movd    mm6, edx
						_DrawPartLineSection_HalfAlphaPixel_:
						{
							mix_2_pixel_color_use_eabdx
							loop	_DrawPartLineSection_HalfAlphaPixel_
						}
       					movd	edx, mm6
						movd    eax, mm5
						sub		edx, eax
						jmp		_DrawPartLineSection_LineLocal_
					}
				}

				_DrawPartLineSection_LineLocal_Alpha_Part_:
				{
					movd    mm5, eax
					mov		ecx, edx
					cmp		ebx, 200
					jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_
						
					//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
					{
						movd    ebx, mm0    // mm0: pPalette
						_DrawPartLineSection_CopyPixel_Part_:
						{
							copy_pixel_use_eax
							loop	_DrawPartLineSection_CopyPixel_Part_
						}						
						movd    eax, mm5
				
						dec		Clipper.height
						jz		_EXIT_WAY_

						sub		eax, edx
						mov		edx, eax
						mov		ebx, 255	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
						jmp		_DrawPartLineSection_LineSkip_
					}
					
					_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
					{
    					movd    mm6, edx
						movd    mm3, ebx    // mm3: nAlpha
						_DrawPartLineSection_HalfAlphaPixel_Part_:
						{
							mix_2_pixel_color_use_eabdx
							loop	_DrawPartLineSection_HalfAlphaPixel_Part_
						}
       					movd	edx, mm6
						movd    eax, mm5
						dec		Clipper.height
						jz		_EXIT_WAY_
						sub		eax, edx
						mov		edx, eax
						movd	ebx, mm3	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
						jmp		_DrawPartLineSection_LineSkip_
					}
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawAlphaRecImage(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if (nExAlpha <= 0)
		return;
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;
	// pBuffer lµ khung vœ thµnh ph»m
	int nPitch;
	void* pBuffer = pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer = (char*)(pBuffer) + Clipper.y * nPitch + Clipper.x*2;
	int nMask32 = pCanvas->m_nMask32;	// rgb mask32
	void* pSprite = pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	__asm
	{
        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        mov     eax, nMask32
        movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha
        // mm4: temp use
        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax

		//Æi Æ’n pBuffer + Clipper.y * nPitch + Clipper.x*2
		mov		edi, pBuffer

		//d˜ li÷u sprite
		mov		esi, pSprite

		//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
			or		edx, edx
			jz		_SkipSpriteAheadContentEnd_
			imul	edx, 4
			add		esi, edx
		}
		_SkipSpriteAheadContentEnd_:
		mov		eax, nSprSkipPerLine
		or		eax, eax
		jnz		_DrawPartLineSection_

		//_DrawFullLineSection_:
		{
			//vœ full spr kh´ng »n ph«n nµo			
			_DrawFullLineSection_Line_:
			{
				movd	edx, mm1    // mm1: Clipper.width
				_DrawFullLineSection_LineLocal_:
				{
					movzx	ebx, byte ptr[esi+3]	//alpha
					mov		eax, [esi]				//fullcolor
					add		esi, 4
					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
                    add     edi, 2
					dec		edx
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.height
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_
				
					_DrawFullLineSection_LineLocal_Alpha_:
					{
						mov		ecx, eax
						dec		edx
						mov		eax, nExAlpha
						imul	ebx, eax
						shr		ebx, 8
						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_
						//vœ Æi”m kÃm theo alpha 0xff000000
						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							mov		[edi], cx
							add		edi, 2
							or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}
						//vœ Æi”m kÃm theo alpha < 255
						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
							shr		ebx, 3
                            movd    mm3, ebx    // mm3: nAlpha
							mix_2_pixel_color_alpha_onrec_16							
							movd    edx, mm6
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
		}
		_DrawPartLineSection_:
		_DrawPartLineSection_Line_:
		{
			movd	edx, mm1    // mm1: Clipper.width
			_DrawPartLineSection_LineLocal_:
			{
				movzx	ebx, byte ptr[esi+3]	//alpha
				mov		eax, [esi]				//fullcolor
				add		esi, 4
				or		ebx, ebx
				jnz		_DrawPartLineSection_LineLocal_Alpha_
				add     edi, 2
				dec		edx
				jg		_DrawPartLineSection_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_
			}
			
			_DrawPartLineSection_LineSkip_:
			{
				add		edi, nBuffNextLine
				mov		edx, nSprSkipPerLine
				imul	edx, 4
				add		esi, edx
				jmp		_DrawPartLineSection_Line_
			}
			_DrawPartLineSection_LineLocal_Alpha_:
			{
				mov		ecx, eax
				dec		edx
				mov		eax, nExAlpha
				imul	ebx, eax
				shr		ebx, 8
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_LineLocal_DirectCopy_:
				{
					mov		[edi], cx
					add		edi, 2
					or		edx, edx
					jnz		_DrawPartLineSection_LineLocal_
					dec		Clipper.height
					jz		_EXIT_WAY_
					jmp		_DrawPartLineSection_LineSkip_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
                    shr		ebx, 3
					movd    mm3, ebx    // mm3: nAlpha
					mix_2_pixel_color_alpha_onrec_16
					movd    edx, mm6
					or		edx, edx
					jnz		_DrawPartLineSection_LineLocal_
					dec		Clipper.height
					jz		_EXIT_WAY_
					jmp		_DrawPartLineSection_LineSkip_
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawAlphaRecImageOpa(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;
	// pBuffer lµ khung vœ thµnh ph»m
	int nPitch;
	void* pBuffer = pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer = (char*)(pBuffer) + Clipper.y * nPitch + Clipper.x*2;
	int nMask32 = pCanvas->m_nMask32;	// rgb mask32
	void* pSprite = pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	int nSprSkip = pNode->m_nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	__asm
	{
        mov     eax, Clipper.width
        movd    mm1, eax        // mm1: Clipper.width

        mov     eax, nMask32
        movd    mm2, eax        // mm2: nMask32

        // mm3: nAlpha
        // mm4: temp use
        // mm7: push ecx, pop ecx
        // mm6: push edx, pop edx
        // mm5: push eax, pop eax

		//Æi Æ’n pBuffer + Clipper.y * nPitch + Clipper.x*2
		mov		edi, pBuffer

		//d˜ li÷u sprite
		mov		esi, pSprite

		//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
			or		edx, edx
			jz		_SkipSpriteAheadContentEnd_
			imul	edx, 4
			add		esi, edx
		}
		_SkipSpriteAheadContentEnd_:
		mov		eax, nSprSkipPerLine
		or		eax, eax
		jnz		_DrawPartLineSection_

		//_DrawFullLineSection_:
		{
			//vœ full spr kh´ng »n ph«n nµo			
			_DrawFullLineSection_Line_:
			{
				movd	edx, mm1    // mm1: Clipper.width
				_DrawFullLineSection_LineLocal_:
				{
					mov		ebx, 255	//alpha
					mov		eax, [esi]				//fullcolor
					add		esi, 4
					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
                    add     edi, 2
					dec		edx
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.height
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_
				
					_DrawFullLineSection_LineLocal_Alpha_:
					{
						mov		ecx, eax
						dec		edx
						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_
						//vœ Æi”m kÃm theo alpha 0xff000000
						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							mov		[edi], cx
							add		edi, 2
							or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}
						//vœ Æi”m kÃm theo alpha < 255
						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
							shr		ebx, 3
                            movd    mm3, ebx    // mm3: nAlpha
							mix_2_pixel_color_alpha_onrec_16							
							movd    edx, mm6
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
		}
		_DrawPartLineSection_:
		_DrawPartLineSection_Line_:
		{
			movd	edx, mm1    // mm1: Clipper.width
			_DrawPartLineSection_LineLocal_:
			{
				mov		ebx, 255	//alpha
				mov		eax, [esi]				//fullcolor
				add		esi, 4
				or		ebx, ebx
				jnz		_DrawPartLineSection_LineLocal_Alpha_
				add     edi, 2
				dec		edx
				jg		_DrawPartLineSection_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_
			}
			
			_DrawPartLineSection_LineSkip_:
			{
				add		edi, nBuffNextLine
				mov		edx, nSprSkipPerLine
				imul	edx, 4
				add		esi, edx
				jmp		_DrawPartLineSection_Line_
			}
			_DrawPartLineSection_LineLocal_Alpha_:
			{
				mov		ecx, eax
				dec		edx
				cmp		ebx, 255
				jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
				//_DrawPartLineSection_LineLocal_DirectCopy_:
				{
					mov		[edi], cx
					add		edi, 2
					or		edx, edx
					jnz		_DrawPartLineSection_LineLocal_
					dec		Clipper.height
					jz		_EXIT_WAY_
					jmp		_DrawPartLineSection_LineSkip_
				}
				
				_DrawPartLineSection_LineLocal_HalfAlpha_:
				{
					movd    mm6, edx
                    shr		ebx, 3
					movd    mm3, ebx    // mm3: nAlpha
					mix_2_pixel_color_alpha_onrec_16
					movd    edx, mm6
					or		edx, edx
					jnz		_DrawPartLineSection_LineLocal_
					dec		Clipper.height
					jz		_EXIT_WAY_
					jmp		_DrawPartLineSection_LineSkip_
				}
			}
		}
		_EXIT_WAY_:
        emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawAlphaRecImage32b(void* node, void* canvas, int bScrMode)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if (nExAlpha <= 0)
		return;
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	// pBuffer lµ khung vœ thµnh ph»m
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += Clipper.y * nPitch + Clipper.x*4;
	BYTE* pSprite = (BYTE*)pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	pSprite += (pNode->m_nWidth * Clipper.top + Clipper.left)*4;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT alpha, nSrcColor, nDstColor, R,G,B,R2,G2,B2;
	int nWidth;
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		nSrcColor = *((UINT*)pSprite);
		pSprite += 4;
		alpha = (nSrcColor & 0xff000000) >> 24;
		if(alpha)
			goto	_DrawFullLineSection_LineLocal_Alpha_;
        pBuffer += 4;
		nWidth--;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		pSprite += nSprSkipPerLine*4;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	_DrawFullLineSection_LineLocal_Alpha_:
		nWidth--;
		alpha = alpha*nExAlpha/255;
		if(alpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		//vœ Æi”m kÃm theo alpha 0xff000000
		if(bScrMode)
		{
			R = (nSrcColor >> 16) & 0xff;
			G = (nSrcColor >> 8) & 0xff;
			B = nSrcColor & 0xff;
			if(R > 8 || G > 8 || B > 8)
			{
				nDstColor = *((UINT*)pBuffer);
				nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
				nDstColor |= 0xff000000;
				*((UINT*)pBuffer) = nDstColor;
			}
		}
		else
		{
			*((UINT*)pBuffer) = nSrcColor;
		}
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		pSprite += nSprSkipPerLine*4;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	//vœ Æi”m kÃm theo alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		nDstColor = *((UINT*)pBuffer);
		R = (nSrcColor >> 16) & 0xff;
		G = (nSrcColor >> 8) & 0xff;
		B = nSrcColor & 0xff;
		if(bScrMode)
		{
			if(R > 8 || G > 8 || B > 8)
			{
				nDstColor = g_BlendColor32b(R,G,B, nDstColor, CBM_SCREEN, alpha);
				nDstColor |= 0xff000000;
				*((UINT*)pBuffer) = nDstColor;
			}
		}
		else
		{
			R2 = (nDstColor >> 16) & 0xff;
			G2 = (nDstColor >> 8) & 0xff;
			B2 = nDstColor & 0xff;
			R = (alpha*R + (255-alpha)*R2)/255;
			G = (alpha*G + (255-alpha)*G2)/255;
			B = (alpha*B + (255-alpha)*B2)/255;
			nDstColor = 0xff000000 | (R << 16) | (G << 8) | B;
			*((UINT*)pBuffer) = nDstColor;
		}
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		pSprite += nSprSkipPerLine*4;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
		goto	_EXIT_WAY_;
	_EXIT_WAY_:
	pCanvas->UnlockCanvas();
}

void g_DrawOpaRecImage32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	int nExAlpha = pNode->m_nAlpha;
	if (nExAlpha <= 0)
		return;
	// tπo khung vu´ng trong phπm vi khung game
	KClipper Clipper;
	if (pCanvas->MakeClip(pNode->m_nX, pNode->m_nY, pNode->m_nWidth, pNode->m_nHeight, &Clipper) == 0)
		return;

	// pBuffer lµ khung vœ thµnh ph»m
	int nPitch;
	BYTE* pBuffer = (BYTE*)pCanvas->LockCanvas(nPitch);
	if (pBuffer == NULL)
		return;
	pBuffer += Clipper.y * nPitch + Clipper.x*4;
	BYTE* pSprite = (BYTE*)pNode->m_pBitmap;	// sprite pointer
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	pSprite += (pNode->m_nWidth * Clipper.top + Clipper.left)*4;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	int nWidth;
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
		memcpy(pBuffer, pSprite, nWidth*4);
		pSprite += nWidth*4;
        pBuffer += nWidth*4;
		pBuffer += nBuffNextLine;
		pSprite += nSprSkipPerLine*4;
		Clipper.height--;
		if(Clipper.height)
			goto	_DrawFullLineSection_Line_;
	pCanvas->UnlockCanvas();
}
