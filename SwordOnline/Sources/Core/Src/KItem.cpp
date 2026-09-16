#include "KCore.h"
#include "MyAssert.H"
#include "KTabFile.h"
#include "KNpc.h"
#include "KItem.h"
#include "KItemSet.h"
#ifndef _STANDALONE
#include "../../../lib/S3DBInterface.h"
#else
#include "S3DBInterface.h"
#endif

#ifndef _SERVER

#include "ImgRef.h"
#include "KPlayer.h"
#include "../../Represent/iRepresent/iRepresentshell.h"
#include "KMagicDesc.h"
#include "KItemGenerator.h"

#endif

KItem	Item[MAX_ITEM];
int GetRandomNumber(int nMin, int nMax);

KItem::KItem()
{
	::memset(&m_CommonAttrib,    0, sizeof(m_CommonAttrib));
	::memset(m_aryBaseAttrib,    0, sizeof(m_aryBaseAttrib));
	::memset(m_aryRequireAttrib, 0, sizeof(m_aryRequireAttrib));
	::memset(m_aryMagicAttrib,   0, sizeof(m_aryMagicAttrib));
	::memset(m_aryMagicAttribEx,   0, sizeof(m_aryMagicAttribEx));
	m_dwID = 0;
	m_nCurrentDur = 0;
	m_nExType = 0;
	m_nGenParam = 0;
	m_nGroupId = 0;		
	m_nExGroupId = 0;
	m_nGroupSerial = 0;
#ifdef _SERVER
	m_bCanSell = TRUE;
#endif

#ifndef _SERVER
	::memset(&m_Image,   0, sizeof(KRUImage));
	m_bShowLevel = 0;
	m_bShowSeries = 0;
	m_bShortKey = 0;
#endif
}

KItem::~KItem()
{
}

void* KItem::GetRequirement(IN int nReq)
{
	int i = sizeof(m_aryRequireAttrib)/sizeof(m_aryRequireAttrib[0]);
	if (nReq >= i)
		return NULL;

	return &m_aryRequireAttrib[nReq];
}
/******************************************************************************
¹¦ÄÜ:	½«itemÉÏµÄÄ§·¨Ó¦ÓÃµ½NPCÉíÉÏ
Èë¿Ú£º	pNPC: Ö¸ÏòNPCµÄÖ¸Õë£¬nMagicAcive£º´ò¿ªµÄÒþ²ØÊôÐÔÊýÄ¿
³ö¿Ú:	Ä§·¨±»Ó¦ÓÃ¡£
		¾ßÌå¹¤×÷ÓÉKNpcµÄ³ÉÔ±º¯ÊýÍê³É¡£
		KItem ¶ÔÏó±¾ÉíÃ»ÓÐ³ÉÔ±±äÁ¿±»ÐÞ¸Ä
******************************************************************************/
void KItem::ApplyMagicAttribToNPC(IN KNpc* pNPC, IN int nMagicActive /* = 0 */, IN BOOL bActAll /*= FALSE*/) const
{
	_ASSERT(this != NULL);
	_ASSERT(nMagicActive >= 0);

	int nCount = nMagicActive;
	int i;

	// »ù´¡ÊôÐÔµ÷ÕûNPC
	for (i = 0; i < sizeof(m_aryBaseAttrib)/sizeof(m_aryBaseAttrib[0]); i++)
	{
		const KItemNormalAttrib* pAttrib;
		pAttrib = &(m_aryBaseAttrib[i]);
		if (pAttrib->nAttribType > 0)
		{
			pNPC->ModifyAttrib(pNPC->m_Index, (void *)pAttrib);
		}
	}
	// Ä§·¨ÊôÐÔµ÷ÕûNPC
	for (i = 0; i < sizeof(m_aryMagicAttrib)/sizeof(m_aryMagicAttrib[0]); i++)
	{
		const KItemNormalAttrib* pAttrib;
		pAttrib = &(m_aryMagicAttrib[i]);

		if (pAttrib->nAttribType > 0)
		{
			if (i & 1)						// ÎªÆæÊý£¬ÊÇºó×º£¨i´ÓÁã¿ªÊ¼£©
			{
				if (nCount > 0)
				{
					pNPC->ModifyAttrib(pNPC->m_Index, (void *)pAttrib);
					nCount--;
				}
			}
			else
			{
				pNPC->ModifyAttrib(pNPC->m_Index, (void *)pAttrib);
			}
		}
	}
	if(bActAll)
	{
		for (i = 0; i < sizeof(m_aryMagicAttribEx)/sizeof(m_aryMagicAttribEx[0]); i++)
		{
			const KItemNormalAttrib* pAttrib;
			pAttrib = &(m_aryMagicAttribEx[i]);
	
			if (pAttrib->nAttribType > 0)
			{
				pNPC->ModifyAttrib(pNPC->m_Index, (void *)pAttrib);
			}
		}
	}
}

