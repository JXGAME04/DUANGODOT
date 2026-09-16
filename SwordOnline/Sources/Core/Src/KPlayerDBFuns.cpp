#include "KCore.h"

#ifdef _SERVER
#include "KEngine.h"
#include "KSubWorldSet.h"
#include "KSubWorld.h"
#include "KPlayer.h"
#ifndef _STANDALONE
#include "../../../lib/S3DBInterface.h"
#else
#include "S3DBInterface.h"
#endif
#include "KNpc.h"
#include "KItem.h"
#include "KItemList.h"
#include "KItemGenerator.h"
#include "KItemSet.h"
#include "KNpcSet.h"
#include "KPlayerSet.h"
#include "KItemChangeRes.h"
#include <time.h>
//#include "MyAssert.H"
#include "KTaskFuns.h"


// ÊÇ·ñ½«Êý¾Ý¿â´æÈ¡µµµÄÊý¾Ý±£´æÏÂÀ´ÒÔ¹©µ÷ÊÔ
//#define DEBUGOPT_DB_ROLEDATA_OUT 




KList g_DBMsgList;

int KPlayer::AddDBPlayer(char * szPlayerName, int sex, DWORD * pdwID)
{
	return FALSE;
}

int KPlayer::LoadDBPlayerInfo(BYTE *pPlayerInfo, int &nStep, unsigned int &nParam)
{
	_ASSERT(pPlayerInfo);
	int nRet = 0;
	int nRetValue = 0;
	
	switch(nStep)
	{
	case STEP_BASE_INFO:
		//Êý¾Ý¿âÖÐÍæ¼Ò»ù±¾ÐÅÏ¢±í
		m_pCurStatusOffset = pPlayerInfo;
		
		if ((nRet = LoadPlayerBaseInfo(pPlayerInfo, m_pCurStatusOffset, nParam)) == 1)
		{
			nRetValue = SendSyncData(nStep, nParam);
			nStep++; 
			nParam = 0;
		}
		else 
		{
			if (nRet == -1)
			{
				nStep++;
				nParam = 0;
				return 0;
			}
			else
			{
				nRetValue = SendSyncData(nStep, nParam);
			}
		}
		break;
	case STEP_FIGHT_SKILL_LIST:
		//************************************************
		// Íæ¼ÒµÄÕ½¶·¼¼ÄÜÁÐ±í
		if ((nRet = LoadPlayerFightSkillList(pPlayerInfo, m_pCurStatusOffset, nParam)) == 1)
		{
			nRetValue = SendSyncData(nStep, nParam);
			nStep++;
			nParam = 0;
		}
		else
		{
			if (nRet == -1)
			{
				nStep++;
				nParam = 0;
				return 0;
			}
//			else
//			{
//				nRetValue = SendSyncData(nStep, nParam);
//			}
		}
		break;
	case STEP_LIFE_SKILL_LIST:
		//*************************************************
		// Íæ¼ÒµÄÉú»î¼¼ÄÜÁÐ±í
		if ((nRet = LoadPlayerLifeSkilllList(pPlayerInfo, m_pCurStatusOffset, nParam)) == 1)
		{
			nRetValue = SendSyncData(nStep, nParam);
			nStep++;
			nParam = 0;
		}
		else
		{
			if (nRet == -1)
			{
				nStep ++;
				nParam = 0;
				return 0;
			}
			else
			{
				nRetValue = SendSyncData(nStep, nParam);
			}
		}
		
		break;
		
	case STEP_TASK_LIST:
		//*************************************************		
		// Íæ¼ÒµÄÈÎÎñÁÐ±í
		if ((nRet = LoadPlayerTaskList(pPlayerInfo, m_pCurStatusOffset, nParam)) == 1)
		{
			
			int i = 0;
			for (i  = 0; i < TASKVALUE_MAXWAYPOINT_COUNT; i ++)
			{
				int nWayPoint = m_cTask.nSave[TASKVALUE_SAVEWAYPOINT_BEGIN + i];
				if (nWayPoint)
				{
					KIndexNode * pNewNode = new KIndexNode;
					pNewNode->m_nIndex = nWayPoint;
					m_PlayerWayPointList.AddTail(pNewNode);
				}
			}
			
			for (i  = 0; i < TASKVALUE_MAXSTATION_COUNT / 2; i ++)
			{
				DWORD Stations = 0;
				if (Stations = (DWORD) m_cTask.GetSaveVal(TASKVALUE_SAVESTATION_BEGIN + i))
				{
					int nStation1 = (int) HIWORD(Stations);
					int nStation2 = (int) LOWORD(Stations);
					
					if (nStation1)
					{
						KIndexNode * pNewNode = new KIndexNode;
						pNewNode->m_nIndex = nStation1;
						m_PlayerStationList.AddTail(pNewNode);
					}
					
					if (nStation2)
					{
						KIndexNode * pNewNode = new KIndexNode;
						pNewNode->m_nIndex = nStation2;
						m_PlayerStationList.AddTail(pNewNode);
					}
				}
			}
			g_TimerTask.LoadTask(this);
			nRetValue = SendSyncData(nStep, nParam);
			nStep++;
			nParam = 0;
		}
		else
		{
			if (nRet == -1)
			{
				nStep ++;
				nParam = 0;
				return 0;
			}
			else
			{
				nRetValue = SendSyncData(nStep, nParam);
			}
		}
		break;
	case STEP_ITEM_LIST:
		//*************************************************
		// Íæ¼ÒµÄ×°±¸ÁÐ±í
		if ((nRet = LoadPlayerItemList(pPlayerInfo, m_pCurStatusOffset, nParam)) == 1)
		{
			nRetValue = SendSyncData(nStep, nParam);
			nStep++;
			nParam = 0;
		}
		else
		{
			if (nRet == -1)
			{
				nStep ++;
				nParam = 0;
				return 0;
			}
			else
			{
				nRetValue = SendSyncData(nStep, nParam);
			}
		}
		break;
	default:
		nStep = STEP_SYNC_END;
		break;
	}
	return nRetValue;
}

