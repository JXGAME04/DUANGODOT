// cImageBmp.cpp: implementation of the cImageBmp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "cImageBmp.h"
#include "fileop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cImageBmp::cImageBmp()
{
	m_info.format = IMAGE_888;
	m_info.bytes_per_pixel = 3;
	m_info.nDirections = 1;
	m_info.nFrames = 1;
	m_info.buffer = NULL;
	m_nFrameInfo = 0;
	for (int i=0; i<MAX_FRAME; i++)
	{
		m_aFrameInfo[i].buffer = NULL;
	}
}

cImageBmp::~cImageBmp()
{
	if (m_info.buffer)
		delete m_info.buffer;
	for (UINT i=0; i<m_nFrameInfo; i++)
	{
		if (m_aFrameInfo[i].buffer)
			delete m_aFrameInfo[i].buffer;
	}
}

//////////////////////////////////////////////////////////////////////
//Load and Save
//////////////////////////////////////////////////////////////////////
HRESULT cImageBmp::LoadFile(const char *filename) 
{
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	CFile file;
	VERIFY(AppOpenFile(file,filename,CFile::modeRead,"image"));
	file.Read(&bfh,sizeof(bfh));
	file.Read(&bi,sizeof(bi));
//	if (bi.biBitCount != 24)
//		return -1;
	if (bi.biBitCount == 24)
		m_info.format = IMAGE_888;
	else if (bi.biBitCount == 32)
		m_info.format = IMAGE_8888;
	else if (bi.biBitCount == 16)
		m_info.format = IMAGE_565;
	m_info.bytes_per_pixel = bi.biBitCount / 8;
	if (m_info.buffer)
		delete m_info.buffer;
	m_info.height = bi.biHeight;
	m_info.width = bi.biWidth;
	int hxw = m_info.height * m_info.width;
	m_info.size = hxw * m_info.bytes_per_pixel;
	m_info.buffer = new BYTE[m_info.size];
	file.Read(m_info.buffer,m_info.size);
	file.Close();

	bool top = false;
	if (!top)
	{
		unsigned char *swap = new unsigned char[m_info.size];
		memcpy(swap, m_info.buffer, m_info.size);
		BYTE * src, * dest;
		BYTE * src_data = m_info.buffer;
		for (unsigned int i = 0; i < m_info.height; i++)
		{
			src = &swap[(m_info.height - i - 1) * m_info.width * m_info.bytes_per_pixel];
			dest = &src_data[i * m_info.width * m_info.bytes_per_pixel];

			memcpy(dest, src, m_info.width * m_info.bytes_per_pixel );
		}

		delete swap;
	}

	m_info.nFrames = 1;
	return S_OK;
}

ULONG cImageBmp::AddImage(iImage* p) 
{
	ASSERT(0);
	return S_OK;
}

HRESULT SaveBMPFile(const char *filename,  const stImageInfo& m_info)
{
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bi;
	bi.biSize  = sizeof(BITMAPINFOHEADER);
	bi.biWidth  = m_info.width;
	bi.biHeight   = m_info.height;
	bi.biPlanes   = 1;
	bi.biBitCount  = m_info.bytes_per_pixel * 8;
	bi.biCompression = 0;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed  = 0;
	bi.biClrImportant = 0;

	bfh.bfType=0x4d42;
	bfh.bfOffBits=sizeof(bfh)+sizeof(BITMAPINFOHEADER);
	bfh.bfSize=bfh.bfOffBits+m_info.width*m_info.height*3;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;

	CFile file;
	if(!file.Open(filename,CFile::modeCreate|CFile::modeWrite))
		return -1;
	file.Write(&bfh,sizeof(bfh));
	file.Write(&bi,sizeof(bi));
	file.Write(m_info.buffer,m_info.width*m_info.height*3);
	file.Close();
	return S_OK;
}

HRESULT cImageBmp::CreateFile(const char *filename,  const stImageInfo& info)
{
	return ::SaveBMPFile(filename,info);
}

HRESULT cImageBmp::SaveFile(const char *filename, stImageInfo* info ) 
{
	ASSERT(0);
	return ::SaveBMPFile(filename,m_info);
}

BOOL cImageBmp::DivideTo(UINT wDivide,UINT hDivide)
{
	if (!(m_info.width % wDivide == 0 && m_info.height % hDivide == 0))
		return FALSE;
	if (m_info.width == wDivide  && m_info.height == hDivide)
		return TRUE;

	stFrameInfo info;
//	info.format = IMAGE_888;
//	info.bytes_per_pixel = m_info.bytes_per_pixel;
//	info.nDirections = 1;
//	info.nFrames = 1;
//	info.buffer = NULL;
	info.width =  wDivide;
	info.height = hDivide;
	info.size = info.width * info.height* m_info.bytes_per_pixel;

	unsigned char* pSrc;
	int ny = m_info.width / wDivide;
	int nx = m_info.height / hDivide;
	for (int y = 0; y < ny; y++ )
	for (int x = 0; x < nx; x++ )
	{
		pSrc = m_info.buffer + y * info.height * m_info.width * m_info.bytes_per_pixel + x * info.width * m_info.bytes_per_pixel;
		info.buffer = new unsigned char[info.size];
		unsigned char* pDes = info.buffer;
		for (UINT j=0; j < info.height; j++)
		{
			memcpy(pDes,pSrc,info.width * m_info.bytes_per_pixel);
			pDes += info.width * m_info.bytes_per_pixel;
			pSrc += m_info.width * m_info.bytes_per_pixel;
		}
		m_aFrameInfo[m_nFrameInfo++] = info;
	}

	m_info.nDirections = 1;
	m_info.nFrames = ny * nx;
	return TRUE;
}


