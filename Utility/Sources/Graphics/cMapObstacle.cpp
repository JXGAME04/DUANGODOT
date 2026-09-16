// cMapObstacle.cpp: implementation of the cMapObstacle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cMapObstacle.h"

#include "cGraphics.h"
#include "cGround.h"
#include "cObstacle.h"
#include "cCamera.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
cMapObstacle::cMapObstacle(cGround* p)
{
	m_pGround = p;
	m_ptOffset = CPoint(-1,-1);
	m_bUpdate = TRUE;
	InitializeCriticalSection(&m_csTextureCache);
}

cMapObstacle::~cMapObstacle()
{
	DeleteCriticalSection(&m_csTextureCache);
}

ObstacleEnum GetType(int n)
{
	if (n & OBSTACLE_BLOCKWALK)
	{
		if (n & OBSTACLE_BLOCKFLY)
		{
			if (n & OBSTACLE_BLOCKJUMP)
				return OBSTACLE_NORMAL;
			else
				return OBSTACLE_CANJUMP;
		}
		else
		{
			if (n & OBSTACLE_BLOCKJUMP)
				return OBSTACLE_CANFLY;
			else
				return OBSTACLE_CANFLYJUMP;
		}
	}
	return OBSTACLE_NULL;
}

int GetBlockNum(ObstacleEnum e)
{
	int n = 0;
	switch (e)
	{
	case OBSTACLE_NORMAL:
		n = OBSTACLE_BLOCKFLY | OBSTACLE_BLOCKWALK | OBSTACLE_BLOCKJUMP;
		break;
	case OBSTACLE_CANJUMP:
		n = OBSTACLE_BLOCKFLY | OBSTACLE_BLOCKWALK ;
		break;
	case OBSTACLE_CANFLY:
		n = OBSTACLE_BLOCKWALK | OBSTACLE_BLOCKJUMP;
		break;
	case OBSTACLE_CANFLYJUMP:
		n = OBSTACLE_BLOCKWALK ;
		break;
	}
	return n;
}

void cMapObstacle::AddObstacle(const std::list<cPoint>& list,ObstacleEnum e)
{
	cLock cs(&m_csTextureCache);
	int n = GetBlockNum(e);
	for (std::list<cPoint>::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		CPoint pt = CPoint((int)( (*it).x-0.5f ) ,(int)( (*it).y-0.5f ));
		stObstaclePoint st;
		st.pt = pt;
		st.nBlock = n;
		m_list.push_back(st);
	}
	m_bUpdate = true;
}

void cMapObstacle::RemoveObstacle(const std::list<cPoint>& list,ObstacleEnum e)
{
	cLock cs(&m_csTextureCache);
	int n = GetBlockNum(e);
	for (std::list<cPoint>::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		CPoint pt = CPoint((int)( (*it).x-0.5f ) ,(int)( (*it).y-0.5f ));
		stObstaclePoint st;
		st.pt = pt;
		st.nBlock = n;
		itObstaclePoint it = std::find(m_list.begin(),m_list.end(),st);
		if (it != m_list.end())
			m_list.erase(it);
	}
	m_bUpdate = true;
}

void cMapObstacle::Clear()
{
	m_list.clear();
}

void cMapObstacle::Build()
{
	ZeroMemory(m_aObstacle,sizeof(m_aObstacle));
	cPoint pt = GetGround()->GetcCamera()->GetFocus();
	m_ptOffset = CPoint(pt.x,pt.y) - CPoint(MAX_WIDTH/2,MAX_HEIGHT/2);
	cLock cs(&m_csTextureCache);
	for (itObstaclePoint it = m_list.begin(); it != m_list.end(); ++it)
	{
		stObstaclePoint st = (*it);		
		st.pt -= m_ptOffset;
		if (st.pt.x < MAX_WIDTH && st.pt.x > 0 && st.pt.y < MAX_HEIGHT && st.pt.y > 0)
			GetObstacle(st.pt) |= st.nBlock;
	}
	m_bUpdate = false;
}

bool cMapObstacle::IsBlock(CPoint pt,ObstacleEnum e)
{	
	pt -= m_ptOffset;
	if (pt.x < MAX_WIDTH && pt.x > 0 && pt.y < MAX_HEIGHT && pt.y > 0)
		return GetObstacle(pt) & e;
	return true;
}

