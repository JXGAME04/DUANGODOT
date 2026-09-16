/*******************************************************************************
// FileName			:	ScriptFuns.cpp
// FileAuthor		:	RomanDou
// FileCreateDate	:	2002-11-19 15:58:20
// FileDescription	:	½Å±¾Ö¸Áî¼¯
// Revision Count	:	
*******************************************************************************/
#ifndef WIN32
#include <string>
#endif

#include "KWin32.h"
#include "KEngine.h"
#include "KDebug.h"
#include "KStepLuaScript.h"
#include "LuaLib.h"
#include "KScriptList.h"
#include <string.h>
#include "LuaFuns.h"
#include "KCore.h"
#include "KNpc.h"
#include "KSubWorld.h"
#include "KObjSet.h"
#include "KItemSet.h"
//#include "KNetClient.h"
#include "KScriptValueSet.h"
#include "KNpcSet.h"
#include "KPlayerSet.h"
#include "KPlayer.h"
#include "KSubWorldSet.h"
#include "KProtocolProcess.h"
#include "KBuySell.h"
#include "KTaskFuns.h"
#include "KPlayerDef.h"
#include "TaskDef.h"
#ifdef _SERVER
//#include "KNetServer.h"
//#include "../MultiServer/Heaven/interface/iServer.h"
#include "KNewProtocolProcess.h"
#include "KTongProtocol.h"
#endif
#include "KSortScript.h"
#ifndef __linux
#include "Shlwapi.h"
#include "windows.h"
#include "winbase.h"
#include <direct.h>
#else
#include "unistd.h"
#endif

#ifdef _STANDALONE
#include "KSG_StringProcess.h"
#else
#include "../../Engine/Src/KSG_StringProcess.h"
#endif
#ifdef _SERVER
#include "KNpcTemplate.h"
extern KLuaScript		*g_pNpcLevelScript;
#endif
#ifndef WIN32
typedef struct  _SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
}	SYSTEMTIME;
typedef struct  _FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
}	FILETIME;
#endif

inline const char* _ip2a(DWORD ip) { in_addr ia; ia.s_addr = ip; return inet_ntoa(ia); }
inline DWORD _a2ip(const char* cp) { return inet_addr(cp); }

KScriptList		g_StoryScriptList;
KStepLuaScript * LuaGetScript(Lua_State * L);
int	GetPlayerIndex(Lua_State * L);
extern int g_GetPriceToStation(int,int);
extern int g_GetPriceToWayPoint(int,int);
extern int g_GetPriceToDock(int ,int );

//BitValue = GetBit(Value, BitNo)
int LuaGetBit(Lua_State * L)
{
	int nBitValue = 0;
	int nIntValue = (int)Lua_ValueToNumber(L, 1);
	int nBitNumber = (int)Lua_ValueToNumber(L, 2);
	
	if (nBitNumber >= 32 || nBitNumber <= 0) 
		goto lab_getbit;
	nBitValue = (nIntValue & (1 << (nBitNumber - 1))) != 0;
lab_getbit:
	Lua_PushNumber(L, nBitValue);
	return 1;
}

//NewBit = SetBit(Value, BitNo, BitValue)
int LuaSetBit(Lua_State * L)
{
	int nIntValue = (int)Lua_ValueToNumber(L, 1);
	int nBitNumber = (int)Lua_ValueToNumber(L, 2);
	int nBitValue = (int)Lua_ValueToNumber(L,3);
	nBitValue = (nBitValue == 1);
	
	if (nBitNumber > 32 || nBitNumber <= 0) 
		goto lab_setbit;
	
	nIntValue = (nIntValue | (1 << (nBitNumber - 1)));
lab_setbit:
	Lua_PushNumber(L, nIntValue);
	return 1;
}

//ByteValue = GetByte(Value, ByteNo)
int LuaGetByte(Lua_State * L)
{
	int nByteValue = 0;
	int nIntValue = (int)Lua_ValueToNumber(L, 1);
	int nByteNumber = (int)Lua_ValueToNumber(L, 2);
	
	if (nByteNumber > 4 || nByteNumber <= 0) 
		goto lab_getByte;
	nByteValue = (nIntValue & (0xff << ((nByteNumber - 1) * 8) )) >> ((nByteNumber - 1) * 8);
	
lab_getByte:
	Lua_PushNumber(L, nByteValue);
	return 1;
}

//NewByte = SetByte(Value, ByteNo, ByteValue)
int LuaSetByte(Lua_State * L)
{
	BYTE * pByte =	NULL;
	int nIntValue = (int)Lua_ValueToNumber(L, 1);
	int nByteNumber = (int)Lua_ValueToNumber(L, 2);
	int nByteValue = (int)Lua_ValueToNumber(L,3);
	nByteValue = (nByteValue & 0xff);
	
	if (nByteNumber > 4 || nByteNumber <= 0) 
		goto lab_setByte;
	
	pByte = (BYTE*)&nIntValue;
	*(pByte + (nByteNumber -1)) = (BYTE)nByteValue;
	//nIntValue = (nIntValue | (0xff << ((nByteNumber - 1) * 8) )) ;
lab_setByte:
	Lua_PushNumber(L, nIntValue);
	return 1;
}
#ifdef _SERVER
int LuaModifyRepute(Lua_State *L)
{
	if (Lua_GetTopIndex(L) <1) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0)
		return 0;
	int nValue = (int)Lua_ValueToNumber(L,1);
	
	Player[nPlayerIndex].m_cTask.SetSaveVal(TASKVALUE_REPUTE, Player[nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_REPUTE) + nValue);	
	
	return 0;	
}

int LuaGetRepute(Lua_State *L)
{
	int nValue = 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0)
		goto lab_getrepute;
	nValue = Player[nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_REPUTE);
lab_getrepute:
	Lua_PushNumber(L, nValue);
	return 1;	
}

int AddOffer(Lua_State *L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	int nValue = (int)Lua_ValueToNumber(L,1);
	Player[nPlayerIndex].m_cTask.SetSaveVal(TASKVALUE_OFFER, Player[nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_OFFER) + nValue);	
	char szPack[16];
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_taskoffer;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
	UINT* p = (UINT*)(pCmd+1);
	*p = Player[nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_OFFER);
	g_pServer->PackDataToClient(Player[nPlayerIndex].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);

	return 0;	
}

int GetOffer(Lua_State *L)
{
	int nValue = 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		goto lab_getoffer;
	nValue = Player[nPlayerIndex].m_cTask.GetSaveVal(TASKVALUE_OFFER);
lab_getoffer:
	Lua_PushNumber(L, nValue);
	return 1;	
}
#endif

int GetSubWorldIndex(Lua_State * L)
{
	Lua_GetGlobal(L, SCRIPT_SUBWORLDINDEX);
	if (lua_isnil(L,Lua_GetTopIndex(L)))
        return -1;
	int nIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
    if (nIndex >= MAX_SUBWORLD || nIndex <= 0) 
	{
		_ASSERT(0);
		return -1;
	}
	if (SubWorld[nIndex].m_nIndex >= MAX_SUBWORLD || SubWorld[nIndex].m_nIndex < 0)
	{
		_ASSERT(0);
		return -1;
	}
    return nIndex;
}

//Idx = SubWorldID2Idx(dwID)
int LuaSubWorldIDToIndex(Lua_State * L)
{
	int nTargetSubWorld = -1;
	int nSubWorldID = 0;
	if (Lua_GetTopIndex(L) < 1)
		goto lab_subworldid2idx;
	
	nSubWorldID = (int)Lua_ValueToNumber(L, 1);
	nTargetSubWorld = g_SubWorldSet.SearchWorld(nSubWorldID);	
	
lab_subworldid2idx:
	Lua_PushNumber(L, nTargetSubWorld);
	return 1;
}


/*
Say(sMainInfo, nSelCount, sSel1, sSel2, sSel3, .....,sSeln) 
Say(nMainInfo, nSelCount, sSel1, sSel2, sSel3, .....,sSeln) 
Say(nMainInfo, nSelCount, SelTab)
Èç¹ûÊÇ¿Í»§¶ËµÄÔò²»»áÏò·þÎñÆ÷¶Ë·¢ËÍÈÎºÎ²Ù×÷

  Say(100, 3, 10, 23,43)
  Say("Ñ¡ÔñÊ²Ã´£¿", 2, "ÊÇ/yes", "·ñ/no");
  Say("Ñ¡Ê²Ã´Ñ½", 2, SelTab);
*/
//**************************************************************************************************************************************************************
//												½çÃæ½Å±¾
//**************************************************************************************************************************************************************
int LuaSelectUI(Lua_State * L)
{
	char * strMain  = NULL;
	int nMainInfo = 0;
	int nDataType = 0;
	int nOptionNum = 0;
	char * pContent = NULL;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;
	Player[nPlayerIndex].m_bWaitingPlayerFeedBack = false;
	
	int nParamNum = Lua_GetTopIndex(L);
	if (nParamNum < 2) return 0;
	
	if (Lua_IsNumber(L,2))
	{
		nOptionNum = (int)Lua_ValueToNumber(L,2);
	}
	else 
	{
		_ASSERT(0);
		return 0;
	}
	
	if  (Lua_IsNumber(L,1))
	{
		nMainInfo = (int)Lua_ValueToNumber(L,1);
		nDataType = 1 ;
	}
	else if (Lua_IsString(L, 1)) 	//¼ì²éÖ÷ÐÅÏ¢ÊÇ×Ö·û´®»¹ÊÇ×Ö·û´®±êÊ¶ºÅ
	{
		strMain = (char *)Lua_ValueToString(L, 1);
		nDataType = 0 ;
	}
	else
		return 0;
	
	BOOL bStringTab = FALSE;//±êÊ¶´«½øÀ´µÄÑ¡ÏîÊý¾Ý´æ·ÅÔÚÒ»¸öÊý×éÖÐ£¬»¹ÊÇÐí¶à×Ö·û´®Àï
	
	if (Lua_IsString(L,3))
		bStringTab = FALSE;
	else if (Lua_IsTable(L, 3))
	{
		bStringTab = TRUE;
	}
	else 
	{if (nOptionNum > 0)  return 0;
	}
	
	if (bStringTab == FALSE)
	{
		//»ñµÃÊµ¼Ê´«ÈëµÄÑ¡ÏîµÄ¸öÊý
		if (nOptionNum > nParamNum - 2) nOptionNum = nParamNum - 2;
	}
	
	if (nOptionNum > MAX_ANSWERNUM) nOptionNum = MAX_ANSWERNUM;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_SELECTDIALOG;
	UiInfo.m_bParam1 = nDataType;//Ö÷ÐÅÏ¢µÄÀàÐÍ£¬×Ö·û´®(0)»òÊý×Ö(1)
	UiInfo.m_bOptionNum = nOptionNum;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	//Ö÷ÐÅÏ¢Îª×Ö·û´®
	if (nDataType == 0)
	{
		if (strMain)
			sprintf(UiInfo.m_pContent, "%s", strMain);
		pContent = UiInfo.m_pContent;
	}
	else if (nDataType == 1) //Ö÷ÐÅÏ¢ÎªÊý×Ö±êÊ¶
	{
		*(int *)UiInfo.m_pContent = nMainInfo;
		pContent = UiInfo.m_pContent + sizeof(int);
		*pContent = 0;
	}
	
	if (nOptionNum > MAX_ANSWERNUM)
		nOptionNum = MAX_ANSWERNUM;
	
	Player[nPlayerIndex].m_nAvailableAnswerNum = nOptionNum;		
	
	for (int i  = 0; i < nOptionNum; i ++)
	{	
		char  pAnswer[100];
		pAnswer[0] = 0;
		
		if (bStringTab)
		{
			Lua_PushNumber(L, i + 1);
			Lua_RawGet(L, 3);
			char * pszString = (char *)Lua_ValueToString(L, Lua_GetTopIndex(L));
			if (pszString)
			{
				g_StrCpyLen(pAnswer, pszString, 100);
			}
		}
		else 
		{
			char * pszString = (char *)Lua_ValueToString(L, i + 3);
			if (pszString)
				g_StrCpyLen(pAnswer, pszString, 100);
		}
		
		char * pFunName = strstr(pAnswer, "/");
		
		if (pFunName)
		{
			g_StrCpyLen(Player[nPlayerIndex].m_szTaskAnswerFun[i], pFunName + 1, sizeof(Player[nPlayerIndex].m_szTaskAnswerFun[0]));
			*pFunName = 0;
			sprintf(pContent, "%s|%s", pContent, pAnswer);
		}
		else 
		{
			strcpy(Player[nPlayerIndex].m_szTaskAnswerFun[i], "main");
			sprintf(pContent, "%s|%s", pContent, pAnswer);
		}
	}
	
	if (nDataType == 0)
		UiInfo.m_nBufferLen  = strlen(pContent);
	else 
		UiInfo.m_nBufferLen = strlen(pContent) + sizeof(int);
	
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
#else
	UiInfo.m_bParam2 = 1;
#endif
	
	if (nOptionNum == 0)
	{
		Player[nPlayerIndex].m_bWaitingPlayerFeedBack = false;
	}
	else
	{
		Player[nPlayerIndex].m_bWaitingPlayerFeedBack = true;
	}
	
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
	return 0;
}


//PutMsg(szMsg/MsgId)
int LuaSendMessageInfo(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_MSGINFO;
	UiInfo.m_bOptionNum = 1;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		UiInfo.m_nBufferLen = sizeof(int);
	}
	else 
	{
		
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
		UiInfo.m_bParam1 = 0;
	}
	
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
#else
	UiInfo.m_bParam2 = 1;
#endif
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
	return 0;
}

//AddGlobalNews(Newsstr)
int LuaAddGlobalNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_NORMAL;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		UiInfo.m_nBufferLen = sizeof(int);
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
		UiInfo.m_bParam1 = 0;
	}
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastGlobal(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddLocalNews(Newsstr)
int LuaAddLocalNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_NORMAL;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		UiInfo.m_nBufferLen = sizeof(int);
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
		UiInfo.m_bParam1 = 0;
	}
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastLocalServer(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddGlobalCountNews(strNew/newid, time)
int LuaAddGlobalCountNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_COUNTING;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	int nTime = (int)Lua_ValueToNumber(L,2);
	
	if (nTime <= 0)
		nTime = 1;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		*(int *)((char *)UiInfo.m_pContent + sizeof(int)) = nTime;
		UiInfo.m_nBufferLen = sizeof(int) * 2;
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
		*(int *)((char *)UiInfo.m_pContent + UiInfo.m_nBufferLen) = nTime;
		UiInfo.m_nBufferLen += sizeof(int);
		UiInfo.m_bParam1 = 0;
	}
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastGlobal(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddLocalCountNews(strNew/newid, time)
int LuaAddLocalCountNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_COUNTING;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	int nTime = (int)Lua_ValueToNumber(L,2);
	
	if (nTime <= 0)
		nTime = 1;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		*(int *)((char *)UiInfo.m_pContent + sizeof(int)) = nTime;
		UiInfo.m_nBufferLen = sizeof(int) * 2;
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
		*(int *)((char *)UiInfo.m_pContent + UiInfo.m_nBufferLen) = nTime;
		UiInfo.m_nBufferLen += sizeof(int);
		UiInfo.m_bParam1 = 0;
	}
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastLocalServer(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddGlobalTimeNews(strNew/newid, year,month,day,hour,mins)
int LuaAddGlobalTimeNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 6) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_TIMEEND;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		UiInfo.m_nBufferLen = sizeof(int) + sizeof(SYSTEMTIME);
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent)) + sizeof(SYSTEMTIME);
		UiInfo.m_bParam1 = 0;
	}
	
	SYSTEMTIME *pSystemTime = 	(SYSTEMTIME *)((char *)UiInfo.m_pContent + UiInfo.m_nBufferLen - sizeof(SYSTEMTIME));
	memset(pSystemTime, 0, sizeof(SYSTEMTIME));
	
	SYSTEMTIME LocalTime ;
	memset(&LocalTime, 0, sizeof(SYSTEMTIME));
	
	LocalTime.wYear = (WORD)Lua_ValueToNumber(L,2);
	LocalTime.wMonth =(WORD)Lua_ValueToNumber(L,3);
	LocalTime.wDay = (WORD)Lua_ValueToNumber(L, 4);
	LocalTime.wHour = (WORD)Lua_ValueToNumber(L,5);
	LocalTime.wMinute = (WORD)Lua_ValueToNumber(L,6);
	FILETIME ft;
	FILETIME sysft;
#ifdef WIN32
	SystemTimeToFileTime(&LocalTime, &ft);
	LocalFileTimeToFileTime(&ft, &sysft);
	FileTimeToSystemTime(&sysft, pSystemTime);
#else
	memcpy(pSystemTime, &LocalTime, sizeof(LocalTime));
#endif
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastGlobal(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddLocalTimeNews(strNew/newid, year,month,day,hour,mins)
int LuaAddLocalTimeNews(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 6) 
		return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NEWSINFO;
	UiInfo.m_bOptionNum = NEWSMESSAGE_TIMEEND;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nMsgId = (int)Lua_ValueToNumber(L,1);
		*((int *)(UiInfo.m_pContent)) = nMsgId;
		UiInfo.m_bParam1 = 1;
		UiInfo.m_nBufferLen = sizeof(int) + sizeof(SYSTEMTIME);
	}
	else 
	{
		g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), 64);
		UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent)) + sizeof(SYSTEMTIME);
		UiInfo.m_bParam1 = 0;
	}
	
	SYSTEMTIME *pSystemTime = 	(SYSTEMTIME *)((char *)UiInfo.m_pContent + UiInfo.m_nBufferLen - sizeof(SYSTEMTIME));
	memset(pSystemTime, 0, sizeof(SYSTEMTIME));
	
	SYSTEMTIME LocalTime ;
	memset(&LocalTime, 0, sizeof(SYSTEMTIME));
	
	LocalTime.wYear = (WORD)Lua_ValueToNumber(L,2);
	LocalTime.wMonth =(WORD)Lua_ValueToNumber(L,3);
	LocalTime.wDay = (WORD)Lua_ValueToNumber(L, 4);
	LocalTime.wHour = (WORD)Lua_ValueToNumber(L,5);
	LocalTime.wMinute = (WORD)Lua_ValueToNumber(L,6);
	FILETIME ft;
	FILETIME sysft;
#ifdef WIN32
	SystemTimeToFileTime(&LocalTime, &ft);
	LocalFileTimeToFileTime(&ft, &sysft);
	FileTimeToSystemTime(&sysft, pSystemTime);
#else
	memcpy(pSystemTime, &LocalTime, sizeof(LocalTime));
#endif
	
#ifndef _SERVER
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;

	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM + UiInfo.m_nBufferLen - 1;
	g_NewProtocolProcess.BroadcastLocalServer(&UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}

//AddNote(str/strid)
int LuaAddNote(Lua_State * L)
{
	char * strMain  = NULL;
	int nMainInfo = 0;
	int nDataType = 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0)
		return 0;
	
	int nParamNum = Lua_GetTopIndex(L);
	if (nParamNum < 1) 
		return 0;
	
	int nParam2 = 0;
	
	
	if  (Lua_IsNumber(L,1))
	{
		nMainInfo = (int)Lua_ValueToNumber(L,1);
		nDataType = 1 ;
	}
	else if (Lua_IsString(L, 1)) 	//¼ì²éÖ÷ÐÅÏ¢ÊÇ×Ö·û´®»¹ÊÇ×Ö·û´®±êÊ¶ºÅ
	{
		strMain = (char *)Lua_ValueToString(L, 1);
		nDataType = 0 ;
	}
	else
		return 0;
	
	if (nParamNum > 1)
	{
		nParam2 = (int)Lua_ValueToNumber(L, 2);
	}
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_NOTEINFO;
	UiInfo.m_bParam1 = nDataType;//Ö÷ÐÅÏ¢µÄÀàÐÍ£¬×Ö·û´®(0)»òÊý×Ö(1)
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
#else
	UiInfo.m_bParam2 = 1;
#endif
	
	UiInfo.m_bOptionNum = 0;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	//Ö÷ÐÅÏ¢Îª×Ö·û´®
	if (nDataType == 0)
	{
		if (strMain)
			sprintf(UiInfo.m_pContent, "%s", strMain);
		int nLen = strlen(strMain);
		*(int*)(UiInfo.m_pContent + nLen) = nParam2;
		UiInfo.m_nBufferLen = nLen + sizeof(int);
	}
	else if (nDataType == 1) //Ö÷ÐÅÏ¢ÎªÊý×Ö±êÊ¶
	{
		*(int *)UiInfo.m_pContent = nMainInfo;
		*(int *)(UiInfo.m_pContent + sizeof(int)) = nParam2;
		UiInfo.m_nBufferLen = sizeof(int) + sizeof(int);
	}
	
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
	return 0;
}

/*
**
**¸ñÊ½1:Talk(SentenceNum, CallBack-Fun(½áÊøºóµÄ»Øµ÷º¯Êý), sTalk1, sTalk2, sTalk3, sTalk4,...sTalkN);  
Talk(SentenceNum, CallBack-Fun(½áÊøºóµÄ»Øµ÷º¯Êý), nTalk1, nTalk2,nTalk3,nTalk4,...nTalkN);  
**¸ñÊ½2:Talk(SentenceNum, CallBack-Fun, SentenceTab);
**Àý×Ó:Talk(3,"EndTalk", "Íæ¼Ò£ºÇëÎÊÏÖÔÚ¼¸µãÖÓÁË£¿", "¹ÍÔ±£ºÏÖÔÚ5µãÖÓÁË","Ì«Ð»Ð»ÄãÁË£¡");
**
*/

