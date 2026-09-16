/*****************************************************************************************
//  Õº–ŒµΩƒ⁄¥Ê«¯”Úµƒ≤Ÿ◊˜
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-11
*****************************************************************************************/
#include "ImageOperation.h"
#include "crtdbg.h"
#include "../../../Engine/Src/Colors.h"
//#include "../../../Engine/src/KDebug.h"
static unsigned int l_uRGBBitMask32 = 0x07e0f81f;

#define		read_alpha_2_ebx_run_length_2_eax		    \
				{	movzx	eax, byte ptr[esi]	}	    \
				{	movzx	ebx, byte ptr[esi + 1]	}	    \
				{	add		esi, 2   			}

#define		copy_pixel_use_eax		\
				{	xor	    eax, eax            }	\
				{	mov	    al, byte ptr [esi]	}	\
				{	add		esi, 1 				}	\
				{	mov		ax, [ebx + eax * 2]	}	\
				{	or	    eax, 0xff000000     }	\
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}
				
#define		copy_pixel_use_eax_32b	/*ebx palette 3 colors*/	\
				{	movzx   eax, byte ptr [esi]	}	\
				{	imul	eax, 3            	}   \
				{	movd	mm4, eax            }   \
				{	inc		esi					}	\
				{	push	ecx                 }   \
				{	push	edx                 }   \
				{	movzx	eax, byte ptr[ebx + eax]	}	\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm4            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
				{	pop		edx                 }   \
				{	pop		ecx                 }   
				
#define		mix_2_pixel_color_use_eabdx									\
				{	movd	mm7, ecx			}						\
                {   xor     eax, eax            }                       \
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	mov	    al, byte ptr[esi]	}						\
				{	inc		esi					}						\
				{	mov     dx, [ebx + eax * 2]	}	/*edx = ...rgb*/	\
				{	movd	ecx, mm2    		}	/* nMask32 */		\
				{	mov		ax, dx				}	/*eax = ...rgb*/	\
				{	shl		eax, 16				}	/*eax = rgb...*/	\
				{	mov		ax, dx				}	/*eax = rgbrgb*/	\
				{	and		eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
				{	mov		bx, dx				}	/*ebx = ...rgb*/	\
				{	shl		ebx, 16				}	/*ebx = rgb...*/	\
				{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
				{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
                {   lea     edx, [ebx + ebx * 2]}                       \
                {   add     eax, edx            }                       \
				{	shr		eax, 2				}	/*c = (3xc1+c2)/4*/	\
				{	and     eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov     dx, ax				}	/*edx = ...r.b*/	\
				{	shr     eax, 16				}	/*eax = ....g.*/	\
				{	add 	edi, 2				}						\
				{	or      ax, dx				}	/*eax = ...rgb*/	\
				{	movd     ecx, mm7			}                       \
				{	mov		[edi - 2], ax		}
/*
void g_ShowDebug(int n)
{
	g_DebugLog("----io [%d]",n);
}
							//push	edx
							//push	ecx
							//mov		eax,1
							//push	eax
							//call	g_ShowDebug
							//pop		eax
							//pop		ecx
							//pop		edx
*/
void RIO_Set16BitImageFormat(int b565)
{
	l_uRGBBitMask32 = b565 ? 0x07e0f81f : 0x03e07c1f;
}

struct KRClipperInfo
{
	int			x;			// ≤√ºı∫ÛµƒX◊¯±Í
	int			y;			// ≤√ºı∫ÛµƒY◊¯±Í
	int			width;		// ≤√ºı∫ÛµƒøÌ∂»
	int			height;		// ≤√ºı∫Ûµƒ∏ﬂ∂»
	int			left;		// …œ±ﬂΩÁ≤√ºÙ¡ø
	int			top;		// ◊Û±ﬂΩÁ≤√ºÙ¡ø
	int			right;		// ”“±ﬂΩÁ≤√ºÙ¡ø
};

static UINT g_BlendColor16b(UINT nSrcColor, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE)
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

static UINT g_BlendColor16b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, BOOL bDstClr16b = FALSE, UINT usA = 0)
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

static UINT g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode, UINT usA = 0)
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

static void g_BlendColor32b(UINT usR, UINT usG, UINT usB, UINT nBlendColor, int nMode,
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

static int  RIO_ClipCopyRect(int nX, int nY, int nSrcWidth, int nSrcHeight, int nDestWidth, int nDestHeight, KRClipperInfo* pClipper)
{
	_ASSERT(pClipper);
	// ≥ı ºªØ≤√ºı¡ø
	pClipper->x = nX;
	pClipper->y = nY;
	pClipper->width = nSrcWidth;
	pClipper->height = nSrcHeight;
	pClipper->top = 0;
	pClipper->left = 0;
	pClipper->right = 0;

	// …œ±ﬂΩÁ≤√ºı
	if (pClipper->y < 0)
	{
		pClipper->y = 0;
		pClipper->top = -nY;
		pClipper->height += nY;
	}
	if (pClipper->height <= 0)
		return 0;
	
	// œ¬±ﬂΩÁ≤√ºı
	if (pClipper->height > nDestHeight - pClipper->y)
		pClipper->height = nDestHeight - pClipper->y;
	if (pClipper->height <= 0)
		return 0;

	// ◊Û±ﬂΩÁ≤√ºı
	if (pClipper->x < 0)
	{
		pClipper->x = 0;
		pClipper->left = -nX;
		pClipper->width += nX;
	}
	if (pClipper->width <= 0)
		return 0;

	// ”“±ﬂΩÁ≤√ºı
	if (pClipper->width > nDestWidth - pClipper->x)
	{
		pClipper->right = pClipper->width + pClipper->x - nDestWidth;
		pClipper->width -= pClipper->right;
	}
	if (pClipper->width <= 0)
		return 0;
	
	return 1;
}

void RIO_CopySprToBuffer(void* pSprite, int nSprWidth, int nSprHeight, void* pPalette,
					 void* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY)
{
	//_ASSERT(pSpr && pBuffer && pPalette);
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer = (char*)(pBuffer) + Clipper.y * nPitch + Clipper.x*4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;

	__asm
	{
		// πedi÷∏œÚbufferªÊ÷∆∆µ„,	(“‘◊÷Ω⁄º∆)	
		mov		edi, pBuffer

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
					lea     edi, [edi + eax * 4]
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
				lea     edi, [edi + eax * 4]
				sub		edx, eax
				jg		_DrawPartLineSection_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_

				lea     edi, [edi + edx * 4]
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
				lea     edi, [edi + eax * 4]
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
				lea     edi, [edi + eax * 4]
				sub		edx, eax
				jg		_DrawPartLineSection_SkipRight_LineLocal_

				dec		Clipper.height
				jz		_EXIT_WAY_

				lea     edi, [edi + edx * 4]
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
				mov		ebx, 255
				dec		Clipper.height
				jg		_DrawPartLineSection_SkipRight_LineSkip_
				jmp		_EXIT_WAY_
			}
		}
		_EXIT_WAY_:
	}
}

