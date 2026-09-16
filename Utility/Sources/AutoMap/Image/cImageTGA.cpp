// cImageTga.cpp: implementation of the cImageTga class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "cImageTga.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cImageTga::cImageTga()
{
	m_info.format = IMAGE_8888;
	m_info.bytes_per_pixel = 4;
	m_info.nDirections = 1;
	m_info.nFrames = 1;
	m_info.buffer = NULL;
}

cImageTga::~cImageTga()
{
	if (m_info.buffer)
		delete m_info.buffer;
}

//////////////////////////////////////////////////////////////////////
//Load and Save
//////////////////////////////////////////////////////////////////////

#define UL_TGA_LR 0x10
#define UL_TGA_BT 0x20


#define UL_GFX_SWAP_RGB     0x0001
#define UL_GFX_PAD          0x0004
#define UL_GFX_COMPRESSED   0x0008
#define UL_GFX_SIGNEDPAD    0x0010
#define UL_GFX_INVERT_VERT  0x0020
#define UL_GFX_INVERT_HORIZ 0x0040


#define UL_GFX_565          0x00020000

#ifndef bread
#define bread() ((byte)fgetc(f))
#endif
#ifndef wread
#define wread() (((byte)bread())|(((byte)bread())<<8))
#endif

HRESULT cImageTga::LoadFile(const char *filename) 
{
	stTgaHeader m_stHeader;


    FILE *f;

    f = fopen(filename, "rb");
    ASSERT(f);

    unsigned char *data;

	const int flags = 0;

    int i;
    int a, r, g, b;
    int right, top;

	int size1 = sizeof(stTgaHeader);
	int count = fread(&m_stHeader,sizeof(stTgaHeader),1,f);
	ASSERT(count == 1);

    m_info.width = m_stHeader.dwWidth;
    m_info.height = m_stHeader.dwHeight;
	m_info.bytes_per_pixel = m_stHeader.bits_per_pixel / 8;

	if (m_info.bytes_per_pixel != 4 || m_stHeader.m_type != 2 && m_stHeader.m_type != 10)
	{
		CString strMsg;
		strMsg.Format("File: %s的格式不对，只支持32位的tga",filename);
		AfxMessageBox(strMsg);
		fclose(f);
		return FALSE;
	}

	ASSERT(m_info.bytes_per_pixel == 4);
	if (m_info.buffer)
		delete m_info.buffer;
    int hxw = m_info.height * m_info.width;
	m_info.size = hxw * m_info.bytes_per_pixel;
    m_info.buffer = new BYTE[m_info.size];
    ASSERT(m_info.buffer);

    right = m_stHeader.m_desc & UL_TGA_LR;
    top = m_stHeader.m_desc & UL_TGA_BT;

    fread((char *) m_id, m_stHeader.m_id_length, 1, f);
    fread((char *) m_color_palette, m_stHeader.m_cm_length, m_stHeader.m_cm_entry_size >> 3, f);

    data = (unsigned char *)m_info.buffer;

	switch (m_stHeader.m_type)
	{
	case 2:
		for (i = 0; i < hxw; i++)
		{
			b = bread();
			g = bread();
			r = bread();
			a = bread();
			if (flags & UL_GFX_SWAP_RGB)
			{
				*data++ = r;
				*data++ = g;
				*data++ = b;
				*data++ = a;
			}
			else
			{
				*data++ = b;
				*data++ = g;
				*data++ = r;
				*data++ = a;
			}
		}
		break;
	case 10:
        i = 0;
        while (i < hxw)
        {
			int rle;
            rle = bread();
            if (rle < 0x80)
            {
                rle++;
                while (rle)
                {
                    if (flags & UL_GFX_SWAP_RGB)
                    {
                        b = bread();
                        g = bread();
                        r = bread();
                        a = bread();
                        *data++ = r;
                        *data++ = g;
                        *data++ = b;
                        *data++ = a;
                    }
                    else
                    {
                        
                        *data++ = bread();
                        *data++ = bread();
                        *data++ = bread();
                        *data++ = bread();
                    }
                    rle--;
                    i++;
					ASSERT(i<=hxw);
                }
            }
            else 
            {
                rle -= 127;
                b = bread();
                g = bread();
                r = bread();
                a = bread();
                
                while (rle)
                {
                    if (flags & UL_GFX_SWAP_RGB)
                    {
                        *data++ = r;
                        *data++ = g;
                        *data++ = b;
                        *data++ = a;
                    }
                    else
                    {
                        *data++ = b;
                        *data++ = g;
                        *data++ = r;
                        *data++ = a;
                    }
                    rle--;
                    i++;
					ASSERT(i<=hxw);
                }
            }
            
        }
        break;
	}

    fclose(f);
    f = NULL;

	if (!top)
	{
		unsigned long *swap = (unsigned long *) new unsigned long[hxw];

		memcpy(swap, m_info.buffer, hxw * sizeof(unsigned long));

		unsigned long * src, * dest;
		unsigned long * src_data = (unsigned long *)m_info.buffer;

		for (unsigned int i = 0; i < m_info.height; i++)
		{
			src = &swap[(m_info.height - i - 1) * m_info.width];
			dest = &src_data[i * m_info.width];

			memcpy(dest, src, m_info.width * sizeof(unsigned long) );
		}

		delete swap;
	}

	return S_OK;
}

