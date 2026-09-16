//---------------------------------------------------------------------------
// Sword3 Core (c) 2002 by Kingsoft
//
// File:	KBasPropTbl.CPP
// Date:	2002.08.14
// Code:	DongBo
// Desc:    cpp file. ±¾ÎÄ¼şÊµÏÖµÄÀàÓÃÓÚ´Ótab fileÖĞ¶Á³öµÀ¾ßµÄ³õÊ¼ÊôĞÔ,
//			²¢Éú³É¶ÔÓ¦µÄÊôĞÔ±í
//---------------------------------------------------------------------------

#include "KCore.h"
#include "KTabFile.h"
#include "MyAssert.H"
#include "KBasPropTbl.h"

#define		TABFILE_PATH				"\\settings\\item"
#define		TABFILE_TASK				"questkey.txt"
#define		TABFILE_MEDICINE			"potion.txt"
#define		TABFILE_MELEEWEAPON			"MeleeWeapon.txt"
#define		TABFILE_RANGEWEAPON			"RangeWeapon.txt"
#define		TABFILE_ARMOR				"Armor.txt"
#define		TABFILE_HELM				"Helm.txt"
#define		TABFILE_BOOT				"Boot.txt"
#define		TABFILE_BELT				"Belt.txt"
#define		TABFILE_AMULET				"Amulet.txt"
#define		TABFILE_RING				"Ring.txt"
#define		TABFILE_CUFF				"Cuff.txt"
#define		TABFILE_PENDANT				"Pendant.txt"
#define		TABFILE_HORSE				"Horse.txt"
#define		TABFILE_MASK				"Mask.txt"
#define		TABFILE_TOWNPORTAL			"TownPortal.txt"
#define		TABFILE_MAGICATTRIB			"magicattrib.txt"
#define		TABFILE_GOLDITEM			"GoldEquip.txt"
#define		TABFILE_GOLDMAGIC			"magicattrib_ge.txt"
#define		TABFILE_SUITACTCOUNT		"suite_activate_count.txt"
#define		TABFILE_MAGICSCRIPT			"magicscript.txt"
// ÒÔÏÂ¶¨ÒåµÄ½á¹¹ÓÃÓÚ¸¨Öú´ÓtabfileÖĞ¶Á³öÊôĞÔµÄ³õÊ¼Öµ
typedef struct tagPROPINFO
{
	int		m_nColumn;
	int		m_nType;		// ÊôĞÔµÄÀàĞÍ. Ïê¼û PI_VARTYPE_...ÏµÁĞ¶¨Òå
	union
	{
	char*	m_pszBuf;		// Ö¸Ïò×Ö·û´®»º³åÇøµÄÖ¸Õë
	int*	m_pnData;		// Ö¸Ïòint±äÁ¿µÄÖ¸Õë
	}m_pData;
	int		m_nBufSize;		// »º³åÇøµÄ³¤¶È
} PROPINFO;
#define		PI_VARTYPE_CHAR		0
#define		PI_VARTYPE_INT		1

char*	TABFILE_EQUIPMENT[] = 
{
	TABFILE_MELEEWEAPON,	//"MeleeWeapon.txt",
	TABFILE_RANGEWEAPON,	//"RangeWeapon.txt",
	TABFILE_ARMOR,			//"Armor.txt",
	TABFILE_HELM,			//"Helm.txt",
	TABFILE_BOOT,			//"Boot.txt",
	TABFILE_BELT,			//"Belt.txt",
	TABFILE_AMULET,			//"Amulet.txt",
	TABFILE_RING,			//"Ring.txt",
	TABFILE_CUFF,			//"Cuff.txt",
	TABFILE_PENDANT,		//"Pendant.txt",
	TABFILE_HORSE,			//"Horse.txt",
	TABFILE_MASK,			//"Mask.txt",
};

//int GetRandomNumber(int nMin, int nMax);

//=============================================================================

/******************************************************************************
	¹¦ÄÜ:	´Ótab fileÖĞ¶ÁÈëÌØ¶¨µÄÊı¾İ¼ÇÂ¼
	Èë¿Ú:	pTF: ¹¤¾ßÀàÖ¸Õë, ÓÃ´Ë¹¤¾ßÀà¶ÁÈ¡tab file
			nRow: ¶ÁµÚnRowÏî¼ÇÂ¼
			pPI[i].m_nType: ¸ø³öÓû¶ÁµÄ¼ÇÂ¼ÖĞµÚi¸öÓòµÄÀàĞÍ, ¿ÉÄÜÊÇÕûĞÍ»ò×Ö·û´®
			pPI[i].m_pData: ½«Êı¾İ¶Áµ½´Ë»º³åÇøÖĞ
			cbFields: Ã¿Ïî¼ÇÂ¼°üº¬ÕâÃ´¶àµÄÓò
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, m_pBuf Ö¸Ïò·ÖÅäµÄÄÚ´æ
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL LoadRecord(IN KTabFile* pTF, IN int nRow,
				IN OUT const PROPINFO* pPI, IN int cbFields)
{
	BOOL bEC = TRUE;

	nRow += 2;	// ¼Ó1: Ìø¹ıtabfileµÄµÚÒ»ĞĞ. ¸ÃĞĞ¸ø³öµÄÊÇ¸÷ÁĞµÄÃû³Æ
				// ÔÙ¼Ó1: KTabFile::GetInteger()º¯ÊıÒªÇónRow´Ó1¿ªÊ¼ËãÆğ

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	for (int n = 0; n < cbFields; n++)
	{
		if (PI_VARTYPE_INT == (pPI+n)->m_nType)
		{	// ¶ÁÈë int ĞÍÊı¾İ
			if (FALSE == pTF->GetInteger(nRow, (pPI+n)->m_nColumn, -1, (pPI+n)->m_pData.m_pnData))
				{ _ASSERT(FALSE); bEC = FALSE; break; }
		}
		else
		{	// ¶ÁÈë×Ö·û´®ĞÍÊı¾İ
			_ASSERT(PI_VARTYPE_CHAR == (pPI+n)->m_nType);

			if (FALSE == pTF->GetString(nRow, (pPI+n)->m_nColumn, "", (pPI+n)->m_pData.m_pszBuf,
										(pPI+n)->m_nBufSize))
				{ _ASSERT(FALSE); bEC = FALSE; break; }
		}
	}
	return bEC;
}

//=============================================================================

KLibOfBPT::KLibOfBPT()
{
	memset(m_GoldStartId, 0, sizeof(int)*MAX_GOLD_GROUP);
}

KLibOfBPT::~KLibOfBPT()
{
}

/******************************************************************************
	¹¦ÄÜ£º	×Ü¿Ø£¬´Ótab fileÖĞ¶ÁÈëµÀ¾ß,Ä§·¨µÈÔ­Ê¼Êı¾İ
	³ö¿Ú:	Ïà¹ØÊı¾İ´æÔÚm_BPTWeapon,m_BPTWeaponDirtyµÈ³ÉÔ±±äÁ¿ÖĞ
******************************************************************************/
BOOL KLibOfBPT::Init()
{
	// ³õÊ¼»¯
	KBasicPropertyTable*	paryBPT[] = {	
											&m_BPTMeleeWeapon,
											&m_BPTRangeWeapon,
											&m_BPTArmor,
											&m_BPTHelm,
											&m_BPTBoot,
											&m_BPTBelt,
											&m_BPTAmulet,
											&m_BPTRing,
											&m_BPTCuff,
											&m_BPTPendant,
											&m_BPTHorse,
											&m_BPTMask,
											&m_BPTMedicine,
											&m_BPTQuest,
											&m_BPTTownPortal,
											&m_BPTMagicAttrib,
											&m_GoldItem,
											&m_GoldMagic,
											&m_SuitActCount,
											&m_BPTMAScript,
										};
	// ½«tab fileÖğ¸ö¶ÁÈë
	const int cbNumOfTables = sizeof(paryBPT)/sizeof(paryBPT[0]);
	for (int i = 0; i < cbNumOfTables; i++)
	{
		if (i < 12)	// tõ mask ®æ l¹i
		{
			KBPT_Equipment* pTemp = (KBPT_Equipment *)paryBPT[i];
			pTemp->Init(i);
		}
		if (FALSE == paryBPT[i]->Load())
			{ _ASSERT(FALSE); return FALSE; }
	}
	// ¹¹ÔìÄ§·¨ÊôĞÔ±í
	InitMALib();
    // ¹¹ÔìÄ§·¨ÊôĞÔË÷Òı±í
    // add by Freeway Chen in 2003.5.30
    InitMAIT();
	return TRUE;
}