void RIO_CopySprToBuffer32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
					 BYTE* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY)
{
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x*4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT alpha, nSrcColor, R,G,B;
	int nPixelBatch, nWidth, i;
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
		for(i = 0;i < nPixelBatch;++i)
		{
			*((UINT*)pBuffer) = 0xff000000;
			pBuffer += 4;
		} //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
			for(i = 0;i < nPixelBatch;++i)
			{
				*((UINT*)pBuffer) = 0xff000000;
				pBuffer += 4;
			}
			goto	_DrawPartLineSection_LineLocal_;
		}
		for(i = 0;i < (nPixelBatch + nWidth);++i)
		{
			*((UINT*)pBuffer) = 0xff000000;
			pBuffer += 4;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		//pBuffer += (nPixelBatch + nWidth)*4;
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
	return;
}

void RIO_CopySprToBufferAlpha(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
			BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
			pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
			goto	_DrawPartLineSection_SkipRight_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*4;
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_SkipRight_LineLocal_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		alpha = tmpAlpha;
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopySprToBufferAlpha32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
			BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha, int bSrcModRemoveBk)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;

	UINT tmpAlpha, alpha, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
			if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
			*((UINT*)pBuffer) = 0xff000000 | (R << 16) | (G << 8) | B;
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
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				nDstColor = (alpha << 24) | (R << 16) | (G << 8) | B;
			}
			else
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				{
				sR = (float)R/255.f;
				sG = (float)G/255.f;
				sB = (float)B/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
				}
			}
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
			if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
			*((UINT*)pBuffer) = 0xff000000 | (R << 16) | (G << 8) | B;
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
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				nDstColor = (alpha << 24) | (R << 16) | (G << 8) | B;
			}
			else
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				{
				sR = (float)R/255.f;
				sG = (float)G/255.f;
				sB = (float)B/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
				}
			}
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
			if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
			*((UINT*)pBuffer) = 0xff000000 | (R << 16) | (G << 8) | B;
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
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				nDstColor = (alpha << 24) | (R << 16) | (G << 8) | B;
			}
			else
			{
				if(!bSrcModRemoveBk || R > 8 || G > 8 || B > 8)
				{
				sR = (float)R/255.f;
				sG = (float)G/255.f;
				sB = (float)B/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
				}
			}
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
	return;
}

