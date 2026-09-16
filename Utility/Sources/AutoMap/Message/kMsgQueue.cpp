// kMsgQueue.cpp: implementation of the kMsgQueue class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "kMsgQueue.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


kMsgQueue::kMsgQueue()
{
	m_ulCount = 1;
	InitializeCriticalSection(&m_criticalSection);
	m_hMessageArrive = CreateEvent(NULL,0,0,0);
}

kMsgQueue::~kMsgQueue()
{
	while(!m_msgQueue.empty())
	{
		iMsg* p = PopMsg();
		p->Release();
	}
	DeleteCriticalSection(&m_criticalSection);
	BOOL b = CloseHandle(m_hMessageArrive);
}

ULONG kMsgQueue::WaitForMessageArrive(long time)
{
	return WaitForSingleObject(m_hMessageArrive,time);
}

bool kMsgQueue::PushMsg(iMsg* pMsg)
{
	EnterCriticalSection(&m_criticalSection);
	m_msgQueue.push(pMsg);
	LeaveCriticalSection(&m_criticalSection);
	
	SetEvent(m_hMessageArrive);
	
	return true;
}

iMsg* kMsgQueue::PopMsg()
{
	EnterCriticalSection(&m_criticalSection);
	if (m_msgQueue.empty())
	{
		LeaveCriticalSection(&m_criticalSection);
		return NULL;
	}
	iMsg* pMsg = m_msgQueue.front();
	m_msgQueue.pop();
	LeaveCriticalSection(&m_criticalSection);
	return pMsg;
}

void kMsgQueue::Clear()
{
	EnterCriticalSection(&m_criticalSection);
	while (!m_msgQueue.empty())
	{
		iMsg* pMsg = m_msgQueue.front();
		m_msgQueue.pop();
		pMsg->Release();
	}
	LeaveCriticalSection(&m_criticalSection);
}


_declspec(dllexport) iMsgQueue* Message_CreateMsgQueue(eMsgQueueType eType,long verson1 ,long verson2 )
{
	return new kMsgQueue;
}
