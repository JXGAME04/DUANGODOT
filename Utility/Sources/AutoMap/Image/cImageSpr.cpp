// cImageSpr.cpp: implementation of the cImageSpr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "cImageSpr.h"

//#include "KEngine.h"
#include "KOctree.h"
//#include "KTgaFile32.h"
//#include "KBitmap.h"
//#include "KMemManager.h"
//#include "KGraphics.h"
#include "KSpriteMaker.h"
#include "ctimer.h"

#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
inline void GetColorR5G6B5(WORD cr, BYTE& r, BYTE& g, BYTE& b)
{
	r = (cr >> 11) << 3;
	g = (((WORD)(cr << 5)) >> 10) << 2;
	b = (((WORD)(cr << 11)) >> 11) << 3;
}

inline void SetColorR5G6B5(WORD& cr, BYTE r, BYTE g, BYTE b)
{
	cr = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
}

inline void GetColorA4R4G4B4Format(WORD cr,BYTE& a, BYTE& r, BYTE& g, BYTE& b)
{
	a = ((cr >> 12) << 4);
	r = ((((WORD) (cr << 4)) >> 12) << 4) ;
	g = ((((WORD) (cr << 8)) >> 12) << 4) ;
	b = ((((WORD) (cr << 12)) >> 12) << 4) ;
}

inline void SetColorA4R4G4B4Format(WORD& cr,BYTE a, BYTE r, BYTE g, BYTE b)
{
	cr =((WORD)(a >> 4)) << 12 | 
		((WORD)(r >> 4)) << 8 | (g >> 4) << 4 | (b >> 4) ; 
}

inline void GetColorA4R4G4B4(WORD cr,BYTE& a, BYTE& r, BYTE& g, BYTE& b)
{
	a = ((cr >> 12));
	r = ((((WORD) (cr << 4)) >> 12)) ;
	g = ((((WORD) (cr << 8)) >> 12)) ;
	b = ((((WORD) (cr << 12)) >> 12)) ;
}

inline void GetColorR4G4B4(WORD cr, BYTE& r, BYTE& g, BYTE& b)
{
	r = ((((WORD) (cr << 4)) >> 12)) ;
	g = ((((WORD) (cr << 8)) >> 12)) ;
	b = ((((WORD) (cr << 12)) >> 12)) ;
}

inline void SetColorA4R4G4B4(WORD& cr,BYTE a, BYTE r, BYTE g, BYTE b)
{
	cr =((WORD)(a)) << 12 | 
		((WORD)(r)) << 8 | (g) << 4 | (b) ; 
}


cImageSpr::cImageSpr()
{
	m_nWidth		= 0;
	m_nHeight		= 0;
	m_nCenterX		= 0;
	m_nCenterY		= 0;
	m_nFrames		= 0;
	m_nColors		= 0;
	m_nDirections	= 1;
	m_nInterval		= 1;
	m_pPal24		= NULL;
	m_pPal16		= NULL;
	m_pOffset		= NULL;
	m_pSprite		= NULL;
	m_Buffer		= NULL;
	m_Palette		= NULL;

	m_size = 0;
	m_nFrameInfo = 0;
}

cImageSpr::~cImageSpr()
{
	if (m_Buffer)
		delete m_Buffer;
	if (m_Palette)
		delete m_Palette;

	for (int i=0; i<m_nFrameInfo; i++)
	{
		if (m_aFrameInfo[i].buffer)
			delete m_aFrameInfo[i].buffer;
	}
}	

//////////////////////////////////////////////////////////////////////
//info of a image
ULONG cImageSpr::GetSize() 
{
	return m_size;
}

HRESULT	cImageSpr::GetCenterToUpleft(int nFrame,int& x,int& y)
{
	int tx = GetCenterX();
	int ty = GetCenterY();
	if (tx == 0 && ty == 0)
	{
		tx = 160;
		ty = 192;
	}
	int xoff,yoff;
	GetOffset(nFrame,xoff,yoff);
	x = tx - xoff;
	y = ty - yoff;
	return 0;
}

HRESULT	cImageSpr::SetCenterToUpleft(int nFrame,int x,int y)
{
	int tx = GetCenterX();
	int ty = GetCenterY();
	if (tx == 0 && ty == 0)
	{
		tx = 160;
		ty = 192;
	}
	int xoff,yoff;
	xoff = tx - x;
	yoff = ty - y;
	SetOffset(nFrame,xoff,yoff);
	return 0;
}

HRESULT	cImageSpr::SetOffset(int nFrame,int x,int y) 
{
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(nFrame));	
	pFrame->OffsetX = x ;
	pFrame->OffsetY = y ; 
	return S_OK;
}

HRESULT cImageSpr::GetOffset(ULONG nFrame, int& x, int &y)
{
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(nFrame));	
	x = pFrame->OffsetX;
	y = pFrame->OffsetY;
	return S_OK;
}

void cImageSpr::GetSize(ULONG nFrame, ULONG& width, ULONG& height)
{
	if (nFrame < 0 || nFrame >= GetFrames())
		return ;
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(nFrame));	
	height = pFrame->Height;
	width = pFrame->Width;
}

HRESULT	cImageSpr::GetAlphaBottom(int nFrame, int&tx, int&ty)
{
	stFrameInfo info;
	GetFrameInfo(info,nFrame);
	tx = 0; 
	ty = 0;
	for (int y = info.height-1; y>0; y--)
	for (unsigned int x = 0; x<info.width; x++)
	{
		int alpha = GetPixelAlpha(nFrame,x,y);
		if (alpha != 0)
		{
			tx = x;
			ty = y;
			return 0;
		}
	}
	return 0;
}

