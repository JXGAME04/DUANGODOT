// cWorld.cpp: implementation of the cWorld class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "TestServer.h"
#include "common.h"
#include "cWorld.h"
#include "cPlayer.h"

#include "cThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef _SERVER
BOOL Server_VerifyPassword(LPCSTR szAccount, LPCSTR szPassword)
{
	return strlen(szPassword) >= 4;
}
#endif

cWorld::cWorld()
{
	m_bWaitingAnswer = FALSE;
	m_pThread = NULL;
	for (int i=0; i<c_nMaxPlayer; i++)
		m_aPlayer[i] = NULL;
}

cWorld::~cWorld()
{
	EndThread();
	for (int i=0; i<c_nMaxPlayer; i++)
		delete m_aPlayer[i] ;
}

cPlayer* cWorld::FindPlayer(LPCSTR szAccount,BOOL bCreate )
{
	for (int i=0; i<c_nMaxPlayer; i++)
	{
		if (m_aPlayer[i] != NULL)
		{
			if (m_aPlayer[i]->IsAccount(szAccount))
				return m_aPlayer[i];
		}
	}
	if (!bCreate)
		return NULL;
	cPlayer* p = CreatePlayer();
	if (p != NULL)
		p->SetAccount(szAccount);
	return p;
}

cPlayer* cWorld::CreatePlayer()
{
	for (int i=0; i<c_nMaxPlayer; i++)
	{
		if (m_aPlayer[i] == NULL)
		{
			m_aPlayer[i] = new cPlayer();
			m_aPlayer[i]->SetID(i);
			return m_aPlayer[i];
		}
	}
//	ASSERT(0);
	return NULL;
}

UINT WorldThread(LPVOID pParam)
{
	cWorld* p= (cWorld*)pParam;
	p->Run();		
	return 0;
}

const int c_nWaitSecond = 4;
BOOL cWorld::Run()
{
	stBindMsg msg;
	while(m_pThread->WaitKillEvent(100) == WAIT_TIMEOUT)
	{
		//pop message 
		if (!PopMessage(msg))
		{
			m_nWaitTime++;
			//10 second
			if (m_nWaitTime>c_nWaitSecond*10 && IsWaitingAnswer())
			{
				//server can not solve the message!
				//can not call close here!
#ifdef _CLIENT
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,FALSE);			
				EnableNetStatus(NetStatus_Quit,FALSE);			
				EnableNetStatus(NetStatus_Send,FALSE);			
				OutputString("system","seems server can not solve your answer, please choose another server!");
				SetWaitingAnswer(FALSE);
#endif
			}
			continue;
		}
		else
		{
			SetWaitingAnswer(FALSE);
		}

#ifdef _SERVER
		Server_HandleMessage(msg);
#endif 
#ifdef _CLIENT
		Client_HandleMessage(msg);
#endif
		delete msg.pMsg;
	}
	return TRUE;
}

BOOL cWorld::RunThread()
{
	m_pThread = new cThread;
	return m_pThread->RunThread(WorldThread,this);
}

BOOL cWorld::EndThread()
{
	if (m_pThread)
	{
		m_pThread->EndThread();
		delete m_pThread;
		m_pThread = NULL;
		return TRUE;
	}	
	return FALSE;
}

void cWorld::PushMessage(stMsg* p,DWORD size,cPlayer* pPlayer,iServerLink* pLink)
{
	stBindMsg msg;
	stMsg* pMsg = stMsg::CreateMsg(*p,size);
	msg.pMsg = pMsg;
	msg.pPlayer = pPlayer;
	msg.pLink = pLink;
	msg.msgSize = size;
	m_queue.Push(msg);
};

#ifdef _CLIENT
void cWorld::Client_Quit()
{
	//clear all in world!
	stMsg_c2s_Quit msg;
	AppGetClient()->SendMessage(&msg,sizeof(&msg));
}

