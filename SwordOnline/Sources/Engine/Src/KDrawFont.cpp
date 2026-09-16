//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KDrawFont.cpp
// Date:	2000.08.08
// Code:	Daniel Wang
// Desc:	Alpha Sprite Font Drawing Functions
//---------------------------------------------------------------------------
#include "KWin32.h"
#include "KCanvas.h"
#include "KDrawFont.h"
#include "DrawSpriteMP.h"
//---------------------------------------------------------------------------
// º¯Êý:	DrawFont
// ¹¦ÄÜ:	»æÖÆ´ø8¼¶AlphaµÄ×ÖÌå
// ²ÎÊý:	KDrawNode*, KCanvas* 
// ·µ»Ø:	void
//---------------------------------------------------------------------------
void g_DrawFont(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	long nX = pNode->m_nX;// x coord
	long nY = pNode->m_nY;// y coord
	long nWidth = pNode->m_nWidth;// width of font
	long nHeight = pNode->m_nHeight;// height of font
	long nColor = pNode->m_nColor;// color of font
	long nAlpha = pNode->m_nAlpha&0x001f;// nAlphaÖµ
	long nTmpAlpha = 0;// nAlphaÖµ2
	void* lpFont = pNode->m_pBitmap;// font pointer

	// ¶Ô»æÖÆÇøÓò½øÐÐ²Ã¼ô
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	long nMask32 = pCanvas->m_nMask32;// RGB mask 32bit
	
	// ¼ÆËãÆÁÄ»ÏÂÒ»ÐÐµÄÆ«ÒÆ
	long nNextLine = nPitch - Clipper.width * 2;
	
	__asm
	{
//---------------------------------------------------------------------------
// ¼ÆËã EDI Ö¸ÏòÆÁÄ»ÆðµãµÄÆ«ÒÆÁ¿ (ÒÔ×Ö½Ú¼Æ)
// edi = nPitch * Clipper.y + nX * 2 + lpBuffer
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, Clipper.x
		add		ebx, ebx
		add     eax, ebx
		mov 	edi, lpBuffer
		add		edi, eax
//---------------------------------------------------------------------------
// ³õÊ¼»¯ ESI Ö¸ÏòÍ¼¿éÊý¾ÝÆðµã (Ìø¹ý Clipper.top ÐÐÍ¼ÐÎÊý¾Ý)
//---------------------------------------------------------------------------
		mov		esi, lpFont
		mov		ecx, Clipper.top
		or		ecx, ecx
		jz		loc_DrawFont_0011
		xor		eax, eax

loc_DrawFont_0008:

		mov		edx, nWidth

loc_DrawFont_0009:

		mov     al, [esi]
		inc     esi
		and		al, 0x1f
		sub		edx, eax
		jg		loc_DrawFont_0009
		dec     ecx
		jg  	loc_DrawFont_0008
//---------------------------------------------------------------------------
// jump acorrd Clipper.left, Clipper.right
//---------------------------------------------------------------------------
loc_DrawFont_0011:

		mov		eax, Clipper.left
		or		eax, eax
		jz		loc_DrawFont_0012
		jmp		loc_DrawFont_exit

loc_DrawFont_0012:

		mov		eax, Clipper.right
		or		eax, eax
		jz		loc_DrawFont_0100
		jmp		loc_DrawFont_exit
//---------------------------------------------------------------------------
// Clipper.left  == 0
// Clipper.right == 0
//---------------------------------------------------------------------------
loc_DrawFont_0100:

		mov		edx, Clipper.width

loc_DrawFont_0101:

		xor		eax, eax
		mov     al, [esi]
		inc     esi
		mov		ebx, eax
		shr		ebx, 5
		or		ebx, ebx
		jnz		loc_DrawFont_0102

		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawFont_0101
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawFont_0100
		jmp		loc_DrawFont_exit

loc_DrawFont_0102:

        shl     ebx, 2    //Alpha1 
		
		push    eax
		push    edx 
		mov     eax,nAlpha
		mul     ebx
        shr     eax,5 
        mov		nTmpAlpha, eax
		pop     edx  
		pop     eax 
   
		and		eax, 0x1f
		mov     ecx, eax
		push	eax
		push    edx

loc_DrawFont_Loop1:

		push	ecx
		mov     ecx, nColor				// ecx = ...rgb
		mov		ax, cx					// eax = ...rgb
		sal		eax, 16					// eax = rgb...
		mov		ax, cx					// eax = rgbrgb
		and		eax, nMask32			// eax = .g.r.b

		mov		cx, [edi]				// ecx = ...rgb
		mov		bx, cx					// ebx = ...rgb
		sal		ebx, 16					// ebx = rgb...
		mov		bx, cx					// ebx = rgbrgb
		and		ebx, nMask32			// ebx = .g.r.b

		mov		ecx, nTmpAlpha 			// ecx = nAlpha
		mul		ecx						// eax = eax*ecx
		neg		ecx						// ecx = -nAlpha
		add		ecx, 0x20				// ecx = 32-nAlpha
		xchg	eax, ebx				// exchange eax,ebx
		mul		ecx						// eax = eax*ecx
		add		eax, ebx				// eax = eax + ebx
		sar		eax, 5					// eax = eax/32
		and     eax, nMask32			// eax = .g.r.b

		mov     cx, ax					// ecx = ...r.b
		sar     eax, 16					// eax = ....g.
		or      ax, cx					// eax = ...rgb
        stosw                       

		pop		ecx
        loop    loc_DrawFont_Loop1

        pop     edx
        pop     eax

		sub		edx, eax
		jg		loc_DrawFont_0101
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawFont_0100
		jmp		loc_DrawFont_exit

loc_DrawFont_exit:
	}
	pCanvas->UnlockCanvas();
}