int KPlayer::UpdateDBPlayerInfo(BYTE* pPlayerInfo)
{
	//×¢Òâ£¬´æµµÈ¡µµ¶¼±ØÐë°´ÕÕPË³Ðò
	if (!pPlayerInfo)
		return -1;

	//Êý¾Ý¿âÖÐÍæ¼Ò»ù±¾ÐÅÏ¢±í
	SavePlayerBaseInfo(pPlayerInfo);

	// Íæ¼ÒµÄÕ½¶·¼¼ÄÜÁÐ±í
	SavePlayerFightSkillList(pPlayerInfo);

	// Íæ¼ÒµÄÉú»î¼¼ÄÜÁÐ±í
	SavePlayerLifeSkilllList(pPlayerInfo);

	// Íæ¼ÒµÄÈÎÎñÁÐ±í
	SavePlayerTaskList(pPlayerInfo);

	// Íæ¼ÒµÄÎïÆ·ÁÐ±í
	SavePlayerItemList(pPlayerInfo);

	return 1;	
}

int	KPlayer::LoadPlayerBaseInfo(BYTE  * pRoleBuffer, BYTE * &pCurData, unsigned int &nParam)
{
	if (!pRoleBuffer)
	{
		KASSERT(pRoleBuffer);
		return -1;
	}
	
	TRoleData * pRoleData = (TRoleData*)pRoleBuffer;
	
	if (nParam != 0) return -1;
	
	int		nSex;
	int		nLevel;	
#define PLAYER_MALE_NPCTEMPLATEID		-1
#define PLAYER_FEMALE_NPCTEMPLATEID		-2


	nLevel = pRoleData->BaseInfo.ifightlevel;
	nSex = pRoleData->BaseInfo.bSex;
	if (nSex)
		nSex = MAKELONG(nLevel, PLAYER_FEMALE_NPCTEMPLATEID);
	else
		nSex = MAKELONG(nLevel, PLAYER_MALE_NPCTEMPLATEID);

	//µÇÈëµã
	m_sLoginRevivalPos.m_nSubWorldID = pRoleData->BaseInfo.irevivalid;
	m_sLoginRevivalPos.m_ReviveID = pRoleData->BaseInfo.irevivalx;
	//---------------------------------------------------------------------------------------------
label_retry:
	POINT Pos;
	g_SubWorldSet.GetRevivalPosFromId(m_sLoginRevivalPos.m_nSubWorldID,
		m_sLoginRevivalPos.m_ReviveID,
		&Pos);
	
	m_sLoginRevivalPos.m_nMpsX = Pos.x;
	m_sLoginRevivalPos.m_nMpsY = Pos.y;
		//---------------------------------------------------------------------------------------------
	PLAYER_REVIVAL_POS	tempPos;
	if (pRoleData->BaseInfo.cUseRevive)
	{
		tempPos.m_nSubWorldID = m_sLoginRevivalPos.m_nSubWorldID;
		tempPos.m_ReviveID = m_sLoginRevivalPos.m_ReviveID;
		tempPos.m_nMpsX = m_sLoginRevivalPos.m_nMpsX;
		tempPos.m_nMpsY = m_sLoginRevivalPos.m_nMpsY;
	}
	else
	{
		tempPos.m_nSubWorldID = pRoleData->BaseInfo.ientergameid;
		tempPos.m_nMpsX = pRoleData->BaseInfo.ientergamex;
		tempPos.m_nMpsY = pRoleData->BaseInfo.ientergamey;
	}

	m_nIndex = NpcSet.Add(nSex, 
		g_SubWorldSet.SearchWorld(tempPos.m_nSubWorldID),
		tempPos.m_nMpsX,
		tempPos.m_nMpsY);

	if(m_nIndex <= 0) 
	{
		g_DebugLog("[Error!]AddNpc Error DBFuns.cpp");
		if (pRoleData->BaseInfo.cUseRevive)
		{
			return -1;
		}
		else
		{
			pRoleData->BaseInfo.cUseRevive = 1;
			goto label_retry;
		}
	}

	m_sDeathRevivalPos = m_sLoginRevivalPos;

	KNpc* pNpc = &Npc[m_nIndex];
	pNpc->m_Kind = kind_player;
	pNpc->SetPlayerIdx(m_nPlayerIndex);
	pNpc->m_Level = nLevel;

	//Íæ¼ÒÐÅÏ¢
	strcpy(pNpc->Name, pRoleData->BaseInfo.szName);
	m_nForbiddenFlag = pRoleData->BaseInfo.nForbiddenFlag;
	
	m_nAttributePoint = pRoleData->BaseInfo.ileftprop;
	m_nSkillPoint = pRoleData->BaseInfo.ileftfight;

	m_nStrength		= pRoleData->BaseInfo.ipower;
	m_nDexterity	= pRoleData->BaseInfo.iagility;
	m_nVitality		= pRoleData->BaseInfo.iouter;
	m_nEngergy		= pRoleData->BaseInfo.iinside;
	m_nLucky		= pRoleData->BaseInfo.iluck;
	m_cTong.Clear();
	m_cTong.DBSetTongNameID(pRoleData->BaseInfo.dwTongID);

	m_nCurStrength = m_nStrength;
	m_nCurDexterity = m_nDexterity;
	m_nCurVitality = m_nVitality;
	m_nCurEngergy = m_nEngergy;
	m_nCurLucky = m_nLucky;
	this->SetFirstDamage();
	this->SetBaseAttackRating();
	this->SetBaseDefence();

	//Íæ¼ÒµÈ¼¶ÐÅÏ¢
	m_nExp			= pRoleData->BaseInfo.fightexp;
	m_nNextLevelExp = PlayerSet.m_cLevelAdd.GetLevelExp(pNpc->m_Level);
	m_dwLeadLevel	= pRoleData->BaseInfo.ileadlevel;
	m_dwLeadExp		= pRoleData->BaseInfo.ileadexp;

	//ÃÅÅÉÐÅÏ¢
	m_cFaction.m_nCurFaction = (char)pRoleData->BaseInfo.nSect;
	m_cFaction.m_nFirstAddFaction = (char)pRoleData->BaseInfo.nFirstSect;
	m_cFaction.m_nAddTimes			= pRoleData->BaseInfo.ijoincount;
	pNpc->m_btRankId				= pRoleData->BaseInfo.isectrole;
	m_nWorldStat	= pRoleData->BaseInfo.nWorldStat;
	m_nSectStat		= pRoleData->BaseInfo.nSectStat;
	
	//ÏÖ½ðºÍÖüÎïÏäÖÐµÄÇ®
	int nCashMoney = 0;
	int nSaveMoney = 0;
//	this->m_ItemList.Init(GetPlayerIndex());
	nCashMoney		= pRoleData->BaseInfo.imoney;
	nSaveMoney		= pRoleData->BaseInfo.isavemoney;
	m_ItemList.SetMoney(nCashMoney, nSaveMoney,0);

	pNpc->m_Series	= pRoleData->BaseInfo.ifiveprop;
	pNpc->m_Camp	= pRoleData->BaseInfo.iteam;
	
	pNpc->m_nSex	= pRoleData->BaseInfo.bSex;

	pNpc->m_LifeMax	= pRoleData->BaseInfo.imaxlife;
	pNpc->m_StaminaMax = pRoleData->BaseInfo.imaxstamina;
	pNpc->m_ManaMax = pRoleData->BaseInfo.imaxinner;

	pNpc->m_LifeReplenish = PLAYER_LIFE_REPLENISH;
	pNpc->m_ManaReplenish = PLAYER_MANA_REPLENISH;
	pNpc->m_StaminaGain = PLAYER_STAMINA_GAIN;
	pNpc->m_StaminaLoss = PLAYER_STAMINA_LOSS;

	this->SetBaseResistData();
	SetBaseSpeedAndRadius();
	pNpc->RestoreNpcBaseInfo();
	
	pNpc->m_CurrentLife = pRoleData->BaseInfo.icurlife;
	pNpc->m_CurrentMana = pRoleData->BaseInfo.icurinner;
	pNpc->m_CurrentStamina = pRoleData->BaseInfo.icurstamina;

	// PKÐÅÏ¢
	m_cPK.SetNormalPKState(pRoleData->BaseInfo.cPkStatus);
	m_cPK.SetPKValue(pRoleData->BaseInfo.ipkvalue);

	// ³õÊ¼»¯²¿·ÖÊý¾Ý£¨ÕâÐ©Êý¾ÝÊý¾Ý¿â²»´æ´¢£©
	m_BuyInfo.Clear();
	m_cMenuState.Release();
	m_cChat.Release();
	m_cTeam.Release();
	m_cTeam.SetCanTeamFlag(m_nPlayerIndex, TRUE);
	m_nPeapleIdx = 0;
	m_nObjectIdx = 0;
	memset(m_szTaskAnswerFun, 0, sizeof(m_szTaskAnswerFun));
	m_nAvailableAnswerNum = 0;
	Npc[m_nIndex].m_ActionScriptID = 0;
	Npc[m_nIndex].m_TrapScriptID = 0;
	m_nViewEquipTime = 0;

	pNpc->m_Experience = 0;
//	memset(pNpc->m_szChatBuffer, 0, sizeof(pNpc->m_szChatBuffer));
//	pNpc->m_nCurChatTime = 0;

	pNpc->m_WeaponType = g_ItemChangeRes.GetWeaponRes(0, 0, 0);
	pNpc->m_ArmorType = g_ItemChangeRes.GetArmorRes(0, 0);
	pNpc->m_HelmType = g_ItemChangeRes.GetHelmRes(0, 0);
	pNpc->m_HorseType = g_ItemChangeRes.GetHorseRes(0, 0);
	pNpc->m_bRideHorse = FALSE;
	nParam = 1;
	pCurData = (BYTE *)&pRoleData->pBuffer;
	// µÇÈëÓÎÏ·Ê±Õ½¶·Ä£Ê½
	pNpc->m_FightMode = pRoleData->BaseInfo.cFightMode;
	// ÊÇ·ñÊÇ´¦ÓÚ¿ç·þÎñÆ÷×´Ì¬
	if (pRoleData->BaseInfo.cIsExchange)
	{
	}
	return 1;
}

