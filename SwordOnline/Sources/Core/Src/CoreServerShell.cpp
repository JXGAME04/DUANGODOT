/*****************************************************************************************
//	Íâ½ç·ÃÎÊCore server ½Ó¿Ú·½·¨
//	Copyright : Kingsoft 2002
//	Author	:   Wooy (Wu yue)
//	CreateTime:	2002-12-20
------------------------------------------------------------------------------------------
*****************************************************************************************/
#include "KCore.h"
#include "CoreServerShell.h"
#include "KThread.h"
#include "KPlayer.h"
#include "KItemList.h"
#include "KSubWorldSet.h"
#include "KProtocolProcess.h"
#include "KNewProtocolProcess.h"
#include "KPlayerSet.h"
#include "KLadder.h"
#include "KTongProtocol.h"
#ifdef _STANDALONE
#include "KLadder.cpp"
#endif

//#include "KNetServer.h"
//#include "../MultiServer/Heaven/Interface/iServer.h"
#ifdef _STANDALONE
#include "IClient.h"
#else
#include "../../lib/S3DBInterface.h"
#include "../../Headers/IClient.h"
#include "../../../Headers/IClient.h"
#include "../../../Headers/KGmProtocol.h"
#endif

#include "LuaFuns.h"
#include "KSortScript.h"
#include "KSubWorld.h"


#include "malloc.h"

class CoreServerShell : public iCoreServerShell
{
public:
	int  GetLoopRate();
	void GetGuid(int nIndex, void* pGuid);
	DWORD GetExchangeMap(int nIndex);
	bool IsPlayerLoginTimeOut(int nIndex);
	void RemovePlayerLoginTimeOut(int nIndex);
	bool IsPlayerExchangingServer(int nIndex);
	void ProcessClientMessage(int nIndex, const char* pChar, int nSize);
	void ProcessNewClientMessage(IClient*, DWORD, DWORD, int nIndex, const char* pChar, int nSize);
	void SendNetMsgToTransfer(IClient* pClient);
	void SendNetMsgToChat(IClient* pClient);
	void SendNetMsgToTong(IClient* pClient);
	void ProcessBroadcastMessage(const char* pChar, int nSize);
	void ProcessExecuteMessage(const char* pChar, int nSize);
	void ClientDisconnect(int nIndex);
	void RemoveQuitingPlayer(int nIndex);
	void* SavePlayerDataAtOnce(int nIndex);
	bool IsCharacterQuiting(int nIndex);
	bool CheckProtocolSize(const char* pChar, int nSize);
	bool PlayerDbLoading(int nPlayerIndex, int bSyncEnd, int& nStep, unsigned int& nParam);
	int  AttachPlayer(const unsigned long lnID, GUID* pGuid);
	void GetPlayerIndexByGuid(GUID* pGuid, int* pnIndex, int* plnID);
	void AddPlayerToWorld(int nIndex);
	void* PreparePlayerForExchange(int nIndex);
	void PreparePlayerForLoginFailed(int nIndex);
	void RemovePlayerForExchange(int nIndex);
	void RecoverPlayerExchange(int nIndex);
	int  AddCharacter(int nExtPoint, int nChangeExtPoint, void* pBuffer, GUID* pGuid);
	int	 AddTempTaskValue(int nIndex, const char* pData);
	//ÏòÓÎÏ··¢ËÍ²Ù×÷
	int	 OperationRequest(unsigned int uOper, unsigned int uParam, int nParam);
	//»ñÈ¡Á¬½Ó×´¿ö
	int	 GetConnectInfo(KCoreConnectInfo* pInfo);
	//BOOL ValidPingTime(int nIndex);
	//´ÓÓÎÏ·ÊÀ½ç»ñÈ¡Êý¾Ý
	int	 GetGameData(unsigned int uDataId, unsigned int uParam, int nParam);
	//ÈÕ³£»î¶¯£¬coreÈç¹ûÒªÊÙÖÕÕýÇÞÔò·µ»Ø0£¬·ñÔò·µ»Ø·Ç0Öµ
	int  Breathe();
	//ÊÍ·Å½Ó¿Ú¶ÔÏó
	void Release();
	void SetSaveStatus(int nIndex, UINT uStatus);
	UINT GetSaveStatus(int nIndex);

	BOOL GroupChat(IClient* pClient, DWORD FromIP, unsigned long FromRelayID, DWORD channid, BYTE tgtcls, DWORD tgtid, const void* pData, size_t size);
	void SetLadder(void* pData, size_t uSize);
	BOOL PayForSpeech(int nIndex, int nType);
	void SetServerTrans(LPVOID pServer, unsigned int lnID);//guve
	void SetServerChat(LPVOID pServer, unsigned int lnID);
	void SetServerTong(LPVOID pServer, unsigned int lnID);
private:
	int	 OnLunch(LPVOID pServer);
	int	 OnShutdown();
};

IServer* g_pTransServer = NULL;//guve
IServer* g_pChatServer = NULL;
IServer* g_pTongServer = NULL;
unsigned int g_nTransID = -1;
unsigned int g_nChatID = -1;
unsigned int g_nTongID = -1;

static CoreServerShell	g_CoreServerShell;

CORE_API void g_InitCore();
#ifndef CORE_STATIC
#ifndef _STANDALONE
extern "C" __declspec(dllexport)
#endif
#else
extern "C"
#endif
iCoreServerShell* CoreGetServerShell()
{
	g_InitCore();
	return &g_CoreServerShell;
}

void CoreServerShell::Release()
{
	g_ReleaseCore();
}

int CoreServerShell::GetLoopRate()
{
	return g_SubWorldSet.m_nLoopRate;
}

	//»ñÈ¡Á¬½Ó×´¿ö
int	 CoreServerShell::GetConnectInfo(KCoreConnectInfo* pInfo)
{
	if (pInfo)
		pInfo->nNumPlayer = PlayerSet.GetPlayerNumber();
	return 1;
}


int CoreServerShell::AddCharacter(int nExtPoint, int nChangeExtPoint, void* pBuffer, GUID* pGuid)
{
	int nIdx = 0;
	const TRoleData* pData = (const TRoleData*)pBuffer;

	if (pData && pData->BaseInfo.szName[0])
	{
		nIdx = PlayerSet.Add((char*)pData->BaseInfo.szName, pGuid);
		if (nIdx <= 0 || nIdx >= MAX_PLAYER)
			return 0;
		strcpy(Player[nIdx].m_AccoutName, pData->BaseInfo.caccname);
		strcpy(Player[nIdx].m_PlayerName, pData->BaseInfo.szName);
		DWORD	dwLen = pData->dwDataLen;
//		_ASSERT(dwLen < 64 * 1024);
		ZeroMemory(Player[nIdx].m_SaveBuffer, sizeof(Player[nIdx].m_SaveBuffer));
		memcpy(Player[nIdx].m_SaveBuffer, pBuffer, dwLen);

		Player[nIdx].m_pStatusLoadPlayerInfo = Player[nIdx].m_SaveBuffer;
		// À©Õ¹µã£¬ÓÃÓÚ»î¶¯
		Player[nIdx].SetExtPoint(nExtPoint, nChangeExtPoint);
		return nIdx;
	}
	return 0;
}

bool CoreServerShell::PlayerDbLoading(int nPlayerIndex, int bSyncEnd, int& nStep, unsigned int& nParam)
{
	TRoleData* pData = (TRoleData *)Player[nPlayerIndex].m_pStatusLoadPlayerInfo;
	
	if (bSyncEnd)
	{
		Player[nPlayerIndex].m_pStatusLoadPlayerInfo = NULL;
		nStep = 0;
		nParam = 0;

		return true;
	}
	else if (pData)	
	{
//		if (0 == Player[nPlayerIndex].LoadDBPlayerInfo((BYTE *)pData, nStep, nParam))
//		{
//			// °ÑÍæ¼ÒµÄµÇÈë×´Ì¬ÉèÖÃÎªÎ´µÇÈë£¬µÈ´ýÊ±ÑÓ×Ô¶¯Çå³ý
//			Player[nPlayerIndex].m_nNetConnectIdx = -1;
//			Player[nPlayerIndex].m_dwLoginTime = -1;
//			return false;
//		}
//		else
//			return true;
		return Player[nPlayerIndex].LoadDBPlayerInfo((BYTE *)pData, nStep, nParam);
	}
	return false;
}

void CoreServerShell::AddPlayerToWorld(int nIndex)
{
//	int nIndex = PlayerSet.FindClient(lnID);
	Player[nIndex].LaunchPlayer();
}