//---------------------------------------------------------------------------
// º¯Êý:	DrawFont
// ¹¦ÄÜ:	»æÖÆ´ø8¼¶AlphaµÄ×ÖÌå
// ²ÎÊý:	KDrawNode*, KCanvas* 
// ·µ»Ø:	void
//---------------------------------------------------------------------------
void g_DrawFontWithBorder(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of font
	int nHeight = pNode->m_nHeight;// height of font
	int nColor = pNode->m_nColor;// color of font
	int nBorderColor = pNode->m_nAlpha;// border color
	void* lpFont = pNode->m_pBitmap;// font pointer

	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;
	lpBuffer = (char*)(lpBuffer) + Clipper.y * nPitch + Clipper.x*2;
	int nMask32 = pCanvas->m_nMask32;// RGB mask 32bit
	int nNextLine = nPitch - Clipper.width * 2;
	int nSprSkip = nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	int nAlpha = (nColor & 0xff000000) >> 24;
	nAlpha >>= 3;
	UINT nCheckColor = nColor & 0xf800;
	if(nCheckColor == 0xf800)
		nColor &= 0xf7ff;
	nColor &= 0xffff;
	int nBAlpha = (nBorderColor & 0xff000000) >> 24;
	nBAlpha >>= 3;
	nCheckColor = nBorderColor & 0xf800;
	if(nCheckColor == 0xf800)
		nBorderColor &= 0xf7ff;
	nBorderColor &= 0xffff;
	__asm
	{
//---------------------------------------------------------------------------
// §i ®Õn ®Çu ®iÓm vÏ toµn côc edi
//---------------------------------------------------------------------------
		mov 	edi, lpBuffer
//---------------------------------------------------------------------------
// ³§i ®Õn ®iÓm vÏ trªn texture
//---------------------------------------------------------------------------
		mov		esi, lpFont
		mov		edx, nSprSkip
		or		edx, edx
		jz		_SkipSpriteAheadContentEnd_
		_SkipSpriteAheadContentLocalStart_:
		{
			movzx   eax, byte ptr[esi]
			mov		ebx, eax
			shr		ebx, 5
			inc     esi
			and		eax, 0x1f
			sub		edx, eax
			jg		_SkipSpriteAheadContentLocalStart_
			neg		edx
		}
_SkipSpriteAheadContentEnd_:
		or		edx, edx
		jz		loc_DrawFont_0100
		mov		eax, edx
		mov		edx, Clipper.width
		jmp		_CheckRemainPixel
loc_DrawFont_0100:
		mov		edx, Clipper.width
loc_DrawFont_0101:
		xor		eax, eax
		mov     al, [esi]
		inc     esi
		mov		ebx, eax
		shr		ebx, 5
		and		eax, 0x1f
_CheckRemainPixel:
		or		ebx, ebx
		jnz		loc_DrawFont_0102
		lea     edi, [edi + eax * 2]
		sub		edx, eax
		jg		loc_DrawFont_0101
		lea     edi, [edi + edx * 2]
		neg		edx
		dec		Clipper.height
		jz		loc_DrawFont_exit
_DrawPartLineSection_LineSkip_:
		add		edi, nNextLine
		mov		eax, edx
		mov		edx, nSprSkipPerLine
		or		edx, edx
		jz		loc_DrawFont_0100
		or		eax, eax
		jnz		_DrawPartLineSection_SkipRemainPixel
_DrawPartLineSection_LineSkipLocal_:
		movzx   eax, byte ptr[esi]
		mov		ebx, eax
		shr		ebx, 5
		inc     esi
		and		eax, 0x1f
_DrawPartLineSection_SkipRemainPixel:
		sub		edx, eax
		jg		_DrawPartLineSection_LineSkipLocal_
		neg		edx
		jmp		_SkipSpriteAheadContentEnd_
loc_DrawFont_0102:
		sub		edx, eax
		jl		_DrawFont_RemainPixel
		cmp		ebx, 7
		jl		DrawFrontWithBorder_DrawBorder
		//mµu chÝnh cña font
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nAlpha
		cmp		eax, 31
		jge		_DrawFont_MaxAlpha
		movd	mm2, nMask32
		movd	mm3, nAlpha
		movd	mm4, nColor
_DrawFont_Color:
		mix_2_pixel_color_alpha_font
		dec		ecx
		jnz		_DrawFont_Color
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_MaxAlpha:
		mov		eax, nColor
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		_DrawFont_MaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		//mµu viÒn cña font
DrawFrontWithBorder_DrawBorder:
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nBAlpha
		cmp		eax, 31
		jge		_DrawFont_BorderMaxAlpha
		movd	mm2, nMask32
		movd	mm3, nBAlpha
		movd	mm4, nBorderColor
_DrawFont_Border:
		mix_2_pixel_color_alpha_font
		dec		ecx
		jnz		_DrawFont_Border
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_BorderMaxAlpha:
		mov		eax, nBorderColor
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		_DrawFont_BorderMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		
_DrawFont_RemainPixel:
		add		eax, edx
		neg		edx
		cmp		ebx, 7
		jl		_DrawFont_RemainPixel_DrawBorder
		//mµu chÝnh cña font
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nAlpha
		cmp		eax, 31
		jge		_DrawFont_RemainMaxAlpha
		movd	mm2, nMask32
		movd	mm3, nAlpha
		movd	mm4, nColor
_DrawFont_RemainColor:
		mix_2_pixel_color_alpha_font
		dec		ecx
		jnz		_DrawFont_RemainColor
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_RemainMaxAlpha:
		mov		eax, nColor
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		_DrawFont_RemainMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		//mµu viÒn cña font
_DrawFont_RemainPixel_DrawBorder:
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nBAlpha
		cmp		eax, 31
		jge		_DrawFont_RemainBorderMaxAlpha
		movd	mm2, nMask32
		movd	mm3, nBAlpha
		movd	mm4, nBorderColor
_DrawFont_RemainBorder:
		mix_2_pixel_color_alpha_font
		dec		ecx
		jnz		_DrawFont_RemainBorder
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_RemainBorderMaxAlpha:
		mov		eax, nBorderColor
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		_DrawFont_RemainBorderMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
loc_DrawFont_exit:
		emms
	}
	pCanvas->UnlockCanvas();
}