void RIO_CopySprToBufferScreen(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
	BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nMode, int nExAlpha, UINT nColor)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
			pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
			goto	_DrawPartLineSection_SkipRight_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*4;
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
					nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
					R = (nSrcColor & 0xf800) >> 8;
					G = (nSrcColor & 0x07e0) >> 3;
					B = (nSrcColor & 0x001f) << 3;
					if(R > 8 || G > 8 || B > 8)
					{
						nDstColor = *((UINT*)pBuffer);
						A2 = (nDstColor & 0xff000000) >> 24;
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
						dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
						dB = (float)((nDstColor & 0x001f) << 3)/255.f;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
						}
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
		alpha = tmpAlpha;
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_ConvertToHardScreen32b(int nWidth, int nHeight, unsigned int* pBuffer)
{
	int i,j;
	UINT alpha, nSrcColor, nDstColor, R,G,B,A2;
	float sA,sR,sG,sB,dA,dR,dG,dB;
	for(i = 0; i<nHeight;++i)
	for(j = 0; j<nWidth;++j)
	{
		int nIdx = i*nWidth+j;
		nSrcColor = *(pBuffer+nIdx);
		nDstColor = 0;
		R = (nSrcColor & 0xff0000)>>16;
		G = (nSrcColor & 0xff00)>>8;
		B = (nSrcColor & 0xff);
		alpha = (nSrcColor & 0xff000000)>>24;
		if(R > 8 || G > 8 || B > 8)
		{
			sR = (float)R/255.f;
			sG = (float)G/255.f;
			sB = (float)B/255.f;
			dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
			dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
			dB = (float)(nDstColor & 0xff)/255.f;
			A2 = (nDstColor & 0xff000000) >> 24;
			sA = (float)alpha/255.f;
			sR = sR + sR - sR*sR;
			sG = sG + sG - sG*sG;
			sB = sB + sB - sB*sB;
			dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
			if(dA < 0.68f)
			{
				dA -= 0.0942f;
				if(dA < 0.f)
					dA = 0.f;
			}
			sA *= dA;
			if(A2 == 0)
			{
				R = (UINT)(sR*255.f);
				G = (UINT)(sG*255.f);
				B = (UINT)(sB*255.f);
				A2 = (UINT)(sA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
			}
			else
			{
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
			}
		}
		*(pBuffer+nIdx) = nDstColor;
	}
}

void RIO_CopySprToBufferScreen32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
	BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nMode, int nExAlpha, UINT nColor)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
						nDstColor = *((UINT*)pBuffer);
						sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
						sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
						sB = (float)(nSrcColor & 0xff)/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
						nDstColor = *((UINT*)pBuffer);
						sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
						sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
						sB = (float)(nSrcColor & 0xff)/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
						nDstColor = *((UINT*)pBuffer);
						sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
						sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
						sB = (float)(nSrcColor & 0xff)/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
						sR = (float)R/255.f;
						sG = (float)G/255.f;
						sB = (float)B/255.f;
						dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
						dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
						dB = (float)(nDstColor & 0xff)/255.f;
						A2 = (nDstColor & 0xff000000) >> 24;
						sA = (float)alpha/255.f;
						sR = sR + sR - sR*sR;
						sG = sG + sG - sG*sG;
						sB = sB + sB - sB*sB;
						dA = 0.2126f * sR + 0.7152f * sG + 0.0722f * sB;
						if(dA < 0.68f)
						{
							dA -= 0.0942f;
							if(dA < 0.f)
								dA = 0.f;
						}
						sA *= dA;
						if(A2 == 0)
						{
							R = (UINT)(sR*255.f);
							G = (UINT)(sG*255.f);
							B = (UINT)(sB*255.f);
							A2 = (UINT)(sA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
						else
						{
							dA = (float)A2/255.f;
							dR = sR*sA + dR*dA*(1.f-sA);
							dG = sG*sA + dG*dA*(1.f-sA);
							dB = sB*sA + dB*dA*(1.f-sA);
							dA = sA + dA*(1.f-sA);
							dR /= dA;
							dG /= dA;
							dB /= dA;
							R = (UINT)(dR*255.f);
							G = (UINT)(dG*255.f);
							B = (UINT)(dB*255.f);
							A2 = (UINT)(dA*255.f);
							nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
						}
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
	return;
}

void RIO_CopySprToBufferBlendColor(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
	BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha, UINT nColor, int nMode)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_LineLocal_;
	_DrawPartLineSection_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
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
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
		pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		if(nWidth)
			goto	_DrawPartLineSection_SkipLeft_LineLocal_;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipLeft_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
			pBuffer += nPixelBatch*4; //ph«n nµy kh´ng vœ, chÿ nh∂y qua
			goto	_DrawPartLineSection_SkipRight_LineLocal_;
		}
		Clipper.height--;
		if(!Clipper.height)
			goto	_EXIT_WAY_;
		pBuffer += (nPixelBatch + nWidth)*4;
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		goto	_DrawPartLineSection_SkipRight_LineLocal_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
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
			*((UINT*)pBuffer) = nSrcColor | 0xff000000;
			pBuffer += 4;
			nPixelBatch--;
		}
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
		while(nPixelBatch)
		{
			nSrcColor = *((WORD*)(pPalette + 2*(*(pSprite++))));
			nSrcColor = g_BlendColor16b(nSrcColor, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
				sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
				sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
				dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
				dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
				dB = (float)((nDstColor & 0x001f) << 3)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
			}
			*((UINT*)pBuffer) = nDstColor;
			pBuffer += 4;
			nPixelBatch--;
		}
		alpha = tmpAlpha;
		nWidth = -nWidth;
		Clipper.height--;
		if(Clipper.height > 0)
			goto	_DrawPartLineSection_SkipRight_LineSkip_;
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopySprToBufferBlendColor32b(BYTE* pSprite, int nSprWidth, int nSprHeight, BYTE* pPalette,
	BYTE* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY, int nExAlpha, UINT nColor, int nMode)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT tmpAlpha, alpha, nSrcColor, nDstColor, R,G,B,A2;
	int nPixelBatch, nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
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
			*((UINT*)pBuffer) = 0xff000000 | nSrcColor;
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
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
				sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
				sB = (float)(nSrcColor & 0xff)/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
			}
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
			*((UINT*)pBuffer) = 0xff000000 | nSrcColor;
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
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
				sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
				sB = (float)(nSrcColor & 0xff)/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
			}
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
			*((UINT*)pBuffer) = 0xff000000 | nSrcColor;
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
			nSrcColor = g_BlendColor32b(R,G,B, nColor, nMode);
			nDstColor = *((UINT*)pBuffer);			
			A2 = (nDstColor & 0xff000000) >> 24;
			if(A2 == 0)
			{
				nDstColor = nSrcColor | (alpha << 24);
			}
			else
			{
				sR = (float)((nSrcColor & 0xff0000) >> 16)/255.f;
				sG = (float)((nSrcColor & 0xff00) >> 8)/255.f;
				sB = (float)(nSrcColor & 0xff)/255.f;
				dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
				dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
				dB = (float)(nDstColor & 0xff)/255.f;
				sA = (float)alpha/255.f;
				dA = (float)A2/255.f;
				dR = sR*sA + dR*dA*(1.f-sA);
				dG = sG*sA + dG*dA*(1.f-sA);
				dB = sB*sA + dB*dA*(1.f-sA);
				dA = sA + dA*(1.f-sA);
				dR /= dA;
				dG /= dA;
				dB /= dA;
				R = (UINT)(dR*255.f);
				G = (UINT)(dG*255.f);
				B = (UINT)(dB*255.f);
				A2 = (UINT)(dA*255.f);
				nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
			}
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
	return;
}

