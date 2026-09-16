//---------------------------------------------------------------------------
// Sword3 Engine (c) 2002 by Kingsoft
//
// File:	KPlayerChat.cpp
// Date:	2002.10.05
// Code:	边城浪子
// Desc:	PlayerChat Class
//---------------------------------------------------------------------------


#include	"KCore.h"
#ifdef _SERVER
//#include	"KNetServer.h"
//#include "../MultiServer/Heaven/Interface/iServer.h"
#include	"KGMCommand.h"
#else
//#include	"KNetClient.h"
#include "../../Headers/IClient.h"
#include	"CoreShell.h"
#endif
#include	"KNpc.h"
#include	"KNpcSet.h"
#include	"KSubWorld.h"
#include	"KPlayer.h"
#include	"KPlayerSet.h"
#include	"KPlayerTeam.h"
#include	"KPlayerChat.h"
#include	"MsgGenreDef.h"
#include	"CoreUseNameDef.h"

#include	"malloc.h"
#ifdef _SERVER
#include "KNewProtocolProcess.h"
#endif
#include "KTongProtocol.h"

void	KPlayerChat::Release()
{
}

//---------------------------------------------------------------------------
//	功能：发送系统消息
//  nType == 0 给全体玩家发送 nType == 1 给某个特定玩家发送
//  dwTargetID:  if (nType == 1) 目标玩家在 player 数组中的位置
//  lpszSendName:  发送者名字，可能是 系统消息、通知、警告、注意 等等 长度不超过 32
//  lpszSentence:  需要发送的语句
//  nSentenceLength:  需要发送语句的长度
//---------------------------------------------------------------------------
#ifdef _SERVER
void	KPlayerChat::SendSystemMsg(int nTargetIdx, const char *pMsg)
{
	if (!pMsg)
		return;
	if (nTargetIdx <= 0 || nTargetIdx >= MAX_PLAYER || Player[nTargetIdx].m_nNetConnectIdx < 0)
		return;
	char szPack[256];
	char szMsg[128];
	g_StrCpyLen(szMsg, pMsg, sizeof(szMsg));
	DYNAMIC_COMMAND* pCmd = (DYNAMIC_COMMAND*)&szPack[0];
	pCmd->ProtocolType = s2c_dynamic_structure;
	pCmd->nBranch = s2cdnmbr_msginfo;
	pCmd->m_wLength = sizeof(DYNAMIC_COMMAND) - 1 + strlen(szMsg)+1;
	strcpy((char*)(pCmd+1), szMsg);
	g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pCmd, pCmd->m_wLength + 1);
}

void	KPlayerChat::SendSystemInfo(int nType, int nTargetIdx, char *lpszSendName, char *lpszSentence, int nSentenceLength)
{
	if (!lpszSendName || !lpszSentence)
		return;
	if (nSentenceLength >= MAX_SENTENCE_LENGTH)
		nSentenceLength = MAX_SENTENCE_LENGTH;


	BOOL bAll = nType == 0;


	size_t chatsize = sizeof(CHAT_CHANNELCHAT_SYNC) + nSentenceLength;
	size_t pckgsize = sizeof(tagExtendProtoHeader) + chatsize;

#ifdef WIN32
	tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
	tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
	pExHeader->ProtocolType = s2c_extendchat;
	pExHeader->wLength = pckgsize - 1;

	CHAT_CHANNELCHAT_SYNC* pCccSync= (CHAT_CHANNELCHAT_SYNC*)(pExHeader + 1);
	pCccSync->ProtocolType = chat_channelchat;
	pCccSync->wSize = chatsize - 1;
	pCccSync->packageID = -1;
	strncpy(pCccSync->someone, lpszSendName, _NAME_LEN - 1); // 可能需要根据玩家身份改动
	pCccSync->channelid = -1;
	pCccSync->sentlen = nSentenceLength;
	memcpy(pCccSync + 1, lpszSentence, nSentenceLength);
	if (bAll)	// 给全体玩家发送
	{
		int nTargetIdx;
		nTargetIdx = PlayerSet.GetFirstPlayer();
		while (nTargetIdx)
		{
			g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);
			nTargetIdx = PlayerSet.GetNextPlayer();
		}
	}
	else			// 给某个特定玩家发送
	{
		if (nTargetIdx <= 0)
		{
#ifndef WIN32
			if (pExHeader)
				delete ((char*)pExHeader);
#endif
			return;
		}
		g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, pExHeader, pckgsize);
	}