/******************************************************************************
	¹¦ÄÜ£º	½¨Á¢Ò»¸öËÄÎ¬µÄÊı×é£¬ÀûÓÃ(Ç°ºó×º¡¢ÎïÆ·ÀàĞÍ¡¢ÎåĞĞ¡¢¼¶±ğ)
            È·¶¨Ò»¸öÄ§·¨ÊôĞÔµÄË÷ÒıÖµÁĞ±í
	Èë¿Ú:	m_BPTMagicAttrib: ÄÚº¬´Ótab file¶ÁÈëµÄÈ«²¿Ä§·¨ÊôĞÔ
	³ö¿Ú:	ËÄÎ¬Êı×é m_CMAIT ·Ö±ğÌî³äÏàÓ¦µÄË÷ÒıÖµÁĞ±í
******************************************************************************/
// Add by Freeway Chen in 2003.5.30
BOOL KLibOfBPT::InitMAIT()
{
    int nResult = false;
    int i = 0;
    int nPrefixPostfix = 0;
    int nType = 0;
    int nSeries = 0;
    int nSeriesMin = 0;
    int nSeriesMax = 0;

    int nLevel  = 0;
    int nLevelMin = 0;
    int nLevelMax = 0;

    //int nTotalCount = 0;

    for (i = 0; i < m_BPTMagicAttrib.NumOfEntries(); i++)
    {
        KMAGICATTRIB_TABFILE *pItem = (KMAGICATTRIB_TABFILE *)m_BPTMagicAttrib.GetRecord(i);
        if (!pItem)
        {
            _ASSERT(pItem);
            continue;
        }

        pItem->m_nUseFlag = false;

        nPrefixPostfix = pItem->m_nPos;
        _ASSERT((nPrefixPostfix >= 0) && (nPrefixPostfix < MATF_PREFIXPOSFIX));

        for (nType = 0; nType < MATF_CBDR; nType++)
        {
            if ((pItem->m_DropRate[nType]) == 0)
                continue; // Èç¹ûÃ»ÓĞ¸ÅÂÊ³öÏÖÕâ¸öÀàĞÍ¾ÍÌøµ½ÏÂÒ»¸ö
            
            nSeriesMin = nSeriesMax = pItem->m_nClass;
            if ((pItem->m_nClass) == -1)
            {
                nSeriesMin = 0;
                nSeriesMax = MATF_SERIES - 1;
            }
            else
            {
                _ASSERT(((pItem->m_nClass) >= 0) && ((pItem->m_nClass) < MATF_SERIES));
            }

            for (nSeries = nSeriesMin; nSeries <= nSeriesMax; nSeries++)
            {
                nLevelMin = pItem->m_nLevel;
                nLevelMax = MATF_LEVEL;

                for (nLevel = nLevelMin; nLevel <= nLevelMax; nLevel++)
                {
                    m_CMAIT[nPrefixPostfix][nType][nSeries][nLevel - 1].Insert(i);
                    //nTotalCount++;
                }
            }
        }
    }

    //#ifdef _DEBUG
    #if 0
    //g_DebugLog("[Ä§·¨ÊôĞÔ]%sÎå·À¿¹ĞÔÉÏÏŞÔö¼Ó%d", pNpc->Name, pMagic->nValue[0]);
    printf("[Ä§·¨ÊôĞÔ]m_CMAIT[Ç°×ººó×º][ÀàĞÍ][ÎåĞĞ][¼¶±ğ]\n");
    for (nPrefixPostfix = 0; nPrefixPostfix < MATF_PREFIXPOSFIX; nPrefixPostfix++)
    {
        for (nType = 0; nType < MATF_CBDR; nType++)
        {
            for (nSeries = 0; nSeries < MATF_SERIES; nSeries++)
            {
                for (nLevel = 1; nLevel < (MATF_LEVEL + 1); nLevel++)
                {
                    KBPT_ClassMAIT *pMAITItem = &m_CMAIT[nPrefixPostfix][nType][nSeries][nLevel - 1];

                    char szOutputString[8192];
                    char szStringContent[4096];
                    szStringContent[0] = '\0';

                    for (i = 0; i < pMAITItem->GetCount(); i++)
                    {
                        KMAGICATTRIB_TABFILE *pItem = (KMAGICATTRIB_TABFILE *)m_BPTMagicAttrib.GetRecord(pMAITItem->Get(i));
                        char szTemp[1024];
                        sprintf(szTemp, " %3d(%-8s) ", pMAITItem->Get(i) + 2, pItem->m_szName);

                        strcat(szStringContent, szTemp);
                    }

                    sprintf(szOutputString, 
                        "[Ä§·¨ÊôĞÔ]m_CMAIT[%d][%d][%d][%d]: Count = %3d, %s  \n",
                        nPrefixPostfix,
                        nType,
                        nSeries,
                        nLevel - 1,
                        pMAITItem->GetCount(),
                        szStringContent
                    );
                    printf("%s", szOutputString);
                    //g_DebugLog("%s", szOutputString);
                    //OutputDebugString(szOutputString);

                }
            }
        }
    }
    ExitProcess(0); // for redirect to File save

    #endif // _DEBUG


    nResult = true;
//Exit0:
    return nResult;
}


