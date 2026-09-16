// cColorMap.cpp: implementation of the cColorMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cColorMap.h"
#include "cLight.h"
#include "position.h"
#include "cPlaneVer.h"
#include "iItem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cColorMap::cColorMap()
{

}

cColorMap::~cColorMap()
{

}

void cColorMap::BeginLighting(cColor cr)
{
	int x,y;
	for (x=0; x<MAX_TILE_X; x++)
	for (y=0; y<MAX_TILE_Y; y++)
	{
		m_aTile[x][y].BeginLighting(cr);
	}
	m_lColor  = cr.Value();
	m_crColor = cr;
}

void cColorMap::EndLighting()
{
	int x,y;
	for (x=0; x<MAX_TILE_X; x++)
	for (y=0; y<MAX_TILE_Y; y++)
	{
		m_aTile[x][y].EndLighting();
	}

	//把格子光照转化成顶点的光照
	
/*
	m_aColor[0][0] = m_aTile[0][0].m_crLight;
	m_aColor[0][MAX_TILE] = m_aTile[0][MAX_TILE-1].m_crLight;
	m_aColor[MAX_TILE][0] = m_aTile[MAX_TILE-1][0].m_crLight;
	m_aColor[MAX_TILE][MAX_TILE] = m_aTile[MAX_TILE-1][MAX_TILE-1].m_crLight;

	for (x=1; x<MAX_TILE; x++)
	{
		y = 0;
		m_aColor[x][y] = (m_aTile[x-1][y].m_crLight + m_aTile[x][y].m_crLight)/2;
		y = MAX_TILE;
		m_aColor[x][y] = (m_aTile[x-1][y].m_crLight + m_aTile[x][y].m_crLight)/2;
	}
	for (y=1; y<MAX_TILE; y++)
	{
		x = 0;
		m_aColor[x][y] = (m_aTile[x][y-1].m_crLight + m_aTile[x][y].m_crLight)/2;
		x = MAX_TILE;
		m_aColor[x][y] = (m_aTile[x][y-1].m_crLight + m_aTile[x][y].m_crLight)/2;
	}
	
	for (x=1; x<MAX_TILE; x++)
	for (y=1; y<MAX_TILE; y++)
	{
		m_aColor[x][y] = (m_aTile[x-1][y-1].m_crLight + m_aTile[x-1][y].m_crLight + 
								m_aTile[x][y-1].m_crLight + m_aTile[x][y].m_crLight) / 4;
	}
*/
}

///////////////////////////////////////////////////////////////////////////////////
//ground tile
///////////////////////////////////////////////////////////////////////////////////
bool cColorMap::GetLightParam(cPoint ptGround,stLightParam& param)
{
	GroundToColorTile(ptGround);
	//如果高度等于m_fLightHeight
	int x = (int)ptGround.x;
	int y = (int)ptGround.y;
	if (x >=0 && x < MAX_TILE_X && y>=0 && y < MAX_TILE_Y)
	{
		bool b = m_aTile[(int)ptGround.x][(int)ptGround.y].GetLightParam(ptGround,param);
		return b;
	}
	return false;
}

bool cColorMap::GetTileType(cPoint ptGround, long& l)
{
	GroundToColorTile(ptGround);
	//如果高度等于m_fLightHeight
	int x = (int)ptGround.x;
	int y = (int)ptGround.y;
	if (x >=0 && x < MAX_TILE_X && y>=0 && y < MAX_TILE_Y)
	{
		l = m_aTile[x][y].GetProperty();
		return true;
	}
	return false;
}

bool cColorMap::GetColor(cPoint ptGround, DWORD& value)
{
	GroundToColorTile(ptGround);
	//如果高度等于m_fLightHeight
	int x = (int)ptGround.x;
	int y = (int)ptGround.y;
	if (x >=0 && x < MAX_TILE_X && y>=0 && y < MAX_TILE_Y)
	{
		bool b = m_aTile[x][y].GetColor(ptGround,value);
		if ( b == false )
			value = m_lColor;
		return b;
	}
	return false;
	//return m_aColor[(int)(ptGround.x+.5f)][(int)(ptGround.y+.5f)];
}