#ifndef WIN32
			if (pExHeader)
				delete ((char*)pExHeader);
#endif
	return;


/*
	if (!lpszSendName || !lpszSentence)
		return;
	if (nSentenceLength >= MAX_SENTENCE_LENGTH)
		nSentenceLength = MAX_SENTENCE_LENGTH;
	if (nType == 0)	// 给全体玩家发送
	{
		PLAYER_SEND_CHAT_SYNC	sChat;
		sChat.ProtocolType = s2c_playersendchat;
		sChat.m_btCurChannel = CHAT_CUR_CHANNEL_SYSTEM;
		sChat.m_dwSourceID = 0;							// 发送者的player id 可能需要根据玩家身份改动
		sChat.m_btNameLen = strlen(lpszSendName);
		if (sChat.m_btNameLen >= 32)
			sChat.m_btNameLen = 31;
		if (lpszPrefix && nPrefixLen > 0)
		{
			sChat.m_btChatPrefixLen = nPrefixLen;
			if (sChat.m_btChatPrefixLen >= CHAT_MSG_PREFIX_MAX_LEN)
				sChat.m_btChatPrefixLen = CHAT_MSG_PREFIX_MAX_LEN - 1;
		}
		else
		{
			sChat.m_btChatPrefixLen = 0;
		}
		sChat.m_wSentenceLen = nSentenceLength;
		sChat.m_wLength = sizeof(PLAYER_SEND_CHAT_SYNC) - 1 - sizeof(sChat.m_szSentence) + sChat.m_btNameLen + sChat.m_btChatPrefixLen + sChat.m_wSentenceLen;
		memcpy(&sChat.m_szSentence[0], lpszSendName, sChat.m_btNameLen); // 可能需要根据玩家身份改动
		if (lpszPrefix)
			memcpy(&sChat.m_szSentence[sChat.m_btNameLen], lpszPrefix, sChat.m_btChatPrefixLen);
		memcpy(&sChat.m_szSentence[sChat.m_btNameLen + sChat.m_btChatPrefixLen], lpszSentence, sChat.m_wSentenceLen); // 可能需要根据玩家身份改动
		int nTargetIdx;
		nTargetIdx = PlayerSet.GetFirstPlayer();
		while (nTargetIdx)
		{
			g_pServer->PackDataToClient(Player[nTargetIdx].m_nNetConnectIdx, (BYTE*)&sChat, sChat.m_wLength + 1);
			nTargetIdx = PlayerSet.GetNextPlayer();
		}
	}
	else			// 给某个特定玩家发送
	{
		int nIdx;
		nIdx = nTargetIdx;
//		nIdx = PlayerSet.FindSame(dwTargetID);
		if (nIdx <= 0)
			return;
		PLAYER_SEND_CHAT_SYNC	sChat;
		sChat.ProtocolType = s2c_playersendchat;
		sChat.m_btCurChannel = CHAT_CUR_CHANNEL_SYSTEM;
		sChat.m_dwSourceID = 0;							// 发送者的player id 可能需要根据玩家身份改动
		sChat.m_btNameLen = strlen(lpszSendName);
		if (sChat.m_btNameLen >= 32)
			sChat.m_btNameLen = 31;
		if (lpszPrefix && nPrefixLen > 0)
		{
			sChat.m_btChatPrefixLen = nPrefixLen;
			if (sChat.m_btChatPrefixLen >= CHAT_MSG_PREFIX_MAX_LEN)
				sChat.m_btChatPrefixLen = CHAT_MSG_PREFIX_MAX_LEN - 1;
		}
		else
		{
			sChat.m_btChatPrefixLen = 0;
		}
		sChat.m_wSentenceLen = nSentenceLength;
		sChat.m_wLength = sizeof(PLAYER_SEND_CHAT_SYNC) - 1 - sizeof(sChat.m_szSentence) + sChat.m_btNameLen + sChat.m_btChatPrefixLen + sChat.m_wSentenceLen;
		memcpy(&sChat.m_szSentence[0], lpszSendName, sChat.m_btNameLen); // 可能需要根据玩家身份改动
		if (lpszPrefix)
			memcpy(&sChat.m_szSentence[sChat.m_btNameLen], lpszPrefix, sChat.m_btChatPrefixLen);
		memcpy(&sChat.m_szSentence[sChat.m_btNameLen + sChat.m_btChatPrefixLen], lpszSentence, sChat.m_wSentenceLen); // 可能需要根据玩家身份改动
		g_pServer->PackDataToClient(Player[nIdx].m_nNetConnectIdx, (BYTE*)&sChat, sChat.m_wLength + 1);
	}
*/
}
#endif

