#include "KCore.h"
#include "KItem.h"
#include "KItemGenerator.h"
#include "KItemSet.h"
//#include "MyAssert.h"

KItemSet	ItemSet;
/*!*****************************************************************************
// Function		: KItemSet::KItemSet
// Purpose		: 
// Return		: 
// Comments		:
// Author		: Spe
*****************************************************************************/
KItemSet::KItemSet()
{
	m_dwIDCreator = 100;
	ZeroMemory(&m_sRepairParam, sizeof(REPAIR_ITEM_PARAM));
#ifdef _SERVER
	m_psItemInfo = NULL;
	m_psBackItemInfo = NULL;
#endif
}

KItemSet::~KItemSet()
{
#ifdef _SERVER
	if (m_psItemInfo)
		delete [] m_psItemInfo;
	m_psItemInfo = NULL;
	if (m_psBackItemInfo)
		delete [] m_psBackItemInfo;
	m_psBackItemInfo = NULL;
#endif
}

/*!*****************************************************************************
// Function		: KItemSet::Init
// Purpose		: 
// Return		: void 
// Comments		:
// Author		: Spe
*****************************************************************************/
void KItemSet::Init()
{
	m_FreeIdx.Init(MAX_ITEM);
	m_UseIdx.Init(MAX_ITEM);

	for (int i = MAX_ITEM - 1; i > 0; i--)
	{
		m_FreeIdx.Insert(i);
	}
#ifdef _SERVER
	if (m_psItemInfo)
		delete [] m_psItemInfo;
	m_psItemInfo = NULL;
	m_psItemInfo = new TRADE_ITEM_INFO[TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT];
	memset(this->m_psItemInfo, 0, sizeof(TRADE_ITEM_INFO) * TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT);
	if (m_psBackItemInfo)
		delete [] m_psBackItemInfo;
	m_psBackItemInfo = NULL;
	m_psBackItemInfo = new TRADE_ITEM_INFO[TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT];
	memset(this->m_psBackItemInfo, 0, sizeof(TRADE_ITEM_INFO) * TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT);
#endif
	KIniFile	IniFile;
	IniFile.Load(ITEM_ABRADE_FILE);
//	Î¬ÐÞ¼Û¸ñ
	IniFile.GetInteger("Repair", "ItemPriceScale", 100, &m_sRepairParam.nPriceScale);
	IniFile.GetInteger("Repair", "MagicPriceScale", 10, &m_sRepairParam.nMagicScale);
//	¹¥»÷Ä¥Ëð
	IniFile.GetInteger("Attack", "Weapon", 256, &m_nItemAbradeRate[enumAbradeAttack][itempart_weapon]);
	IniFile.GetInteger("Attack", "Head", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_head]);
	IniFile.GetInteger("Attack", "Body", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_body]);
	IniFile.GetInteger("Attack", "Belt", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_belt]);
	IniFile.GetInteger("Attack", "Foot", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_foot]);
	IniFile.GetInteger("Attack", "Cuff", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_cuff]);
	IniFile.GetInteger("Attack", "Amulet", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_amulet]);
	IniFile.GetInteger("Attack", "Ring1", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_ring1]);
	IniFile.GetInteger("Attack", "Ring2", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_ring2]);
	IniFile.GetInteger("Attack", "Pendant", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_pendant]);
	IniFile.GetInteger("Attack", "Horse", 0, &m_nItemAbradeRate[enumAbradeAttack][itempart_horse]);
// ·ÀÓùÄ¥Ëð
	IniFile.GetInteger("Defend", "Weapon", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_weapon]);
	IniFile.GetInteger("Defend", "Head", 64, &m_nItemAbradeRate[enumAbradeDefend][itempart_head]);
	IniFile.GetInteger("Defend", "Body", 64, &m_nItemAbradeRate[enumAbradeDefend][itempart_body]);
	IniFile.GetInteger("Defend", "Belt", 64, &m_nItemAbradeRate[enumAbradeDefend][itempart_belt]);
	IniFile.GetInteger("Defend", "Foot", 64, &m_nItemAbradeRate[enumAbradeDefend][itempart_foot]);
	IniFile.GetInteger("Defend", "Cuff", 64, &m_nItemAbradeRate[enumAbradeDefend][itempart_cuff]);
	IniFile.GetInteger("Defend", "Amulet", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_amulet]);
	IniFile.GetInteger("Defend", "Ring1", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_ring1]);
	IniFile.GetInteger("Defend", "Ring2", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_ring2]);
	IniFile.GetInteger("Defend", "Pendant", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_pendant]);
	IniFile.GetInteger("Defend", "Horse", 0, &m_nItemAbradeRate[enumAbradeDefend][itempart_horse]);
// ÒÆ¶¯Ä¥Ëð
	IniFile.GetInteger("Move", "Weapon", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_weapon]);
	IniFile.GetInteger("Move", "Head", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_head]);
	IniFile.GetInteger("Move", "Body", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_body]);
	IniFile.GetInteger("Move", "Belt", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_belt]);
	IniFile.GetInteger("Move", "Foot", 64, &m_nItemAbradeRate[enumAbradeMove][itempart_foot]);
	IniFile.GetInteger("Move", "Cuff", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_cuff]);
	IniFile.GetInteger("Move", "Amulet", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_amulet]);
	IniFile.GetInteger("Move", "Ring1", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_ring1]);
	IniFile.GetInteger("Move", "Ring2", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_ring2]);
	IniFile.GetInteger("Move", "Pendant", 0, &m_nItemAbradeRate[enumAbradeMove][itempart_pendant]);
	IniFile.GetInteger("Move", "Horse", 64, &m_nItemAbradeRate[enumAbradeMove][itempart_horse]);

}