ULONG cImageTga::AddImage(iImage* p) 
{
	ASSERT(0);
	return S_OK;
}

unsigned char GetAlpha(long value)
{
	return (unsigned char)(value>>24);
}

ULONG ImageSetAlpha1(stImageInfo& info) 
{
	long* pDes = (long*)info.buffer;
	if (pDes == NULL)
		return 0;
	if (GetAlpha(pDes[info.width - 1]) == 0 || GetAlpha(pDes[(info.height-1)*info.width]) == 0)
	{
		long cr = 0x01010101;
		if (GetAlpha(pDes[0]) == 0)
			pDes[0] = cr;
		//使用这两个点得到的图小一些,同时保证了组件的正确
//		if (GetAlpha(pDes[(info.height - 1) * info.width ]) == 0)
//			pDes[(info.height - 1) * info.width] = cr;
		if (GetAlpha(pDes[info.height * info.width - 1]) == 0)
			pDes[info.height * info.width - 1] = cr;
	}
	return S_OK;
}

ULONG cImageTga::SetAlpha1() 
{	
	return ImageSetAlpha1(m_info);
}

void SaveTGAFile(const char *filename, const stImageInfo& info)
{
	ASSERT(info.bytes_per_pixel == 4);

	cImageTga::stTgaHeader m_stHeader;
	memset(&m_stHeader,0,sizeof(m_stHeader));

	m_stHeader.m_type = 2;
	m_stHeader.bits_per_pixel = 32;
	m_stHeader.dwHeight = (short)info.height;
	m_stHeader.dwWidth = (short)info.width;
	m_stHeader.m_desc = UL_TGA_BT;

//四个角设置alpha值
	long* pDes = (long*)info.buffer;
	if (GetAlpha(pDes[info.width - 1]) == 0 || GetAlpha(pDes[(info.height-1)*info.width]) == 0)
	{
		long cr = 0x01010101;
		if (GetAlpha(pDes[0]) == 0)
			pDes[0] = cr;
		if (GetAlpha(pDes[info.height * info.width - 1]) == 0)
			pDes[info.height * info.width - 1] = cr;
	}
//	pDes[info.width - 1] = cr;
//	pDes[(info.height-1)*info.width] = cr;


	CFile file;
	if (!AppOpenFile(file,filename,CFile::modeCreate|CFile::modeWrite,"image"))
		return;
	
	file.Write(&m_stHeader,sizeof(cImageTga::stTgaHeader));
	file.Write(info.buffer,info.height*info.width*4);
	
	file.Close();
}