/******************************************************************************
	¹¦ÄÜ£º	¸ù¾İÔ­Ê¼µÄÄ§·¨ÊôĞÔ±í£¬Í³¼Æ³öÃ¿ÖÖ×°±¸¸÷ÓĞÄÄĞ©¿ÉÄÜµÄÄ§·¨ÊôĞÔ
	Èë¿Ú:	m_BPTMagicAttrib: ÄÚº¬´Ótab file¶ÁÈëµÄÈ«²¿Ä§·¨ÊôĞÔ
	³ö¿Ú:	Ô­Ê¼µÄÄ§·¨ÊôĞÔ±í±»·ÖÀà, ·ÖÀàºóµÄÊı¾İ´æÈëm_CMATÊı×éÖĞ
			m_CMAT[i]ÖĞ¸ø³öÁËÊÊÓÃÓÚµÚiÖÖ×°±¸µÄÈ«²¿Ä§·¨ÊôĞÔ
******************************************************************************/
BOOL KLibOfBPT::InitMALib()
{
	BOOL bEC = FALSE;
	// È·¶¨Ã¿ÖÖ×°±¸¸÷ÓĞ¶àÉÙÖÖ¿ÉÄÜµÄÄ§·¨ÊôĞÔ
    int naryMACount[2][MATF_CBDR];	// µÚiÖÖ×°±¸¹²ÓĞnaryMACount[0][i]ÖÖ¿ÉÄÜµÄÇ°×º
									//			  ºÍnaryMACount[1][i]ÖÖ¿ÉÄÜµÄºó×º
	m_BPTMagicAttrib.GetMACount((int*)naryMACount);
	// ¸ù¾İÈ¡»ØµÄÊıÖµ£¬Îª¸÷×°±¸µÄÄ§·¨ÊôĞÔË÷Òı±í·ÖÅäÄÚÈİ
	for (int i = 0; i < MATF_CBDR; i++)	// -1 because of horse
	{
		_ASSERT(naryMACount[0][i] > 0);	// ²»¿ÉÄÜÃ»ÓĞÒ»¸öÄ§·¨Ç°×ºÊÊÓÃÓÚ¸Ã×°±¸
		_ASSERT(naryMACount[1][i] > 0);	// ²»¿ÉÄÜÃ»ÓĞÒ»¸öÄ§·¨ºó×ºÊÊÓÃÓÚ¸Ã×°±¸
		if (FALSE == m_CMAT[0][i].GetMemory(naryMACount[0][i]))
			return bEC;
		if (FALSE == m_CMAT[1][i].GetMemory(naryMACount[1][i]))
			return bEC;
	}
	// ±éÀúÄ§·¨ÊôĞÔ±í£¬½¨Á¢Æğ¸÷×°±¸µÄÄ§·¨ÊôĞÔË÷Òı±í
	const int nNumOfMA = m_BPTMagicAttrib.NumOfEntries();	// Ä§·¨ÊôĞÔµÄ×ÜÊı
	_ASSERT(nNumOfMA > 0);
	int m, n;		// µÚmÏîÄ§·¨ÊôĞÔÊÊÓÃÓÚµÚnÖÖ×°±¸
	for (m = 0; m < nNumOfMA; m++)				// ±éÀúËùÓĞÄ§·¨ÊôĞÔ
	{
		const KMAGICATTRIB_TABFILE* pRec;
		pRec = m_BPTMagicAttrib.GetRecord(m);		
		for (n = 0; n < MATF_CBDR; n++)			// ¹²ÓĞMATF_CBDRÖÖ×°±¸´øÄ§·¨
		{	// È·ÈÏÄ§·¨ÊôĞÔÊÊÓÃÓÚÄÄĞ©×°±¸
			if (0 != pRec->m_DropRate[n])
			{	// ÔËĞĞÖÁ´Ë, ËµÃ÷µÚmÏîÄ§·¨ÊôĞÔÊÊÓÃÓÚµÚnÖÖ×°±¸
				int nPos;
				nPos = (0 == pRec->m_nPos) ? 1 : 0;
				m_CMAT[nPos][n].Set(m);
			}
		}
	}
	bEC = TRUE;
	return bEC;
}

const KBASICPROP_EQUIPMENT_GOLD* KLibOfBPT::GetGoldItemRecord(IN int nIndex) const
{
	return (KBASICPROP_EQUIPMENT_GOLD*)m_GoldItem.GetRecord(nIndex);
}

const MAATTRIB_GOLDEQUIP* KLibOfBPT::GetGoldMagicRecord(IN int nIndex) const
{
	return (MAATTRIB_GOLDEQUIP*)m_GoldMagic.GetRecord(nIndex);
}

const int KLibOfBPT::GetGoldItemNumber() const
{
	return m_GoldItem.GetRecordCount();
}

int KLibOfBPT::FindGoldStartId(int nGroupId)
{
	if(nGroupId <= 0 || nGroupId >= MAX_GOLD_GROUP)
		return 0;
	if(m_GoldStartId[nGroupId])
		return m_GoldStartId[nGroupId];
	for(int i=0;i<m_GoldItem.GetRecordCount();++i)
	{
		const KBASICPROP_EQUIPMENT_GOLD* p = m_GoldItem.GetRecord(i);
		if(p && p->m_nGroupId == nGroupId)
		{
			m_GoldStartId[nGroupId] = i+1;
			break;
		}
	}
	return m_GoldStartId[nGroupId];
}

const KEQCP_REQ* KLibOfBPT::GetSuitACRecord(IN int nIndex) const
{
	return (KEQCP_REQ*)m_SuitActCount.GetRecord(nIndex);
}

const int KLibOfBPT::GetSuitACNumber() const
{
	return m_SuitActCount.GetRecordCount();
}

/******************************************************************************
	¹¦ÄÜ£º	»ñÈ¡Ö¸¶¨µÄCMAT
******************************************************************************/
const KBPT_ClassifiedMAT* KLibOfBPT::GetCMAT(int nPos, int i) const
{
	_ASSERT(this != NULL);

	if (nPos != 0 && nPos != 1)
		{ _ASSERT(FALSE); return NULL; }
	if (i < 0 || i >= MATF_CBDR)
		{ _ASSERT(FALSE); return NULL; }
	return &(m_CMAT[nPos][i]);
}

/******************************************************************************
	¹¦ÄÜ£º	»ñÈ¡Ö¸¶¨µÄCMAIT
******************************************************************************/
// Add by Freeway Chen in 2003.5.30
const KBPT_ClassMAIT*       KLibOfBPT::GetCMIT(IN int nPrefixPostfix, IN int nType, IN int nSeries, int nLevel) const
{
    _ASSERT((nPrefixPostfix >= 0) && (nPrefixPostfix < MATF_PREFIXPOSFIX));
    if (!((nPrefixPostfix >= 0) && (nPrefixPostfix < MATF_PREFIXPOSFIX)))
        return NULL;

    _ASSERT((nType >= 0) && (nType < MATF_CBDR));
    if (!((nType >= 0) && (nType < MATF_CBDR)))
        return NULL;

    _ASSERT((nSeries >= 0) && (nSeries < MATF_SERIES));
    if (!((nSeries >= 0) && (nSeries < MATF_SERIES)))
        return NULL;

    _ASSERT(((nLevel - 1) >= 0) && ((nLevel - 1) < MATF_LEVEL));    // nLevel is from 1..MATF_LEVEL
    if (!(((nLevel - 1) >= 0) && ((nLevel - 1) < MATF_LEVEL)))
        return NULL;

    return &m_CMAIT[nPrefixPostfix][nType][nSeries][nLevel - 1];
}

const KMAGICATTRIB_TABFILE* KLibOfBPT::GetMARecord(IN int i) const
{
	return m_BPTMagicAttrib.GetRecord(i);
}