int	KPlayer::LoadPlayerItemList(BYTE * pRoleBuffer , BYTE* &pItemBuffer, unsigned int &nParam)
{
	KASSERT(pRoleBuffer);
	
	int nItemCount = ((TRoleData *)pRoleBuffer)->nItemCount;
	TDBItemData * pItemData = (TDBItemData *)pItemBuffer;
	
	if (nItemCount == 0) return 1;
	if (nParam != 0)
//		m_ItemList.Init(m_nPlayerIndex);
//	else 
	{
		//Èç¹ûÒªÇó»ñµÃµÄÎïÆ·ºÅ´óÓÚÊµ¼ÊµÄÎïÆ·ÊýÁ¿£¬ÔòÍË³ö
		if (nParam >= nItemCount )
			return -1;
	}

	int nItemClass;
	int nLocal = 0;
	int nItemX = 0;
	int nItemY = 0;
	int nBegin = nParam;
	int nEnd = nParam + DBLOADPERTIME_ITEM;
	int nMagicParam[16];
	if ( nEnd > nItemCount)
		nEnd = nItemCount;

	nParam = nEnd;		
		
	for (int i = nBegin ; i < nEnd; i ++)
	{
		
		KItem NewItem;	
		//lÊy d÷ liÖu database **************************************************************
		nItemClass	= pItemData->iequipclasscode;
		NewItem.m_CommonAttrib.nDetailType		= pItemData->idetailtype;
		NewItem.m_CommonAttrib.nParticularType	= pItemData->iparticulartype;
		NewItem.m_CommonAttrib.nLevel			= pItemData->ilevel;
		NewItem.m_CommonAttrib.nSeries			= pItemData->iseries;
		NewItem.SetExType(pItemData->iextype);
		if(pItemData->iextype == extype_gold)
			nItemClass = item_gengold;
		else if(pItemData->iextype == extype_platina)
			nItemClass = item_genplatina;
		else if(pItemData->iextype == extype_purple)
			nItemClass = item_genpurple;
		nItemX			= pItemData->ix;
		nItemY			= pItemData->iy;
		nLocal			= pItemData->ilocal;
		
		ZeroMemory(&nMagicParam, sizeof(nMagicParam));
		int nDur = 0, nMagic = 0, nMagicEx = 0, nGenParam = 0, nCount = 0;
		// cã d÷ liÖu thªm vµo sau sizeof(TDBItemData)
		if (pItemData->extsize)
		{
			int nExtCount = pItemData->extsize/(1+sizeof(int));
			iExtData* pExtData = (iExtData*)((BYTE*)pItemData + sizeof(TDBItemData));
			for(int iext = 0; iext < nExtCount; ++iext)
			{
				if(pExtData->type == iextp_dur)
				{
					nDur = pExtData->nValue;
				}
				else if(pExtData->type == iextp_magic)
				{
					nMagicParam[nMagic++] = pExtData->nValue;
					pExtData++;
					nMagicParam[nMagic++] = pExtData->nValue;
					++iext;
				}
				else if(pExtData->type == iextp_genpr)
				{
					nGenParam = pExtData->nValue;
				}
				else if(pExtData->type == iextp_exmagic)
				{
					nMagicParam[12+nMagicEx++] = pExtData->nValue;
					pExtData++;
					nMagicParam[12+nMagicEx++] = pExtData->nValue;
					++iext;
				}
				else if(pExtData->type == iextp_count)
				{
					nCount = pExtData->nValue;
				}
				pExtData++;
			}
		}
		
		BOOL bGetEquiptResult = FALSE;
		switch(nItemClass)
		{
		case item_equip :			// trang bÞ
			bGetEquiptResult = ItemGen.Gen_ExistEquipment(
				NewItem.m_CommonAttrib.nDetailType, 
				NewItem.m_CommonAttrib.nParticularType,
				NewItem.m_CommonAttrib.nSeries,
				NewItem.m_CommonAttrib.nLevel, 
				nMagicParam,
				&NewItem);
			break;
		case item_medicine:			// thuèc men
				bGetEquiptResult = ItemGen.Gen_Medicine(
					NewItem.m_CommonAttrib.nDetailType,
					NewItem.m_CommonAttrib.nLevel,
					&NewItem);
			break;
		case item_task:				// nhiÖm vô
			{
				bGetEquiptResult = ItemGen.Gen_Quest(NewItem.m_CommonAttrib.nDetailType, &NewItem, nCount);
			}
			break;
		case item_townportal:	//thæ ®Þa phï
			{
				bGetEquiptResult = ItemGen.Gen_TownPortal(&NewItem);
			}
			break;
		case item_gengold:
			{
				bGetEquiptResult = ItemGen.Gen_GoldEquip(0, NewItem.m_CommonAttrib.nDetailType, &NewItem, nMagicParam);
			}
			break;
		case item_genpurple:
			{
				bGetEquiptResult = ItemGen.Gen_PurpleEquip(NewItem.m_CommonAttrib.nDetailType, 
				NewItem.m_CommonAttrib.nParticularType,
				NewItem.m_CommonAttrib.nSeries,
				NewItem.m_CommonAttrib.nLevel, 
				nMagicParam,
				nGenParam,
				&NewItem);
			}
			break;
		case item_mascript:
			{
				bGetEquiptResult = ItemGen.Gen_MAScript(NewItem.m_CommonAttrib.nDetailType,
				NewItem.m_CommonAttrib.nParticularType,
				NewItem.m_CommonAttrib.nLevel,
				NewItem.m_CommonAttrib.nSeries,
				nCount, nMagicParam, &NewItem);
			}
			break;
		}
		if(!bGetEquiptResult)
		{
			pItemData = (TDBItemData*)((BYTE*)pItemData + sizeof(TDBItemData) + pItemData->extsize);
			continue;
		}
		if (nDur != 0)
			NewItem.SetDurability(nDur);
		
		pItemData = (TDBItemData*)((BYTE*)pItemData + sizeof(TDBItemData) + pItemData->extsize);
		
		int nIndex = ItemSet.Add(&NewItem);
		
		if (nIndex <= 0) 
		{
			KASSERT(0);
			continue;
		}
		m_ItemList.Add(nIndex, nLocal, nItemX, nItemY);
		
	}

	pItemBuffer	= (BYTE *)pItemData;

	if (nParam >= nItemCount)
		return 1;
	else 
		return 0;
}