HRESULT cImageTga::SaveFile(const char *filename, stImageInfo* info ) 
{
//	ASSERT(0);
	SaveTGAFile(filename,m_info);
	return S_OK;
}
//////////////////////////////////////////////////////////////////////
//info of a image
//////////////////////////////////////////////////////////////////////
void cImageTga::GetImageInfo(stImageInfo& info) 
{
/*
	info.format = IMAGE_8888;
	info.bytes_per_pixel = 4;
	info.nDirections = 1;
	info.nFrames = 1;
	info.size = m_info.width * m_info.height * m_info.bytes_per_pixel;
	info.buffer = m_info.buffer;
*/
	info = m_info;
}

void cImageTga::GetFrameInfo(stFrameInfo& info,ULONG nFrame ) 
{
	ASSERT(nFrame == 0);
	info.x = 0;
	info.y = 0;
	info.size = m_info.size;
	info.width = m_info.width;
	info.height = m_info.height;
	info.buffer = m_info.buffer;
}
//////////////////////////////////////////////////////////////////////
//render to target
//////////////////////////////////////////////////////////////////////
HRESULT cImageTga::Render(stImageRender& render) 
{
	ASSERT(0);
	return S_OK;
}

CBitmap* cImageTga::ImageAlphaToBitmap(int nFrame,int nDepth )
{
	ASSERT(m_info.buffer);

	CBitmap* pBmp = new CBitmap;
	if (nDepth == 16)
	{
		WORD* buffer = new WORD[m_info.width*m_info.height];

		WORD* pDest = buffer;
		BYTE* pSrc = m_info.buffer;
		for (unsigned int i=0; i<m_info.width*m_info.height; i++)
		{
			WORD b = *(pSrc++);
			WORD g = *(pSrc++);
			WORD r = *(pSrc++);
			WORD a = *(pSrc++);
			(*pDest++) = (a >> 3) | ((a >> 2) << 5) | ((a >> 3) << 11);
		}
		
		VERIFY(pBmp->CreateBitmap(m_info.width,m_info.height,
			1,16,buffer));

		delete buffer;
	}
	else if (nDepth == 32)
	{
		DWORD* buffer = new DWORD[m_info.width*m_info.height];

		DWORD* pDest = buffer;
		BYTE* pSrc = m_info.buffer;
		for (unsigned int i=0; i<m_info.width*m_info.height; i++)
		{
			DWORD b = *(pSrc++);
			DWORD g = *(pSrc++);
			DWORD r = *(pSrc++);
			DWORD a = *(pSrc++);
			(*pDest++) = (a << 24) | (a << 16) | (a << 8) | a;
		}
		
		VERIFY(pBmp->CreateBitmap(m_info.width,m_info.height,
			1,32,buffer));

		delete buffer;
	}
	return pBmp;
}