void g_DrawFontWithBorder32b(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	int nX = pNode->m_nX;// x coord
	int nY = pNode->m_nY;// y coord
	int nWidth = pNode->m_nWidth;// width of font
	int nHeight = pNode->m_nHeight;// height of font
	int nColor = pNode->m_nColor;// color of font
	int nBorderColor = pNode->m_nAlpha;// ±ßÔµµÄÑÕÉ«Öµ
	void* lpFont = pNode->m_pBitmap;// font pointer

	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;
	lpBuffer = (char*)(lpBuffer) + Clipper.y * nPitch + Clipper.x*4;
	int nNextLine = nPitch - Clipper.width * 4;
	int nSprSkip = nWidth * Clipper.top + Clipper.left;
	int nSprSkipPerLine = Clipper.left + Clipper.right;
	int nAlpha = (nColor & 0xff000000) >> 24;
	nColor &= 0xffffff;
	int nBAlpha = (nBorderColor & 0xff000000) >> 24;
	nBorderColor &= 0xffffff;
	int R,G,B,bR,bG,bB;
	R = (nColor & 0xff0000) >> 16;
	G = (nColor & 0xff00) >> 8;
	B = (nColor & 0xff);
	bR = (nBorderColor & 0xff0000) >> 16;
	bG = (nBorderColor & 0xff00) >> 8;
	bB = (nBorderColor & 0xff);
	__asm
	{
//---------------------------------------------------------------------------
// §i ®Õn ®Çu ®iÓm vÏ toµn côc edi
//---------------------------------------------------------------------------
		mov 	edi, lpBuffer
//---------------------------------------------------------------------------
// ³§i ®Õn ®iÓm vÏ trªn texture
//---------------------------------------------------------------------------
		mov		esi, lpFont
		mov		edx, nSprSkip
		or		edx, edx
		jz		_SkipSpriteAheadContentEnd_
		_SkipSpriteAheadContentLocalStart_:
		{
			movzx   eax, byte ptr[esi]
			mov		ebx, eax
			shr		ebx, 5
			inc     esi
			and		eax, 0x1f
			sub		edx, eax
			jg		_SkipSpriteAheadContentLocalStart_
			neg		edx
		}
_SkipSpriteAheadContentEnd_:
		or		edx, edx
		jz		loc_DrawFont_0100
		mov		eax, edx
		mov		edx, Clipper.width
		jmp		_CheckRemainPixel
loc_DrawFont_0100:
		mov		edx, Clipper.width
loc_DrawFont_0101:
		xor		eax, eax
		mov     al, [esi]
		inc     esi
		mov		ebx, eax
		shr		ebx, 5
		and		eax, 0x1f
_CheckRemainPixel:
		or		ebx, ebx
		jnz		loc_DrawFont_0102
		lea     edi, [edi + eax * 4]
		sub		edx, eax
		jg		loc_DrawFont_0101
		lea     edi, [edi + edx * 4]
		neg		edx
		dec		Clipper.height
		jz		loc_DrawFont_exit
_DrawPartLineSection_LineSkip_:
		add		edi, nNextLine
		mov		eax, edx
		mov		edx, nSprSkipPerLine
		or		edx, edx
		jz		loc_DrawFont_0100
		or		eax, eax
		jnz		_DrawPartLineSection_SkipRemainPixel
_DrawPartLineSection_LineSkipLocal_:
		movzx   eax, byte ptr[esi]
		mov		ebx, eax
		shr		ebx, 5
		inc     esi
		and		eax, 0x1f
_DrawPartLineSection_SkipRemainPixel:
		sub		edx, eax
		jg		_DrawPartLineSection_LineSkipLocal_
		neg		edx
		jmp		_SkipSpriteAheadContentEnd_
loc_DrawFont_0102:
		sub		edx, eax
		jl		_DrawFont_RemainPixel
		cmp		ebx, 7
		jl		DrawFrontWithBorder_DrawBorder
		//mµu chÝnh cña font
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nAlpha
		cmp		eax, 254
		jge		_DrawFont_MaxAlpha
		movd	mm2, nAlpha
		movd	mm3, R
		movd	mm4, G
		movd	mm5, B
_DrawFont_Color:
		mix_2_pixel_color_alpha_font_32
		dec		ecx
		jnz		_DrawFont_Color
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_MaxAlpha:
		mov		eax, nColor
		or		eax, 0xff000000
		mov		[edi], eax
		add		edi, 4
		dec		ecx
		jnz		_DrawFont_MaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		//mµu viÒn cña font
DrawFrontWithBorder_DrawBorder:
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nBAlpha
		cmp		eax, 254
		jge		_DrawFont_BorderMaxAlpha
		movd	mm2, nBAlpha
		movd	mm3, bR
		movd	mm4, bG
		movd	mm5, bB
_DrawFont_Border:
		mix_2_pixel_color_alpha_font_32
		dec		ecx
		jnz		_DrawFont_Border
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_BorderMaxAlpha:
		mov		eax, nBorderColor
		or		eax, 0xff000000
		mov		[edi], eax
		add		edi, 4
		dec		ecx
		jnz		_DrawFont_BorderMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		or		edx, edx
		jg		loc_DrawFont_0101
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		
_DrawFont_RemainPixel:
		add		eax, edx
		neg		edx
		cmp		ebx, 7
		jl		_DrawFont_RemainPixel_DrawBorder
		//mµu chÝnh cña font
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nAlpha
		cmp		eax, 254
		jge		_DrawFont_RemainMaxAlpha
		movd	mm2, nAlpha
		movd	mm3, R
		movd	mm4, G
		movd	mm5, B
_DrawFont_RemainColor:
		mix_2_pixel_color_alpha_font_32
		dec		ecx
		jnz		_DrawFont_RemainColor
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_RemainMaxAlpha:
		mov		eax, nColor
		or		eax, 0xff000000
		mov		[edi], eax
		add		edi, 4
		dec		ecx
		jnz		_DrawFont_RemainMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
		//mµu viÒn cña font
_DrawFont_RemainPixel_DrawBorder:
		mov		ecx, eax
		movd	mm0, ebx
		movd	mm1, edx
		mov		eax, nBAlpha
		cmp		eax, 254
		jge		_DrawFont_RemainBorderMaxAlpha
		movd	mm2, nBAlpha
		movd	mm3, bR
		movd	mm4, bG
		movd	mm5, bB
_DrawFont_RemainBorder:
		mix_2_pixel_color_alpha_font_32
		dec		ecx
		jnz		_DrawFont_RemainBorder
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
_DrawFont_RemainBorderMaxAlpha:
		mov		eax, nBorderColor
		or		eax, 0xff000000
		mov		[edi], eax
		add		edi, 4
		dec		ecx
		jnz		_DrawFont_RemainBorderMaxAlpha
		movd	ebx, mm0
		movd	edx, mm1
		dec		Clipper.height
		jz		loc_DrawFont_exit
		jmp		_DrawPartLineSection_LineSkip_
loc_DrawFont_exit:
		emms
	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
// º¯Êý:	DrawFontSolid
// ¹¦ÄÜ:	»æÖÆÊµÐÄ×ÖÌå
// ²ÎÊý:	KDrawNode*, KCanvas* 
// ·µ»Ø:	void
//---------------------------------------------------------------------------
void g_DrawFontSolid(void* node, void* canvas)
{
	KDrawNode* pNode = (KDrawNode *)node;
	KCanvas* pCanvas = (KCanvas *)canvas;
	
	long nX = pNode->m_nX;// x coord
	long nY = pNode->m_nY;// y coord
	long nWidth = pNode->m_nWidth;// width of font
	long nHeight = pNode->m_nHeight;// height of font
	long nColor = pNode->m_nColor;// color of font
	long nAlpha = pNode->m_nAlpha&0x001f;// nAlphaÖµ
	void* lpFont = pNode->m_pBitmap;// font pointer

	// ¶Ô»æÖÆÇøÓò½øÐÐ²Ã¼ô
	KClipper Clipper;
	if (!pCanvas->MakeClip(nX, nY, nWidth, nHeight, &Clipper))
		return;

	int nPitch;
	void* lpBuffer = pCanvas->LockCanvas(nPitch);
	if (lpBuffer == NULL)
		return;

	long nMask32 = pCanvas->m_nMask32;// RGB mask 32bit

	// ¼ÆËãÆÁÄ»ÏÂÒ»ÐÐµÄÆ«ÒÆ
	long nNextLine = nPitch - Clipper.width * 2;
	
	__asm
	{
//---------------------------------------------------------------------------
// ¼ÆËã EDI Ö¸ÏòÆÁÄ»ÆðµãµÄÆ«ÒÆÁ¿ (ÒÔ×Ö½Ú¼Æ)
// edi = nPitch * Clipper.y + nX * 2 + lpBuffer
//---------------------------------------------------------------------------
		mov		eax, nPitch
		mov		ebx, Clipper.y
		mul		ebx
		mov     ebx, Clipper.x
		add		ebx, ebx
		add     eax, ebx
		mov 	edi, lpBuffer
		add		edi, eax
//---------------------------------------------------------------------------
// ³õÊ¼»¯ ESI Ö¸ÏòÍ¼¿éÊý¾ÝÆðµã (Ìø¹ý Clipper.top ÐÐÍ¼ÐÎÊý¾Ý)
//---------------------------------------------------------------------------
		mov		esi, lpFont
		mov		ecx, Clipper.top
		or		ecx, ecx
		jz		loc_DrawFontSolid_0011
		xor		eax, eax

loc_DrawFontSolid_0008:

		mov		edx, nWidth

loc_DrawFontSolid_0009:

		mov     al, [esi]
		inc     esi
		and		al, 0x1f
		sub		edx, eax
		jg		loc_DrawFontSolid_0009
		dec     ecx
		jg  	loc_DrawFontSolid_0008
//---------------------------------------------------------------------------
// jump acorrd Clipper.left, Clipper.right
//---------------------------------------------------------------------------
loc_DrawFontSolid_0011:

		mov		eax, Clipper.left
		or		eax, eax
		jz		loc_DrawFontSolid_0012
		jmp		loc_DrawFontSolid_exit

loc_DrawFontSolid_0012:

		mov		eax, Clipper.right
		or		eax, eax
		jz		loc_DrawFontSolid_0100
		jmp		loc_DrawFontSolid_exit
//---------------------------------------------------------------------------
// Clipper.left  == 0
// Clipper.right == 0
//---------------------------------------------------------------------------
loc_DrawFontSolid_0100:

		mov		edx, Clipper.width

loc_DrawFontSolid_0101:

		xor		eax, eax
		mov     al, [esi]
		inc     esi
		mov		ebx, eax
		shr		ebx, 5
		or		ebx, ebx
		jnz		loc_DrawFontSolid_0102

		add		edi, eax
		add		edi, eax
		sub		edx, eax
		jg		loc_DrawFontSolid_0101
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawFontSolid_0100
		jmp		loc_DrawFontSolid_exit

loc_DrawFontSolid_0102:

        and		eax, 0x1f
		mov     ecx, eax
		push	eax
		push    edx

loc_DrawFontSolid_Loop1:

		push	ecx
		mov     ecx, nColor				// ecx = ...rgb
		mov		ax, cx					// eax = ...rgb
		sal		eax, 16					// eax = rgb...
		mov		ax, cx					// eax = rgbrgb
		and		eax, nMask32			// eax = .g.r.b

		mov		cx, [edi]				// ecx = ...rgb
		mov		bx, cx					// ebx = ...rgb
		sal		ebx, 16					// ebx = rgb...
		mov		bx, cx					// ebx = rgbrgb
		and		ebx, nMask32			// ebx = .g.r.b

		mov		ecx, nAlpha 			// ecx = nAlpha
		mul		ecx						// eax = eax*ecx
		neg		ecx						// ecx = -nAlpha
		add		ecx, 0x20				// ecx = 32-nAlpha
		xchg	eax, ebx				// exchange eax,ebx
		mul		ecx						// eax = eax*ecx
		add		eax, ebx				// eax = eax + ebx
		sar		eax, 5					// eax = eax/32
		and     eax, nMask32			// eax = .g.r.b

		mov     cx, ax					// ecx = ...r.b
		sar     eax, 16					// eax = ....g.
		or      ax, cx					// eax = ...rgb
        stosw                       

		pop		ecx
        loop    loc_DrawFontSolid_Loop1

        pop     edx
        pop     eax

		sub		edx, eax
		jg		loc_DrawFontSolid_0101
		add		edi, nNextLine
		dec		Clipper.height
		jg		loc_DrawFontSolid_0100
		jmp		loc_DrawFontSolid_exit

loc_DrawFontSolid_exit:
	}
	pCanvas->UnlockCanvas();
}
//---------------------------------------------------------------------------