void cImageSpr::GetFrameInfo(stFrameInfo& info,ULONG nFrame) 
{
//	ASSERT(nFrame < m_nFrames);
	if (nFrame >= m_nFrames)
		nFrame = 0;
	GetSize(nFrame,info.width,info.height);
	GetOffset(nFrame,info.x,info.y);
	GetCenterToUpleft(nFrame,info.xCenterToUpleft,info.yCenterToUpleft);
}

void cImageSpr::GetImageInfo(stImageInfo& info)
{
	info.format = IMAGE_256;
	info.bytes_per_pixel = 1;
	info.buffer = m_Buffer;
	info.nDirections = GetDirections();
	info.nFrames = GetFrames();
	info.size = GetSize();
}

//////////////////////////////////////////////////////////////////////
//render
//////////////////////////////////////////////////////////////////////

HRESULT cImageSpr::Render(stImageRender& render)
{
	//if (render.format == IMAGE_565 && render.eOp == Render_Copy)

	if (render.eOp == Render_Copy) //不考虑源图以及裁减
	{
		if (render.format == IMAGE_565 )
		{
			MakePalette();
			RenderToR5G6B5(render);		
			return S_OK;
		}
		//if (render.format == IMAGE_4444 && render.eOp == Render_Copy)
		if (render.format == IMAGE_4444 )
		{
			Make4444Palette();
			RenderToA4R4G4B4(render);		
			return S_OK;
		}
/*
		if (render.format == IMAGE_8888)
		{
			Make8888Palette();
			RenderToA8R8G8B8(render);		
			return S_OK;		
		}
*/
	}
	else if (render.eOp == Render_Alpha) //考虑源图以及裁减
	{
		//des format
		if (render.format == IMAGE_565 )
		{
			MakePalette();
			return RenderToR5G6B5Alpha(render);		
		}
		if (render.format == IMAGE_4444)
		{
			Make4444Palette();
			return RenderToA4R4G4B4Alpha2(render);		
//			return RenderToA4R4G4B4Alpha(render);		
		}
	}
	return -1;
}

CBitmap* cImageSpr::ImageToBitmap(ULONG nFrame,int nDepth)
{
	if (nFrame < 0 || nFrame >= GetFrames())
		return NULL;

	MakePalette();

	ULONG width,height;
	GetSize(nFrame,width,height);

	WORD* pDest = new WORD[width*height];
	ZeroMemory(pDest,sizeof(WORD)*width*height);
	stImageRender render;
	render.ptDes.x = 0;
	render.ptDes.y = 0;
	render.buffer = pDest;
//	render.width = width;
//	render.height = height;
	render.nPitch = width * 2;
	render.nFrame = nFrame;
	RenderToR5G6B5(render);
	
	CBitmap* pBmp = new CBitmap;
	if (nDepth == 16)
	{
		VERIFY(pBmp->CreateBitmap(width,height,1,16,render.buffer));
	}
	else if (nDepth == 32)
	{
		DWORD* buffer32 = new DWORD[width*height];
		char* p32 = (char*)buffer32;
		WORD* p16 = pDest;
		BYTE a,r,g,b;
		for (int i=0; i<width*height; i++)
		{
			GetColorR5G6B5(*p16++,r,g,b);
			*p32++ = b;
			*p32++ = g;
			*p32++ = r;
			*p32++ = a;
		}
		VERIFY(pBmp->CreateBitmap(width,height,1,32,buffer32));
		delete buffer32;
	}
	else
	{
		delete pBmp;
		pBmp = NULL;
	}
	delete pDest;
	return pBmp;
}

HRESULT cImageSpr::RenderToA4R4G4B4(stImageRender& render)
{
	if (render.nFrame < 0 || render.nFrame >= GetFrames())
		return FALSE;
	// goto frame
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(render.nFrame));	
	int height = pFrame->Height;
	int width = pFrame->Width;

	long nNextLine = render.nPitch - width * 2;// next line add
	PBYTE pSrc	= pFrame->Sprite;
	PBYTE pPalette = (PBYTE)GetPalette();	
	WORD* pDest = (WORD*)render.buffer;
	pDest += ( render.ptDes.y * render.nPitch / 2 + render.ptDes.x ) ;

	__asm
	{
		
		//设置源与目的地址至esi和edi
		mov		edi, pDest
		mov		esi, pSrc
			
loc_DrawSprite_0100:
		
		//edx存放宽度	
		mov		edx, width
			
loc_DrawSprite_0101:
		
		//压缩格式为 n个alpha [rgba,rgba,.......],alpha为0时，无rgba各值
		
		movzx	eax, byte ptr[esi]//ax存放n
		inc		esi
		movzx	ebx, byte ptr[esi]//bx存放alpha值
		//mov		nAlpha, ebx
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSprite_0102
		
		push	eax	
		mov		ecx, eax
			
loc_FillZeroAlpha:
		
		//romandou
		
		mov		[edi], 0
		inc		edi
		inc		edi
		dec		ecx
		jnz     loc_FillZeroAlpha
		pop     eax
		
		sub		edx, eax
		jg		loc_DrawSprite_0101
		add		edi, nNextLine
		dec		height
		jnz		loc_DrawSprite_0100
		jmp		loc_DrawSprite_exit
		
			
			//alpha值不为0时
loc_DrawSprite_0102:
		
		push	eax // n值
		push	edx // 当前行还剩多长未处理 

		and		bx, 0x00f0
		shl		bx, 8
		push	ebx	// nAlpha值
		
		mov		ecx, eax
		
			
loc_DrawSprite_0103:
		
		mov     ebx, pPalette
		movzx	eax, byte ptr[esi]
		inc		esi
		mov		dx, [ebx + eax * 2]  //获得 16位 r4g4b4a4 a = 0
		
		pop     ebx   //ebx = nAlpha
		push	ebx  
		
		or		dx, bx //生成最终的r4g4b4a4值
		mov		[edi], dx
		inc		edi
		inc		edi
		dec		ecx
		jnz		loc_DrawSprite_0103
		pop     ebx   //ebx = nAlpha
		pop		edx   //edx = 该行剩于
		pop		eax   //eax = n
		sub		edx, eax
		jg		loc_DrawSprite_0101
		add		edi, nNextLine
		dec		height
		jnz		loc_DrawSprite_0100
		jmp		loc_DrawSprite_exit
			
loc_DrawSprite_exit:
		
		
	}
	return S_OK;		
}

