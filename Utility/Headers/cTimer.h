// cTimer.h: interface for the cTimer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTIMER_H__70B07BC3_45A5_4B75_B772_13DF96785949__INCLUDED_)
#define AFX_CTIMER_H__70B07BC3_45A5_4B75_B772_13DF96785949__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cTimer  
{
public:
	cTimer(BOOL bAutoSettimer = TRUE)
	{
		if (bAutoSettimer)
			SetTimer();
	}
	virtual ~cTimer(){};
	LARGE_INTEGER c1;
	void SetTimer()
	{
		BOOL b = QueryPerformanceCounter(&c1);
	}
	DWORD GetTimeMS()
	{
		LARGE_INTEGER c2;
		LARGE_INTEGER freq;
		BOOL b = QueryPerformanceCounter(&c2);
		DWORD t = c2.LowPart - c1.LowPart;
		QueryPerformanceFrequency(&freq);
		DWORD f= freq.LowPart/1000;
		t = t / f;	
		return t;
	}
};

class cTimerFunction : public cTimer
{
public:
	cTimerFunction(LPCSTR szFunctionName, DWORD nTimeRange = 10, BOOL bAutoSettimer = TRUE) : cTimer(bAutoSettimer)
	{
		m_strFunctionName = szFunctionName;
		m_nTimeRange = nTimeRange;
	}
	CString m_strFunctionName;
	DWORD m_nTimeRange;
	virtual ~cTimerFunction(){
		DWORD t = GetTimeMS();
		if (t > m_nTimeRange)
		{
			CString strMessage;
			strMessage.Format("%s cost time %d. \n", m_strFunctionName,t);
			OutputDebugString(strMessage);
		}
	};
};

//œﬁÀŸ
class cTimerLimit : public cTimer
{
public:
	cTimerLimit(DWORD ms,BOOL bAutoSettimer = TRUE) : cTimer(bAutoSettimer){m_nMs = ms;} //ms ∫¡√Î
	DWORD m_nMs;
	virtual ~cTimerLimit(){
		DWORD t = GetTimeMS();
		if (t < m_nMs)
		{
			Sleep(m_nMs - t);
		}
	}
};

#endif // !defined(AFX_CTIMER_H__70B07BC3_45A5_4B75_B772_13DF96785949__INCLUDED_)
