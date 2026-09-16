// cColorTile.h: interface for the cColorTile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CColorTile_H__EF93D5B9_44B5_491D_A716_E3B4A031026A__INCLUDED_)
#define AFX_CColorTile_H__EF93D5B9_44B5_491D_A716_E3B4A031026A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

class cColorTile  
{
public:
	cColorTile();
	virtual ~cColorTile();
	void Clear();

	int m_lProperty;					//属性
	BOOL IsProperty(TileEnum e){return m_lProperty & e;};
	void AddProperty(TileEnum e){m_lProperty |= e;};
	void ClearProperty(TileEnum e){m_lProperty &= ~e;};
	int GetProperty(){return m_lProperty;}

	BOOL IsBlockLight(){return IsProperty(TILE_BLOCKLIGHT);}
	BOOL IsBlockWalk(){return IsProperty(TILE_BLOCKWALK);}
	BOOL IsBlockFly(){return IsProperty(TILE_BLOCKFLY);}

	void AddObstacle(cObstacle* p);
	BOOL RemoveObstacle(cObstacle* p);
	
	cColor m_crLight;					//光强
	long m_lColor;
	bool m_bLight;						//是否被照亮？
	bool BeginLighting(cColor cr);
	bool EndLighting();
	float m_zLight;
	bool GetColor(cPoint ptGround,DWORD& value);
	stLightParam m_paramLight;
	bool GetLightParam(cPoint ptGround,stLightParam& param);
	
	stVertexGround m_v;					//定点
	int	m_x,m_y;
	void SetPos(int x,int y){m_x = x; m_y = y;}
	void GetPos(int& x, int& y){x = m_x; y = m_y;}

	cPlaneVerList m_listPlaneVer;
	void AddLightedPlane(cPlaneVer* p);
	BOOL RemoveLightedPlane(cPlaneVer* p);
	BOOL GroundPointInTile(cPoint ptGround);

	void Light(cLight* p,cPoint ptColorTile);
};

#endif // !defined(AFX_CColorTile_H__EF93D5B9_44B5_491D_A716_E3B4A031026A__INCLUDED_)
