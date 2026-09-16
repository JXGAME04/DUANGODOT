// cMissle.cpp: implementation of the cMissle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "status.h"
#include "cMissle.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cMissle::cMissle()
{
	m_nLife = 0;
	m_nBlock = 0;
}

cMissle::~cMissle()
{

}

BOOL cMissle::Run()
{
	if (m_bSetCurrentAnimation == false)
	{
		SetCurrentAnimation(m_strAni);
		m_bSetCurrentAnimation = true;
	}
	m_nLife ++ ;
	if (m_nLife > 120) //120 * 0.3 * 32 = 1000 
		return FALSE;
	switch (m_eStatus)
	{
	case STATUS_WALK:
		{
			bool bStop = true;
			cPoint ptOffset = GetPosition();
			ptOffset+=m_vSpeed;
			cPoint vSpeed;
			if (!GetGraphics()->GetiGround()->BlockObstacle(ptOffset,OBSTACLE_BLOCKFLY))
			{
				SetPosition(ptOffset);
				IncreaseFrame();
				bStop = false;
			}
			else
			{
				//ChangeAnimation(ANI_WAIT);
				Turn(m_eDirection);
				m_nBlock = 0;
				m_eStatus = STATUS_BLOCK;
			}
			break;
		}
	case STATUS_WAIT:
		break;
	case STATUS_BLOCK:
		if (m_nBlock++ > 30)
			return FALSE;
		break;
	}
	return TRUE;
}
