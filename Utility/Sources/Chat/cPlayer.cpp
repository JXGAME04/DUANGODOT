// cPlayer.cpp: implementation of the cPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "common.h"
#include "cPlayer.h"
#include "cWorld.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlayer::cPlayer()
{
	m_pLink = NULL;
}

cPlayer::~cPlayer()
{
	RELEASE(m_pLink);
}

void cPlayer::BindLink(iServerLink* pLink)
{
	RELEASE(m_pLink);
	m_pLink = pLink;
	m_pLink->AddRef();
}

UINT cPlayer::HandleMessage(stMsg* pMsg,DWORD size)
{
	//handle what can handled, like walk ...

	switch (pMsg->GetType())
	{
	case MsgIdle:
		//still connnect!
		return 0;
	}

	AppGetWorld()->PushMessage(pMsg,size,this,GetLink());

	return 0;
}

UINT PlayerHandleMessage(stMsg* pMsg,DWORD size, void* p,iServerLink* pLink)
{
	//if the player can solve the msg, solve it!
	//else world solve it!
	if (p == NULL)
	{
		if (pMsg->GetType() == MsgIdle)
			return 0;
		//it is a netconnect msg;
		//handled by the gameworld;
		AppGetWorld()->PushMessage(pMsg,size,NULL,pLink);
	}
	else
	{
		cPlayer* pPlayer = (cPlayer*)p;	
		return pPlayer->HandleMessage(pMsg,size);
	}
	return 0;
}