/******************************************************************************
¹¦ÄÜ:	½«itemÉÏµÄÄ§·¨´ÓNPCÉíÉÏÒÆ³ý
Èë¿Ú£º	pNPC: Ö¸ÏòNPCµÄÖ¸Õë£¬nMagicAcive£º´ò¿ªµÄÒþ²ØÊôÐÔÊýÄ¿
³ö¿Ú:	Ä§·¨±»Ó¦ÓÃ¡£
		¾ßÌå¹¤×÷ÓÉKNpcµÄ³ÉÔ±º¯ÊýÍê³É¡£
		KItem ¶ÔÏó±¾ÉíÃ»ÓÐ³ÉÔ±±äÁ¿±»ÐÞ¸Ä
******************************************************************************/
void KItem::RemoveMagicAttribFromNPC(IN KNpc* pNPC, IN int nMagicActive /* = 0 */, IN BOOL bActAll /*= FALSE*/) const
{
	_ASSERT(this != NULL);
	_ASSERT(nMagicActive >= 0);

	int nCount = nMagicActive;
	int	i;
	
	// »ù´¡ÊôÐÔµ÷ÕûNPC
	for (i = 0; i < sizeof(m_aryBaseAttrib)/sizeof(m_aryBaseAttrib[0]); i++)
	{
		const KItemNormalAttrib* pAttrib;
		pAttrib = &(m_aryBaseAttrib[i]);
		if (pAttrib->nAttribType > 0)
		{
			KItemNormalAttrib RemoveAttrib;
			RemoveAttrib.nAttribType = pAttrib->nAttribType;
			RemoveAttrib.nValue[0] = -pAttrib->nValue[0];
			RemoveAttrib.nValue[1] = -pAttrib->nValue[1];
			RemoveAttrib.nValue[2] = -pAttrib->nValue[2];
			pNPC->ModifyAttrib(pNPC->m_Index, (void *)&RemoveAttrib);
		}
	}

	for (i = 0; i < sizeof(m_aryMagicAttrib)/sizeof(m_aryMagicAttrib[0]); i++)
	{
		const KItemNormalAttrib* pAttrib;
		pAttrib = &(m_aryMagicAttrib[i]);

		if (pAttrib->nAttribType > 0)		// TODO: Îª -1 ¶¨ÒåÒ»¸ö³£Á¿?
		{
			KItemNormalAttrib RemoveAttrib;
			if (i & 1)						// ÎªÆæÊý£¬ÊÇºó×º£¨i´ÓÁã¿ªÊ¼£©
			{
				if (nCount > 0)
				{
					RemoveAttrib.nAttribType = pAttrib->nAttribType;
					RemoveAttrib.nValue[0] = -pAttrib->nValue[0];
					RemoveAttrib.nValue[1] = -pAttrib->nValue[1];
					RemoveAttrib.nValue[2] = -pAttrib->nValue[2];
					pNPC->ModifyAttrib(pNPC->m_Index, (void *)&RemoveAttrib);
					nCount--;
				}
			}
			else
			{
				RemoveAttrib.nAttribType = pAttrib->nAttribType;
				RemoveAttrib.nValue[0] = -pAttrib->nValue[0];
				RemoveAttrib.nValue[1] = -pAttrib->nValue[1];
				RemoveAttrib.nValue[2] = -pAttrib->nValue[2];
				pNPC->ModifyAttrib(pNPC->m_Index, (void *)&RemoveAttrib);
			}
		}
	}
	if(bActAll)
	{
		for (i = 0; i < sizeof(m_aryMagicAttribEx)/sizeof(m_aryMagicAttribEx[0]); i++)
		{
			const KItemNormalAttrib* pAttrib;
			pAttrib = &(m_aryMagicAttribEx[i]);
	
			if (pAttrib->nAttribType > 0)
			{
				KItemNormalAttrib RemoveAttrib;
				RemoveAttrib.nAttribType = pAttrib->nAttribType;
				RemoveAttrib.nValue[0] = -pAttrib->nValue[0];
				RemoveAttrib.nValue[1] = -pAttrib->nValue[1];
				RemoveAttrib.nValue[2] = -pAttrib->nValue[2];
				pNPC->ModifyAttrib(pNPC->m_Index, (void *)&RemoveAttrib);
			}
		}
	}
}

/******************************************************************************
¹¦ÄÜ:	½«itemÉÏµÄµÚNÏîÒþ²ØÄ§·¨ÊôÐÔÓ¦ÓÃµ½NPCÉíÉÏ
Èë¿Ú£º	pNPC: Ö¸ÏòNPCµÄÖ¸Õë
³ö¿Ú:	Ä§·¨±»Ó¦ÓÃ¡£
		¾ßÌå¹¤×÷ÓÉKNpcµÄ³ÉÔ±º¯ÊýÍê³É¡£
		KItem ¶ÔÏó±¾ÉíÃ»ÓÐ³ÉÔ±±äÁ¿±»ÐÞ¸Ä
******************************************************************************/
void KItem::ApplyHiddenMagicAttribToNPC(IN KNpc* pNPC, IN int nMagicActive) const
{
	_ASSERT(this != NULL);
	if (nMagicActive <= 0)
		return;

	const KItemNormalAttrib* pAttrib;
	pAttrib = &(m_aryMagicAttrib[(nMagicActive << 1) - 1]);	// ºó×ºÎªÒþ²ØÊôÐÔËùÒÔ³Ë2¼õÒ»
	if (pAttrib->nAttribType > 0)
	{
		pNPC->ModifyAttrib(pNPC->m_Index, (void *)pAttrib);
	}
}

/******************************************************************************
¹¦ÄÜ:	½«itemÉÏµÄµÚNÏîÒþ²ØÄ§·¨ÊôÐÔ´ÓNPCÉíÉÏÒÆ³ý
Èë¿Ú£º	pNPC: Ö¸ÏòNPCµÄÖ¸Õë£¬nMagicActive£ºµÚnÏîÄ§·¨ÊôÐÔ
³ö¿Ú:	Ä§·¨±»ÒÆ³ý¡£
		¾ßÌå¹¤×÷ÓÉKNpcµÄ³ÉÔ±º¯ÊýÍê³É¡£
		KItem ¶ÔÏó±¾ÉíÃ»ÓÐ³ÉÔ±±äÁ¿±»ÐÞ¸Ä
******************************************************************************/
void KItem::RemoveHiddenMagicAttribFromNPC(IN KNpc* pNPC, IN int nMagicActive) const
{
	_ASSERT(this != NULL);
	if (nMagicActive <= 0)
		return;

	const KItemNormalAttrib* pAttrib;
	pAttrib = &(m_aryMagicAttrib[(nMagicActive << 1) - 1]);	// ºó×ºÎªÒþ²ØÊôÐÔËùÒÔ³Ë2¼õÒ»
	if (pAttrib->nAttribType > 0)
	{
		KItemNormalAttrib RemoveAttrib;
		RemoveAttrib.nAttribType = pAttrib->nAttribType;
		RemoveAttrib.nValue[0] = -pAttrib->nValue[0];
		RemoveAttrib.nValue[1] = -pAttrib->nValue[1];
		RemoveAttrib.nValue[2] = -pAttrib->nValue[2];
		pNPC->ModifyAttrib(pNPC->m_Index, (void *)&RemoveAttrib);
	}
}

