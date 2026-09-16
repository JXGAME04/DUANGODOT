// cColorMap.h: interface for the cColorMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CCOLORMAP_H__A75BEFAB_5340_4B19_A557_5B38383FD5D3__INCLUDED_)
#define AFX_CCOLORMAP_H__A75BEFAB_5340_4B19_A557_5B38383FD5D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
#include "GraphicsPublic.h"
#include "cColorTile.h"

class cColorMap  
{
public:
	cColorMap();
	virtual ~cColorMap();

	//为了光照使用
	//每个Ground 单元 被分为2*2的小tile也就是(16*8)的大小
	cColorTile m_aTile[MAX_TILE_X][MAX_TILE_Y];//格子信息
//	cColor m_aColor[MAX_TILE+1][MAX_TILE+1];//顶点光照
	bool GetTileType(cPoint ptGround, long& l);
	bool GetColor(cPoint ptGround,DWORD& value);

//base function
	void InitTile();
	void ClearTile();
	void RebuildTile();

//档光
	void GetPlaneLightedTile(cPlaneVer* p, cPointList & ptlist);
	BOOL AddPlaneToTile(cPlaneVer* p);
	BOOL RemovePlaneFromTile(cPlaneVer* p);

//光照
	cPointList m_aBlocked[MAX_LIGHT_TILE][MAX_LIGHT_TILE];
	cColor m_crColor;
	long m_lColor;
	void BeginLighting(cColor cr);
	void EndLighting();
	void GetLightMap(cLight* p);
	bool GetLightParam(cPoint ptGround,stLightParam& param);

//阻挡
	void GetObstacleTile(cObstacle* p, cPointList & ptlist);
	BOOL AddObstacleToTile(cObstacle* p);
	BOOL RemoveObstacleFromTile(cObstacle* p);

//偏移
	int m_xOffGround,m_yOffGround;
	void SetOffset(int x, int y){m_xOffGround = x; m_yOffGround = y;}
	void GroundToColorTile(cPoint& ptTile);
	cColorTile* GetTileByGround(cPoint& ptGround);

};

#endif // !defined(AFX_CCOLORMAP_H__A75BEFAB_5340_4B19_A557_5B38383FD5D3__INCLUDED_)
