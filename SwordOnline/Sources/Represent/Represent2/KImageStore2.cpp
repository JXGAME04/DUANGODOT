/*****************************************************************************************
//  ÎÞÌùÍ¼µÄÍ¼ÐÎ×ÊÔ´¹ÜÀí
//	Copyright : Kingsoft 2002-2003
//	Author	: Wooy(Wu yue)
//	CreateTime:	2002-11-11
*****************************************************************************************/
#include "../../Engine/Src/KEngine.h"
#include "KImageStore2.h"
#include "../iRepresent/KRepresentUnit.h"
#include <crtdbg.h.>
#include "../../Engine/Src/KColors.h"

unsigned int ImageNameToId(const char* pszName)
{
	unsigned int uId = 0;
	if (pszName && pszName[0])
	{
		for (int i = 1; (*pszName); pszName++,i++)
		{
			if ((*pszName) < 'A' || (*pszName) > 'Z')
				uId = (uId + i * (*pszName)) % 0x8000000b * 0xffffffef;
			else						//ÖÐÎÄ×Ö·ûµÄºó×Ö½ÚÒà¿ÉÄÜ±»×ª»»£¬ÈËÎªµØ¼Ó´óÁËÖØÂëµÄ¸ÅÂÊ¡ö
				uId = (uId + i * ((*pszName) + 0x20)) % 0x8000000b * 0xffffffef;
		}
		uId ^= 0x12345678;
	}
	return uId;
}

char* KImageStore2::GetAdjustColorPalette(int nISPosition, unsigned uColor)
{
	char* pPalette = NULL;
	if (nISPosition >= 0 && nISPosition < m_nNumImages)
	{
		_KISImageObj& Img = m_pObjectList[nISPosition];
		if (Img.pObject && Img.bType == ISI_T_SPR)
		{
			if (Img.pcAdjustColorPalettes == NULL)
			{
				char* pOrigPalette = ((char*)Img.pObject) + sizeof(SPRHEAD);
				Img.pcAdjustColorPalettes = CreateAdjustColorPalette(
					pOrigPalette, ((SPRHEAD*)Img.pObject)->Colors);
			}
            if (Img.pcAdjustColorPalettes)
			{
				for (unsigned int i = 0; i < m_uNumSprAdustColor; i ++)
				{
					if ((uColor & 0xffffff) == m_uSprAdjustColorList[i])
					{
						pPalette = Img.pcAdjustColorPalettes + (2 * i * ((SPRHEAD*)Img.pObject)->Colors);
						break;
					}
				}
			}
			if (pPalette == NULL)
				pPalette = ((char*)Img.pObject) + sizeof(SPRHEAD);
		}
	}
	return pPalette;
}

char* KImageStore2::CreateAdjustColorPalette(const char* pOrigPalette, int nNumColor)
{
	char* pPalette = NULL;
	if (pOrigPalette && nNumColor > 0 && m_uNumSprAdustColor)
	{
		_ASSERT(nNumColor <= 256);
		pPalette = (char*)malloc(2 * nNumColor * m_uNumSprAdustColor);
		if (pPalette)
		{
			KPAL24	OrigPalette[256];
			g_Pal16ToPal24((KPAL16*)pOrigPalette, OrigPalette, nNumColor);

			unsigned short*	pusACP = (unsigned short*)pPalette;
			for (unsigned int i = 0; i < m_uNumSprAdustColor; i++)
			{
				unsigned int	uAR = (m_uSprAdjustColorList[i] >> 16) & 0xff;
				unsigned int	uAG = (m_uSprAdjustColorList[i] >> 8) & 0xff;
				unsigned int	uAB = m_uSprAdjustColorList[i] & 0xff;
				for (int nIndex = 0; nIndex < nNumColor; nIndex++)
				{
					unsigned int uR = uAR * OrigPalette[nIndex].Red / 256;
					unsigned int uG = uAG * OrigPalette[nIndex].Green / 256;
					unsigned int uB = uAB * OrigPalette[nIndex].Blue / 256;
					*(pusACP++) = g_RGB(uR, uG, uB);
				}
			}
		}
	}
	return pPalette;
}

//ÉèÖÃÆ«É«ÁÐ±í
unsigned int KImageStore2::SetAdjustColorList(unsigned int* puColorList, unsigned int uCount)
{
	if (uCount > MAX_ADJUSTABLE_COLOR_NUM)
		uCount = MAX_ADJUSTABLE_COLOR_NUM;
	ClearAllAdjustColorPalette();
	if (puColorList && uCount)
	{
		memcpy(m_uSprAdjustColorList, puColorList, sizeof(unsigned int) * uCount);
		for (unsigned int i = 0; i < uCount; i++)
		{
			m_uSprAdjustColorList[i] &= 0xffffff;
		}
		m_uNumSprAdustColor = uCount;
	}
	else
	{
		m_uNumSprAdustColor = 0;
	}
	return m_uNumSprAdustColor;
}