int	KPlayer::LoadPlayerFightSkillList(BYTE * pRoleBuffer, BYTE * &pFightBuffer, unsigned int& nParam)
{
	KASSERT(pRoleBuffer);
	int nSkillCount = 0;
	char szSkillId[100];
	char szSkillLevel[100];
	nSkillCount = ((TRoleData*)(pRoleBuffer))->nFightSkillCount;
	
	if (nSkillCount == 0)	
		return 1;
	
	/*if (nParam >= nSkillCount )	
		return -1;
	
	int nBegin	= nParam;
	int nEnd	= nBegin + DBLOADPERTIME_SKILL;
	if (nEnd > nSkillCount) 	nEnd = nSkillCount;
	nParam = nEnd;*/
	TDBSkillData * pSkillData = NULL;
	//if (nBegin == 0)
		pSkillData = (TDBSkillData *)((BYTE*)pRoleBuffer + ((TRoleData*)pRoleBuffer)->dwFSkillOffset);
	//else
	//	pSkillData = (TDBSkillData*)pFightBuffer;
	for (int i = 0 ; i < nSkillCount; i ++, pSkillData ++ )
	{
		int nSkillId = 0;
		int nSkillLevel = 0;
		nSkillId = pSkillData->m_nSkillId;
		nSkillLevel = pSkillData->m_nSkillLevel;
		Npc[m_nIndex].m_SkillList.Add(nSkillId, nSkillLevel);
	}
	pFightBuffer = (BYTE*) pSkillData;

	/*if (nParam >= nSkillCount) return 1;
	else 
		return 0;*/
	return 1;
}

