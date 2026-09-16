//---------------------------------------------------------------------------
// Sword3 Core (c) 2002 by Kingsoft
//
// File:	KItemGenerator.CPP
// Date:	2002.08.26
// Code:	DongBo
// Desc:    CPP file. ±¾ÎÄ¼şÊµÏÖµÄÀàÓÃÓÚÉú³ÉµÀ¾ß
//---------------------------------------------------------------------------
#include "KEngine.h"
#include "KCore.h"
#include "MyAssert.H"
#include "KItem.h"
#include "KSubWorldSet.h"
#include "KItemGenerator.h"

KItemGenerator	ItemGen;			//	×°±¸Éú³ÉÆ÷

KItemGenerator::KItemGenerator()
{
}

KItemGenerator::~KItemGenerator()
{
}

/******************************************************************************
	¹¦ÄÜ£º	Êı¾İ³õÊ¼»¯. ´Ótab fileÖĞ¶ÁÈ¡Êı¾İ
******************************************************************************/
BOOL KItemGenerator::Init()
{
	if (!m_BPTLib.Init())		// ´Ëµ÷ÓÃ´ÓÈô¸ÉµÄtab fileÖĞÔØÈëËùÓĞµÄ³õÊ¼ÊôĞÔ
		return FALSE;

	for (int i = equip_meleeweapon; i <= equip_mask; i++)
	{
		switch(i)
		{
		case equip_meleeweapon:
			m_EquipNumOfEntries[i] = m_BPTLib.GetMeleeWeaponRecordNumber();
			break;
		case equip_rangeweapon:
			m_EquipNumOfEntries[i] = m_BPTLib.GetRangeWeaponRecordNumber();
			break;
		case equip_armor:
			m_EquipNumOfEntries[i] = m_BPTLib.GetArmorRecordNumber();
			break;
		case equip_helm:
			m_EquipNumOfEntries[i] = m_BPTLib.GetHelmRecordNumber();
			break;
		case equip_boots:
			m_EquipNumOfEntries[i] = m_BPTLib.GetBootRecordNumber();
			break;
		case equip_belt:
			m_EquipNumOfEntries[i] = m_BPTLib.GetBeltRecordNumber();
			break;
		case equip_cuff:
			m_EquipNumOfEntries[i] = m_BPTLib.GetCuffRecordNumber();
			break;
		case equip_amulet:
			m_EquipNumOfEntries[i] = m_BPTLib.GetAmuletRecordNumber();
			break;
		case equip_ring:
			m_EquipNumOfEntries[i] = m_BPTLib.GetRingRecordNumber();
			break;
		case equip_pendant:
			m_EquipNumOfEntries[i] = m_BPTLib.GetPendantRecordNumber();
			break;
		case equip_horse:
			m_EquipNumOfEntries[i] = m_BPTLib.GetHorseRecordNumber();
			break;
		case equip_mask:
			m_EquipNumOfEntries[i] = m_BPTLib.GetMaskRecordNumber();
			break;
		}
	}
	m_MedNumOfEntries = m_BPTLib.GetMedicineRecordNumber();
	return TRUE;

}

