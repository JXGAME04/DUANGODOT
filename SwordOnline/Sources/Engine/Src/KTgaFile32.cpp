//---------------------------------------------------------------------------
// Sword3 Engine (c) 1999-2000 by Kingsoft
//
// File:	KTgaFile32.cpp
// Date:	2000.08.08
// Code:	Daniel Wang
// Desc:	24bit TGA file class
//---------------------------------------------------------------------------
#include "KWin32.h"
#include "KDebug.h"
#include "KMemClass.h"
#include "KColors.h"
#include "KPakFile.h"
#include "KTgaFile32.h"


tImageTGA *LoadTGA(const char *filename)
{
	tImageTGA *pImageData = NULL;		// This stores our important image data
	WORD width = 0, height = 0;			// The dimensions of the image
	BYTE length = 0;					// The length in bytes to the pixels
	BYTE imageType = 0;					// The image type (RLE, RGB, Alpha...)
	BYTE bits = 0;						// The bits per pixel for the image (16, 24, 32)
	int channels = 0;					// The channels of the image (3 = RGA : 4 = RGBA)
	int stride = 0;						// The stride (channels * width)
	int i = 0;							// A counter
		
	// This function loads in a TARGA (.TGA) file and returns its data to be
	// used as a texture or what have you.  This currently loads in a 16, 24
	// and 32-bit targa file, along with RLE compressed files.  Eventually you
	// will want to do more error checking to make it more robust.  This is
	// also a perfect start to go into a modular class for an engine.
	// Basically, how it works is, you read in the header information, then
	// move your file pointer to the pixel data.  Before reading in the pixel
	// data, we check to see the if it's an RLE compressed image.  This is because
	// we will handle it different.  If it isn't compressed, then we need another
	// check to see if we need to convert it from 16-bit to 24 bit.  24-bit and
	// 32-bit textures are very similar, so there's no need to do anything special.
	// We do, however, read in an extra bit for each color.

	// Open a file pointer to the targa file and check if it was found and opened 
	KPakFile			File;
	if (!File.Open(filename))
		return NULL;
	// Allocate the structure that will hold our eventual image data (must free it!)
	pImageData = new tImageTGA;//(tImageTGA*)malloc(sizeof(tImageTGA));
	// Read in the length in bytes from the header to the pixel data
	File.Read(&length, sizeof(BYTE));
	// Jump over one BYTE
	File.Seek(1, FILE_CURRENT);
	// Read in the imageType (RLE, RGB, etc...)
	File.Read(&imageType, sizeof(BYTE));
	// Skip past general information we don't care about
	File.Seek(9, FILE_CURRENT); 

	// Read the width, height and bits per pixel (16, 24 or 32)
	File.Read(&width,  sizeof(WORD));
	File.Read(&height, sizeof(WORD));
	File.Read(&bits,   sizeof(BYTE));
	
	// Now we move the file pointer to the pixel data
	File.Seek(length + 1, FILE_CURRENT); 

	// Check if the image is RLE compressed or not
	if(imageType != TGA_RLE)
	{
		// Check if the image is a 24 or 32-bit image
		if(bits == 24 || bits == 32)
		{
			// Calculate the channels (3 or 4) - (use bits >> 3 for more speed).
			// Next, we calculate the stride and allocate enough memory for the pixels.
			channels = bits / 8;
			stride = channels * width;
			pImageData->data = new unsigned char[stride * height];

			// Load in all the pixel data line by line
			for(int y = 0; y < height; y++)
			{
				// Store a pointer to the current line of pixels
				unsigned char *pLine = &(pImageData->data[stride * y]);

				// Read in the current line of pixels
				File.Read(pLine, stride);
			
				// Go through all of the pixels and swap the B and R values since TGA
				// files are stored as BGR instead of RGB (or use GL_BGR_EXT verses GL_RGB)
				for(i = 0; i < stride; i += channels)
				{
					int temp     = pLine[i];
					pLine[i]     = pLine[i + 2];
					pLine[i + 2] = (unsigned char)temp;
				}
			}
		}
		// Check if the image is a 16 bit image (RGB stored in 1 unsigned short)
		else if(bits == 16)
		{
			unsigned short pixels = 0;
			int r=0, g=0, b=0;

			// Since we convert 16-bit images to 24 bit, we hardcode the channels to 3.
			// We then calculate the stride and allocate memory for the pixels.
			channels = 3;
			stride = channels * width;
			pImageData->data = new unsigned char[stride * height];

			// Load in all the pixel data pixel by pixel
			for(int i = 0; i < width*height; i++)
			{
				// Read in the current pixel
				File.Read(&pixels, sizeof(unsigned short));
				
				// To convert a 16-bit pixel into an R, G, B, we need to
				// do some masking and such to isolate each color value.
				// 0x1f = 11111 in binary, so since 5 bits are reserved in
				// each unsigned short for the R, G and B, we bit shift and mask
				// to find each value.  We then bit shift up by 3 to get the full color.
				b = (pixels & 0x1f) << 3;
				g = ((pixels >> 5) & 0x1f) << 3;
				r = ((pixels >> 10) & 0x1f) << 3;
				
				// This essentially assigns the color to our array and swaps the
				// B and R values at the same time.
				pImageData->data[i * 3 + 0] = (unsigned char)r;
				pImageData->data[i * 3 + 1] = (unsigned char)g;
				pImageData->data[i * 3 + 2] = (unsigned char)b;
			}
		}	
		// Else return a NULL for a bad or unsupported pixel format
		else
		{
			delete pImageData;
			pImageData = NULL;
			File.Close();
			return NULL;
		}
	}
	else if (bits == 16 && imageType == TGA_RLE)
	{
		channels = 3;                       // 閜 th鄋h 24-bit
		stride   = channels * width;
		pImageData->data = new unsigned char[stride * height];
	
		BYTE r, g, b;
		BYTE rleID   = 0;
		int  iPixel  = 0;
		while (iPixel < width * height)
		{
			File.Read(&rleID, 1);
			int count = (rleID & 0x7F) + 1;
	
			if (rleID < 128)                   // raw packet
			{
				while (count--)
				{
					unsigned short px;
					File.Read(&px, 2);
	
					b =  (px & 0x1F) << 3;
					g = ((px >> 5)  & 0x1F) << 3;
					r = ((px >> 10) & 0x1F) << 3;
	
					unsigned char* dst = &pImageData->data[iPixel * 3];
					dst[0] = r; dst[1] = g; dst[2] = b;
					++iPixel;
				}
			}
			else                                 // RLE packet
			{
				unsigned short px;
				File.Read(&px, 2);
	
				b =  (px & 0x1F) << 3;
				g = ((px >> 5)  & 0x1F) << 3;
				r = ((px >> 10) & 0x1F) << 3;
	
				while (count--)
				{
					unsigned char* dst = &pImageData->data[iPixel * 3];
					dst[0] = r; dst[1] = g; dst[2] = b;
					++iPixel;
				}
			}
		}
	}
	// Else, it must be Run-Length Encoded (RLE)
	else
	{
		// First, let me explain real quickly what RLE is.  
		// For further information, check out Paul Bourke's intro article at: 
		// http://astronomy.swin.edu.au/~pbourke/dataformats/rle/
		// 
		// Anyway, we know that RLE is a basic type compression.  It takes
		// colors that are next to each other and then shrinks that info down
		// into the color and a integer that tells how much of that color is used.
		// For instance:
		// aaaaabbcccccccc would turn into a5b2c8
		// Well, that's fine and dandy and all, but how is it down with RGB colors?
		// Simple, you read in an color count (rleID), and if that number is less than 128,
		// it does NOT have any optimization for those colors, so we just read the next
		// pixels normally.  Say, the color count was 28, we read in 28 colors like normal.
		// If the color count is over 128, that means that the next color is optimized and
		// we want to read in the same pixel color for a count of (colorCount - 127).
		// It's 127 because we add 1 to the color count, as you'll notice in the code.

		// Create some variables to hold the rleID, current colors read, channels, & stride.
		BYTE rleID = 0;
		int colorsRead = 0;
		channels = bits / 8;
		stride = channels * width;

		// Next we want to allocate the memory for the pixels and create an array,
		// depending on the channel count, to read in for each pixel.
		pImageData->data = new unsigned char[stride * height];
		BYTE *pColors = new BYTE [channels];

		// Load in all the pixel data
		while(i < width*height)
		{
			// Read in the current color count + 1
			File.Read(&rleID, sizeof(BYTE));
			
			// Check if we don't have an encoded string of colors
			if(rleID < 128)
			{
				// Increase the count by 1
				rleID++;

				// Go through and read all the unique colors found
				while(rleID)
				{
					// Read in the current color
					File.Read(pColors, sizeof(BYTE) * channels);

					// Store the current pixel in our image array
					pImageData->data[colorsRead + 0] = pColors[2];
					pImageData->data[colorsRead + 1] = pColors[1];
					pImageData->data[colorsRead + 2] = pColors[0];

					// If we have a 4 channel 32-bit image, assign one more for the alpha
					if(bits == 32)
						pImageData->data[colorsRead + 3] = pColors[3];

					// Increase the current pixels read, decrease the amount
					// of pixels left, and increase the starting index for the next pixel.
					i++;
					//ASSERT(i<=width*height);
					rleID--;
					colorsRead += channels;
				}
			}
			// Else, let's read in a string of the same character
			else
			{
				// Minus the 128 ID + 1 (127) to get the color count that needs to be read
				rleID -= 127;

				// Read in the current color, which is the same for a while
				File.Read(pColors, sizeof(BYTE) * channels);

				// Go and read as many pixels as are the same
				while(rleID)
				{
					// Assign the current pixel to the current index in our pixel array
					pImageData->data[colorsRead + 0] = pColors[2];
					pImageData->data[colorsRead + 1] = pColors[1];
					pImageData->data[colorsRead + 2] = pColors[0];

					// If we have a 4 channel 32-bit image, assign one more for the alpha
					if(bits == 32)
						pImageData->data[colorsRead + 3] = pColors[3];

					// Increase the current pixels read, decrease the amount
					// of pixels left, and increase the starting index for the next pixel.
					i++;
					//ASSERT(i<=width*height);
					rleID--;
					colorsRead += channels;
				}
				
			}
				
		}

		// Free up pColors
		delete[] pColors;
	}

	// Fill in our tImageTGA structure to pass back
	pImageData->channels = channels;
	pImageData->sizeX    = width;
	pImageData->sizeY    = height;

	// Return the TGA data (remember, you must free this data after you are done)
	return pImageData;
}