HRESULT cMapObstacle::Draw()
{
	if (!GetGround()->GetGraphics()->IsShow(cGraphics::GS_OBSTACLE))
		return S_OK;

	if (m_ptOffset == CPoint(-1,-1) || 	m_bUpdate == true)
		Build();

	cPointList ptlist[OBSTACLE_CANFLYJUMP+1];
	cRect rc = GetGround()->GetRectGround();
	for (int x = rc.left; x <= rc.right; x++)
	for (int y = rc.top; y <= rc.bottom; y++)
	{
		CPoint pt = CPoint(x,y);
		pt -= m_ptOffset;
		if (pt.x < MAX_WIDTH && pt.x > 0 && pt.y < MAX_HEIGHT && pt.y > 0)
		{
			char& e = GetObstacle(pt);
			if (e != OBSTACLE_NULL)
			{
				ObstacleEnum type = GetType(e);	
				ptlist[type].push_back(cPoint(x+0.5f,y+0.5f,0));
			}
		}
		else
		{
//			ASSERT(0);
		}
	}
	for (int i=0; i<=OBSTACLE_CANFLYJUMP; i++)
	{
		GetGround()->DrawPointAsRect(ptlist[i],cObstacle::GetObstacleColor((ObstacleEnum)i,192));
	}

	return S_OK;
}

const float c_fSpeed = 0.1f;

//·½·¨Ò»£¬
//1. if there is a line from start to end, most case
//2. if block at the obstacle then follow obstacle's line,modify the start till 1.
BOOL Reach(cPoint ptStart, cPoint ptEnd)
{
	cPoint p = ptEnd - ptStart;
	return (abs(p.x) < c_fSpeed && abs(p.y) < c_fSpeed);
}

BOOL cMapObstacle::IsLine(cPoint ptStart, cPoint ptEnd)
{
	cPoint vSpeed = ptEnd - ptStart;
	float f = c_fSpeed / sqrt(vSpeed.x*vSpeed.x+vSpeed.y*vSpeed.y+vSpeed.z*vSpeed.z);
	vSpeed.x *= f;
	vSpeed.y *= f;
	vSpeed.z *= f;
	while (!Reach(ptStart,ptEnd))
	{
		ptStart += vSpeed;
		if (IsBlock(ptStart,OBSTACLE_BLOCKWALK))
		{
			ptStart -= vSpeed; //the ptStart is the first point
			return FALSE;
		}
	}
	return TRUE;
}

BOOL cMapObstacle::AStartFindPath(std::list<cPoint>& ptlist, cPoint ptStart, cPoint ptEnd)
{
	return FALSE;
}

BOOL cMapObstacle::FindWalkPath(std::list<cPoint>& ptlist, cPoint ptStart, cPoint ptEnd)
{
	if (IsLine(ptStart,ptEnd))
	{
		ptlist.push_back(ptEnd);
		return TRUE;
	}
	cPoint ptMiddle = ptStart + ptEnd;
	ptMiddle.x /= 2;
	ptMiddle.y /= 2;

	cPoint vTemp = ptEnd-ptStart;
	cPoint vSpeed = cPoint(-vTemp.y,vTemp.x);
	float c_fStep = 1.f;
	float f = c_fStep / sqrt(vSpeed.x*vSpeed.x+vSpeed.y*vSpeed.y+vSpeed.z*vSpeed.z);
	vSpeed.x *= f;
	vSpeed.y *= f;
	vSpeed.z *= f;

	const int c_nStep = 20;

	for (int i=0; i<c_nStep; i++)
	{
		cPoint ptSelect = ptMiddle;
		ptSelect += cPoint(vSpeed.x*i,vSpeed.y*i);
		if (IsLine(ptStart,ptSelect) && IsLine(ptSelect,ptEnd)) 
		{
			ptlist.push_back(ptSelect);
			ptlist.push_back(ptEnd);
			return TRUE;
		}
		ptSelect = ptMiddle;
		ptSelect -= cPoint(vSpeed.x*i,vSpeed.y*i);
		if (IsLine(ptStart,ptSelect) && IsLine(ptSelect,ptEnd)) 
		{
			ptlist.push_back(ptSelect);
			ptlist.push_back(ptEnd);
			return TRUE;
		}
	}
	return FALSE;
}