#ifdef _SERVER
void KPlayerChat::SendGlobalSystemInfo(char *lpszSendName, char *lpszSentence, int nSentenceLength)
{
	if (!lpszSendName || !lpszSentence)
		return;
	if (nSentenceLength >= MAX_SENTENCE_LENGTH)
		nSentenceLength = MAX_SENTENCE_LENGTH;

	size_t chatsize = sizeof(CHAT_CHANNELCHAT_SYNC) + nSentenceLength;
	size_t pckgsize = sizeof(tagExtendProtoHeader) + chatsize;

#ifdef WIN32
	tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)_alloca(pckgsize);
#else
	tagExtendProtoHeader* pExHeader = (tagExtendProtoHeader*)(new char[pckgsize]);
#endif
	pExHeader->ProtocolType = s2c_extendchat;
	pExHeader->wLength = pckgsize - 1;

	CHAT_CHANNELCHAT_SYNC* pCccSync= (CHAT_CHANNELCHAT_SYNC*)(pExHeader + 1);
	pCccSync->ProtocolType = chat_channelchat;
	pCccSync->wSize = chatsize - 1;
	pCccSync->packageID = -1;
	strncpy(pCccSync->someone, lpszSendName, _NAME_LEN - 1); // 可能需要根据玩家身份改动
	pCccSync->channelid = -1;
	pCccSync->sentlen = nSentenceLength;
	memcpy(pCccSync + 1, lpszSentence, nSentenceLength);

	g_NewProtocolProcess.BroadcastGlobal(pExHeader, pckgsize);

#ifndef WIN32
	if (pExHeader)
		delete ((char*)pExHeader);
#endif
}
#endif

#ifdef _SERVER
void KPlayerChat::SendInfoToGM(char *lpszAccName, char *lpszRoleName, char *lpszSentence, int nSentenceLength)
{
	if (!lpszAccName ||
		!lpszRoleName ||
		!lpszSentence)
		return;
	if (nSentenceLength >= MAX_SENTENCE_LENGTH)
		nSentenceLength = MAX_SENTENCE_LENGTH;

	size_t chatsize = sizeof(CHAT_MSG_EX) + nSentenceLength;
	size_t pckgsize = sizeof(RELAY_ASKWAY_DATA) + chatsize;


#ifdef WIN32
	RELAY_ASKWAY_DATA* pExHeader = (RELAY_ASKWAY_DATA*)_alloca(pckgsize);
#else
	RELAY_ASKWAY_DATA* pExHeader = (RELAY_ASKWAY_DATA*)(new char[pckgsize]);
#endif
	pExHeader->ProtocolFamily = pf_relay;
	pExHeader->ProtocolID = relay_c2c_askwaydata;
	pExHeader->nFromIP = 0;
	pExHeader->nFromRelayID = 0;
	pExHeader->seekRelayCount = 0;
	pExHeader->seekMethod = rm_gm;
	pExHeader->wMethodDataLength = 0;
	pExHeader->routeDateLength = chatsize;

	CHAT_MSG_EX* pChatMsgEx= (CHAT_MSG_EX*)(pExHeader + 1);
	pChatMsgEx->ProtocolFamily = pf_playercommunity;
	pChatMsgEx->ProtocolID = playercomm_channelchat;
	strcpy(pChatMsgEx->m_szSourceName, lpszRoleName);
	strcpy(pChatMsgEx->m_szAccountName, lpszAccName);
	pChatMsgEx->SentenceLength = nSentenceLength;
	memcpy(pChatMsgEx + 1, lpszSentence, nSentenceLength);

	g_NewProtocolProcess.PushMsgInTransfer(pExHeader, pckgsize);

#ifndef WIN32
	if (pExHeader)
		delete ((char*)pExHeader);
#endif
}
#endif