const int KLibOfBPT::GetMARecordNumber() const
{
	return m_BPTMagicAttrib.NumOfEntries();
}

const KBASICPROP_EQUIPMENT* KLibOfBPT::GetMeleeWeaponRecord(IN int i) const
{
	return m_BPTMeleeWeapon.GetRecord(i);
}

const int KLibOfBPT::GetMeleeWeaponRecordNumber() const
{
	return m_BPTMeleeWeapon.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetRangeWeaponRecord(IN int i) const
{
	return m_BPTRangeWeapon.GetRecord(i);
}

const int KLibOfBPT::GetRangeWeaponRecordNumber() const
{
	return m_BPTRangeWeapon.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetArmorRecord(IN int i) const
{
	return m_BPTArmor.GetRecord(i);
}

const int KLibOfBPT::GetArmorRecordNumber() const
{
	return m_BPTArmor.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetHelmRecord(IN int i) const
{
	return m_BPTHelm.GetRecord(i);
}

const int KLibOfBPT::GetHelmRecordNumber() const
{
	return m_BPTHelm.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetBootRecord(IN int i) const
{
	return m_BPTBoot.GetRecord(i);
}

const int KLibOfBPT::GetBootRecordNumber() const
{
	return m_BPTBoot.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetBeltRecord(IN int i) const
{
	return m_BPTBelt.GetRecord(i);
}

const int KLibOfBPT::GetBeltRecordNumber() const
{
	return m_BPTBelt.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetAmuletRecord(IN int i) const
{
	return m_BPTAmulet.GetRecord(i);
}

const int KLibOfBPT::GetAmuletRecordNumber() const
{
	return m_BPTAmulet.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetRingRecord(IN int i) const
{
	return m_BPTRing.GetRecord(i);
}

const int KLibOfBPT::GetRingRecordNumber() const
{
	return m_BPTRing.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetCuffRecord(IN int i) const
{
	return m_BPTCuff.GetRecord(i);
}

const int KLibOfBPT::GetCuffRecordNumber() const
{
	return m_BPTCuff.NumOfEntries();
}

const KBASICPROP_EQUIPMENT*	KLibOfBPT::GetPendantRecord(IN int i) const
{
	return m_BPTPendant.GetRecord(i);
}

const int KLibOfBPT::GetPendantRecordNumber() const
{
	return m_BPTPendant.NumOfEntries();
}

const KBASICPROP_EQUIPMENT* KLibOfBPT::GetHorseRecord(IN int i) const
{
	return m_BPTHorse.GetRecord(i);
}

const int KLibOfBPT::GetHorseRecordNumber() const
{
	return m_BPTHorse.NumOfEntries();
}

const KBASICPROP_EQUIPMENT* KLibOfBPT::GetMaskRecord(IN int i) const
{
	return m_BPTMask.GetRecord(i);
}

const int KLibOfBPT::GetMaskRecordNumber() const
{
	return m_BPTMask.NumOfEntries();
}

const KBASICPROP_MEDICINE* KLibOfBPT::GetMedicineRecord(IN int i) const
{
	return m_BPTMedicine.GetRecord(i);
}

const int KLibOfBPT::GetMedicineRecordNumber() const
{
	return m_BPTMedicine.NumOfEntries();
}

const KBASICPROP_QUEST* KLibOfBPT::GetQuestRecord(IN int i) const
{
	return m_BPTQuest.GetRecord(i);
}

const int KLibOfBPT::GetQuestRecordNumber() const
{
	return m_BPTQuest.NumOfEntries();
}

const KBASICPROP_MASCRIPT* KLibOfBPT::GetMAScriptRecord(IN int i) const
{
	return m_BPTMAScript.GetRecord(i);
}

const int KLibOfBPT::GetMAScriptRecordNumber() const
{
	return m_BPTMAScript.NumOfEntries();
}

int KLibOfBPT::FindMAScriptRecord(IN int nDetail, IN int nParticular)
{
	return m_BPTMAScript.FindRecord(nDetail, nParticular);
}

const KBASICPROP_TOWNPORTAL* KLibOfBPT::GetTownPortalRecord(IN int i) const
{
	return m_BPTTownPortal.GetRecord(i);
}

const int KLibOfBPT::GetTownPortalRecordNumber() const
{
	return m_BPTTownPortal.NumOfEntries();
}


KBasicPropertyTable::KBasicPropertyTable()
{
	m_pBuf = NULL;
	m_nNumOfEntries = 0;
	m_nSizeOfEntry = 0;
	m_szTabFile[0] = 0;
}

KBasicPropertyTable::~KBasicPropertyTable()
{
	ReleaseMemory();
}

/******************************************************************************
	¹¦ÄÜ:	¼ÇÂ¼tab fileÖĞ¹²ÓĞ¶àÉÙÏîÊı¾İ¼ÇÂ¼
******************************************************************************/
void KBasicPropertyTable::SetCount(int cbCount)
{
	_ASSERT(cbCount>0);
	_ASSERT(0==m_nNumOfEntries);	// this function is supposed to be called only once
	m_nNumOfEntries = cbCount;
}

/******************************************************************************
	¹¦ÄÜ:	·ÖÅäÄÚ´æ,ÓÃÓÚ±£´æ´Ótab fileÖĞ¶ÁÈëµÄÊı¾İ
	Èë¿Ú:	m_nNumOfEntries: ¹²ÓĞÕâÃ´¶àÏîÊı¾İ¼ÇÂ¼
			m_nSizeOfEntry: Ã¿ÏîÊı¾İ¼ÇÂ¼µÄ´óĞ¡(×Ö½Ú)
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, m_pBuf Ö¸Ïò·ÖÅäµÄÄÚ´æ
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KBasicPropertyTable::GetMemory()
{
	_ASSERT(NULL == m_pBuf);
	_ASSERT(m_nNumOfEntries > 0 && m_nSizeOfEntry > 0);

	BOOL bEC = FALSE;
	const int nMemSize = m_nSizeOfEntry * m_nNumOfEntries;
	void* pBuf = new BYTE[nMemSize];
	_ASSERT(pBuf != NULL);
	if (pBuf != NULL)
	{
		m_pBuf = pBuf;
		bEC = TRUE;
	}
	return bEC;
}

/******************************************************************************
	¹¦ÄÜ:	ÊÍ·ÅÄÚ´æ
******************************************************************************/
void KBasicPropertyTable::ReleaseMemory()
{
	if (m_pBuf)
	{
		delete []m_pBuf;
		m_pBuf = NULL;
		m_nNumOfEntries = 0;
	}
}

/******************************************************************************
	¹¦ÄÜ:	¶ÁÈëtab fileÖĞµÄÈ«²¿Êı¾İ
	Èë¿Ú:	m_szTabFile: ÎÄ¼şÃû
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, È«²¿Êı¾İ¶ÁÈëm_pBufËùÖ¸»º³åÇøÖĞ.
				m_nNumOfEntries ¸ø³ö¹²¶ÁÈë¶àÉÙÏîÊı¾İ
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KBasicPropertyTable::Load()
{
	BOOL bEC = FALSE;
	KTabFile	theLoader;

	// ¼ÓÔØtab file
	g_SetRootPath(NULL);

//	::g_SetFilePath(TABFILE_PATH);
	char	szFileName[FILE_NAME_LENGTH];
	g_UnitePathAndName(TABFILE_PATH, m_szTabFile, szFileName);
	if (FALSE == theLoader.Load(szFileName))
		{ _ASSERT(FALSE); return bEC; }

	// È·¶¨fileÄÚ¸ø³öÁË¶àÉÙÏî¼ÇÂ¼
	const int cbItems = theLoader.GetHeight() - 1;	// µÚÒ»ĞĞ¸ø³ö¸÷ÁĞÃû³Æ,
	if (cbItems <= 0)							// Êµ¼ÊÊı¾İ´ÓµÚ2ĞĞ¿ªÊ¼¸ø³ö.
		{ _ASSERT(FALSE); return bEC; }
	SetCount(cbItems);

	// ·ÖÅäÄÚ´æ£¬¹¹½¨ÊôĞÔ±í
	if (FALSE == GetMemory())
		{ _ASSERT(FALSE); return bEC; }

	// ½«ÊôĞÔ¼ÇÂ¼ÖğÌõ¶ÁÈë
	int i;
	for (i = 0; i < cbItems; i++)
	{
		if (FALSE == LoadRecord(i, &theLoader))
			{ _ASSERT(FALSE); return bEC; }
	}

	bEC = TRUE;
	return bEC;
}

//============================================================================
KBPT_TownPortal::KBPT_TownPortal()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_TOWNPORTAL);
	::strcpy(m_szTabFile, TABFILE_TOWNPORTAL);
}

KBPT_TownPortal::~KBPT_TownPortal()
{
}

BOOL KBPT_TownPortal::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	KBASICPROP_TOWNPORTAL* pBuf = (KBASICPROP_TOWNPORTAL*)m_pBuf;
	pBuf = pBuf + i;			// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO aryPI[] =
	{
		{1, PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2, PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3, PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{4, PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{5, PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{6, PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{7, PI_VARTYPE_INT,  (char*)&(pBuf->m_nPrice), 0},
		{8, PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
	};

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

const KBASICPROP_TOWNPORTAL* KBPT_TownPortal::GetRecord(IN int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_TOWNPORTAL*)m_pBuf) + i) : NULL;
}

//============================================================================
KBPT_Quest::KBPT_Quest()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_QUEST);
	::strcpy(m_szTabFile, TABFILE_TASK);
}

KBPT_Quest::~KBPT_Quest()
{
}

BOOL KBPT_Quest::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	KBASICPROP_QUEST* pBuf = (KBASICPROP_QUEST*)m_pBuf;
	pBuf = pBuf + i;			// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO aryPI[] =
	{
		{1, PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2, PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3, PI_VARTYPE_INT,  (char*)&(pBuf->m_nDetailType), 0},
		{4, PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{5, PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{6, PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{7, PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{8, PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{9, PI_VARTYPE_INT,  (char*)&(pBuf->m_bCanSell), 0},
		{10, PI_VARTYPE_INT,  (char*)&(pBuf->m_nMaxStack), 0},
	};

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));

}

const KBASICPROP_QUEST* KBPT_Quest::GetRecord(IN int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_QUEST*)m_pBuf) + i) : NULL;

}


KBPT_MagicScript::KBPT_MagicScript()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_MASCRIPT);
	::strcpy(m_szTabFile, TABFILE_MAGICSCRIPT);
}

KBPT_MagicScript::~KBPT_MagicScript()
{
}

BOOL KBPT_MagicScript::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	KBASICPROP_MASCRIPT* pBuf = (KBASICPROP_MASCRIPT*)m_pBuf;
	pBuf = pBuf + i;
	const PROPINFO aryPI[] =
	{
		{1, PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2, PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3, PI_VARTYPE_INT,  (char*)&(pBuf->m_nDetailType), 0},
		{4, PI_VARTYPE_INT,  (char*)&(pBuf->m_nParticularType), 0},
		{5, PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{6, PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{7, PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{8, PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{9, PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_nPrice), 0},
		{14, PI_VARTYPE_CHAR,			pBuf->m_szScript, sizeof(pBuf->m_szScript)},
		{15, PI_VARTYPE_INT,  (char*)&(pBuf->m_nSkillID), 0},
		{18, PI_VARTYPE_INT,  (char*)&(pBuf->m_bShowLevel), 0},
		{19, PI_VARTYPE_INT,  (char*)&(pBuf->m_bShortKey), 0},
		{21, PI_VARTYPE_INT,  (char*)&(pBuf->m_nMaxStack), 0},
		{23, PI_VARTYPE_INT,  (char*)&(pBuf->m_bRegSeries), 0},
	};

	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));

}

const KBASICPROP_MASCRIPT* KBPT_MagicScript::GetRecord(IN int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_MASCRIPT*)m_pBuf) + i) : NULL;

}

int KBPT_MagicScript::FindRecord(IN int nDetail, IN int nParticular)
{
	if(nParticular < 0)
		return -1;
	int i = nParticular;
	if(i >= m_nNumOfEntries)
		i = m_nNumOfEntries - 1;
	int nPrev = i;
	int nNext = i + 1;
	while(nPrev >= 0 || nNext < m_nNumOfEntries)
	{
		if(nPrev >= 0)
		{
			const KBASICPROP_MASCRIPT* pP = GetRecord(nPrev);
			if(pP->m_nParticularType == nParticular && pP->m_nDetailType == nDetail)
				return nPrev;
		}
		if(nNext < m_nNumOfEntries)
		{
			const KBASICPROP_MASCRIPT* pN = GetRecord(nNext);
			if(pN->m_nParticularType == nParticular && pN->m_nDetailType == nDetail)
				return nNext;
		}
		--nPrev;
		++nNext;
	}
	return -1;
}

//============================================================================

KBPT_Medicine::KBPT_Medicine()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_MEDICINE);
	::strcpy(m_szTabFile, TABFILE_MEDICINE);
}

KBPT_Medicine::~KBPT_Medicine()
{
}

BOOL KBPT_Medicine::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	// ³õÊ¼»¯
	KBASICPROP_MEDICINE* pBuf = (KBASICPROP_MEDICINE*)m_pBuf;
	pBuf = pBuf + i;	// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO	aryPI[] =
	{
		{1 , PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nDetailType), 0},
		{4 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nParticularType), 0},
		{5 , PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{6 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{7 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{8 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{9 , PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_nPrice), 0},
		{12, PI_VARTYPE_INT,  (char*)&(pBuf->m_nLevel), 0},
		{14, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[0].nAttrib), 0},
		{15, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[0].nValue), 0},
		{16, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[0].nTime), 0},
		{17, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[1].nAttrib), 0},
		{18, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[1].nValue), 0},
		{19, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryAttrib[1].nTime), 0},
	};

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

/******************************************************************************
	¹¦ÄÜ:	»ñÈ¡Ö¸¶¨µÄÒ©Æ·µÄÊôĞÔ
	Èë¿Ú:	i: ÒªÇó»ñÈ¡µÚiÏîÒ©Æ·µÄÊôĞÔ
	³ö¿Ú:	³É¹¦Ê±·µ»ØÖ¸Ïò¸ÃÒ©²ÄÊôĞÔµÄÖ¸Õë(Ò»¸öKBASICPROP_MEDICINE½á¹¹)
			Ê§°ÜÊ±·µ»ØNULL
******************************************************************************/
const KBASICPROP_MEDICINE* KBPT_Medicine::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_MEDICINE*)m_pBuf) + i) : NULL;
}

KBPT_Equipment::KBPT_Equipment()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_EQUIPMENT);
}

KBPT_Equipment::~KBPT_Equipment()
{
}

void KBPT_Equipment::Init(IN int i)
{
	::strcpy(m_szTabFile, TABFILE_EQUIPMENT[i]);
}

BOOL KBPT_Equipment::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	// ³õÊ¼»¯
	KBASICPROP_EQUIPMENT* pBuf = (KBASICPROP_EQUIPMENT*)m_pBuf;
	pBuf = pBuf + i;	// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO	aryPI[] =
	{
		{1 , PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nDetailType), 0},
		{4 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nParticularType), 0},
		{5 , PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{6 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{7 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{8 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{9 , PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{10, PI_VARTYPE_INT,  (char*)&(pBuf->m_nSeries), 0},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_nPrice), 0},
		{12, PI_VARTYPE_INT,  (char*)&(pBuf->m_nLevel), 0},
		{14, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].nType), 0},
		{15, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].sRange.nMin), 0},
		{16, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].sRange.nMax), 0},
		{17, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].nType), 0},
		{18, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].sRange.nMin), 0},
		{19, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].sRange.nMax), 0},
		{20, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].nType), 0},
		{21, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].sRange.nMin), 0},
		{22, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].sRange.nMax), 0},
		{23, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].nType), 0},
		{24, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].sRange.nMin), 0},
		{25, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].sRange.nMax), 0},
		{26, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].nType), 0},
		{27, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].sRange.nMin), 0},
		{28, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].sRange.nMax), 0},
		{29, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].nType), 0},
		{30, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].sRange.nMin), 0},
		{31, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].sRange.nMax), 0},
		{32, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].nType), 0},
		{33, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].sRange.nMin), 0},
		{34, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].sRange.nMax), 0},
		{35, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[0].nType), 0},
		{36, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[0].nPara), 0},
		{37, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[1].nType), 0},
		{38, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[1].nPara), 0},
		{39, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[2].nType), 0},
		{40, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[2].nPara), 0},
		{41, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[3].nType), 0},
		{42, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[3].nPara), 0},
		{43, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[4].nType), 0},
		{44, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[4].nPara), 0},
		{45, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[5].nType), 0},
		{46, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[5].nPara), 0},
	};

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