int LuaTalkUI(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	Player[nPlayerIndex].m_bWaitingPlayerFeedBack = false;
	int nMainInfo = 0;
	int nDataType = 0;
	int nOptionNum = 0;
	char * pContent = NULL;
	
	int nParamNum = Lua_GetTopIndex(L);
	if (nParamNum < 3) 
		return 0;
	
	if (Lua_IsNumber(L,1))
	{
		nOptionNum = (int)Lua_ValueToNumber(L,1);
	}
	else 
	{
		_ASSERT(0);
		return 0;
	}
	
	const char * pCallBackFun = Lua_ValueToString(L,2);
	
	//¼ì²éÖ÷ÐÅÏ¢ÊÇ×Ö·û´®»¹ÊÇ×Ö·û´®±êÊ¶ºÅ
	
	if  (Lua_IsNumber(L,3))
	{
		nDataType = 1 ;
	}
	else if (Lua_IsString(L, 3)) 
	{
		nDataType = 0 ;
	}
	else
		return 0;
	
	
	//»ñµÃÊµ¼Ê´«ÈëµÄÑ¡ÏîµÄ¸öÊý
	if (nOptionNum > nParamNum - 2) 
		nOptionNum = nParamNum - 2;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_TALKDIALOG;
	UiInfo.m_bParam1 = nDataType;//Ö÷ÐÅÏ¢µÄÀàÐÍ£¬×Ö·û´®(0)»òÊý×Ö(1)
	UiInfo.m_bOptionNum = nOptionNum;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	pContent = UiInfo.m_pContent;
	pContent[0] = 0;
	size_t nContentLen = 0;
	for (int i  = 0; i < nOptionNum; i ++)
	{	
		const char * pString = NULL;
		if (!nDataType)//StringInfo
		{
			pString = Lua_ValueToString(L, i + 3);
			if (nContentLen  + strlen(pString) >= MAX_SCIRPTACTION_BUFFERNUM)
			{
				nOptionNum = i;
				UiInfo.m_bOptionNum = nOptionNum;
				break;
			}
			nContentLen += strlen(pString);
			sprintf(pContent, "%s%s|", pContent, pString);
		}
		else
		{
			int j = (int)Lua_ValueToNumber(L, i + 3);
			sprintf(pContent, "%s%d|", pContent, j);
		}
	}
	UiInfo.m_nBufferLen  = strlen(pContent);
	
	if (!pCallBackFun || strlen(pCallBackFun) <= 0)
	{
		UiInfo.m_nParam = 0;
		Player[nPlayerIndex].m_nAvailableAnswerNum = 0;
		Player[nPlayerIndex].m_bWaitingPlayerFeedBack = false;
	}
	else
	{
		UiInfo.m_nParam = 1;
		Player[nPlayerIndex].m_nAvailableAnswerNum = 1;
		g_StrCpyLen(Player[nPlayerIndex].m_szTaskAnswerFun[0], pCallBackFun, sizeof(Player[nPlayerIndex].m_szTaskAnswerFun[0]));
		Player[nPlayerIndex].m_bWaitingPlayerFeedBack = true;
	}
	
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
#else
	UiInfo.m_bParam2 = 1;
#endif
	
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
	return 0;
	
}

int LuaIncludeFile(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0) return 0;
	
	if (Lua_IsString(L,1))
	{
		const char * pFileName = lua_tostring(L,1);	
		char lszCurrentDirectory[MAX_PATH];
		int nLen = 0;
		if (pFileName[0] != '\\' && pFileName[0] != '/')
		{
			getcwd(lszCurrentDirectory, MAX_PATH);
			nLen = strlen(lszCurrentDirectory);
			if (lszCurrentDirectory[nLen - 1] == '\\' || lszCurrentDirectory[nLen - 1] == '/')
				lszCurrentDirectory[nLen - 1] = 0;
#ifdef WIN32
			g_StrCat(lszCurrentDirectory, "\\");
			g_StrCat(lszCurrentDirectory, (char*)pFileName);
#else
			g_StrCat(lszCurrentDirectory, "/");
			g_StrCat(lszCurrentDirectory, (char*)pFileName);
			for (int i = 0; lszCurrentDirectory[i]; i++)
			{
				if (lszCurrentDirectory[i] == '\\')
					lszCurrentDirectory[i] = '/';
			}
#endif
		}
		else
		{
			g_GetRootPath(lszCurrentDirectory);
			nLen = strlen(lszCurrentDirectory);
			if (lszCurrentDirectory[nLen - 1] == '\\' || lszCurrentDirectory[nLen - 1] == '/')
				lszCurrentDirectory[nLen - 1] = 0;
#ifdef WIN32
			g_StrCat(lszCurrentDirectory, "\\");
			g_StrCat(lszCurrentDirectory, (char*)pFileName + 1);
#else
			g_StrCat(lszCurrentDirectory, "/");
			g_StrCat(lszCurrentDirectory, (char*)pFileName + 1);
			for (int i = 0; lszCurrentDirectory[i]; i++)
			{
				if (lszCurrentDirectory[i] == '\\')
					lszCurrentDirectory[i] = '/';
			}
#endif
		}
		strlwr(lszCurrentDirectory + nLen);
		lua_dofile(L, lszCurrentDirectory);
		return 0;
	}
	else
		return 0;
}

//**************************************************************************************************************************************************************
//												ÈÎÎñ½Å±¾
//**************************************************************************************************************************************************************

int LuaGetTaskValue(Lua_State * L)
{
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0) 
	{
		int nValue = (int)Player[nPlayerIndex].m_cTask.GetSaveVal((int)Lua_ValueToNumber(L,1));		
		Lua_PushNumber(L, nValue);
	}
	else
		Lua_PushNil(L);
	
	return 1;
}

int LuaSetTaskValue(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	int nValueIndex = (int)Lua_ValueToNumber(L, 1);
	int nValue = (int)Lua_ValueToNumber(L, 2);
	
	if (nPlayerIndex <= 0) return 0;
	Player[nPlayerIndex].m_cTask.SetSaveVal(nValueIndex, nValue);
	return 0;
}


#ifndef _SERVER
#define MAX_TEMPVALUENUM_INCLIENT 500
int g_TempValue[MAX_TEMPVALUENUM_INCLIENT];
#endif

int LuaGetTempTaskValue(Lua_State * L)
{
	int nTempIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
	
#ifdef _SERVER
	if(nTempIndex >= MAX_TEMP_TASK)
	{
		Lua_PushNil(L) ;
		return 1;
	}
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) 
	{
		Lua_PushNil(L);
		return 1;
	}
	
	int nValue = Player[nPlayerIndex].m_cTask.GetClearVal(nTempIndex);
	Lua_PushNumber(L, nValue);
#else
	
	if (nTempIndex >= 0 && nTempIndex < MAX_TEMPVALUENUM_INCLIENT)
		Lua_PushNumber(L, g_TempValue[nTempIndex]);
	else 
		Lua_PushNil(L);
#endif
	return 1;
}

int LuaSetTempTaskValue(Lua_State * L)
{
	int nTempIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L) - 1);
	int nValue = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
#ifdef _SERVER	
	Lua_GetGlobal(L, SCRIPT_PLAYERINDEX);
	int nPlayerIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
	if (nPlayerIndex <= 0) return 0;
	Player[nPlayerIndex].m_cTask.SetClearVal(nTempIndex, nValue);
#else
	g_TempValue[nTempIndex] = nValue;
#endif
	return 0;
}

#ifdef _SERVER
//---------------------------------½»Ò×¡¢ÂòÂô¡¢´ò¿ª´¢²ØÏä-----------------------
//Sale(id)
//------------------------------------------------------------------------------
int LuaSale(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nShopId = (int)Lua_ValueToNumber(L,1);
		//----ÔÚÒÔÏÂ¼ÓÈëÂòÂôµÄÊµ¼Ê´úÂë!
		BuySell.OpenSale(nPlayerIndex, nShopId - 1);
	}
	return 0;
}

int LuaTrade(Lua_State * L)
{
	//Question Î´Ð´
	return 0;
}

int LuaOpenBox(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	BYTE	NetCommand = (BYTE)s2c_openstorebox;
	g_pServer->PackDataToClient(Player[nPlayerIndex].m_nNetConnectIdx, &NetCommand, sizeof(BYTE));
	return 0;
}

//---------------------------------Ê±¼äÈÎÎñ-------------------------------------
//SetTimer(Time, TimerTaskId)
int LuaSetTimer(Lua_State  * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2 ) return 0;
	int nPlayerIndex  = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	Player[nPlayerIndex].SetTimer((DWORD) (int)Lua_ValueToNumber(L, 1), (int)Lua_ValueToNumber(L,2));
	return 0;
}

int LuaStopTimer(Lua_State * L)
{
	int nPlayerIndex  = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	Player[nPlayerIndex].CloseTimer();
	return 0;
}

int LuaGetCurTimerId(Lua_State * L)
{
	int nPlayerIndex  = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	int nTimerId = Player[nPlayerIndex].m_TimerTask.GetTaskId();
	Lua_PushNumber(L, nTimerId);
	return 1;
}

int LuaGetRestTime(Lua_State * L)
{
	int nPlayerIndex  = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
	{
		Lua_PushNil(L);
		return 1;
	}
	int nRestTime = Player[nPlayerIndex].m_TimerTask.GetRestTime();//m_dwTimeTaskTime - g_SubWorldSet.GetGameTime();
	
	if (nRestTime > 0)
		Lua_PushNumber(L, nRestTime);
	else
		Lua_PushNumber(L, 0);
	
	return 1;
}

int LuaGetMissionRestTime(Lua_State * L)
{
	int RestTime = 0;
	if (Lua_GetTopIndex(L) >= 2)
	{
		int nSubWorldIndex = GetSubWorldIndex(L);
		if (nSubWorldIndex >= 0) 
		{
			int nMissionId = (int)Lua_ValueToNumber(L, 1);
			int nTimerId = (int)Lua_ValueToNumber(L, 2);
			
			if (nMissionId < 0 || nTimerId < 0 )
				goto lab_getmissionresttime;
			
			KMission Mission;
			Mission.SetMissionId(nMissionId);
			KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
			if (pMission)
			{
				RestTime = (int)pMission->GetTimerRestTimer(nTimerId);
			}
		}
	}
	
lab_getmissionresttime:
	Lua_PushNumber(L, RestTime);
	return 1;
}


//**************************************************************************************************************************************************************
//												×é¶Ó½Å±¾
//**************************************************************************************************************************************************************
int LuaIsLeader(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0 ) 
	{
		if (Player[nPlayerIndex].m_cTeam.m_nFlag && Player[nPlayerIndex].m_cTeam.m_nFigure == TEAM_CAPTAIN)
			Lua_PushNumber(L,1);
		else 
			Lua_PushNumber(L,0);
		
	}
	else 
		Lua_PushNumber(L, 0);
	return 1;
}

int LuaGetTeamId(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		if (Player[nPlayerIndex].m_cTeam.m_nFlag)
			Lua_PushNumber(L, Player[nPlayerIndex].m_cTeam.m_nID);
		else Lua_PushNil(L);
	}
	else 
		Lua_PushNil(L);
	return 1;
}

int LuaGetTeamSize(Lua_State * L)
{
	int nTeamSize = 0;
	int nTeamId = -1;
	if (Lua_GetTopIndex(L) >= 1)
	{
		nTeamId = Lua_ValueToNumber(L, 1);
	}
	else
	{
		int nPlayerIndex = GetPlayerIndex(L);
		if (nPlayerIndex > 0)
		{
			if (Player[nPlayerIndex].m_cTeam.m_nFlag)
				nTeamId = Player[nPlayerIndex].m_cTeam.m_nID;
			else 
				nTeamId = -1;
		}
	}
	
	if (nTeamId < 0)
		nTeamSize = 0;
	else
		nTeamSize = g_Team[nTeamId].m_nMemNum + 1;
	Lua_PushNumber(L, nTeamSize);
	return 1;
}

int LuaLeaveTeam(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		if (Player[nPlayerIndex].m_cTeam.m_nFlag)
		{
			PLAYER_APPLY_LEAVE_TEAM	sLeaveTeam;
			sLeaveTeam.ProtocolType = c2s_teamapplyleave;
			Player[nPlayerIndex].LeaveTeam((BYTE*)&sLeaveTeam);
		}
	}
	return 0;
}

int LuaSetCreateTeamOption(Lua_State * L)
{
	int nState = (int)Lua_ValueToNumber(L, 1);
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		if (nState)
			Player[nPlayerIndex].m_cTeam.SetCanTeamFlag(nPlayerIndex, TRUE);
		else
			Player[nPlayerIndex].m_cTeam.SetCanTeamFlag(nPlayerIndex, FALSE);
	}
	return 0;
}
//**************************************************************************************************************************************************************
//												ÁÄÌìÏûÏ¢½Å±¾
//**************************************************************************************************************************************************************

int	LuaMsgToPlayer(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0) return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0) 
	{
		const char *  szMsg = Lua_ValueToString(L,1);
		if (szMsg)
			KPlayerChat::SendSystemInfo(1, nPlayerIndex, MESSAGE_SYSTEM_ANNOUCE_HEAD, (char *) szMsg, strlen(szMsg) );
	}
	
	return 0;
}

int LuaMsgToTeam(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0	) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		if (Player[nPlayerIndex].m_cTeam.m_nID >= 0)
		{
			const	char * szMsg = Lua_ValueToString(L,1);
			int nTeamLeaderId = g_Team[Player[nPlayerIndex].m_cTeam.m_nID].m_nCaptain;
			if (nTeamLeaderId > 0)
				KPlayerChat::SendSystemInfo(1, nTeamLeaderId, "×é¶ÓÏûÏ¢",(char *) szMsg, strlen(szMsg) );
			
			for (int i = 0; i < MAX_TEAM_MEMBER; i ++)
			{
				int nMemberId = g_Team[Player[nPlayerIndex].m_cTeam.m_nID].m_nMember[i] ;
				if (nMemberId > 0)
				{
					if (szMsg)
						KPlayerChat::SendSystemInfo(1, nMemberId, "×é¶ÓÏûÏ¢", (char *)szMsg, strlen(szMsg) );
				}
			}
		}
	}
	return 0;
}

int LuaMsgToSubWorld(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0 ) return 0;
	
	const char * szMsg = Lua_ValueToString(L,1);
	if (szMsg)
		KPlayerChat::SendSystemInfo(0, 0, MESSAGE_SYSTEM_ANNOUCE_HEAD, (char *)szMsg, strlen(szMsg) );
	return 0;
}

int LuaMsgToAroundRegion(Lua_State * L)
{
	
	return 0;
}

//**************************************************************************************************************************************************************
//												Ö÷½Ç½Å±¾
//**************************************************************************************************************************************************************

//**************************************************************************************************************************************************************
//												Ö÷½Ç½Å±¾
//**************************************************************************************************************************************************************

/*¹¦ÄÜ£ºÈÃÍæ¼Ò½øÈëÐÂµÄÒ»¸öÓÎÏ·ÊÀ½ç
nPlayerIndex:Ö÷½ÇµÄIndex
nSubWorldIndex:ÓÎÏ·ÊÀ½çid
nPosX:
nPosY:
*/
//NewWorld(WorldId, X,Y)
int  LuaEnterNewWorld(Lua_State * L) 
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) 
		return 0;
	
	int nResult = 0;
	if (Lua_GetTopIndex(L) >= 3)
	{
		DWORD dwWorldId = (DWORD)Lua_ValueToNumber(L, 1);
		nResult = Npc[Player[nPlayerIndex].m_nIndex].ChangeWorld(dwWorldId, (int)Lua_ValueToNumber(L,2) * 32, (int)Lua_ValueToNumber(L,3) * 32);
	}
	Lua_PushNumber(L, nResult);
	return 1;
}

//SetPos(X,Y)
int LuaSetPos(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount != 2) return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	
	int nX = (int) Lua_ValueToNumber(L,1);
	int nY = (int) Lua_ValueToNumber(L,2);
	
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].SetPos(nX * 32, nY * 32);
	}
	return 0;
}

int LuaGetPos(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex > 0)
	{
		int nPosX = 0;
		int nPosY = 0;
		Npc[Player[nPlayerIndex].m_nIndex].GetMpsPos(&nPosX, &nPosY);
		Lua_PushNumber(L, nPosX);
		Lua_PushNumber(L, nPosY);
		Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_SubWorldIndex); 
	}
	else
		return 0;
	return 3;
}

//W,X,Y = GetWorldPos()
int LuaGetNewWorldPos(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex > 0)
	{
		int nPosX = 0;
		int nPosY = 0;
		Npc[Player[nPlayerIndex].m_nIndex].GetMpsPos(&nPosX, &nPosY);

		int nSubWorldIndex = Npc[Player[nPlayerIndex].m_nIndex].m_SubWorldIndex;
		int nSubWorldID = 0;
		if (nSubWorldIndex >= 0 && nSubWorldIndex < MAX_SUBWORLD)
		{
			nSubWorldID = SubWorld[nSubWorldIndex].m_SubWorldID;
		}

		Lua_PushNumber(L, nSubWorldID); 
		Lua_PushNumber(L, ((int)(nPosX / 32)));
		Lua_PushNumber(L, ((int)(nPosY / 32)));
	}
	else
	{
		Lua_PushNumber(L, 0);
		Lua_PushNumber(L, 0);
		Lua_PushNumber(L, 0);
		return 3;
	}
	return 3;
}


int LuaDropItem(Lua_State *L)
{
	return 0;
}

/*AddEventItem(id)*/
//KTabFile g_EventItemTab;
int LuaAddEventItem(Lua_State *L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if (nParamNum < 1)
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L,0);
		return 1;
	}

	int nEventId = (int)Lua_ValueToNumber(L,1);
	int nCount = 0;
	if(nParamNum >= 2)
	{
		nCount = (int)Lua_ValueToNumber(L,2);
		if(nCount <= 0)
			nCount = 0;
	}
	int nIndex = ItemSet.Add(item_task, 0, 0, nCount, nEventId, 0, 0);
	
	if (nIndex <= 0)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	int		x, y;
	if (Player[nPlayerIndex].m_ItemList.CheckCanPlaceInEquipment(Item[nIndex].GetWidth(), Item[nIndex].GetHeight(), &x, &y))
	{
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_equiproom, x, y);
	}
	else
	{
		int		nIdx = Player[nPlayerIndex].m_ItemList.Hand();
		if (nIdx)
		{
			Player[nPlayerIndex].m_ItemList.Remove(nIdx);
			
			KMapPos			sMapPos;
			KObjItemInfo	sInfo;
			
			Player[nPlayerIndex].GetAboutPos(&sMapPos);
			
			sInfo.m_nItemID = nIdx;
			sInfo.m_nItemWidth = Item[nIdx].GetWidth();
			sInfo.m_nItemHeight = Item[nIdx].GetHeight();
			sInfo.m_nMoneyNum = 0;
			strcpy(sInfo.m_szName, Item[nIdx].GetName());
			sInfo.m_nColorID = 0;
			sInfo.m_nMovieFlag = 1;
			sInfo.m_nSoundFlag = 1;
			
			int nObj = ObjSet.Add(Item[nIdx].GetObjIdx(), sMapPos, sInfo);
			if (nObj >= 0)
			{
				if (Item[nIdx].GetGenre() == item_task)
				{
					Object[nObj].SetEntireBelong(nPlayerIndex);
				}
				else
				{
					Object[nObj].SetItemBelong(nPlayerIndex);
				}
			}
		}
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_hand, 0 ,0);
	}		
	Lua_PushNumber(L, nIndex);
	return 1;
}

/*AddItem(nItemClass, nDetailType, nParticualrType, nLevel, nSeries, nLuck, nItemLevel..6)*/
int LuaAddItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	if (nParamNum < 6)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	int nItemClass		= (int)Lua_ValueToNumber(L, 1);
	int nDetailType		= (int)Lua_ValueToNumber(L, 2);
	int nParticularType	= (int)Lua_ValueToNumber(L, 3);
	int nLevel			= (int)Lua_ValueToNumber(L, 4);
	int nSeries			= (int)Lua_ValueToNumber(L, 5);
	int nLuck			= (int)Lua_ValueToNumber(L, 6);
	int nItemLevel[6];
	
	ZeroMemory(nItemLevel, sizeof(nItemLevel));
	if(nParamNum >= 7)
	nItemLevel[0] = (int)Lua_ValueToNumber(L, 7);
	
	if (nParamNum >= 12)
	{
		nItemLevel[1] = (int)Lua_ValueToNumber(L, 8);
		nItemLevel[2] = (int)Lua_ValueToNumber(L, 9);
		nItemLevel[3] = (int)Lua_ValueToNumber(L, 10);
		nItemLevel[4] = (int)Lua_ValueToNumber(L, 11);
		nItemLevel[5] = (int)Lua_ValueToNumber(L, 12);
	}
	else
	{
		for (int i = 0; i < 5; i ++)
			nItemLevel[i + 1] = nItemLevel[0];
	}
	int nIndex;
	if(nItemClass >= item_gengold)
		nIndex = ItemSet.Add(nItemClass, nSeries, nLevel, nLuck, nDetailType, nParticularType, NULL);
	else
		nIndex = ItemSet.Add(nItemClass, nSeries, nLevel, nLuck, nDetailType, nParticularType, nItemLevel);
	if (nIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	
	int		x, y;
	if (Player[nPlayerIndex].m_ItemList.CheckCanPlaceInEquipment(Item[nIndex].GetWidth(), Item[nIndex].GetHeight(), &x, &y))
	{
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_equiproom, x, y);
	}
	else
	{
		int		nIdx = Player[nPlayerIndex].m_ItemList.Hand();
		if (nIdx)
		{
			Player[nPlayerIndex].m_ItemList.Remove(nIdx);
			
			KMapPos sMapPos;
			KObjItemInfo	sInfo;
			
			Player[nPlayerIndex].GetAboutPos(&sMapPos);
			
			sInfo.m_nItemID = nIdx;
			sInfo.m_nItemWidth = Item[nIdx].GetWidth();
			sInfo.m_nItemHeight = Item[nIdx].GetHeight();
			sInfo.m_nMoneyNum = 0;
			strcpy(sInfo.m_szName, Item[nIdx].GetName());
			sInfo.m_nColorID = 0;
			sInfo.m_nMovieFlag = 1;
			sInfo.m_nSoundFlag = 1;
			
			int nObj = ObjSet.Add(Item[nIdx].GetObjIdx(), sMapPos, sInfo);
			if (nObj >= 0)
			{
				if (Item[nIdx].GetGenre() == item_task)
				{
					Object[nObj].SetEntireBelong(nPlayerIndex);
				}
				else
				{
					Object[nObj].SetItemBelong(nPlayerIndex);
				}
			}
		}
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_hand, 0 ,0);
	}
	
	Lua_PushNumber(L, nIndex);
	
	return 1;
	
}

int AddGoldItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	if (nParamNum < 2)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	int nLuck = (int)Lua_ValueToNumber(L, 1);
	int nRowID = (int)Lua_ValueToNumber(L, 2);
	int nIndex = ItemSet.Add(item_gengold, 0, 0, nLuck, nRowID, 0,  NULL);
	if (nIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	if (nParamNum >= 3)	//add hÖ thèng, kh«ng add vµo r­¬ng
	{
		Lua_PushNumber(L, nIndex);
		return 1;
	}
	int		x, y;
	if (Player[nPlayerIndex].m_ItemList.CheckCanPlaceInEquipment(Item[nIndex].GetWidth(), Item[nIndex].GetHeight(), &x, &y))
	{
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_equiproom, x, y);
	}
	else
	{
		int		nIdx = Player[nPlayerIndex].m_ItemList.Hand();
		if (nIdx)
		{
			Player[nPlayerIndex].m_ItemList.Remove(nIdx);
			
			KMapPos sMapPos;
			KObjItemInfo	sInfo;
			
			Player[nPlayerIndex].GetAboutPos(&sMapPos);
			
			sInfo.m_nItemID = nIdx;
			sInfo.m_nItemWidth = Item[nIdx].GetWidth();
			sInfo.m_nItemHeight = Item[nIdx].GetHeight();
			sInfo.m_nMoneyNum = 0;
			strcpy(sInfo.m_szName, Item[nIdx].GetName());
			sInfo.m_nColorID = 0;
			sInfo.m_nMovieFlag = 1;
			sInfo.m_nSoundFlag = 1;
			
			int nObj = ObjSet.Add(Item[nIdx].GetObjIdx(), sMapPos, sInfo);
			if (nObj >= 0)
			{
				Object[nObj].SetItemBelong(nPlayerIndex);
			}
		}
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_hand, 0 ,0);
	}
	
	Lua_PushNumber(L, nIndex);
	
	return 1;
}

int AddPurpleItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	
	if (nParamNum < 5)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	int nItemClass		= (int)Lua_ValueToNumber(L, 1);
	int nDetailType		= (int)Lua_ValueToNumber(L, 2);
	int nParticularType	= (int)Lua_ValueToNumber(L, 3);
	int nLevel			= (int)Lua_ValueToNumber(L, 4);
	int nSeries			= (int)Lua_ValueToNumber(L, 5);
	int nRows = 6;
	if (nParamNum >= 6)
		nRows = (int)Lua_ValueToNumber(L, 6);
	int nIndex = ItemSet.Add(item_genpurple, nSeries, nLevel, nRows, nDetailType, nParticularType,  NULL);
	if (nIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	
	int		x, y;
	if (Player[nPlayerIndex].m_ItemList.CheckCanPlaceInEquipment(Item[nIndex].GetWidth(), Item[nIndex].GetHeight(), &x, &y))
	{
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_equiproom, x, y);
	}
	else
	{
		int		nIdx = Player[nPlayerIndex].m_ItemList.Hand();
		if (nIdx)
		{
			Player[nPlayerIndex].m_ItemList.Remove(nIdx);
			
			KMapPos sMapPos;
			KObjItemInfo	sInfo;
			
			Player[nPlayerIndex].GetAboutPos(&sMapPos);
			
			sInfo.m_nItemID = nIdx;
			sInfo.m_nItemWidth = Item[nIdx].GetWidth();
			sInfo.m_nItemHeight = Item[nIdx].GetHeight();
			sInfo.m_nMoneyNum = 0;
			strcpy(sInfo.m_szName, Item[nIdx].GetName());
			sInfo.m_nColorID = 0;
			sInfo.m_nMovieFlag = 1;
			sInfo.m_nSoundFlag = 1;
			
			int nObj = ObjSet.Add(Item[nIdx].GetObjIdx(), sMapPos, sInfo);
			if (nObj >= 0)
			{
				Object[nObj].SetItemBelong(nPlayerIndex);
			}
		}
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_hand, 0 ,0);
	}
	
	Lua_PushNumber(L, nIndex);
	
	return 1;
}

int AddGenItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);	
	if (nParamNum < 7)
	{
		Lua_PushNumber(L,0);
		return 1;
	}
	int nExType			= (int)Lua_ValueToNumber(L, 1);
	int nItemClass		= (int)Lua_ValueToNumber(L, 2);
	int nDetailType		= (int)Lua_ValueToNumber(L, 3);
	int nParticularType	= (int)Lua_ValueToNumber(L, 4);
	int nLevel			= (int)Lua_ValueToNumber(L, 5);
	int nSeries			= (int)Lua_ValueToNumber(L, 6);
	int nLuck			= (int)Lua_ValueToNumber(L, 7);
	if(nExType == extype_gold)
		nItemClass = item_gengold;
	else if(nExType == extype_platina)
		nItemClass = item_genplatina;
	else if(nExType == extype_purple)
		nItemClass = item_genpurple;

	int nIndex = ItemSet.Add(nItemClass, nSeries, nLevel, nLuck, nDetailType, nParticularType,  NULL);
	if (nIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	Lua_PushNumber(L, nIndex);
	
	return 1;
}

int PlaceItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0)
		return 0;
	
	if (nParamNum < 1)
		return 0;
	int nIndex = (int)Lua_ValueToNumber(L, 1);
	if(nIndex <= 0 || nIndex >= MAX_ITEM || Item[nIndex].GetID() == 0)
		return 0;
	
	int		x, y;
	if (Player[nPlayerIndex].m_ItemList.CheckCanPlaceInEquipment(Item[nIndex].GetWidth(), Item[nIndex].GetHeight(), &x, &y))
	{
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_equiproom, x, y);
	}
	else
	{
		int		nIdx = Player[nPlayerIndex].m_ItemList.Hand();
		if (nIdx)
		{
			Player[nPlayerIndex].m_ItemList.Remove(nIdx);
			
			KMapPos sMapPos;
			KObjItemInfo	sInfo;
			
			Player[nPlayerIndex].GetAboutPos(&sMapPos);
			
			sInfo.m_nItemID = nIdx;
			sInfo.m_nItemWidth = Item[nIdx].GetWidth();
			sInfo.m_nItemHeight = Item[nIdx].GetHeight();
			sInfo.m_nMoneyNum = 0;
			strcpy(sInfo.m_szName, Item[nIdx].GetName());
			sInfo.m_nColorID = 0;
			sInfo.m_nMovieFlag = 1;
			sInfo.m_nSoundFlag = 1;
			
			int nObj = ObjSet.Add(Item[nIdx].GetObjIdx(), sMapPos, sInfo);
			if (nObj >= 0)
			{
				if (Item[nIdx].GetGenre() == item_task)
				{
					Object[nObj].SetEntireBelong(nPlayerIndex);
				}
				else
				{
					Object[nObj].SetItemBelong(nPlayerIndex);
				}
			}
		}
		Player[nPlayerIndex].m_ItemList.Add(nIndex, pos_hand, 0 ,0);
	}
	return 0;
}

int SetItemDur(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2)
		return 0;
	int nIndex = (int)Lua_ValueToNumber(L, 1);
	if(nIndex <= 0 || nIndex >= MAX_ITEM || Item[nIndex].GetID() == 0)
		return 0;
	Item[nIndex].SetDurability((int)Lua_ValueToNumber(L, 2));
	return 0;
}

int SetItemMA(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);	
	if (nParamNum < 4)
		return 0;
	int nIndex = (int)Lua_ValueToNumber(L, 1);
	if(nIndex <= 0 || nIndex >= MAX_ITEM || Item[nIndex].GetID() == 0)
		return 0;
	int nAtRow = (int)Lua_ValueToNumber(L, 2);
	if(nAtRow <= 0 || nAtRow > 8)
		return 0;
	if(nAtRow <= 6)
	{
		nAtRow--;
		Item[nIndex].m_aryMagicAttrib[nAtRow].nAttribType = (int)Lua_ValueToNumber(L, 3);
		Item[nIndex].m_aryMagicAttrib[nAtRow].nValue[0] = (int)Lua_ValueToNumber(L, 4);
		Item[nIndex].m_aryMagicAttrib[nAtRow].nValue[1] = -1;
		if(Item[nIndex].m_aryMagicAttrib[nAtRow].nAttribType == magic_addphysicsdamage_p)
			Item[nIndex].m_aryMagicAttrib[nAtRow].nValue[2] = 6;
		else if(nParamNum >= 5)
			Item[nIndex].m_aryMagicAttrib[nAtRow].nValue[2] = (int)Lua_ValueToNumber(L, 5);
		else
			Item[nIndex].m_aryMagicAttrib[nAtRow].nValue[2] = 0;
		if(Item[nIndex].m_aryMagicAttrib[nAtRow].nAttribType == magic_indestructible_b)
		{
			Item[nIndex].SetDurability(-1);
		}
	}
	else
	{
		nAtRow -= 7;
		Item[nIndex].m_aryMagicAttribEx[nAtRow].nAttribType = (int)Lua_ValueToNumber(L, 3);
		Item[nIndex].m_aryMagicAttribEx[nAtRow].nValue[0] = (int)Lua_ValueToNumber(L, 4);
		Item[nIndex].m_aryMagicAttribEx[nAtRow].nValue[1] = -1;
		if(Item[nIndex].m_aryMagicAttribEx[nAtRow].nAttribType == magic_addphysicsdamage_p)
			Item[nIndex].m_aryMagicAttribEx[nAtRow].nValue[2] = 6;
		else if(nParamNum >= 5)
			Item[nIndex].m_aryMagicAttribEx[nAtRow].nValue[2] = (int)Lua_ValueToNumber(L, 5);
		else
			Item[nIndex].m_aryMagicAttribEx[nAtRow].nValue[2] = 0;
	}
	return 0;
}

int LuaGetTaskItemCount(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	int nRetCount = 0;
	if(nPlayerIndex <= 0 || nParamNum < 1)
	{
		Lua_PushNumber(L , nRetCount);
		return 1;
	}
	if (nParamNum <= 2)
	{
		int nEventId = (int)Lua_ValueToNumber(L,1);
		nRetCount = Player[nPlayerIndex].m_ItemList.GetTaskItemNum(item_task, nEventId, 0);
	}
	else
	{
		int nGenre = (int)Lua_ValueToNumber(L,1);
		int nDetail = (int)Lua_ValueToNumber(L,2);
		int nPart = (int)Lua_ValueToNumber(L,3);
		int nRoomPos = pos_equiproom;
		if(nParamNum >= 4)
			nRoomPos = (int)Lua_ValueToNumber(L,4);
		int nLevel = 0;
		if(nParamNum >= 5)
			nLevel = (int)Lua_ValueToNumber(L,5);
		int nSeries = -1;
		if(nParamNum >= 6)
			nSeries = (int)Lua_ValueToNumber(L,6);
		nRetCount = Player[nPlayerIndex].m_ItemList.GetTaskItemNum(nGenre, nDetail, nPart,
												nRoomPos, nLevel, nSeries);
	}
	Lua_PushNumber(L , nRetCount);
	return 1;
}


/*
DelItem (nPlayerIndex, nItemTemplateId)
<= 2 th«ng sè: xãa item_task theo sè l­îng
>= 3 th«ng sè: xãa item theo genre,detailtype,particular theo sè l­îng
*/
int LuaDelItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	if(nPlayerIndex <= 0 || nParamNum < 1)
		return 0;
	if (nParamNum <= 2)
	{
		int nEventId = (int)Lua_ValueToNumber(L,1);
		int nCount = 1;
		if(nParamNum == 2)
			nCount = (int)Lua_ValueToNumber(L,2);
		Player[nPlayerIndex].m_ItemList.RemoveTaskItem(item_task, nEventId, 0, nCount);
	}
	else
	{
		int nGenre = (int)Lua_ValueToNumber(L,1);
		int nDetail = (int)Lua_ValueToNumber(L,2);
		int nPart = (int)Lua_ValueToNumber(L,3);
		int nCount = 1;
		if(nParamNum >= 4)
			nCount = (int)Lua_ValueToNumber(L,4);
		int nRoomPos = pos_equiproom;
		if(nParamNum >= 5)
			nRoomPos = (int)Lua_ValueToNumber(L,5);
		int nLevel = 0;
		if(nParamNum >= 6)
			nLevel = (int)Lua_ValueToNumber(L,6);
		int nSeries = -1;
		if(nParamNum >= 7)
			nSeries = (int)Lua_ValueToNumber(L,7);
		Player[nPlayerIndex].m_ItemList.RemoveTaskItem(nGenre, nDetail, nPart, nCount,
												nRoomPos, nLevel, nSeries);
	}
	
	return 0;
}

int ClearItem(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	int nPlayerIndex = GetPlayerIndex(L);
	if(nPlayerIndex <= 0)
		return 0;
	int nRoomPos = pos_equiproom;
	if(nParamNum >= 1)
		nRoomPos = (int)Lua_ValueToNumber(L,1);
	Player[nPlayerIndex].m_ItemList.ClearItem(nRoomPos);
	return 0;
}
/*HaveItem(nItemTemplateId);
chØ check item_task theo detailtype
*/
int LuaHaveItem(Lua_State *L)
{
	if (Lua_GetTopIndex(L) > 0)
	{
		int nPlayerIndex = GetPlayerIndex(L);
		if (nPlayerIndex > 0)
		{
			int nEventId = (int)Lua_ValueToNumber(L,1);
			
			int nResult = Player[nPlayerIndex].m_ItemList.IsTaskItemExist(nEventId);
			Lua_PushNumber(L, (nResult != 0));
		}
		else 
		{
			Lua_PushNumber(L,0);
		}
	}
	else
	{
		Lua_PushNumber(L , 0);
	}
	
	return 1;
}

/*
AddMagic(nPlayerIndex, nMagicID, nLevel)
DelMagic(nPlayerIndex, nMagicId)
HaveMagic(nPlayerIndex, nMagicId)
GetMagicLevel(nPlayerIndex, nMagicId)
SetMagicLevel(nPlayerIndex, nMagicId, nLevel)
ModifyMagicLevel(nPlayerIndex ,nMagicId, nDLevel)
*/
int LuaAddMagic(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	int nPlayerIndex = 0;
	if (nParamCount < 1) return 0;
	nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	int nSkillId = 0;
	if (Lua_IsNumber(L, 1))
	{
		nSkillId = (int)Lua_ValueToNumber (L, 1);
	}
	else 
	{
		const char * sSkillName = Lua_ValueToString(L, 1);
		g_OrdinSkillsSetting.GetInteger((char*)sSkillName, "SkillId", 0, &nSkillId);
		if (nSkillId <= 0 ) return 0;
	}
    Player[nPlayerIndex].m_nIndex;
	int nLevel = 0;
	if (nParamCount >=2)
		nLevel = (int)Lua_ValueToNumber(L,2);
	else
		nLevel = 0;
	
    int nRet = Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.Add(nSkillId, nLevel);
	if (nRet)
	{
		PLAYER_SKILL_LEVEL_SYNC NewSkill;
		NewSkill.ProtocolType = s2c_playerskilllevel;
		NewSkill.m_nSkillID = nSkillId;
		NewSkill.m_nSkillLevel = Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.GetCurrentLevel(nSkillId);
		NewSkill.m_nLeavePoint = Player[nPlayerIndex].m_nSkillPoint;
		g_pServer->PackDataToClient(Player[nPlayerIndex].m_nNetConnectIdx, (BYTE*)&NewSkill, sizeof(PLAYER_SKILL_LEVEL_SYNC));
	}
	
	return 0;
}

int LuaDelMagic(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	int nPlayerIndex = 0;
	if (nParamCount < 1) return 0;
	
    nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) return 0;
	
	int nSkillId = 0;
	if (Lua_IsNumber(L, 1))
	{
		nSkillId = (int)Lua_ValueToNumber (L, 1);
	}
	else 
	{
		const char * sSkillName = Lua_ValueToString(L, 1);
		g_OrdinSkillsSetting.GetInteger((char*)sSkillName, "SkillId", 0, &nSkillId);
		if (nSkillId <= 0 ) return 0;
	}
	Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.Remove(nSkillId);
	return 0;
	
}



int LuaHaveMagic(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	int nPlayerIndex = 0;
	if (nParamCount < 1) return 0;
	
	nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) return 0;
	
	int nSkillId = 0;
	if (Lua_IsNumber(L, 1))
	{
		nSkillId = (int)Lua_ValueToNumber (L, 1);
	}
	else 
	{
		const char * sSkillName = Lua_ValueToString(L, 1);
		g_OrdinSkillsSetting.GetInteger((char *)sSkillName, "SkillId", 0, &nSkillId);
		if (nSkillId <= 0 ) 
		{
			Lua_PushNumber(L, -1);
		}
		return 1;
	}
	
	if (Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.FindSame(nSkillId))
	{
		Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.GetLevel(nSkillId));
	}
	else
	{
		Lua_PushNumber(L, -1);
	}
	
	return 1;
}

int LuaGetSkillIdInSkillList(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	int nSkillIndex = (int)Lua_ValueToNumber (L, 1);
	int nSkillId = 0;
	if (nSkillIndex > 0) 
	{
		nSkillId = Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.GetSkillId(nSkillIndex);
	}
	Lua_PushNumber(L, nSkillId);
	return 1;
}

int LuaSetSkillLevel(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2) 
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	int nSkillId = 0;
	
	if (Lua_IsNumber(L, 1))
	{
		nSkillId = (int)Lua_ValueToNumber (L, 1);
	}
	else 
	{
		const char * sSkillName = Lua_ValueToString(L, 1);
		g_OrdinSkillsSetting.GetInteger((char *)sSkillName, "SkillId", 0, &nSkillId);
		if (nSkillId <= 0 ) return 0;
	}
	int nLevel = (int)Lua_ValueToNumber(L, 2);
	if (nLevel >= 0)
		Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.SetSkillLevelDirectlyUsingId(nSkillId, nLevel);
	return 0;
}

int LuaRollBackSkills(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	int nSkillId = 0;
	int nTotalSkill = Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.ClearSkillLevel(true);
	Lua_PushNumber(L, nTotalSkill);
	return 1;
}

int LuaUpdateSkillList(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	Player[nPlayerIndex].SendSyncData_Skill();
	return 0;
}

int LuaGetMagicLevel(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	int nPlayerIndex = 0;
	
	if (nParamCount < 1) return 0;
	nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) return 0;
	
	int nSkillId = 0;
	if (Lua_IsNumber(L, 1))
	{
		nSkillId = (int)Lua_ValueToNumber (L, 1);
	}
	else 
	{
		const char * sSkillName = Lua_ValueToString(L, 1);
		g_OrdinSkillsSetting.GetInteger((char *)sSkillName, "SkillId", 0, &nSkillId);
		if (nSkillId <= 0 ) return 0;
	}
	Lua_PushNumber(L,Npc[Player[nPlayerIndex].m_nIndex].m_SkillList.GetLevel(nSkillId));
	return 1;
	
}
/*
int LuaSetMagicLevel(Lua_State * L)
{
int nParamCount = Lua_GetTopIndex(L);
int nPlayerIndex = 0;
int nTemp = 0;
if (nParamCount < 1) return 0;

  nPlayerIndex = GetPlayerIndex(L);
  
	if (nPlayerIndex <= 0) return 0;
	
	  int nSkillId = 0;
	  if (Lua_IsNumber(L, nTemp))
	  {
	  nSkillId = (int)Lua_ValueToNumber (L, 1);
	  }
	  else 
	  {
	  const char * sSkillName = Lua_ValueToString(L, 1);
	  nSkillId = g_OrdinSkillsSetting.FindRow((char *)sSkillName) - 2;
	  if (nSkillId <= 0 ) return 0;
	  }
	  int nNpcIndex = Player[nPlayerIndex].m_nIndex;
	  if (nNpcIndex > 0)
	  Lua_PushNumber(L,Npc[nNpcIndex].m_SkillList.SetSkillLevel(nSkillId, (int)Lua_ValueToNumber(L, 2)));
	  return 0;
	  }
*/
//**************************************************************************************************************************************************************
//												NPC²Ù×÷½Å±¾
//**************************************************************************************************************************************************************
/*nNpcTemplateId GetNpcTmpId(sName)
¹¦ÄÜ´ÓNpcÄ£°åÖÐ»ñµÃÃû³ÆÎªsNameµÄNpcÔÚÄ£°åÖÐµÄId
sName:NpcÃû³Æ
nNpcTemplateID:Ä£°åÖÐId
*/

int LuaGetNpcTemplateID(Lua_State * L)
{
	if (Lua_GetTopIndex(L) > 0)
	{
		if (Lua_IsString(L,1))
		{
			const char * pName = lua_tostring(L,1);	
			int nId = g_NpcSetting.FindRow((char*)pName) - 2;
			Lua_PushNumber(L, nId);
		}
		else
			return 0;
	}
	else 
		return 0;
	return 1;
}

/*
nNpcIndex AddNpc(nNpcTemplateId,nLevel, nSubWorldIndex, nPosX, nPosY )

  ¹¦ÄÜ£ºÔö¼ÓÒ»¸öÌØ¶¨µÄNPC
  ²ÎÊý£º
  nNpcTemplateId: NPCÔÚNPCÄ£°åÖÐµÄid
  nLevel:NpcµÄµÈ¼¶
  nSubWorldIndex:Ëù´¦µÄÊÀ½çid
  nPosX£ºX (µã×ø±ê)
  nPosY£ºY (µã×ø±ê)
  nNpcIndex:Ôö¼Óºó£¬½«·µ»Ø¸ÃNpcÔÚÓÎÏ·ÊÀ½çµÄIndex£¬Èç¹û²»³É¹¦·µ»Ønil 
*/

int LuaAddNpc(Lua_State * L)
{
	char * pName = NULL;
	int	   nId = 0;
	if (Lua_GetTopIndex(L) < 5) return 0;
	
	if (Lua_IsNumber(L,1))
	{
		nId = (int)Lua_ValueToNumber(L,1);
	}
	else if	(Lua_IsString(L,1))
	{
		pName = (char *)lua_tostring(L,1);	
		nId = g_NpcSetting.FindRow((char*)pName) - 2;
	}
	else return 0;
	
	if (nId < 0) nId = 0;	
	
	int nLevel = (int)lua_tonumber(L,2);
	//if (nLevel >= 128) nLevel = 127;
	if (nLevel < 0 ) nLevel = 1;
	
	int	nNpcIdxInfo = MAKELONG(nLevel, nId);//(nId << 7) + nLevel;
	//question
	int nNpcIdx = NpcSet.Add(nNpcIdxInfo, (int)lua_tonumber(L, 3), (int)lua_tonumber(L,4), (int)lua_tonumber(L,5));
	//	g_StrCpy(Npc[nNpcIdx].Name, (char*)pName);
	Lua_PushNumber(L, nNpcIdx);
    return 1;
}

/*nResult DelNpc (nNpcIndex)
¹¦ÄÜ£ºÉ¾³ýÒ»¸öÌØ¶¨µÄNPC
nResult:·µ»Ø³É¹¦Óë·ñ,1Îª³É¹¦,0ÎªÊ§°Ü
*/
int LuaDelNpc(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <= 0 ) return 0 ;
	int nNpcIndex = (int)Lua_ValueToNumber(L, 1);
	if (nNpcIndex > 0)
	{
		if (Npc[nNpcIndex].m_RegionIndex >= 0)
		{
			SubWorld[Npc[nNpcIndex].m_SubWorldIndex].m_Region[Npc[nNpcIndex].m_RegionIndex].RemoveNpc(nNpcIndex);
			SubWorld[Npc[nNpcIndex].m_SubWorldIndex].m_Region[Npc[nNpcIndex].m_RegionIndex].DecRef(Npc[nNpcIndex].m_MapX, Npc[nNpcIndex].m_MapY, obj_npc);
		}
		NpcSet.Remove(nNpcIndex);
	}
	return 0;
}