unsigned int KImageStore2::CreateImage(const char* pszName, int nWidth, int nHeight, int nType, bool bNodeCache, bool bScrMode)
{
	unsigned int uImage = ImageNameToId(pszName);
	KSGImageContent* pBitmap = NULL;
	AlphaRecContent* pImg = NULL;
	int nIdx;


    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	if (nWidth > 0 && nHeight > 0 && uImage && (nType == ISI_T_BITMAP16 || nType == ISI_T_DRAWINGRC))
	{
		if ((nIdx = FindImage(uImage, 0)) < 0 &&	//±ØÐëÊÇ²»´æÔÚÍ¬idµÄÍ¼ÐÎ
			(m_nNumImages < m_nNumReserved || ExpandSpace()))	//ÓÐ¿Õ¼ä´æ·ÅÍ¼ÐÎ¶ÔÏó
		{
			if(nType == ISI_T_BITMAP16)
				pBitmap = (KSGImageContent *)malloc(KSG_IMAGE_CONTENT_SIZE(nWidth, nHeight));
			else
				pImg = (AlphaRecContent *)malloc(CREATE_REC_CONTENT_SIZE(nWidth, nHeight));
		}
	}
	
	if (pBitmap)
	{
		pBitmap->nWidth = nWidth;
		pBitmap->nHeight = nHeight;

		nIdx = - nIdx - 1;
		for (int i = m_nNumImages; i > nIdx; i--)
		{
			m_pObjectList[i] = m_pObjectList[i - 1];
		}
		m_pObjectList[nIdx].bNotCacheable = bNodeCache;
		m_pObjectList[nIdx].bSingleFrameLoad = false;
		m_pObjectList[nIdx].bType = ISI_T_BITMAP16;
		m_pObjectList[nIdx].pObject = pBitmap;
		m_pObjectList[nIdx].pFrames = NULL;
		m_pObjectList[nIdx].uId = uImage;
		m_pObjectList[nIdx].pSurface = NULL;
		m_pObjectList[nIdx].bRef = true;
		m_pObjectList[nIdx].pImgData = NULL;
		m_pObjectList[nIdx].pScaleImg = NULL;
		m_nNumImages++;
	}
	else if (pImg)
	{
		pImg->nWidth = nWidth;
		pImg->nHeight = nHeight;
		pImg->bScrMode = (BOOL)bScrMode;
		nIdx = - nIdx - 1;
		for (int i = m_nNumImages; i > nIdx; i--)
		{
			m_pObjectList[i] = m_pObjectList[i - 1];
		}
		m_pObjectList[nIdx].bNotCacheable = bNodeCache;
		m_pObjectList[nIdx].bSingleFrameLoad = false;
		m_pObjectList[nIdx].bType = ISI_T_DRAWINGRC;
		m_pObjectList[nIdx].pObject = pImg;
		m_pObjectList[nIdx].pFrames = NULL;
		m_pObjectList[nIdx].uId = uImage;
		m_pObjectList[nIdx].pSurface = NULL;
		m_pObjectList[nIdx].bRef = true;
		m_pObjectList[nIdx].pImgData = NULL;
		m_pObjectList[nIdx].pScaleImg = NULL;
		m_nNumImages++;
	}
	else
		uImage = 0;
	return uImage;
}

void KImageStore2::Free()
{
    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	if (m_pObjectList)
	{
        for (int i = 0; i < m_nNumImages; i++)
			FreeImageObject(m_pObjectList[i]);
		free(m_pObjectList);
		m_pObjectList = NULL;
	}
	m_nNumImages = 0;
	m_nNumReserved = 0;

	m_uImageAccessCounter = 0;
}

void KImageStore2::FreeImage(const char* pszImage)
{
    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	unsigned int uImage = ImageNameToId(pszImage);
	int nIdx = FindImage(uImage, 0);
	if (nIdx >= 0)
	{
		FreeImageObject(m_pObjectList[nIdx]);
		m_nNumImages--;
		for (int i = nIdx; i < m_nNumImages; i++)
			m_pObjectList[i] = m_pObjectList[i + 1];
	}
}

void* KImageStore2::GetExistedCreateBitmap(const char* pszImage, unsigned int uImage, short& nImagePosition)
{
	if (uImage == 0)
		uImage = ImageNameToId(pszImage);

    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	nImagePosition = FindImage(uImage, nImagePosition);
	void* pObject = NULL;
	if (nImagePosition >= 0 &&
		(m_pObjectList[nImagePosition].bType == ISI_T_BITMAP16 ||
		m_pObjectList[nImagePosition].bType == ISI_T_DRAWINGRC) &&
		/*m_pObjectList[nImagePosition].bNotCacheable == true &&*/
		m_pObjectList[nImagePosition].pObject)
	{
		m_pObjectList[nImagePosition].bRef = true;
		pObject = (m_pObjectList[nImagePosition].pObject);
	}
	return pObject;
}