HRESULT cImageSpr::RenderToR5G6B5(stImageRender& render)
{
	if (render.nFrame < 0 || render.nFrame >= GetFrames())
		return FALSE;
	// goto frame
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(render.nFrame));	
	int height = pFrame->Height;
	int width = pFrame->Width;

	long nNextLine = render.nPitch - width * 2;// next line add
	PBYTE pSrc	= pFrame->Sprite;
	PBYTE pPalette = (PBYTE)GetPalette();	
	WORD* pDest = (WORD*)render.buffer;
	pDest += ( render.ptDes.y * render.nPitch / 2 + render.ptDes.x ) ;

	//pDest = (WORD*)((char*)pDest + nPitch * (dwTexHeight - height) ) ;

	__asm
	{
		
		//设置源与目的地址至esi和edi
		mov		edi, pDest
		mov		esi, pSrc
		
loc_DrawSprite_0100:
		
		//edx存放宽度	
		mov		edx, width
			
loc_DrawSprite_0101:
		
		//压缩格式为 n个alpha [rgba,rgba,.......],alpha为0时，无rgba各值
		
		movzx	eax, byte ptr[esi]//ax存放n
		inc		esi
		movzx	ebx, byte ptr[esi]//bx存放alpha值
		//mov		nAlpha, ebx
		inc		esi
		or		ebx, ebx
		jnz		loc_DrawSprite_0102
		
		push	eax	
		mov		ecx, eax
			
loc_FillZeroAlpha:
		
		//romandou
		
		mov		[edi], 0
		inc		edi
		inc		edi
		dec		ecx
		jnz     loc_FillZeroAlpha
		pop     eax
		
		sub		edx, eax
		jg		loc_DrawSprite_0101
		add		edi, nNextLine
		dec		height
		jnz		loc_DrawSprite_0100
		jmp		loc_DrawSprite_exit
			
			
			//alpha值不为0时
loc_DrawSprite_0102:
		
		push	eax // n值
		push	edx // 当前行还剩多长未处理 
		push	ebx	// nAlpha值
		
		mov		ecx, eax
			
			
loc_DrawSprite_0103:
		
		mov     ebx, pPalette
		movzx	eax, byte ptr[esi]
		inc		esi
		mov		dx, [ebx + eax * 2]  //获得 16位 r4g4b4a4 a = 0
		
		mov		[edi], dx
		inc		edi
		inc		edi
		dec		ecx
		jnz		loc_DrawSprite_0103
		pop     ebx   //ebx = nAlpha
		pop		edx   //edx = 该行剩于
		pop		eax   //eax = n
		sub		edx, eax
		jg		loc_DrawSprite_0101
		add		edi, nNextLine
		dec		height
		jnz		loc_DrawSprite_0100
		jmp		loc_DrawSprite_exit
			
loc_DrawSprite_exit:	
	}
	return S_OK;		
}

//4444 --> 565 with alpha

//4444 --> 4444 with alpha
BYTE g_aAlphaResult4444[16*16*16];
void InitAlpha4444()
{
	for (int i=0; i<16; i++)
	for (int j=0; j<16; j++)
	{
		for (int a = 0; a < 16; a++)
		{
			int ii = (i << 4) + 8;
			int jj = (j << 4) + 8;
			int aa = (a << 4);
			g_aAlphaResult4444[(i<<8) | (j<<4) | (a)] = ((ii * (255 - aa) + jj * aa) / 255) >>4 ;
		}
	}
}

inline BYTE GetAlpha4444(BYTE i, BYTE j, BYTE a)
{
	return g_aAlphaResult4444[(i<<8) | (j<<4) | (a)];
	//return (((i<<4) * (255 - (a<<4)) + (j<<4) * (a<<4)) / 255) >>4 ;
}


inline void OffsetSprY(int y, int width, BYTE* & pSrc)
{
	int w = width;
	while(y)
	{
		int n = *pSrc++;
		int alpha = *pSrc++;
		if (alpha != 0)
		{
			pSrc+=n;
		}
		w-=n;
		ASSERT(w >= 0);
		if (w == 0)
		{
			w = width;
			y--;
			continue;
		}
	}
}

inline void OffsetSprX(int x, int& n, int& alpha, BYTE* & pSrc)
{
	n=0;
	while(x)
	{
		n = *pSrc++;
		alpha = *pSrc++;
		if (x <= n)
		{
			if (alpha != 0)
			{
				pSrc+=x;
			}
			n = n - x;
//			x = 0;
			break;
		}
		else
		{
			if (alpha != 0)
			{
				pSrc+=n;
			}
			x-=n;			
		}
	}
}

