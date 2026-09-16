// cThread.h: interface for the cThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTHREAD_H__33914845_4AFD_45DD_9A37_4D78C0D7A62B__INCLUDED_)
#define AFX_CTHREAD_H__33914845_4AFD_45DD_9A37_4D78C0D7A62B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//typedef  UINT (*ThreadFunc)( LPVOID pParam );

class cThread  
{
public:
	cThread(){m_pThread = NULL;};
	virtual ~cThread(){};

	CWinThread* m_pThread;

	HANDLE m_hKillEvent;
	DWORD WaitKillEvent(DWORD dwMilliseconds = 0)
	{
		return WaitForSingleObject(m_hKillEvent,dwMilliseconds);
	}

	BOOL RunThread(AFX_THREADPROC Thread, LPVOID lParam = NULL, int nPriority = THREAD_PRIORITY_NORMAL)
	{
		m_hKillEvent = CreateEvent(NULL,0,0,0);
		m_pThread = AfxBeginThread(Thread,lParam,nPriority);
		ASSERT(m_pThread );
		return TRUE;
	}
	BOOL EndThread()
	{
		if (m_pThread == NULL)
			return TRUE;
		SetEvent(m_hKillEvent);
		//if father thread exit beforce child, the child will be in a dead lock!
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		//for we use a AfxbeginThread not new, so dont delete it !
		//delete m_pThread;
		if (m_hKillEvent )
			CloseHandle(m_hKillEvent);
		return TRUE;
	}

//	ThreadFunc m_pFunction;
//	static UINT Thread( LPVOID pParam );
};

#endif // !defined(AFX_CTHREAD_H__33914845_4AFD_45DD_9A37_4D78C0D7A62B__INCLUDED_)