LPDIRECTDRAWSURFACE CreateDirectDrawSurfaceFromBitmap(KSGImageContent* pBitmap)
{
	LPDIRECTDRAWSURFACE pSurface = NULL;
	if (pBitmap && g_pDirectDraw)
	{
		pSurface = g_pDirectDraw->CreateSurface(pBitmap->nWidth, pBitmap->nHeight);
	}
	return pSurface;
}

bool KImageStore2::CreateBitmapSurface(const char* pszImage, unsigned int& uImage, short& nImagePosition)
{
	if (uImage == 0)
		uImage = ImageNameToId(pszImage);

	bool bOk = false;

	KAutoCriticalSection AutoLock(m_ImageProcessLock);

	nImagePosition = FindImage(uImage, nImagePosition);
	if (nImagePosition >= 0)
	{
		_KISImageObj& Img = m_pObjectList[nImagePosition];
        if (Img.bType == ISI_T_BITMAP16 && Img.bNotCacheable == true && Img.pObject)
		{
			if (Img.pSurface == NULL)
			{
				Img.pSurface = CreateDirectDrawSurfaceFromBitmap(
								(KSGImageContent *)(Img.pObject));
			}
			bOk = (Img.pSurface != NULL);
		}
	}
	return bOk;
}

void* KImageStore2::CreateSprData(int& bExistData, int nWidth, int nHeight, unsigned int uImage,
									short nImagePosition, int nFrame, int nTotalFrame)
{
	bExistData = 0;
	KAutoCriticalSection AutoLock(m_ImageProcessLock);
	if ((nImagePosition = FindImage(uImage, nImagePosition)) >= 0)
	{
		_KISImageObj&	ImgObj = m_pObjectList[nImagePosition];
		if (ImgObj.bType != ISI_T_SPR)
			return NULL;
		if(ImgObj.pImgData == NULL)
		{
			ImgObj.pImgData = (AlphaRecContent**)malloc(sizeof(AlphaRecContent*)*nTotalFrame);
			if(!ImgObj.pImgData)
				return NULL;
			ImgObj.nFrameNum = nTotalFrame;
			memset(ImgObj.pImgData, 0, sizeof(AlphaRecContent*)*nTotalFrame);
		}
		if(ImgObj.pImgData[nFrame])
		{
			bExistData = 1;
			return ImgObj.pImgData[nFrame];
		}
		ImgObj.pImgData[nFrame] = (AlphaRecContent*)malloc(CREATE_REC_CONTENT_SIZE(nWidth, nHeight));
		if(!ImgObj.pImgData[nFrame])
			return NULL;
		ImgObj.pImgData[nFrame]->nWidth = nWidth;
		ImgObj.pImgData[nFrame]->nHeight = nHeight;
		memset(ImgObj.pImgData[nFrame]->Data, 0, nWidth * nHeight * sizeof(unsigned int));
		return ImgObj.pImgData[nFrame];
	}
	return NULL;
}

unsigned char* KImageStore2::FindScaleData(unsigned int uImage, short nImagePosition, int nFrame,
										int nWidth, int nHeight, unsigned char*& pPal)
{
	KAutoCriticalSection AutoLock(m_ImageProcessLock);
	if ((nImagePosition = FindImage(uImage, nImagePosition)) >= 0)
	{
		_KISImageObj&	ImgObj = m_pObjectList[nImagePosition];
		if (ImgObj.bType != ISI_T_SPR && ImgObj.bType != ISI_T_DRAWINGRC)
			return NULL;
		if(ImgObj.pScaleImg == NULL)
		{
			ImgObj.nScaleNum = 0;
			return NULL;
		}
		char szBuff[64];
		sprintf(szBuff, "#%u_%d_%d_%d", uImage, nFrame, nWidth, nHeight);
		unsigned int uNameID = ImageNameToId(szBuff);
		for(int i=0;i<ImgObj.nScaleNum;++i)
		{
			if(ImgObj.pScaleImg[i].uID == uNameID)
			{
				pPal = ImgObj.pScaleImg[i].pPal;
				return ImgObj.pScaleImg[i].pSprite;
			}
		}
	}
	return NULL;
}