int	KPlayer::LoadPlayerLifeSkilllList(BYTE * pRoleBuffer, BYTE * &pFriendBuffer, unsigned int& nParam)
{
	KASSERT(pRoleBuffer);
	int nSkillCount = 0;
	char szSkillId[100];
	char szSkillLevel[100];
	nSkillCount = ((TRoleData*)(pRoleBuffer))->nLiveSkillCount;
	
	if (nSkillCount == 0)
		return 1;

	if (nParam >= nSkillCount )		return -1;
	int nBegin	= nParam;
	int nEnd	= nBegin + DBLOADPERTIME_SKILL;
	if (nEnd > nSkillCount) 	nEnd = nSkillCount;
	nParam = nEnd;

	TDBSkillData * pSkillData = (TDBSkillData*)pFriendBuffer;
	for (int i = nBegin ; i < nEnd; i ++, pSkillData ++ )
	{
		int nSkillId = 0;
		int nSkillLevel = 0;
		nSkillId = pSkillData->m_nSkillId;
		nSkillLevel = pSkillData->m_nSkillLevel;
		Npc[m_nIndex].m_SkillList.Add(nSkillId, nSkillLevel);
	}
	pFriendBuffer = (BYTE*) pSkillData;

	if (nParam >= nSkillCount) return 1;
	else 
		return 0;

}