/*
nDelCount DelNpcsInRgn(nSubWorld,nRegionId, nKind) 
¹¦ÄÜ£ºÉ¾³ýÄ³¸öÓÎÏ·ÊÀ½çÖÐÄ³¸öRegionÄÚµÄËùÓÐÄ³ÀàµÄNPC
·µ»Ø:É¾³ýµÄNpc¸öÊý
*/

int LuaDelNpcsInRgn(Lua_State * L)
{
	//Question
	return 0;
}
/*
nDelCount DelNpcsInWld(nSubWorldId, nKind)
¹¦ÄÜ£ºÉ¾³ýÄ³¸öÓÎÏ·ÊÀ½çÖÐµÄËùÓÐNpc
*/
int LuaDelNpcsInWld(Lua_State * L)
{
	return 0;
}

/*
SetNpcPos (nNpcIndex, x, y)
¹¦ÄÜ£ºÉèÖÃ/ÐÞ¸ÄÒ»¸öNPCµÄÎ»ÖÃ
²ÎÊý£º
nNpcIndex:NpcµÄid 
x£ºX×ø±ê
y£ºY×ø±ê
*/
int LuaSetNpcPos(Lua_State * L)
{
	int nParamCount = 0;
	if ((nParamCount = Lua_GetTopIndex(L)) < 3) return 0;
	int nNpcIndex = (int)Lua_ValueToNumber(L, 1);
	if (nNpcIndex <= 0) return 0;
	Npc[nNpcIndex].m_MapX = (int)Lua_ValueToNumber(L, 2);
	Npc[nNpcIndex].m_MapY = (int)Lua_ValueToNumber(L, 3);
	
	return 0;
}


/*SetNpcDthSct (nNpcIndex, map, ¡°*.txt¡± )
¹¦ÄÜ£ºÉèÖÃNPCËÀÍö½Å±¾
²ÎÊý£º
nNpcIndex£ºNPCIndex
*.txt£º½Å±¾ÎÄ¼þÃû
*/
int LuaSetNpcActionScript(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2 ) return 0;
	int nNpcIndex = (int)Lua_ValueToNumber(L, 1);
	if (nNpcIndex <= 0 || nNpcIndex >= MAX_NPC) return 0;
	strcpy(Npc[nNpcIndex].ActionScript, Lua_ValueToString(L,2));
	Npc[nNpcIndex].m_ActionScriptID = g_FileName2Id((char *)Lua_ValueToString(L,2));
	return 0;
}

/*
SetRevivalPos(subworldid = -1, revid )
¹¦ÄÜ£ºÉèÖÃNpcÖØÉúµã
*/
int LuaSetPlayerRevivalPos(Lua_State * L)
{
	//Question
	int nParamCount = Lua_GetTopIndex(L);
	int nPlayerIndex = 0;
	int nBeginIndex = 2;
	nPlayerIndex = GetPlayerIndex(L);
	int nSubWorldId = 0;
	int nRevId = 0;
	if (nPlayerIndex < 0) 
	{
		return 0;
	}
	
	if (nParamCount >= 2)
	{
		nSubWorldId = (int) Lua_ValueToNumber(L, 1);
		nRevId = (int) Lua_ValueToNumber(L, 2);
	}
	else if (nParamCount == 1)
	{
		nSubWorldId = -1;
		nRevId = (int) Lua_ValueToNumber(L, 1);
	}
	else 
	{
		return 0;
	}
	
	Player[nPlayerIndex].SetRevivalPos(nSubWorldId, nRevId);
	return 0;
}

//**************************************************************************************************************************************************************
//												ÁÄÌìÏûÏ¢½Å±¾
//**************************************************************************************************************************************************************

//**********************************************************************************************
//							Ö÷½ÇÊôÐÔ»ñµÃ
//**********************************************************************************************


#define MacroFun_GetPlayerInfoInt(L, MemberName) { int nPlayerIndex = GetPlayerIndex(L);\
	if (nPlayerIndex > 0){	int nNpcIndex = Player[nPlayerIndex].m_nIndex;	if (nNpcIndex > 0)Lua_PushNumber(L, Npc[nNpcIndex].MemberName);\
	else Lua_PushNil(L);}\
	else Lua_PushNil(L);\
return 1;}														

//ÕóÓª
int LuaGetPlayerCurrentCamp(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L, m_CurrentCamp);
}

int LuaGetPlayerCamp(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L, m_Camp);
}

int LuaSetPlayerCamp(Lua_State * L)
{
	int nValue = (int)Lua_ValueToNumber(L,1);
	if (nValue < 0 ) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].SetCamp(nValue);
	}
	return 0;
}

int LuaSetPlayerCurrentCamp(Lua_State * L)
{
	int nValue = (int)Lua_ValueToNumber(L,1);
	if (nValue < 0 ) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].SetCurrentCamp(nValue);
	}
	return 0;
}

int LuaSetNpcCurCamp(Lua_State * L)
{
	int nNpcIndex = (int)Lua_ValueToNumber(L,1);
	if (nNpcIndex <= 0 && nNpcIndex > MAX_NPC) return 0;
	int nValue = (int )Lua_ValueToNumber(L,2);
	if (nValue >= camp_num) return 0;
	Npc[nNpcIndex].SetCurrentCamp(nValue);
	return 0;
}

int LuaRestorePlayerCamp(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].RestoreCurrentCamp();
	}
	return 0;
}

int LuaOpenTong(Lua_State * L)
{	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_OPENTONGUI;
	UiInfo.m_bOptionNum = 0;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	UiInfo.m_bParam1 = 0;
	UiInfo.m_nParam = 0;
	UiInfo.m_nBufferLen = 0;
	
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
#else
	UiInfo.m_bParam2 = 1;
	UiInfo.ProtocolType = (BYTE)s2c_scriptaction;
	UiInfo.m_wProtocolLong = sizeof(PLAYER_SCRIPTACTION_SYNC) - MAX_SCIRPTACTION_BUFFERNUM - 1;
	g_pServer->PackDataToClient(Player[nPlayerIndex].m_nNetConnectIdx, &UiInfo, UiInfo.m_wProtocolLong + 1);
#endif
	return 0;
}



//ÃÅÅÉ
int LuaGetPlayerFaction(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);				
	if (nPlayerIndex > 0)								
	{	
		char FactionName[32];
		Player[nPlayerIndex].GetFactionName(FactionName);
		Lua_PushString(L, FactionName);
	}
	else
	{
		Lua_PushString(L,"");
	}
	return 1;
}

int LuaChangePlayerFaction(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);				
	int nResult = 0;
	if (nPlayerIndex > 0)							
	{
		const char * szFactionName = Lua_ValueToString(L,1);
		Player[nPlayerIndex].LeaveCurFaction();
		if (strlen(szFactionName) == 0) 
		{
			nResult = 1;
		}
		else
		{
			nResult = Player[nPlayerIndex].AddFaction((char *)szFactionName);
		}
	}
	Lua_PushNumber(L, nResult);
	return 1;
}

//¿¹ÐÔ *************************************************************************************
//0±íÊ¾µ±Ç°,1±íÊ¾Ô­Ê¼µÄ,2±íÊ¾×î´óµÄ
int LuaGetPlayerColdResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentColdResist);break;
		case 1:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_ColdResist); break;
		case 2:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_ColdResistMax);break;
		default:
			Lua_PushNil(L);
		} 
	}
	else 
		Lua_PushNil(L);
	return 1;
}

int LuaSetPlayerColdResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nValue = (int)Lua_ValueToNumber(L,2);
		if (nValue < 0) nValue = 0;
		if (nValue > Npc[Player[nPlayerIndex].m_nIndex].m_ColdResistMax) nValue = Npc[Player[nPlayerIndex].m_nIndex].m_ColdResistMax;
		
		int nType = (int)Lua_ValueToNumber(L,1);
		
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Npc[Player[nPlayerIndex].m_nIndex].m_CurrentColdResist = nValue;
			break;
			
		case 1:
			Npc[Player[nPlayerIndex].m_nIndex].m_ColdResist = nValue;
			break;
		case 2:
			Npc[Player[nPlayerIndex].m_nIndex].m_ColdResistMax = nValue;
			break;
		}
		
		
	} 
	return 0;
	
}

int LuaGetPlayerFireResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentFireResist);break;
		case 1:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_FireResist); break;
		case 2:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_FireResistMax);break;
		default:
			Lua_PushNil(L);
		}
	} 
	else 
		Lua_PushNil(L);
	return 1;
}

int LuaSetPlayerFireResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		
		int nValue = (int)Lua_ValueToNumber(L,2);
		if (nValue < 0) nValue = 0;
		if (nValue > Npc[Player[nPlayerIndex].m_nIndex].m_FireResistMax) nValue = Npc[Player[nPlayerIndex].m_nIndex].m_FireResistMax;
		
		int nType = (int)Lua_ValueToNumber(L,1);
		
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Npc[Player[nPlayerIndex].m_nIndex].m_CurrentFireResist = nValue;
			break;
			
		case 1:
			Npc[Player[nPlayerIndex].m_nIndex].m_FireResist = nValue;
			break;
		case 2:
			Npc[Player[nPlayerIndex].m_nIndex].m_FireResistMax = nValue;
			break;
		}
		
		
	} 
	return 0;
	
}


int LuaGetPlayerLightResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentLightResist);break;
		case 1:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_LightResist); break;
		case 2:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_LightResistMax);break;
		default:
			Lua_PushNil(L);
		}
	} 
	else
		Lua_PushNil(L);
	return 1;
}

int LuaSetPlayerLightResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nValue = (int)Lua_ValueToNumber(L,2);
		if (nValue < 0) nValue = 0;
		if (nValue > Npc[Player[nPlayerIndex].m_nIndex].m_LightResistMax) nValue = Npc[Player[nPlayerIndex].m_nIndex].m_LightResistMax;
		
		int nType = (int)Lua_ValueToNumber(L,1);
		
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Npc[Player[nPlayerIndex].m_nIndex].m_CurrentLightResist = nValue;
			break;
			
		case 1:
			Npc[Player[nPlayerIndex].m_nIndex].m_LightResist = nValue;
			break;
		case 2:
			Npc[Player[nPlayerIndex].m_nIndex].m_LightResistMax = nValue;
			break;
		}
	} 
	else 
		Lua_PushNil(L);
	return 0;
}


int LuaGetPlayerPoisonResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentPoisonResist);break;
		case 1:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResist); break;
		case 2:
			Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResistMax);break;
		default:
			Lua_PushNil(L);
		}
	} 
	else
		Lua_PushNil(L);
	return 1;
}

int LuaSetPlayerPoisonResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		{
			int nValue = (int)Lua_ValueToNumber(L,2);
            if (nValue < 0) nValue = 0;
            if (nValue > Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResistMax) nValue = Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResistMax;
			
            int nType = (int)Lua_ValueToNumber(L,1);
			
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Npc[Player[nPlayerIndex].m_nIndex].m_CurrentPoisonResist = nValue;
				break;
				
			case 1:
				Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResist = nValue;
                break;
			case 2:
				Npc[Player[nPlayerIndex].m_nIndex].m_PoisonResistMax = nValue;
                break;
			}
		}
		
	} 
	
	
	return 0;
	
}

int LuaGetPlayerPhysicsResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		{
			int nType = (int)Lua_ValueToNumber(L,1);
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentPhysicsResist);break;
			case 1:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResist); break;
			case 2:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResistMax);break;
			default:
				Lua_PushNil(L);
			}
		}
		
	} 
	else
		Lua_PushNil(L);
	return 1;
}



int LuaSetPlayerPhysicsResist(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		{
			int nValue = (int)Lua_ValueToNumber(L,2);
            if (nValue < 0) nValue = 0;
            if (nValue > Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResistMax) nValue = Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResistMax;
			
            int nType = (int)Lua_ValueToNumber(L,1);
			
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Npc[Player[nPlayerIndex].m_nIndex].m_CurrentPhysicsResist = nValue;
				break;
				
			case 1:
				Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResist = nValue;
                break;
			case 2:
				Npc[Player[nPlayerIndex].m_nIndex].m_PhysicsResistMax = nValue;
                break;
			}
		}
		
	} 
	return 0;
	
}


//¾­ÑéÖµ*********************************************************************
int LuaGetPlayerExp(Lua_State *L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_nExp);
	}
	else
		Lua_PushNil(L);
	return 1;
}

//AddExp(200,10,0)
int LuaModifyPlayerExp(Lua_State * L)
{
	int bAllTeamGet = 0;
	if(Lua_GetTopIndex(L) >= 3)		bAllTeamGet = (int)Lua_ValueToNumber(L,3);
	
	int nDValue = (int)Lua_ValueToNumber(L,1);
	int nTarLevel = (int)Lua_ValueToNumber(L,2);
	
	int nPlayerIndex = GetPlayerIndex(L);				
	if (nPlayerIndex > 0)								
	{									
		if (bAllTeamGet)
			Player[nPlayerIndex].AddExp(nDValue, nTarLevel);
		else
			Player[nPlayerIndex].AddSelfExp(nDValue, nTarLevel);
	}
	return 0;
}

int LuaAddOwnExp(Lua_State * L)
{
	if (Lua_GetTopIndex(L) <=0 ) return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nExp = (int)Lua_ValueToNumber(L,1);
		if (nExp > 0)
			Player[nPlayerIndex].DirectAddExp(nExp);
	}
	return 0;
}

int LuaGetPlayerLevel(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_Level);
	} 
	else
		Lua_PushNil(L);
	return 1;
}

int LuaGetPlayerLife(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		{
			int nType = (int)Lua_ValueToNumber(L,1);
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentLife);break;
			case 1:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_LifeMax); break;
			case 2:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_LifeMax);break;
			default:
				Lua_PushNil(L);
			}
		}
		
	} 
	else
		Lua_PushNil(L);
	return 1;
}

int LuaRestorePlayerLife(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].RestoreLife();
	}
	return 0;
}

int LuaRestorePlayerMana(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].RestoreMana();
	}
	return 0;
}

int LuaRestorePlayerStamina(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].RestoreStamina();
	}
	return 0;
}

int LuaGetPlayerLifeReplenish(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt( L, m_LifeReplenish);
}


int LuaGetPlayerMana(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		{
			int nType = (int)Lua_ValueToNumber(L,1);
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentMana);break;
			case 1:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_ManaMax); break;
			case 2:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_ManaMax);break;
			default:
				Lua_PushNil(L);
			}
		}
		
	} 
	else
		Lua_PushNil(L);
	return 1;
}

int LuaGetPlayerStamina(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		{
			int nType = (int)Lua_ValueToNumber(L,1);
			switch((int)Lua_ValueToNumber(L,1))
			{
			case 0:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_CurrentStamina);break;
			case 1:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_StaminaMax); break;
			case 2:
				Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_StaminaMax);break;
			default:
				Lua_PushNil(L);
			}
		}
		
	} 
	else
		Lua_PushNil(L);
	return 1;
}


int LuaGetPlayerManaReplenish(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L , m_ManaReplenish);
}

int LuaGetPlayerDefend(Lua_State * L)
{
    MacroFun_GetPlayerInfoInt(L , m_Defend);
}

int LuaGetPlayerSex(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L , m_nSex);
}

int LuaGetPlayerIndex(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L , GetPlayerIdx());
	return 0;
}

int LuaGetPlayerSeries(Lua_State * L)
{
	MacroFun_GetPlayerInfoInt(L , m_Series);
}

int LuaSetPlayerSeries(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nValue = (int)Lua_ValueToNumber(L,1);
		Npc[Player[nPlayerIndex].m_nIndex].SetSeries(nValue);
	} 
	return 0;
    
}

int LuaGetPlayerCount(Lua_State * L)
{
	Lua_PushNumber(L, PlayerSet.GetPlayerNumber());
	return 1;
}

int LuaGetPlayerName(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushString(L, Player[nPlayerIndex].m_PlayerName);
	}
	else
		Lua_PushNil(L);
	
	return 1;
	
}


int LuaGetPlayerID(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_dwID);
	}
	else
		Lua_PushNumber(L, 0);
	
	return 1;
}

int LuaGetPlayerLeadExp(Lua_State * L)
{
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_dwLeadExp);
	}
	else
		Lua_PushNil(L);
	
	return 1;
}

int LuaGetPlayerLeadLevel(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_dwLeadLevel);
	}
	else
		Lua_PushNil(L);
	
	return 1;
}



int LuaGetPlayerRestAttributePoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_nAttributePoint);
	}
	else
		Lua_PushNil(L);
	
	return 1;
}

int LuaGetPlayerRestSkillPoint(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
        Lua_PushNumber(L, Player[nPlayerIndex].m_nSkillPoint);
	}
	else
		Lua_PushNil(L);
	
	return 1;
}

/*
int LuaModifyPlayerRestSkillPoint(Lua_State *L)
{
//Question
int nPlayerIndex = GetPlayerIndex(L);
if (nPlayerIndex > 0)
{
int nDValue = (int)Lua_ValueToNumber(L, 1);

  int nNewSkillPoint = Player[nPlayerIndex].m_nSkillPoint + nDValue;
  if (nNewSkillPoint < 0 ) return 0;
  
	Player[nPlayerIndex].m_nSkillPoint = nNewSkillPoint;
	}
	
	  return 0;
	  }
*/

//»ù±¾ÊôÐÔ
int LuaGetPlayerLucky(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nCurLucky);break;
		case 1:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nLucky);break;
		case 2:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nLucky);break;
		default:
			Lua_PushNil(L);
		}
		
	} 
	return 1;
}

int LuaGetPlayerEngergy(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nCurEngergy);break;
		case 1:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nEngergy);break;
		case 2:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nEngergy);break;
		default:
			Lua_PushNil(L);
		}
	}
	
	return 1;
}

int LuaGetPlayerDexterity(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nCurDexterity);break;
		case 1:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nDexterity);break;
		case 2:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nDexterity);break;
		default:
			Lua_PushNil(L);
		}
		
	} 
	return 1;
}

int LuaGetPlayerStrength(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nCurStrength);break;
		case 1:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nStrength);break;
		case 2:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nStrength);break;
		default:
			Lua_PushNil(L);
		}
		
	} 
	return 1;
	
}

int LuaGetPlayerVitality(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0)
	{
		
		int nType = (int)Lua_ValueToNumber(L,1);
		switch((int)Lua_ValueToNumber(L,1))
		{
		case 0:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nCurVitality);break;
		case 1:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nVitality);break;
		case 2:
			Lua_PushNumber(L, Player[nPlayerIndex].m_nVitality);break;
		default:
			Lua_PushNil(L);
		}
		
	} 
	return 1;
	
}

int IncProp(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 2)
		return 0;
    int nPlayerIndex = GetPlayerIndex(L);
    if (nPlayerIndex > 0)
	{
		int nType = (int)Lua_ValueToNumber(L, 1);
		int nAddNo = (int)Lua_ValueToNumber(L, 2);
		if(nAddNo <= 0)
			return 0;
		if(Player[nPlayerIndex].m_nAttributePoint <= 0)
			return 0;
		if(nAddNo > Player[nPlayerIndex].m_nAttributePoint)
			nAddNo = Player[nPlayerIndex].m_nAttributePoint;
		PLAYER_ADD_BASE_ATTRIBUTE_COMMAND sAdd;
		sAdd.m_nAddNo = nAddNo;
		switch(nType)
		{
			case 1:
				sAdd.m_btAttribute = ATTRIBUTE_STRENGTH;
			break;
			case 2:
				sAdd.m_btAttribute = ATTRIBUTE_DEXTERITY;
			break;
			case 3:
				sAdd.m_btAttribute = ATTRIBUTE_VITALITY;
			break;
			case 4:
				sAdd.m_btAttribute = ATTRIBUTE_ENGERGY;
			break;
			default:
				sAdd.m_btAttribute = ATTRIBUTE_STRENGTH;
			break;
		}
		Player[nPlayerIndex].AddBaseAttribute((BYTE*)&sAdd);
	}
	return 0;
}

int LuaGetPlayerCashMoney(Lua_State * L)
{
    int nPlayerIndex = GetPlayerIndex(L);
	
    if (nPlayerIndex > 0)
	{
		Lua_PushNumber(L, Player[nPlayerIndex].m_ItemList.GetMoney(room_equipment));
	}
	else Lua_PushNumber(L,0);
	
	return 1;
}

int LuaPlayerPayMoney(Lua_State * L)
{
    
    int nPlayerIndex = GetPlayerIndex(L);
    if (nPlayerIndex > 0)
	{
        int nMoney = (int)Lua_ValueToNumber(L, 1);
        if (nMoney <= 0) return 0;
        if (Player[nPlayerIndex].Pay(nMoney))
			Lua_PushNumber(L, 1);
		else
			Lua_PushNumber(L, 0);
	}
	else
		Lua_PushNumber(L, 0);
	
	return 1;
}

int LuaPlayerEarnMoney (Lua_State  *L)
{
    int nPlayerIndex = GetPlayerIndex(L);
    if (nPlayerIndex > 0)
	{
        int nMoney = (int)Lua_ValueToNumber(L, 1);
        if (nMoney <= 0) return 0;
        Player[nPlayerIndex].Earn(nMoney);
	}
	return 0;	
}

int LuaPlayerPrePayMoney(Lua_State *L)
{   
    int nPlayerIndex = GetPlayerIndex(L);
    if (nPlayerIndex > 0)
	{
        int nMoney = (int)Lua_ValueToNumber(L, 1);
        if (nMoney <= 0) return 0;
        if (Player[nPlayerIndex].PrePay(nMoney))
			Lua_PushNumber(L, 1);
		else
			Lua_PushNumber(L, 0);
	}
	else
		Lua_PushNumber(L, 0);
	
	return 1;
}


int LuaPlayerExecuteScript(Lua_State * L)
{
	
	return 1;
}

//Attack dwID, Damage
int LuaAttackNpc(Lua_State * L)
{
	int nParamCount = 0;
	if ( (nParamCount = Lua_GetTopIndex(L)) < 2) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	DWORD nNpcID = (DWORD)Lua_ValueToNumber(L,1);
	int nNpcIndex = Player[nPlayerIndex].FindAroundNpc(nNpcID);//NpcSet.SearchID(nNpcID);
	if (nNpcIndex <= 0) return 0;
	
	KMagicAttrib DamageMagicAttribs[MAX_MISSLE_DAMAGEATTRIB];
	memset(DamageMagicAttribs, 0, sizeof(DamageMagicAttribs));
	
	DamageMagicAttribs[0].nAttribType = magic_attackrating_v;
	DamageMagicAttribs[0].nValue[0] = 65534;
	
	DamageMagicAttribs[1].nAttribType = magic_ignoredefense_p;
	DamageMagicAttribs[1].nValue[0] = 1;
	
	for (int i = 0; i < nParamCount - 1; i++)
	{
		int nVlau = (int)Lua_ValueToNumber(L, 2 + i);
		DamageMagicAttribs[i + 2].nValue[0] = (int)Lua_ValueToNumber(L, 2 + i);
		DamageMagicAttribs[i + 2].nValue[2] = (int)Lua_ValueToNumber(L, 2 + i);
	}
	
	Npc[nNpcIndex].ReceiveDamage(Player[nPlayerIndex].m_nIndex, 0, DamageMagicAttribs, 0, TRUE);
	return 0;
}