void CoreServerShell::ProcessClientMessage(int nIndex, const char* pChar, int nSize)
{
	PlayerSet.ProcessClientMessage(nIndex, pChar, nSize);
}

void CoreServerShell::ProcessNewClientMessage(IClient* pTransfer,
									   DWORD dwFromIP, DWORD dwFromRelayID,
									   int nPlayerIndex,
									   const char* pChar, int nSize)
{
	g_NewProtocolProcess.ProcessNetMsg(pTransfer, dwFromIP, dwFromRelayID,
										nPlayerIndex, (BYTE*)pChar, nSize);

}

void CoreServerShell::SendNetMsgToTransfer(IClient* pClient)
{
	g_NewProtocolProcess.SendNetMsgToTransfer(pClient);
}

void CoreServerShell::SendNetMsgToChat(IClient* pClient)
{
	g_NewProtocolProcess.SendNetMsgToChat(pClient);
}

void CoreServerShell::SendNetMsgToTong(IClient* pClient)
{
	g_NewProtocolProcess.SendNetMsgToTong(pClient);
}

void CoreServerShell::ProcessBroadcastMessage(const char* pChar, int nSize)
{
	g_NewProtocolProcess.BroadcastLocalServer(pChar, nSize);
}

void CoreServerShell::ProcessExecuteMessage(const char* pChar, int nSize)
{
	g_NewProtocolProcess.ExecuteLocalServer(pChar, nSize);
}

void CoreServerShell::ClientDisconnect(int nIndex)
{
//	PlayerSet.Remove(nClient);
	PlayerSet.PrepareRemove(nIndex);
}

