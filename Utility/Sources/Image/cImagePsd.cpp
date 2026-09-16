// cImagePsd.cpp: implementation of the cImagePsd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "cImagePsd.h"
#include "cImageTga.h"
#include <list>
#include <vector>
#include "psd.h"
#include <fstream>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cImagePsd::cImagePsd()
{
	for (int i=0; i<MAX_LAYER; i++)
		m_psdData[i].buffer = NULL;
}

cImagePsd::~cImagePsd()
{
	for (int i=0; i<MAX_LAYER; i++)
	{
		if (m_psdData[i].buffer)
		{
			delete m_psdData[i].buffer;
			m_psdData[i].buffer = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//Load and Save
//////////////////////////////////////////////////////////////////////
struct ExampleSurface 
{
    typedef BYTE pixel_type;
    typedef pixel_type * pixel_iterator;
	
    SIZE m_Size;
    pixel_iterator m_Storage;
	
    int Width() { return m_Size.cx; }
    int Height() { return m_Size.cy; }
	
    pixel_iterator Iterator( const int x = 0, const int y = 0 ) 
	{
        if ( !m_Storage ) return 0;
        return m_Storage + (y * m_Size.cx) + x;
    }
	
    void Destroy() 
	{
        if ( m_Storage ) 
		{
            delete [] m_Storage;
            m_Storage = 0;
        }
        m_Size.cx = m_Size.cy = 0;
    }
	
    bool Create( const int width, const int height ) 
	{
        Destroy();		
        m_Storage = new pixel_type [ width * height ];
        m_Size.cx = width; m_Size.cy = height;
        return (0 != m_Storage);
    }
	
    ~ExampleSurface() { Destroy(); }
	
    ExampleSurface() : m_Storage(0) { m_Size.cx = m_Size.cy = 0; }	

	void TrimTo(CRect r)
	{
		pixel_iterator p = new pixel_type [r.Width() * r.Height()];
		for (int y=0; y<r.Height(); y++)
		for (int x=0; x<r.Width(); x++)
		{
			p[y * r.Width() + x] = m_Storage[(y + r.top) * m_Size.cx + (x + r.left)];
		}
		Destroy();
		m_Storage = p;
		m_Size.cx = r.Width();
		m_Size.cy = r.Height();
	}

	CRect Trim()
	{
		pixel_type c = *m_Storage;
		
		CRect r(m_Size.cx,m_Size.cy,0,0);
		for (int y = 0; y < m_Size.cy; y++)
		for (int x = 0; x < m_Size.cx; x++)
		{
			if (m_Storage[y*m_Size.cx + x] != c)
			{
				if (x<r.left)
					r.left = x;
				if (x>r.right)
					r.right = x;
				if (y<r.top)
					r.top = y;
				if (y>r.bottom)
					r.bottom = y;
			}
		}
		if (r.Width() == m_Size.cx &&  r.Height() == m_Size.cy)
			return r;
		TrimTo(r);
		return r;
	}
};

HRESULT cImagePsd::LoadFile(const char *pszFilename) 
{
	typedef BPT::TPSDFileLoader<ExampleSurface> loader_type;
	
	typedef loader_type::channel_collection_type::iterator channel_iterator;
	
	loader_type psdLoader;
	
	loader_type::channel_collection_type * pChannels = psdLoader.Load( pszFilename );

#ifdef _DEBUG_PSD
	CString s = pszFilename;
	s = s + ".txt";
	ofstream outfile(s);
	if (!outfile.is_open())
		return 0;
#endif

	m_nLayer = 0;

//	#define outfile std::cout
	CString sSign1[] = 
	{
		"Red",
		"Green",
		"Blue",
		"Whole Alpha",
		"Alpha",
		"Shadow",
		""
	};
	CString sSign2[] = 
	{
		"Red",
		"Green",
		"Blue",
		"Alpha",
		"Shadow",
		""
	};
	CString* sSign;
	if (pChannels->size() == 5)
	{
		sSign = sSign2;
		m_nLayer = 2;
	}
	else if (pChannels->size() == 6)	
	{
		sSign = sSign1;
		m_nLayer = 2;
	}
	else if (pChannels->size() == 4)
	{
		CString strMsg ;
		strMsg.Format("file: %s, 没有阴影的请用tga格式!",pszFilename);
		AfxMessageBox(strMsg);
		sSign = sSign2;
		m_nLayer = 1;
	}
	else
	{
		CString strMsg ;
		strMsg.Format("file: %s, channel error!",pszFilename);
		AfxMessageBox(strMsg);
		return 0;
	}

	if ( pChannels )
	{
		int channelCount = pChannels->size();		
#ifdef _DEBUG_PSD
		outfile << "There are " << channelCount << "channels.\n" << std::endl;		
#endif

		int channel = 0;		
		CRect rect;
		for ( channel_iterator it = pChannels->begin(); it != pChannels->end(); it++,channel++) 
		{
			if (sSign[channel].IsEmpty())
				break;
			ExampleSurface * pSurface = *it;			
			int i = -1;
			if (sSign[channel] == "Alpha")
			{
				i = 0;
				rect = pSurface->Trim();
			}
			if (sSign[channel] == "Shadow")
			{
				i = 1;
				pSurface->Trim();
			}
			if (i == -1)
				continue;

			ExampleSurface::pixel_iterator pPixel = pSurface->Iterator();

			m_psdData[i].bytes_per_pixel = 4;
			m_psdData[i].width = pSurface->Width();
			m_psdData[i].height = pSurface->Height();
			int hxw = m_psdData[i].width * m_psdData[i].height;
			m_psdData[i].buffer =(unsigned char*) new unsigned long[hxw];
			BYTE* pChar = m_psdData[i].buffer;
			for (int j=0; j<hxw; j++)
			{
				BYTE c = *pPixel++;
				*pChar++ = 0;
				*pChar++ = 0;
				*pChar++ = 0;
				*pChar++ = c;
			}
		}

		channel = 0;		
		for ( it = pChannels->begin(); it != pChannels->end(); it++,channel++ ) 
		{			
			if (sSign[channel].IsEmpty())
				break;

			ExampleSurface * pSurface = *it;			

			BYTE* pChar = m_psdData[0].buffer;

			if (sSign[channel] == "Red")
			{
				pSurface->TrimTo(rect);
				ExampleSurface::pixel_iterator pPixel = pSurface->Iterator();
				int hxw = m_psdData[0].width * m_psdData[0].height;
				for (int j=0; j<hxw; j++)
				{
					BYTE c = *pPixel++;
					pChar++ ;
					pChar++ ;
					*pChar++ = c;
					pChar++ ;
				}
			}
			else if (sSign[channel] == "Green")
			{
				pSurface->TrimTo(rect);
				ExampleSurface::pixel_iterator pPixel = pSurface->Iterator();
				int hxw = m_psdData[0].width * m_psdData[0].height;
				for (int j=0; j<hxw; j++)
				{
					BYTE c = *pPixel++;
					pChar++ ;
					*pChar++ = c;
					pChar++ ;
					pChar++ ;
				}
			}
			else if (sSign[channel] == "Blue")
			{
				pSurface->TrimTo(rect);
				ExampleSurface::pixel_iterator pPixel = pSurface->Iterator();
				int hxw = m_psdData[0].width * m_psdData[0].height;
				for (int j=0; j<hxw; j++)
				{
					BYTE c = *pPixel++;
					*pChar++ = c;
					pChar++ ;
					pChar++ ;
					pChar++ ;
				}
			}		
		}

		psdLoader.Unload( pChannels );
	}

	return S_OK;
}

ULONG cImagePsd::AddImage(iImage* p) 
{
	ASSERT(0);
	return S_OK;
}

HRESULT cImagePsd::SaveFile(const char *filename, stImageInfo* info ) 
{
//	ASSERT(0);
	SavePSDToTGA(filename);		
	return S_OK;
}
//////////////////////////////////////////////////////////////////////
//info of a image
//////////////////////////////////////////////////////////////////////
void cImagePsd::GetImageInfo(stImageInfo& info) 
{
}

void cImagePsd::GetFrameInfo(stFrameInfo& info,ULONG nFrame ) 
{
}
//////////////////////////////////////////////////////////////////////
//render to target
//////////////////////////////////////////////////////////////////////
HRESULT cImagePsd::Render(stImageRender& render) 
{
	ASSERT(0);
	return S_OK;
}

CBitmap* cImagePsd::ImageToBitmap(ULONG nFrame,int nDepth) 
{
	ASSERT(0);
	return NULL;
}

void cImagePsd::SaveImageInfoToTGA(const char *szFile, const stImageInfo& info)
{
	SaveTGAFile(szFile,info);
}

void cImagePsd::SavePSDToTGA(const char *szFolder)
{
	for (int i=0; i<m_nLayer; i++)
	{
		CString sName;
		sName.Format("%d.tgb",i);
		CString sPathName = szFolder;
		sPathName += "\\";
		sPathName += sName;

		SaveImageInfoToTGA(sPathName,m_psdData[i]);
	}
}