int	KPlayer::LoadPlayerTaskList(BYTE * pRoleBuffer, BYTE * &pTaskBuffer, unsigned int& nParam)
{
	KASSERT(pRoleBuffer);
	while(m_PlayerStationList.GetHead())
	{
		KIndexNode * pNode = (KIndexNode * ) m_PlayerStationList.GetHead();
		m_PlayerStationList.RemoveHead();
		delete pNode;
	}
	while(m_PlayerWayPointList.GetHead())
	{
		KIndexNode * pNode = (KIndexNode * ) m_PlayerWayPointList.GetHead();
		m_PlayerWayPointList.RemoveHead();
		delete pNode;
	}

	if(nParam == 0)	m_cTask.Release();

	int nTaskCount = 0;
	int nTaskId = 0;
	int nTaskDegee = 0;
	char szTaskIDKey[100];
	char szTaskValueKey[100];
	nTaskCount = ((TRoleData* )pRoleBuffer)->nTaskCount;

	if (nTaskCount == 0) return 1;
	if (nParam >= nTaskCount ) return -1;
	int nBegin	= nParam;
	int nEnd	= nBegin + DBLOADPERTIME_TASK;
	if (nEnd > nTaskCount) nEnd = nTaskCount;
	nParam = nEnd;
	TDBTaskData * pTaskData = (TDBTaskData*) pTaskBuffer;
	for (int i = nBegin; i < nEnd; i ++ , pTaskData ++)
	{
		nTaskId = pTaskData->m_nTaskId;
		nTaskDegee = pTaskData->m_nTaskValue;
		if (nTaskId >= MAX_TASK) 
		{
			KASSERT(0);
			continue;//ÈÎÎñID³¬¹ýÉÏÏÞÁË£¡
		}
		m_cTask.SetSaveVal(nTaskId, nTaskDegee);
	}
	pTaskBuffer = (BYTE*) pTaskData;
	if (nParam >= nTaskCount) return 1;
	else 
	return 0;
}

int	KPlayer::SavePlayerBaseInfo(BYTE * pRoleBuffer)
{
	_ASSERT(pRoleBuffer);
	if (m_nIndex <= 0) return -1;

	KNpc * pNpc = &Npc[m_nIndex];
	TRoleData * pRoleData = (TRoleData*)pRoleBuffer;
	//Íæ¼ÒÐÅÏ¢
	memset(pRoleData, 0, sizeof(TRoleData));
	strcpy(pRoleData->BaseInfo.szName, m_PlayerName);
	if (m_AccoutName[0])
		strcpy(pRoleData->BaseInfo.caccname, m_AccoutName);
	pRoleData->BaseInfo.nForbiddenFlag = m_nForbiddenFlag;
	pRoleData->BaseInfo.ileftprop = m_nAttributePoint;
	pRoleData->BaseInfo.ileftfight = m_nSkillPoint;
	pRoleData->BaseInfo.ipower = m_nStrength;
	pRoleData->BaseInfo.iagility = m_nDexterity;
	pRoleData->BaseInfo.iouter = m_nVitality;
	pRoleData->BaseInfo.iinside	=  m_nEngergy;
	pRoleData->BaseInfo.iluck = m_nLucky;
	pRoleData->BaseInfo.dwTongID = m_cTong.GetTongNameID();

	//Íæ¼ÒµÄÏÔÊ¾ÐÅÏ¢	-- remark by spe because client ui display changed 2003/07/21
//	pRoleData->BaseInfo.ihelmres = pNpc->m_HelmType;
//	pRoleData->BaseInfo.iarmorres = pNpc->m_ArmorType;
//	pRoleData->BaseInfo.iweaponres = pNpc->m_WeaponType;
	
	//Íæ¼ÒµÈ¼¶ÐÅÏ¢
	pRoleData->BaseInfo.fightexp = m_nExp;
	pRoleData->BaseInfo.ifightlevel = pNpc->m_Level;
	
	pRoleData->BaseInfo.ileadlevel = m_dwLeadLevel;
	pRoleData->BaseInfo.ileadexp =	m_dwLeadExp;

	//ÃÅÅÉÐÅÏ¢
	pRoleData->BaseInfo.nSect =		m_cFaction.m_nCurFaction;
	pRoleData->BaseInfo.nFirstSect = 	m_cFaction.m_nFirstAddFaction;
	pRoleData->BaseInfo.ijoincount = m_cFaction.m_nAddTimes;
	pRoleData->BaseInfo.isectrole = pNpc->m_btRankId;
	pRoleData->BaseInfo.nWorldStat	= m_nWorldStat;
	pRoleData->BaseInfo.nSectStat	= m_nSectStat;
	
	//ÏÖ½ðºÍÖüÎïÏäÖÐµÄÇ®
	int nCashMoney = 0;
	int nSaveMoney = 0;
	
	nCashMoney  = m_ItemList.GetMoney(room_equipment);
	nSaveMoney	= m_ItemList.GetMoney(room_repository);
	pRoleData->BaseInfo.imoney			= nCashMoney;
	pRoleData->BaseInfo.isavemoney		= nSaveMoney;
	pRoleData->BaseInfo.ifiveprop		= pNpc->m_Series;
	pRoleData->BaseInfo.iteam			= pNpc->m_Camp;
	pRoleData->BaseInfo.bSex			= pNpc->m_nSex;
	pRoleData->BaseInfo.imaxlife		= pNpc->m_LifeMax;
	pRoleData->BaseInfo.imaxstamina		= pNpc->m_StaminaMax;
	pRoleData->BaseInfo.imaxinner		= pNpc->m_ManaMax;
	pRoleData->BaseInfo.icurlife		= pNpc->m_CurrentLife;
	pRoleData->BaseInfo.icurinner		= pNpc->m_CurrentMana;
	pRoleData->BaseInfo.icurstamina		= pNpc->m_CurrentStamina;

	//µÇÈëµã
	pRoleData->BaseInfo.irevivalid = 	m_sLoginRevivalPos.m_nSubWorldID;
	pRoleData->BaseInfo.irevivalx = 	m_sLoginRevivalPos.m_ReviveID;
	pRoleData->BaseInfo.irevivaly = 	0;

	if (m_bExchangeServer)	// ×¼±¸¿ç·þÎñÆ÷£º°Ñ´æÅÌµãÉèÎªÄ¿±êµã
	{
		pRoleData->BaseInfo.cUseRevive = 0;
		pRoleData->BaseInfo.ientergameid = m_sExchangePos.m_dwMapID;
		pRoleData->BaseInfo.ientergamex = m_sExchangePos.m_nX;
		pRoleData->BaseInfo.ientergamey = m_sExchangePos.m_nY;
		pRoleData->BaseInfo.cFightMode = (BYTE)pNpc->m_FightMode;
	}
	else if (pNpc->m_SubWorldIndex >= 0 && pNpc->m_RegionIndex >= 0 && pNpc->m_Doing != do_death && pNpc->m_Doing != do_revive)
	{
		pRoleData->BaseInfo.cUseRevive = m_bUseReviveIdWhenLogin;
		pRoleData->BaseInfo.ientergameid = SubWorld[pNpc->m_SubWorldIndex].m_SubWorldID;
		pNpc->GetMpsPos(&pRoleData->BaseInfo.ientergamex, &pRoleData->BaseInfo.ientergamey);
		pRoleData->BaseInfo.cFightMode = (BYTE)pNpc->m_FightMode;
	}
	else
	{
		pRoleData->BaseInfo.cUseRevive = 1;
		pRoleData->BaseInfo.cFightMode = 0;
		if (pNpc->m_Doing == do_death || pNpc->m_Doing == do_revive)
		{
			pRoleData->BaseInfo.icurlife = pNpc->m_LifeMax;
			pRoleData->BaseInfo.icurinner = pNpc->m_ManaMax;
			pRoleData->BaseInfo.icurstamina = pNpc->m_StaminaMax;
		}
	}

	//PKÏà¹Ø
	pRoleData->BaseInfo.cPkStatus = (BYTE)m_cPK.GetNormalPKState();
	pRoleData->BaseInfo.ipkvalue = m_cPK.GetPKValue();
	pRoleData->dwFSkillOffset = (BYTE * )pRoleData->pBuffer - (BYTE *)pRoleBuffer;
	return 1;
	
}