void cColorMap::InitTile()
{
	int x,y;
	for (x = 0;  x < MAX_TILE_X; x++)
	for (y = 0;  y < MAX_TILE_Y; y++)
	{
		m_aTile[x][y].SetPos(x,y);
	}

	cPoint p0(MAX_LIGHT_RADIUS*GROUND_DIVIDE,MAX_LIGHT_RADIUS*GROUND_DIVIDE);

	for (x = 0; x < MAX_LIGHT_TILE; x++)
	for (y = 0; y < MAX_LIGHT_TILE; y++)
	{
		//line from (x,y) to 0 !
		if (x == 9 && y == 2)
		{
			int t;
			t = 0;
		}
		cPoint p1 = cPoint(x,y);
		cPointList ptlist;
//		if (p0.x == p1.x)
//			continue;
		if (p0.x < p1.x)
			GetSeamCrossTile(p0,p1,ptlist);
		else
			GetSeamCrossTile(p1,p0,ptlist);
		for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
		{
			cPoint pt = (*it);
			if (pt == p0)
				continue;
			int m = (int)pt.x;
			int n = (int)pt.y;
			if (std::find(m_aBlocked[m][n].begin(),m_aBlocked[m][n].end(),p1) == m_aBlocked[m][n].end())
				m_aBlocked[m][n].push_back(p1);
		}
	}

	for (x = 0; x < MAX_LIGHT_TILE; x++)
	for (y = 0; y <= MAX_LIGHT_RADIUS; y++)
	{
		m_aBlocked[x][y].remove(cPoint(x,y));
	}
/*
	CPoint aPoint[256];
	for (x = 0; x < MAX_LIGHT_TILE; x++)
	for (y = 0; y < MAX_LIGHT_TILE; y++)
	{
		if (x == MAX_LIGHT_RADIUS && y == MAX_LIGHT_RADIUS)
		{
			int t;
			t = 0;
		}
		int nPoint = 0;
		for (itcPoint it = m_aBlocked[x][y].begin(); it != m_aBlocked[x][y].end(); ++it)
		{
			aPoint[nPoint].x = (*it).x;
			aPoint[nPoint].y = (*it).y;
			nPoint++;
		}
		CString s;
		s.Format("tile %d,%d block tile %d: ",x,y,nPoint);
		OutputDebugString(s);
		PointArrayToString(aPoint,nPoint,s);
		OutputDebugString(s);
		OutputDebugString("\n");
	}
*/
}

void cColorMap::ClearTile()
{
	for (int x = 0;  x < MAX_TILE_X; x++)
	for (int y = 0;  y < MAX_TILE_Y; y++)
	{
		m_aTile[x][y].Clear();
	}
}

void cColorMap::GroundToColorTile(cPoint& ptGround)
{
	ptGround.x -= m_xOffGround;
	ptGround.y -= m_yOffGround;
	ptGround.x *= GROUND_DIVIDE;
	ptGround.y *= GROUND_DIVIDE;	
}

cColorTile* cColorMap::GetTileByGround(cPoint& ptGround)
{
	GroundToColorTile(ptGround);
	int x = ptGround.x;
	int y = ptGround.y;
	if (x >= 0 && x < MAX_TILE_X && y >= 0 && y < MAX_TILE_Y)
		return &m_aTile[x][y];
	return NULL;
}

//这条线段经过那些的tile,
void cColorMap::GetPlaneLightedTile(cPlaneVer* p, cPointList & ptlist)
{
	switch(p->GetLightedType())
	{
	case LIGHTED_POINT:
		{
			cPoint pt;
			p->GetLightedPoint(pt);
			GroundToColorTile(pt);
			float r = p->GetLightedRadius();
			r *= GROUND_DIVIDE;
			//确定那些的格子挡光
			ptlist.push_back(cPoint((int)pt.x,(int)pt.y));
			cRect rc(pt.x-r,pt.y-r,pt.x+r,pt.y+r);
			for (int x = ceil(rc.l); x <= floor(rc.r); x++)
			for (int y = ceil(rc.t); y <= floor(rc.b); y++)
			{
				if (x != (int)pt.x && y != (int)pt.y)
					ptlist.push_back(cPoint(x,y));
			}
			break;
		}
	case LIGHTED_SEAM:
		{
			cPoint p1,p2;
			p->GetLightedSeam(p1,p2);
			ASSERT(p1.x < p2.x);
			GroundToColorTile(p1);
			GroundToColorTile(p2);
			//一条线段可以跨越多少的格子?
			GetSeamCrossTile(p1,p2,ptlist);
			break;
		}
	}
}
/*
void cColorMap::GetPlaneLightingTile(cPlaneVer* p, cPointList & ptlist)
{
	//如果受光是基于线段的
	switch (p->GetLightingType())
	{
	case LIGHTING_SEAM:
		{
			cPoint p1,p2;
			p->GetLightingSeam(p1,p2);//得到几何意义上的seam而不是排序的seam
			ASSERT(p1.x < p2.x);
			GroundToColorTile(p1);
			GroundToColorTile(p2);
			//一条线段可以跨越多少的格子?
			GetSeamCrossTile(p1,p2,ptlist);
			break;
		}
	case LIGHTING_POINT:
		{
			cPoint pt;
			p->GetLightedPoint(pt);
			GroundToColorTile(pt);
			int x = pt.x;
			int y = pt.y;
			ptlist.push_back(cPoint(x,y));
			break;
		}	
	}
}
*/