void CoreServerShell::RemoveQuitingPlayer(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;

	if (Player[nIndex].IsWaitingRemove())
	{
		PlayerSet.RemoveQuiting(nIndex);
	}
}
//--------------------------------------------------------------------------
//	¹¦ÄÜ£º´ÓÓÎÏ·ÊÀ½ç»ñÈ¡Êý¾Ý
//	²ÎÊý£ºunsigned int uDataId --> ±íÊ¾»ñÈ¡ÓÎÏ·Êý¾ÝµÄÊý¾ÝÏîÄÚÈÝË÷Òý£¬ÆäÖµÎªÃ·¾ÙÀàÐÍ
//							GAMEDATA_INDEXµÄÈ¡ÖµÖ®Ò»¡£
//		  unsigned int uParam  --> ÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//		  int nParam --> ÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//	·µ»Ø£ºÒÀ¾ÝuDataIdµÄÈ¡ÖµÇé¿ö¶ø¶¨¡£
//--------------------------------------------------------------------------
int	CoreServerShell::GetGameData(unsigned int uDataId, unsigned int uParam, int nParam)
{
	int nRet = 0;
	switch(uDataId)
	{
	case SGDI_CHARACTER_ACCOUNT:
		if (uParam)
		{
			nRet = PlayerSet.GetPlayerAccount(nParam, (char *)uParam);
			if (nRet == FALSE)
				((char *)uParam)[0] = 0;
		}
		break;
	case SGDI_CHARACTER_NAME:
		if (uParam)
		{
			nRet = PlayerSet.GetPlayerName(nParam, (char*)uParam);
			if (nRet == FALSE)
				((char *)uParam)[0] = 0;
		}
		break;
	case SGDI_CHARACTER_FIND:
		if(uParam)
		{
			if(nParam)
			{
				nRet = PlayerSet.FindSame(uParam);
			}
			else
			{
				char* pName = (char*)uParam;
				nRet = PlayerSet.FindSame(g_FileName2Id(pName));
			}
		}
		break;
	case SGDI_CHARACTER_EXTPOINTCHANGED:
		if (uParam)
		{
			if (uParam >= MAX_PLAYER)
			{
				nRet = 0;
				break;
			}
			nRet = Player[uParam].GetExtPointChanged();
		}
		break;
	case SGDI_CHARACTER_EXTPOINT:
		if (uParam)
		{
			if (uParam >= MAX_PLAYER)
			{
				nRet = 0;
				break;
			}
			nRet = Player[uParam].GetExtPoint();
		}
		break;
	case SGDI_LOADEDMAP_ID:
		if (uParam)
		{
			int i;
			int nMax = nParam;
			if(nMax < MAX_SUBWORLD) nMax = MAX_SUBWORLD;
			for (i = 0; i < nMax; i++)
			{
				if (SubWorld[i].m_SubWorldID != -1)
				{
					((USHORT *)uParam)[i] = SubWorld[i].m_SubWorldID;
				}
				else
				{
					nRet = i;
					break;
				}
			}
		}
		break;
	case SGDI_CHARACTER_ID:
		if (uParam)
		{
			if (uParam >= MAX_PLAYER)
			{
				nRet = 0;
				break;
			}
			nRet = Player[uParam].m_dwID;
		}
		break;
	case SGDI_CHARACTER_NETID:
		nRet = -1; //guve
		if (uParam)
		{
			if (uParam >= MAX_PLAYER)
				break;
			nRet = Player[uParam].m_nNetConnectIdx;
		}
		break;
	case SGDI_CHARACTER_SEX:
		nRet = 0;
		if(nParam > 0 && nParam < MAX_PLAYER && Player[nParam].m_nIndex > 0)
			nRet = Npc[Player[nParam].m_nIndex].m_nSex;
		break;
	case SGDI_TONG_APPLY_CREATE:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_APPLY_CREATE	*pApply = (STONG_SERVER_TO_CORE_APPLY_CREATE*)uParam;
			if (pApply->m_nPlayerIdx <= 0 || pApply->m_nPlayerIdx >= MAX_PLAYER)
				break;
			nRet = Player[pApply->m_nPlayerIdx].m_cTong.CheckCreateCondition(pApply->m_nCamp, pApply->m_szTongName);
			if(!nRet)
			{
				KPlayerChat::SendSystemMsg(pApply->m_nPlayerIdx, "Thµnh lËp bang héi thÊt b¹i");
			}
		}
		break;

	// ÉêÇë¼ÓÈë°ï»á
	// uParam : struct STONG_SERVER_TO_CORE_APPLY_ADD point
	case SGDI_TONG_APPLY_ADD:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_APPLY_ADD	*pAdd = (STONG_SERVER_TO_CORE_APPLY_ADD*)uParam;
			if (pAdd->m_nPlayerIdx <= 0 || pAdd->m_nPlayerIdx >= MAX_PLAYER)
				break;
			Player[pAdd->m_nPlayerIdx].m_cTong.TransferAddApply(pAdd->m_uDestID);
		}
		break;

	// ÅÐ¶Ï¼ÓÈë°ï»áÌõ¼þÊÇ·ñºÏÊÊ
	// uParam : ´«ÈëµÃ char point £¬ÓÃÓÚ½ÓÊÕ°ï»áÃû³Æ
	// nParam : struct STONG_SERVER_TO_CORE_CHECK_ADD_CONDITION point
	case SGDI_TONG_CHECK_ADD_CONDITION:
		{
			nRet = 0;
			STONG_SERVER_TO_CORE_CHECK_ADD_CONDITION *pAdd = (STONG_SERVER_TO_CORE_CHECK_ADD_CONDITION*)nParam;
			if (pAdd->m_nSelfIdx <= 0 || pAdd->m_nSelfIdx >= MAX_PLAYER)
				break;
			if (pAdd->m_nTargetIdx <= 0 || pAdd->m_nTargetIdx >= MAX_PLAYER || Player[pAdd->m_nTargetIdx].m_dwID != pAdd->m_dwNameID)
				break;
			if (Player[pAdd->m_nSelfIdx].m_cTong.CheckAddCondition(pAdd->m_nTargetIdx))
			{
				*((UINT*)uParam) = Player[pAdd->m_nSelfIdx].m_cTong.GetTongNameID();
				nRet = 1;
			}
		}
		break;

	// »ñµÃ°ï»áÐÅÏ¢
	// uParam : ´«ÈëµÄ STONG_SERVER_TO_CORE_GET_INFO point
	case SGDI_TONG_GET_INFO:
		{
			nRet = 0;
			STONG_SERVER_TO_CORE_GET_INFO	*pInfo = (STONG_SERVER_TO_CORE_GET_INFO*)uParam;
			int nCurTime = g_SubWorldSet.GetGameTime();
			switch (pInfo->m_nInfoID)
			{
			case enumTONG_APPLY_INFO_ID_TONG_HEAD:
				{
					if(!pInfo->m_nParam1)
						break;
					if (pInfo->m_nSelfIdx <= 0 || pInfo->m_nSelfIdx >= MAX_PLAYER)
						break;
					if(Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeInfo >= nCurTime)
						break;
					Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeInfo = nCurTime + 3;
					if(Player[pInfo->m_nSelfIdx].m_cTong.GetTongNameID() == pInfo->m_nParam1)
						pInfo->m_nParam2 = 1;//me
					else
						pInfo->m_nParam2 = 0;//other
					nRet = 1;
				}
				break;
			case enumTONG_APPLY_INFO_MEMBERPAGE:
				{
					if(!pInfo->m_nParam1)
						break;
					if (pInfo->m_nSelfIdx <= 0 || pInfo->m_nSelfIdx >= MAX_PLAYER)
						break;
					if(Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					if(Player[pInfo->m_nSelfIdx].m_cTong.GetTongNameID() != pInfo->m_nParam1)
						break;
					nRet = 1;
				}
				break;
			case enumTONG_APPLY_INFO_TONGPAGE:
				{
					if (pInfo->m_nSelfIdx <= 0 || pInfo->m_nSelfIdx >= MAX_PLAYER)
						break;
					if(Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					nRet = 1;
				}
				break;
			case enumTONG_APPLY_INFO_RECORD:
				{
					if (pInfo->m_nSelfIdx <= 0 || pInfo->m_nSelfIdx >= MAX_PLAYER)
						break;
					if(!Player[pInfo->m_nSelfIdx].m_cTong.CheckIn())
						break;
					if(Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[pInfo->m_nSelfIdx].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					nRet = 1;
				}
				break;
			}
		}
		break;

	// ÅÐ¶ÏÊÇ·ñÓÐÈÎÃüÈ¨Àû
	// uParam : ´«ÈëµÄ TONG_APPLY_INSTATE_COMMAND point
	// nParam : PlayerIndex
	case SGDI_TONG_INSTATE_POWER:
		if (uParam)
		{
			nRet = 0;
			TONG_APPLY_INSTATE_COMMAND	*pApply = (TONG_APPLY_INSTATE_COMMAND*)uParam;
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0 || Player[nParam].m_dwID == pApply->m_uDestID)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckInstatePower(pApply);
		}
		break;

	// nguoi duoc chuyen vi bang chu
	case SGDI_TONG_BE_INSTATED:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_BE_INSTATED	*pInstated = (STONG_SERVER_TO_CORE_BE_INSTATED*)uParam;
			if (pInstated->m_nPlayerIdx <= 0 || pInstated->m_nPlayerIdx >= MAX_PLAYER)
				break;
			if (Player[pInstated->m_nPlayerIdx].m_nIndex <= 0)
				break;
			if (Player[pInstated->m_nPlayerIdx].m_dwID != pInstated->m_dwPlayerNameID)
				break;
			Player[pInstated->m_nPlayerIdx].m_cTong.BeInstated(pInstated);
		}
		break;

	case SGDI_TONG_KICK_POWER:
		{
			nRet = 0;
			if (!uParam)
				break;
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckKickPower(uParam);
		}
		break;

	// ±nguoi bi kich ra bang
	case SGDI_TONG_BE_KICKED:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_BE_KICKED	*pKicked = (STONG_SERVER_TO_CORE_BE_KICKED*)uParam;
			if (pKicked->m_nPlayerIdx <= 0 || pKicked->m_nPlayerIdx >= MAX_PLAYER)
				break;
			if (Player[pKicked->m_nPlayerIdx].m_nIndex <= 0)
				break;
			if (Player[pKicked->m_nPlayerIdx].m_dwID != pKicked->m_dwPlayerNameID)
				break;
			if (Player[pKicked->m_nPlayerIdx].m_cTong.GetTongNameID() != pKicked->m_dwTongNameID)
				break;
			Player[pKicked->m_nPlayerIdx].m_cTong.BeKicked();
		}
		break;

	case SGDI_TONG_LEAVE_POWER:
		{
			nRet = 0;
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0)
				break;
			nRet = Player[nParam].m_cTong.CheckLeavePower();
		}
		break;

	case SGDI_TONG_LEAVE:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_LEAVE	*pLeave = (STONG_SERVER_TO_CORE_LEAVE*)uParam;
			if (pLeave->m_nPlayerIdx <= 0 || pLeave->m_nPlayerIdx >= MAX_PLAYER)
				break;
			if (Player[pLeave->m_nPlayerIdx].m_nIndex <= 0
			|| Player[pLeave->m_nPlayerIdx].m_dwID != pLeave->m_dwPlayerNameID
			|| Player[pLeave->m_nPlayerIdx].m_cTong.GetTongNameID() != pLeave->m_dwTongNameID)
				break;
			Player[pLeave->m_nPlayerIdx].m_cTong.Leave(pLeave);
		}
		break;

	// Àë¿ª°ï»áÅÐ¶Ï
	// uParam : ´«ÈëµÄ TONG_APPLY_CHANGE_MASTER_COMMAND point
	// nParam : PlayerIndex
	case SGDI_TONG_CHANGE_MASTER_POWER:
		if (uParam)
		{
			nRet = 0;
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckChangeMasterPower(uParam);
		}
		break;

	case SGDI_TONG_CHANGE_AS:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_CHANGE_AS	*pAs = (STONG_SERVER_TO_CORE_CHANGE_AS*)uParam;
			if (pAs->m_nPlayerIdx <= 0 || pAs->m_nPlayerIdx >= MAX_PLAYER)
				break;
			if (Player[pAs->m_nPlayerIdx].m_nIndex <= 0)
				break;
			if (Player[pAs->m_nPlayerIdx].m_dwID != pAs->m_dwPlayerNameID
			|| Player[pAs->m_nPlayerIdx].m_cTong.GetTongNameID() != pAs->m_dwTongNameID)
				break;
			if(pAs->m_btFigure < 0)
			{
				KPlayerChat::SendSystemMsg(pAs->m_nPlayerIdx, "ChuyÓn vÞ bang chñ kh«ng thÓ tiÕn hµnh liªn tôc, xin ®îi mét thêi gian.");
				break;
			}
			Player[pAs->m_nPlayerIdx].m_cTong.ChangeAs(pAs);
		}
		break;

	// °broadcast master
	case SGDI_TONG_CHANGE_MASTER:
		if (uParam)
		{
			STONG_CHANGE_MASTER_SYNC	*pChange = (STONG_CHANGE_MASTER_SYNC*)uParam;
			int nIdx;
			nIdx = PlayerSet.GetFirstPlayer();
			while (nIdx)
			{
				if (Player[nIdx].m_cTong.GetTongNameID() == pChange->m_dwTongNameID)
				{
					Player[nIdx].m_cTong.ChangeMaster(pChange->m_szName);
				}
				nIdx = PlayerSet.GetNextPlayer();
			}
		}
		break;

	// »ñµÃ°ï»áÃû×Ö·û´®×ª»»³ÉµÄ dword
	// nParam : PlayerIndex
	case SGDI_TONG_GET_TONG_NAMEID:
		{
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0)
				break;
			nRet = Player[nParam].m_cTong.GetTongNameID();
		}
		break;

	// µÇÂ½Ê±ºò»ñµÃ°ï»áÐÅÏ¢
	// uParam : ´«ÈëµÄ STONG_SERVER_TO_CORE_LOGIN point
	case SGDI_TONG_LOGIN:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_LOGIN	*pLogin = (STONG_SERVER_TO_CORE_LOGIN*)uParam;
			if (pLogin->m_dwParam <= 0 || pLogin->m_dwParam >= MAX_PLAYER)
				break;
			if (Player[pLogin->m_dwParam].m_nIndex <= 0)
				break;
			Player[pLogin->m_dwParam].m_cTong.Login(pLogin);
		}
		break;
		
	// Í¨Öªcore·¢ËÍÄ³Íæ¼ÒµÄ°ï»áÐÅÏ¢
	// nParam : player index
	case SGDI_TONG_SEND_SELF_INFO:
		{
			if (nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if (Player[nParam].m_nIndex <= 0)
				break;
			Player[nParam].m_cTong.SendSelfInfo();
		}
		break;
	case SGDI_TONG_APPLY_RIGHT:
		{
			STONG_SERVER_TO_CORE_APPLY_ADD	*pCmd = (STONG_SERVER_TO_CORE_APPLY_ADD*)uParam;
			if (pCmd->m_nPlayerIdx <= 0 || pCmd->m_nPlayerIdx >= MAX_PLAYER
			|| !Player[pCmd->m_nPlayerIdx].m_cTong.CheckIn())
				break;
			if(Player[pCmd->m_nPlayerIdx].m_dwID == pCmd->m_uDestID)
				break;
			if(Player[pCmd->m_nPlayerIdx].m_cTong.GetFigure() != enumTONG_FIGURE_MASTER)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[pCmd->m_nPlayerIdx].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[pCmd->m_nPlayerIdx].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = 1;
		}
		break;
	case SGDI_TONG_RIGHT_SENDER_RET:
		{
			STONG_DISTRIB_RIGHT_COMMAND	*pInfo = (STONG_DISTRIB_RIGHT_COMMAND*)uParam;
			if(pInfo->m_nSenderPIdx <= 0 || pInfo->m_nSenderPIdx >= MAX_PLAYER)
				break;
			if(Player[pInfo->m_nSenderPIdx].m_dwID != pInfo->m_uSenderID)
				break;
			if(Player[pInfo->m_nSenderPIdx].m_cTong.GetTongNameID() != pInfo->m_dwTongNameID)
				break;
			TONG_RIGHT_SENDER_SYNC sSync;
			sSync.ProtocolType		= s2c_extendtong;
			sSync.m_btMsgId			= enumTONG_SYNC_ID_SENDER_RIGHT;
			sSync.m_wLength			= sizeof(sSync) - 1;
			sSync.m_uDestID = pInfo->m_uDestID;
			sSync.m_uRight = pInfo->m_uRight;
			if(g_pServer)
			g_pServer->PackDataToClient(Player[pInfo->m_nSenderPIdx].m_nNetConnectIdx, &sSync, sSync.m_wLength + 1);
		}
		break;
	case SGDI_TONG_BE_RIGHT_RET:
		if(uParam)
		{
			STONG_BE_RIGHT_SYNC	*pInfo = (STONG_BE_RIGHT_SYNC*)uParam;
			if(nParam <= 0 || nParam >= MAX_PLAYER)
				break;
			if(Player[nParam].m_dwID != pInfo->m_dwPlayerNameID
			|| Player[nParam].m_cTong.GetTongNameID() != pInfo->m_dwTongNameID)
				break;
			Player[nParam].m_cTong.BeRight(pInfo->m_uRight);
		}
		break;
	case SGDI_TONG_CHANGERECRUIT_POWER:
		{
			nRet = 0;
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckChangeRecruitPower();
		}
		break;
	case SGDI_TONG_CHANGERECRUIT_RET:
		if(uParam)
		{
			STONG_CHANGERECRUIT_SYNC	*pInfo = (STONG_CHANGERECRUIT_SYNC*)uParam;
			if(pInfo->m_nPlayerIdx <= 0 || pInfo->m_nPlayerIdx >= MAX_PLAYER)
				break;
			if(Player[pInfo->m_nPlayerIdx].m_dwID != pInfo->m_dwPlayerNameID)
				break;
			if(Player[pInfo->m_nPlayerIdx].m_cTong.GetTongNameID() != pInfo->m_dwTongNameID)
				break;
			TONG_CHANGE_RECRUIT_SYNC sSync;
			sSync.ProtocolType		= s2c_extendtong;
			sSync.m_btMsgId			= enumTONG_SYNC_ID_CHANGE_RECRUIT;
			sSync.m_wLength			= sizeof(sSync) - 1;
			sSync.m_bLockRecruit	= pInfo->m_bLockRecruit;
			if(g_pServer)
			g_pServer->PackDataToClient(Player[pInfo->m_nPlayerIdx].m_nNetConnectIdx, &sSync, sSync.m_wLength + 1);
		}
		break;
	case SGDI_TONG_DELETE:
		if(uParam)
		{
			int nIdx;
			nIdx = PlayerSet.GetFirstPlayer();
			while (nIdx)
			{
				if (Player[nIdx].m_cTong.GetTongNameID() == uParam)
				{
					Player[nIdx].m_cTong.Delete();
				}
				nIdx = PlayerSet.GetNextPlayer();
			}
		}
		break;
	case SGDI_TONG_CONTRIBMONEY_POWER:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckContribMoney(nParam);
		}
		break;
	case SGDI_TONG_WITHDRAWMONEY_POWER:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckWithDrawMoney(nParam);
		}
		break;
	case SGDI_TONG_WITHDRAWMONEY_SUCC:
		{
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			Player[uParam].m_ItemList.AddMoney(room_equipment, nParam*10000);
		}
		break;
	case SGDI_TONG_STOREOFFER_POWER:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckStoreOffer(nParam);
		}
		break;
	case SGDI_TONG_DISPENSEOFFER_POWER:
		if(uParam)
		{
			nRet = 0;
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			TONG_APPLY_DISPENSEOFFER_COMMAND* pInfo = (TONG_APPLY_DISPENSEOFFER_COMMAND*)uParam;
			if(pInfo->m_uDestID == 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckDispenseOffer(pInfo->m_uDestID, pInfo->m_nMoney);
		}
		break;
	case SGDI_TONG_BE_DISPENSEOFFER:
		{
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			if(nParam <= 0)
				break;
			int nDestDayOffer = Player[uParam].m_cTask.GetSaveVal(TASKVALUE_OFFER_ONDAY);
			if(nDestDayOffer + nParam > defTONG_MAX_OFFER_DAYLIMIT)
				Player[uParam].m_cTask.SetSaveVal(TASKVALUE_OFFER_ONDAY, defTONG_MAX_OFFER_DAYLIMIT);
			else
				Player[uParam].m_cTask.SetSaveVal(TASKVALUE_OFFER_ONDAY, nDestDayOffer + nParam);
			nDestDayOffer = Player[uParam].m_cTask.GetSaveVal(TASKVALUE_OFFER) + nParam;
			Player[uParam].m_cTask.SetSaveVal(TASKVALUE_OFFER, nDestDayOffer);
			char szPack[16];
			DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
			pCmd->ProtocolType = s2c_dynamic_structure;
			pCmd->nBranch = s2cdnmbr_taskoffer;
			pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
			UINT* p = (UINT*)(pCmd+1);
			*p = nDestDayOffer;
			if(g_pServer)
				g_pServer->PackDataToClient(Player[uParam].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
		}
		break;
	case SGDI_TONG_ASSIGNMONEY_POWER:
		if(uParam)
		{
			nRet = 0;
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			TONG_APPLY_ASSIGNFUND_COMMAND* pInfo = (TONG_APPLY_ASSIGNFUND_COMMAND*)uParam;
			nRet = Player[nParam].m_cTong.CheckAssignMoney(pInfo->nMemberPoint, pInfo->nManagerPoint, pInfo->nDirectorPoint);
		}
		break;
	case SGDI_TONG_ASSIGNMONEY_RET:
		if(uParam)
		{
			STONG_ASSIGNFUND_SYNC* pInfo = (STONG_ASSIGNFUND_SYNC*)uParam;
			int nIdx = PlayerSet.GetFirstPlayer();
			while (nIdx)
			{
				if (Player[nIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID)
				{
					if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_DIRECTOR)
					{
						if(pInfo->nDirectorPoint > 0)
							Player[nIdx].Earn(pInfo->nDirectorPoint*10000);
					}
					else if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MANAGER)
					{
						if(pInfo->nManagerPoint > 0)
							Player[nIdx].Earn(pInfo->nManagerPoint*10000);
					}
					else if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MEMBER)
					{
						if(pInfo->nMemberPoint > 0)
							Player[nIdx].Earn(pInfo->nMemberPoint*10000);
					}
				}
				nIdx = PlayerSet.GetNextPlayer();
			}
		}
		break;
	case SGDI_TONG_ASSIGNOFFER_RET:
		if(uParam)
		{
			STONG_ASSIGNFUND_SYNC* pInfo = (STONG_ASSIGNFUND_SYNC*)uParam;
			char szPack[16];
			DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
			pCmd->ProtocolType = s2c_dynamic_structure;
			pCmd->nBranch = s2cdnmbr_taskoffer;
			pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + sizeof(int);
			STONG_CONTRIBMONEY_COMMAND	sCmd;
			sCmd.ProtocolFamily	= pf_tong;
			sCmd.ProtocolID		= enumC2S_TONG_STOREOFFER;
			sCmd.m_nPlayerIdx		= 0;
			sCmd.m_dwPlayerNameID = 0;
			sCmd.m_dwTongNameID	= pInfo->m_dwTongNameID;
			int nIdx = PlayerSet.GetFirstPlayer();
			while (nIdx)
			{
				if (Player[nIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID)
				{
					if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_DIRECTOR)
					{
						if(pInfo->nDirectorPoint > 0)
						{
							int nAdd = pInfo->nDirectorPoint;
							int nDestDayOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER_ONDAY);
							if(nDestDayOffer + pInfo->nDirectorPoint > defTONG_MAX_OFFER_DAYLIMIT)
								nAdd = defTONG_MAX_OFFER_DAYLIMIT - nDestDayOffer;
							if(nAdd > 0)
							{
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER_ONDAY, nDestDayOffer + nAdd);
								int nCurOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER) + nAdd;
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER, nCurOffer);
								UINT* p = (UINT*)(pCmd+1);
								*p = nCurOffer;
								g_pServer->PackDataToClient(Player[nIdx].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
							}
							nAdd = pInfo->nDirectorPoint - nAdd;
							if(nAdd > 0) //con thua`, tra~ lai cong hien du tru~
							{
								sCmd.m_nMoney = nAdd;
								g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
							}
						}
					}
					else if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MANAGER)
					{
						if(pInfo->nManagerPoint > 0)
						{
							int nAdd = pInfo->nManagerPoint;
							int nDestDayOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER_ONDAY);
							if(nDestDayOffer + pInfo->nManagerPoint > defTONG_MAX_OFFER_DAYLIMIT)
								nAdd = defTONG_MAX_OFFER_DAYLIMIT - nDestDayOffer;
							if(nAdd > 0)
							{
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER_ONDAY, nDestDayOffer + nAdd);
								int nCurOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER) + nAdd;
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER, nCurOffer);
								UINT* p = (UINT*)(pCmd+1);
								*p = nCurOffer;
								g_pServer->PackDataToClient(Player[nIdx].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
							}
							nAdd = pInfo->nManagerPoint - nAdd;
							if(nAdd > 0) //con thua`, tra~ lai cong hien du tru~
							{
								sCmd.m_nMoney = nAdd;
								g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
							}
						}
					}
					else if(Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MEMBER)
					{
						if(pInfo->nMemberPoint > 0)
						{
							int nAdd = pInfo->nMemberPoint;
							int nDestDayOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER_ONDAY);
							if(nDestDayOffer + pInfo->nMemberPoint > defTONG_MAX_OFFER_DAYLIMIT)
								nAdd = defTONG_MAX_OFFER_DAYLIMIT - nDestDayOffer;
							if(nAdd > 0)
							{
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER_ONDAY, nDestDayOffer + nAdd);
								int nCurOffer = Player[nIdx].m_cTask.GetSaveVal(TASKVALUE_OFFER) + nAdd;
								Player[nIdx].m_cTask.SetSaveVal(TASKVALUE_OFFER, nCurOffer);
								UINT* p = (UINT*)(pCmd+1);
								*p = nCurOffer;
								g_pServer->PackDataToClient(Player[nIdx].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
							}
							nAdd = pInfo->nMemberPoint - nAdd;
							if(nAdd > 0) //con thua`, tra~ lai cong hien du tru~
							{
								sCmd.m_nMoney = nAdd;
								g_NewProtocolProcess.PushMsgInTong((const void*)&sCmd, sizeof(sCmd));
							}
						}
					}
				}
				nIdx = PlayerSet.GetNextPlayer();
			}
		}
		break;
	case SGDI_TONG_TRANSMONEY_POWER:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckTransMoney(nParam);
		}
		break;
	case SGDI_TONG_STOREBUILDFUND_CHECK:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckStoreBuildFund(nParam);
		}
		break;
	case SGDI_TONG_STOREBUILDFUND_RET:
		if(uParam)
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			STONG_STOREBUILDFUND_SYNC* pInfo = (STONG_STOREBUILDFUND_SYNC*)uParam;
			Player[nParam].m_cTong.StoreBuildFundRet(pInfo->m_nWeekOffer, pInfo->m_nAddOffer);
		}
		break;
	case SGDI_TONG_WEEKLYRESET:
		if(uParam)
		{
			int nIdx;
			nIdx = PlayerSet.GetFirstPlayer();
			while (nIdx)
			{
				if (Player[nIdx].m_cTong.GetTongNameID() == uParam)
				{
					Player[nIdx].m_cTong.ResetWeekTask(nParam);
				}
				nIdx = PlayerSet.GetNextPlayer();
			}
		}
		break;
	case SGDI_TONG_ANNOUNCE_CHECK:
		{
			nRet = 0;
			if(uParam <= 0 || uParam >= MAX_PLAYER || Player[uParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[uParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[uParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[uParam].m_cTong.CheckAnnouncePower();
		}
		break;
	case SGDI_TONG_RECEIVEPRICE_RET:
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			Player[nParam].ExecuteScript("\\script\\tong\\supportnpc.lua", "ReceiveTongPrice", uParam);
		}
		break;
	case SGDI_TONG_PLAYERPRICE_RET:
		if(uParam)
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			STONG_PLAYERPRICE_SYNC	*pInfo = (STONG_PLAYERPRICE_SYNC*)uParam;
			if(pInfo->m_btSuccessFlag == 0)
				Player[nParam].ExecuteScript("\\script\\tong\\supportnpc.lua", "ReceivePlayerPrice", pInfo->m_nPrice);
			else
				Player[nParam].ExecuteScript("\\script\\tong\\supportnpc.lua", "FailPlayerPrice", pInfo->m_btSuccessFlag);
		}
		break;
	case SGDI_TONG_CHANGETITLE_CHECK:
		nRet = 0;
		if(uParam)
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			TONG_APPLY_CHANGETITLE_COMMAND	*pCmd = (TONG_APPLY_CHANGETITLE_COMMAND*)uParam;
			char szTitle[defTONG_NAME_MAX_LENGTH+1];
			memcpy(szTitle, pCmd->m_szName, defTONG_NAME_MAX_LENGTH);
			szTitle[defTONG_NAME_MAX_LENGTH] = 0;
			nRet = Player[nParam].m_cTong.CheckChangeTitlePower(pCmd->m_nDestFigure, szTitle);
		}
		break;
	case SGDI_TONG_BE_CHANGETITLE:
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			STONG_CHANGETITLE_SYNC	*pInfo = (STONG_CHANGETITLE_SYNC*)uParam;
			if(Player[nParam].m_dwID != pInfo->m_dwPlayerNameID
			|| Player[nParam].m_cTong.GetTongNameID() != pInfo->m_dwTongNameID)
				break;
			Player[nParam].m_cTong.ChangeTitle(pInfo->m_szName);
		}
		break;
	case SGDI_TONG_ADDWEEKGOAL_RET:
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			STONG_ADDWEEKGOAL_SYNC	*pInfo = (STONG_ADDWEEKGOAL_SYNC*)uParam;
			if(Player[nParam].m_dwID != pInfo->m_dwPlayerNameID
			|| Player[nParam].m_cTong.GetTongNameID() != pInfo->m_dwTongNameID)
				break;
			Player[nParam].m_cTong.StoreBuildFundRet(pInfo->m_nWeekOffer, pInfo->m_nAddOffer);
		}
		break;
	case SGDI_TONG_CHANGETITLEALL_CHECK:
		nRet = 0;
		if(uParam)
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			TONG_APPLY_CHANGETITLE_COMMAND	*pCmd = (TONG_APPLY_CHANGETITLE_COMMAND*)uParam;
			char szTitle[defTONG_NAME_MAX_LENGTH+1];
			memcpy(szTitle, pCmd->m_szName, defTONG_NAME_MAX_LENGTH);
			szTitle[defTONG_NAME_MAX_LENGTH] = 0;
			nRet = Player[nParam].m_cTong.CheckChangeTitleAllPower(szTitle);
		}
		break;
	case SGDI_TONG_CHANGETITLEALL_RET:
		if(uParam)
		{
			STONG_CHANGETITLE_SYNC	*pInfo = (STONG_CHANGETITLE_SYNC*)uParam;
			if((int)pInfo->m_uDestID != -1)
			{
				int nIdx;
				nIdx = PlayerSet.GetFirstPlayer();
				while (nIdx)
				{
					if (Player[nIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID
					&& (Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MEMBER
					|| Player[nIdx].m_cTong.GetFigure() == enumTONG_FIGURE_MANAGER)
					&& Npc[Player[nIdx].m_nIndex].m_nSex == (int)pInfo->m_uDestID)
					{
						Player[nIdx].m_cTong.ChangeTitle(pInfo->m_szName);
					}
					nIdx = PlayerSet.GetNextPlayer();
				}
			}
			//sync cho sender (m_uDestID -1 la` thoi` gian cach')
			if(pInfo->m_nPlayerIdx > 0 && pInfo->m_nPlayerIdx < MAX_PLAYER
			&& Player[pInfo->m_nPlayerIdx].m_nIndex > 0
			&& Player[pInfo->m_nPlayerIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID
			&& Player[pInfo->m_nPlayerIdx].m_dwID == pInfo->m_dwPlayerNameID)
			{
				if((int)pInfo->m_uDestID == -1)
				{
					KPlayerChat::SendSystemMsg(pInfo->m_nPlayerIdx, "Kh«ng thÓ ®æi tªn tÊt c¶ thµnh viªn liªn tôc, xin ®îi mét thêi gian.");
					break;
				}
				TONG_CHANGETITLE_SYNC	sSync;
				sSync.ProtocolType		= s2c_extendtong;
				sSync.m_btMsgId			= enumTONG_SYNC_ID_CHANGETITLEALL;
				sSync.m_wLength			= sizeof(sSync) - 1;
				sSync.m_uDestID			= pInfo->m_uDestID;
				strcpy(sSync.m_szName, pInfo->m_szName);
				if (g_pServer)
					g_pServer->PackDataToClient(Player[pInfo->m_nPlayerIdx].m_nNetConnectIdx, &sSync, sSync.m_wLength + 1);
			}
		}
		break;
	case SGDI_TONG_CHANGECAMP_CHECK:
		{
			nRet = 0;
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
				break;
			Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
			nRet = Player[nParam].m_cTong.CheckChangeCampPower(uParam);
		}
		break;
	case SGDI_TONG_CHANGECAMP_RET:
		if(uParam)
		{
			STONG_CHANGECAMP_SYNC	*pInfo = (STONG_CHANGECAMP_SYNC*)uParam;
			if(pInfo->m_nCamp <= 0)
			{
				if(pInfo->m_nPlayerIdx > 0 && pInfo->m_nPlayerIdx < MAX_PLAYER
				&& Player[pInfo->m_nPlayerIdx].m_nIndex > 0
				&& Player[pInfo->m_nPlayerIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID
				&& Player[pInfo->m_nPlayerIdx].m_dwID == pInfo->m_dwPlayerNameID)
				{
					if(pInfo->m_nCamp == -1)
					{
						KPlayerChat::SendSystemMsg(pInfo->m_nPlayerIdx, "Kh«ng thÓ ®æi phe ph¸i bang héi liªn tôc, xin ®îi mét thêi gian.");
					}
					else
					{
						char Buff[80];
						sprintf(Buff, "Ng©n quü bang kh«ng ®ñ %d v¹n l­îng ®Ó thay ®æi phe ph¸i", pInfo->m_nMoney);
						KPlayerChat::SendSystemMsg(pInfo->m_nPlayerIdx, Buff);
					}
				}
			}
			else //if(pInfo->m_nCamp >= camp_justice && pInfo->m_nCamp <= camp_balance)
			{
				int nIdx;
				nIdx = PlayerSet.GetFirstPlayer();
				while (nIdx)
				{
					if (Player[nIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID)
					{
						Player[nIdx].m_cTong.ChangeCamp(pInfo->m_nCamp);
					}
					nIdx = PlayerSet.GetNextPlayer();
				}
				//sync cho sender, cap nhat so tien
				if(pInfo->m_nPlayerIdx > 0 && pInfo->m_nPlayerIdx < MAX_PLAYER
				&& Player[pInfo->m_nPlayerIdx].m_nIndex > 0
				&& Player[pInfo->m_nPlayerIdx].m_cTong.GetTongNameID() == pInfo->m_dwTongNameID
				&& Player[pInfo->m_nPlayerIdx].m_dwID == pInfo->m_dwPlayerNameID)
				{
					TONG_CONTRIBMONEY_SYNC	sSync;
					sSync.ProtocolType		= s2c_extendtong;
					sSync.m_btMsgId			= enumTONG_SYNC_ID_CONTRIBMONEY;
					sSync.m_wLength			= sizeof(sSync) - 1;
					sSync.m_nMoney			= pInfo->m_nMoney;
					if (g_pServer)
						g_pServer->PackDataToClient(Player[pInfo->m_nPlayerIdx].m_nNetConnectIdx, &sSync, sSync.m_wLength + 1);
				}
			}
		}
		break;
	case SGDI_TONG_ACTION:
		if(uParam)
		{
			if(nParam <= 0 || nParam >= MAX_PLAYER || Player[nParam].m_nIndex <= 0)
				break;
			int nCurTime = g_SubWorldSet.GetGameTime();
			TONG_APPLY_ACTION_COMMAND	*pInfo = (TONG_APPLY_ACTION_COMMAND*)uParam;
			switch(pInfo->m_nAction)
			{
				case TONG_ACTION_UPBUILDLEVEL:
				{
					if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					if(!Player[nParam].m_cTong.CheckCanAction(pInfo->m_nAction))
						break;

					break;
				}
				case TONG_ACTION_ENTERMAP:
				{
					if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					if(!Player[nParam].m_cTong.CheckCanAction(pInfo->m_nAction))
						break;

					break;
				}
				case TONG_ACTION_CREATEMAP:
				{
					if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					if(!Player[nParam].m_cTong.CheckCanAction(pInfo->m_nAction))
						break;

					break;
				}
				case TONG_ACTION_CONFIGMAP:
				{
					if(Player[nParam].m_cTong.m_nNextTimeMemPage >= nCurTime)
						break;
					Player[nParam].m_cTong.m_nNextTimeMemPage = nCurTime + 3;
					if(!Player[nParam].m_cTong.CheckCanAction(pInfo->m_nAction))
						break;

					break;
				}
			}
			
		}
		break;
	default:
		break;
	}
	return nRet;
}

//--------------------------------------------------------------------------
//	¹¦ÄÜ£ºÏòÓÎÏ··¢ËÍ²Ù×÷
//	²ÎÊý£ºunsigned int uDataId --> CoreÍâ²¿¿Í»§¶ÔcoreµÄ²Ù×÷ÇëÇóµÄË÷Òý¶¨Òå
//							ÆäÖµÎªÃ·¾ÙÀàÐÍGAMEOPERATION_INDEXµÄÈ¡ÖµÖ®Ò»¡£
//		  unsigned int uParam  --> ÒÀ¾ÝuOperIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//		  int nParam --> ÒÀ¾ÝuOperIdµÄÈ¡ÖµÇé¿ö¶ø¶¨
//	·µ»Ø£ºÈç¹û³É¹¦·¢ËÍ²Ù×÷ÇëÇó£¬º¯Êý·µ»Ø·Ç0Öµ£¬·ñÔò·µ»Ø0Öµ¡£
//--------------------------------------------------------------------------
int	CoreServerShell::OperationRequest(unsigned int uOper, unsigned int uParam, int nParam)
{
	int nRet = 1;
	switch(uOper)
	{		
	case SSOI_BROADCASTING:
		nRet = PlayerSet.Broadcasting((char*)uParam, nParam);
		break;
	case SSOI_LAUNCH:	//Æô¶¯·þÎñ
		nRet = OnLunch((LPVOID)uParam);
		break;
	case SSOI_SHUTDOWN:	//¹Ø±Õ·þÎñ
		nRet = OnShutdown();
		break;
	case SSOI_RELOAD_WELCOME_MSG:
		PlayerSet.ReloadWelcomeMsg();
		break;

	// ket qua sau khi tao bang hoi s3relay
	case SSOI_TONG_CREATE:
		{
			STONG_SERVER_TO_CORE_CREATE_SUCCESS	*pCreate = (STONG_SERVER_TO_CORE_CREATE_SUCCESS*)uParam;
			if (pCreate->m_nPlayerIdx <= 0 || pCreate->m_nPlayerIdx >= MAX_PLAYER
			|| Player[pCreate->m_nPlayerIdx].m_dwID != pCreate->m_dwPlayerNameID)
			{
				nRet = 0;
				break;
			}
			nRet = Player[pCreate->m_nPlayerIdx].m_cTong.Create(pCreate->m_nCamp, pCreate->m_szTongName);
		}
		break;

	case SSOI_TONG_REFUSE_ADD:
		if (uParam)
		{
			STONG_SERVER_TO_CORE_REFUSE_ADD	*pRefuse = (STONG_SERVER_TO_CORE_REFUSE_ADD*)uParam;
			if (pRefuse->m_nSelfIdx > 0 && pRefuse->m_nSelfIdx < MAX_PLAYER)
			{
				Player[pRefuse->m_nSelfIdx].m_cTong.SendRefuseMessage(pRefuse->m_nTargetIdx, pRefuse->m_dwNameID);
			}
		}
		break;
	//ket qua sau khi add member
	case SSOI_TONG_ADD:
		if (uParam)
		{
			nRet = 0;
			STONG_SERVER_TO_CORE_ADD_SUCCESS	*pAdd = (STONG_SERVER_TO_CORE_ADD_SUCCESS*)uParam;
			if (pAdd->m_nPlayerIdx <= 0 || pAdd->m_nPlayerIdx >= MAX_PLAYER
			|| Player[pAdd->m_nPlayerIdx].m_dwID != pAdd->m_dwPlayerNameID)
				break;
			nRet = Player[pAdd->m_nPlayerIdx].m_cTong.AddTong(
				pAdd->m_nCamp,
				pAdd->m_szTongName,
				pAdd->m_szMasterName,
				pAdd->m_nFigure);
		}
		break;
	case SSOI_TONG_CREATE_FAIL:
	{
		if(nParam > 0 && nParam < MAX_PLAYER && Player[nParam].m_dwID == uParam)
		{
			KPlayerChat::SendSystemMsg(nParam, "Thµnh lËp bang héi thÊt b¹i");
		}
		break;
	}
	default:
		nRet = 0;
		break;
	}	
	return nRet;
}

void CoreServerShell::SetServerTrans(LPVOID pServer, unsigned int lnID) //guve
{
	g_pTransServer = reinterpret_cast< IServer * >(pServer);
	g_nTransID = lnID;
}

void CoreServerShell::SetServerChat(LPVOID pServer, unsigned int lnID)
{
	g_pChatServer = reinterpret_cast< IServer * >(pServer);
	g_nChatID = lnID;
}

void CoreServerShell::SetServerTong(LPVOID pServer, unsigned int lnID)
{
	g_pTongServer = reinterpret_cast< IServer * >(pServer);
	g_nTongID = lnID;
}

int CoreServerShell::OnLunch(LPVOID pServer)
{
	g_SetServer(pServer);

//	g_SetFilePath("\\script");
	KLuaScript * pStartScript =(KLuaScript*) g_GetScript("\\script\\ServerScript.lua");
	int i = 0;
	
	if (!pStartScript)
		g_DebugLog("Load ServerScript failed!");
	else
	{	
		pStartScript->CallFunction("StartGame",0,"");
	}

	PlayerSet.ReloadWelcomeMsg();

	return true;
}

int CoreServerShell::OnShutdown()
{
	return true;
}

//ÈÕ³£»î¶¯£¬coreÈç¹ûÒªÊÙÖÕÕýÇÞÔò·µ»Ø0£¬·ñÔò·µ»Ø·Ç0Öµ
int CoreServerShell::Breathe()
{
	g_SubWorldSet.MessageLoop();
	g_SubWorldSet.MainLoop();
	return true;
}

bool CoreServerShell::CheckProtocolSize(const char* pChar, int nSize)
{
	WORD wCheckSize;
	BYTE nProtocol = (BYTE)pChar[0];

	if (nProtocol >= c2s_end || nProtocol <= c2s_gameserverbegin)
	{
		g_DebugLog("[error]NetServer:Invalid Protocol!");
		return false;
	}

	if (g_nProtocolSize[nProtocol - c2s_gameserverbegin - 1] == -1)
	{
		wCheckSize = *(WORD*)&pChar[1] + PROTOCOL_MSG_SIZE;
	}
	else
	{
		wCheckSize = g_nProtocolSize[nProtocol - c2s_gameserverbegin - 1];
	}
	if (wCheckSize != nSize)
	{
		g_DebugLog("[error]ÍøÂç½ÓÊÕÐ­Òé´óÐ¡²»Æ¥Åä");
#ifndef _WIN32
		printf("[error]ÍøÂç½ÓÊÕÐ­Òé´óÐ¡²»Æ¥Åä<%d>, should %d, but %d\n", nProtocol, wCheckSize, nSize);
#endif
		return false;
	}
	return true;
}


int CoreServerShell::AttachPlayer(const unsigned long lnID, GUID* pGuid)
{
	return PlayerSet.AttachPlayer(lnID, pGuid);
}

void* CoreServerShell::SavePlayerDataAtOnce(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
	{
		return NULL;
	}

	if (Player[nIndex].Save())
	{
		Player[nIndex].m_uMustSave = SAVE_REQUEST;
		return &Player[nIndex].m_SaveBuffer;
	}
	else
	{
		return NULL;
	}
}

bool CoreServerShell::IsCharacterQuiting(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
	{
		return FALSE;
	}	
	return Player[nIndex].IsWaitingRemove();
}

bool CoreServerShell::IsPlayerLoginTimeOut(int nIndex)
{
	return Player[nIndex].IsLoginTimeOut();
}

void CoreServerShell::RemovePlayerLoginTimeOut(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;
	if (Player[nIndex].IsLoginTimeOut())
	{
		PlayerSet.RemoveLoginTimeOut(nIndex);
	}
}


int CoreServerShell::AddTempTaskValue(int nIndex, const char* pData)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return 0;

	return Player[nIndex].AddTempTaskValue((void *)pData);
}

void CoreServerShell::GetPlayerIndexByGuid(GUID* pGuid, int* pnIndex, int* plnID)
{
	*pnIndex = PlayerSet.GetPlayerIndexByGuid(pGuid);
	if (*pnIndex)
	{
		*plnID = Player[*pnIndex].m_nNetConnectIdx;
	}
	else
	{
		*plnID = -1;
	}

	if (*plnID == -1)
	{
		*pnIndex = 0;
	}
}

void* CoreServerShell::PreparePlayerForExchange(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return NULL;
	PlayerSet.PrepareExchange(nIndex);
	return &Player[nIndex].m_SaveBuffer;
}

bool CoreServerShell::IsPlayerExchangingServer(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return false;

	return Player[nIndex].IsExchangingServer();
}

void CoreServerShell::RemovePlayerForExchange(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;

	PlayerSet.RemoveExchanging(nIndex);
}

void CoreServerShell::GetGuid(int nIndex, void* pGuid)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;
	memcpy(pGuid, &Player[nIndex].m_Guid, sizeof(GUID));
}