/******************************************************************************
¹¦ÄÜ:	¸ù¾ÝÅäÖÃÎÄ¼þÖÐµÄÊý¾Ý,ÎªitemµÄ¸÷Ïî¸³³õÖµ
Èë¿Ú£º	pData: ¸ø³öÀ´×ÔÅäÖÃÎÄ¼þµÄÊý¾Ý
³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, ÒÔÏÂ³ÉÔ±±äÁ¿±»Öµ:
			m_CommonAttrib,m_aryBaseAttrib,m_aryRequireAttrib
		Ê§°ÜÊ±·µ»ØÁã
ËµÃ÷:	CBR: Common,Base,Require
******************************************************************************/
BOOL KItem::SetAttrib_CBR(IN const KBASICPROP_EQUIPMENT* pData)
{
	_ASSERT(pData != NULL);
	
	BOOL bEC = FALSE;
	if (pData)
	{
		//SetAttrib_Common(pData);
		*this = *pData;		// ÔËËã·ûÖØÔØ
		SetAttrib_Base(pData->m_aryPropBasic);
		SetAttrib_Req(pData->m_aryPropReq);
		bEC = TRUE;
	}
	return bEC;
}

BOOL KItem::SetAttrib_Base(const KEQCP_BASIC* pBasic)
{
	for (int i = 0;
		 i < sizeof(m_aryBaseAttrib)/sizeof(m_aryBaseAttrib[0]); i++)
	{
		if(pBasic[i].nType > 0)
		{
		KItemNormalAttrib* pDst;
		const KEQCP_BASIC* pSrc;
		pDst = &(m_aryBaseAttrib[i]);
		pSrc = &(pBasic[i]);
		pDst->nAttribType = pSrc->nType;
		pDst->nValue[0] = ::GetRandomNumber(pSrc->sRange.nMin, pSrc->sRange.nMax);
		pDst->nValue[1] = 0;	// RESERVED
		pDst->nValue[2] = 0;	// RESERVED
		if (pDst->nAttribType == magic_durability_v)
			SetDurability(pDst->nValue[0]);
		}
	}
	if (m_nCurrentDur == 0)	// ËµÃ÷Ã»ÓÐÄÍ¾Ã¶ÈÊôÐÔ
		m_nCurrentDur = -1;
	return TRUE;
}

BOOL KItem::SetAttrib_Req(const KEQCP_REQ* pReq)
{
	for (int i = 0;
		 i < sizeof(m_aryRequireAttrib)/sizeof(m_aryRequireAttrib[0]); i++)
	{
		if(pReq[i].nType > 0)
		{
		KItemNormalAttrib* pDst;
		pDst = &(m_aryRequireAttrib[i]);
		pDst->nAttribType = pReq[i].nType;
		pDst->nValue[0] = pReq[i].nPara;
		pDst->nValue[1] = 0;	// RESERVED
		pDst->nValue[2] = 0;	// RESERVED
		}
	}
	return TRUE;
}