void KImageStore2::AddScaleData(unsigned int uImage, short nImagePosition, int nFrame, int nWidth, int nHeight,
								unsigned char* pSprite, unsigned char* pPal)
{
	KAutoCriticalSection AutoLock(m_ImageProcessLock);
	if ((nImagePosition = FindImage(uImage, nImagePosition)) >= 0)
	{
		_KISImageObj&	ImgObj = m_pObjectList[nImagePosition];
		if (ImgObj.bType != ISI_T_SPR && ImgObj.bType != ISI_T_DRAWINGRC)
			return;
		char szBuff[64];
		sprintf(szBuff, "#%u_%d_%d_%d", uImage, nFrame, nWidth, nHeight);
		unsigned int uNameID = ImageNameToId(szBuff);
		ImgObj.pScaleImg = (ScaleImageInfo*)realloc(ImgObj.pScaleImg, sizeof(ScaleImageInfo)*(ImgObj.nScaleNum + 1));
		ImgObj.pScaleImg[ImgObj.nScaleNum].uID = uNameID;
		ImgObj.pScaleImg[ImgObj.nScaleNum].pSprite = pSprite;
		ImgObj.pScaleImg[ImgObj.nScaleNum].pPal = pPal;
		ImgObj.nScaleNum++;
	}
}

void KImageStore2::FreeScaleData(unsigned int uImage, short nImagePosition)
{
	KAutoCriticalSection AutoLock(m_ImageProcessLock);
	if ((nImagePosition = FindImage(uImage, nImagePosition)) >= 0)
	{
		_KISImageObj&	ImgObject = m_pObjectList[nImagePosition];
		if(ImgObject.pScaleImg)
		{
			for(int i=0; i < ImgObject.nScaleNum;i++)
			{
				if(ImgObject.pScaleImg[i].pSprite)
				{
					free(ImgObject.pScaleImg[i].pSprite);
					ImgObject.pScaleImg[i].pSprite = NULL;
					if(ImgObject.pScaleImg[i].pPal)
					{
						free(ImgObject.pScaleImg[i].pPal);
						ImgObject.pScaleImg[i].pPal = NULL;
					}
				}
			}
			free(ImgObject.pScaleImg);
			ImgObject.pScaleImg = NULL;
			ImgObject.nScaleNum = 0;
		}
	}
}

void* KImageStore2::GetImage(const char* pszImage, unsigned int& uImage,
					short& nImagePosition, int nFrame, int nType, void*& pFrameData, int bFindById)
{
	pFrameData = NULL;
	if (uImage == 0)
	{
		if ((uImage = ImageNameToId(pszImage)) == 0)
			return NULL;
	}

    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	void* pObject = NULL;
	if ((nImagePosition = FindImage(uImage, nImagePosition)) >= 0)
	{
		_KISImageObj&	ImgObj = m_pObjectList[nImagePosition];
		if (ImgObj.bType == (unsigned char)nType)
		{
			if (ImgObj.pObject)
			{
				pObject = ImgObj.pObject;
				if (ImgObj.bType == ISI_T_SPR)
				{
					if ((pFrameData = GetSprFrame(pszImage, ImgObj, nFrame)) == NULL)
						pObject = NULL;
				}
				else if (ImgObj.bType == ISI_T_BITMAP16)
				{
					pFrameData = ImgObj.pSurface;
				}
			}
			ImgObj.bRef = true;
		}
		//else ÂÔÈ¥Í¬idµ«ÊÇ²»Í¬Í¼ÐÎÀàÐÍÇé¿öµÄ´¦Àí¡£ÒòÎªÕâÊÇÊÜÏÞÇé¿ö

		//ÎªÁËÖ´ÐÐÐ§ÂÊËùÒÔÎ´°ÑÏÂÃæµÄÅÐ¶Ï·ÅÔÚCheckBalanceº¯ÊýÌåÀïÃæ¡£ÏÂÍ¬¡£
		if (m_nNumImages > m_nBalanceNum && (++m_uImageAccessCounter) > m_uCheckPoint)
			CheckBalance();
		return pObject;
	}

	// ÓÐ¿Õ¼ä
	if (m_nNumImages < m_nNumReserved || ExpandSpace())
	{
		nImagePosition = - nImagePosition - 1;	// FindImageÊ±ÒÑ¾­ÕÒºÃÎ»ÖÃÁË
		for (int i = m_nNumImages; i > nImagePosition; i--)
		{
			m_pObjectList[i] = m_pObjectList[i - 1];
		}

		_KISImageObj& ImgObj = m_pObjectList[nImagePosition];		
		ImgObj.bNotCacheable = false;
		ImgObj.bRef = true;
		ImgObj.bSingleFrameLoad = false;
		ImgObj.bType = (unsigned char)nType;
		ImgObj.pFrames = NULL;
		ImgObj.pObject = NULL;
		ImgObj.uId = uImage;
		ImgObj.pcAdjustColorPalettes = NULL;
		ImgObj.pImgData = NULL;
		ImgObj.pScaleImg = NULL;
		
		pObject = LoadImage(pszImage, ImgObj, nFrame, pFrameData, bFindById);
		m_nNumImages++;

		if (m_nNumImages > m_nBalanceNum && (++m_uImageAccessCounter) > m_uCheckPoint)
			CheckBalance();
	}
	return pObject;
}