DWORD CoreServerShell::GetExchangeMap(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return -1;

	return Player[nIndex].m_sExchangePos.m_dwMapID;
}

void CoreServerShell::RecoverPlayerExchange(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;

	Player[nIndex].m_bExchangeServer = FALSE;
	if (Player[nIndex].m_nIndex > 0)
	{
		KNpc* pNpc = &Npc[Player[nIndex].m_nIndex];
		pNpc->m_bExchangeServer = FALSE;
		pNpc->m_FightMode = pNpc->m_OldFightMode;
	}
	Player[nIndex].Earn(Player[nIndex].m_nPrePayMoney);
	Player[nIndex].m_nPrePayMoney = 0;
}

void CoreServerShell::SetSaveStatus(int nIndex, UINT uStatus)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;

	Player[nIndex].m_uMustSave = uStatus;
}

UINT CoreServerShell::GetSaveStatus(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return SAVE_IDLE;

	return Player[nIndex].m_uMustSave;
}


void CoreServerShell::PreparePlayerForLoginFailed(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return;

	PlayerSet.PrepareLoginFailed(nIndex);
}
//BOOL CoreServerShell::ValidPingTime(int nIndex)
//{
//	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
//		return FALSE;
//
//	if (Player[nIndex].m_uLastPingTime == -1)
//		return TRUE;
//	
//#define	MAX_PING_TIME	(60 * 20)	//	1min
//	if (g_SubWorldSet.GetGameTime() - Player[nIndex].m_uLastPingTime > MAX_PING_TIME)
//	{
//		return FALSE;
//	}
//	return TRUE;
//}

