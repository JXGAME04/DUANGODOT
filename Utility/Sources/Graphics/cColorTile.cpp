// cColorTile.cpp: implementation of the cColorTile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cColorTile.h"
#include "cPlaneVer.h"
#include "cGround.h"
#include "clight.h"
#include "cObstacle.h"

#include "position.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//int cColorTile::m_xOffGround = 0;
//int cColorTile::m_yOffGround = 0;
cColorTile::cColorTile()
{
	m_x = -1;
	m_y = -1;
	m_lProperty = 0;
}

cColorTile::~cColorTile()
{
}

void cColorTile::Clear()
{
	m_listPlaneVer.clear();
	m_lProperty = 0;
}

bool cColorTile::BeginLighting(cColor cr)
{
	m_bLight = false;
	m_crLight = cr;
	return true;
}

bool cColorTile::EndLighting()
{
	if (m_bLight)
	{
		m_crLight.Format();
		m_lColor = m_crLight.Value();
	}
	return true;
}

BOOL cColorTile::GroundPointInTile(cPoint ptGround)
{
	return FALSE;
}

void cColorTile::AddObstacle(cObstacle* p)
{
	ObstacleEnum e = p->GetType();
	switch (e)
	{
	case OBSTACLE_CANFLYJUMP:
		AddProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_CANFLY:
		AddProperty(TILE_BLOCKJUMP);
		AddProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_CANJUMP:
		AddProperty(TILE_BLOCKFLY);
		AddProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_NORMAL:
		AddProperty(TILE_BLOCKJUMP);
		AddProperty(TILE_BLOCKFLY);
		AddProperty(TILE_BLOCKWALK);
		break;
	}
}

BOOL cColorTile::RemoveObstacle(cObstacle* p)
{
	ObstacleEnum e = p->GetType();
	switch (e)
	{
	case OBSTACLE_CANFLYJUMP:
		ClearProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_CANFLY:
		ClearProperty(TILE_BLOCKJUMP);
		ClearProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_CANJUMP:
		ClearProperty(TILE_BLOCKFLY);
		ClearProperty(TILE_BLOCKWALK);
		break;
	case OBSTACLE_NORMAL:
		ClearProperty(TILE_BLOCKJUMP);
		ClearProperty(TILE_BLOCKFLY);
		ClearProperty(TILE_BLOCKWALK);
		break;
	}
	return TRUE;
}

void cColorTile::AddLightedPlane(cPlaneVer* p)
{
	AddProperty(TILE_BLOCKLIGHT);
	m_listPlaneVer.push_back(p);
}

BOOL cColorTile::RemoveLightedPlane(cPlaneVer* p)
{
	m_listPlaneVer.remove(p);
	if (m_listPlaneVer.empty())
		ClearProperty(TILE_BLOCKLIGHT);	
	return TRUE;
}

/*
bool cColorTile::IsLightPlaneVer()
{
	//test verplane 是否照亮. 如果照亮，该格子视为非障碍，该格子往上一格子视为障碍
}
*/

void cColorTile::Light(cLight* p,cPoint ptColorTile)
{
	if (m_crLight.r >= 255 && m_crLight.b >= 255 && m_crLight.g >= 255)
		return;
	float r = p->GetRange();
	r *= GROUND_DIVIDE;
	float fDis = DistanceSquare(cPoint(m_x+.5f,m_y+.5f),ptColorTile);
	if (fDis < r*r)
	{
		m_crLight += p->GetColor() * (1 - fDis / (r*r) ); 
		m_bLight = true;
		m_zLight = ptColorTile.z;
	}
	
	m_paramLight.ClearMask();
	p->GetParam(m_paramLight);
}

bool cColorTile::GetLightParam(cPoint ptGround,stLightParam& param)
{
	param = m_paramLight;
	return true;
}

bool cColorTile::GetColor(cPoint ptGround,DWORD& value)
{
	if (!m_bLight)
		return false;
	float fz = fabs(ptGround.z - m_zLight);
	value = m_lColor;
	return true;
}