void cWorld::Client_HandleMessage(stBindMsg& msg)
{
	stMsg* p = msg.pMsg;

	switch (p->GetType())
	{
	case Msg_Net_ServerConnect:
		{
			stMsg_Net_ServerConnect* pMsg = (stMsg_Net_ServerConnect*)p;
			if (pMsg->bConnect == TRUE)
			{
				OutputString("system","success connect to server!");
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,TRUE);			
				EnableNetStatus(NetStatus_Quit,FALSE);			
				EnableNetStatus(NetStatus_Send,FALSE);			
			}
			else
			{
				OutputString("system","can not connect to server!");
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,FALSE);			
				EnableNetStatus(NetStatus_Quit,FALSE);			
				EnableNetStatus(NetStatus_Send,FALSE);			
			}
		}
		break;
	case Msg_Net_ServerDown:
		{
			OutputString("system","disconnect from server!");
			EnableNetStatus(NetStatus_Connect,TRUE);			
			EnableNetStatus(NetStatus_Verify,FALSE);			
			EnableNetStatus(NetStatus_Quit,FALSE);			
			EnableNetStatus(NetStatus_Send,FALSE);			
		}
		break;
	case Msg_s2c_Chat:
		{
			stMsg_s2c_Chat* pMsg = (stMsg_s2c_Chat* )p;
			OutputString(pMsg->szAccount,pMsg->szChat);
		}
		break;
	case Msg_s2c_Account:
		{
			stMsg_s2c_Account* pMsg = (stMsg_s2c_Account*)p;
			if(pMsg->lResult >= 0)
			{
				if (pMsg->lResult == 0)
					OutputString("system","password is ok!");
				else
					OutputString("system","reconnect to server!");
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,FALSE);			
				EnableNetStatus(NetStatus_Quit,TRUE);			
				EnableNetStatus(NetStatus_Send,TRUE);			
			}
			else
			{
				EnableNetStatus(NetStatus_Connect,TRUE);			
				EnableNetStatus(NetStatus_Verify,TRUE);
				EnableNetStatus(NetStatus_Quit,FALSE);
				EnableNetStatus(NetStatus_Send,FALSE);
				if (pMsg->lResult == -1)
					OutputString("system","password is invalid!");
				else if (pMsg->lResult == -2)
					OutputString("system","grab by another ip!");
				else if (pMsg->lResult == -3)
				{
					OutputString("system","player is full! please try again!");
					//can not use close in this thread!!!
					EnableNetStatus(NetStatus_Verify,FALSE);
				}
			}
		}
		break;
	}
}
#endif 

void cWorld::QuitPlayer(cPlayer* p)
{
	int id = p->GetID(); 
	iServerLink* pLink = p->GetLink();
	if (pLink != NULL)
	{
		pLink->BindPlayer(NULL);
		pLink->CloseClient();
		pLink->Create();
	}
	delete p;
	m_aPlayer[id] = NULL;
}

#ifdef _SERVER
void cWorld::Server_HandleMessage(stBindMsg& msg)
{
	stMsg* p = msg.pMsg;

	switch (p->GetType())
	{
//server message
	case Msg_Net_Dead:
		{
			if (msg.pPlayer != NULL)
			{
				OutputString(msg.pPlayer->m_szAccount,"disconnect too much time");
				QuitPlayer(msg.pPlayer);
			}
			break;
		}
	case Msg_Net_ClientDown:
		{
			OutputString("system","a client disconet");
		}
		break;
	case Msg_Net_ClientConnect:
		{
			OutputString("system","a client connect");
		}
		break;
	case Msg_c2s_Account:
		{
			//valid the password and name
			//find if there is already some one here!
			stMsg_c2s_Account* pMsg = (stMsg_c2s_Account*)p;

			stMsg_s2c_Account msg2;
			if (!Server_VerifyPassword(pMsg->szAccount,pMsg->szPassword))
			{
				msg2.lResult = -1;
				OutputString(pMsg->szAccount,"player connect use wrong password");
			}
			else
			{
				cPlayer* pPlayer = FindPlayer(pMsg->szAccount,TRUE);
				if(pPlayer == NULL)
				{
					//server is full!
					msg2.lResult = -3;
				}
				else
				{
					iServerLink* pLink = pPlayer->GetLink();

					if (pLink != NULL)
					{
						if (pLink->IsConnect())
						{
							msg2.lResult = -2;
							pLink->SendMessage(&msg2,sizeof(msg2));
							pLink->CloseClient();
							pLink->Create();
							OutputString(pMsg->szAccount,"player connect from another ip");
							msg2.lResult = 2;
						}
						else
						{
							msg2.lResult = 1;
							pLink->Create();
							OutputString(pMsg->szAccount,"player reconnect");
						}
						pLink->BindPlayer(NULL);
					}
					else
					{
						msg2.lResult = 0;
						OutputString(pMsg->szAccount,"create a new player");
					}
					pPlayer->BindLink(msg.pLink);
					msg.pLink->BindPlayer(pPlayer);		
				}
			}
			msg.pLink->SendMessage(&msg2,sizeof(stMsg_s2c_Account));
		}
		break;
	case Msg_c2s_Chat:
		{
			if (msg.pPlayer == NULL)
				break;
			stMsg_c2s_Chat* pMsg = (stMsg_c2s_Chat*)p;
			stMsg_s2c_Chat msg2;
			strcpy(msg2.szChat,pMsg->szChat);
			strcpy(msg2.szAccount,msg.pPlayer->m_szAccount);
			if (strlen(pMsg->szAccount) > 0)
			{
				cPlayer* pPlayer = FindPlayer(pMsg->szAccount,FALSE);
				if (pPlayer != NULL)
					pPlayer->GetLink()->SendMessage(&msg2,sizeof(stMsg_s2c_Chat));
			}
			else
			{
				AppGetServer()->SendMessage(&msg2,sizeof(stMsg_s2c_Chat));
			}
		}
		break;
	case Msg_c2s_Quit:
		{
			if (msg.pPlayer == NULL)
				break;
			//save player's data?
			QuitPlayer(msg.pPlayer);
			OutputString(msg.pPlayer->m_szAccount,"quit world");
		}
		break;
	}
}
#endif

