//---------------------------------------------------------------------------
// Sword3 Engine (c) 2002 by Kingsoft
//
// File:	KPlayerChat.h
// Date:	2002.10.05
// Code:	边城浪子
// Desc:	PlayerChat Class
//---------------------------------------------------------------------------

#ifndef KPLAYERCHAT_H
#define KPLAYERCHAT_H

#include	"GameDataDef.h"
#ifndef _SERVER
#include	"KNode.h"
#include	"KList.h"
#endif
#include <string>

#ifdef _SERVER
#include	<list>
#endif

class KPlayerChat
{
public:
	void			Release();

#ifdef _SERVER
	// 发送系统消息 nType = 0 给全体玩家发送 nType == 1 给某个特定玩家发送
	static	void	SendSystemInfo(int nType, int nTargetIdx, char *lpszSendName, char *lpszSentence, int nSentenceLength);
	static	void	SendGlobalSystemInfo(char *lpszSendName, char *lpszSentence, int nSentenceLength);
	typedef std::list<std::string>	STRINGLIST;
	static	void	MakeBrother(const STRINGLIST& brothers);
	static	void	MakeEnemy(char* szPlayer, char* szEnemy);
	static	void	SendInfoToGM(char *lpszAccName, char *lpszRoleName, char *lpszSentence, int nSentenceLength);
	static	void	SendInfoToIP(DWORD nIP, DWORD nID, char *lpszAccName, char *lpszRoleName, char *lpszSentence, int nSentenceLength);
	static	void	SendSystemMsg(int nTargetIdx, const char *pMsg);//guve
#endif

};

#endif