void cImageBmp::SaveFileDivide(const char *filename, UINT wDivide, UINT hDivide, int& i)
{
	if (!(m_info.width % wDivide == 0 && m_info.height % hDivide == 0))
		return;
	if (m_info.width == wDivide  && m_info.height == hDivide)
		return;

	stImageInfo info;
	info.format = IMAGE_888;
	info.bytes_per_pixel = 3;
	info.nDirections = 1;
	info.nFrames = 1;
	info.buffer = NULL;
	info.width =  wDivide;
	info.height = hDivide;
	info.size = info.width * info.height* info.bytes_per_pixel;
	info.buffer = new unsigned char[info.size];

	unsigned char* pSrc;
	int ny = m_info.width / wDivide;
	int nx = m_info.height / hDivide;
	for (int y = ny-1; y >=0 ; y-- )
	for (int x = 0; x < nx; x++ )
	{
		pSrc = m_info.buffer + y * info.height * m_info.width * 3 + x * info.width * 3;
		unsigned char* pDes = info.buffer;
		for (UINT j=0; j < info.height; j++)
		{
			memcpy(pDes,pSrc,info.width * 3);
			pDes += info.width * 3;
			pSrc += m_info.width * 3;
		}
		char name[128];
		sprintf(name,filename,i++);
		::SaveBMPFile(name,info);
	}
}

//////////////////////////////////////////////////////////////////////
//info of a image
//////////////////////////////////////////////////////////////////////
void cImageBmp::GetImageInfo(stImageInfo& info) 
{
	info = m_info;
}

void cImageBmp::GetFrameInfo(stFrameInfo& info,ULONG nFrame ) 
{
	//ASSERT(nFrame == 0);
	if (m_info.nFrames == 1)
	{
		ASSERT(m_nFrameInfo == 0);
		info.buffer = m_info.buffer;
		info.width = m_info.width;
		info.height = m_info.height;
		info.x = m_info.x;
		info.y = m_info.y;
		info.size = m_info.size;
	}
	else
	{
		ASSERT(nFrame < m_nFrameInfo);
		info = m_aFrameInfo[nFrame];
	}
}
//////////////////////////////////////////////////////////////////////
//render to target
//////////////////////////////////////////////////////////////////////
HRESULT cImageBmp::Render(stImageRender& render) 
{
	//self is a 32 bit
	int n = m_info.bytes_per_pixel;
	if (render.rcSrc.right >= m_info.width)
	{
		render.rcSrc.right = m_info.width - 1;
	}
	if (render.rcSrc.bottom  >= m_info.height)
	{
		render.rcSrc.bottom = m_info.height - 1;
	}

	if (n == 2)
	{
		if (render.format == IMAGE_565)
		{
			BYTE* pDes = (BYTE*)render.buffer;
			BYTE* pSrc = (BYTE*)m_info.buffer;
			int nPitch = m_info.width * n;
			pDes += render.ptDes.y * render.nPitch + (render.ptDes.x * 2);
			pSrc += render.rcSrc.top * nPitch + render.rcSrc.left * n;
			for (int j=0; j<render.rcSrc.Height(); j++)
			{
				memcpy(pDes,pSrc,render.rcSrc.Width() * 2);
				pDes += render.nPitch;
				pSrc += nPitch;
			}
		}
	}
	else if (n == 4)
	{
		if (render.format == IMAGE_565)
		{
			BYTE* pDes = (BYTE*)render.buffer;
			BYTE* pSrc = (BYTE*)m_info.buffer;
			int nPitch = m_info.width * n;
			pDes += render.ptDes.y * render.nPitch + (render.ptDes.x * 2);
			pSrc += render.rcSrc.top * nPitch + render.rcSrc.left * n;
			for (int j=0; j<render.rcSrc.Height(); j++)
			{
				WORD* pD = (WORD*)pDes;
				BYTE* pS = (BYTE*)pSrc;
				for (int i=0; i<render.rcSrc.Width(); i++)
				{
					WORD b = *pS++;
					WORD g = *pS++;
					WORD r = *pS++;
					WORD a = *pS++;
					*pD++ = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
	//				*pD++ = 0xff00;
				}
				pDes += render.nPitch;
				pSrc += nPitch;
			}
		}
	}
	else if (n == 3)
	{
		if (render.format == IMAGE_565)
		{
			BYTE* pDes = (BYTE*)render.buffer;
			BYTE* pSrc = (BYTE*)m_info.buffer;
			int nPitch = m_info.width * n;
			pDes += render.ptDes.y * render.nPitch + (render.ptDes.x * 2);
			pSrc += render.rcSrc.top * nPitch + render.rcSrc.left * n;
			for (int j=0; j<render.rcSrc.Height(); j++)
			{
				WORD* pD = (WORD*)pDes;
				BYTE* pS = (BYTE*)pSrc;
				for (int i=0; i<render.rcSrc.Width(); i++)
				{
					WORD b = *pS++;
					WORD g = *pS++;
					WORD r = *pS++;
					*pD++ = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
	//				*pD++ = 0xff00;
				}
				pDes += render.nPitch;
				pSrc += nPitch;
			}
		}
	}
	return S_OK;
}

CBitmap* cImageBmp::ImageToBitmap(ULONG nFrame,int nDepth) 
{
	ASSERT(m_info.buffer);

	CBitmap* pBmp = new CBitmap;
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
	return pBmp;
}
