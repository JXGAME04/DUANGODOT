// cMapObstacle.h: interface for the cMapObstacle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMAPOBSTACLE_H__DBF947BC_20BA_4F65_9E4D_B565E1D43E83__INCLUDED_)
#define AFX_CMAPOBSTACLE_H__DBF947BC_20BA_4F65_9E4D_B565E1D43E83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//地图的障碍
#include "iGraphics.h"

#include "cLock.h"
struct stObstaclePoint
{
	CPoint pt;
	int nBlock;
	bool operator == (const stObstaclePoint& st)
	{
		return nBlock == st.nBlock && pt == st.pt;
	}
};
#include <list>
typedef std::list<stObstaclePoint> ObstaclePointList;
typedef ObstaclePointList::iterator itObstaclePoint;

/*
template<> struct std::hash<CPoint>
{
	size_t operator()(CPoint pt) const {return (pt.x<<16) | pt.y;};
};
struct equal_point
{
	bool operator() (CPoint p1,CPoint p2) const
	{
		return (bool)(p1 == p2);
	}
};
	
//int hash_point(CPoint pt);
	
#include <hash_map>
typedef std::hash_map<CPoint, int, std::hash<CPoint>, equal_point> ObstacleMap;
typedef ObstacleMap::iterator itObstacleMap;
*/


//be sure the region is in (200*200)
//数组导致无法命中cache,同时,内存开销很大

class cGround;
class cMapObstacle  
{
public:
	enum {	MAX_REGION = 6,
			MAX_WIDTH = c_nWidthRegion*MAX_REGION,
			MAX_HEIGHT = c_nHeightRegion*MAX_REGION,};

	cMapObstacle(cGround* p);
	virtual ~cMapObstacle();
	cGround* m_pGround;
	cGround* GetGround(){return m_pGround;}

	CPoint m_ptOffset;
	void SetCenter(CPoint ptOffset){m_ptOffset = ptOffset;};
	
	bool m_bUpdate;
	char m_aObstacle[MAX_WIDTH][MAX_HEIGHT];
	ObstaclePointList m_list;
	CRITICAL_SECTION m_csTextureCache;
	
	char& GetObstacle(CPoint pt){//	ASSERT(pt.x < MAX_HEIGHT && pt.x > 0 && pt.y < MAX_HEIGHT && pt.y > 0);
		return m_aObstacle[pt.x][pt.y];
	}
//	void RemoveObstacle(CPoint pt,int n){GetObstacle(pt) &= ~n;}

	void AddObstacle(const std::list<cPoint>& list,ObstacleEnum e);
	void RemoveObstacle(const std::list<cPoint>& list,ObstacleEnum e);

	void Clear();
	void Build();
	bool IsBlock(CPoint pt,ObstacleEnum e);
	bool IsBlock(cPoint pt,ObstacleEnum e){return IsBlock(CPoint(pt.x,pt.y),e);};
	BOOL IsLine(cPoint ptStart, cPoint ptEnd);

	BOOL FindWalkPath(std::list<cPoint>& ptlist, cPoint ptStart, cPoint ptEnd);
	BOOL AStartFindPath(std::list<cPoint>& ptlist, cPoint ptStart, cPoint ptEnd);
	
	//shall we also provide a IsBlock(cPoint pt, cPoint vSpeed, ObstacleEnum e);

	HRESULT Draw();
};

#endif // !defined(AFX_CMAPOBSTACLE_H__DBF947BC_20BA_4F65_9E4D_B565E1D43E83__INCLUDED_)