int LuaSetPlayerChatForbiddenFlag(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0)
		return 0;
	int nFlag = Lua_ValueToNumber(L, 1);
	Player[nPlayerIndex].SetChatForbiddenFlag(nFlag);
	return 0;
}


int LuaKickOutPlayer(Lua_State *L)
{
	if (Lua_GetTopIndex(L) < 1)
		return 0;

	const char*	pszName = (const char*)Lua_ValueToString(L, 1);
	
	int nIndex = PlayerSet.GetFirstPlayer();
	while(nIndex > 0)
	{
		if (strcmp(Player[nIndex].m_PlayerName, pszName) == 0)
			break;
		
		nIndex = PlayerSet.GetNextPlayer();
	}
	if (nIndex && Player[nIndex].m_nNetConnectIdx >= 0)
	{
		printf("GM Kick out specific player.\n");
		g_pServer->ShutdownClient(Player[nIndex].m_nNetConnectIdx);
	}
	return 0;
}

int LuaKickOutAccount(Lua_State *L)
{
	if (Lua_GetTopIndex(L) < 1)
		return 0;

	const char*	pszName = (const char*)Lua_ValueToString(L, 1);
	
	int nIndex = PlayerSet.GetFirstPlayer();
	while(nIndex > 0)
	{
		if (strcmpi(Player[nIndex].m_AccoutName, pszName) == 0)
			break;
		
		nIndex = PlayerSet.GetNextPlayer();
	}
	if (nIndex && Player[nIndex].m_nNetConnectIdx >= 0)
	{
		printf("GM Kick out specific player.\n");
		g_pServer->ShutdownClient(Player[nIndex].m_nNetConnectIdx);
	}
	return 0;
}

int LuaKickOutSelf(Lua_State *L)
{
	int nIndex = GetPlayerIndex(L);
	
	if (nIndex <= 0)
		return 0;
	
	if (Player[nIndex].m_nNetConnectIdx >= 0)
	{
		printf("GM Kick out player one.\n");
		g_pServer->ShutdownClient(Player[nIndex].m_nNetConnectIdx);
	}
	return 0;
}

int LuaKillNpc(Lua_State * L)
{
	int nParamCount = 0;
	if ( (nParamCount = Lua_GetTopIndex(L)) < 1) return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	DWORD nNpcID = (DWORD)Lua_ValueToNumber(L,1);
	int nNpcIndex = Player[nPlayerIndex].FindAroundNpc(nNpcID);
	if (nNpcIndex <= 0) return 0;
	
	KMagicAttrib DamageMagicAttribs[MAX_MISSLE_DAMAGEATTRIB];
	memset(DamageMagicAttribs, 0, sizeof(DamageMagicAttribs));
	
	DamageMagicAttribs[0].nAttribType = magic_attackrating_v;
	DamageMagicAttribs[0].nValue[0] = 50000;
	
	
	
	DamageMagicAttribs[1].nAttribType = magic_ignoredefense_p;
	DamageMagicAttribs[1].nValue[0] = 1;
	
	
	DamageMagicAttribs[2].nValue[0] = 50000;
	DamageMagicAttribs[2].nValue[2] = 50000;
	
	Npc[nNpcIndex].ReceiveDamage(Player[nPlayerIndex].m_nIndex, 0, DamageMagicAttribs, 0, TRUE);
	return 0;
}

int LuaKillPlayer(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) 
		return 0;
	KMagicAttrib DamageMagicAttribs[MAX_MISSLE_DAMAGEATTRIB];
	memset(DamageMagicAttribs, 0, sizeof(DamageMagicAttribs));
	
	DamageMagicAttribs[0].nAttribType = magic_attackrating_v;
	DamageMagicAttribs[0].nValue[0] = 50000;
	
	DamageMagicAttribs[1].nAttribType = magic_ignoredefense_p;
	DamageMagicAttribs[1].nValue[0] = 1;
	
	
	DamageMagicAttribs[2].nValue[0] = 50000;
	DamageMagicAttribs[2].nValue[2] = 50000;
	
	Npc[Player[nPlayerIndex].m_nIndex].ReceiveDamage(Player[nPlayerIndex].m_nIndex, 0, DamageMagicAttribs, 0, TRUE);
	return 0;
}

int AddSkillState(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 4)
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	int nSkillId = (int)Lua_ValueToNumber(L, 1);
	int nLevel = (int)Lua_ValueToNumber(L, 2);
	int nSave = (int)Lua_ValueToNumber(L, 3);
	int nTime = (int)Lua_ValueToNumber(L, 4);
	KSkill * pSkill = (KSkill *) g_SkillManager.GetSkill(nSkillId, nLevel);
	if(!pSkill)
		return 0;
	int nImmediate = pSkill->GetImmediateNum();
	int nState = pSkill->GetStateNum();
	if(nImmediate > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].SetStateSkillEffect(Player[nPlayerIndex].m_nIndex,
			nSkillId, nLevel, pSkill->GetImmediateAttribs(), nImmediate, nTime);
	}
	else if(nState > 0)
	{
		Npc[Player[nPlayerIndex].m_nIndex].SetStateSkillEffect(Player[nPlayerIndex].m_nIndex,
			nSkillId, nLevel, pSkill->GetStateAttribs(), nState, nTime);
	}
	return 0;
}

int LuaSetFightState(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Npc[Player[nPlayerIndex].m_nIndex].SetFightMode(Lua_ValueToNumber(L,1) != 0);
	return 0;
}

int LuaGetFightState(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_FightMode);
	return 1;
}

int LuaGetLevel(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)		Lua_PushNumber(L,0);
	
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Lua_PushNumber(L, Npc[Player[nPlayerIndex].m_nIndex].m_Level);
	return 1;
}
//
int	LuaUseTownPortal(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	
	Player[nPlayerIndex].UseTownPortal();
	return 0;
}

int LuaReturnFromTownPortal(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	
	Player[nPlayerIndex].BackToTownPortal();
	return 0;
}

int GetNpcData(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 4)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	int nNpcIdx = (int)Lua_ValueToNumber(L, 1);
	if(nNpcIdx <= 0 || nNpcIdx >= MAX_NPC || !Npc[nNpcIdx].m_dwID || Npc[nNpcIdx].m_Kind != kind_normal)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	char* pDataName = (char *)Lua_ValueToString(L, 2);
	int nLevel = (int)Lua_ValueToNumber(L, 3);
	int nSeries = (int)Lua_ValueToNumber(L, 4);
	if(nLevel <= 0)
		nLevel = 1;
	else if(nLevel >= MAX_NPC_LEVEL)
		nLevel = MAX_NPC_LEVEL-1;
	if(nSeries < 0 || nSeries >= series_num)
		nSeries = 0;
	Lua_PushNumber(L, Npc[nNpcIdx].GetDataFromScript(pDataName, nLevel, nSeries));
	return 1;
}

int GetNpcLvScript(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 1)
	{
		Lua_PushString(L, "");
		return 1;
	}
	int nNpcIdx = (int)Lua_ValueToNumber(L, 1);
	if(nNpcIdx <= 0 || nNpcIdx >= MAX_NPC || !Npc[nNpcIdx].m_dwID || Npc[nNpcIdx].m_Kind != kind_normal)
	{
		Lua_PushString(L, "");
		return 1;
	}
	int nNpcTemplateId = Npc[nNpcIdx].m_NpcSettingIdx;
	if (!g_pNpcTemplate[nNpcTemplateId][0])
	{
		Lua_PushString(L, "");
		return 1;
	}
	KLuaScript * pLevelScript = NULL;		
			pLevelScript = (KLuaScript*)g_GetScript(
			g_pNpcTemplate[nNpcTemplateId][0]->m_dwLevelSettingScript
			);
		
	if (pLevelScript == NULL)
		pLevelScript = g_pNpcLevelScript;
	if(!pLevelScript)
	{
		Lua_PushString(L, "");
		return 1;
	}
	Lua_PushString(L, pLevelScript->m_szScriptName);
	return 1;
}

int GetTongName(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if(nPlayerIndex <= 0)
	{
		Lua_PushString(L, "");
		return 1;
	}
	char szGet[32];
	Player[nPlayerIndex].m_cTong.GetTongName(&szGet[0]);
	Lua_PushString(L, szGet);
	return 1;
}

int GetTongID(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if(nPlayerIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	Lua_PushNumber(L, Player[nPlayerIndex].m_cTong.GetTongNameID());
	return 1;
}

int TCreate(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 3)
		return 0;
	char* szTName = (char*)Lua_ValueToString(L, 1);
	char* szMaster = (char*)Lua_ValueToString(L, 2);
	STONG_CREATE_COMMAND	sCreate;
	sCreate.ProtocolFamily = pf_tong;
	sCreate.ProtocolID = enumC2S_TONG_CREATE;
	sCreate.m_btCamp = (BYTE)Lua_ValueToNumber(L, 3);
	if (sCreate.m_btCamp != camp_justice && sCreate.m_btCamp != camp_evil && sCreate.m_btCamp != camp_balance)
		sCreate.m_btCamp = camp_justice;
	sCreate.m_dwPlayerNameID = g_FileName2Id(szMaster);
	sCreate.m_nSex = 0;
	int nPlayerIdx = PlayerSet.FindSame(sCreate.m_dwPlayerNameID);
	if(nPlayerIdx > 0)
	{
		if(Player[nPlayerIdx].m_cTong.CheckIn())
			return 0;
		sCreate.m_nSex = Npc[Player[nPlayerIdx].m_nIndex].m_nSex;
	}
	sCreate.m_btPlayerNameLength = strlen(szMaster);
	sCreate.m_btTongNameLength = strlen(szTName);
	if(sCreate.m_btTongNameLength == 0 || sCreate.m_btTongNameLength >= defTONG_NAME_MAX_LENGTH
	|| sCreate.m_btPlayerNameLength == 0)
		return 0;
	memcpy(sCreate.m_szBuffer, szTName, sCreate.m_btTongNameLength);
	memcpy(&sCreate.m_szBuffer[sCreate.m_btTongNameLength], szMaster, sCreate.m_btPlayerNameLength);
	sCreate.m_wLength = sizeof(STONG_CREATE_COMMAND) - sizeof(sCreate.m_szBuffer) + sCreate.m_btTongNameLength + sCreate.m_btPlayerNameLength;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCreate, sCreate.m_wLength);
	return 0;
}

int TAddMember(Lua_State * L)
{
	int nParamNum = Lua_GetTopIndex(L);
	if(nParamNum < 2)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	char* szMem = (char*)Lua_ValueToString(L, 2);
	UINT dwNameID = g_FileName2Id(szMem);
	int nPlayerIdx = PlayerSet.FindSame(dwNameID);
	STONG_ADD_MEMBER_COMMAND	sTong;
	sTong.m_nSex = 0;
	if(nPlayerIdx > 0)
	{
		if(Player[nPlayerIdx].m_cTong.CheckIn())
			return 0;
		sTong.m_nSex = Npc[Player[nPlayerIdx].m_nIndex].m_nSex;
	}
	sTong.ProtocolFamily = pf_tong;
	sTong.ProtocolID = enumC2S_TONG_ADD_MEMBER;
	sTong.m_dwTongNameID = uTongID;
	sTong.m_dwPlayerNameID = dwNameID;
	sTong.m_uSenderID = 0;
	sTong.m_nSenderPIdx = 0;
	sTong.m_btFigure = enumTONG_FIGURE_MEMBER;
	if(nParamNum > 2)
	{
		sTong.m_btFigure = (BYTE)Lua_ValueToNumber(L, 3);
		if(sTong.m_btFigure >= enumTONG_FIGURE_MASTER)
			sTong.m_btFigure = enumTONG_FIGURE_MEMBER;
	}
	sTong.m_btPlayerNameLength = strlen(szMem);
	memcpy(&sTong.m_szBuffer[0], szMem, sTong.m_btPlayerNameLength);
	sTong.m_wLength = sizeof(STONG_ADD_MEMBER_COMMAND) - sizeof(sTong.m_szBuffer) + sTong.m_btPlayerNameLength;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sTong, sTong.m_wLength);
	return 0;
}

int TGetMaster(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
	{
		Lua_PushString(L,"");
		return 1;
	}
	Lua_PushString(L, Player[nPlayerIndex].m_cTong.m_szMasterName);
	return 1;
}

int TAddMoney(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 2)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	int nAddValue = (int)Lua_ValueToNumber(L, 2);
	if(nAddValue > 0)
	{
		STONG_CONTRIBMONEY_COMMAND	sCmd;
		sCmd.ProtocolFamily	= pf_tong;
		sCmd.ProtocolID		= enumC2S_TONG_CONTRIBMONEY;
		sCmd.m_nPlayerIdx		= 0;
		sCmd.m_dwPlayerNameID = 0;
		sCmd.m_dwTongNameID	= uTongID;
		sCmd.m_nMoney = nAddValue;
		g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	}
	else if(nAddValue < 0)
	{
		STONG_CONTRIBMONEY_COMMAND	sCmd;
		sCmd.ProtocolFamily	= pf_tong;
		sCmd.ProtocolID		= enumC2S_TONG_WITHDRAWMONEY;
		sCmd.m_nPlayerIdx		= 0;
		sCmd.m_dwPlayerNameID = 0;
		sCmd.m_dwTongNameID	= uTongID;
		sCmd.m_nMoney = -nAddValue;
		g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	}
	return 0;
}

int TAddStoreOffer(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 2)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	int nAddValue = (int)Lua_ValueToNumber(L, 2);
	if(nAddValue > 0)
	{
		STONG_CONTRIBMONEY_COMMAND	sCmd;
		sCmd.ProtocolFamily	= pf_tong;
		sCmd.ProtocolID		= enumC2S_TONG_STOREOFFER;
		sCmd.m_nPlayerIdx		= 0;
		sCmd.m_dwPlayerNameID = 0;
		sCmd.m_dwTongNameID	= uTongID;
		sCmd.m_nMoney = nAddValue;
		g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	}
	else if(nAddValue < 0)
	{
		STONG_DISPENSEOFFER_COMMAND	sCmd;
		sCmd.ProtocolFamily		= pf_tong;
		sCmd.ProtocolID			= enumC2S_TONG_DISPENSEOFFER;
		sCmd.m_nPlayerIdx		= 0;
		sCmd.m_dwPlayerNameID	= 0;
		sCmd.m_dwTongNameID		= uTongID;
		sCmd.m_nMoney			= -nAddValue;
		sCmd.m_nDestPIdx		= 0;
		sCmd.m_uDestID			= 0;
		g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	}
	return 0;
}

int TAddBuildFund(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 2)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	int nAddValue = (int)Lua_ValueToNumber(L, 2);
	if(nAddValue == 0)
		return 0;
	STONG_CONTRIBMONEY_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_STOREBUILDFUND;
	sCmd.m_nPlayerIdx		= 0;
	sCmd.m_dwPlayerNameID = 0;
	sCmd.m_dwTongNameID	= uTongID;
	sCmd.m_nMoney = nAddValue;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	return 0;
}

int TResetTask(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 1)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	STONG_RESETTASK_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_RESETTASK;
	sCmd.m_dwTongNameID	= uTongID;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	
	return 0;
}

int GetWeekGoal(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	Lua_PushNumber(L, Player[nPlayerIndex].m_cTong.GetWeekGoalType());
	return 1;
}

int GetWeekOffer(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	Lua_PushNumber(L, Player[nPlayerIndex].m_cTong.GetWeekOffer());
	return 1;
}

int SetWeekOffer(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	int nValue = (int)Lua_ValueToNumber(L,1);
	Player[nPlayerIndex].m_cTong.SetWeekOffer(nValue);
	return 0;
}

int TAddWeekGoal(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 3)
		return 0;
	UINT uTongID;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
		return 0;
	UINT uPlayerID = (UINT)Lua_ValueToNumber(L, 2);
	int nValue = (int)Lua_ValueToNumber(L, 3);
	if(nValue == 0)
		return 0;
	STONG_ADDWEEKGOAL_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_ADDWEEKGOAL;
	sCmd.m_dwTongNameID	= uTongID;
	sCmd.m_dwPlayerNameID	= uPlayerID;
	sCmd.m_nValue	= nValue;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	
	return 0;
}

int TSetWeekGoalLevel(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 2)
	{
		Lua_PushNumber(L, -1);
		return 1;
	}
	int nPlayerIndex = GetPlayerIndex(L);
	UINT uTongID;
	UINT uPlayerID = 0;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
	{
		Lua_PushNumber(L, -1);
		return 1;
	}
	if(nPlayerIndex > 0)
	{
		int nCurTime = g_SubWorldSet.GetGameTime();
		if(Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage >= nCurTime)
		{
			Lua_PushNumber(L, -1);
			return 1;
		}
		Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
		if(Player[nPlayerIndex].m_cTong.GetTongNameID() != uTongID)
		{
			Lua_PushNumber(L, -1);
			return 1;
		}
		UINT uRight = Player[nPlayerIndex].m_cTong.GetRight();
		if(!(uRight & defRIGHT_WEEKGOAL))
		{
			Lua_PushNumber(L, 0);
			return 1;
		}
		uPlayerID = Player[nPlayerIndex].m_dwID;
	}
	int nValue = (int)Lua_ValueToNumber(L, 2);
	if(nValue < 0)
		nValue = 0;
	else if(nValue > 2)
		nValue = 2;
	STONG_ADDWEEKGOAL_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_TASKLEVEL;
	sCmd.m_dwTongNameID	= uTongID;
	sCmd.m_dwPlayerNameID	= uPlayerID;
	sCmd.m_nValue	= nValue;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	
	Lua_PushNumber(L, 1);
	return 1;
}

int TWeekGoalPrice(Lua_State * L)
{
	if(Lua_GetTopIndex(L) < 1)
	{
		Lua_PushNumber(L, -1);
		return 1;
	}
	int nPlayerIndex = GetPlayerIndex(L);
	UINT uTongID;
	UINT uPlayerID = 0;
	if(Lua_IsNumber(L,1))
	{
		uTongID = (UINT)Lua_ValueToNumber(L, 1);
	}
	else
	{
		char* szTName = (char*)Lua_ValueToString(L, 1);
		uTongID = g_FileName2Id(szTName);
	}
	if(uTongID == 0)
	{
		Lua_PushNumber(L, -1);
		return 1;
	}
	if(nPlayerIndex > 0)
	{
		int nCurTime = g_SubWorldSet.GetGameTime();
		if(Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage >= nCurTime)
		{
			Lua_PushNumber(L, -1);
			return 1;
		}
		Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
		if(Player[nPlayerIndex].m_cTong.GetTongNameID() != uTongID)
		{
			Lua_PushNumber(L, -1);
			return 1;
		}
		UINT uRight = Player[nPlayerIndex].m_cTong.GetRight();
		if(!(uRight & defRIGHT_WEEKGOAL))
		{
			Lua_PushNumber(L, 0);
			return 1;
		}
		uPlayerID = Player[nPlayerIndex].m_dwID;
	}
	STONG_ADDWEEKGOAL_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_RECVPRICE;
	sCmd.m_dwTongNameID	= uTongID;
	sCmd.m_dwPlayerNameID	= uPlayerID;
	sCmd.m_nValue	= uPlayerID?nPlayerIndex:0;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	
	Lua_PushNumber(L, 1);
	return 1;
}

int TPlayerPrice(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if(nPlayerIndex <= 0)
		return 0;
	UINT uTongID = Player[nPlayerIndex].m_cTong.GetTongNameID();
	if(uTongID == 0)
		return 0;
	int nCurTime = g_SubWorldSet.GetGameTime();
	if(Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage >= nCurTime)
		return 0;
	Player[nPlayerIndex].m_cTong.m_nNextTimeMemPage = nCurTime + 3;

	STONG_ADDWEEKGOAL_COMMAND	sCmd;
	sCmd.ProtocolFamily	= pf_tong;
	sCmd.ProtocolID		= enumC2S_TONG_PLAYERPRICE;
	sCmd.m_dwTongNameID	= uTongID;
	sCmd.m_dwPlayerNameID	= Player[nPlayerIndex].m_dwID;
	sCmd.m_nValue	= nPlayerIndex;
	g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
	
	return 0;
}

#endif

int FileName2Id(Lua_State * L)
{
	if(Lua_GetTopIndex(L) <= 0)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	UINT uRet = g_FileName2Id((char*)Lua_ValueToString(L, 1));
	Lua_PushNumber(L, uRet);
	return 1;
}

int GetPlayerIndex(Lua_State * L)
{
	Lua_GetGlobal(L, SCRIPT_PLAYERINDEX);
    if (lua_isnil(L,Lua_GetTopIndex(L)))
        return -1;
	int nIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
    if (nIndex >= MAX_PLAYER || nIndex <= 0) 
	{
		_ASSERT(0);
		return -1;
	}	if (Player[nIndex].m_nIndex >= MAX_NPC || Player[nIndex].m_nIndex <= 0)
	{
		_ASSERT(0);
		return -1;
	}
    return nIndex;
}

int GetObjIndex(Lua_State * L)
{
	Lua_GetGlobal(L, SCRIPT_OBJINDEX);
    if (lua_isnil(L,Lua_GetTopIndex(L)))
        return -1;
	int nIndex = (int)Lua_ValueToNumber(L, Lua_GetTopIndex(L));
    if (nIndex >= MAX_OBJECT || nIndex <= 0) 
	{
		_ASSERT(0);
		return -1;
	}
	if (Object[nIndex].m_nIndex != nIndex)
	{
		_ASSERT(0);
		return -1;
	}
    return nIndex;
}




int  LuaMessage(Lua_State * L)
{
	const char * szString;
	szString  = lua_tostring (L,1);
	g_DebugLog((char *)szString);
	return 0;
}
#ifdef _SERVER
//AddStation(N)
int LuaAddPlayerWayPoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	int nWayPoint = (int)Lua_ValueToNumber(L,1);
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	{
		KIndexNode * pNode = (KIndexNode*) Player[nPlayerIndex].m_PlayerWayPointList.GetHead();
		while(pNode)
		{
			if (pNode->m_nIndex == nWayPoint) return 0;
			pNode = (KIndexNode*)pNode->GetNext();
		}
		
		KIndexNode * pNewNode = new KIndexNode;
		pNewNode->m_nIndex = nWayPoint;
		int nCount = Player[nPlayerIndex].m_PlayerWayPointList.GetNodeCount();
		for (int i = 0; i < nCount - 2; i ++ )
		{
			KIndexNode *  pDelNode = (KIndexNode*)Player[nPlayerIndex].m_PlayerWayPointList.RemoveHead();
			delete pDelNode;
		}
		Player[nPlayerIndex].m_PlayerWayPointList.AddTail(pNewNode);
	}
	return 0;
}