void* KImageStore2::GetSprFrame(const char* pszImageFile, _KISImageObj& ImgObject, int nFrame)
{
	SPRHEAD* pSprHeader = (SPRHEAD*)ImgObject.pObject;
	void* pFrameData = NULL;

	//_ASSERT(pszImageFile && pSprHeader);
	if (nFrame >= 0 && nFrame < pSprHeader->Frames)
	{
		_KISImageFrameObj* pFrame;
		if (ImgObject.bSingleFrameLoad == false)
		{	//Ò»´Î¼ÓÔØÈ«²¿Ö¡µÄÍ¼ÐÎ
			pFrame = ImgObject.pFrames;
			//_ASSERT(pFrame);
			//_ASSERT(pFrame->pOffsetTable);
			pFrameData = (((char*)pFrame->pOffsetTable) + pFrame->sOffTableSize +
				((SPROFFS*)pFrame->pOffsetTable)[nFrame].Offset);
			pFrame->bRef = true;
		}
		else
		{
			pFrame = &ImgObject.pFrames[nFrame];
			pFrame->bRef = true;
			if ((pFrameData = pFrame->pFrameData) == NULL)
			{	//Ö¸¶¨µÄÖ¡Êý¾Ý»¹²»´æÔÚ
				pFrame->pFrameData = SprGetFrame((SPRHEAD*)ImgObject.pObject, nFrame);
				pFrameData = pFrame->pFrameData;
			}
		}
	}
	return pFrameData;
}

bool KImageStore2::GetImageParam(const char* pszImage, int nType, KImageParam* pImageData)
{
	unsigned int uImage = ImageNameToId(pszImage);
	
	if (uImage == 0)
		return false;

    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	short nImagePosition = FindImage(uImage, -1);
	if (nImagePosition < 0)
	{
		void* pFrameData;
		if (GetImage(pszImage, uImage, nImagePosition, 0, nType, pFrameData) == NULL)
			nImagePosition = -1;
	}
	if (nImagePosition >= 0)
	{
		_KISImageObj&	ImgObj = m_pObjectList[nImagePosition];
		if (ImgObj.pObject && ImgObj.bType == (unsigned char)nType)
		{
			if (pImageData)
			{
				if (ImgObj.bType == ISI_T_SPR)
				{
					SPRHEAD* pSprHeader = (SPRHEAD*)ImgObj.pObject;
					pImageData->nWidth = pSprHeader->Width;
					pImageData->nHeight = pSprHeader->Height;
					pImageData->nInterval = pSprHeader->Interval;
					pImageData->nNumFrames = pSprHeader->Frames;
					pImageData->nNumFramesGroup = pSprHeader->Directions;
					pImageData->nReferenceSpotX = pSprHeader->CenterX;
					pImageData->nReferenceSpotY = pSprHeader->CenterY;				
				}
				else if (ImgObj.bType == ISI_T_BITMAP16)
				{
					pImageData->nWidth  = ((KSGImageContent*)(ImgObj.pObject))->nWidth;
					pImageData->nHeight = ((KSGImageContent*)(ImgObj.pObject))->nHeight;
					pImageData->nInterval = 0;
					pImageData->nNumFrames = 1;
					pImageData->nNumFramesGroup = 1;
					pImageData->nReferenceSpotX = 0;
					pImageData->nReferenceSpotY = 0;
				}
			}
			return true;
		}
	}
	return false;
}

bool KImageStore2::GetImageFrameParam(const char* pszImage, int nType,
		int nFrame, KRPosition2* pOffset, KRPosition2* pSize)
{	
	bool bRet = false;
	short	nPos = -1;
	unsigned int uImage = 0;
	void* pFrame;
	void* pImage = GetImage(pszImage, uImage, nPos, nFrame, nType, pFrame);
	if (pImage)
	{
		if (nType == ISI_T_SPR)
		{
			if (pOffset)
			{
				pOffset->nX = ((SPRFRAME*)pFrame)->OffsetX;
				pOffset->nY = ((SPRFRAME*)pFrame)->OffsetY;
			}
			if (pSize)
			{
				pSize->nX = ((SPRFRAME*)pFrame)->Width;
				pSize->nY = ((SPRFRAME*)pFrame)->Height;
			}
		}
		else if (nType == ISI_T_BITMAP16)
		{
			if (pOffset)
			{
				pOffset->nX = 0;
				pOffset->nY = 0;
			}
			if (pSize)
			{
				pSize->nX = ((KSGImageContent*)pImage)->nWidth;
				pSize->nY = ((KSGImageContent*)pImage)->nHeight;
			}
		}
		bRet = true;
	}
	return bRet;
}