int	KPlayer::SavePlayerItemList(BYTE * pRoleBuffer)
{
	_ASSERT(pRoleBuffer);

	TRoleData * pRoleData = (TRoleData*) pRoleBuffer;

	TDBItemData * pItemData = (TDBItemData*) ((BYTE*)pRoleData +  pRoleData->dwItemOffset);

	int	nItemCount = 0;

	int nIdx = 0;
	while(1)
	{
		nIdx = m_ItemList.m_UseIdx.GetNext(nIdx);
		if (nIdx == 0 )
			break;
		pItemData->extsize = 0;
		iExtData* pExtData = (iExtData*)((BYTE*)pItemData + sizeof(TDBItemData));
		int nItemIndex = m_ItemList.m_Items[nIdx].nIdx;
		pItemData->iextype = Item[nItemIndex].GetExType();
		pItemData->iequipclasscode =  Item[nItemIndex].m_CommonAttrib.nItemGenre;
		if(pItemData->iextype == extype_gold || pItemData->iextype == extype_platina)
			pItemData->idetailtype =  Item[nItemIndex].GetGenParam();
		else if(pItemData->iextype == extype_purple)
		{
			pItemData->idetailtype =  Item[nItemIndex].m_CommonAttrib.nDetailType;
			pExtData->type = iextp_genpr;
			pExtData->nValue = Item[nItemIndex].GetGenParam();
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
		}
		else
		{
			pItemData->idetailtype =  Item[nItemIndex].m_CommonAttrib.nDetailType;
			if((Item[nItemIndex].m_CommonAttrib.nItemGenre == item_task ||
			Item[nItemIndex].m_CommonAttrib.nItemGenre == item_mascript) &&
			Item[nItemIndex].m_CommonAttrib.nMaxStack > 0)
			{
				pExtData->type = iextp_count;
				pExtData->nValue = Item[nItemIndex].m_CommonAttrib.nStack;
				pItemData->extsize += 1 + sizeof(int);
				pExtData++;
			}
		}
		pItemData->iparticulartype =  Item[nItemIndex].m_CommonAttrib.nParticularType;
		pItemData->ilevel =  Item[nItemIndex].m_CommonAttrib.nLevel;
		pItemData->iseries =  Item[nItemIndex].m_CommonAttrib.nSeries;
		pItemData->ilocal =  m_ItemList.m_Items[nIdx].nPlace;
		pItemData->ix =  m_ItemList.m_Items[nIdx].nX;
		pItemData->iy =  m_ItemList.m_Items[nIdx].nY;
		pItemData->iequipversion = ITEM_VERSION;
		int nDur = Item[nItemIndex].GetDurability();
		if(nDur != 0)
		{
			pExtData->type = iextp_dur;
			pExtData->nValue = nDur;
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
		}
		int i;
		for(i=0; i<6; ++i)
		{
			if(!Item[nItemIndex].m_aryMagicAttrib[i].nAttribType)
				break;
			pExtData->type = iextp_magic;
			pExtData->nValue = Item[nItemIndex].m_aryMagicAttrib[i].nAttribType;
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
			pExtData->type = iextp_magic;
			pExtData->nValue = Item[nItemIndex].m_aryMagicAttrib[i].nValue[0];
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
		}
		for(i=0; i<2; ++i)
		{
			if(!Item[nItemIndex].m_aryMagicAttribEx[i].nAttribType)
				break;
			pExtData->type = iextp_exmagic;
			pExtData->nValue = Item[nItemIndex].m_aryMagicAttribEx[i].nAttribType;
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
			pExtData->type = iextp_exmagic;
			pExtData->nValue = Item[nItemIndex].m_aryMagicAttribEx[i].nValue[0];
			pItemData->extsize += 1 + sizeof(int);
			pExtData++;
		}
		pItemData = (TDBItemData*)((BYTE*)pItemData + sizeof(TDBItemData) + pItemData->extsize);
		nItemCount ++;
	}
	
	pRoleData->nItemCount = nItemCount;
	
	pRoleData->dwFriendOffset = (BYTE *)pItemData - (BYTE * )pRoleData;
	pRoleData->dwDataLen = (BYTE*)pItemData - (BYTE*)pRoleBuffer;
	return 1;
}