HRESULT cImageSpr::RenderToA4R4G4B4Alpha2(stImageRender& render)
{
	CString strMessage;
	strMessage.Format("%s RenderToA4R4G4B4Alpha2 frame %d ", m_strName, render.nFrame);
	cTimerFunction timer(strMessage);
	
	if (render.nFrame < 0 || render.nFrame >= GetFrames())
		return FALSE;
	// goto frame
	SPRFRAME* pFrame = (SPRFRAME*)(GetFrame(render.nFrame));	
	int height = pFrame->Height;
	int width = pFrame->Width;

	WORD* pPalette = (WORD*)GetPalette();	

	PBYTE pSrc	= pFrame->Sprite;
	WORD* pDest = (WORD*)render.buffer;
	pDest += ( render.ptDes.y * render.nPitch / 2 + render.ptDes.x);
	CRect rcSrc = render.rcSrc;
	int render_width = rcSrc.Width();

	// pSrc should have a offset
	if (rcSrc.top > 0)
		OffsetSprY(rcSrc.top,width,pSrc);
	int xoff = rcSrc.left;
	for (int y=0; y<rcSrc.Height(); y++)
	{
		WORD* pD = pDest;
		int n=0;
		int alpha;
		if (xoff > 0)
			OffsetSprX(xoff,n,alpha,pSrc);
		//next time off
		xoff = ( width - rcSrc.right + rcSrc.left );
		pDest += render.nPitch / 2;
		//render to pD length render_width

		int w = render_width;
		//w 剩下多少
		if (n == 0)
		{
			n = *pSrc++;
			alpha = *pSrc++;
		}

		while (w > 0)
		{
			//前面剩下的
			int minnw = min(n,w);
			if (alpha != 0)
			{
				//render to des of w!
				if (alpha >= 0xf0)
				{
					for (int x = 0; x < minnw; x++)
					{
						*pD++ = pPalette[*pSrc++] | (0xf000);
					}
				}
				else
				{
					BYTE a2 = alpha >> 4;
					for (int x = 0; x < minnw; x++)
					{
						//get src color
						BYTE r2,g2,b2;
						GetColorR4G4B4(pPalette[*pSrc++],r2,g2,b2);

						//get des color
						BYTE a1,r1,g1,b1;
						GetColorA4R4G4B4(*pD,a1,r1,g1,b1);
						if (a1 == 0)
						{
							SetColorA4R4G4B4(*pD++,a2,r2,g2,b2);
						}
						else
						{
							BYTE r,g,b;
							r = GetAlpha4444(r1,r2,a2);
							g = GetAlpha4444(g1,g2,a2);
							b = GetAlpha4444(b1,b2,a2);
							SetColorA4R4G4B4(*pD++,0x0f,r,g,b);
						}
					}
				}
				pSrc += (n - minnw);
			}
			else
				pD += minnw;
			if (n >= w)
			{
				xoff -= n - w;
				break;
			}
			w -= minnw;
			//end n >= w

			n = *pSrc++;
			alpha = *pSrc++;
		}
	}
	//Offset src to x!
	return S_OK;		
}

HRESULT cImageSpr::RenderToA4R4G4B4Alpha(stImageRender& render)
{
	cTimerFunction timer("RenderToA4R4G4B4Alpha");

	//first render it to a 4444 surface
	stFrameInfo info;
	GetFrameInfo(info,render.nFrame);
	WORD* pW = new WORD[info.width * info.height];
	memset(pW,0,sizeof(WORD)*info.width*info.height);
	stImageRender rd;
	rd.buffer = pW;
	rd.nPitch = 2 * info.width;
	rd.eOp = Render_Copy;
	rd.nFrame = render.nFrame;
	rd.ptDes = CPoint(0,0);
	rd.rcSrc = CRect(0,0,info.width,info.height);
	rd.format = IMAGE_4444;
	Render(rd);

	if (render.rcSrc.Width() > info.width || render.rcSrc.Height() > info.height)
	{
		ASSERT(0);
		return -1;
	}

	{
//		cTimerFunction timer("RenderToA4R4G4B4Alpha next part");
		//then render the rd4444 to target4444!
		WORD* pDest = (WORD*)render.buffer;
		pDest += ( render.ptDes.y * render.nPitch / 2 + render.ptDes.x );
		WORD* pSrc = (WORD*)rd.buffer;
		pSrc += (int)( render.rcSrc.top * rd.nPitch / 2 + render.rcSrc.left );
		for (int y=0; y<render.rcSrc.Height(); y++)
		{
			WORD* pD = pDest;
			WORD* pS = pSrc;
			for (int x=0; x<render.rcSrc.Width(); x++)
			{
				WORD d = *pD;
				WORD s = *pS++;
				BYTE a2,r2,g2,b2;

				a2 = ((s >> 12));
				if (a2 == 0)//透明
					pD++;
				else if (a2 >= 0x0f)//不透明
				{
					*pD++ = s;
				}
				else
				{
					BYTE a1,r1,g1,b1;
					a1 = ((d >> 12));
					if (a1 == 0)
						*pD++ = s;
					else
					{
	//					GetColorA4R4G4B4(s,a2,r2,g2,b2);
						r2 = ((((WORD) (s << 4)) >> 12)) ;
						g2 = ((((WORD) (s << 8)) >> 12)) ;
						b2 = ((((WORD) (s << 12)) >> 12)) ;
	//					GetColorA4R4G4B4(d,a1,r1,g1,b1);
						r1 = ((((WORD) (d << 4)) >> 12)) ;
						g1 = ((((WORD) (d << 8)) >> 12)) ;
						b1 = ((((WORD) (d << 12)) >> 12)) ;
						BYTE r,g,b;
						r = GetAlpha4444(r1,r2,a2);
						g = GetAlpha4444(g1,g2,a2);
						b = GetAlpha4444(b1,b2,a2);
						SetColorA4R4G4B4(*pD,0x0f,r,g,b);
						pD++;
					}
				}
			}
			pDest += render.nPitch / 2;
			pSrc += rd.nPitch / 2;
		}
	}
	delete pW;
	
	return S_OK;
}

