#include "KCore.h"
#include "../../Represent/iRepresent/iRepresentShell.h"
#include "KSubWorldSet.h"

iRepresentShell*	g_pRepresent = 0;

unsigned int	l_CTime = 0;

unsigned int IR_CGetCurrentTime()
{
	return l_CTime ;
}

//--------------------------------------------------------------------------
//	功能：更新图形换帧计算用时钟
//--------------------------------------------------------------------------
void IR_CUpdateTime()
{
	l_CTime = timeGetTime();
}

//--------------------------------------------------------------------------
//	功能：换帧计算
//--------------------------------------------------------------------------
void IR_CNextFrame(int& nFrame, int nTotalFrame, unsigned int uInterval, unsigned int& uFlipTime)
{
	if (nTotalFrame > 1 && uInterval)
	{
		while ((l_CTime - uFlipTime) >= uInterval)
		{
			uFlipTime += uInterval;
			if ((++nFrame) >= nTotalFrame)
			{
				uFlipTime = l_CTime;
				nFrame = 0;
				break;
			}
		}
	}
}