int	KPlayer::SavePlayerFightSkillList(BYTE * pRoleBuffer)
{
	_ASSERT(pRoleBuffer);
	if (m_nIndex <= 0) 
		return FALSE;
	TRoleData * pRoleData = (TRoleData *)pRoleBuffer;
	TDBSkillData * pSkillData = (TDBSkillData *) (pRoleBuffer + pRoleData->dwFSkillOffset);
	int nCount = Npc[m_nIndex].m_SkillList.UpdateDBSkillList((BYTE *)pSkillData);
	if (nCount > 0)
	{
		pRoleData->nFightSkillCount = nCount;
		pRoleData->dwLSkillOffset = (BYTE*)pSkillData - pRoleBuffer + sizeof(TDBSkillData) * nCount;
	}
	else
	{
		pRoleData->nFightSkillCount  = 0;
		pRoleData->dwLSkillOffset = (BYTE*)pSkillData - pRoleBuffer;
	}
	return 1;
}

int	KPlayer::SavePlayerLifeSkilllList(BYTE * pRoleBuffer)
{
	_ASSERT(pRoleBuffer);
	TRoleData * pRoleData = (TRoleData *)pRoleBuffer;
	TDBSkillData * pSkillData = (TDBSkillData *) (pRoleBuffer + pRoleData->dwLSkillOffset);
	pRoleData->dwTaskOffset = (BYTE*)pSkillData - pRoleBuffer;
	return 1;
}

int	KPlayer::SavePlayerTaskList(BYTE * pRoleBuffer)
{
	_ASSERT(pRoleBuffer);
	TRoleData * pRoleData = (TRoleData *) pRoleBuffer;
	int nTaskCount = 0;
	int nTaskId = 0;
	int nTaskDegee = 0;
	char szTaskIDKey[100];
	char szTaskValueKey[100];

	KIndexNode * pNode = (KIndexNode*)m_PlayerStationList.GetHead();
	int n = 0;
	memset(&m_cTask.nSave[TASKVALUE_SAVESTATION_BEGIN], 0, (TASKVALUE_MAXSTATION_COUNT / 2) * sizeof(int));
	memset(&m_cTask.nSave[TASKVALUE_SAVEWAYPOINT_BEGIN], 0, TASKVALUE_MAXWAYPOINT_COUNT * sizeof(int));

	while(pNode)
	{
		if (n >= TASKVALUE_MAXSTATION_COUNT) break;
		DWORD dwValue = m_cTask.nSave[TASKVALUE_SAVESTATION_BEGIN + n / 2];
		DWORD OrValue = pNode->m_nIndex ;
		OrValue = OrValue << ((n % 2) * 16);
		dwValue = dwValue | OrValue;
		m_cTask.nSave[TASKVALUE_SAVESTATION_BEGIN + n / 2] = dwValue;
		n++;
		pNode = (KIndexNode*)pNode->GetNext();
	}
	
	n = 0;
	pNode = (KIndexNode*) m_PlayerWayPointList.GetHead();
	while(pNode)
	{
		if (n >= TASKVALUE_MAXWAYPOINT_COUNT) break;
		if (pNode->m_nIndex == 0) continue;
		m_cTask.nSave[TASKVALUE_SAVEWAYPOINT_BEGIN + n] = pNode->m_nIndex;
		n++;
		pNode = (KIndexNode*)pNode->GetNext();
	}
	
	
	g_TimerTask.SaveTask(this);


	TDBTaskData * pTaskData = (TDBTaskData *)(pRoleBuffer + pRoleData->dwTaskOffset);
	for (int i = 0; i < MAX_TASK; i ++)
	{
		if (!m_cTask.nSave[i]) continue;
		
		pTaskData->m_nTaskId = i;
		pTaskData->m_nTaskValue = m_cTask.GetSaveVal(i);
		nTaskCount ++;
		pTaskData ++;
	}
		
	pRoleData->nTaskCount = nTaskCount;
	pRoleData->dwItemOffset = (BYTE *)pTaskData - pRoleBuffer;
	return 1;
}
#endif