HRESULT cImageSpr::RenderToR5G6B5Alpha(stImageRender& render)
{
	//first render it to a 4444 surface
	stFrameInfo info;
	GetFrameInfo(info,render.nFrame);
	WORD* pW = new WORD[info.width * info.height];
	memset(pW,0,sizeof(WORD)*info.width*info.height);
	stImageRender rd;
	rd.buffer = pW;
	rd.nPitch = 2 * info.width;
	rd.eOp = Render_Copy;
	rd.nFrame = render.nFrame;
	rd.ptDes = CPoint(0,0);
	rd.rcSrc = CRect(0,0,info.width,info.height);
	rd.format = IMAGE_4444;
	Render(rd);

	//then render the rd4444 to target565!
	
	WORD* pDest = (WORD*)render.buffer;
	pDest += ( render.ptDes.y * render.nPitch / 2 + render.ptDes.x );
	WORD* pSrc = (WORD*)rd.buffer;
	pSrc += (int)( render.rcSrc.top * rd.nPitch / 2 + render.rcSrc.left );
	for (int y=0; y<render.rcSrc.Height(); y++)
	{
		WORD* pD = pDest;
		WORD* pS = pSrc;
		for (int x=0; x<render.rcSrc.Width(); x++)
		{
			WORD d = *pD;
			WORD s = *pS++;
			BYTE a1,r1,g1,b1;
			GetColorR5G6B5(d,r1,g1,b1);
			BYTE a2,r2,g2,b2;
			GetColorA4R4G4B4Format(s,a2,r2,g2,b2);
			if (a2 == 0)//透明
				pD++;
			else if (a2 >= 0xf0)//不透明
			{
				SetColorR5G6B5(*pD,r2,g2,b2);
				pD++;
//				*pD++ = ((WORD)(r2 >> 3)) << 11 | ((WORD)(g2 >> 2)) << 5 | (b2 >> 3); 
			}
			else
			{
				BYTE r,g,b;
				r = (r1 * (255 - a2) + r2 * a2) / 255;
				g = (g1 * (255 - a2) + g2 * a2) / 255;
				b = (b1 * (255 - a2) + b2 * a2) / 255;
				SetColorR5G6B5(*pD,r,g,b);
				pD++;
//				*pD++ = ((WORD)(r >> 3)) << 11 | ((WORD)(g >> 2)) << 5 | (b >> 3); 
			}
		}
		pDest += render.nPitch / 2;
		pSrc += rd.nPitch / 2;
	}
	delete pW;
	
	return S_OK;
}


