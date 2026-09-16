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
	cThread();
	virtual ~cThread();

	CWinThread* m_pThread;

	HANDLE m_hEndEvent;
	BOOL SetEndEvent();

	HANDLE m_hKillEvent;
	DWORD WaitKillEvent(DWORD dwMilliseconds = 0);

	virtual HRESULT RunThread(AFX_THREADPROC Thread, LPVOID lParam = NULL, int nPriority = THREAD_PRIORITY_NORMAL) ;
	virtual HRESULT EndThread() ;
//	ThreadFunc m_pFunction;
//	static UINT Thread( LPVOID pParam );
};

#endif // !defined(AFX_CTHREAD_H__33914845_4AFD_45DD_9A37_4D78C0D7A62B__INCLUDED_)
