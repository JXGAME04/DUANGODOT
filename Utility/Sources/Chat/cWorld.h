// cWorld.h: interface for the cWorld class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CWORLD_H__E10BE219_0AB5_40E9_8C0B_EC014477682A__INCLUDED_)
#define AFX_CWORLD_H__E10BE219_0AB5_40E9_8C0B_EC014477682A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cPlayer;
const int c_nMaxPlayer = c_nMaxLink;

class cThread;

#include "cSafeQueue.h"

struct stBindMsg
{
	cPlayer* pPlayer;
	iServerLink* pLink;
	DWORD msgSize;
	stMsg* pMsg;
};

class cWorld  
{
public:
	cWorld();
	virtual ~cWorld();

	cThread* m_pThread;
	BOOL Run();
	BOOL RunThread();
	BOOL EndThread();

	cSafeQueue<stBindMsg> m_queue;
	BOOL PopMessage(stBindMsg& msg) {return m_queue.Pop(msg);};
	void PushMessage(stMsg* p,DWORD size,cPlayer* pPlayer,iServerLink* pLink);

	cPlayer* m_aPlayer[c_nMaxPlayer];
	cPlayer* CreatePlayer();
	cPlayer* FindPlayer(LPCSTR szAccount,BOOL bCreate = FALSE);
	void QuitPlayer(cPlayer* p);

	BOOL m_bWaitingAnswer;
	int m_nWaitTime;
	BOOL IsWaitingAnswer(){return m_bWaitingAnswer;}
	void SetWaitingAnswer(BOOL b){m_bWaitingAnswer = b;m_nWaitTime = 0;}


#ifdef _CLIENT
	void Client_Quit();
	void Client_HandleMessage(stBindMsg& msg);
#endif 
#ifdef _SERVER
	void Server_HandleMessage(stBindMsg& msg);
#endif 
};

#endif // !defined(AFX_CWORLD_H__E10BE219_0AB5_40E9_8C0B_EC014477682A__INCLUDED_)