/******************************************************************************
	¹¦ÄÜ:	»ñÈ¡Ö¸¶¨µÄ×°±¸µÄÊôĞÔ
	Èë¿Ú:	i: ÒªÇó»ñÈ¡µÚiÏî×°±¸µÄÊôĞÔ
	³ö¿Ú:	³É¹¦Ê±·µ»ØÖ¸Ïò¸Ã×°±¸ÊôĞÔµÄÖ¸Õë(Ò»¸öKBASICPROP_EQUIPMENT½á¹¹)
			Ê§°ÜÊ±·µ»ØNULL
******************************************************************************/
const KBASICPROP_EQUIPMENT* KBPT_Equipment::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_EQUIPMENT*)m_pBuf) + i) : NULL;
}

VMA_Magic_GoldEquip::VMA_Magic_GoldEquip()
{
	m_nSizeOfEntry = sizeof(MAATTRIB_GOLDEQUIP);
	::strcpy(m_szTabFile, TABFILE_GOLDMAGIC);
}

VMA_Magic_GoldEquip::~VMA_Magic_GoldEquip()
{
}

BOOL VMA_Magic_GoldEquip::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	MAATTRIB_GOLDEQUIP* pBuf = (MAATTRIB_GOLDEQUIP*)m_pBuf;
	pBuf = pBuf + i;
	const PROPINFO	aryPI[] =
	{
		{5 , PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.nPropKind), 0},
		{6 , PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[0].nMin), 0},
		{7 , PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[0].nMax), 0},
		{8 , PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[1].nMin), 0},
		{9 , PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[1].nMax), 0},
		{10, PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[2].nMin), 0},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_GoldMA.aryRange[2].nMax), 0},
	};

	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

