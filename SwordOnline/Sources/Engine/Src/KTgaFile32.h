//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KBmpFile24.h
// Date:	2000.08.08
// Code:	Daniel Wang
// Desc:	Header File
//---------------------------------------------------------------------------
#ifndef KTgaFile32_H
#define KTgaFile32_H
//---------------------------------------------------------------------------
#include "KMemClass.h"

#define TGA_RGB		 2		// This tells us it's a normal RGB (really BGR) file
#define TGA_A		 3		// This tells us it's a ALPHA file
#define TGA_RLE		10		// This tells us that the targa is Run-Length Encoded (RLE)

struct tImageTGA
{
	int channels;			// The channels in the image (3 = RGB : 4 = RGBA)
	int sizeX;				// The width of the image in pixels
	int sizeY;				// The height of the image in pixels
	unsigned char *data;	// The image pixel data
	tImageTGA()
	{
		data = NULL;
	}
	~tImageTGA()
	{
		if(data)
		{
			delete [] data;
			data = NULL;
		}
	}
};

tImageTGA *LoadTGA(const char *filename);

typedef struct
{
	BYTE		IDLength;					// ID length
	BYTE		ColorMapType;				// Color map type
	BYTE		ImageType;					// Image type
	BYTE		ColorMapSpec[5];			// Color map specification
	WORD		X,Y;							// Image specification
	WORD		Width,Height;
	BYTE		PixelDep;
	BYTE		Desc;
} TGAFILEHEADER;

typedef struct
{
	short		Size;						// Extension size
	char		AuthorName[41];				// Author name
	char		AuthorCmts[324];			// Author comments
	short		DateTimeStamp[6];			// Date time stamp
	char		JobID[41];					// Job name/ID
	short		JobTime[3];					// Job time
	char		SoftwareID[41];				// Software ID
	BYTE		SoftWareVer[3];				// Software Version
	int			KeyColor;					// Key color
	short		PixelRatio[2];				// Pixel aspect ratio
	short		GammaVal[2];				// Gamma value
	int			ClrCorrOft;					// Color correction offset
	int			PostageStampOft;			// Postage stamp offset
	int			ScanLineOft;				// Scan line offset
	BYTE		AttribType;					// Attribute type
} TGAEXTAREAHEADER;

//---------------------------------------------------------------------------
class ENGINE_API KTgaFile32
{
public:
	KMemClass	m_Buffer;
private:
	int		m_nWidth;
	int		m_nHeight;

public:
	int		GetWidth()	{ return m_nWidth; };
	int		GetHeight() { return m_nHeight; };
	BOOL	Load2Buffer(LPCSTR lpFileName);
};
//---------------------------------------------------------------------------
#endif