CBitmap* cImageTga::ImageToBitmap(ULONG nFrame,int nDepth) 
{
	ASSERT(m_info.buffer);

	CBitmap* pBmp = new CBitmap;
	if (nDepth == 16)
	{
		WORD* buffer = new WORD[m_info.width*m_info.height];

		WORD* pDest = buffer;
		BYTE* pSrc = m_info.buffer;
		for (unsigned int i=0; i<m_info.width*m_info.height; i++)
		{
			WORD b = *(pSrc++);
			WORD g = *(pSrc++);
			WORD r = *(pSrc++);
			WORD a = *(pSrc++);
			(*pDest++) = (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
		}
		
		VERIFY(pBmp->CreateBitmap(m_info.width,m_info.height,
			1,16,buffer));

		delete buffer;
	}
	else if (nDepth == 32)
	{
		VERIFY(pBmp->CreateBitmap(m_info.width,m_info.height,
			1,32,m_info.buffer));
	}
	return pBmp;	
}

HRESULT	cImageTga::AlphaFormat()
{
	BYTE* pSrc;
	BYTE bb,gg,rr;
	unsigned int i;
	//get color when alpha = 0;
	pSrc = m_info.buffer;
	for (i=0; i<m_info.width*m_info.height; i++)
	{
		BYTE b = *(pSrc++);
		BYTE g = *(pSrc++);
		BYTE r = *(pSrc++);
		BYTE a = *(pSrc++);
		if (a == 0)
		{
			bb = b;
			gg = g;
			rr = r;
			break;
		}
	}
	pSrc = m_info.buffer;
	for (i=0; i<m_info.width*m_info.height; i++)
	{
		WORD b = *(pSrc);
		WORD g = *(pSrc+1);
		WORD r = *(pSrc+2);
		WORD a = *(pSrc+3);
		// c = src * alpha + des * (1-alpha)
		// src = (c - des * (1-alpha)) / alpha
		if (a != 0 && a != 255)
		{
			short bbb = (b * 255 - bb * (255-a) ) / a;
			short ggg = (g * 255 - gg * (255-a) ) / a;
			short rrr = (r * 255 - rr * (255-a) ) / a;
			if (bbb < 0)
				bbb = 0;
			if (ggg < 0)
				ggg = 0;
			if (rrr < 0)
				rrr = 0;
			*(pSrc) = (BYTE)bbb;
			*(pSrc+1) = (BYTE)ggg;
			*(pSrc+2) = (BYTE)rrr;
		}
		pSrc += 4;
	}
	return S_OK;
}

BOOL cImageTga::DivideTo(LPCSTR szFile, UINT wide, UINT high)
{
	UINT wDivide = m_info.width / (wide+1);
	UINT hDivide = m_info.height / (high+1);

	stImageInfo info;
	info.format = m_info.format;
	info.bytes_per_pixel = m_info.bytes_per_pixel;
	unsigned char* pSrc;
	int nx = wide+1;//m_info.width / wDivide;
	int ny = high+1;//m_info.height / hDivide;
	int index = 0;
	CString strName;
	for (int y = 0; y < ny; y++ )
	for (int x = 0; x < nx; x++ )
	{
		if (x != nx-1)
			info.width =  wDivide;
		else
			info.width = m_info.width - (nx-1)*wDivide;
		
		if (y != 0)
			info.height = hDivide;
		else
			info.height = m_info.height - (ny-1)*hDivide;

		info.size = info.width * info.height* m_info.bytes_per_pixel;

		strName.Format(szFile,index++);
		pSrc = m_info.buffer + y * hDivide * m_info.width * m_info.bytes_per_pixel + x * wDivide * m_info.bytes_per_pixel;
		info.buffer = new unsigned char[info.size];
		unsigned char* pDes = info.buffer;
		for (UINT j=0; j < info.height; j++)
		{
			memcpy(pDes,pSrc,info.width * m_info.bytes_per_pixel);
			pDes += info.width * m_info.bytes_per_pixel;
			pSrc += m_info.width * m_info.bytes_per_pixel;
		}
//		m_aFrameInfo[m_nFrameInfo++] = info;
		SaveTGAFile(strName,info);
		delete info.buffer;
	}

	m_info.nDirections = 1;
	m_info.nFrames = ny * nx;
	return TRUE;
}

void cImageTga::DivideRect(LPCSTR szFile,const CRect& rect,stDivide* pDV)
{
	CRect r(0,0,m_info.width,m_info.height);
	CRect rc = rect;
	rc &= r;
	if (r.IsRectEmpty())
	{
//		r为空仍然要继续
//		return;
	}
	stImageInfo info;
	info.width = rc.Width();
	info.height = rc.Height();
	info.bytes_per_pixel = m_info.bytes_per_pixel;
	info.format = m_info.format;
    int hxw = info.height * info.width;
	info.size = hxw * info.bytes_per_pixel;
    info.buffer = new BYTE[info.size];
	long* pDes = (long* ) info.buffer;
	long* pSrc = (long* ) m_info.buffer;

	for (unsigned int y = 0; y < info.height; y++)
	for (unsigned int x = 0; x < info.width; x++)
	{
		pDes[y*info.width+x] = pSrc[(y+rc.top)*m_info.width+(x+rc.left)];
	}

	if (pDV != NULL)
	{
		ASSERT(0);
#pragma message (ATTENTION("这一段已经不再支持了"))
		stDivide& dv = *pDV;
		int xx1 = dv.aPoint[0].x;
		int yy1 = dv.aPoint[0].y - dv.nHeight;
		for (int i=1; i<dv.nPoint; i++)
		{
			int xx2,yy2;
			xx2 = dv.aPoint[i].x;
			yy2 = dv.aPoint[i].y - dv.nHeight;

			//  直线方程：
			//	y(x2-x1) - x(y2-y1) + (x1y2 - x2y1) = 0 
			//	将直线向下移动2个像素,去掉直线下方的点
			float add = 1.f;
			float x1 = (float)xx1;
			float x2 = (float)xx2;
			float y1 = (float)yy1+add;
			float y2 = (float)yy2+add;
			
			for (int x=(int)x1; x<x2; x++)
			for (int y=(int)min(y1,y2); y<rc.Height(); y++)
			{
				float f = (float)y * (x2 - x1) - (float)x * (y2 - y1) + (x1*y2 - x2*y1);
				const float minf = 0.01f;
				if (f > minf) 
				{
					ASSERT(y>=rc.top && y<rc.bottom && x>=rc.left && x<rc.right);
					pDes[(y-rc.top)*info.width + x-rc.left] = 0;
				}
			}
			xx1 = xx2;
			yy1 = yy2;
		}
	}

	ImageSetAlpha1(info);
	SaveTGAFile(szFile,info);
	delete info.buffer;
}

BOOL cImageTga::DivideTo(LPCSTR szFile,stDivide& dv)
{
	CRect r(0,0,m_info.width,m_info.height);
	if (dv.aPoint[0].x < 0)
		dv.aPoint[0].x = 0;
	if (dv.aPoint[dv.nPoint-1].x >= m_info.width)
		dv.aPoint[dv.nPoint-1].x = m_info.width-1;
	for (int i=0; i<dv.nPoint; i++)
	{
		if (dv.aPoint[i].x < 0 || dv.aPoint[i].x >= m_info.width)
			return FALSE;
		if (dv.aPoint[i].y < 0)
			dv.aPoint[i].y = 0;
		if (dv.aPoint[i].y >= m_info.height)
			dv.aPoint[i].y = m_info.height-1;
		if (dv.eDoodadType != 0) //HOR
		{
			if (dv.aPoint[i].y - dv.nHeight < 0)
				return FALSE;
		}
	}

	CString strFile = szFile; //like file%d.name

	int index = 0;
	if (dv.eDoodadType == 0) //ver
	{
		int x0,y0; 
		int x1,y1;
		CString strName;
		int index = 0;
		for (int y=0; y<=dv.nHigh; y++)
		{
			x0 = dv.aPoint[0].x;
			y0 = dv.aPoint[0].y;
			for (int i=1; i<dv.nPoint; i++)
			{
				strName.Format(strFile,index);
				x1 = dv.aPoint[i].x;
				y1 = dv.aPoint[i].y;

				int bottom = max(y1, y0) - (dv.nHigh-y) * dv.nHeight;
				int top = bottom - dv.nHeight;
				if (y == 0)
					DivideRect(strName,CRect(x0,0,x1,bottom));
				else
					DivideRect(strName,CRect(x0,top,x1,bottom));					

				x0 = x1;
				y0 = y1;
				index++;
			}
		}

		//删除以前多余的文件
		for (i=(dv.nPoint-1)*(dv.nHigh+1); i<99; i++)
		{
			strName.Format(strFile,i);
			CFileFind f;
			if (f.FindFile(strName))
			{
				AppDeleteFolder(strName);
			}
		}
	}
	else
	{
		ASSERT(0);
#pragma message (ATTENTION("这一段已经不再支持了"))
		ASSERT(dv.nHeight != 0);
		int ymax;

		int x0 = dv.aPoint[0].x;
		int y0 = dv.aPoint[0].y;
		ymax = y0 - dv.nHeight;
		CString strName;
		for (int i=1; i<dv.nPoint; i++)
		{
			int x1,y1;
			x1 = dv.aPoint[i].x;
			y1 = dv.aPoint[i].y;
			ymax = max(ymax,y1 - dv.nHeight);

			CRect r;
			r.left = x0;
			r.right = x1;
			r.bottom = max(y0,y1);
			r.top = min(y0,y1) - dv.nHeight;
			strName.Format(strFile,i);
			DivideRect(strName,r);
			x0 = x1;
			y0 = y1;
		}
		//hor next
		strName.Format(strFile,i);
		DivideRect(strName,CRect(dv.aPoint[0].x,0,dv.aPoint[dv.nPoint-1].x,ymax+1),&dv);
	}
	return TRUE;
}

HRESULT	cImageTga::CreateGrid(LPCSTR szFile,int width,int height,int wTile, int hTile)
{
	stImageInfo info;
	info.width = width;
	info.height = height;
	info.bytes_per_pixel = 4;
	int size = width * height * info.bytes_per_pixel;
	info.buffer = new unsigned char[size];
	ZeroMemory(info.buffer,size);
	//0xffff0000 red 0xff00ff00 green
	long cr1 = 0xffffff00;
	long cr = 0xff0000ff;
	long* pLong = (long*)info.buffer;
	for (int x=0; x<width; x++)
	{
		if (x % wTile == 0 || x == width - 1)
		{
			for (int y=0; y<height; y++)
			{
				if (x == 0 || x == width - 1)
					pLong[y*width+x] = cr1;
				else
					pLong[y*width+x] = cr;
			}
		}
	}
	for (int y=0; y<width; y++)
	{
		if (y % hTile == 0 || y == height - 1)
		{
			for (int x=0; x<height; x++)
			{
				if (y == 0 || y == height - 1)
					pLong[y*width+x] = cr1;
				else
					pLong[y*width+x] = cr;
			}
		}
	}
	SaveTGAFile(szFile,info);
	return S_OK;
}

HRESULT cImageTga::MakeSmallestImage(BOOL bRemoveShadow)
{
	CRect rc(m_info.width-1,m_info.height-1,0,0);
	ULONG* pLong = (ULONG* )m_info.buffer;

	unsigned char testAlpha = 0;
	if (bRemoveShadow)
		testAlpha = 254;
	for (int y=0; y<m_info.height; y++)
	for (int x=0; x<m_info.width; x++)
	{
		ULONG c = *pLong++;
		if (GetAlpha(c) > testAlpha)
		{
			rc.left = min(rc.left,x);
			rc.right = max(rc.right,x);
			rc.top = min(rc.top,y);
			rc.bottom = max(rc.bottom,y);
		}
	}
	rc.right ++;
	rc.bottom ++;
	if (rc == CRect(0,0,m_info.width,m_info.height))
		return S_OK;
	ASSERT(!rc.IsRectEmpty());
		
	ULONG* pBuffer = new ULONG[rc.Width()*rc.Height()];
	ULONG* pSrc = (ULONG* )m_info.buffer;
	ULONG* pDes = pBuffer;
	pSrc += rc.top * m_info.width + rc.left;
	for (int i=0; i<rc.Height(); i++)
	{
		memcpy(pDes,pSrc,rc.Width()*sizeof(ULONG));
		pDes += rc.Width();
		pSrc += m_info.width;
	}

	m_info.width = rc.Width();
	m_info.height = rc.Height();
	m_info.size = rc.Width() * rc.Height() * sizeof(ULONG);
	delete m_info.buffer;
	m_info.buffer = (unsigned char*)pBuffer;

	return S_OK;
}

HRESULT	cImageTga::RemoveShadow()
{
	return S_OK;
}