//---------------------------------------------------------------------------
// 函数:	Load
// 功能:	打开TGA文科
// 参数:	lpFileName	文件名
// 返回:	TRUE		成功
//			FALSE		失败
//---------------------------------------------------------------------------
BOOL KTgaFile32::Load2Buffer(LPCSTR lpFileName)
{
	int					i, j;
	TGAFILEHEADER		FileHeader;
	KPakFile			File;
	KMemClass			Buffer;
	
	// open the file
	if (!File.Open(lpFileName))
		return FALSE;

	// read file head
	File.Read(&FileHeader, sizeof(FileHeader));

	if (FileHeader.ColorMapType)
	{
		if((FileHeader.ImageType != 2 && FileHeader.ImageType != 10) 
			|| FileHeader.ColorMapSpec[4] != 32)
		{
			return FALSE;	// 非32位色的TGA或不是TGA
		}
	}

	if (FileHeader.PixelDep != 32 && FileHeader.PixelDep != 24)
	{
		return FALSE;	// 非32位TGA
	}

	if(FileHeader.IDLength)
	{
		File.Seek(FileHeader.IDLength, FILE_CURRENT);
	}

	if(FileHeader.ColorMapType)
	{
		i= (FileHeader.ColorMapSpec[4] + 1) / 8 * (*(WORD*)(&FileHeader.ColorMapSpec[2]));
		File.Seek(i,FILE_CURRENT);
	}
	i=FileHeader.Width*FileHeader.Height*4;
	
	Buffer.Alloc(i);
	if(FileHeader.ImageType == 2)
	{
		if(FileHeader.PixelDep == 32)
		{
			int nReadSize = File.Read(Buffer.GetMemPtr(), i);
			if (nReadSize != i)
				return FALSE;	// 读的文件有错
		}
		else
		{
			BYTE* pPtr = (BYTE*)Buffer.GetMemPtr();
			BYTE btRead[3];
			for(UINT k=0;k < (UINT)FileHeader.Width*FileHeader.Height;k++)
			{
				if(File.Read(&btRead[0], sizeof(BYTE)*3) != sizeof(BYTE)*3)
					return FALSE;
				pPtr[0] = btRead[0];
				pPtr[1] = btRead[1];
				pPtr[2] = btRead[2];
				pPtr[3] = 0xff;
				pPtr += 4;
			}
		}
	}
	else
	{
		if(FileHeader.PixelDep == 24)
			return FALSE;
		BYTE	TempData[4] ;
		int		TempState =0, TempCount =0 ;
		BYTE	*ppData ;
		ppData = (BYTE*)Buffer.GetMemPtr();
		do
		{
			j = File.Read( TempData, (TempState==0)?1:4 );
			if( j != ((TempState==0)?1:4) )
			{
				return FALSE;// 文件有错
			}
			
			if( !TempState )
			{
				TempState = (TempData[0] & 0x80)?1:2 ;
				TempCount = TempData[0] & 0x7F ;
			}
			else
			{
				if( TempState == 1 )	// RLE 压缩
				{
					for(; TempCount>=0 ; TempCount -- )
					{
						*(ppData++) = TempData[0];
						*(ppData++) = TempData[1];
						*(ppData++) = TempData[2];
						*(ppData++) = TempData[3];
					}
				}
				else		// NON-RLE 压缩
				{
					*(ppData++) = TempData[0];
					*(ppData++) = TempData[1];
					*(ppData++) = TempData[2];
					*(ppData++) = TempData[3];
					TempCount -- ;
				}
				
				if( TempCount < 0 )	
				{
					TempCount = 0 ;
					TempState = 0 ;
				}
			}
		} while( ppData < (BYTE*)Buffer.GetMemPtr() + i ) ;
	}
	m_nWidth	= FileHeader.Width;
	m_nHeight	= FileHeader.Height;

	// 把TGA格式的Buffer转为标准格式（点为：B G R A）
	BYTE	*pTemp1, *pTemp2, *pTmp1;
	m_Buffer.Alloc(m_nWidth * m_nHeight * 4);

	pTemp1 = (BYTE*)Buffer.GetMemPtr();
	pTemp2 = (BYTE*)m_Buffer.GetMemPtr();

	int flag = (FileHeader.Desc&0x30)>>4;
	switch(flag)
	{
	case 0:	// 每行本身是顺序，行与行是倒的
		pTemp1 += m_nWidth * m_nHeight * 4;
		for (j = 0; j < m_nHeight; j++)
		{
			pTemp1 -= m_nWidth * 4;
			pTmp1 = pTemp1;
			for (i = 0; i < m_nWidth; i++)
			{
				pTemp2[0]	= pTmp1[0];
				pTemp2[1]	= pTmp1[1];
				pTemp2[2]	= pTmp1[2];
				pTemp2[3]	= pTmp1[3];
				pTmp1	+= 4;
				pTemp2	+= 4;
			}
		}
		break;
	case 1:	// 所有点都是倒的
		pTemp1 += m_nWidth * m_nHeight * 4;
		for (i = 0; i < m_nWidth * m_nHeight; i++)
		{
			pTemp1 -= 4;
			pTemp2[0]	= pTemp1[0];
			pTemp2[1]	= pTemp1[1];
			pTemp2[2]	= pTemp1[2];
			pTemp2[3]	= pTemp1[3];
			pTemp2 += 4;
		}
		break;
	case 2:	// 所有点都是正的
		for (i = 0; i < m_nWidth * m_nHeight; i++)
		{
			pTemp2[0]	= pTemp1[0];
			pTemp2[1]	= pTemp1[1];
			pTemp2[2]	= pTemp1[2];
			pTemp2[3]	= pTemp1[3];
			pTemp1	+= 4;
			pTemp2	+= 4;
		}
		break;
	case 3:	// 行与行是正的，每行本身的点是倒的
		for (j = 0; j < m_nHeight; j++)
		{
			pTemp1 += m_nWidth * 4;
			pTmp1 = pTemp1;
			for (i = 0; i < m_nWidth; i++)
			{
				pTmp1 -= 4;
				pTemp2[0]	= pTmp1[0];
				pTemp2[1]	= pTmp1[1];
				pTemp2[2]	= pTmp1[2];
				pTemp2[3]	= pTmp1[3];
				pTemp2 += 4;
			}
		}
		break;
	default:
		break;
	}
	return TRUE;
}