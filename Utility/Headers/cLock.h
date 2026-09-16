// cLock.h: interface for the cLock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLOCK_H__71673B55_BE61_44C2_B9F4_20CEF936B72B__INCLUDED_)
#define AFX_CLOCK_H__71673B55_BE61_44C2_B9F4_20CEF936B72B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cLock  
{
public:
	CRITICAL_SECTION* m_pCS;
	BOOL m_bLocked;
	cLock(CRITICAL_SECTION* p,BOOL bLock = TRUE)
	{
		ASSERT(p);
		m_pCS = p;
		m_bLocked = FALSE;
		if (bLock)
		{
			Lock();
		}
	};
	virtual ~cLock()
	{
		Unlock();
	}
	void Lock()
	{
		if (m_pCS)
		{
			EnterCriticalSection(m_pCS);		
			m_bLocked = TRUE;
		}
	}
	void Unlock()
	{
		if (m_bLocked)
		{
			if (m_pCS)
				LeaveCriticalSection(m_pCS);	
			m_pCS = NULL;
			m_bLocked = FALSE;
		}
	}
};

#endif // !defined(AFX_CLOCK_H__71673B55_BE61_44C2_B9F4_20CEF936B72B__INCLUDED_)