int LuaAddPlayerStation(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	int nStation = (int )Lua_ValueToNumber(L,1);
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	{
		KIndexNode * pNode = (KIndexNode*) Player[nPlayerIndex].m_PlayerStationList.GetHead();
		while(pNode)
		{
			if (pNode->m_nIndex == nStation) return 0;
			pNode = (KIndexNode*)pNode->GetNext();
		}
		
		KIndexNode * pNewNode = new KIndexNode;
		pNewNode->m_nIndex = nStation;
		Player[nPlayerIndex].m_PlayerStationList.AddTail(pNewNode);
	}
	return 0;
}

int LuaGetPlayerStationCount(Lua_State *L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Lua_PushNumber(L, Player[nPlayerIndex].m_PlayerStationList.GetNodeCount());
	return 1;
}

//»ñµÃµ±Ç°Íæ¼ÒÓÐÐ§³ÇÊÐÁÐ±íÖÐµÄµÚn¸ö£¨³ýÈ¥µ±Ç°ËùÔÚ³ÇÊÐ£©
int LuaGetPlayerStation(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	
	if (Lua_GetTopIndex(L) < 2)
	{
		Lua_PushNumber(L, 0);
		return 1;
	}
	
	int nStationId = 0;
	KIndexNode * pNode = 	(KIndexNode*)Player[nPlayerIndex].m_PlayerStationList.GetHead();
	if (pNode)
	{
		int nNo = (int )Lua_ValueToNumber(L, 1);
		int nCurStation = (int) Lua_ValueToNumber(L,2);
		int nVisitNo = 0;
		while(pNode)
		{
			if (pNode->m_nIndex != nCurStation && g_GetPriceToStation( nCurStation , pNode->m_nIndex) > 0) 
			{
				nVisitNo ++;
				if (nVisitNo == nNo)
				{
					nStationId = pNode->m_nIndex;
					break;
				}
			}
			pNode = (KIndexNode*)pNode->GetNext();
		}
	}
	
	Lua_PushNumber(L, nStationId);
	
	return 1;
}

int LuaGetPlayerWayPoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	
	KIndexNode * pNode = (KIndexNode*)	Player[nPlayerIndex].m_PlayerWayPointList.GetHead();
	if (pNode)
	{
		int nNo = (int)Lua_ValueToNumber(L, 1);
		if (nNo > TASKVALUE_MAXWAYPOINT_COUNT) 
			Lua_PushNumber(L, 0);
		else
		{
			for (int i = 0; i < nNo - 1; i ++)
			{
				if (pNode == NULL ) break;
				pNode = (KIndexNode *)pNode->GetNext();
			}
			
			if (pNode)
				Lua_PushNumber(L, pNode->m_nIndex);
			else
				Lua_PushNumber(L, 0);
			
		}
	}
	else
		Lua_PushNumber(L, 0);
	
	return 1;
}
//¸ù¾Ý´«ÈëµÄ³ÇÊÐÎ¨Ò»id»ñµÃÆäÃû³Æ
int LuaGetStationName(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	if (Lua_GetTopIndex(L) <= 0 ) 
	{
		Lua_PushString(L, "");
		return 1;
	}
	int nStationId = (int)Lua_ValueToNumber(L,1);
	char szName[50];
	g_StationTabFile.GetString(nStationId + 1, "DESC", "ÎÞÃû³Ç",  szName, 50 );
	Lua_PushString(L, szName);
	return 1;
}

int LuaGetWayPointName(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	if (Lua_GetTopIndex(L) <= 0 ) 
	{
		Lua_PushString(L, "");
		return 1;
	}
	int nWayPointId = (int)Lua_ValueToNumber(L,1);
	char szName[50];
	g_WayPointTabFile.GetString(nWayPointId + 1, "DESC", "Î´¼ÇÂ¼",  szName, 50 );
	Lua_PushString(L, szName);
	return 1;
}

//GetCityCount 
int LuaGetAllStationCount(Lua_State * L)
{
	int nCityCount = g_StationTabFile.GetHeight() - 1;
	if (nCityCount < 0) nCityCount = 0;
	Lua_PushNumber(L,nCityCount);
	return 1;
}

//cityid, price = GetCity(citynum, curcity)
int LuaGetCity(Lua_State * L)
{
	return 0;
}


int LuaGetPriceToWayPoint(Lua_State *L)
{
	
	int nCurStation = (int)Lua_ValueToNumber(L,1);
	int nDesWayPoint = (int)Lua_ValueToNumber(L,2);
	Lua_PushNumber(L, g_GetPriceToWayPoint(nCurStation, nDesWayPoint));
	return 1;
}

int LuaGetPriceToStation(Lua_State *L)
{
	int nCurStation	 = (int)Lua_ValueToNumber(L,1);
	int nNextStation = (int)Lua_ValueToNumber(L,2);
	Lua_PushNumber(L, g_GetPriceToStation(nCurStation, nNextStation));
	return 1;
}

int LuaGetStationPos(Lua_State * L)
{
	int nStationId = (int)Lua_ValueToNumber(L,1);
	char szPos[100] ;
	int nCount = 0;
	int nRow = g_StationTabFile.FindColumn("COUNT");
	g_StationTabFile.GetInteger(nStationId + 1, nRow,  0, &nCount);
	if (nCount <= 0) return 0;
	int nRandSect = g_Random(100) % nCount + 1;
	char szSectName[32];
	sprintf(szSectName, "SECT%d", nRandSect);
	char szValue[100];
	nRow = g_StationTabFile.FindColumn(szSectName);
	g_StationTabFile.GetString(nStationId + 1, nRow, "0,0,0", szValue, 100);
	
    int nX, nY, nWorld;
    const char *pcszTemp = szValue;
    
    nWorld = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nX = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nY = KSG_StringGetInt(&pcszTemp, 0);
	//sscanf(szValue, "%d,%d,%d", &nWorld, &nX, &nY);
	
    Lua_PushNumber(L,nWorld);
	Lua_PushNumber(L,nX);
	Lua_PushNumber(L,nY);
	return 3;
}

int LuaGetWayPointPos(Lua_State * L)
{
	int nWayPointId = (int)Lua_ValueToNumber(L,1);
	char szPos[100] ;
	int nCount = 0;
	char szValue[30];
	int nRow;
	nRow = g_WayPointTabFile.FindColumn("SECT");
	g_WayPointTabFile.GetString(nWayPointId + 1, nRow, "0,0,0", szValue, 30);
	int nX, nY, nWorld;
    const char *pcszTemp = szValue;
    
    nWorld = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nX = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nY = KSG_StringGetInt(&pcszTemp, 0);
	//sscanf(szValue, "%d,%d,%d", &nWorld, &nX, &nY);
	
	Lua_PushNumber(L,nWorld);
	Lua_PushNumber(L,nX);
	Lua_PushNumber(L,nY);
	return 3;
}

int LuaGetRank(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Lua_PushNumber(L,Npc[Player[nPlayerIndex].m_nIndex].m_btRankId);
	return 1;
}

int LuaSetRank(Lua_State * L)
{
	BYTE btRank = (BYTE)Lua_ValueToNumber(L,1);
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	Npc[Player[nPlayerIndex].m_nIndex].m_btRankId = btRank;
	return 0;
}

int LuaSetObjPropState(Lua_State *L)
{
	int  nParamNum = ( int ) Lua_GetTopIndex(L);
	int nState  = 1;
	
	if (nParamNum >= 1)
	{
		nState = (int)Lua_ValueToNumber(L,1);
		nState = (nState == 0)?0 : 1;
	}
	
	int nIndex = 0;
	if ((nIndex = GetObjIndex(L)) < 0) 
		return 0;
	
	Object[nIndex].SetState(nState);
	return 0;
}


int	LuaGetServerName(Lua_State * L)
{
	char szServerName[100]  ;
	unsigned long   stServerNameLen = 100;

#ifndef __linux
	if (GetComputerName(szServerName, &stServerNameLen))
	{
		Lua_PushString(L, szServerName);
	}
	else
#else
	if (SOCKET_ERROR != gethostname(szServerName, sizeof(szServerName)))
	{
		Lua_PushString(L, szServerName);
	}
	else
#endif
	Lua_PushString(L, "");

	return 1;
}

//GetWharfCount(nDock)
int LuaGetDockCount(Lua_State * L)
{
	int nCount = 0;
	int nCurStation = 0;
	int nTotalCount = 0;
	int i  = 0;
	if (Lua_GetTopIndex(L) < 1 )  
	{
		goto DockCount;
	}
	
	nCurStation = (int)Lua_ValueToNumber(L,1);
	nTotalCount = g_DockPriceTabFile.GetHeight() - 1;
	
	for (i = 0; i < nTotalCount; i ++)
	{
		int nPrice = g_GetPriceToDock(nCurStation, i + 1);
		if (nPrice > 0) nCount ++;
	}
	
DockCount:
	Lua_PushNumber(L, nCount);
	return 1;
}

int LuaGetDockPrice(Lua_State * L)
{
	int nCurDock = (int)Lua_ValueToNumber(L,1);
	int nDesDock = (int)Lua_ValueToNumber(L,2);
	Lua_PushNumber(L, g_GetPriceToDock(nCurDock, nDesDock));
	return 1;
}

int LuaGetDock(Lua_State * L)
{
	int nCurDock = (int)Lua_ValueToNumber(L, 1);
	int nDock = (int)Lua_ValueToNumber(L, 2);
	int nCount = 0;
	int nTotalCount = g_DockPriceTabFile.GetHeight() - 1;
	int nGetDock = 0;
	
	for (int i = 0; i < nTotalCount; i ++)
	{
		int nPrice = g_GetPriceToDock(nCurDock, i + 1);
		if (nPrice > 0) 
		{
			nCount ++ ;
			if (nCount == nDock)
			{
				nGetDock = i + 1;
				break;
			}
		}
	}
	Lua_PushNumber(L, nGetDock);
	return 1;
}

int LuaGetDockName(Lua_State * L)
{
	int nDock  = (int)Lua_ValueToNumber(L, 1);
	char szName[100] ;
	
	if (nDock > g_DockPriceTabFile.GetHeight() - 1)
	{
		strcpy(szName, "Î´ÖªÂëÍ·");
		goto DockName;
	}
	
	g_DockTabFile.GetString(nDock + 1, "DESC", "Î´ÖªÂëÍ·", szName, 100);
	
DockName:
	Lua_PushString (L, szName);
	return 1;
}

int LuaGetDockPos(Lua_State * L)
{
	int nDock  = (int)Lua_ValueToNumber(L, 1);
	if (nDock > g_DockTabFile.GetHeight() - 1) 
	{
		printf("GetWharfPos Script Is Error!");
		return 0;
	}
	
	char szPos[100] ;
	int nCount = 0;
	int nRow = g_DockTabFile.FindColumn("COUNT");
	g_DockTabFile.GetInteger(nDock + 1, nRow,  0, &nCount);
	if (nCount <= 0) return 0;
	int nRandSect = g_Random(100) % nCount + 1;
	char szSectName[32];
	sprintf(szSectName, "SECT%d", nRandSect);
	char szValue[100];
	nRow = g_DockTabFile.FindColumn(szSectName);
	g_DockTabFile.GetString(nDock + 1, nRow, "0,0,0", szValue, 100);
	int nX, nY, nWorld;
    const char *pcszTemp = szValue;
    
    nWorld = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nX = KSG_StringGetInt(&pcszTemp, 0);
    KSG_StringSkipSymbol(&pcszTemp, ',');
    nY = KSG_StringGetInt(&pcszTemp, 0);
	//sscanf(szValue, "%d,%d,%d", &nWorld, &nX, &nY);
	
	Lua_PushNumber(L,nWorld);
	Lua_PushNumber(L,nX);
	Lua_PushNumber(L,nY);
	return 3;
}

int LuaGetWayPointFightState(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) return 0;
	int nFightState = 0;
	if (Lua_GetTopIndex(L) > 0 ) 
	{
		int nWayPointId = (int)Lua_ValueToNumber(L,1);
		g_WayPointTabFile.GetInteger(nWayPointId + 1, "FightState", 0,  &nFightState);
	}
	Lua_PushNumber(L, nFightState);
	return 1;
}

// SetMissionValue(valueid, value)
int LuaSetMissionValue(Lua_State * L)
{
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex < 0) 
		return 0;
	
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2) 
		return 0;
	
	int nValueId = (int)Lua_ValueToNumber(L, 1);
	int nValue = (int)Lua_ValueToNumber(L, 2);
	
	if (nValueId  < 0)
		return 0;
	SubWorld[nSubWorldIndex].m_MissionArray.SetMissionValue(nValueId, nValue);
	return 0;
}

int LuaGetMissionValue(Lua_State * L)
{
	int nResultValue = 0;
	int nSubWorldIndex = -1;
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 1) 
		goto lab_getmissionvalue;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	
	if (nSubWorldIndex >= 0) 
	{
		int  nValueId = (int)Lua_ValueToNumber(L, 1);
		if (nValueId > 0)
			nResultValue = SubWorld[nSubWorldIndex].m_MissionArray.GetMissionValue(nValueId);
	}
	
lab_getmissionvalue:
	Lua_PushNumber(L, nResultValue);
	return 1;
}


// SetMissionValue(mapid/mapname, valueid, value)
int LuaSetGlobalMissionValue(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2) 
		return 0;
	
	int nValueId = (int)Lua_ValueToNumber(L, 1);
	int nValue = (int)Lua_ValueToNumber(L, 2);
	
	if (nValueId  < 0)
		return 0;
	g_GlobalMissionArray.SetMissionValue(nValueId, nValue);
	return 0;
}

int LuaGetGlobalMissionValue(Lua_State * L)
{
	int nResultValue = 0;
	int nValueId = 0;
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 1) 
		goto lab_getglobalmissionvalue;
	nValueId = (int)Lua_ValueToNumber(L, 1);
	if (nValueId < 0)
		goto lab_getglobalmissionvalue;
	
	nResultValue = g_GlobalMissionArray.GetMissionValue(nValueId);
	
lab_getglobalmissionvalue:
	Lua_PushNumber(L, nResultValue);
	return 1;
}

//StartMission(missionid)
int LuaInitMission(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nMissionId = (int)Lua_ValueToNumber(L, 1);
	if (nMissionId < 0 )
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex < 0) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) 
		return 0;
	
	KMission Mission;
	Mission.SetMissionId(nMissionId);
	KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
	if (pMission)
	{
		_ASSERT(0);
		return 0;
	}
	
	pMission = SubWorld[nSubWorldIndex].m_MissionArray.Add();
	if (pMission)
	{
		pMission->m_MissionPlayer.Clear();
		pMission->SetMissionId(nMissionId);
		char szScript[MAX_PATH];
		g_MissionTabFile.GetString(nMissionId + 1, 2, "", szScript, MAX_PATH);
		if (szScript[0])
			Player[nPlayerIndex].ExecuteScript(szScript, "InitMission", "");
	}
	
	return 0;
}

int LuaRunMission(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nMissionId = (int)Lua_ValueToNumber(L, 1);
	if (nMissionId < 0 )
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex < 0) 
		return 0;
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) 
		return 0;
	KMission Mission;
	Mission.SetMissionId(nMissionId);
	KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
	if (pMission)
	{
		char szScript[MAX_PATH];
		g_MissionTabFile.GetString(nMissionId + 1, 2, "", szScript, MAX_PATH);
		if (szScript[0])
			Player[nPlayerIndex].ExecuteScript(szScript, "RunMission", "");
	}
	
	return 0;
}
//CloseMission(missionId)
int LuaCloseMission(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nMissionId = (int)Lua_ValueToNumber(L, 1);
	if (nMissionId < 0 )
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex < 0) 
		return 0;
	KMission StopMission;
	StopMission.SetMissionId(nMissionId);
	KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&StopMission);
	if (pMission)
	{
		char szScript[MAX_PATH];
		g_MissionTabFile.GetString(nMissionId + 1, 2, "", szScript, MAX_PATH);
		if (szScript[0])
			pMission->ExecuteScript(szScript, "EndMission",0);
		pMission->StopMission();
		SubWorld[nSubWorldIndex].m_MissionArray.Remove(pMission);
		
	}
	return 0;
}
//StopMissionTimer(missionid, timerid)
int LuaStopMissionTimer(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2) 
		return 0;
	int nMissionId = (int)Lua_ValueToNumber(L, 1);
	int nTimerId = (int)Lua_ValueToNumber(L, 2);
	int nSubWorldIndex = GetSubWorldIndex(L);
	
	if (nMissionId < 0 || nTimerId < 0 ) 
		return 0;
	
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			KTimerTaskFun StopTimer;
			StopTimer.SetTimer(1, nTimerId);
			KTimerTaskFun * pTimer = pMission->m_cTimerTaskSet.GetData(&StopTimer);
			if (pTimer)
			{
				pTimer->CloseTimer();
				pMission->m_cTimerTaskSet.Remove(pTimer);
			}
		}
		
	}
	
	return 0;
}

//StartMissionTimer(missionid, timerid, time)
int LuaStartMissionTimer(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 3) 
		return 0;
	int nMissionId = (int)Lua_ValueToNumber(L, 1);
	int nTimerId = (int)Lua_ValueToNumber(L, 2);
	int nTimeInterval = (int)Lua_ValueToNumber(L, 3);
	int nSubWorldIndex = GetSubWorldIndex(L);
	
	if (nMissionId < 0 || nTimerId < 0 || nTimeInterval < 0) 
		return 0;
	
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			KTimerTaskFun * pTimer = pMission->m_cTimerTaskSet.Add();
			if (pTimer)
			{
				pTimer->SetTimer(nTimeInterval, nTimerId);
			}
		}
		
	}
	return 0;
}
//SetTempRev(worldid, x, y)
int LuaSetDeathRevivalPos(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	
	if (nPlayerIndex <= 0) 
		return 0;
	int nParamCount = Lua_GetTopIndex(L);
	
	PLAYER_REVIVAL_POS * pTempRev = Player[nPlayerIndex].GetDeathRevivalPos();
	
	if (nParamCount > 2)
	{
		pTempRev->m_nSubWorldID  = (int) Lua_ValueToNumber(L, 1);
		pTempRev->m_nMpsX = (int) Lua_ValueToNumber(L, 2);
		pTempRev->m_nMpsY = (int) Lua_ValueToNumber(L, 3); 
	}
	else if (nParamCount == 1)
	{
		pTempRev->m_nSubWorldID = SubWorld[Npc[Player[nPlayerIndex].m_nIndex].m_SubWorldIndex].m_SubWorldID;
		POINT Pos;
		int nRevId = (int) Lua_ValueToNumber(L, 1);
		g_SubWorldSet.GetRevivalPosFromId(pTempRev->m_nSubWorldID, nRevId, &Pos);
		pTempRev->m_ReviveID = nRevId;
		pTempRev->m_nMpsX = Pos.x;
		pTempRev->m_nMpsY = Pos.y;
	}
	else 
	{
		return 0;
	}
	
	return 0;
}

//AddMSPlayer(MissionId, PlayerIndex, groupid); / AddMSPlayer(MissionId, groupid)
int LuaAddMissionPlayer(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2) 
		return 0;
	int nMissionId = 0;
	int nPlayerIndex = 0;
	int nGroupId = 0;
	if (nParamCount >=3)
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
		nPlayerIndex = (int )Lua_ValueToNumber(L,2);
		nGroupId = (int) Lua_ValueToNumber(L,3);
	}
	else
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
		nGroupId = (int) Lua_ValueToNumber(L,2);
		nPlayerIndex = GetPlayerIndex(L);
	}
	
	if (nMissionId < 0 || nPlayerIndex <= 0 || nGroupId <0)
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			pMission->AddPlayer(nPlayerIndex, Player[nPlayerIndex].m_dwID, nGroupId);
		}
	}
	return 0;
}

int LuaRevivalAllNpc(Lua_State * L)
{
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		SubWorld[nSubWorldIndex].RevivalAllNpc();
	}
	return 0;
}

//RemoveMSPlayer(MissionId, PlayerIndex, groupid)
int LuaRemoveMissionPlayer(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2) 
		return 0;
	int nMissionId = 0;
	int nPlayerIndex = 0;
	int nGroupId = 0;
	if (nParamCount >=3)
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
		nPlayerIndex = (int )Lua_ValueToNumber(L,2);
		nGroupId = (int) Lua_ValueToNumber(L,3);
	}
	else
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
		nGroupId = (int) Lua_ValueToNumber(L,2);
		nPlayerIndex = GetPlayerIndex(L);
	}
	
	if (nMissionId < 0 || nPlayerIndex <= 0 || nGroupId <0)
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			pMission->RemovePlayer(nPlayerIndex);
			//pMission->AddPlayer(nPlayerIndex, Player[nPlayerIndex].m_dwID, nGroupId);
		}
	}
	return 0;
}
//GetNextPlayer(mission, idx,group)
int LuaGetNextPlayer(Lua_State * L)
{
	unsigned long nPlayerIndex = 0;
	
	if (Lua_GetTopIndex(L) < 2)
	{
		Lua_PushNumber(L ,0);
		Lua_PushNumber(L, 0);
		return 2;
	}
	
	int nMissionId	= (int)Lua_ValueToNumber(L, 1);
	int nIdx		= (int)Lua_ValueToNumber(L, 2);
	int nGroup		= (int)Lua_ValueToNumber(L, 3);
	int nSubWorldIndex = GetSubWorldIndex(L);
	int nResultIdx = 0;
	
	if (nMissionId < 0 || nIdx < 0 || nGroup < 0) 
		goto lab_getnextplayer;
	
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			nResultIdx = pMission->GetNextPlayer(nIdx, nGroup, nPlayerIndex);
		}
	}
	
lab_getnextplayer:	
	Lua_PushNumber(L, nResultIdx);
	Lua_PushNumber(L, nPlayerIndex);
	return 2;
}

//MSMsg2Group(missionid, string , group)
int LuaMissionMsg2Group(Lua_State * L)
{
	int nMissionId = (int)Lua_ValueToNumber(L,1);
	char * strMsg = (char *)Lua_ValueToString(L, 2);
	int	nGroupId = (int) Lua_ValueToNumber(L, 3);
	
	if (nMissionId < 0 || !strMsg || nGroupId <0)
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			pMission->Msg2Group(strMsg, nGroupId);
		}
	}
	
	return 0;
}