int KImageStore2::GetImagePixelAlpha(const char* pszImage, int nType, int nFrame, int nX, int nY)
{
	int nRet = 0;
	short	nPos = -1;
	unsigned int uImage = 0;
	SPRFRAME* pFrame;
	void* pImage = GetImage(pszImage, uImage, nPos, 0, nType, (void*&)pFrame);
	if (pImage)
	{
		if (nType == ISI_T_SPR)
		{
			nX -= pFrame->OffsetX;
			nY -= pFrame->OffsetY;
			if (nX >= 0  && nX < pFrame->Width && nY >= 0 && nY < pFrame->Height)
			{
				int	nNumPixels = pFrame->Width;
				void*	pSprite =  pFrame->Sprite;
				nY++;
				_asm
				{
					//Ê¹SDIÖ¸ÏòspriteÖÐµÄÍ¼ÐÎÊý¾ÝÎ»ÖÃ
					mov		esi, pSprite
				dec_line:
					dec		nY				//¼õµôÒ»ÐÐ
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
					mov		edx, nX
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

					mov		nRet, ebx
				}
			}
		}
		else if (nType == ISI_T_BITMAP16)
		{
			if (nX >= 0 && nY >= 0 &&
				nX < ((KSGImageContent*)pImage)->nWidth &&
				nY < ((KSGImageContent*)pImage)->nHeight)
			{
				nRet = 255;
			}
		}
	}
	return nRet;
}

bool KImageStore2::Init()
{
	Free();
	return ExpandSpace();
}

bool KImageStore2::SaveImage(const char* pszFile, const char* pszImage, int nFileType)
{
	return 0;
}

void KImageStore2::SetBalanceParam(int nNumImage, unsigned int uCheckPoint)
{
	m_nBalanceNum = nNumImage;
	m_uCheckPoint = uCheckPoint;
}

void KImageStore2::CheckBalance()
{
	m_uImageAccessCounter = 0;
	int i, j;
    int nNewNumImages = 0;
    /*
    MEMORYSTATUS MemStatus;
    GlobalMemoryStatus(&MemStatus);

    // Èç¹ûÊ£Óà¿É·ÖÅäµÄÎïÀíÄÚ´æ´óÓÚÕû¸öÎïÀíÄÚ´æµÄÒ»¸ö±ÈÀý£¬Ôò²»ÐèÒª½øÐÐCacheµ÷Õû
    if (MemStatus.dwAvailPhys > (MemStatus.dwTotalPhys / 32))
    {
        if (m_nNumImages < 1024)    // CacheÖÐÍ¼Æ¬µÄÉÏÏÞ£¬CacheÖÐ×î¶àÓÐ1024ÕÅÍ¼Æ¬
            return;
    }
*/
    //KAutoCriticalSection AutoLock(m_ImageProcessLock);
    
    nNewNumImages = 0;
    for (i = 0; i < m_nNumImages; i++)
    {
        if (m_pObjectList[i].bNotCacheable == false)
        {
			if (m_pObjectList[i].bRef == false)
			{
				FreeImageObject(m_pObjectList[i]);
                continue;
            }

			if (
                (m_pObjectList[i].bSingleFrameLoad == true) &&
				(m_pObjectList[i].pObject) &&
				(m_pObjectList[i].bType == ISI_T_SPR)
            )
			{
				int nNumFrame = ((SPRHEAD*)(m_pObjectList[i].pObject))->Frames;
				for (j = 0; j < nNumFrame; j++)
				{
					if (m_pObjectList[i].pFrames[j].bRef == false)
					{
						FreeImageObject(m_pObjectList[i], j);
					}
                    else
                    {
                        m_pObjectList[i].pFrames[j].bRef = false;
                    }
				}
			}
        }

        m_pObjectList[nNewNumImages] = m_pObjectList[i];
        m_pObjectList[nNewNumImages].bRef = false;
        nNewNumImages++;
    }
    m_nNumImages =  nNewNumImages;
}

bool KImageStore2::ExpandSpace()
{
	_KISImageObj* pNewList = (_KISImageObj *)realloc(m_pObjectList,
						(m_nNumReserved + ISBP_EXPAND_SPACE_STEP) * sizeof(_KISImageObj));
	if (pNewList)
	{
		m_pObjectList = pNewList;
		m_nNumReserved += ISBP_EXPAND_SPACE_STEP;
		return true;
	}
	return false;
}

