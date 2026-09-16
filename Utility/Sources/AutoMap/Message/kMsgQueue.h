// kMsgQueue.h: interface for the kMsgQueue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KMSGQUEUE_H__ABC6F4F2_E08F_4E7A_89D6_149B29CD99F5__INCLUDED_)
#define AFX_KMSGQUEUE_H__ABC6F4F2_E08F_4E7A_89D6_149B29CD99F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iMsg.h"

class iMsg;

#include <queue>
typedef std::queue<iMsg*> msgQueue;

class kMsgQueue : public iMsgQueue  
{
public:
	kMsgQueue();
	virtual ~kMsgQueue();

	ULONG m_ulCount;
	virtual ULONG AddRef() {return ++m_ulCount; };
	virtual ULONG Release() {ULONG count = --m_ulCount; if(m_ulCount == 0)delete this; return count;};

public:
	HANDLE m_hMessageArrive;
	ULONG WaitForMessageArrive(long time);
	
	msgQueue m_msgQueue;
	CRITICAL_SECTION m_criticalSection;
	void Clear();
	
	bool PushMsg(iMsg* pMsg);
	iMsg* PopMsg();
};

#endif // !defined(AFX_KMSGQUEUE_H__ABC6F4F2_E08F_4E7A_89D6_149B29CD99F5__INCLUDED_)
