// cThread.cpp: implementation of the cThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cThread.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cThread::cThread()
{
}

cThread::~cThread()
{

}

HRESULT cThread::RunThread(AFX_THREADPROC Thread, LPVOID lParam, int nPriority)
{
	m_hEndEvent = CreateEvent(NULL,0,0,0);
	m_hKillEvent = CreateEvent(NULL,0,0,0);
	m_pThread = AfxBeginThread(Thread,lParam,nPriority);
	ASSERT(m_pThread );

	return S_OK;
}

HRESULT cThread::EndThread()
{
	if (m_pThread != NULL)
	{
		SetEvent(m_hKillEvent);
		WaitForSingleObject(m_hEndEvent,INFINITE);
		//if father thread exit beforce child, the child will be in a dead lock!
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		//for we use a AfxbeginThread not new, so dont delete it !
//		delete m_pThread;
		if (m_hKillEvent )
			CloseHandle(m_hKillEvent);
		if (m_hEndEvent)
			CloseHandle(m_hEndEvent);
	}
	m_pThread = NULL;

	return S_OK;
}

BOOL cThread::SetEndEvent()
{
	BOOL b = SetEvent(m_hEndEvent);
	ASSERT(b);
	return b;
}

DWORD cThread::WaitKillEvent(DWORD dwMilliseconds )
{
	return WaitForSingleObject(m_hKillEvent,dwMilliseconds);
}