BOOL CoreServerShell::GroupChat(IClient* pClient, DWORD FromIP, unsigned long FromRelayID, DWORD channid, BYTE tgtcls, DWORD tgtid, const void* pData, size_t size)
{
	switch(tgtcls)
	{

	case tgtcls_team:
		{{
		if (tgtid < 0 || tgtid >= MAX_TEAM)
			return FALSE;

		size_t pckgsize = sizeof(tagExtendProtoHeader) + size;
#ifdef WIN32
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
		pExHeader->ProtocolType = s2c_extendchat;
		pExHeader->wLength = pckgsize - 1;
		memcpy(pExHeader + 1, pData, size);

		int nTargetIdx;
		// ¸ø¶Ó³¤·¢
		nTargetIdx = g_Team[tgtid].m_nCaptain;
//		if (FromRelayID != Player[nTargetIdx].m_nNetConnectIdx)
			g_pServer->SendData(Player[nTargetIdx].m_nNetConnectIdx, pData, size);
		// ¸ø¶ÓÔ±·¢
		for (int i = 0; i <	MAX_TEAM_MEMBER; i++)
		{
			nTargetIdx = g_Team[tgtid].m_nMember[i];
			if (nTargetIdx < 0)
				continue;

//			if (FromRelayID != Player[nTargetIdx].m_nNetConnectIdx)
				g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);
		}
#ifndef WIN32
		delete ((char*)pExHeader);
#endif
		}}
		break;

	case tgtcls_fac:
		{{
		size_t pckgsize = sizeof(tagExtendProtoHeader) + size;
#ifdef WIN32
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
		pExHeader->ProtocolType = s2c_extendchat;
		pExHeader->wLength = pckgsize - 1;
		memcpy(pExHeader + 1, pData, size);

		int nTargetIdx;
		nTargetIdx = PlayerSet.GetFirstPlayer();
		while (nTargetIdx)
		{
			if (Player[nTargetIdx].m_cFaction.m_nCurFaction == tgtid
)//				&& FromRelayID != Player[nTargetIdx].m_nNetConnectIdx)
				g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);

			nTargetIdx = PlayerSet.GetNextPlayer();
		}
#ifndef WIN32
		delete ((char*)pExHeader);
#endif
		}}
		break;

	case tgtcls_tong:
		{{
		size_t pckgsize = sizeof(tagExtendProtoHeader) + size;
#ifdef WIN32
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
		pExHeader->ProtocolType = s2c_extendchat;
		pExHeader->wLength = pckgsize - 1;
		memcpy(pExHeader + 1, pData, size);

		int nTargetIdx;
		nTargetIdx = PlayerSet.GetFirstPlayer();
		while (nTargetIdx)
		{
			if (Player[nTargetIdx].m_cTong.GetTongNameID() == tgtid
)//				&& FromRelayID != Player[nTargetIdx].m_nNetConnectIdx)
				g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);

			nTargetIdx = PlayerSet.GetNextPlayer();
		}
#ifndef WIN32
		delete ((char*)pExHeader);
#endif
		}}
		break;

	case tgtcls_scrn:
		{{

//		int nMaxRelayPlayer = (1024 - 32 - sizeof(CHAT_GROUPMAN) - size) / sizeof(WORD);
//		if (nMaxRelayPlayer <= 0)
//			return FALSE;


		int idxNPC = Player[tgtid].m_nIndex;
		int idxSubWorld = Npc[idxNPC].m_SubWorldIndex;
		int idxRegion = Npc[idxNPC].m_RegionIndex;
//		_ASSERT(idxSubWorld >= 0 && idxRegion >= 0);
		int nOX = Npc[idxNPC].m_MapX;
		int nOY = Npc[idxNPC].m_MapY;
		int nTX = 0;
		int nTY = 0;
		if (idxSubWorld < 0 || idxRegion < 0)
			return FALSE;


//		size_t basesize = sizeof(CHAT_GROUPMAN) + size;
//		BYTE buffer[1024];
//
//		CHAT_GROUPMAN* pCgc = (CHAT_GROUPMAN*)buffer;
//		pCgc->ProtocolType = chat_groupman;
//		pCgc->wChatLength = size;
//		pCgc->byHasIdentify = false;
//
//		void* pExPckg = pCgc + 1;
//		memcpy(pExPckg, pData, size);
//
//		WORD* pPlayers = (WORD*)((BYTE*)pExPckg + size);
//
//
//		pCgc->wPlayerCount = 0;


		size_t pckgsize = sizeof(tagExtendProtoHeader) + size;
#ifdef WIN32
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
		pExHeader->ProtocolType = s2c_extendchat;
		pExHeader->wLength = pckgsize - 1;
		memcpy(pExHeader + 1, pData, size);

#define	MAX_SYNC_RANGE	23
		static POINT POff[9] = 
		{
			{0, 0},
			{0, 32},
			{-16, 32},
			{-16, 0},
			{-16, -32},
			{0, -32},
			{16, -32},
			{16, 0},
			{16, 32},
		};

		KRegion* pRegionBase = &SubWorld[idxSubWorld].m_Region[idxRegion];

		for (int i = -1; i < 8; i++)
		{
			KRegion* pRegion = NULL;
			if (i < 0)
				pRegion = pRegionBase;
			else
			{
				if (pRegionBase->m_nConnectRegion[i] < 0)
					continue;
				pRegion = &SubWorld[idxSubWorld].m_Region[pRegionBase->m_nConnectRegion[i]];
			}
			if (pRegion == NULL)
				continue;


			KIndexNode *pNode = (KIndexNode *)pRegion->m_PlayerList.GetHead();
			while(pNode)
			{
//				_ASSERT(pNode->m_nIndex > 0 && pNode->m_nIndex < MAX_PLAYER);

				//if (FromRelayID != Player[pNode->m_nIndex].m_nNetConnectIdx)
				{
					int nTargetNpc = Player[pNode->m_nIndex].m_nIndex;
					if (nTargetNpc > 0)
					{
						nTX = Npc[nTargetNpc].m_MapX + POff[i + 1].x;
						nTY = Npc[nTargetNpc].m_MapY + POff[i + 1].y;
						
						if ((nTX - nOX) * (nTX - nOX) + (nTY - nOY) * (nTY - nOY) < MAX_SYNC_RANGE * MAX_SYNC_RANGE)
							g_pServer->PackDataToClient(Player[pNode->m_nIndex].m_nNetConnectIdx, pExHeader, pckgsize);
					}


//					pPlayers[pCgc->wPlayerCount] = (WORD)Player[pNode->m_nIndex].m_nNetConnectIdx;
//					++ pCgc->wPlayerCount;
//
//					if (pCgc->wPlayerCount >= nMaxRelayPlayer)
//					{
//						size_t pckgsize = basesize + sizeof(WORD) * pCgc->wPlayerCount;
//						pCgc->wSize = pckgsize - 1;
//						
//						pClient->SendPackToServer(pCgc, pckgsize);
//
//						pCgc->wPlayerCount = 0;
//					}
				}

				pNode = (KIndexNode *)pNode->GetNext();
			}
		}

//		if (pCgc->wPlayerCount > 0)
//		{
//			size_t pckgsize = basesize + sizeof(WORD) * pCgc->wPlayerCount;
//			pCgc->wSize = pckgsize - 1;
//
//			pClient->SendPackToServer(pCgc, pckgsize);
//		}

#ifndef WIN32
		delete (char*)pExHeader;
#endif
		}}
		break;


	case tgtcls_bc:
		{{
		size_t pckgsize = sizeof(tagExtendProtoHeader) + size;
#ifdef WIN32
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
		tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
		pExHeader->ProtocolType = s2c_extendchat;
		pExHeader->wLength = pckgsize - 1;
		memcpy(pExHeader + 1, pData, size);

		int nTargetIdx;
		nTargetIdx = PlayerSet.GetFirstPlayer();
		while (nTargetIdx)
		{
			g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);

			nTargetIdx = PlayerSet.GetNextPlayer();
		}
#ifndef WIN32
		delete ((char*)pExHeader);
#endif
		}}
		break;


	default:
		break;
	}
	return TRUE;
}