void RIO_CopyBitmap16ToBuffer(WORD* pBitmap, int nBmpWidth, int nBmpHeight,
					 BYTE* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY, int nExAlpha)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nBmpWidth, nBmpHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	pBitmap += nBmpWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT nSrcColor, nDstColor, R,G,B,A2;
	int nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
		
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		nWidth--;
		if(nExAlpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		nSrcColor = *(pBitmap++);
		*((UINT*)pBuffer) = nSrcColor | 0xff000000;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			pBitmap += nSprSkipPerLine;
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		nSrcColor = *(pBitmap++);
		nDstColor = *((UINT*)pBuffer);
		A2 = (nDstColor & 0xff000000) >> 24;
		if(A2 == 0)
		{
			nDstColor = nSrcColor | (nExAlpha << 24);
		}
		else
		{
			sR = (float)((nSrcColor & 0xf800) >> 8)/255.f;
			sG = (float)((nSrcColor & 0x07e0) >> 3)/255.f;
			sB = (float)((nSrcColor & 0x001f) << 3)/255.f;
			dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
			dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
			dB = (float)((nDstColor & 0x001f) << 3)/255.f;
			sA = (float)nExAlpha/255.f;
			dA = (float)A2/255.f;
			dR = sR*sA + dR*dA*(1.f-sA);
			dG = sG*sA + dG*dA*(1.f-sA);
			dB = sB*sA + dB*dA*(1.f-sA);
			dA = sA + dA*(1.f-sA);
			dR /= dA;
			dG /= dA;
			dB /= dA;
			R = (UINT)(dR*255.f);
			G = (UINT)(dG*255.f);
			B = (UINT)(dB*255.f);
			A2 = (UINT)(dA*255.f);
			nDstColor = (A2 << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
		}
		*((UINT*)pBuffer) = nDstColor;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			pBitmap += nSprSkipPerLine;
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopyBitmap16ToBuffer32b(WORD* pBitmap, int nBmpWidth, int nBmpHeight,
					 BYTE* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY, int nExAlpha)
{
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nBmpWidth, nBmpHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	pBitmap += nBmpWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT nSrcColor, nDstColor, R,G,B,A2;
	int nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
		
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		nWidth--;
		if(nExAlpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		nSrcColor = *(pBitmap++);
		R = (nSrcColor & 0xf800) >> 8;
		G = (nSrcColor & 0x07e0) >> 3;
		B = (nSrcColor & 0x001f) << 3;
		*((UINT*)pBuffer) = 0xff000000 | (R << 16) | (G << 8) | B;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			pBitmap += nSprSkipPerLine;
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		nSrcColor = *(pBitmap++);
		R = (nSrcColor & 0xf800) >> 8;
		G = (nSrcColor & 0x07e0) >> 3;
		B = (nSrcColor & 0x001f) << 3;
		nDstColor = *((UINT*)pBuffer);
		A2 = (nDstColor & 0xff000000) >> 24;
		if(A2 == 0)
		{
			nDstColor = (nExAlpha << 24) | (R << 16) | (G << 8) | B;
		}
		else
		{
			sR = (float)R/255.f;
			sG = (float)G/255.f;
			sB = (float)B/255.f;
			dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
			dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
			dB = (float)(nDstColor & 0xff)/255.f;
			sA = (float)nExAlpha/255.f;
			dA = (float)A2/255.f;
			dR = sR*sA + dR*dA*(1.f-sA);
			dG = sG*sA + dG*dA*(1.f-sA);
			dB = sB*sA + dB*dA*(1.f-sA);
			dA = sA + dA*(1.f-sA);
			dR /= dA;
			dG /= dA;
			dB /= dA;
			R = (UINT)(dR*255.f);
			G = (UINT)(dG*255.f);
			B = (UINT)(dB*255.f);
			A2 = (UINT)(dA*255.f);
			nDstColor = (A2 << 24) | (R << 16) | (G << 8) | B;
		}
		*((UINT*)pBuffer) = nDstColor;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			pBitmap += nSprSkipPerLine;
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopyShadowToBuffer(UINT nSrcColor, int nBmpWidth, int nBmpHeight,
					 BYTE* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY)
{
	int nExAlpha = (nSrcColor & 0xff000000)>>24;
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nBmpWidth, nBmpHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT nDstColor, R,G,B,A2,tR,tG,tB;
	int nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
	R = (nSrcColor & 0xff0000)>>16;
	G = (nSrcColor & 0xff00)>>8;
	B = (nSrcColor & 0xff);
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		nWidth--;
		if(nExAlpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		*((UINT*)pBuffer) = 0xff000000 | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		nDstColor = *((UINT*)pBuffer);
		A2 = (nDstColor & 0xff000000) >> 24;
		if(A2 == 0)
		{
			nDstColor = (nExAlpha << 24) | ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
		}
		else
		{
			sR = (float)R/255.f;
			sG = (float)G/255.f;
			sB = (float)B/255.f;
			dR = (float)((nDstColor & 0xf800) >> 8)/255.f;
			dG = (float)((nDstColor & 0x07e0) >> 3)/255.f;
			dB = (float)((nDstColor & 0x001f) << 3)/255.f;
			sA = (float)nExAlpha/255.f;
			dA = (float)A2/255.f;
			dR = sR*sA + dR*dA*(1.f-sA);
			dG = sG*sA + dG*dA*(1.f-sA);
			dB = sB*sA + dB*dA*(1.f-sA);
			dA = sA + dA*(1.f-sA);
			dR /= dA;
			dG /= dA;
			dB /= dA;
			tR = (UINT)(dR*255.f);
			tG = (UINT)(dG*255.f);
			tB = (UINT)(dB*255.f);
			A2 = (UINT)(dA*255.f);
			nDstColor = (A2 << 24) | ((tR >> 3) << 11) | ((tG >> 2) << 5) | (tB >> 3);
		}
		*((UINT*)pBuffer) = nDstColor;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopyShadowToBuffer32b(UINT nSrcColor, int nBmpWidth, int nBmpHeight,
					 BYTE* pBuffer, int nBufferWidth, int nBufferHeight,
					 int nX, int nY)
{
	int nExAlpha = (nSrcColor & 0xff000000)>>24;
	if(nExAlpha <= 0)
		return;
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nBmpWidth, nBmpHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	int nPitch = nBufferWidth * 4;
	pBuffer += Clipper.y * nPitch + Clipper.x * 4;
	int nBuffNextLine = nPitch - Clipper.width * 4;// next line add
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	UINT nDstColor, R,G,B,A2,tR,tG,tB;
	int nWidth;
	float sA,sR,sG,sB,dA,dR,dG,dB;
	R = (nSrcColor & 0xff0000)>>16;
	G = (nSrcColor & 0xff00)>>8;
	B = (nSrcColor & 0xff);
	_DrawFullLineSection_Line_:
		nWidth = Clipper.width;
	_DrawFullLineSection_LineLocal_:
		nWidth--;
		if(nExAlpha < 255)
			goto	_DrawFullLineSection_LineLocal_HalfAlpha_;
		*((UINT*)pBuffer) = nSrcColor;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	//vœ 1 Æi”m vÌi alpha < 255
	_DrawFullLineSection_LineLocal_HalfAlpha_:
		nDstColor = *((UINT*)pBuffer);
		A2 = (nDstColor & 0xff000000) >> 24;
		if(A2 == 0)
		{
			nDstColor = nSrcColor;
		}
		else
		{
			sR = (float)R/255.f;
			sG = (float)G/255.f;
			sB = (float)B/255.f;
			dR = (float)((nDstColor & 0xff0000) >> 16)/255.f;
			dG = (float)((nDstColor & 0xff00) >> 8)/255.f;
			dB = (float)(nDstColor & 0xff)/255.f;
			sA = (float)nExAlpha/255.f;
			dA = (float)A2/255.f;
			dR = sR*sA + dR*dA*(1.f-sA);
			dG = sG*sA + dG*dA*(1.f-sA);
			dB = sB*sA + dB*dA*(1.f-sA);
			dA = sA + dA*(1.f-sA);
			dR /= dA;
			dG /= dA;
			dB /= dA;
			tR = (UINT)(dR*255.f);
			tG = (UINT)(dG*255.f);
			tB = (UINT)(dB*255.f);
			A2 = (UINT)(dA*255.f);
			nDstColor = (A2 << 24) | (tR << 16) | (tG << 8) | tB;
		}
		*((UINT*)pBuffer) = nDstColor;
		pBuffer += 4;
		if(nWidth)
			goto	_DrawFullLineSection_LineLocal_;
		pBuffer += nBuffNextLine;
		Clipper.height--;
		if(Clipper.height)
		{
			goto	_DrawFullLineSection_Line_;
		}
		goto	_EXIT_WAY_;
	
	_EXIT_WAY_:
	return;
}

void RIO_CopySprToBuffer3LevelAlpha(void* pSpr, int nSprWidth, int nSprHeight, void* pPalette,
					 void* pBuffer, int nBufferWidth, int nBufferHeight, int nX, int nY)
{
	_ASSERT(pSpr && pBuffer && pPalette);
	// ∂‘ªÊ÷∆«¯”ÚΩ¯––≤√ºÙ
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	int	nPitch = nBufferWidth + nBufferWidth;
	// pBuffer÷∏œÚ∆¡ƒª∆µ„µƒ∆´“∆Œª÷√ (“‘◊÷Ω⁄º∆)
	pBuffer = (char*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
	long nBuffNextLine = nPitch - Clipper.width * 2;// next line add
	long nSprSkip = nSprWidth * Clipper.top + Clipper.left;
	long nSprSkipPerLine = Clipper.left + Clipper.right;

	__asm
	{
		// πedi÷∏œÚbufferªÊ÷∆∆µ„, πesi÷∏œÚÕºøÈ ˝æ›∆µ„,(Ã¯π˝nSprSkip∏ˆœÒµ„µƒÕº–Œ ˝æ›)
		mov		edi, pBuffer
		mov		esi, pSpr

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
						push	eax
						mov		ecx, eax

						cmp		ebx, 200
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							mov     ebx, pPalette
							_DrawFullLineSection_CopyPixel_:
							{
								copy_pixel_use_eax
								loop	_DrawFullLineSection_CopyPixel_
							}

							pop		eax
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_
	
							add		edi, nBuffNextLine
							dec		Clipper.height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
						}

						_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							push	edx							
							_DrawFullLineSection_HalfAlphaPixel_:
							{
								mix_2_pixel_color_use_eabdx
								loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							pop		edx
							pop		eax
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
					cmp		eax, edx
					jnl		_DrawPartLineSection_LineLocal_Alpha_Part_		//≤ªƒ‹»´ª≠’‚eax∏ˆœ‡Õ¨alpha÷µµƒœÒµ„£¨∫Û√Ê”–µ„“—æ≠≥¨≥ˆ«¯”Ú

					push	eax
					mov		ecx, eax
					cmp		ebx, 200
					jl		_DrawPartLineSection_LineLocal_HalfAlpha_
						
					//_DrawPartLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
						_DrawPartLineSection_CopyPixel_:
						{
							copy_pixel_use_eax
							loop	_DrawPartLineSection_CopyPixel_
						}						
						pop		eax
						sub		edx, eax
						jmp		_DrawPartLineSection_LineLocal_
					}
					
					_DrawPartLineSection_LineLocal_HalfAlpha_:
					{
						push	edx
						_DrawPartLineSection_HalfAlphaPixel_:
						{
							mix_2_pixel_color_use_eabdx
							loop	_DrawPartLineSection_HalfAlphaPixel_
						}
						pop		edx
						pop		eax
						sub		edx, eax
						jmp		_DrawPartLineSection_LineLocal_
					}
				}

				_DrawPartLineSection_LineLocal_Alpha_Part_:
				{
					push	eax
					mov		ecx, edx
					cmp		ebx, 200
					jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_
						
					//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
					{
						mov     ebx, pPalette
						_DrawPartLineSection_CopyPixel_Part_:
						{
							copy_pixel_use_eax
							loop	_DrawPartLineSection_CopyPixel_Part_
						}						
						pop		eax
				
						dec		Clipper.height
						jz		_EXIT_WAY_

						sub		eax, edx
						mov		edx, eax
						mov		ebx, 255	//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
						jmp		_DrawPartLineSection_LineSkip_
					}
					
					_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
					{
						push	edx
						_DrawPartLineSection_HalfAlphaPixel_Part_:
						{
							mix_2_pixel_color_use_eabdx
							loop	_DrawPartLineSection_HalfAlphaPixel_Part_
						}
						pop		edx
						pop		eax
						dec		Clipper.height
						jz		_EXIT_WAY_
						sub		eax, edx
						mov		edx, eax
						mov		ebx, 128
						jmp		_DrawPartLineSection_LineSkip_//»Áπ˚œÎ“™»∑«–µƒ‘≠ebx(alpha)÷µø…“‘‘⁄«∞Õ∑push ebx£¨¥À¥¶popªÒµ√
					}
				}
			}
		}
		_EXIT_WAY_:
		emms
	}
}