/******************************************************************************
	¹¦ÄÜ£º	Éú³ÉÒ©Æ·
	Èë¿Ú:	nDetailType: Ò©Æ·ÀàĞÍ
			nLevel: µÈ¼¶
			nVersion: °æ±¾ºÅ
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã, Ïà¹ØÊı¾İÔÚ pItem ËùÖ¸¶ÔÏóÖĞ¸ø³ö
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KItemGenerator::Gen_Medicine(IN int nDetailType,
								  IN int nLevel,
								  IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	
	const int i = nDetailType * 5 + nLevel - 1;	// ±¾¹«Ê½ÓÉ²ß»®¶¨Òå
												// µÈ¼¶Êı¾İ´Ó1¿ªÊ¼£¬ËùÒÔÒª¼õ»ØÀ´
	const KBASICPROP_MEDICINE* pMed = NULL;
	pMed = m_BPTLib.GetMedicineRecord(i);
	if (NULL == pMed)
	{
		_ASSERT(FALSE);
		return bEC;
	}
	*pItem = *pMed;
	return TRUE;
}
/******************************************************************************
T¹o item hoµng kim
Creator: Guve
******************************************************************************/
BOOL KItemGenerator::Gen_GoldEquip(IN int nLuck, IN int nRowId, IN OUT KItem* pItem, IN int* pnaryMALevel)
{
	if(nRowId <= 0)
		return FALSE;
	const KBASICPROP_EQUIPMENT_GOLD* pEqu = m_BPTLib.GetGoldItemRecord(nRowId-1);
	if(!pEqu)
		return FALSE;
	pItem->SetExType(extype_gold);
	pItem->SetGenParam(nRowId);
	pItem->SetAttrib_CBR((KBASICPROP_EQUIPMENT*)pEqu);
	pItem->SetAttrib_Extend(pEqu);
	KItemNormalAttrib	sMA[6];	// thuéc tİnh sÏ thu thËp ®­îc
	KItemNormalAttrib	sMAEx[2];	// thuéc tİnh sÏ thu thËp ®­îc
	int nMA = 0;
	if(!pnaryMALevel)
	{
#ifdef _SERVER
		if(nLuck < 0)
			nLuck = 0;
		int i;
		for(i=0;i<6;++i)
		{
			const MAATTRIB_GOLDEQUIP* pMaGe = m_BPTLib.GetGoldMagicRecord(pEqu->m_aryMagicId[i] - 1);
			if(!pMaGe)
				continue;
			sMA[nMA].nAttribType = pMaGe->m_GoldMA.nPropKind;
			sMA[nMA].nValue[1] = -1;
			sMA[nMA].nValue[2] = pMaGe->m_GoldMA.aryRange[2].nMin;
			int nDist = pMaGe->m_GoldMA.aryRange[0].nMax - pMaGe->m_GoldMA.aryRange[0].nMin;
			int nCalcLuck = nLuck;
			if(nDist <= 0)
			{
				sMA[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin;
			}
			else if(nCalcLuck >= 200)
			{
				sMA[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMax;
			}
			else
			{
				if(nCalcLuck > 100)
					nCalcLuck = 100;
				int nMid = nDist/2+1;
				if(nDist < 10)
				{
					if(g_Random(100 - nCalcLuck) < 20)
					{
						nMid = GetRandomNumber(nMid*10, nDist*10+5)/10;
					}
					else
					{
						nMid = g_Random(nMid*10)/10;
					}
					sMA[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + nMid;
				}
				else
				{
					if(g_Random(100 - nCalcLuck) < 20)
					{
						sMA[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + 
										+ GetRandomNumber(nMid, nDist);
					}
					else
					{
						sMA[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + g_Random(nMid);
					}
				}
			}
			++nMA;
		}
		nMA = 0;
		for(i=0;i<2;++i)
		{
			const MAATTRIB_GOLDEQUIP* pMaGe = m_BPTLib.GetGoldMagicRecord(pEqu->m_aryMagicEx[i] - 1);
			if(!pMaGe)
				continue;
			sMAEx[nMA].nAttribType = pMaGe->m_GoldMA.nPropKind;
			sMAEx[nMA].nValue[1] = -1;
			sMAEx[nMA].nValue[2] = pMaGe->m_GoldMA.aryRange[2].nMin;
			int nDist = pMaGe->m_GoldMA.aryRange[0].nMax - pMaGe->m_GoldMA.aryRange[0].nMin;
			int nCalcLuck = nLuck;
			if(nDist <= 0)
			{
				sMAEx[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin;
			}
			else if(nCalcLuck >= 200)
			{
				sMAEx[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMax;
			}
			else
			{
				if(nCalcLuck > 100)
					nCalcLuck = 100;
				int nMid = nDist/2+1;
				if(nDist < 10)
				{
					if(g_Random(100 - nCalcLuck) < 20)
					{
						nMid = GetRandomNumber(nMid*10, nDist*10+5)/10;
					}
					else
					{
						nMid = g_Random(nMid*10)/10;
					}
					sMAEx[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + nMid;
				}
				else
				{
					if(g_Random(100 - nCalcLuck) < 20)
					{
						sMAEx[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + 
										+ GetRandomNumber(nMid, nDist);
					}
					else
					{
						sMAEx[nMA].nValue[0] = pMaGe->m_GoldMA.aryRange[0].nMin + g_Random(nMid);
					}
				}
			}
			++nMA;
		}
#endif
	}
	else
	{
		int i;
		for(i=0;i<6;++i)
		{
			const MAATTRIB_GOLDEQUIP* pMaGe = m_BPTLib.GetGoldMagicRecord(pEqu->m_aryMagicId[i] - 1);
			if(!pMaGe)
				continue;
			if(!pnaryMALevel[nMA*2])
				break;
			sMA[nMA].nAttribType = pnaryMALevel[nMA*2];
			sMA[nMA].nValue[0] = pnaryMALevel[nMA*2+1];
			sMA[nMA].nValue[1] = -1;
			sMA[nMA].nValue[2] = pMaGe->m_GoldMA.aryRange[2].nMin;
			++nMA;
		}
		nMA = 0;
		for(i=0;i<2;++i)
		{
			const MAATTRIB_GOLDEQUIP* pMaGe = m_BPTLib.GetGoldMagicRecord(pEqu->m_aryMagicEx[i] - 1);
			if(!pMaGe)
				continue;
			if(!pnaryMALevel[12+nMA*2])
				break;
			sMAEx[nMA].nAttribType = pnaryMALevel[12+nMA*2];
			sMAEx[nMA].nValue[0] = pnaryMALevel[12+nMA*2+1];
			sMAEx[nMA].nValue[1] = -1;
			sMAEx[nMA].nValue[2] = pMaGe->m_GoldMA.aryRange[2].nMin;
			++nMA;
		}
	}
	pItem->SetAttrib_MA(sMA);
	pItem->SetAttrib_MA(sMAEx, true);
	return TRUE;
}

const KBASICPROP_EQUIPMENT_GOLD* KItemGenerator::GetGoldItemRecord(IN int nIndex)
{
	return m_BPTLib.GetGoldItemRecord(nIndex);
}

int KItemGenerator::GetGoldItemNumber()
{
	return m_BPTLib.GetGoldItemNumber();
}
/******************************************************************************
T¹o item tr¾ng, xanh
Editor: Guve
******************************************************************************/
BOOL KItemGenerator::Gen_Equipment(IN int nDetailType,
								   IN int nParticularType,
								   IN int nSeriesReq,
								   IN int nLevel,
								   IN const int* pnaryMALevel,
								   IN int nLucky,
								   IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	// i lµ dßng thø trong c¸c file item.txt
	int i = nParticularType * 10 + nLevel - 1;

	const KBASICPROP_EQUIPMENT* pEqu = NULL;
	switch(nDetailType)
	{
	case equip_meleeweapon:
		pEqu = m_BPTLib.GetMeleeWeaponRecord(i);
		break;
	case equip_rangeweapon:
		pEqu = m_BPTLib.GetRangeWeaponRecord(i);
		break;
	case equip_armor:
		pEqu = m_BPTLib.GetArmorRecord(i);
		break;
	case equip_helm:
		pEqu = m_BPTLib.GetHelmRecord(i);
		break;
	case equip_boots:
		pEqu = m_BPTLib.GetBootRecord(i);
		break;
	case equip_belt:
		pEqu = m_BPTLib.GetBeltRecord(i);
		break;
	case equip_amulet:
		pEqu = m_BPTLib.GetAmuletRecord(i);
		break;
	case equip_ring:
		pEqu = m_BPTLib.GetRingRecord(i);
		break;
	case equip_cuff:
		pEqu = m_BPTLib.GetCuffRecord(i);
		break;
	case equip_pendant:
		pEqu = m_BPTLib.GetPendantRecord(i);
		break;
	case equip_horse:
		pEqu = m_BPTLib.GetHorseRecord(i);
		break;
	case equip_mask:
		pEqu = m_BPTLib.GetMaskRecord(nParticularType);
		break;
	default:
		break;
	}
	if (NULL == pEqu)
		{ _ASSERT(FALSE); return bEC; }
	//®· lÊy ®­îc dßng trong item.txt -> set thuéc tİnh tr¾ng c¬ b¶n
	pItem->SetAttrib_CBR(pEqu);
	if(nDetailType == equip_mask)
	{
		if(nLevel > 0)
			pItem->SetLevel(nLevel);
		else
			pItem->SetLevel(0);
	}
	else
		pItem->SetSeries(nSeriesReq);
	
	if (NULL == pnaryMALevel || nDetailType >= equip_horse) //item tr¾ng, kh«ng magic
		return TRUE;
	// b¾t ®Çu t¹o magic
	KItemNormalAttrib	sMA[6];	// thuéc tİnh sÏ thu thËp ®­îc
#ifndef _SERVER	//ë client chØ cËp nhËt
	bEC = TRUE;
	for(i=0;i<6;++i)
	{
		if(!pnaryMALevel[i*2])
			break;
		sMA[i].nAttribType = pnaryMALevel[i*2];
		sMA[i].nValue[0] = pnaryMALevel[i*2+1];
		sMA[i].nValue[1] = -1;
		if(sMA[i].nAttribType == magic_addphysicsdamage_p)
			sMA[i].nValue[2] = 6;
	}
#else	//ë server cÇn t¹o míi
	bEC = Gen_MagicAttrib(nDetailType, pnaryMALevel, nSeriesReq, nLucky, sMA);
#endif
	if (bEC)
		pItem->SetAttrib_MA(sMA);

	return bEC;
}

/******************************************************************************
T¹o item tİm huyÒn tinh
Editor: Guve
******************************************************************************/
BOOL KItemGenerator::Gen_PurpleEquip(IN int nDetailType,
								   IN int nParticularType,
								   IN int nSeriesReq,
								   IN int nLevel,
								   IN const int* pnaryMALevel,
								   IN int nRows,
								   IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	// i lµ dßng thø trong c¸c file item.txt
	int i = nParticularType * 10 + nLevel - 1;

	const KBASICPROP_EQUIPMENT* pEqu = NULL;
	switch(nDetailType)
	{
	case equip_meleeweapon:
		pEqu = m_BPTLib.GetMeleeWeaponRecord(i);
		break;
	case equip_rangeweapon:
		pEqu = m_BPTLib.GetRangeWeaponRecord(i);
		break;
	case equip_armor:
		pEqu = m_BPTLib.GetArmorRecord(i);
		break;
	case equip_helm:
		pEqu = m_BPTLib.GetHelmRecord(i);
		break;
	case equip_boots:
		pEqu = m_BPTLib.GetBootRecord(i);
		break;
	case equip_belt:
		pEqu = m_BPTLib.GetBeltRecord(i);
		break;
	case equip_amulet:
		pEqu = m_BPTLib.GetAmuletRecord(i);
		break;
	case equip_ring:
		pEqu = m_BPTLib.GetRingRecord(i);
		break;
	case equip_cuff:
		pEqu = m_BPTLib.GetCuffRecord(i);
		break;
	case equip_pendant:
		pEqu = m_BPTLib.GetPendantRecord(i);
		break;
	case equip_horse:
		pEqu = m_BPTLib.GetHorseRecord(i);
		break;
	case equip_mask:
		pEqu = m_BPTLib.GetMaskRecord(nParticularType);
		break;
	default:
		break;
	}
	if (NULL == pEqu)
		{ _ASSERT(FALSE); return bEC; }
	//®· lÊy ®­îc dßng trong item.txt -> set thuéc tİnh tr¾ng c¬ b¶n
	pItem->SetExType(extype_purple);
	if(nRows <= 0 || nRows > 6)
		nRows = 6;
	pItem->SetGenParam(nRows);
	pItem->SetAttrib_CBR(pEqu);
	if(nDetailType == equip_mask)
	{
		if(nLevel > 0)
			pItem->SetLevel(nLevel);
		else
			pItem->SetLevel(0);
	}
	else
		pItem->SetSeries(nSeriesReq);
	
	if (NULL == pnaryMALevel) //item tr¾ng, kh«ng magic
		return TRUE;
	bEC = TRUE;
	// b¾t ®Çu t¹o magic
	KItemNormalAttrib	sMA[6];	// thuéc tİnh sÏ thu thËp ®­îc
	for(i=0;i<6;++i)
	{
		if(!pnaryMALevel[i*2])
			break;
		sMA[i].nAttribType = pnaryMALevel[i*2];
		sMA[i].nValue[0] = pnaryMALevel[i*2+1];
		sMA[i].nValue[1] = -1;
		if(sMA[i].nAttribType == magic_addphysicsdamage_p)
			sMA[i].nValue[2] = 6;
	}
	pItem->SetAttrib_MA(sMA);

	return bEC;
}

#define MAX_LUCKRAND	100000
// Chän 6 dßng thuéc tİnh
BOOL KItemGenerator::Gen_MagicAttrib(
    int nType, const int* pnaryMALevel, int nSeriesReq, int nLucky, KItemNormalAttrib* pnaryMA)
{
    int nResult = false;
    KBPT_ClassMAIT  SelectedMagicTable;	
	// n¬i l­u 6 dßng ®Ó set m_nUseFlag
    KMAGICATTRIB_TABFILE *pMagicAttrTable[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

    int i = 0;
	double dLucky = 1.0-(double)nLucky/100.0;
	if(dLucky < 0.0)
		dLucky = 0.0;
    _ASSERT(pnaryMALevel);
    _ASSERT(pnaryMA);

    for (i = 0; i < 6; i++) 
    {
        // gÆp level ®Çu vµo = 0 th× ng­ng
        if (pnaryMALevel[i] == 0)	
        	break;

        SelectedMagicTable.Clear();

        // lÊy dßng trong file magicattrib.txt
        const KBPT_ClassMAIT *pCMITItem = m_BPTLib.GetCMIT(
            1 - (i & 1),        // Èn = 0, hiÖn = 1
            nType,              // 0->9: lo¹i trang bŞ EQUIPDETAILTYPE
            nSeriesReq,         // 0->4: ngò hµnh 
            pnaryMALevel[i]     // level cña thuéc tİnh
        );

        _ASSERT(pCMITItem);
        if (!pCMITItem)
        {
            g_DebugLog(
                "[GenMagicAttrib] GetCMIT Error, nPrefixPostfix = %d, nType = %d, nSeries = %d, nLevel = %d", 
                1 - (i & 1), nType, nSeriesReq, pnaryMALevel[i]
            );
            break;
        }

        // sè l­îng thuéc tİnh cña level nµy
        int nCMITItemCount = pCMITItem->GetCount();
        int j = 0;
		int nDecide = MAX_LUCKRAND - GetRandomNumber(0, MAX_LUCKRAND);
        nDecide += (int)(dLucky*nDecide); //0->200000 + 0->200000*(100-lucky)/100

        for (j = 0; j < nCMITItemCount; j++)
        {
            // lÊy dßng thuéc tİnh cô thÓ
            int nMAIndex = pCMITItem->Get(j);
            KMAGICATTRIB_TABFILE *pMAItem = (KMAGICATTRIB_TABFILE *)m_BPTLib.GetMARecord(nMAIndex);
            if (!pMAItem)
            {
                _ASSERT(pMAItem);
                continue;
            }

            // ®· trïng lÆp trong 6 dßng
            if (pMAItem->m_nUseFlag)
                continue;

            // sè random > gi¸ trŞ may m¾n => kh«ng chän
            if ((pMAItem->m_DropRate[nType]) <= nDecide)
                continue;
            
            // kiÓm tra trïng lÆp id thuéc tİnh
            int k = 0;
            for (k = 0; k < i; k++)
            {
                _ASSERT(pMagicAttrTable[k]);

                if ((pMagicAttrTable[k]->m_MagicAttrib.nPropKind) == (pMAItem->m_MagicAttrib.nPropKind))
                    break;
            }

            // ®· trïng lÆp
            if (k < i)
                continue;
			//chän ®­îc thuéc tİnh
            SelectedMagicTable.Insert(nMAIndex);
        }

        int nSelectedCount = SelectedMagicTable.GetCount();

        if (nSelectedCount == 0)
        {
            // kh«ng chän ®­îc thuéc tİnh nµo
            break;
        }
        
        // lÊy ngÉu nhiªn 1 thuéc tİnh trong c¸c thuéc tİnh ®· chän ®­îc
        int nLuckyItemIndex = GetRandomNumber(0, nSelectedCount - 1);
        KMAGICATTRIB_TABFILE *pMAItem = (KMAGICATTRIB_TABFILE *)m_BPTLib.GetMARecord(
             SelectedMagicTable.Get(nLuckyItemIndex)
        );

        // l­u t¹m thêi ®Ó set m_nUseFlag
        pMagicAttrTable[i] = pMAItem;

        _ASSERT(pMAItem);
        _ASSERT(!(pMAItem->m_nUseFlag));

        // set ®· sö dông thuéc tİnh nµy
        pMAItem->m_nUseFlag = true;
		
        pnaryMA[i].nAttribType = pMAItem->m_MagicAttrib.nPropKind;
        pnaryMA[i].nValue[0] = GetRandomNumber(
            pMAItem->m_MagicAttrib.aryRange[0].nMin, 
            pMAItem->m_MagicAttrib.aryRange[0].nMax
        );
        pnaryMA[i].nValue[1] = GetRandomNumber(
            pMAItem->m_MagicAttrib.aryRange[1].nMin, 
            pMAItem->m_MagicAttrib.aryRange[1].nMax
        );
        pnaryMA[i].nValue[2] = GetRandomNumber(
            pMAItem->m_MagicAttrib.aryRange[2].nMin, 
            pMAItem->m_MagicAttrib.aryRange[2].nMax
        );
		/*g_DebugLog(
			"[GenMagicAttrib] pnaryMA[%d]: (%d:%s), nAttribType = %d, nValue[0] = %d, nValue[1] = %d, nValue[2] = %d,", 
			i,
			SelectedMagicTable.Get(nLuckyItemIndex),
			pMAItem->m_szName,
			pnaryMA[i].nAttribType,
			pnaryMA[i].nValue[0],
			pnaryMA[i].nValue[1],
			pnaryMA[i].nValue[2]
		);*/
    }
	//kh«ng cÇn clear c¸c dßng cuèi sãt l¹i, v× KItemNormalAttrib khi khëi t¹o ®· lµ 0
   /* for (NULL; i < 6; i++)
    {
        pnaryMA[i].nAttribType = 0;
        pnaryMA[i].nValue[0] = 0;
        pnaryMA[i].nValue[1] = 0;
        pnaryMA[i].nValue[2] = 0;
    }*/

    for (i = 0; i < 6; i++)
    {
        if (!pMagicAttrTable[i])
            break;

        _ASSERT(pMagicAttrTable[i]->m_nUseFlag);

        pMagicAttrTable[i]->m_nUseFlag = false;    // ®Æt l¹i ch­a sö dông thuéc tİnh cho lÇn sau
    }

    nResult = true;
//Exit0:
	return nResult;
}

// Following Code is implement by DongBo
//BOOL KItemGenerator::Gen_MagicAttrib(int nType, const int* pnaryMALevel, int nSeriesReq, int nLucky,
//									 KItemNormalAttrib* pnaryMA)
//{
//	
//    // È·¶¨ÊÊÓÃÓÚ¸Ã×°±¸µÄÈ«²¿Ä§·¨ÊôĞÔ¼°ÆäÊıÁ¿
//	if (FALSE == GMA_GetAvaliableMA(nType))
//		{ _ASSERT(FALSE); return FALSE; }
//
//	// È·¶¨ÉÏÊö¸÷Ä§·¨ÊôĞÔµÄµÈ¼¶,³öÏÖ¸ÅÂÊ
//	if (FALSE == GMA_GetLevelAndDropRate(nType))
//		{ _ASSERT(FALSE); return FALSE; }
//
//	// ´ÓÉÏÊöÄ§·¨ÊôĞÔÖĞÑ¡³ö·ûºÏÌõ¼şµÄ. ²ß»®ÒªÇóÑ¡³ö6¸ö
//	for (int i = 0; i < 6; i++)				// ÆæÊı: Ç°×º, Å¼Êı: ºó×º
//	{
//		GMA_ChooseMA((i&1), pnaryMALevel[i], nLucky, &(pnaryMA[i]));
//		if (pnaryMA[i].nAttribType == 0)
//			break;
//	}
//	return TRUE;
//}

/******************************************************************************
	¹¦ÄÜ:	È·¶¨ÊÊÓÃÓÚÖ¸¶¨×°±¸µÄÈ«²¿Ä§·¨ÊôĞÔ¼°ÆäÊıÁ¿
	Èë¿Ú:	nType: Ö¸¶¨×°±¸
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã. m_GMA_naryMA[0]¸ø³öÊÊÓÃÓÚ¸Ã×°±¸µÄÈ«²¿Ä§·¨ÊôĞÔÇ°×º
							m_GMA_nCount[0]¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄ×ÜÊı
							m_GMA_naryMA[1]¸ø³öÊÊÓÃÓÚ¸Ã×°±¸µÄÈ«²¿Ä§·¨ÊôĞÔºó×º
							m_GMA_nCount[1]¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄ×ÜÊı
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KItemGenerator::GMA_GetAvaliableMA(int nType)
{
	//TODO: ¿ÉÄÜĞèÒª¶ÔnType½øĞĞ×ª»», ´Ó×°±¸ÀàĞÍÖµ±ä³É m_BPTLib.m_CMAT Êı×éµÄË÷Òı
	for (int i = 0; i < 2; i++)		// 0: Ç°×º, 1: ºó×º
	{
		const KBPT_ClassifiedMAT* pCMAT;
		pCMAT = m_BPTLib.GetCMAT(i, nType);
		if (pCMAT == NULL)
			{ _ASSERT(FALSE); return FALSE; }
		m_GMA_nCount[i] = NUMOFCMA;
		pCMAT->GetAll((int*)m_GMA_naryMA[i], &(m_GMA_nCount[i]));
	}
	return TRUE;
}

/******************************************************************************
	¹¦ÄÜ:	È·¶¨¸÷¸ø¶¨Ä§·¨ÊôĞÔµÄµÈ¼¶ÒªÇóºÍ³öÏÖ¸ÅÂÊ
	Èë¿Ú:	nType: Ö¸¶¨×°±¸
			m_GMA_naryMAÊı×é: ¸ø³öÈ«²¿Ä§·¨ÊôĞÔµÄË÷ÒıÖµ
	³ö¿Ú:	³É¹¦Ê±·µ»Ø·ÇÁã. m_GMA_naryLevelÊı×é¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄµÈ¼¶ÒªÇó
							m_GMA_naryDropRate¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄ³öÏÖ¸ÅÂÊ
			Ê§°ÜÊ±·µ»ØÁã
******************************************************************************/
BOOL KItemGenerator::GMA_GetLevelAndDropRate(int nType)
{
	//TODO: ¿ÉÄÜĞèÒª¶ÔnType½øĞĞ×ª»», ´Ó×°±¸ÀàĞÍÖµ±ä³É m_BPTLib.m_CMAT Êı×éµÄË÷Òı

	for (int i = 0; i < 2; i++)		// 0: Ç°×º, 1: ºó×º
	{
		for (int n = 0; n < m_GMA_nCount[i]; n++)
		{
			const KMAGICATTRIB_TABFILE* pMATF;
			pMATF = GetMARecord(m_GMA_naryMA[i][n]);
			if (pMATF == NULL)
				{ _ASSERT(FALSE); return FALSE; }
			m_GMA_naryLevel[i][n] = pMATF->m_nLevel;
			m_GMA_naryDropRate[i][n] = pMATF->m_DropRate[nType];		// ¿ÉÓÅ»¯
		}
	}
	return TRUE;
}

/******************************************************************************
	¹¦ÄÜ:	´ÓÈ«²¿¿ÉÓÃµÄÄ§·¨ÊôĞÔÖĞÑ¡³ö·ûºÏÌõ¼şµÄÄ§·¨
	Èë¿Ú:	nPos: 0: Ñ¡Ç°×º, 1: Ñ¡ºó×º
			nLevel: ¶ÔÄ§·¨ÊôĞÔµÄµÈ¼¶ÒªÇó
			nLucky: ÔËÆøÖµ
			m_GMA_naryMAÊı×é: ¸ø³öÈ«²¿¿ÉÓÃµÄÄ§·¨ÊôĞÔµÄË÷ÒıÖµ
	³ö¿Ú:	*pINA ¸ø³öËùÑ¡Ä§·¨µÄºËĞÄ²ÎÊı
******************************************************************************/
void KItemGenerator::GMA_ChooseMA(int nPos, int nLevel, int nLucky, KItemNormalAttrib* pINA)
{
	_ASSERT(pINA != NULL);

	// ¸³³õÖµ
	pINA->nAttribType = 0;
	pINA->nValue[0] = 0;
	pINA->nValue[1] = 0;
	pINA->nValue[2] = 0;

	// É¸Ñ¡³öÈ«²¿¿ÉÓÃµÄÄ§·¨ÊôĞÔ
	int nDropRate = ::GetRandomNumber(0, 100) - nLucky;
	int nCount = GMA_GetCandidateMA(nPos, nLevel, nDropRate);
	if (nCount > 0)
	{
		int nFinal = ::GetRandomNumber(0, nCount);
		int nMAi = m_GMA_naryCandidateMA[nFinal];
		int nMA = m_GMA_naryMA[nPos][nMAi];
		if (nMA == -1)	// Ëæ»ú³öÖØ¸´µÄÊôĞÔÁË
		{
			pINA->nAttribType = 0;
			return;
		}
		m_GMA_naryMA[nPos][nMAi] = -1;		// ÖÃÎª-1,±íÊ¾¸ÃMAÒÑÓÃ
		const KMAGICATTRIB_TABFILE* pMATF = GetMARecord(nMA);
		if (NULL != pMATF)
		{
			const KMACP* pMACP = &(pMATF->m_MagicAttrib);
			pINA->nAttribType = pMACP->nPropKind;
			pINA->nValue[0] = ::GetRandomNumber(pMACP->aryRange[0].nMin, pMACP->aryRange[0].nMax);
			pINA->nValue[1] = ::GetRandomNumber(pMACP->aryRange[1].nMin, pMACP->aryRange[1].nMax);
			pINA->nValue[2] = ::GetRandomNumber(pMACP->aryRange[2].nMin, pMACP->aryRange[2].nMax);
		}
	}
}

/******************************************************************************
	¹¦ÄÜ:	»ñÈ¡Ö¸¶¨µÄÄ§·¨ÊôĞÔ¼ÇÂ¼
	Èë¿Ú:	i: Ö¸¶¨¼ÇÂ¼
	³ö¿Ú:	³É¹¦Ê±·µ»ØÖ¸Ïò¸Ã¼ÇÂ¼µÄÖ¸Õë
			Ê§°ÜÊ±·µ»ØNULL
******************************************************************************/
const KMAGICATTRIB_TABFILE* KItemGenerator::GetMARecord(int i) const
{
	return m_BPTLib.GetMARecord(i);
}

/******************************************************************************
	¹¦ÄÜ:	´ÓÈ«²¿¿ÉÓÃµÄÄ§·¨ÊôĞÔÖĞÑ¡³ö·ûºÏÌõ¼şµÄÄ§·¨
	Èë¿Ú:	nPos: 0: Ñ¡Ç°×º, 1: Ñ¡ºó×º
			nLevel: ¶ÔÄ§·¨ÊôĞÔµÄµÈ¼¶ÒªÇó
			nDropRate: Ñ¡³ö³öÏÖ¸ÅÂÊ´óÓÚ´ËÖµµÄÄ§·¨ÊôĞÔ
			m_GMA_naryMAÊı×é: ¸ø³öÈ«²¿¿ÉÓÃµÄÄ§·¨ÊôĞÔµÄË÷ÒıÖµ
			m_GMA_naryLevelÊı×é: ¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄµÈ¼¶ÒªÇó
			m_GMA_naryDropRateÊı×é: ¸ø³öÉÏÊöÄ§·¨ÊôĞÔµÄ³öÏÖ¸ÅÂÊ
	³ö¿Ú:	m_GMA_naryCandidateMAÊı×é¸ø³ö·ûºÏÌõ¼şµÄÄ§·¨
	ËµÃ÷:	Èô n = m_GMA_naryMA[nPos][m_GMA_naryCandidateMA[i]]
			Ôò GetMARecord(n) ·µ»ØÖ¸ÏòÄ§·¨ÊôĞÔ½á¹¹µÄÖ¸Õë
******************************************************************************/
int KItemGenerator::GMA_GetCandidateMA(int nPos, int nLevel, int nDropRate)
{
	int nCount = 0;
	for (int i = 0; i < m_GMA_nCount[nPos]; i++)
	{
		if (m_GMA_naryMA[nPos][i] != -1 &&		// -1±íÊ¾¸ÃMAÒÑÓÃ¹ı
			m_GMA_naryLevel[nPos][i] == nLevel &&
			m_GMA_naryDropRate[nPos][i] >= nDropRate)
		{
			m_GMA_naryCandidateMA[nCount++] = i;
		}
	}
	return nCount;
}

/******************************************************************************
T¹o item khi load tõ database
Editor: Guve
******************************************************************************/
BOOL KItemGenerator::Gen_ExistEquipment(IN int nDetailType,
								   IN int nParticularType,
								   IN int nSeriesReq,
								   IN int nLevel,
								   IN const int* pnaryMALevel,
								   IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	int i = nParticularType * 10 + nLevel - 1;
	const KBASICPROP_EQUIPMENT* pEqu = NULL;
	switch(nDetailType)
	{
	case equip_meleeweapon:
		pEqu = m_BPTLib.GetMeleeWeaponRecord(i);
		break;
	case equip_rangeweapon:
		pEqu = m_BPTLib.GetRangeWeaponRecord(i);
		break;
	case equip_armor:
		pEqu = m_BPTLib.GetArmorRecord(i);
		break;
	case equip_helm:
		pEqu = m_BPTLib.GetHelmRecord(i);
		break;
	case equip_boots:
		pEqu = m_BPTLib.GetBootRecord(i);
		break;
	case equip_belt:
		pEqu = m_BPTLib.GetBeltRecord(i);
		break;
	case equip_amulet:
		pEqu = m_BPTLib.GetAmuletRecord(i);
		break;
	case equip_ring:
		pEqu = m_BPTLib.GetRingRecord(i);
		break;
	case equip_cuff:
		pEqu = m_BPTLib.GetCuffRecord(i);
		break;
	case equip_pendant:
		pEqu = m_BPTLib.GetPendantRecord(i);
		break;
	case equip_horse:
		pEqu = m_BPTLib.GetHorseRecord(i);
		break;
	case equip_mask:
		pEqu = m_BPTLib.GetMaskRecord(nParticularType);
		break;
	default:
		break;
	}
	if (NULL == pEqu)
		{ _ASSERT(FALSE); return bEC; }
	pItem->SetAttrib_CBR(pEqu);
	if(nDetailType == equip_mask)
	{
		if(nLevel > 0)
			pItem->SetLevel(nLevel);
		else
			pItem->SetLevel(0);
	}
	else
		pItem->SetSeries(nSeriesReq);
	bEC = TRUE;
	KItemNormalAttrib	sMA[6];
	for(i=0; i<6; ++i)
	{
		if(!pnaryMALevel[i*2])
			break;
		sMA[i].nAttribType = pnaryMALevel[i*2];
		sMA[i].nValue[0] = pnaryMALevel[i*2+1];
		sMA[i].nValue[1] = -1;
		if(sMA[i].nAttribType == magic_addphysicsdamage_p)
			sMA[i].nValue[2] = 6;
	}
	pItem->SetAttrib_MA(sMA);

	return bEC;
}

BOOL KItemGenerator::Gen_Quest(IN int nDetailType, IN OUT KItem* pItem, IN int nCount)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	
	const KBASICPROP_QUEST* pQuest = NULL;
	pQuest = m_BPTLib.GetQuestRecord(nDetailType);
	if (NULL == pQuest)
	{
		_ASSERT(FALSE);
		return bEC;
	}
	*pItem = *pQuest;
	if(nCount > 0)
		pItem->SetCount(nCount);
	return TRUE;
}

BOOL KItemGenerator::Gen_TownPortal(IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	
	const KBASICPROP_TOWNPORTAL* pPortal = NULL;
	pPortal = m_BPTLib.GetTownPortalRecord(0);
	if (NULL == pPortal)
	{
		_ASSERT(FALSE);
		return bEC;
	}
	*pItem = *pPortal;
	return TRUE;
}

BOOL KItemGenerator::Gen_MAScript(IN int nDetailType,
								   IN int nParticularType,
								   IN int nLevel,
								   IN int nSeriesReq,
								   IN int nCount,
								   IN const int* pnaryMALevel,
								   IN OUT KItem* pItem)
{
	_ASSERT(this != NULL);
	_ASSERT(pItem != NULL);

	BOOL bEC = FALSE;
	int i = m_BPTLib.FindMAScriptRecord(nDetailType, nParticularType);
	const KBASICPROP_MASCRIPT* pMAScript = m_BPTLib.GetMAScriptRecord(i);
	if (NULL == pMAScript)
	{
		return bEC;
	}
	*pItem = *pMAScript;
	pItem->SetGenParam(i);
	if(nLevel > 0)
		pItem->SetLevel(nLevel);
	if(nSeriesReq >= 0 && nSeriesReq < series_num)
		pItem->SetSeries(nSeriesReq);
	pItem->SetCount(nCount);
	if(pMAScript->m_nSkillID > 0)
		pItem->SetGroupId(pMAScript->m_nSkillID);
	return TRUE;
}