BOOL cImageSpr::Save()
{
	CFile File;
	if (!AppOpenFile(File,m_strName,CFile::modeCreate | CFile::modeWrite,"image"))
		return FALSE;
	File.Write(m_Buffer,m_size);
	File.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
// 函数:	Load
// 功能:	载入
// 参数:	FileName 文件名
// 返回:	TRUE－成功
//			FALSE－失败
//---------------------------------------------------------------------------
BOOL cImageSpr::Load(LPCSTR FileName)
{
	CFile		File;
	SPRHEAD*	pHeader;
	PBYTE		pTemp;

	// open the file
	if (!AppOpenFile(File,FileName,CFile::modeRead | CFile::shareDenyWrite,"image"))
	{
		if (!AppOpenFile(File,"\\system\\spr\\white.spr",CFile::modeRead | CFile::shareDenyWrite,"image"))
			return FALSE;
	}

	m_strName = FileName;

	m_size = File.GetLength();
	m_Buffer = new BYTE[m_size];
	pTemp = m_Buffer;
	
	// read data from file
	File.Read(pTemp, File.GetLength());

	// check file header setup sprite member
	pHeader = (SPRHEAD*)pTemp;

//	if (strcpy(pHeader->Comment, "SPR"))
//		return FALSE;

	// get sprite info
	m_nWidth      = pHeader->Width;
	m_nHeight     = pHeader->Height;
	m_nCenterX    = pHeader->CenterX;
	m_nCenterY    = pHeader->CenterY;
	m_nFrames     = pHeader->Frames;
	m_nColors     = pHeader->Colors;
	m_nDirections = pHeader->Directions;
	m_nInterval   = pHeader->Interval;

	// setup palette pointer
	pTemp += sizeof(SPRHEAD);
	m_pPal24 = (KPAL24*)pTemp;

	// setup offset pointer
	pTemp += m_nColors * sizeof(KPAL24);
	m_pOffset = (SPROFFS*)pTemp;

	// setup sprite pointer
	pTemp += m_nFrames * sizeof(SPROFFS);
	m_pSprite = (LPBYTE)pTemp; // 相对偏移

	// make color table
	m_Palette = new BYTE[m_nColors * sizeof(KPAL16)];
//	MakePalette();

	return TRUE;
}

//---------------------------------------------------------------------------
// 函数:	Load
// 功能:	载入
// 参数:	FileName 文件名
// 返回:	TRUE－成功
//			FALSE－失败
//---------------------------------------------------------------------------
BOOL cImageSpr::LoadInfo(LPCSTR FileName)
{
	CFile		File;
	SPRHEAD*	pHeader;
	PBYTE		pTemp;

	// open the file
	if (!AppOpenFile(File,FileName,CFile::modeRead | CFile::shareDenyWrite,"image"))
	{
//		if (!AppOpenFile(File,"\\system\\txt\\planedefault.spr",CFile::modeRead | CFile::shareDenyWrite,"image"))
		return FALSE;
	}

	m_strName = FileName;

	m_size = File.GetLength();
	
	SPRHEAD head;
	
	// read data from file
	File.Read(&head, sizeof(SPRHEAD));

	// check file header setup sprite member
	pHeader = &head;

//	if (strcpy(pHeader->Comment, "SPR"))
//		return FALSE;

	// get sprite info
	m_nWidth      = pHeader->Width;
	m_nHeight     = pHeader->Height;
	m_nCenterX    = pHeader->CenterX;
	m_nCenterY    = pHeader->CenterY;
	m_nFrames     = pHeader->Frames;
	m_nColors     = pHeader->Colors;
	m_nDirections = pHeader->Directions;
	m_nInterval   = pHeader->Interval;

	int size = sizeof(SPRHEAD);
	int size2;
	size2 = m_nColors * sizeof(KPAL24) + m_nFrames * sizeof(SPROFFS);
	size += size2;
	m_Buffer = new BYTE[size];
	memcpy(m_Buffer,&head,sizeof(SPRHEAD));
	pTemp = m_Buffer;
	pTemp += sizeof(SPRHEAD);
	File.Read(pTemp,size2);

	m_pPal24 = (KPAL24*)pTemp;

	// setup offset pointer
	pTemp += m_nColors * sizeof(KPAL24);
	m_pOffset = (SPROFFS*)pTemp;

	return TRUE;
}


//---------------------------------------------------------------------------
// 函数:	Free
// 功能:	释放
// 参数:	void
// 返回:	void
//---------------------------------------------------------------------------
void cImageSpr::Free()
{
	delete m_Buffer;
	delete m_Palette;
	m_nFrames = 0;
	m_nColors = 0;
	m_pPal24  = NULL;
	m_pPal16  = NULL;
	m_pOffset = NULL;
	m_pSprite = NULL;
}
//---------------------------------------------------------------------------
// 函数:	MakePalette
// 功能:	生成色盘
// 参数:	void
// 返回:	void
//---------------------------------------------------------------------------
void cImageSpr::MakePalette()
{
	m_pPal16 = (KPAL16*)m_Palette;
	g_Pal24ToPal16( (::KPAL24*)m_pPal24, (KPAL16*)m_pPal16, m_nColors);
}
//---------------------------------------------------------------------------
// 函数:	Make4444Palette
// 功能:	生成4444色盘，其中alpha层为0
// 参数:	void
// 返回:	void
//---------------------------------------------------------------------------
void cImageSpr::Make4444Palette()
{
//	m_Palette = new BYTE[m_nColors * sizeof(KPAL16)];
	m_pPal16 = (KPAL16*)m_Palette;
	
	int nColors = m_nColors;
	KPAL24 * pPal24 = m_pPal24;
	KPAL16 * pPal16 = m_pPal16;
	
	while (nColors -- > 0)
	{
		//*pPal16 = pPal24->Red<<4 | pPal24->Green<<8 | pPal24->Blue << 12 | 0;
		WORD	Red = (pPal24->Red) >> 4;
		WORD	Green = (pPal24->Green) >> 4;
		WORD	Blue = (pPal24->Blue) >> 4;

		*pPal16 = (Red << 8) | (Green << 4) | Blue;

		pPal16++;
		pPal24++;
	}
	
}
//---------------------------------------------------------------------------
// 函数:	RGB
// 功能:	返回一个DWORD色彩值
// 参数:	red		红色分量
//			green	绿色分量
//			blue	蓝色分量
// 返回:	色彩值
//---------------------------------------------------------------------------
WORD g_RGB565(int nRed, int nGreen, int nBlue)
{
	WORD wColor;
	__asm
	{
		xor		ecx, ecx
		mov		ebx, 0xff
		mov		eax, nRed
		and		eax, ebx
		shr		eax, 3
		shl		eax, 11
		or		ecx, eax
		mov		eax, nGreen
		and		eax, ebx
		shr		eax, 2
		shl		eax, 5
		or		ecx, eax
		mov		eax, nBlue
		and		eax, ebx
		shr		eax, 3
		or		ecx, eax
		mov		wColor, cx
	}
	return wColor;
}
//---------------------------------------------------------------------------
// 函数:	Pal24ToPal16
// 功能:	24位调色板转化位16位调色板
// 参数:	pPal24		KPAL24指针
//			pPal16		KPAL16指针
//			nClolors	颜色数(0 - 256)
// 返回:	void
//---------------------------------------------------------------------------
/*
void g_Pal24ToPal16(KPAL24* pPal24, KPAL16* pPal16, int nColors)
{
	while (nColors-- > 0)
	{
		*pPal16 = g_RGB565(pPal24->Red, pPal24->Green, pPal24->Blue);
		pPal16++;
		pPal24++;
	}
}
*/

//---------------------------------------------------------------------------
// 函数:	NexFrame
// 功能:	取得下一帧
// 参数:	nFrame		当前帧
// 返回:	下一帧
//---------------------------------------------------------------------------
UINT cImageSpr::NextFrame(UINT nFrame)
{
	nFrame++;
	if (nFrame >= m_nFrames)
		nFrame = 0;
	return nFrame;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// 函数:	GetPixelAlpha
// 功能:	取得某一点的alpha值
// 参数:	nFrame		当前帧
//---------------------------------------------------------------------------
int	cImageSpr::GetPixelAlpha(UINT nFrame, int x, int y)
{
	int	nAlpha = 0;
	if (nFrame >= 0 && nFrame < m_nFrames)
	{
		SPRFRAME* pFrame = (SPRFRAME*)(m_pSprite + m_pOffset[nFrame].Offset);
//		x -= pFrame->OffsetX;
//		y -= pFrame->OffsetY;
		if (x >= 0  && x < pFrame->Width && y >= 0 && y < pFrame->Height)
		{

			int	nNumPixels = pFrame->Width;
			void*	pSprite = pFrame->Sprite;
			y++;
			_asm
			{
				//使SDI指向sprite中的图形数据位置
				mov		esi, pSprite
			dec_line:
				dec		y				//减掉一行
				jz		last_line

				mov		edx, nNumPixels
			skip_line:
				movzx	eax, byte ptr[esi]
				inc		esi
				movzx	ebx, byte ptr[esi]
				inc		esi
				or		ebx, ebx
				jz		skip_line_continue
				add		esi, eax
			skip_line_continue:
				sub		edx, eax
				jg		skip_line
				jmp		dec_line

			last_line:
				mov		edx, x
			last_line_alpha_block:
				movzx	eax, byte ptr[esi]
				inc		esi
				movzx	ebx, byte ptr[esi]
				inc		esi
				or		ebx, ebx
				jz		last_line_continue
				add		esi, eax
			last_line_continue:
				sub		edx, eax
				jg		last_line_alpha_block
				mov		nAlpha, ebx
			}
		}
	}
	return nAlpha;
}

//---------------------------------------------------------------------------
// 函数:	GetFrame
// 功能:	Get Sprite Frame
// 参数:	nFrame	帧
// 返回:	void
//---------------------------------------------------------------------------
LPVOID cImageSpr::GetFrame(UINT nFrame)
{
	LPBYTE	pSprite = m_pSprite;

	// check frame range
	if (nFrame < 0 || nFrame >= m_nFrames)
		return NULL;
	
	// goto frame
	pSprite += m_pOffset[nFrame].Offset;
	
	return (LPVOID)pSprite;
}


//////////////////////////////////////////////////////////////////////
//load and save
//////////////////////////////////////////////////////////////////////

ULONG cImageSpr::AddImage(iImage* p)
{
	ASSERT(m_Buffer == NULL && m_strName.IsEmpty());

	stImageInfo info;
	p->GetImageInfo(info);
	ASSERT(info.buffer != NULL && info.size != 0);
	ASSERT(info.format == IMAGE_888 || info.format == IMAGE_8888);
//	ASSERT(info.nFrames == 1);
	m_info = info;
	for (UINT i=0; i<info.nFrames; i++)
	{
		stFrameInfo frameInfo;
		p->GetFrameInfo(frameInfo,i);
		m_aFrameInfo[m_nFrameInfo] = frameInfo;
		m_aFrameInfo[m_nFrameInfo].buffer = new BYTE[info.size];
		memcpy(m_aFrameInfo[m_nFrameInfo].buffer, frameInfo.buffer, frameInfo.size);
		m_nFrameInfo++;
	}
	return 0;	
}

HRESULT	cImageSpr::LoadFileInfo(const char *filename)
{
	CString sPathName = filename;
//	AppGetFullPath(sPathName);
	ASSERT(sPathName.Right(3).CompareNoCase("spr") == 0);
	return LoadInfo(sPathName);
}

HRESULT cImageSpr::LoadFile(const char *filename)
{
	//debug
	CString sPathName = filename;
	if (sPathName.Right(3).CompareNoCase("spr") != 0)
		return -1;
	m_nFrameInfo = 0;
	BOOL b = Load(sPathName);
	if (!b)
	{
//		AfxMessageBox("请更新spr资源或清除临时文件!");
		return -1;
	}
	return 0;
}

HRESULT cImageSpr::SaveFile(const char *filename, stImageInfo* pInfo)
{
	//NO need save
	if (m_nFrameInfo == 0)
	{
		//just copy
		if (m_strName.IsEmpty())
			return -1;
		AppCopyFile(m_strName,filename);
		return S_OK;
	}
	if (m_nFrameInfo == 0)
		return -1;

	ASSERT(m_nFrameInfo > 0);
	m_info.nFrames = m_nFrameInfo; 
	m_nFrames = m_nFrameInfo;
	switch (m_info.format)
	{
	case IMAGE_888:
		return SaveFile888(filename, pInfo);
		break;
	case IMAGE_8888:
		return SaveFile8888(filename, pInfo);
		break;
	default:
		ASSERT(0);
	}
	return S_OK;
}

HRESULT cImageSpr::SaveFile8888(const char *filename, stImageInfo* pInfo)
{
	int nDirections = 1;
	if (pInfo != NULL)
	{
		if (pInfo->nDirections != 0 && pInfo->nDirections != 1 )
		{
			nDirections = m_nFrameInfo / pInfo->nDirections;
//			ASSERT(m_nFrameInfo % pInfo->nDirections == 0);
			if (m_nFrameInfo % pInfo->nDirections != 0)
			{
				CString strErr;
				strErr.Format("'%s' 方向：%d 帧数：%d 该套图片不齐，请重新生成!",filename,pInfo->nDirections,m_nFrameInfo);
				AfxMessageBox(strErr);
			}
		}
	}

	KOctree	ImageTree;
	for (int i=0; i<m_nFrameInfo; i++)
	{
//		/4 because format = IMAGE_8888
		BYTE* buffer = m_aFrameInfo[i].buffer;
		int sz = m_aFrameInfo[i].size/m_info.bytes_per_pixel;
		for (int j=0; j<sz; j++)
		{
			ImageTree.Insert(*(buffer+2), *(buffer+1), *(buffer+0));
			//no alpha
			buffer += m_info.bytes_per_pixel;
		}
	}
	int	nColors = ImageTree.m_nColors;
	ImageTree.InitPalette();
	ImageTree.ReduceNode();
	ImageTree.MakePalette(&ImageTree.RootNode);
	

	KSpriteMaker	SprMaker;
	for (i=0; i<m_nFrameInfo; i++)
	{
		int sz = m_aFrameInfo[i].size/m_info.bytes_per_pixel;
		KBitmap	Bitmap;
		KBitmap	Alpha;
		Bitmap.Init(m_aFrameInfo[i].width,m_aFrameInfo[i].height,256);
		Alpha.Init(m_aFrameInfo[i].width,m_aFrameInfo[i].height,256);
		BYTE *pBitmap = (BYTE *)Bitmap.GetBitmap();
		BYTE *pAlpha = (BYTE *)Alpha.GetBitmap();
		BYTE* buffer = m_aFrameInfo[i].buffer;
		for (int j=0; j<sz; j++)
		{
			*pBitmap++ = ImageTree.QueryIndex(*(buffer+2), *(buffer+1), *(buffer+0));
			//no alpha
			buffer += m_info.bytes_per_pixel - 1;
			*pAlpha++ = *buffer++;
		}
		g_Pal24ToPal32(ImageTree.m_Palette, Bitmap.GetPal32(), 256);
		SprMaker.AddAlphaFrame(&Bitmap, &Alpha);			
	}
	SprMaker.m_nDirections = nDirections;
	char sprName[256];
	strcpy(sprName,filename);
	SprMaker.Save(sprName);

	return 0;
}

HRESULT cImageSpr::SaveFile888(const char *filename, stImageInfo* pInfo)
{
	int nDirections = 1;
	if (pInfo != NULL)
	{
		if (pInfo->nDirections != 0)
		{
			nDirections = m_nFrameInfo / pInfo->nDirections;
			if (m_nFrameInfo % pInfo->nDirections != 0)
			{
				CString strErr;
				strErr.Format("'%s' 方向：%d 帧数：%d 该套图片不齐，请重新生成!",filename,pInfo->nDirections,m_nFrameInfo);
				AfxMessageBox(strErr);
			}
		}
	}

	KOctree	ImageTree;
	for (int i=0; i<m_nFrameInfo; i++)
	{
//		/4 because format = IMAGE_8888
		BYTE* buffer = m_aFrameInfo[i].buffer;
		int sz = m_aFrameInfo[i].size/m_info.bytes_per_pixel;
		for (int j=0; j<sz; j++)
		{
			ImageTree.Insert(*(buffer+2), *(buffer+1), *(buffer+0));
			buffer += m_info.bytes_per_pixel;
		}
	}
	int	nColors = ImageTree.m_nColors;
	ImageTree.InitPalette();
	ImageTree.ReduceNode();
	ImageTree.MakePalette(&ImageTree.RootNode);

	KSpriteMaker SprMaker;
	for (i=0; i<m_nFrameInfo; i++)
	{
		int sz = m_aFrameInfo[i].size/m_info.bytes_per_pixel;
		KBitmap	Bitmap;
		Bitmap.Init(m_aFrameInfo[i].width,m_aFrameInfo[i].height,256);
		BYTE *pBitmap = (BYTE *)Bitmap.GetBitmap();
		BYTE* buffer = m_aFrameInfo[i].buffer;
		for (int j=0; j<sz; j++)
		{
			*pBitmap++ = ImageTree.QueryIndex(*(buffer+2), *(buffer+1), *(buffer+0));
			buffer += m_info.bytes_per_pixel;
			//no alpha
		}
		g_Pal24ToPal32(ImageTree.m_Palette, Bitmap.GetPal32(), 256);
		SprMaker.AddFrame(&Bitmap, FALSE);			
	}
	SprMaker.m_nDirections = nDirections;
	char sprName[256];
	strcpy(sprName,filename);
	SprMaker.Save(sprName);

	return 0;
}

BOOL cImageSpr::IsAlphaBad()
{
	for (int i=0; i<GetFrames(); i++)
	{
		for (int x=0; x<2; x++)
		for (int y=0; y<2; y++)
		{
			int a = GetPixelAlpha(i,x,y);
			if (a > 0 && a < 10)
			{
			}
			else
			{
				goto next;
			}
		}
		return TRUE;
next:
		;
	}
	return FALSE;
}

/*
HRESULT	cImageSpr::RemoveShadow()
{
	int nFrame = GetFrames()-1;
	//最右边的阴影部分
	int x = FindShadowPos(nFrame);
	//修改buffer
	//修改最终的size
	//存储
	return 0;
}
*/