/*!*****************************************************************************
// Function		: KItemSet::SearchID
// Purpose		: 
// Return		: int 
// Argumant		: DWORD dwID
// Comments		:
// Author		: Spe
*****************************************************************************/
int KItemSet::SearchID(DWORD dwID)
{
	int nIdx = 0;

	while(1)
	{
		nIdx = m_UseIdx.GetNext(nIdx);
		if (!nIdx)
			break;
		if (Item[nIdx].GetID() == dwID)
			break;
	}
	return nIdx;
	
}

int KItemSet::Add(KItem* pItem)
{
	KASSERT(NULL != pItem);

	int i = FindFree();

	if (!i)
		return 0;

	Item[i] = *pItem;
#ifdef _SERVER
	SetID(i);
#endif
	m_FreeIdx.Remove(i);
	m_UseIdx.Insert(i);
	return i;

}
/*!*****************************************************************************
// Function		: KItemSet::Add
// Purpose		: 
// Return		: int Êý×é±àºÅ
// Argumant		: int µÀ¾ßÀàÐÍ£¨×°±¸£¿Ò©Æ·£¿¿óÊ¯£¿¡­¡­£©
// Argumant		: int Ä§·¨µÈ¼¶£¨Èç¶ÔÓ¦ÓÚ×°±¸£¬¾ÍÊÇÒ»°ã×°±¸£¬À¶É«×°±¸£¬ÁÁ½ðµÈ¡­¡­£©
// Argumant		: int ÎåÐÐÊôÐÔ
// Argumant		: int µÈ¼¶
// Argumant		: int ÐÒÔËÖµ
// Comments		:
// Author		: Spe
*****************************************************************************/
int KItemSet::Add(int nItemGenre, int nSeries, 
				  int nLevel, int nLuck, int nDetailType/*=-1*/, 
				  int nParticularType/*=-1*/, int* pnMagicLevel)
{
	int i = FindFree();
	
	if (i == 0)
		return 0;

	KItem*	pItem = &Item[i];
	BOOL bGetEquiptResult = FALSE;
	switch(nItemGenre)
	{
	case item_equip:			// trang bÞ
		bGetEquiptResult = ItemGen.Gen_Equipment(nDetailType, nParticularType, nSeries, nLevel, pnMagicLevel, nLuck, pItem);
		break;
	case item_medicine:			// thuèc men
		bGetEquiptResult = ItemGen.Gen_Medicine(nDetailType, nLevel, pItem);
		break;
	case item_task:				// nhiÖm vô
		bGetEquiptResult = ItemGen.Gen_Quest(nDetailType, pItem, nLuck);
		break;
	case item_townportal:
		bGetEquiptResult = ItemGen.Gen_TownPortal(pItem);
		break;
	case item_gengold:
		bGetEquiptResult = ItemGen.Gen_GoldEquip(nLuck, nDetailType, pItem, pnMagicLevel);
		break;
	case item_genpurple:
		bGetEquiptResult = ItemGen.Gen_PurpleEquip(nDetailType, nParticularType, nSeries, nLevel, pnMagicLevel, nLuck, pItem);
		break;
	case item_mascript:
		bGetEquiptResult = ItemGen.Gen_MAScript(nDetailType, nParticularType, nLevel, nSeries, nLuck, pnMagicLevel, pItem);
		break;
	default:
		return 0;
	}
	if(!bGetEquiptResult)
		return 0;
#ifdef _SERVER
	SetID(i);
#endif
	m_FreeIdx.Remove(i);
	m_UseIdx.Insert(i);
	return i;
}

/*!*****************************************************************************
// Function		: KItemSet::FindFree
// Purpose		: 
// Return		: int 
// Comments		:
// Author		: Spe
*****************************************************************************/
int KItemSet::FindFree()
{
	return m_FreeIdx.GetNext(0);
}

void KItemSet::Remove(IN int nIdx)
{
	Item[nIdx].Remove();
	
	m_UseIdx.Remove(nIdx);
	m_FreeIdx.Insert(nIdx);
}

void KItemSet::SetID(IN int nIdx)
{
	Item[nIdx].SetID(m_dwIDCreator);
	m_dwIDCreator++;
}

#ifdef _SERVER
//---------------------------------------------------------------------------
//	¹¦ÄÜ£ºcopy m_psItemInfo to m_psBackItemInfo
//---------------------------------------------------------------------------
void	KItemSet::BackItemInfo()
{
	_ASSERT(this->m_psItemInfo);
	_ASSERT(this->m_psBackItemInfo);
	if (!m_psItemInfo)
		return;
	if (!m_psBackItemInfo)
		m_psBackItemInfo = new TRADE_ITEM_INFO[TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT];
	memcpy(m_psBackItemInfo, this->m_psItemInfo, sizeof(TRADE_ITEM_INFO) * TRADE_ROOM_WIDTH * TRADE_ROOM_HEIGHT);
}
#endif

int KItemSet::GetAbradeRange(IN int nType, IN int nPart)
{
	if (nType < 0 || nType >= enumAbradeNum)
		return 0;
	if (nPart < 0 || nPart >= itempart_horse)
		return 0;

	return m_nItemAbradeRate[nType][nPart];
}