const MAATTRIB_GOLDEQUIP* VMA_Magic_GoldEquip::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((MAATTRIB_GOLDEQUIP*)m_pBuf) + i) : NULL;
}

VMA_Suit_ActivateCount::VMA_Suit_ActivateCount()
{
	m_nSizeOfEntry = sizeof(KEQCP_REQ);
	::strcpy(m_szTabFile, TABFILE_SUITACTCOUNT);
}

VMA_Suit_ActivateCount::~VMA_Suit_ActivateCount()
{
}

BOOL VMA_Suit_ActivateCount::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	KEQCP_REQ* pBuf = (KEQCP_REQ*)m_pBuf;
	pBuf = pBuf + i;
	const PROPINFO	aryPI[] =
	{
		{1 , PI_VARTYPE_INT,  (char*)&(pBuf->nType), 0},
		{2 , PI_VARTYPE_INT,  (char*)&(pBuf->nPara), 0},
	};

	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

const KEQCP_REQ* VMA_Suit_ActivateCount::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KEQCP_REQ*)m_pBuf) + i) : NULL;
}

//------------------------------------
KBPT_Equipment_Gold::KBPT_Equipment_Gold()
{
	m_nSizeOfEntry = sizeof(KBASICPROP_EQUIPMENT_GOLD);
	::strcpy(m_szTabFile, TABFILE_GOLDITEM);
}

KBPT_Equipment_Gold::~KBPT_Equipment_Gold()
{
}