#ifdef _SERVER
void KPlayerChat::SendInfoToIP(DWORD nIP, DWORD nID, char *lpszAccName, char *lpszRoleName, char *lpszSentence, int nSentenceLength)
{
	if (!lpszAccName ||
		!lpszRoleName ||
		!lpszSentence)
		return;
	if (nSentenceLength >= MAX_SENTENCE_LENGTH)
		nSentenceLength = MAX_SENTENCE_LENGTH;

	size_t chatsize = sizeof(CHAT_MSG_EX) + nSentenceLength;
	size_t pckgsize = sizeof(RELAY_DATA) + chatsize;


#ifdef WIN32
	RELAY_DATA* pExHeader = (RELAY_DATA*)_alloca(pckgsize);
#else
	RELAY_DATA* pExHeader = (RELAY_DATA*)(new char[pckgsize]);
#endif
	pExHeader->ProtocolFamily = pf_relay;
	pExHeader->ProtocolID = relay_c2c_data;
	pExHeader->nToIP = nIP;
	pExHeader->nToRelayID = nID;
	pExHeader->nFromIP = 0;
	pExHeader->nFromRelayID = 0;
	pExHeader->routeDateLength = chatsize;

	CHAT_MSG_EX* pChatMsgEx= (CHAT_MSG_EX*)(pExHeader + 1);
	pChatMsgEx->ProtocolFamily = pf_playercommunity;
	pChatMsgEx->ProtocolID = playercomm_channelchat;
	strcpy(pChatMsgEx->m_szSourceName, lpszRoleName);
	strcpy(pChatMsgEx->m_szAccountName, lpszAccName);
	pChatMsgEx->SentenceLength = nSentenceLength;
	memcpy(pChatMsgEx + 1, lpszSentence, nSentenceLength);

	g_NewProtocolProcess.PushMsgInTransfer(pExHeader, pckgsize);

#ifndef WIN32
	if (pExHeader)
		delete ((char*)pExHeader);
#endif
}
#endif

#ifdef _SERVER
#define BROTHER_UNITNAME "亲人\n"
void KPlayerChat::MakeBrother(const STRINGLIST& brothers)
{
	if (brothers.size() == 0)
		return;

	static const size_t max_packagesize = 1000;
	char buffer[max_packagesize];	//max package size
	size_t maxsize = max_packagesize - 1;	//留个0的位置
	size_t basesize = sizeof(FRIEND_ASSOCIATEBEVY);

	FRIEND_ASSOCIATEBEVY* pGf = (FRIEND_ASSOCIATEBEVY*)(buffer);
	pGf->ProtocolFamily = pf_friend;
	pGf->ProtocolID = friend_c2s_associatebevy;

	int nG = strlen(BROTHER_UNITNAME) + 1;
	strcpy(buffer + basesize, BROTHER_UNITNAME);
	basesize += nG;

	size_t cursor = basesize;

	for (STRINGLIST::const_iterator itFriend = brothers.begin(); itFriend != brothers.end(); itFriend++)
	{
		std::string dst = *itFriend;
		size_t appendsize = dst.size() + 1;

		if (cursor + appendsize > maxsize)
		{
			buffer[cursor++] = specOver;	//加个结尾,发走

			g_NewProtocolProcess.PushMsgInTong(buffer, cursor);

			cursor = basesize;	//从头开始
		}

		strcpy(buffer + cursor, dst.c_str());
		cursor += appendsize;
	}

	if (cursor > basesize)
	{
		buffer[cursor++] = specOver;

		g_NewProtocolProcess.PushMsgInTong(buffer, cursor);
	}
}
#endif

#ifdef _SERVER
#define ENEMY_UNITNAME	 "仇人\n"
void KPlayerChat::MakeEnemy(char* szPlayer, char* szEnemy)
{
	if (!szPlayer || szPlayer[0] == 0 ||
		!szEnemy || szEnemy[0] == 0)
		return;

	int nG = strlen(ENEMY_UNITNAME) + 1;
	int nP = strlen(szPlayer) + 1;
	int nE = strlen(szEnemy) + 1;
	size_t fsize = sizeof(FRIEND_ASSOCIATE) + nG + nP + nE;

#ifdef WIN32
	FRIEND_ASSOCIATE* pCccSync = (FRIEND_ASSOCIATE*)_alloca(fsize);
#else
	FRIEND_ASSOCIATE* pCccSync = (FRIEND_ASSOCIATE*)(new char[fsize]);
#endif
	pCccSync->ProtocolFamily = pf_friend;
	pCccSync->ProtocolID = friend_c2s_associate;
	pCccSync->bidir = 0;
	char* pBuf = (char*)(pCccSync + 1);
	strcpy(pBuf, ENEMY_UNITNAME);
	pBuf += nG;
	strcpy(pBuf, szPlayer);
	pBuf += nP;
	strcpy(pBuf, szEnemy);
	pBuf += nE;

	g_NewProtocolProcess.PushMsgInTong(pCccSync, fsize);

#ifndef WIN32
	if (pCccSync)
		delete ((char*)pCccSync);
#endif
}
#endif