int KImageStore2::FindImage(unsigned int uImage, int nPossiblePosition)
{
	int nPP = nPossiblePosition;
	if (nPP < 0 || nPP >= m_nNumImages)
	{
		if (m_nNumImages <= 0)
		{
			return -1;
		}
		else
		{
			nPP = m_nNumImages / 2;
		}
	}
	if (m_pObjectList[nPP].uId == uImage)
		return nPP;
	int nFrom, nTo, nTryRange;
	nTryRange = ISBP_TRY_RANGE_DEF;
	if (m_pObjectList[nPP].uId > uImage)
	{
		nFrom = 0;
		nTo = nPP - 1;
		nPP -= nTryRange;
	}
	else
	{
		nFrom = nPP + 1;
		nTo = m_nNumImages - 1;
		nPP += nTryRange;
	}
	if (nFrom + nTryRange >= nTo)
		nPP = (nFrom + nTo) / 2;

	while (nFrom < nTo)
	{
		if (m_pObjectList[nPP].uId < uImage)
		{
			nFrom = nPP + 1;
		}
		else if (m_pObjectList[nPP].uId > uImage)
		{
			nTo = nPP - 1;
		}
		else
		{
			return nPP;
		}
		nPP = (nFrom + nTo) / 2;
	}
	if (nFrom == nTo)
	{
		if (m_pObjectList[nPP].uId > uImage)
		{
			nPP = - nPP - 1;
		}
		else if (m_pObjectList[nPP].uId < uImage)
		{
			nPP = - nPP - 2;
		}
	}
	else
	{
		nPP = - nFrom -1;
	}
	return nPP;
}

void KImageStore2::FreeImageObject(_KISImageObj& ImgObject, int nFrame/*=-1*/)
{
	if(ImgObject.pObject)
	{
		if (ImgObject.bType == ISI_T_SPR)
		{
			if(nFrame < 0)
			{
				if(ImgObject.pImgData)
				{
					for(int i=0; i < ImgObject.nFrameNum;i++)
					{
						if(ImgObject.pImgData[i])
						{
							free(ImgObject.pImgData[i]);
							ImgObject.pImgData[i] = NULL;
						}
					}
					free(ImgObject.pImgData);
					ImgObject.pImgData = NULL;
				}
				if(ImgObject.pScaleImg)
				{
					for(int i=0; i < ImgObject.nScaleNum;i++)
					{
						if(ImgObject.pScaleImg[i].pSprite)
						{
							free(ImgObject.pScaleImg[i].pSprite);
							ImgObject.pScaleImg[i].pSprite = NULL;
							free(ImgObject.pScaleImg[i].pPal);
							ImgObject.pScaleImg[i].pPal = NULL;
						}
					}
					free(ImgObject.pScaleImg);
					ImgObject.pScaleImg = NULL;
				}
			}
			if (ImgObject.bSingleFrameLoad == false)
			{
				free (ImgObject.pFrames);
				ImgObject.pFrames = NULL;
				if (ImgObject.pcAdjustColorPalettes)
				{
					free(ImgObject.pcAdjustColorPalettes);
					ImgObject.pcAdjustColorPalettes = NULL;
				}
				if (ImgObject.pObject)
				{
					SprReleaseHeader((SPRHEAD*)ImgObject.pObject);
					ImgObject.pObject = NULL;
				}
			}
			else
			{
				int nNumFrame = ((SPRHEAD*)ImgObject.pObject)->Frames;
				if (nFrame >= 0 && nFrame < nNumFrame)
				{
					if (ImgObject.pFrames[nFrame].pFrameData)
					{
						SprReleaseFrame((SPRFRAME*)ImgObject.pFrames[nFrame].pFrameData);
						ImgObject.pFrames[nFrame].pFrameData = NULL;
					}
				}
				else if (nFrame < 0)
				{
					for (nFrame = 0; nFrame < nNumFrame; nFrame++)
					{
						if (ImgObject.pFrames[nFrame].pFrameData)
							SprReleaseFrame((SPRFRAME*)ImgObject.pFrames[nFrame].pFrameData);
					}
					free (ImgObject.pFrames);
					ImgObject.pFrames = NULL;
					if (ImgObject.pcAdjustColorPalettes)
					{
						free(ImgObject.pcAdjustColorPalettes);
						ImgObject.pcAdjustColorPalettes = NULL;
					}
					SprReleaseHeader((SPRHEAD*)(ImgObject.pObject));
					ImgObject.pObject = NULL;
				}
			}
		}
		else if (ImgObject.bType == ISI_T_BITMAP16 || ImgObject.bType == ISI_T_DRAWINGRC)
		{
			if(ImgObject.bType == ISI_T_DRAWINGRC)
			{
				if(ImgObject.pImgData)
				{
					for(int i=0; i < ImgObject.nFrameNum;i++)
					{
						if(ImgObject.pImgData[i])
						{
							free(ImgObject.pImgData[i]);
							ImgObject.pImgData[i] = NULL;
						}
					}
					free(ImgObject.pImgData);
					ImgObject.pImgData = NULL;
				}
				if(ImgObject.pScaleImg)
				{
					for(int i=0; i < ImgObject.nScaleNum;i++)
					{
						if(ImgObject.pScaleImg[i].pSprite)
						{
							free(ImgObject.pScaleImg[i].pSprite);
							ImgObject.pScaleImg[i].pSprite = NULL;
							if(ImgObject.pScaleImg[i].pPal)
							{
								free(ImgObject.pScaleImg[i].pPal);
								ImgObject.pScaleImg[i].pPal = NULL;
							}
						}
					}
					free(ImgObject.pScaleImg);
					ImgObject.pScaleImg = NULL;
				}
			}
			if (ImgObject.pObject)
			{
				if (ImgObject.bType == ISI_T_BITMAP16)
					free((KSGImageContent *)ImgObject.pObject);
				else
					free((AlphaRecContent *)ImgObject.pObject);
				ImgObject.pObject = NULL;
			}
		}
		else
			ImgObject.pObject = NULL;
	}
}