//MSMsg2Group(missionid, string)
int LuaMissionMsg2All(Lua_State * L)
{
	int nMissionId = (int)Lua_ValueToNumber(L,1);
	char * strMsg = (char *)Lua_ValueToString(L, 2);
	
	if (nMissionId < 0 || !strMsg)
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			pMission->Msg2All(strMsg);
		}
	}
	
	return 0;
}

//MSMsg2Group(missionid, string , group)
int LuaMissionMsg2Player(Lua_State * L)
{
	int nMissionId = (int)Lua_ValueToNumber(L,1);
	char * strMsg = (char *)Lua_ValueToString(L, 2);
	int	nPlayerIndex = (int) Lua_ValueToNumber(L, 3);
	
	if (nMissionId < 0 || !strMsg || nPlayerIndex <0)
		return 0;
	
	int nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			pMission->Msg2Group(strMsg, nPlayerIndex);
		}
	}
	
	return 0;
}

int LuaMissionPlayerCount(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	unsigned long ulCount = 0;
	int nMissionId = 0;
	int nGroupId = 0;
	int nSubWorldIndex = 0;
	if (nParamCount < 1) 
		goto lab_getmissionplayercount;
	
	if (nParamCount >= 2)
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
		nGroupId = (int) Lua_ValueToNumber(L,2);
	}
	else
	{
		nMissionId = (int)Lua_ValueToNumber(L,1);
	}
	
	if (nMissionId < 0 || nGroupId <0)
		goto lab_getmissionplayercount;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			ulCount = pMission->GetGroupPlayerCount(nGroupId);
		}
	}
	
lab_getmissionplayercount:
	Lua_PushNumber(L, ulCount);
	return 1;
}

int LuaSetPlayerDeathScript(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) 
		return 0;
	char * szScript = (char *)Lua_ValueToString(L, 1);
	Player[nPlayerIndex].m_dwDeathScriptId = g_FileName2Id(szScript);
	return 0;
}

int LuaNpcIndexToPlayerIndex(Lua_State * L)
{
	int nResult = 0;
	int nNpcIndex = (int)Lua_ValueToNumber(L, 1);
	if (nNpcIndex <=  0 || nNpcIndex >= MAX_NPC)
		goto lab_npcindextoplayerindex;
	
	if (Npc[nNpcIndex].m_Index > 0 && Npc[nNpcIndex].IsPlayer())
	{
		if (Npc[nNpcIndex].GetPlayerIdx() > 0)
			nResult = Npc[nNpcIndex].GetPlayerIdx();
	}
	
lab_npcindextoplayerindex:
	Lua_PushNumber(L, nResult);
	return 1;
	
}
//  
int LuaGetMissionPlayer_PlayerIndex(Lua_State * L)
{
	unsigned long nResult = 0;
	int nSubWorldIndex = 0;
	if (Lua_GetTopIndex(L) < 2) 
		goto lab_getmissionplayer_npcindex;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		int nMissionId = (int)Lua_ValueToNumber(L, 1);
		int nDataIndex = (int)Lua_ValueToNumber(L, 2);
		if (nMissionId < 0 || nDataIndex < 0)
			goto lab_getmissionplayer_npcindex;
		
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			nResult = pMission->GetMissionPlayer_PlayerIndex(nDataIndex);
		}
	}
	
lab_getmissionplayer_npcindex:
	Lua_PushNumber(L,nResult);
	return 1;
}

int LuaGetMissionPlayer_DataIndex(Lua_State * L)
{
	unsigned long nResult = 0;
	int nSubWorldIndex = 0;
	if (Lua_GetTopIndex(L) < 2) 
		goto lab_getmissionplayer_dataindex;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		int nMissionId = (int)Lua_ValueToNumber(L, 1);
		int nPlayerIndex = (int)Lua_ValueToNumber(L, 2);
		if (nMissionId < 0 || nPlayerIndex < 0)
			goto lab_getmissionplayer_dataindex;
		
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			nResult = pMission->GetMissionPlayer_DataIndex(nPlayerIndex);
		}
	}
	
lab_getmissionplayer_dataindex:
	Lua_PushNumber(L,nResult);
	return 1;
}

//SetMPParam(missionid, nDidx, vid, v)
int LuaSetMissionPlayerParam(Lua_State * L)
{
	int nSubWorldIndex = 0;
	if (Lua_GetTopIndex(L) < 4) 
		return 0;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		int nMissionId = (int)Lua_ValueToNumber(L, 1);
		int nDataIndex = (int)Lua_ValueToNumber(L, 2);
		int nParamId =	 (int)Lua_ValueToNumber(L ,3);
		int nValue =	 (int )Lua_ValueToNumber(L, 4);
		
		if (nMissionId < 0 || nDataIndex < 0 || nParamId > 2)
			return 0;
		
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			if (nParamId == 1)
				pMission->m_MissionPlayer.SetParam1(nDataIndex, nValue);
			else
				pMission->m_MissionPlayer.SetParam2(nDataIndex, nValue);
		}
	}
	return 0;
}

int LuaGetMissionPlayerParam(Lua_State * L)
{
	int nResult = 0;
	int nSubWorldIndex = 0;
	if (Lua_GetTopIndex(L) < 3) 
		goto lab_getmissionplayerparam;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		int nMissionId = (int)Lua_ValueToNumber(L, 1);
		int nDataIndex = (int)Lua_ValueToNumber(L, 2);
		int nParamId =	 (int)Lua_ValueToNumber(L ,3);
		
		if (nMissionId < 0 || nDataIndex < 0 || nParamId > 2)
			goto lab_getmissionplayerparam;
		
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			if (nParamId == 1)
				nResult = pMission->m_MissionPlayer.GetParam1(nDataIndex);
			else
				nResult = pMission->m_MissionPlayer.GetParam2(nDataIndex);
		}
	}
lab_getmissionplayerparam:
	Lua_PushNumber(L, nResult);
	return 1;
}

int LuaGetPlayerMissionGroup(Lua_State * L)
{
	int nResult = 0;
	int nSubWorldIndex = 0;
	if (Lua_GetTopIndex(L) < 2) 
		goto lab_getmissionplayergroup;
	
	nSubWorldIndex = GetSubWorldIndex(L);
	if (nSubWorldIndex >= 0) 
	{
		int nMissionId = (int)Lua_ValueToNumber(L, 1);
		int nNpcIndex = (int)Lua_ValueToNumber(L, 2);
		
		if (nMissionId < 0 || nNpcIndex < 0)
			goto lab_getmissionplayergroup;
		
		KMission Mission;
		Mission.SetMissionId(nMissionId);
		KMission * pMission = SubWorld[nSubWorldIndex].m_MissionArray.GetData(&Mission);
		if (pMission)
		{
			nResult = pMission->GetMissionPlayer_GroupId(nNpcIndex);
		}			
	}
lab_getmissionplayergroup:
	Lua_PushNumber(L ,nResult);
	return 1;
	
}

int LuaSetPlayerRevivalOptionWhenLogout(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0) return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) 
		return 0;
	int nType = (int)Lua_ValueToNumber(L, 1);
	
	if (nType)
		Player[nPlayerIndex].SetLoginType(1);
	else
		Player[nPlayerIndex].SetLoginType(0);
	
	return 0;
}



int LuaSetPlayerPKValue(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;	
	int nPKValue = (int)Lua_ValueToNumber(L,1);
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	if (Player[nPlayerIndex].m_nIndex <= 0) 
		return 0;
	Player[nPlayerIndex].m_cPK.SetPKValue(nPKValue);
	return 0;
}

int LuaGetPlayerPKValue(Lua_State * L)
{
	int nPKValue = 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		goto lab_getplayerpkvalue;
	
	if (Player[nPlayerIndex].m_nIndex <= 0) 
		goto lab_getplayerpkvalue;
	nPKValue = Player[nPlayerIndex].m_cPK.GetPKValue();
	
lab_getplayerpkvalue:
	Lua_PushNumber(L, nPKValue);
	return 1;
}


int	LuaGetCurNpcIndex(Lua_State * L)
{
	int nNpcIndex = 0;
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		goto lab_getcurnpcindex;
	
lab_getcurnpcindex:
	Lua_PushNumber(L, Player[nPlayerIndex].m_nIndex);
	return 1;
}




//showladder(count, ladderid1, ladderid2.....)
int LuaShowLadder(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if (nParamCount < 2) 
		return 0;
	int nLadderCount = (DWORD) Lua_ValueToNumber(L, 1);
	if (nLadderCount <= 0)
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	
	if (nLadderCount > nParamCount - 1 )
		nLadderCount = nParamCount - 1;
	BYTE Buffer[sizeof(LADDER_LIST) + 50 * sizeof(DWORD)];
	LADDER_LIST * pLadderList = (LADDER_LIST*)&Buffer;
	pLadderList->ProtocolType = s2c_ladderlist;
	pLadderList->nCount = nLadderCount;
	pLadderList->wSize = sizeof(LADDER_LIST) + nLadderCount * sizeof(DWORD) - 1;
	for (int i = 0; i < nLadderCount; i ++)
	{
		pLadderList->dwLadderID[i] = (DWORD)Lua_ValueToNumber(L, i + 2);
	}
	g_pServer->PackDataToClient(Player[nPlayerIndex].m_nNetConnectIdx, &Buffer, pLadderList->wSize + 1);
	return 0;
	
}

int LuaSwearBrother(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	int nTeamId = Lua_ValueToNumber(L, 1);
	
	if (nTeamId >= MAX_TEAM || nTeamId < 0)
		return 0;
	
	KPlayerChat::STRINGLIST BrotherList;
	_ASSERT(g_Team[nTeamId].m_nCaptain > 0);

	std::string strCapName;
	strCapName = Npc[Player[g_Team[nTeamId].m_nCaptain].m_nIndex].Name;
	BrotherList.push_back(strCapName);

	for (int i  = 0; i < g_Team[nTeamId].m_nMemNum; i++)
	{
		int nPlayerIndex = g_Team[nTeamId].m_nMember[i];
		if ( nPlayerIndex > 0 && nPlayerIndex < MAX_PLAYER)
		{
			std::string strName;
			strName = 	Npc[Player[nPlayerIndex].m_nIndex].Name;
			BrotherList.push_back(strName);
		}
	}
	
	KPlayerChat::MakeBrother(BrotherList);
	return 0;
}

int LuaMakeEnemy(Lua_State * L)
{
	
	return 0;
}


int LuaAddLeadExp(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1)
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	
	int nLeadExp = (int)Lua_ValueToNumber(L, 1);
	Player[nPlayerIndex].AddLeadExp(nLeadExp);
	return 0;
}

int LuaGetLeadLevel(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	int nLeadLevel = 0;
	if (nPlayerIndex <= 0)
		goto lab_getleadlevel;
	nLeadLevel = (int)Player[nPlayerIndex].m_dwLeadLevel;
	
lab_getleadlevel:
	Lua_PushNumber(L, nLeadLevel);
	return 1;
}

int LuaAddMagicPoint(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1)
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		return 0;
	int nSkillPoint = (int)Lua_ValueToNumber(L, 1);
	
	Player[nPlayerIndex].m_nSkillPoint += nSkillPoint;
	if (Player[nPlayerIndex].m_nSkillPoint < 0)
		Player[nPlayerIndex].m_nSkillPoint = 0;
	
	return 0;
}

int LuaGetMagicPoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	int nSkillPoint = 0;
	if (nPlayerIndex <= 0)
		goto lab_getmagicpoint;
	
lab_getmagicpoint:
	Lua_PushNumber(L, Player[nPlayerIndex].m_nSkillPoint);
	return 1;
}

int LuaAddPropPoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	int nPropPoint = 0;
	if (nPlayerIndex <= 0)
		return 0;
	nPropPoint = (int)Lua_ValueToNumber(L, 1);
	Player[nPlayerIndex].m_nAttributePoint += nPropPoint;
	return 0;
}


//PayExtPoint
int LuaPayExtPoint(Lua_State * L)
{
	int nResult = 0;
	int nPay = 0;
	int nPlayerIndex = 0;
	if (Lua_GetTopIndex(L) < 1)
		goto lab_payextpoint;
	
	nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		goto lab_payextpoint;
	nPay = Lua_ValueToNumber(L, 1);
	if (nPay < 0)
		goto lab_payextpoint;
	nResult = Player[nPlayerIndex].PayExtPoint(nPay);

lab_payextpoint:
	Lua_PushNumber(L, nResult);
	return 1;
}

//PayExtPoint
int LuaGetExtPoint(Lua_State * L)
{
	int nResult = 0;
	int nPlayerIndex = 0;
	if (Lua_GetTopIndex(L) < 1)
		goto lab_getextpoint;
	
	nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
		goto lab_getextpoint;
	
	nResult = Player[nPlayerIndex].GetExtPoint();
	
lab_getextpoint:
	Lua_PushNumber(L, nResult);
	return 1;
}

int LuaGetRestPropPoint(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	int nPropPoint = 0;
	if (nPlayerIndex <= 0)
		goto lab_getrestproppoint;
	nPropPoint = Player[nPlayerIndex].m_nAttributePoint;
	
lab_getrestproppoint:
	Lua_PushNumber(L, nPropPoint);
	return 1;
}

//Msg2GM(str, id)
int LuaMsgToGameMaster(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if ( nParamCount < 1 ) 
		return 0;
	int nParamID = 0;
	const char * szMsg = Lua_ValueToString(L, 1) ;
	if (!szMsg) 
		return 0;

	if (nParamCount < 2)
	{
		nParamID = 0;
	}
	else
	{
		nParamID = (int) Lua_ValueToNumber(L, 2);
	}

	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex > 0) 
	{
		char szID[32];

		sprintf(szID, "%d", nParamID);
		//KPlayerChat::SendInfoToGM(Player[nPlayerIndex].m_AccoutName, Npc[Player[nPlayerIndex].m_nIndex].Name, (char *) szMsg, strlen(szMsg) );
		KPlayerChat::SendInfoToGM("GM", szID, (char *) szMsg, strlen(szMsg) );
	}
	return 0;
}

//Msg2IP(IP, ID, str)
int LuaMsgToIP(Lua_State * L)
{
	int nParamCount = Lua_GetTopIndex(L);
	if ( nParamCount < 3 ) 
		return 0;
	int nIP = 0;
	const char * szIP = Lua_ValueToString(L, 1) ;
	nIP = _a2ip(szIP);
	if (nIP == 0)
		return 0;

	int nID = (int) Lua_ValueToNumber(L, 2);
	const char * szMsg = Lua_ValueToString(L, 3) ;
	if (!szMsg) 
		return 0;

	int nParamID = 0;
	if (nParamCount < 4)
	{
		nParamID = 0;
	}
	else
	{
		nParamID = (int) Lua_ValueToNumber(L, 4);
	}
	char szID[32];
	sprintf(szID, "%d", nParamID);
	KPlayerChat::SendInfoToIP(nIP, nID, "GM", szID, (char *) szMsg, strlen(szMsg) );
	return 0;
}

int LuaGetPlayerInfo(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	char szDesMsg[250];
	char szMsg[800];
	int nNpcIdx = 0;
	KNpc * pNpc = NULL;
	KPlayer* pPlayer = NULL;
	szDesMsg[0] = 0;
	if (nPlayerIndex <= 0)
		goto lab_getplayerinfo;
	
	pPlayer = &Player[nPlayerIndex];
	nNpcIdx = pPlayer->m_nIndex;
	pNpc = &Npc[nNpcIdx];
	sprintf(szMsg, "Name:%s,Lvl:%d,IP:%s,Lf:%d|%d,Mn:%d|%d,Mny:%d,sp:%d,ap:%d", pNpc->Name, pNpc->m_Level, g_pServer->GetClientInfo(pPlayer->m_nNetConnectIdx), pNpc->m_CurrentLife, pNpc->m_CurrentLifeMax, pNpc->m_CurrentMana, pNpc->m_CurrentManaMax, pPlayer->m_ItemList.GetMoney(room_equipment), pPlayer->m_nSkillPoint, pPlayer->m_nAttributePoint );
	g_StrCpyLen(szDesMsg, szMsg, 250);
	
lab_getplayerinfo:
	Lua_PushString(L, szDesMsg);
	return 1;
}

int LuaGetIP(Lua_State * L)
{
	int nPlayerIndex = GetPlayerIndex(L);
	char szDesMsg[200];
	szDesMsg[0] = 0;
	KPlayer * pPlayer = NULL;
	if (nPlayerIndex <= 0)
		goto lab_getplayerip;
	pPlayer = &Player[nPlayerIndex];
	strcpy(szDesMsg, g_pServer->GetClientInfo(pPlayer->m_nNetConnectIdx));
	
lab_getplayerip:
	Lua_PushString(L, szDesMsg);
	return 1;
}

int LuaSetDeathPunish(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;

	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex <= 0)
	{
		int nState = Lua_ValueToNumber(L, 1);
		if (nState == 0)
			Npc[Player[nPlayerIndex].m_nIndex].m_nCurPKPunishState = 0;
		else
			Npc[Player[nPlayerIndex].m_nIndex].m_nCurPKPunishState = enumDEATH_MODE_PKBATTLE_PUNISH;
	}
	return 0;
}



int LuaHideNpc(Lua_State * L)
{
	if (Lua_GetTopIndex(L) < 2)
		return 0;
	
	int nNpcIndex  = 0;
	
	if (Lua_IsNumber(L,1))
	{
		nNpcIndex = (int)Lua_ValueToNumber(L, 1);
	}
	else 
	{
		const char * szName = Lua_ValueToString(L, 1);
		int nSubWorldIndex = GetSubWorldIndex(L);
		if (nSubWorldIndex < 0)
			return 0;
		
		nNpcIndex = SubWorld[nSubWorldIndex].FindNpcFromName(szName);
	}
	
	if (nNpcIndex > 0 || nNpcIndex < MAX_NPC)
	{
		int nFrame = Lua_ValueToNumber(L, 2);
		if (nFrame <= 0)
			nFrame = 1;
		
		Npc[nNpcIndex].ExecuteRevive();
		Npc[nNpcIndex].m_Frames.nTotalFrame = nFrame;
		Npc[nNpcIndex].m_Frames.nCurrentFrame = 0;
	}
	
	return 0;
}

#endif

int LuaPlayMusic(Lua_State * L)//PlayMusic(musicname,loop=1, vol );
{
	if (Lua_GetTopIndex(L) < 1) 
		return 0;
	
	int nPlayerIndex = GetPlayerIndex(L);
	if (nPlayerIndex < 0) return 0;
	
	PLAYER_SCRIPTACTION_SYNC UiInfo;
	UiInfo.m_bUIId = UI_PLAYMUSIC;
	UiInfo.m_bOptionNum = 1;
	UiInfo.m_nOperateType = SCRIPTACTION_UISHOW;
	
	int nMsgId = 0;
	
	g_StrCpyLen(UiInfo.m_pContent, Lua_ValueToString(L,1), sizeof(UiInfo.m_pContent));
	UiInfo.m_nBufferLen = strlen(((char *)UiInfo.m_pContent));
	UiInfo.m_bParam1 = 0;
	
#ifndef _SERVER
	UiInfo.m_bParam2 = 0;
#else
	UiInfo.m_bParam2 = 1;
#endif
	
	Player[nPlayerIndex].DoScriptAction(&UiInfo);
	return 0;
}

int LuaFadeInMusic(Lua_State * L)
{
	return 0;
}

int LuaFadeOutMusic(Lua_State * L)
{
	return 0;
}




#ifndef _SERVER

int LuaPlaySound(Lua_State * L)
{
	
	return 0;
}

