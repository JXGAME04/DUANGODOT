// cFps.cpp: implementation of the cFps class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cFps.h"

#include <Mmsystem.h>

#pragma comment(lib,"Winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


cFps::cFps()
{
	m_ulCount = 1;
}

cFps::~cFps()
{

}

void cFps::Init()
{
	now_fps = 0;
	count = 0;
	per_count = 0;
	old_time = timeGetTime();
	pre_time = 0;
}

//do frame
void cFps::Frame()
{
	//do frame..
	count++;	

	new_time = timeGetTime();

	if (new_time - old_time > 1000)
	{
		now_fps = per_count;
		old_time = new_time;
		per_count = 0;
	}
	else
		per_count++;

	time = (new_time - start_time)/1000;
	fps = count / time;
}


void cFps::KeepFps(int frame)
{
	unsigned long ms = 1000 / frame;
	if (new_time - pre_time < ms)
		WaitForSingleObject(GetCurrentProcess(), ms - (new_time - pre_time));	// never singalled
	pre_time = new_time;
}

_declspec(dllexport) iFps* Common_CreateFps()
{
	return new cFps ;
}



 