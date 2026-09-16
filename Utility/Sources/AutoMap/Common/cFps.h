// cFps.h: interface for the cFps class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFPS_H__7F66A868_9BA7_4F9A_8FE6_D4F638CE0962__INCLUDED_)
#define AFX_CFPS_H__7F66A868_9BA7_4F9A_8FE6_D4F638CE0962__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iFps.h"

class cFps  : public iFps
{
public:
	cFps();
	virtual ~cFps();
	ULONG m_ulCount;
	virtual ULONG AddRef() {return ++m_ulCount; };
	virtual ULONG Release() {ULONG count = --m_ulCount; if(m_ulCount == 0)delete this; return count;};
	unsigned long per_count,count;
	unsigned long start_time,old_time,new_time,pre_time;
	unsigned long time, tick_time;
	unsigned long now_fps,fps;
	unsigned long GetCount(){return count;}	
	unsigned long GetTime(){return time;}
	unsigned long GetFps(){return fps;}
	unsigned long GetNowFps(){return now_fps;}
	void Init();
	void Frame();
	void KeepFps(int frame);
};

#endif // !defined(AFX_CFPS_H__7F66A868_9BA7_4F9A_8FE6_D4F638CE0962__INCLUDED_)
