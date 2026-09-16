#pragma once

//it is used for multipul thread!
#include <queue>
#include "clock.h"

template<typename T>
class cSafeQueue
{
public:
	CRITICAL_SECTION m_cs;
	cSafeQueue() {InitializeCriticalSection(&m_cs);} 
	~cSafeQueue(){DeleteCriticalSection(&m_cs);}

	virtual void Push(T p) {cLock lock(&m_cs);
							m_queue.push(p);};

	virtual  BOOL Pop(T& p) {		
							cLock lock(&m_cs);
							if (m_queue.empty())
								return FALSE;
							p = m_queue.front();
							m_queue.pop();
							return TRUE;};
	std::queue<T> m_queue;
};