BOOL KBPT_Equipment_Gold::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	// ³õÊ¼»¯
	KBASICPROP_EQUIPMENT_GOLD* pBuf = (KBASICPROP_EQUIPMENT_GOLD*)m_pBuf;
	pBuf = pBuf + i;	// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO	aryPI[] =
	{
		{1 , PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nItemGenre), 0},
		{3 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nDetailType), 0},
		{4 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nParticularType), 0},
		{5 , PI_VARTYPE_CHAR,			pBuf->m_szImageName, sizeof(pBuf->m_szImageName)},
		{6 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nObjIdx), 0},
		{7 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nWidth), 0},
		{8 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nHeight), 0},
		{9 , PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{10, PI_VARTYPE_INT,  (char*)&(pBuf->m_nSeries), 0},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_nPrice), 0},
		{12, PI_VARTYPE_INT,  (char*)&(pBuf->m_nLevel), 0},
		{14, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].nType), 0},
		{15, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].sRange.nMin), 0},
		{16, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[0].sRange.nMax), 0},
		{17, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].nType), 0},
		{18, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].sRange.nMin), 0},
		{19, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[1].sRange.nMax), 0},
		{20, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].nType), 0},
		{21, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].sRange.nMin), 0},
		{22, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[2].sRange.nMax), 0},
		{23, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].nType), 0},
		{24, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].sRange.nMin), 0},
		{25, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[3].sRange.nMax), 0},
		{26, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].nType), 0},
		{27, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].sRange.nMin), 0},
		{28, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[4].sRange.nMax), 0},
		{29, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].nType), 0},
		{30, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].sRange.nMin), 0},
		{31, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[5].sRange.nMax), 0},
		{32, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].nType), 0},
		{33, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].sRange.nMin), 0},
		{34, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropBasic[6].sRange.nMax), 0},
		{35, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[0].nType), 0},
		{36, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[0].nPara), 0},
		{37, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[1].nType), 0},
		{38, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[1].nPara), 0},
		{39, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[2].nType), 0},
		{40, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[2].nPara), 0},
		{41, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[3].nType), 0},
		{42, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[3].nPara), 0},
		{43, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[4].nType), 0},
		{44, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[4].nPara), 0},
		{45, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[5].nType), 0},
		{46, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryPropReq[5].nPara), 0},
		{47, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[0]), 0},
		{48, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[1]), 0},
		{49, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[2]), 0},
		{50, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[3]), 0},
		{51, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[4]), 0},
		{52, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicId[5]), 0},
		{53, PI_VARTYPE_INT,  (char*)&(pBuf->m_nGroupId), 0},
		{54, PI_VARTYPE_INT,  (char*)&(pBuf->m_nExGroupId), 0},
		{55, PI_VARTYPE_INT,  (char*)&(pBuf->m_nGroupSerial), 0},
		{56, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicEx[0]), 0},
		{57, PI_VARTYPE_INT,  (char*)&(pBuf->m_aryMagicEx[1]), 0},
	};

	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	return ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0]));
}

/******************************************************************************
	¹¦ÄÜ:	»ñÈ¡Ö¸¶¨µÄ×°±¸µÄÊôĞÔ
	Èë¿Ú:	i: ÒªÇó»ñÈ¡µÚiÏî×°±¸µÄÊôĞÔ
	³ö¿Ú:	³É¹¦Ê±·µ»ØÖ¸Ïò¸Ã×°±¸ÊôĞÔµÄÖ¸Õë(Ò»¸öKBASICPROP_EQUIPMENT½á¹¹)
			Ê§°ÜÊ±·µ»ØNULL
******************************************************************************/
const KBASICPROP_EQUIPMENT_GOLD* KBPT_Equipment_Gold::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KBASICPROP_EQUIPMENT_GOLD*)m_pBuf) + i) : NULL;
}

//============================================================================

KBPT_MagicAttrib_TF::KBPT_MagicAttrib_TF()
{
	m_nSizeOfEntry = sizeof(KMAGICATTRIB_TABFILE);
	::strcpy(m_szTabFile, TABFILE_MAGICATTRIB);
	::memset(&m_naryMACount, 0, sizeof(m_naryMACount));
}

KBPT_MagicAttrib_TF::~KBPT_MagicAttrib_TF()
{
}

BOOL KBPT_MagicAttrib_TF::LoadRecord(int i, KTabFile* pTF)
{
	_ASSERT(pTF != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);

	// ³õÊ¼»¯
	KMAGICATTRIB_TABFILE* pBuf = (KMAGICATTRIB_TABFILE*)m_pBuf;
	pBuf = pBuf + i;	// ¶ÁÈëµÄÊôĞÔ¼ÇÔÚ pBuf ËùÖ¸½á¹¹ÖĞ
	const PROPINFO	aryPI[] =
	{
		{1 , PI_VARTYPE_CHAR,			pBuf->m_szName, sizeof(pBuf->m_szName)},
		{2 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nPos), 0},
		{3 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nClass), 0},
		{4 , PI_VARTYPE_INT,  (char*)&(pBuf->m_nLevel), 0},
		{5 , PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.nPropKind), 0},
		{6 , PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[0].nMin), 0},
		{7 , PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[0].nMax), 0},
		{8 , PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[1].nMin), 0},
		{9 , PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[1].nMax), 0},
		{10, PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[2].nMin), 0},
		{11, PI_VARTYPE_INT,  (char*)&(pBuf->m_MagicAttrib.aryRange[2].nMax), 0},
		{12, PI_VARTYPE_CHAR,			pBuf->m_szIntro, sizeof(pBuf->m_szIntro)},
		{13, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[0]), 0},
		{14, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[1]), 0},
		{15, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[2]), 0},
		{16, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[3]), 0},
		{17, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[4]), 0},
		{18, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[5]), 0},
		{19, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[6]), 0},
		{20, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[7]), 0},
		{21, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[8]), 0},
		{22, PI_VARTYPE_INT,  (char*)&(pBuf->m_DropRate[9]), 0},
	};
	
	// Öğ¸ö¶ÁÈë¸÷ÏîÊôĞÔ
	if (FALSE == ::LoadRecord(pTF, i, aryPI, sizeof(aryPI)/ sizeof(aryPI[0])))
		return FALSE;

    pBuf->m_nUseFlag = false;       // ³õÊ¼»¯ÎªÃ»ÓĞÊ¹ÓÃ

	// Êı¾İÍ³¼Æ£ºÃ¿ÖÖ×°±¸¿ÉÊÊÓÃµÄÄ§·¨ÊıÄ¿
	for (int n = 0; n < MATF_CBDR; n++)
	{
		if (0 != pBuf->m_DropRate[n])
		{
			int nPos;
			nPos = (0 == pBuf->m_nPos) ? 1 : 0;
			m_naryMACount[nPos][n]++;
		}
	}
	return TRUE;
}

/******************************************************************************
	¹¦ÄÜ:	»ñÈ¡Ö¸¶¨µÄÄ§·¨ÊôĞÔ
	Èë¿Ú:	i: ÒªÇó»ñÈ¡µÚiÏîÄ§·¨ÊôĞÔ
	³ö¿Ú:	³É¹¦Ê±·µ»ØÖ¸Ïò¸ÃÄ§·¨ÊôĞÔµÄÖ¸Õë(Ò»¸öKMAGICATTRIB_TABFILE½á¹¹)
			Ê§°ÜÊ±·µ»ØNULL
******************************************************************************/
const KMAGICATTRIB_TABFILE* KBPT_MagicAttrib_TF::GetRecord(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfEntries);
	return (i >= 0 && i < m_nNumOfEntries) ?
		(((KMAGICATTRIB_TABFILE*)m_pBuf) + i) : NULL;
}

void KBPT_MagicAttrib_TF::GetMACount(int* pnCount) const
{
	_ASSERT(this != NULL);
	_ASSERT(pnCount != NULL);
	::memcpy(pnCount, m_naryMACount, sizeof(m_naryMACount));
}

//============================================================================
// Magic Item Index Table Class
KBPT_ClassMAIT::KBPT_ClassMAIT()
{
	m_pnTable = NULL;
	m_nSize = 0;
	m_nNumOfValidData = 0;
}

KBPT_ClassMAIT::~KBPT_ClassMAIT()
{
    m_nSize = 0;
	m_nNumOfValidData = 0;

	if (m_pnTable)
    {
        delete []m_pnTable;
        m_pnTable = NULL;
    }
}