void* KImageStore2::LoadImage(const char* pszImageFile, _KISImageObj& ImgObj, int nFrame, void*& pFrameData, int bFindById)
{
	void* pRet = NULL;
	_KISImageFrameObj* pFrameObj = NULL;
	if (ImgObj.bType == ISI_T_SPR)
	{
		SPROFFS*	pOffsTable = NULL;
		SPRHEAD*  pSprHeader = SprGetHeader(pszImageFile, pOffsTable, bFindById);
		if (pSprHeader)
		{
			if (pOffsTable)	//spr th­êng
			{
				pFrameObj = (_KISImageFrameObj*)malloc(sizeof(_KISImageFrameObj));
				if (pFrameObj)
				{
					ImgObj.pObject = pSprHeader;
					ImgObj.pFrames = pFrameObj;
					ImgObj.bNotCacheable = false;
					ImgObj.bSingleFrameLoad = false;
					pFrameObj->pOffsetTable = pOffsTable;
					pFrameObj->sOffTableSize = sizeof(SPROFFS) * pSprHeader->Frames;
					pFrameObj->bRef = true;

					if (nFrame >= 0 && nFrame < pSprHeader->Frames)
					{
						pFrameData = ((char*)pOffsTable + pFrameObj->sOffTableSize +
							pOffsTable[nFrame].Offset);
						pRet = pSprHeader;
					}					
				}
			}
			else	//type_frame
			{
				int nSize = sizeof(_KISImageFrameObj) * pSprHeader->Frames;
				pFrameObj = (_KISImageFrameObj*)malloc(nSize);
				if (pFrameObj)
				{
					memset(pFrameObj, 0, nSize);
					ImgObj.pObject = pSprHeader;
					ImgObj.pFrames = pFrameObj;
					ImgObj.bNotCacheable = false;
					ImgObj.bSingleFrameLoad = true;
					if (nFrame >= 0 && nFrame < pSprHeader->Frames)
					{
						pFrameData = (SPRFRAME*)SprGetFrame(pSprHeader, nFrame);
						if (pFrameData)
						{
							ImgObj.pFrames[nFrame].bRef = true;
							ImgObj.pFrames[nFrame].pFrameData = (SPRFRAME*)pFrameData;
							pRet = pSprHeader;
						}
					}
				}
			}
			if (ImgObj.pObject)
			{
				if(g_pDirectDraw->GetRGBBitCount() != 32)	//chÕ ®é 16bit míi cÇn chuyÓn palette
				{
					g_Pal24ToPal16((KPAL24*)(&pSprHeader[1]),
					(KPAL16*)(&pSprHeader[1]), pSprHeader->Colors);
				}
			}
			else
			{
				SprReleaseHeader(pSprHeader);
				ImgObj.pcAdjustColorPalettes = NULL;
			}
		}
	}
	else if (ImgObj.bType == ISI_T_BITMAP16)
	{
		ImgObj.pObject = get_jpg_image(pszImageFile);
		pRet = ImgObj.pObject;
	}
	return pRet;
}

KImageStore2::KImageStore2()
{
	m_pObjectList = NULL;
	m_nNumReserved = 0;
	m_nNumImages  = 0;
	m_nBalanceNum = ISBP_BALANCE_NUM_DEF;
	m_uCheckPoint = ISBP_CHECK_POINT_DEF;
	m_uImageAccessCounter = 0;
}

KImageStore2::~KImageStore2()
{
	Free();
}

void KImageStore2::ClearAllAdjustColorPalette()
{
    KAutoCriticalSection AutoLock(m_ImageProcessLock);

	for (int i = 0; i < m_nNumImages; i++)
	{
		if (m_pObjectList[i].pcAdjustColorPalettes)
		{
			free (m_pObjectList[i].pcAdjustColorPalettes);
			m_pObjectList[i].pcAdjustColorPalettes = NULL;
		}
	}
}