void CoreServerShell::SetLadder(void* pData, size_t uSize)
{
	Ladder.Init(pData, uSize);
}

BOOL CoreServerShell::PayForSpeech(int nIndex, int nType)
{
	if (nIndex <= 0 || nIndex >= MAX_PLAYER)
		return FALSE;

	int nNpcIdx = Player[nIndex].m_nIndex;
	if (nNpcIdx <= 0)
		return FALSE;
	if (Player[nIndex].m_nForbiddenFlag & KPlayer::FF_CHAT)	//±»½ûÑÔ
		return FALSE;
	int nLevel = Npc[nNpcIdx].m_Level;
	int nMaxMana = Npc[nNpcIdx].m_CurrentManaMax;
	switch (nType)
	{
	case 0:		//khong ton gi
		return TRUE;
		break;
	case 1:		//10% mana
		{
			return Npc[nNpcIdx].Cost(attrib_mana, nMaxMana / 10);
		}
		break;
	case 2:		//2: 20Lv + 20% mana
		{
			if (nLevel < 20)
				return FALSE;
			return Npc[nNpcIdx].Cost(attrib_mana, nMaxMana / 5);
		}
		break;
	case 3:		//30Lv + 80% mana
		{
			if (nLevel < 30)
				return FALSE;
			return Npc[nNpcIdx].Cost(attrib_mana, nMaxMana * 4 / 5);
		}
		break;
	default:
		return FALSE;	//²»ÈÏÊ¶µÄÀà±ð²»·¢ËÍ
	}
}