int LuaPlaySprMovie(Lua_State * L)
{
	return 0;
}
#endif
TLua_Funcs GameScriptFuns[] = 
{
	//Í¨ÓÃÖ¸Áî
	
	{"Say", LuaSelectUI},
	{"Talk", LuaTalkUI},
	{"GetTaskTemp", LuaGetTempTaskValue},
	{"SetTaskTemp", LuaSetTempTaskValue},
	{"Message", LuaMessage},
	
	{"GetBit",	LuaGetBit},
	{"GetByte",	LuaGetByte},
	{"SetBit",	LuaSetBit},
	{"SetByte",	LuaSetByte},
	{"Include",LuaIncludeFile},
	{"PutMessage", LuaSendMessageInfo},
	{"AddGlobalNews",LuaAddGlobalNews},
	{"AddGlobalTimeNews",LuaAddGlobalTimeNews},
	{"AddGlobalCountNews",LuaAddGlobalCountNews	},
	{"AddLocalNews",LuaAddLocalNews},
	{"AddLocalTimeNews",LuaAddLocalTimeNews},
	{"AddLocalCountNews",LuaAddLocalCountNews	},
	
	//·þÎñÆ÷¶Ë½Å±¾Ö¸Áî
#ifdef _SERVER
	//-----------------»ù±¾¹¦ÄÜ
	{"AddRepute", LuaModifyRepute},	//ÐÞ¸ÄÉùÍûÖµ
	{"GetRepute",LuaGetRepute},	
	{"AddOffer",AddOffer},	
	{"GetOffer",GetOffer},	
	{"GetNpcIdx", LuaGetCurNpcIndex},
	
	{"SetTimer",		LuaSetTimer},		//SetTimer(Ê±¼äÁ¿, Ê±¼äTaskId):¸øÍæ¼Ò´ò¿ª¼ÆÊ±Æ÷,Ê±¼äµ½Ê±½«×Ô¶¯µ÷ÓÃOnTimerº¯Êý
	{"StopTimer",		LuaStopTimer},		//StopTimer()£º¹Ø±Õµ±Ç°Íæ¼ÒµÄ¼ÆÊ±Æ÷
	{"GetRestTime",		LuaGetRestTime},	//GetRestTime:»ñµÃ¼ÆÊ±Æ÷½«´¥·¢µÄÊ£ÓÚÊ±¼ä	
	{"GetTimerId",		LuaGetCurTimerId},	//CurTimerId = GetTimerId():»ñµÃµ±Ç°Ö´ÐÐµÄ¼ÆÊ±Æ÷µÄid,Èç¹ûÃ»ÓÐÔò·µ»Ø0
	{"GetTask",			LuaGetTaskValue},	//GetTask(ÈÎÎñºÅ):»ñµÃµ±Ç°Íæ¼Ò¸ÃÈÎÎñºÅµÄÖµ
	{"SetTask",			LuaSetTaskValue},	//SetTask(ÈÎÎñºÅ,Öµ):ÉèÖÃÈÎÎñÖµ
	{"IsCaptain",		LuaIsLeader},		//IsCaptain()ÊÇ·ñÎª¶Ó³¤
	{"GetTeam",			LuaGetTeamId},		//GetTeam()·µ»ØÍæ¼Ò¶ÓÎéID
	{"GetTeamSize",		LuaGetTeamSize},	//GetTeamSize()
	{"LeaveTeam",		LuaLeaveTeam},		//LeaveTeam()ÈÃÍæ¼ÒÀë¿ª×ÔÉí¶ÓÎé
	{"Msg2Player",		LuaMsgToPlayer	},	//Msg2Player(ÏûÏ¢)
	{"Msg2Team",		LuaMsgToTeam},		//Msg2Team(ÏûÏ¢)Í¨ÖªÍæ¼ÒµÄ×é
	{"Msg2SubWorld",	LuaMsgToSubWorld},	//Msg2SubWorld(ÏûÏ¢)Í¨ÖªÊÀ½ç
	{"Msg2Region",		LuaMsgToAroundRegion},//Msg2Region(ÏûÏ¢)Í¨ÖªÖÜÎ§Region
	{"Msg2GM",			LuaMsgToGameMaster}, //Msg2GM(StrInfo)
	{"Msg2IP",			LuaMsgToIP}, //Msg2IP(IP, ID, StrInfo)
	{"GetInfo",			LuaGetPlayerInfo}, //str = GetInfo()
	{"GetIP",			LuaGetIP},
	{"SetPos",			LuaSetPos},			//SetPos(x,y)½øÈëÄ³µã	
	{"GetPos",			LuaGetPos},			//GetPos() return x,y,subworldindex
	{"GetWorldPos",		LuaGetNewWorldPos},	//W,X,Y = GetWorldPos()·µ»ØÓÚNewWorldÅäÅãµÄ×ø±ê
	{"NewWorld",		LuaEnterNewWorld},
	{"DropItem",		LuaDropItem},		//DropItem(NpcId, ÎïÆ·id»òÎïÆ·Ãû)
	{"AddItem",			LuaAddItem},		//AddItem(nItemClass, nDetailType, nParticualrType, nLevel, nSeries, nLuck, nItemLevel..6)
	{"AddEventItem",	LuaAddEventItem	},	//AddEventItem(ÊÂ¼þÎïÆ·ID)
	{"AddGoldItem",			AddGoldItem},
	{"AddPurpleItem",		AddPurpleItem},
	{"AddGenItem",		AddGenItem},
	{"SetItemDur",		SetItemDur},
	{"PlaceItem",		PlaceItem},
	{"SetItemMA",		SetItemMA},
	{"DelItem",			LuaDelItem},
	{"ClearItem",		ClearItem},
	{"HaveItem",		LuaHaveItem},
	{"GetItemCount",	LuaGetTaskItemCount}, //GetItemCount(TaskItemName Or TaskItemId)
	{"AddMagic",		LuaAddMagic},		//AddMagic(Ä§·¨id»òÄ§·¨Ãû)¸øÍæ¼Ò¼ÓÄ³¸öÄ§·¨
	{"DelMagic",		LuaDelMagic},		//DelMagic(Ä§·¨id»òÄ§·¨Ãû)
	{"HaveMagic",		LuaHaveMagic},		//HaveMagic(Ä§·¨id»òÄ§·¨Ãû)·µ»Ø0»ò1
	{"GetMagicLevel",	LuaGetMagicLevel},	//GetMagicLevel(Ä§·¨id»òÄ§·¨Ãû)·µ»ØµÈ¼¶
	{"AddMagicPoint",	LuaAddMagicPoint},
	{"GetMagicPoint",	LuaGetMagicPoint},
	
	{"SubWorldID2Idx",	LuaSubWorldIDToIndex}, //SubWorldID2Idx
	
	
	{"AddLeadExp",		LuaAddLeadExp},
	{"GetLeadLevel",	LuaGetLeadLevel},
	
	{"SetFightState",	LuaSetFightState}, 
	{"GetFightState",	LuaGetFightState},
	
	{"AddNpc",			LuaAddNpc},			//AddNpc(ÈËÎïÄ£°åid»òÈËÎïÄ£°åÃû,Ëù´¦ÊÀ½çid£¬µã×ø±êx,µã×ø±êy),·µ»ØnpcidÖµ
	{"DelNpc",			LuaDelNpc},			//DelNpc(Npcid)
	{"SetNpcScript",	LuaSetNpcActionScript},	//SetNpcScript(npcid, ½Å±¾ÎÄ¼þÃû)ÉèÖÃnpcµ±Ç°µÄ½Å±¾
	{"SetRevPos",		LuaSetPlayerRevivalPos},//SetRevPos(µãÎ»ÖÃX£¬µãÎ»ÖÃY)ÉèÖÃÍæ¼ÒµÄµ±Ç°ÊÀ½çµÄµÈÈëµãÎ»ÖÃ
	{"SetTempRevPos",	LuaSetDeathRevivalPos}, //SetTempRevPos(subworldid, x, y ) or SetTempRevPos(id);
	{"GetCurCamp",		LuaGetPlayerCurrentCamp},//GetCurCamp()»ñµÃÍæ¼ÒµÄµ±Ç°ÕóÓª
	{"GetCamp",			LuaGetPlayerCamp	},//GetCamp()»ñµÃÍæ¼ÒÕóÓª
	{"SetCurCamp",		LuaSetPlayerCurrentCamp},//SetCurCamp(ÕóÓªºÅ):ÉèÖÃÍæ¼Òµ±Ç°ÕóÓª
	{"SetCamp",			LuaSetPlayerCamp},		  //SetCamp(ÕóÓªºÅ):ÉèÖÃÕóÓª	
	{"RestoreCamp",		LuaRestorePlayerCamp	},//RestoreCamp()»Ö¸´ÕóÓª
	{"GetFaction",		LuaGetPlayerFaction,	},//GetFaction()»ñµÃÍæ¼ÒµÄÃÅÅÉÃû 
	{"SetFaction",		LuaChangePlayerFaction},  //SetFaction(ÃÅÅÉÃû):ÉèÖÃÍæ¼ÒÃÅÅÉÃû
	{"GetColdR",		LuaGetPlayerColdResist},	
	{"SetColdR",		LuaGetPlayerColdResist},
	{"GetFireR",		LuaGetPlayerFireResist	},
	{"SetFireR",		LuaSetPlayerFireResist	},
	{"GetLightR",		LuaGetPlayerLightResist	},
	{"SetLightR",		LuaSetPlayerLightResist},
	{"GetPoisonR",		LuaGetPlayerPoisonResist},
	{"SetPoisonR",		LuaSetPlayerPoisonResist},
	{"GetPhyR",			LuaGetPlayerPhysicsResist	},
	{"SetPhyR",			LuaSetPlayerPhysicsResist	},
	{"GetExp",			LuaGetPlayerExp	},			//GetExp():»ñµÃÍæ¼ÒµÄµ±Ç°¾­ÑéÖµ
	{"AddExp",			LuaModifyPlayerExp},		//AddExp(¾­ÑéÖµ£¬¶Ô·½µÈ¼¶£¬ÊÇ·ñ×é¶Ó¹²Ïí¾­ÑéÖµ)
	{"AddOwnExp",		LuaAddOwnExp	},			//AddOwnExp(Exp)£¬¸øÍæ¼ÒÖ±½Ó¼Ó¾­Ñé
	{"GetLife",			LuaGetPlayerLife},			//GetLife()»ñµÃÍæ¼ÒµÄÉúÃüÖµ
	{"RestoreLife",		LuaRestorePlayerLife},		//RestoreLife()»Ö¸´Íæ¼ÒµÄÉúÃü
	{"GetMana",			LuaGetPlayerMana},			//GetMana()»ñµÃÍæ¼ÒµÄMana
	{"RestoreMana",		LuaRestorePlayerMana},		//RestoreMana()»Ö¸´Íæ¼ÒµÄMana
	{"GetStamina",		LuaGetPlayerStamina},		//GetStamina()»ñµÃÍæ¼ÒStamina
	{"RestoreStamina",	LuaRestorePlayerStamina},	//RestoreMana()»Ö¸´Íæ¼ÒµÄStamina
	{"GetDefend",		LuaGetPlayerDefend},		//GetDefend()»ñµÃÍæ¼ÒµÄ·ÀÓùÁ¦
	{"GetSex",			LuaGetPlayerSex},			//GetSex()»ñµÃÍæ¼ÒµÄÐÔ±ð
	{"GetSeries",		LuaGetPlayerSeries},		//GetSeries()»ñµÃÍæ¼ÒµÄÏµ0man/1woman
	{"SetSeries",		LuaSetPlayerSeries},		//SetSeries(ÐÔ±ðºÅ)
	{"GetName",			LuaGetPlayerName},			//GetName()»ñµÃÍæ¼ÒµÄÐÕÃû
	{"GetUUID",			LuaGetPlayerID},			//GetUUID()»ñµÃÍæ¼ÒµÄÎ¨Ò»ID
	{"GetLeadExp",		LuaGetPlayerLeadExp},		//GetLeadExp()»ñµÃÍæ¼ÒµÄÍ³ÂÊ¾­ÑéÖµ
	{"GetLeadLevel",	LuaGetPlayerLeadLevel},		//GetLeadLevel()»ñµÃÍæ¼ÒµÄÍ³ÂÊµÈ¼¶
	{"GetLevel",		LuaGetLevel},				//GetLevel()GetPlayers Level
	{"GetRestAP",		LuaGetPlayerRestAttributePoint},//GetRestAP()»ñµÃÍæ¼ÒµÄÊ£ÓÚÊôÐÔµãÊý
	{"GetRestSP",		LuaGetPlayerRestSkillPoint},	//GetRestSP()»ñµÃÍæ¼ÒµÄÊ£ÓÚ¼¼ÄÜµãÊý	
	{"GetLucky",		LuaGetPlayerLucky},			//GetLucky()»ñµÃÍæ¼ÒµÄÐÒÔËÖµ
	{"GetEng",			LuaGetPlayerEngergy},		//GetEng()»ñµÃÍæ¼ÒµÄÁ¦Á¿ÖµEng
	{"GetDex",			LuaGetPlayerDexterity},		//GetDex()»ñµÃÍæ¼ÒµÄDex
	{"GetStrg",			LuaGetPlayerStrength},		//GetStrg()
	{"GetVit",			LuaGetPlayerVitality},		//GetVit()
	{"IncProp",			IncProp},		//IncProp()
	{"GetCash",			LuaGetPlayerCashMoney},		//GetCash()»ñµÃÍæ¼ÒµÄÏÖ½ð
	{"Pay",				LuaPlayerPayMoney},			//Pay(½ð¶îÊý)¿Û³ýÍæ¼Ò½ðÇ®³É¹¦·µ»Ø1£¬Ê§°Ü·µ»Ø0
	{"Earn",			LuaPlayerEarnMoney},		//Earn(½ð¶îÊý)Ôö¼ÓÍæ¼Ò½ðÇ®
	{"PrePay",			LuaPlayerPrePayMoney},		//¸¶¶¨½ð£¬³É¹¦·µ»Ø1£¬Ê§°Ü·µ»Ø0
	{"ExeScript",		LuaPlayerExecuteScript},	//ExeScript(½Å±¾ÎÄ¼þÃû,²ÎÊý),Ö´ÐÐÊ±»áµ÷ÓÃmainº¯Êý	
	{"AttackNpc",		LuaAttackNpc},				//AttackNpc(NpcDwid,ÎïÀíÉËº¦Öµ£¬±ù£¬»ð£¬µç£¬¶¾£©
	{"KillNpc",			LuaKillNpc},				//KillNpc(NpcDWID)
	{"KillPlayer",		LuaKillPlayer},				//KillPlayer();
	{"Sale",			LuaSale},					//Sale(SaleId)ÂòÂô£¬SaleIdÎª±ãÂôµÄÎïÆ·ÐÅÏ¢ÁÐ±íid
	{"UseTownPortal",	LuaUseTownPortal	},
	{"ReturnFromPortal",LuaReturnFromTownPortal	},
	{"SetNpcCurCamp",	LuaSetNpcCurCamp},
	{"OpenBox",			LuaOpenBox},
	{"AddStation",	LuaAddPlayerStation},
	{"AddTermini",		LuaAddPlayerWayPoint},
	{"GetStation",		LuaGetPlayerStation	},
	{"GetStationCount", LuaGetPlayerStationCount},
	
	{"GetCityCount", LuaGetAllStationCount},
	{"GetCity", LuaGetCity},
	
	{"GetWayPoint",		LuaGetPlayerWayPoint},
	{"GetStationName",	LuaGetStationName},
	{"GetWayPointName", LuaGetWayPointName},
	{"GetPrice2Station", LuaGetPriceToStation},
	{"GetPrice2WayPoint", LuaGetPriceToWayPoint	},
	{"GetStationPos",	LuaGetStationPos},
	{"GetWayPointPos",	LuaGetWayPointPos},
	{"GetPlayerCount",	LuaGetPlayerCount},
	{"GetRank",			LuaGetRank},//GetRank()
	{"SetRank",			LuaSetRank},//SetRank(id)
	{"SetPropState",	LuaSetObjPropState},//SetPropState( hide = 1) hide obj
	{"GetServerName",	LuaGetServerName},
	
	
	//------------------Station Script ---------------
	{"GetWharfName",	LuaGetDockName},
	{"GetWharfCount",	LuaGetDockCount},
	{"GetWharfPrice",	LuaGetDockPrice},
	{"GetWharf",		LuaGetDock},
	{"GetWharfPos",		LuaGetDockPos},
	{"GetTerminiFState", LuaGetWayPointFightState},
	//------------------------------------------------
	{"KickOutPlayer",	LuaKickOutPlayer},
	{"KickOutSelf",		LuaKickOutSelf},
	{"GetSkillId",		LuaGetSkillIdInSkillList},
	{"SetSkillLevel",	LuaSetSkillLevel},
	{"SetChatFlag",		LuaSetPlayerChatForbiddenFlag},
	//------------------------------------------------
	
	{"AddNote", LuaAddNote},
	//-----------------Mission Script-----------------
	{"GetMissionV", LuaGetMissionValue},//GetMissionV(Vid)
	{"SetMissionV", LuaSetMissionValue},//SetMissionV(Vid, Value)
	{"GetGlbMissionV", LuaGetGlobalMissionValue	},
	{"SetGlbMissionV", LuaSetGlobalMissionValue	},
	{"OpenMission", LuaInitMission},//OpenMission(missionid)
	{"RunMission", LuaRunMission},
	{"CloseMission", LuaCloseMission},//CloseMission(missionid)
	{"StartMissionTimer", LuaStartMissionTimer},////StartMissionTimer(missionid, timerid, time)
	{"StopMissionTimer", LuaStopMissionTimer},
	{"GetMSRestTime", LuaGetMissionRestTime}, //GetMSRestTime(missionid, timerid)
	{"GetMSIdxGroup",LuaGetPlayerMissionGroup},//GetPlayerGroup(missionid, playerid);
	
	{"AddMSPlayer", LuaAddMissionPlayer},
	{"DelMSPlayer", LuaRemoveMissionPlayer},
	{"GetNextPlayer", LuaGetNextPlayer},
	{"PIdx2MSDIdx", LuaGetMissionPlayer_DataIndex},//(missionid, pidx)
	{"MSDIdx2PIdx", LuaGetMissionPlayer_PlayerIndex},//(missionid, dataidx)
	{"NpcIdx2PIdx", LuaNpcIndexToPlayerIndex},
	{"GetMSPlayerCount", LuaMissionPlayerCount},//GetMSPlayerCount(missionid, group = 0)

	{"RevivalAllNpc",	LuaRevivalAllNpc},
	
	{"SetPMParam", LuaSetMissionPlayerParam },
	{"GetPMParam", LuaGetMissionPlayerParam},
	{"Msg2MSGroup", LuaMissionMsg2Group},
	{"Msg2MSAll", LuaMissionMsg2All},
	{"Msg2MSPlayer", LuaMissionMsg2Player},
	{"SetDeathScript", LuaSetPlayerDeathScript},
	{"Death", LuaKillNpc},

	{"HideNpc", LuaHideNpc}	,//HideNpc(npcindex/npcname, hidetime)
	{"SetLogoutRV", LuaSetPlayerRevivalOptionWhenLogout},
	{"SetCreateTeam",LuaSetCreateTeamOption},
	{"GetPK", LuaGetPlayerPKValue},  //pkValue = GetPK() 
	{"SetPK", LuaSetPlayerPKValue}, //SetPK(pkValue)
	//------------------------------------------------
	//ÅÅÃûÏà¹Øº¯Êý
	{"ShowLadder", LuaShowLadder}, //ShowLadder(LadderCount, LadderId1,LadderId2,...);
	//------------------------------------------------

	{"OpenTong",	LuaOpenTong},	//OpenTong()Í¨ÖªÍæ¼Ò´ò¿ª°ï»á½çÃæ
	{"SetPunish",	LuaSetDeathPunish},// SetPunish(0/1) 0±íÊ¾²»ÊÜÈÎºÎ³Í·£
	//-------------------------------------------------
	//½á°Ý
	{ "SwearBrother", LuaSwearBrother}, // ret = SwearBrother(TeamId);
	{"MakeEnemy",	LuaMakeEnemy}, //½á³ð MakeEnemy(enemyname)
	{"RollbackSkill", LuaRollBackSkills},
	{"UpdateSkill", LuaUpdateSkillList},
	//-------------------------------------------------

	{"AddProp",		LuaAddPropPoint},//¼ÓÍæ¼ÒÊôÐÔµã
	{"GetProp",		LuaGetRestPropPoint },
	
	{"GetExtPoint",	LuaGetExtPoint},
	{"PayExtPoint",	LuaPayExtPoint},
	
	//{"Trade",			LuaTrade	},				//Trade("maininfo", "IniFileName.iniµÄÂ·¾¶Ãû")
	//Trade("MainInfo", n, "item1|price1|function1", "item2|price2|function2", ......, "itemn|pricen|functionn")
	
	//¿Í»§¶Ë½Å±¾Ö¸Áî
	
	{"AddSkillState",	AddSkillState},
	{"GetNpcData",	GetNpcData},
	{"GetNpcLvScript",	GetNpcLvScript},
	{"GetTongName",	GetTongName},
	{"GetTongID",	GetTongID},
	{"TCreate",	TCreate},
	{"TAddMember",	TAddMember},
	{"TGetMaster",	TGetMaster},
	{"TAddMoney",	TAddMoney},
	{"TAddStoreOffer",	TAddStoreOffer},
	{"TAddBuildFund",	TAddBuildFund},
	{"TResetTask",	TResetTask},
	{"GetWeekGoal",		GetWeekGoal},
	{"GetWeekOffer",	GetWeekOffer},
	{"SetWeekOffer",	SetWeekOffer},
	{"TAddWeekGoal",	TAddWeekGoal},
	{"TSetWeekGoalLevel",	TSetWeekGoalLevel},
	{"TWeekGoalPrice",	TWeekGoalPrice},
	{"TPlayerPrice",	TPlayerPrice},
#else 
	{"PlaySound", LuaPlaySound}, //PlaySound(Sound);
	{"PlaySprMovie",LuaPlaySprMovie},//PlaySprMovie(npcindex, Movie, times)
#endif
	{"FileName2Id",		FileName2Id},
	{"PlayMusic", LuaPlayMusic}, //PlayMusic(Music,Loop)
	{"FadeInMusic",LuaFadeInMusic},
	{"FadeOutMusic",LuaFadeOutMusic},
};


TLua_Funcs WorldScriptFuns[] =// ·ÇÖ¸¶ÔÍæ¼ÒµÄ½Å±¾Ö¸Áî¼¯
{
	//Í¨ÓÃÖ¸Áî

	{"AddLocalNews",LuaAddLocalNews},
	{"AddLoaclTimeNews",LuaAddLocalTimeNews},
	{"AddLocalCountNews",LuaAddLocalCountNews	},
	//·þÎñÆ÷¶Ë½Å±¾Ö¸Áî
#ifdef _SERVER
	{"Msg2SubWorld",	LuaMsgToSubWorld},	//Msg2SubWorld(ÏûÏ¢)Í¨ÖªÊÀ½ç
	{"Msg2IP",			LuaMsgToIP}, //Msg2IP(IP, ID, StrInfo)	
	{"SubWorldID2Idx",	LuaSubWorldIDToIndex}, //SubWorldID2Idx
	{"GetServerName",	LuaGetServerName},
	{"KickOutPlayer",	LuaKickOutPlayer},
	{"KickOutAccount",	LuaKickOutAccount},
#endif

//	{"GetMissionV", LuaGetMissionValue},//GetMissionV(Vid)
//	{"SetMissionV", LuaSetMissionValue},//SetMissionV(Vid, Value)
//	{"GetGlbMissionV", LuaGetGlobalMissionValue	},
//	{"SetGlbMissionV", LuaSetGlobalMissionValue	},
//	{"OpenMission", LuaInitMission},//OpenMission(missionid)
//	{"RunMission", LuaRunMission},
//	{"CloseMission", LuaCloseMission},//CloseMission(missionid)
//	{"StartMissionTimer", LuaStartMissionTimer},////StartMissionTimer(missionid, timerid, time)
//	{"StopMissionTimer", LuaStopMissionTimer},
//	{"GetMSRestTime", LuaGetMissionRestTime}, //GetMSRestTime(missionid, timerid)
//	{"GetMSIdxGroup",LuaGetPlayerMissionGroup},//GetPlayerGroup(missionid, playerid);
//	
//	{"AddMSPlayer", LuaAddMissionPlayer},
//	{"DelMSPlayer", LuaRemoveMissionPlayer},
//	{"GetNextPlayer", LuaGetNextPlayer},
//	{"PIdx2MSDIdx", LuaGetMissionPlayer_DataIndex},//(missionid, pidx)
//	{"MSDIdx2PIdx", LuaGetMissionPlayer_PlayerIndex},//(missionid, dataidx)
//	{"NpcIdx2PIdx", LuaNpcIndexToPlayerIndex},

	//{"GetMSPlayerCount", LuaMissionPlayerCount},//GetMSPlayerCount(missionid, group = 0)

//	{"RevivalAllNpc",	LuaRevivalAllNpc},
	
	//{"SetPMParam", LuaSetMissionPlayerParam },
	//{"GetPMParam", LuaGetMissionPlayerParam},
	//{"Msg2MSGroup", LuaMissionMsg2Group},
	//{"Msg2MSAll", LuaMissionMsg2All},
	//{"Msg2MSPlayer", LuaMissionMsg2Player},


}; 

int g_GetGameScriptFunNum()
{
	return sizeof(GameScriptFuns)  / sizeof(GameScriptFuns[0]);
}

int g_GetWorldScriptFunNum()
{
	return sizeof(WorldScriptFuns)  / sizeof(WorldScriptFuns[0]);
}