/******************************************************************************
¹¦ÄÜ:	¸ù¾Ý´«ÈëµÄÊý¾Ý, ÎªitemµÄÄ§·¨ÊôÐÔ¸³³õÖµ
Èë¿Ú£º	pMA: ¸ø³öÊý¾Ý
³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, ÒÔÏÂ³ÉÔ±±äÁ¿±»Öµ:
			m_aryMagicAttrib
		Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KItem::SetAttrib_MA(IN const KItemNormalAttrib* pMA, bool bExMagic)
{
	if (NULL == pMA)
		{ _ASSERT(FALSE); return FALSE; }
	if(bExMagic)
	{
		for (int i = 0; i < sizeof(m_aryMagicAttribEx) / sizeof(m_aryMagicAttribEx[0]); i++)
		{
			m_aryMagicAttribEx[i] = pMA[i];
			if (m_aryMagicAttribEx[i].nAttribType == magic_indestructible_b)
			{
				SetDurability(-1);
			}
		}
	}
	else
	{
		for (int i = 0; i < sizeof(m_aryMagicAttrib) / sizeof(m_aryMagicAttrib[0]); i++)
		{
			m_aryMagicAttrib[i] = pMA[i];
			if (m_aryMagicAttrib[i].nAttribType == magic_indestructible_b)
			{
				SetDurability(-1);
			}
		}
	}
	return TRUE;
}

/******************************************************************************
¹¦ÄÜ:	¸ù¾Ý´«ÈëµÄÊý¾Ý, ÎªitemµÄÄ§·¨ÊôÐÔ¸³³õÖµ
Èë¿Ú£º	pMA: ¸ø³öÊý¾Ý
³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, ÒÔÏÂ³ÉÔ±±äÁ¿±»Öµ:
			m_aryMagicAttrib
		Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KItem::SetAttrib_MA(IN const KMACP* pMA)
{
	if (NULL == pMA)
		{ _ASSERT(FALSE); return FALSE; }

	for (int i = 0; i < sizeof(m_aryMagicAttrib) / sizeof(m_aryMagicAttrib[0]); i++)
	{
		const KMACP* pSrc;
		KItemNormalAttrib* pDst;
		pSrc = &(pMA[i]);
		pDst = &(m_aryRequireAttrib[i]);

		pDst->nAttribType = pSrc->nPropKind;
		pDst->nValue[0] =  ::GetRandomNumber(pSrc->aryRange[0].nMin, pSrc->aryRange[0].nMax);
		pDst->nValue[1] =  ::GetRandomNumber(pSrc->aryRange[1].nMin, pSrc->aryRange[1].nMax);
		pDst->nValue[2] =  ::GetRandomNumber(pSrc->aryRange[2].nMin, pSrc->aryRange[2].nMax);
	}
	return TRUE;
}

void KItem::operator = (const KBASICPROP_EQUIPMENT& sData)
{
	KItemCommonAttrib* pCA = &m_CommonAttrib;
	pCA->nItemGenre		 = sData.m_nItemGenre;
	pCA->nDetailType	 = sData.m_nDetailType;
	pCA->nParticularType = sData.m_nParticularType;
	pCA->nObjIdx		 = sData.m_nObjIdx;
	pCA->nStack			 = 1;
	pCA->nWidth			 = sData.m_nWidth;
	pCA->nHeight		 = sData.m_nHeight;
	pCA->nPrice			 = sData.m_nPrice;
	pCA->nLevel			 = sData.m_nLevel;
	pCA->nSeries		 = sData.m_nSeries;
	::strcpy(pCA->szItemName,  sData.m_szName);
#ifndef _SERVER
	::strcpy(pCA->szImageName, sData.m_szImageName);
	::strcpy(pCA->szIntro,	   sData.m_szIntro);
	m_Image.Color.Color_b.a = 255;
	m_Image.nISPosition = IMAGE_IS_POSITION_INIT;
	m_Image.nType = ISI_T_SPR;
	::strcpy(m_Image.szImage, pCA->szImageName);
#endif
}

void KItem::operator = (const KBASICPROP_QUEST& sData)
{
	// ¸³Öµ: ¹²Í¬ÊôÐÔ²¿·Ö
	KItemCommonAttrib* pCA = &m_CommonAttrib;
	pCA->nItemGenre		 = sData.m_nItemGenre;
	pCA->nDetailType	 = sData.m_nDetailType;
	pCA->nObjIdx		 = sData.m_nObjIdx;
	pCA->nStack			 = 1;
	pCA->nMaxStack		 = sData.m_nMaxStack;
	pCA->nWidth			 = sData.m_nWidth;
	pCA->nHeight		 = sData.m_nHeight;
	pCA->nPrice			 = 0;
	pCA->nLevel			 = 1;
	::strcpy(pCA->szItemName,  sData.m_szName);
#ifdef _SERVER
	m_bCanSell = (sData.m_bCanSell>0)?TRUE:FALSE;
#endif

#ifndef _SERVER
	::strcpy(pCA->szImageName, sData.m_szImageName);
	::strcpy(pCA->szIntro,	   sData.m_szIntro);
	m_Image.Color.Color_b.a = 255;
	m_Image.nISPosition = IMAGE_IS_POSITION_INIT;
	m_Image.nType = ISI_T_SPR;
	::strcpy(m_Image.szImage, pCA->szImageName);
#endif
}

void KItem::operator = (const KBASICPROP_TOWNPORTAL& sData)
{
	// ¸³Öµ: ¹²Í¬ÊôÐÔ²¿·Ö
	KItemCommonAttrib* pCA = &m_CommonAttrib;
	pCA->nItemGenre		 = sData.m_nItemGenre;
	pCA->nObjIdx		 = sData.m_nObjIdx;
	pCA->nStack			 = 1;
	pCA->nWidth			 = sData.m_nWidth;
	pCA->nHeight		 = sData.m_nHeight;
	pCA->nPrice			 = sData.m_nPrice;
	pCA->nLevel			 = 1;
	::strcpy(pCA->szItemName,  sData.m_szName);
#ifndef _SERVER
	::strcpy(pCA->szImageName, sData.m_szImageName);
	::strcpy(pCA->szIntro,	   sData.m_szIntro);
	m_Image.Color.Color_b.a = 255;
	m_Image.nISPosition = IMAGE_IS_POSITION_INIT;
	m_Image.nType = ISI_T_SPR;
	::strcpy(m_Image.szImage, pCA->szImageName);
#endif
}

void KItem::operator = (const KBASICPROP_MASCRIPT& sData)
{
	KItemCommonAttrib* pCA = &m_CommonAttrib;
	pCA->nItemGenre		 = sData.m_nItemGenre;
	pCA->nDetailType	 = sData.m_nDetailType;
	pCA->nParticularType = sData.m_nParticularType;
	pCA->nObjIdx		 = sData.m_nObjIdx;
	pCA->nStack			 = 1;
	pCA->nMaxStack		 = sData.m_nMaxStack;
	pCA->nWidth			 = sData.m_nWidth;
	pCA->nHeight		 = sData.m_nHeight;
	pCA->nPrice			 = sData.m_nPrice;
	pCA->nLevel			 = 1;
	::strcpy(pCA->szItemName,  sData.m_szName);
#ifndef _SERVER
	m_bShowLevel = sData.m_bShowLevel;
	m_bShowSeries = sData.m_bRegSeries;
	m_bShortKey = sData.m_bShortKey;
	::strcpy(pCA->szImageName, sData.m_szImageName);
	::strcpy(pCA->szIntro,	   sData.m_szIntro);
	m_Image.Color.Color_b.a = 255;
	m_Image.nISPosition = IMAGE_IS_POSITION_INIT;
	m_Image.nType = ISI_T_SPR;
	::strcpy(m_Image.szImage, pCA->szImageName);
#endif
}

void KItem::operator = (const KBASICPROP_MEDICINE& sData)
{
	// ¸³Öµ: ¹²Í¬ÊôÐÔ²¿·Ö
	KItemCommonAttrib* pCA = &m_CommonAttrib;
	pCA->nItemGenre		 = sData.m_nItemGenre;
	pCA->nDetailType	 = sData.m_nDetailType;
	pCA->nParticularType = sData.m_nParticularType;
	pCA->nObjIdx		 = sData.m_nObjIdx;
	pCA->nStack			 = 1;
	pCA->nWidth			 = sData.m_nWidth;
	pCA->nHeight		 = sData.m_nHeight;
	pCA->nPrice			 = sData.m_nPrice;
	pCA->nLevel			 = sData.m_nLevel;
	::strcpy(pCA->szItemName,  sData.m_szName);
#ifndef _SERVER
	::strcpy(pCA->szImageName, sData.m_szImageName);
	::strcpy(pCA->szIntro,	   sData.m_szIntro);
#endif
	// ¸³Öµ: »ù±¾ÊôÐÔ²¿·Ö
	KItemNormalAttrib* pBA = m_aryBaseAttrib;
	pBA[0].nAttribType = sData.m_aryAttrib[0].nAttrib;
	pBA[0].nValue[0]   = sData.m_aryAttrib[0].nValue;
	pBA[0].nValue[1]   = sData.m_aryAttrib[0].nTime;
	pBA[1].nAttribType = sData.m_aryAttrib[1].nAttrib;
	pBA[1].nValue[0]   = sData.m_aryAttrib[1].nValue;
	pBA[1].nValue[1]   = sData.m_aryAttrib[1].nTime;
	
#ifndef _SERVER
	m_Image.Color.Color_b.a = 255;
	m_Image.nISPosition = IMAGE_IS_POSITION_INIT;
	m_Image.nType = ISI_T_SPR;
	::strcpy(m_Image.szImage, pCA->szImageName);
#endif

}

void KItem::SetAttrib_Extend(const KBASICPROP_EQUIPMENT_GOLD* pData)
{
	if(pData->m_nGroupId >= 0)
	m_nGroupId = pData->m_nGroupId;
	if(pData->m_nExGroupId >= 0)
	m_nExGroupId = pData->m_nExGroupId;
	if(pData->m_nGroupSerial >= 0)
	m_nGroupSerial = pData->m_nGroupSerial;
}

void KItem::Remove()
{
	::memset(&m_CommonAttrib,    0, sizeof(m_CommonAttrib));
	::memset(m_aryBaseAttrib,    0, sizeof(m_aryBaseAttrib));
	::memset(m_aryRequireAttrib, 0, sizeof(m_aryRequireAttrib));
	::memset(m_aryMagicAttrib,   0, sizeof(m_aryMagicAttrib));
	::memset(m_aryMagicAttribEx,   0, sizeof(m_aryMagicAttribEx));
	m_dwID = 0;
	m_nCurrentDur = 0;
	m_nExType = 0;
	m_nGenParam = 0;
	m_nGroupId = 0;		
	m_nExGroupId = 0;
	m_nGroupSerial = 0;
#ifdef _SERVER
	m_bCanSell = TRUE;
#endif

#ifndef _SERVER
	::memset(&m_Image,   0, sizeof(KRUImage));
	m_bShowLevel = 0;
	m_bShowSeries = 0;
	m_bShortKey = 0;
#endif
}

BOOL KItem::SetBaseAttrib(IN const KItemNormalAttrib* pAttrib)
{
	if (!pAttrib)
		return FALSE;

	for (int i = 0; i < sizeof(m_aryBaseAttrib) / sizeof(m_aryBaseAttrib[0]); i++)
	{
		m_aryBaseAttrib[i] = pAttrib[i];
	}
	return TRUE;
}

BOOL KItem::SetRequireAttrib(IN const KItemNormalAttrib* pAttrib)
{
	if (!pAttrib)
		return FALSE;

	for (int i = 0; i < sizeof(m_aryRequireAttrib) / sizeof(m_aryRequireAttrib[0]); i++)
	{
		m_aryRequireAttrib[i] = pAttrib[i];
	}
	return TRUE;
}

BOOL KItem::SetMagicAttrib(IN const KItemNormalAttrib* pAttrib)
{
	return SetAttrib_MA(pAttrib);
}

//------------------------------------------------------------------
//	Ä¥Ëð£¬·µ»ØÖµ±íÊ¾Ê£ÓàÄÍ¾Ã¶È
//------------------------------------------------------------------
int KItem::Abrade(IN const int nRandRange)
{
	if (m_nCurrentDur == -1 || nRandRange == 0)	// ÓÀ²»Ä¥Ëð
		return -1;

	if (g_Random(nRandRange) == 0)	// nRandRange·ÖÖ®Ò»µÄ¸ÅÂÊ
	{
		m_nCurrentDur--;
		if (m_nCurrentDur == 0)
		{
			return 0;
		}
	}
	return m_nCurrentDur;
}

#ifndef _SERVER
void KItem::Paint(int nX, int nY)
{
	m_Image.oPosition.nX = nX;
	m_Image.oPosition.nY = nY;
	m_Image.bRenderStyle = IMAGE_RENDER_STYLE_ALPHA;
	g_pRepresent->DrawPrimitives(1, &m_Image, RU_T_IMAGE, TRUE);
	if(m_CommonAttrib.nMaxStack > 0)
	{
		char szBuff[16];
		sprintf(szBuff, "%d", m_CommonAttrib.nStack);
		g_pRepresent->OutputText(12, szBuff, KRF_ZERO_END, nX+27-strlen(szBuff)*6, nY+12,
			0xffffffff, 0, TEXT_IN_SINGLE_PLANE_COORD, 0xff000000);
	}
}

static char g_szActSeries[][32] =
{
	"<color=Earth>Thæ<color>",
	"<color=Water>Thñy<color>",
	"<color=Metal>Kim<color>",
	"<color=Wood>Méc<color>",
	"<color=Fire>Háa<color>",
};

void KItem::GetDesc(char* pszMsg, bool bShowPrice, int nPriceScale, int nActiveAttrib,
					BOOL bActivateAll, int nUiType)
{
	char szTemp[128];
	if (m_CommonAttrib.nItemGenre == item_equip)
	{
		if(m_nExType == extype_gold)	//hoµng kim
		{
			strcpy(pszMsg, "<color=Yellow>");
		}
		else if(m_nExType == extype_platina)	//b¹ch kim
		{
			strcpy(pszMsg, "<color=Yellow><bclr=Blue>");
		}
		else if(m_nExType == extype_purple)	//tÝm
		{
			strcpy(pszMsg, "<color=Purple>");
		}
		else if (m_aryMagicAttrib[0].nAttribType)	// xanh
		{
			strcpy(pszMsg, "<color=Blue>");
		}
		else	//tr¾ng
		{
			strcpy(pszMsg, "<color=White>");
		}
	}
	else if(m_CommonAttrib.nItemGenre == item_task)
	{
		strcpy(pszMsg, "<color=Yellow>");
	}
	else
	{
		strcpy(pszMsg, "<color=White>");
	}
		
	strcat(pszMsg, m_CommonAttrib.szItemName);
	if (m_CommonAttrib.nItemGenre == item_equip)
	{
		if(m_nExType == extype_platina && m_CommonAttrib.nLevel >= 0)
		{
			sprintf(szTemp, " +%d", m_CommonAttrib.nLevel);
			strcat(pszMsg, szTemp);
		}
		else if(m_CommonAttrib.nLevel > 0)
		{
			sprintf(szTemp, " [CÊp %d]", m_CommonAttrib.nLevel);
			strcat(pszMsg, szTemp);
		}
	}
	else if(m_CommonAttrib.nItemGenre == item_mascript)
	{
		if(m_bShowLevel)
		{
			sprintf(szTemp, " [CÊp %d]", m_CommonAttrib.nLevel);
			strcat(pszMsg, szTemp);
		}
	}
	strcat(pszMsg, "<bclr>\n");
	if (bShowPrice && nPriceScale > 0)
	{
		sprintf(szTemp, "<color=White>Gi¸ c¶: %d ", m_CommonAttrib.nPrice / nPriceScale);
		strcat(pszMsg, szTemp);
		strcat(pszMsg, "l­îng<color>");
		strcat(pszMsg, "\n");
	}
	if ((m_CommonAttrib.nItemGenre == item_equip)
	|| (m_CommonAttrib.nItemGenre == item_mascript && m_bShowSeries))
	{
		switch(m_CommonAttrib.nSeries)
		{
		case series_metal:
			strcat(pszMsg, "<color=White>Thuéc tÝnh ngò hµnh: <color=Metal>Kim");
			break;
		case series_wood:
			strcat(pszMsg, "<color=White>Thuéc tÝnh ngò hµnh: <color=Wood>Méc");
			break;
		case series_water:
			strcat(pszMsg, "<color=White>Thuéc tÝnh ngò hµnh: <color=Water>Thñy");
			break;
		case series_fire:
			strcat(pszMsg, "<color=White>Thuéc tÝnh ngò hµnh: <color=Fire>Háa");
			break;
		case series_earth:
			strcat(pszMsg, "<color=White>Thuéc tÝnh ngò hµnh: <color=Earth>Thæ ");
			break;
		}
	}
	strcat(pszMsg, "\n");
	strcat(pszMsg, "<color=White>");
	strcat(pszMsg, m_CommonAttrib.szIntro);
	strcat(pszMsg, "\n");
	int i;
	for (i = 0; i < 7; i++)
	{
		if (!m_aryBaseAttrib[i].nAttribType)
		{
			continue;
		}
		if (m_aryBaseAttrib[i].nAttribType == magic_durability_v)
		{
			if (m_nCurrentDur == -1)
				sprintf(szTemp, "<color=Yellow>Kh«ng thÓ ph¸ hñy<color=White>");
			else
				sprintf(szTemp, "§é bÒn: %d / %d", GetDurability(), GetMaxDurability());
			strcat(pszMsg, szTemp);
		}
		else
		{
			char* pszInfo = (char *)g_MagicDesc.GetDesc(&m_aryBaseAttrib[i]);
			if (!pszInfo || !pszInfo[0])
				continue;
			strcat(pszMsg, pszInfo);
		}
		strcat(pszMsg, "\n");
	}
	for (i = 0; i < 6; i++)
	{
		if (!m_aryRequireAttrib[i].nAttribType)
		{
			continue;
		}
		char* pszInfo = (char *)g_MagicDesc.GetDesc(&m_aryRequireAttrib[i]);
		if (!pszInfo || !pszInfo[0])
			continue;
		if (Player[CLIENT_PLAYER_INDEX].m_ItemList.EnoughAttrib(&m_aryRequireAttrib[i]))
		{
			strcat(pszMsg, "<color=White>");
		}
		else
		{
			strcat(pszMsg, "<color=Red>");
		}
		strcat(pszMsg, pszInfo);
		strcat(pszMsg, "\n");
	}

	for (i = 0; i < 6; i++)
	{
		bool bCont = false;
		if (!m_aryMagicAttrib[i].nAttribType)
			bCont = true;
		char* pszInfo = (char *)g_MagicDesc.GetDesc(&m_aryMagicAttrib[i]);
		if (!pszInfo || !pszInfo[0])
			bCont = true;
		if(bCont && m_nExType != extype_purple)
			continue;
		if(m_nExType == extype_gold || m_nExType == extype_platina)
		{
			if ((i & 1) == 0 || m_CommonAttrib.nDetailType >= equip_horse)
			{
				strcat(pszMsg, "<color=Yellow>");
			}
			else
			{
				if ((i>>1) < nActiveAttrib)
				{
					strcat(pszMsg, "<color=Yellow>");
				}
				else
				{
					strcat(pszMsg, "<color=0x787800>");
				}
			}
		}
		else if(m_nExType == extype_purple)
		{
			if(!bCont)
			{
				if ((i & 1) == 0 || m_CommonAttrib.nDetailType >= equip_horse)
				{
					strcat(pszMsg, "<color=Purple>");
				}
				else
				{
					if ((i>>1) < nActiveAttrib)
					{
						strcat(pszMsg, "<color=Purple>");
					}
					else
					{
						strcat(pszMsg, "<color=0x643278>");
					}
				}
			}
			else if(i < m_nGenParam)
			{
				strcat(pszMsg, "<color=yellow>Ch­a kh¶m n¹m\n");
			}
		}
		else if(m_CommonAttrib.nItemGenre == item_equip)
		{
			if ((i & 1) == 0 || m_CommonAttrib.nDetailType >= equip_horse)
			{
				strcat(pszMsg, "<color=HBlue>");
			}
			else
			{
				if ((i>>1) < nActiveAttrib)
				{
					strcat(pszMsg, "<color=HBlue>");
				}
				else
				{
					strcat(pszMsg, "<color=DBlue>");
				}
			}
		}
		if(!bCont)
		{
			strcat(pszMsg, pszInfo);
			strcat(pszMsg, "\n");
		}
	}
	
	if(m_nExType == extype_gold || m_nExType == extype_platina)
	{
		for (i = 0; i < 2; i++)
		{
			if (!m_aryMagicAttribEx[i].nAttribType)
				continue;
			char* pszInfo = (char *)g_MagicDesc.GetDesc(&m_aryMagicAttribEx[i]);
			if (!pszInfo || !pszInfo[0])
				continue;
			if(i == 0)
				strcat(pszMsg, "\n");
			if(m_nExType == extype_gold)
			{
				if (bActivateAll)
				{
					strcat(pszMsg, "<color=0xec9cc9>");
				}
				else
				{
					strcat(pszMsg, "<color=0x787800>");
				}
			}
			else
			{
				if (bActivateAll)
				{
					strcat(pszMsg, "<color=White><bclr=Blue>");
				}
				else
				{
					strcat(pszMsg, "<color=0x787800>");
				}
			}
			strcat(pszMsg, pszInfo);
			strcat(pszMsg, "<bclr>\n");
		}
	}
	
	if(m_nExType == extype_gold && m_CommonAttrib.nDetailType < equip_horse)
	{
		if(m_nGroupId && !m_nExGroupId && !m_nGroupSerial)
		{
			int nCount = 0;
			int nBgIdx = ItemGen.FindGoldStartId(m_nGroupId);
			if(nBgIdx > 0)
			{
				--nBgIdx;
				int nEndIdx = -1;
				for(int e = nBgIdx; e < ItemGen.GetGoldItemNumber(); ++e)
				{
					const KBASICPROP_EQUIPMENT_GOLD* p = ItemGen.GetGoldItemRecord(e);
					if(p->m_nGroupId != m_nGroupId)
					{
						nEndIdx = e-1;
						break;
					}
				}
				if(nEndIdx < 0)
					nEndIdx = ItemGen.GetGoldItemNumber()-1;
				nCount = nEndIdx + 1 - nBgIdx;
			}
			if(nCount > 0)
			{
				strcat(pszMsg, "\n");
				int nMainBg = -1;
				int b;
				for(b = m_nGenParam-1; b >= 0; --b)
				{
					const KBASICPROP_EQUIPMENT_GOLD* p = ItemGen.GetGoldItemRecord(b);
					if(p->m_nGroupId != m_nGroupId)
					{
						nMainBg = b+1;
						break;
					}
				}
				if(nMainBg < 0)
					nMainBg = 0;
				if((m_nGenParam-1) - b > nCount)	//2 bé cïng group n»m gÇn nhau
					nMainBg += nCount;
				for(i = nMainBg; i < nMainBg+nCount; ++i)
				{
					const KBASICPROP_EQUIPMENT_GOLD* p = ItemGen.GetGoldItemRecord(i);
					bool bLight = false;
					for (int k = 0; k < itempart_horse; ++k)
					{
						int nIdx = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipment(k);
						if(nIdx > 0 && p->m_nGroupId == Item[nIdx].m_nGroupId)
						{
							if(i == Item[nIdx].m_nGenParam - 1)
							{
								bLight = true;
								break;
							}
							int nCheckBg = -1;
							int d;
							for(d = Item[nIdx].m_nGenParam-1; d >= 0; --d)
							{
								const KBASICPROP_EQUIPMENT_GOLD* p2 = ItemGen.GetGoldItemRecord(d);
								if(p2->m_nGroupId != Item[nIdx].m_nGroupId)
								{
									nCheckBg = d+1;
									break;
								}
							}
							if(nCheckBg < 0)
								nCheckBg = 0;
							if((Item[nIdx].m_nGenParam-1) - d > nCount)
								nCheckBg += nCount;
							int nDist = nCheckBg - nMainBg;
							if(i == (Item[nIdx].m_nGenParam - 1) - nDist)
							{
								bLight = true;
								break;
							}
						}
					}
					if(bLight)
						strcat(pszMsg, "<color=Green>");
					else
						strcat(pszMsg, "<color=0x007800>");
					strcat(pszMsg, p->m_szName);
					strcat(pszMsg, "\n");
				}
			}
		}
		else if(m_nGroupId && m_nExGroupId && m_nGroupSerial)
		{
			int nBgIdx = ItemGen.FindGoldStartId(m_nGroupId);
			if(nBgIdx > 0)
			{
				--nBgIdx;
				int aryGrSerieal[10];
				int aryGrSerRow[10];
				ZeroMemory(aryGrSerieal, sizeof(int)*10);
				int nCount = 0;
				for(int e = nBgIdx; e < ItemGen.GetGoldItemNumber(); ++e)
				{
					const KBASICPROP_EQUIPMENT_GOLD* p = ItemGen.GetGoldItemRecord(e);
					if(p->m_nGroupId != m_nGroupId)
						break;
					bool bExist = false;
					for(int s = 0; s < nCount; ++s)
					{
						if(aryGrSerieal[s] == p->m_nGroupSerial)
						{
							bExist = true;
							break;
						}
					}
					if(!bExist)
					{
						aryGrSerieal[nCount] = p->m_nGroupSerial;
						aryGrSerRow[nCount] = e;
						++nCount;
						if(nCount >= 10)
							break;
					}
				}
				if(nCount > 0)
				{
					strcat(pszMsg, "\n");
					for(i = 0; i < nCount; ++i)
					{
						const KBASICPROP_EQUIPMENT_GOLD* p = ItemGen.GetGoldItemRecord(aryGrSerRow[i]);
						bool bLight = false;
						for (int k = 0; k < itempart_horse; ++k)
						{
							int nIdx = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipment(k);
							if(nIdx > 0 && p->m_nExGroupId == Item[nIdx].m_nExGroupId
							&& p->m_nGroupSerial == Item[nIdx].m_nGroupSerial)
							{
								bLight = true;
								break;
							}
						}
						if(bLight)
							strcat(pszMsg, "<color=Green>");
						else
							strcat(pszMsg, "<color=0x007800>");
						strcat(pszMsg, p->m_szName);
						strcat(pszMsg, "\n");
					}
				}
			}
		}
	}
	if(nUiType == UOC_EQUIPTMENT && m_CommonAttrib.nDetailType < equip_horse)
	{
		int nSeries = m_CommonAttrib.nSeries;
		if(nSeries < 0 || nSeries >= series_num)
			nSeries = 0;
		int nItemPart = -1;
		for (i = 0; i < itempart_horse; ++i)
		{
			int nIdx = Player[CLIENT_PLAYER_INDEX].m_ItemList.GetEquipment(i);
			if(nIdx > 0 && this == &Item[nIdx])
			{
				nItemPart = i;
				break;
			}
		}
		switch (nItemPart)
		{
			case itempart_head:
			case itempart_weapon:
				strcat(pszMsg, "\n");
				strcat(pszMsg, "<color=yellow>CÇn hÖ ");
				strcat(pszMsg, g_szActSeries[nSeries]);
				strcat(pszMsg, " cña Trang Phôc vµ D©y ChuyÒn ®Ó kÝch thÝch thuéc tÝnh ©m\n");
			break;
			case itempart_foot:
			case itempart_ring1:
				strcat(pszMsg, "\n");
				strcat(pszMsg, "<color=yellow>CÇn hÖ ");
				strcat(pszMsg, g_szActSeries[nSeries]);
				strcat(pszMsg, " cña Mò vµ Vò KhÝ ®Ó kÝch thÝch thuéc tÝnh ©m\n");
			break;
			case itempart_cuff:
			case itempart_pendant:
				strcat(pszMsg, "\n");
				strcat(pszMsg, "<color=yellow>CÇn hÖ ");
				strcat(pszMsg, g_szActSeries[nSeries]);
				strcat(pszMsg, " cña Giµy vµ NhÉn Trªn ®Ó kÝch thÝch thuéc tÝnh ©m\n");
			break;
			case itempart_belt:
			case itempart_ring2:
				strcat(pszMsg, "\n");
				strcat(pszMsg, "<color=yellow>CÇn hÖ ");
				strcat(pszMsg, g_szActSeries[nSeries]);
				strcat(pszMsg, " cña Ngäc Béi vµ Bao Cæ Tay ®Ó kÝch thÝch thuéc tÝnh ©m\n");
			break;
			case itempart_body:
			case itempart_amulet:
				strcat(pszMsg, "\n");
				strcat(pszMsg, "<color=yellow>CÇn hÖ ");
				strcat(pszMsg, g_szActSeries[nSeries]);
				strcat(pszMsg, " cña Th¾t L­ng vµ NhÉn D­íi ®Ó kÝch thÝch thuéc tÝnh ©m\n");
			break;
		}
	}
}
#endif

int KItem::GetMaxDurability()
{
	for (int i = 0; i < 7; i++)
	{
		if (m_aryBaseAttrib[i].nAttribType == magic_durability_v)
		{
			return m_aryBaseAttrib[i].nValue[0];
		}
	}
	return -1;
}

int KItem::GetTotalMagicLevel()
{
	int nRet = 0;
	for (int i = 0; i < 6; i++)
	{
		if(m_aryBaseAttrib[i].nAttribType)
			nRet++;
	}
	return nRet;
}

int KItem::GetRepairPrice()
{
	if (ItemSet.m_sRepairParam.nMagicScale == 0)
		return 0;

	if (GetGenre() != item_equip)
		return 0;

	if (m_nCurrentDur == -1)
		return 0;

	int nMaxDur = GetMaxDurability();
	int nSumMagic = GetTotalMagicLevel();

	if (nMaxDur <= 0)
		return 0;


	return m_CommonAttrib.nPrice * ItemSet.m_sRepairParam.nPriceScale / 100 * (nMaxDur - m_nCurrentDur) / nMaxDur * (ItemSet.m_sRepairParam.nMagicScale + nSumMagic) / ItemSet.m_sRepairParam.nMagicScale;
}

BOOL KItem::CanBeRepaired()
{
	if (GetGenre() != item_equip)
		return FALSE;

	if (m_nCurrentDur == -1)
		return FALSE;

	int nMaxDur = GetMaxDurability();
	if (m_nCurrentDur == nMaxDur)
		return FALSE;

	return TRUE;
}