//obstacle经过那些的tile,
void cColorMap::GetObstacleTile(cObstacle* p, cPointList & ptlist)
{
}

BOOL cColorMap::AddObstacleToTile(cObstacle* p)
{
	cPointList ptlist;
	GetObstacleTile(p,ptlist);
	for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		int x = pt.x;
		int y = pt.y;
		if (x >= 0 && x < MAX_TILE_X && y >= 0 && y < MAX_TILE_Y)
			m_aTile[x][y].AddObstacle(p);
	}
	return TRUE;
}

BOOL cColorMap::RemoveObstacleFromTile(cObstacle* p)
{
	cPointList ptlist;
	GetObstacleTile(p,ptlist);
	for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		int x = pt.x;
		int y = pt.y;
		if (x >= 0 && x < MAX_TILE_X && y >= 0 && y < MAX_TILE_Y)
			m_aTile[x][y].RemoveObstacle(p);
	}
	return TRUE;
}

BOOL cColorMap::AddPlaneToTile(cPlaneVer* p)
{
	cPointList ptlist;
	GetPlaneLightedTile(p,ptlist);
	for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		int x = pt.x;
		int y = pt.y;
		if (x >= 0 && x < MAX_TILE_X && y >= 0 && y < MAX_TILE_Y)
			m_aTile[x][y].AddLightedPlane(p);
	}
	return TRUE;
}

BOOL cColorMap::RemovePlaneFromTile(cPlaneVer* p)
{
	cPointList ptlist;
	GetPlaneLightedTile(p,ptlist);
	for (itcPoint it = ptlist.begin(); it != ptlist.end(); ++it)
	{
		cPoint pt = (*it);
		int x = pt.x;
		int y = pt.y;
		if (x >= 0 && x < MAX_TILE_X && y >= 0 && y < MAX_TILE_Y)
			m_aTile[x][y].RemoveLightedPlane(p);
	}
	return TRUE;
}

void cColorMap::GetLightMap(cLight* p)
{
	cPoint pt = p->GetOffset();
	GroundToColorTile(pt);
	int n = MAX_TILE_X;
	int m = MAX_TILE_Y;
	if (pt.x >= 0 && pt.x <= MAX_TILE_X && pt.y >= 0 && pt.y <= MAX_TILE_Y)
	{
		float r = p->GetRange();
		r *= GROUND_DIVIDE;
		cRect rcLight = cRect(pt.x-r,pt.y-r,pt.x+r,pt.y+r);
		cRect rcMax = cRect(0,0,MAX_TILE_X,MAX_TILE_Y);
		rcLight = rcLight & rcMax;
		cColor cr = p->GetColor();

		bool aLight[MAX_LIGHT_TILE][MAX_LIGHT_TILE];
		for (int i=0; i<MAX_LIGHT_TILE; i++)
		for (int j=0; j<MAX_LIGHT_TILE; j++)
			aLight[i][j] = true;

		int x,y;
		for(x = rcLight.l; x < rcLight.r; x++)
		for(y = rcLight.t; y < rcLight.b; y++)
		{
//可以制作一张表来简化运算，如果某个点是障碍，那么查表可以得到那些点不亮
			if (m_aTile[x][y].IsBlockLight())
			{
				int offx = x - (int)pt.x + MAX_LIGHT_RADIUS*GROUND_DIVIDE;
				int offy = y - (int)pt.y + MAX_LIGHT_RADIUS*GROUND_DIVIDE;
				//then get the nolight list point
				for (itcPoint it = m_aBlocked[offx][offy].begin(); it != m_aBlocked[offx][offy].end(); ++it)
				{
					cPoint pt = (*it);
					if (pt.x > MAX_LIGHT_RADIUS)
					{
						int t;
						t = 0;
					}
					aLight[(int)pt.x][(int)pt.y] = false;
				}
			}
		}

		for(x = rcLight.l; x < rcLight.r; x++)
		for(y = rcLight.t; y < rcLight.b; y++)
		{
			int offx = x - (int)pt.x + MAX_LIGHT_RADIUS*GROUND_DIVIDE;
			int offy = y - (int)pt.y + MAX_LIGHT_RADIUS*GROUND_DIVIDE;
			if (aLight[offx][offy])
			{
				m_aTile[x][y].Light(p,pt);
			}
		}
	}
}

//ptTo是否被ptLight照亮