/******************************************************************************
	¹¦ÄÜ:	½«Ë÷Òı±íÇå¿Õ£¬²¢²»ÊÍ·ÅÄÚ´æ
	Èë¿Ú:	
	³ö¿Ú:	³É¹¦·µ»Ø·ÇÁã£¬Ê§°Ü·µ»ØÁã
	ËµÃ÷:	
******************************************************************************/
BOOL KBPT_ClassMAIT::Clear()
{
    m_nNumOfValidData = 0; 
    
    return true;   
}


/******************************************************************************
	¹¦ÄÜ:	´ÓË÷Òı±íÖĞÈ¡Öµ
	Èë¿Ú:	i: ´ÓË÷Òı±íÖĞ¸ÃÎ»ÖÃÈ¡Öµ
	³ö¿Ú:	·µ»ØËùÈ¡µÄÖµ
	ËµÃ÷:	Èô·µ»ØÖµÎªnItemIndex, Ôòµ÷ÓÃKLibOfBPT::m_BPTMagicAttrib.GetRecord(nItemIndex)
            ¿É»ñµÃ¶ÔÓ¦µÄÄ§·¨ÊôĞÔ
******************************************************************************/
int KBPT_ClassMAIT::Get(int i) const
{
	_ASSERT(this != NULL);
	_ASSERT(i >= 0 && i < m_nNumOfValidData);
    _ASSERT(m_pnTable);
	if(i < 0 || i >= m_nNumOfValidData)
		return -1;
	return m_pnTable[i];
}


/******************************************************************************
	¹¦ÄÜ:	¸øË÷Òı±í²åÈëÒ»¸öĞÂµÄÖµ
	Èë¿Ú:	nItemIndex: Óû¸³Ö®Öµ
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, m_pnTable[m_nNumOfValidData]ÎªËù¸³Ö®Öµ
			m_nNumOfValidDataÖ¸ÏòÏÂÒ»¿ÕÎ»
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KBPT_ClassMAIT::Insert(int nItemIndex)
{
    int nResult = false;

	_ASSERT(this != NULL);
	_ASSERT(nItemIndex >= 0);	// nData ÎªÊı×éÏÂ±ê

    if (!m_pnTable)
    {
        m_pnTable = new int [4];
        if (!m_pnTable)
            goto Exit0;

        m_nNumOfValidData = 0;
        m_nSize = 4;
    }

    if (m_nNumOfValidData >= m_nSize)
    {
        int *pnaryTemp = new int [m_nSize + 8];
        if (!pnaryTemp)
            goto Exit0;

        memcpy(pnaryTemp, m_pnTable, m_nNumOfValidData * sizeof(int));

        m_nSize += 8;
        delete []m_pnTable;
        m_pnTable = pnaryTemp;
        pnaryTemp = NULL;
    }

	m_pnTable[m_nNumOfValidData++] = nItemIndex;

    nResult = true;
Exit0:
    return nResult;
}



//============================================================================

KBPT_ClassifiedMAT::KBPT_ClassifiedMAT()
{
	m_pnTable = NULL;
	m_nSize = 0;
	m_nNumOfValidData = 0;
}

KBPT_ClassifiedMAT::~KBPT_ClassifiedMAT()
{
	ReleaseMemory();
}

/******************************************************************************
	¹¦ÄÜ:	ÎªË÷Òı±í·ÖÅäÄÚ´æ
	Èë¿Ú:	nCount: Ë÷Òı±í´óĞ¡(Êı¾İÏîµÄÊıÄ¿)
	³ö¿Ú:	m_pnTable Ö¸Ïò·ÖÅäµÄÄÚ´æ
			m_nSize = nCount
******************************************************************************/
BOOL KBPT_ClassifiedMAT::GetMemory(int nCount)
{
	_ASSERT(this != NULL);
	_ASSERT(nCount > 0);
	_ASSERT(NULL == m_pnTable);
	_ASSERT(0 == m_nSize);

	BOOL bEC = FALSE;
	int* pnBuf = new int[nCount];
	if (pnBuf)
	{
		m_pnTable = pnBuf;
		m_nSize = nCount;
		bEC = TRUE;
	}
	return bEC;
}

/******************************************************************************
	¹¦ÄÜ:	ÊÍ·ÅË÷Òı±íËùÓÃµÄÄÚ´æ
******************************************************************************/
void KBPT_ClassifiedMAT::ReleaseMemory()
{
	if (m_pnTable)
	{
		delete []m_pnTable;
		m_pnTable = NULL;
		m_nSize = 0;
	}
}

/******************************************************************************
	¹¦ÄÜ:	¸øË÷Òı±í¸³Öµ
	Èë¿Ú:	nData: Óû¸³Ö®Öµ
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, m_pnTable[m_nNumOfValidData]ÎªËù¸³Ö®Öµ
			m_nNumOfValidDataÖ¸ÏòÏÂÒ»¿ÕÎ»
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KBPT_ClassifiedMAT::Set(int nData)
{
	_ASSERT(this != NULL);
	_ASSERT(nData >= 0);	// nData ÎªÊı×éÏÂ±ê
	_ASSERT(m_nNumOfValidData >= 0 && m_nNumOfValidData < m_nSize);
	m_pnTable[m_nNumOfValidData++] = nData;
	return TRUE;
}

/******************************************************************************
	¹¦ÄÜ:	´ÓË÷Òı±íÖĞÈ¡Öµ
	Èë¿Ú:	nIndex: ´ÓË÷Òı±íÖĞ¸ÃÎ»ÖÃÈ¡Öµ
	³ö¿Ú:	·µ»ØËùÈ¡µÄÖµ
	ËµÃ÷:	Èô·µ»ØÖµÎªi, Ôòµ÷ÓÃKLibOfBPT::m_BPTMagicAttrib.GetRecord(i)¿É»ñµÃ
			¶ÔÓ¦µÄÄ§·¨ÊôĞÔ
******************************************************************************/
int KBPT_ClassifiedMAT::Get(int nIndex) const
{
	_ASSERT(this != NULL);
	_ASSERT(nIndex >= 0 && nIndex < m_nSize);
	return (nIndex >= 0 && nIndex < m_nSize) ? m_pnTable[nIndex] : 0;
}

/******************************************************************************
	¹¦ÄÜ:	½«Êı¾İµ¼³öµ½¸ø¶¨µÄ»º³åÇøÖĞ
	Èë¿Ú:	pnaryBuf: ¼¶³åÇøÖ¸Õë
			*pnCount: »º³åÇø¿ÉÈİÄÉ¶àÉÙ¸öintĞÍÊı¾İ
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, Êı¾İ±»µ¼³öµ½»º³åÇøÖĞ, *pnCount ¸ø³öÊı¾İ¸öÊı
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KBPT_ClassifiedMAT::GetAll(int* pnaryBuf, int* pnCount) const
{
	_ASSERT(this != NULL);
	_ASSERT(pnaryBuf != NULL);
	_ASSERT(pnCount != NULL);

	BOOL bEC = FALSE;
	if (pnaryBuf && pnCount && *pnCount > 0)
	{
		int nLength = *pnCount;
		if(m_nSize< nLength) nLength = m_nSize;
		::memcpy(pnaryBuf, m_pnTable, nLength*sizeof(int));
		*pnCount = nLength;
		bEC = TRUE;
	}
	return bEC;
}

