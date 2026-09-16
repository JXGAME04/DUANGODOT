// cObstacle.cpp: implementation of the cObstacle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cObstacle.h"

#include "cGraphics.h"
#include "cGround.h"
#include "cCamera.h"

#include "iItem.h"
#include "position.h"
#include "cMapObstacle.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cObstacle::cObstacle(cGraphics* p) : cGroundObj(p)
{
}

cObstacle::~cObstacle()
{

}

void cObstacle::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stObstacleParam;
	cGroundObj::Init(false);
	stObstacleParam& m_param = *GetObstacleParam();
	m_param.iPoint = -1;
	m_param.fRadius = 0.2f;
	m_param.eElement = GT_OBSTACLE;
}

/**************************************************************************/
//save and load	
/**************************************************************************/
HRESULT cObstacle::Load(const stSection* p)
{
	cGroundObj::Load(p);

	if (p != NULL)
	{
		stObstacleParam& m_param = *GetObstacleParam();
		GetSectionValue(p,"obstacletype",cast_int(m_param.eObstacle));
		GetSectionValue(p,"pointnum",m_param.nPoint);
		GetSectionValue(p,"radius",m_param.fRadius);
		CString strValue;
		GetSectionValue(p,"pointarray",strValue);
		if (m_param.nPoint == 1)
			m_param.aPoint[0] = cPoint(0,0,0);
		else
			StringToPointArray(strValue,m_param.aPoint);
	}
	return S_OK;
}

HRESULT cObstacle::Save(stSection* p)
{
	cGroundObj::Save(p);

	ASSERT(p);

	stObstacleParam& m_param = *GetObstacleParam();
	SetSectionValue(p,"obstacletype",cast_int(m_param.eObstacle));
	SetSectionValue(p,"pointnum",m_param.nPoint);
	SetSectionValue(p,"radius",m_param.fRadius);
	CString strValue;
	PointArrayToString(m_param.aPoint,m_param.nPoint,strValue);
	SetSectionValue(p,"pointarray",strValue);
	return S_OK;
}

/**************************************************************************/
//param
/**************************************************************************/
/*
void cObstacle::OffsetToZero(cPoint pt) 
{
	stObstacleParam& m_param = *GetObstacleParam();
	SetOffset(cPoint(0,0));
	for (int i=0; i<m_param.nPoint; i++)
		m_param.aPoint[i] += pt;	
}
*/

HRESULT cObstacle::GetParam(stObjParam& pm)
{
	return cObj::GetParam(pm);
}

float cObstacle::GetLength()
{
	if (IsPoint())
		return 0;
	float fLength = 0;
	stObstacleParam& m_param = *GetObstacleParam();
	cPoint p0 = m_param.aPoint[0];
	for (int i=1; i<m_param.nPoint; i++)
	{
		cPoint p1 = m_param.aPoint[i];
		fLength += DistanceMax(p0,p1);
		p0 = p1;
	}
	return fLength;
}

void cObstacle::SetOffset(const cPoint& pt)
{
	GetGroundObjParam()->ptOffset = pt;
	UpdateTileList();
};

///////////////////////////////////////////////////////////////
//tilelist
///////////////////////////////////////////////////////////////
BOOL cObstacle::SetInGround(BOOL b)
{
	cGroundObj::SetInGround(b);

	if (IsInGround())
	{
		GetGround()->GetMapObstacle()->AddObstacle(m_listTile,GetType());
	}
	else
	{
		GetGround()->GetMapObstacle()->RemoveObstacle(m_listTile,GetType());		
	}

	return b;
}

HRESULT cObstacle::RemoveTileList()
{
	//not so simpe clear
	if (IsInGround())
		GetGround()->GetMapObstacle()->RemoveObstacle(m_listTile,GetType());
	m_listTile.clear();
	return S_OK;
}

HRESULT cObstacle::UpdateTileList()
{
	RemoveTileList();
	CreateTileList();
	return S_OK;
}

HRESULT cObstacle::CreateTileList()
{
	//create the list
	stObstacleParam m_param;
	stObstacleParam& param = *GetObstacleParam();
	m_param = param;
	int nPoint = m_param.nPoint;
	if (m_param.nPoint == 1)
	{
		m_param.aPoint[0].x = -m_param.fRadius;
		m_param.aPoint[0].y = -m_param.fRadius;
		m_param.aPoint[1].x = m_param.fRadius;
		m_param.aPoint[1].y = -m_param.fRadius;
		m_param.aPoint[2].x = m_param.fRadius;
		m_param.aPoint[2].y = m_param.fRadius;
		m_param.aPoint[3].x = -m_param.fRadius;
		m_param.aPoint[3].y = m_param.fRadius;
		m_param.aPoint[4].x = -m_param.fRadius;
		m_param.aPoint[4].y = -m_param.fRadius;
		nPoint = 5;
	}

	bool bBlock = false;
	for (int i=0; i<nPoint-1; i++)
	{
		cPoint p1 = GetOffset() + m_param.aPoint[i];
		cPoint p2 = GetOffset() + m_param.aPoint[i+1];
		cRect rect = GetGround()->GetRectGround();
//		if (PointInRect(rect,p1) || PointInRect(rect,p1))
		{
			std::list<cPoint> ptlist;
			if (p1.x > p2.x)
			{
				cPoint pp;
				pp = p1;
				p1 = p2;
				p2 = pp;
			}		
			GetSeamCrossTile(p1,p2,ptlist);
			for (std::list<cPoint>::iterator it = ptlist.begin(); it != ptlist.end(); ++it)
			{
				cPoint p = (*it);
				p.x += 0.5f;
				p.y += 0.5f;
				if (std::find(m_listTile.begin(),m_listTile.end(),p) == m_listTile.end())
					m_listTile.push_back(p);
			}
		}
	}

	if (IsInGround())
		GetGround()->GetMapObstacle()->AddObstacle(m_listTile,GetType());
	return S_OK;
}

HRESULT cObstacle::setParam(stObjParam& pm) 
{
	HRESULT hr = cGroundObj::setParam(pm);
	if (!pm.IsEMask(MASK_OBSTACLE))
		return hr;

	stObstacleParam& param = *((stObstacleParam* )&pm);
	stObstacleParam& m_param = *GetObstacleParam();

//因为这个导致redo and undo的不同
	if (param.IsSetMask(stObstacleParam::MASK_OBSTACLE_POINTINFO))
	{
		m_param.nPoint = param.nPoint;
		if (m_param.nPoint >= MAX_POINT)
		{
			ASSERT(0);
			m_param.nPoint = MAX_POINT - 1;
		}	
		for (int i=0; i<m_param.nPoint; i++)
		{
			m_param.aPoint[i] = param.aPoint[i];
		}
		UpdateTileList();
	}
	if (param.IsSetMask(stObstacleParam::MASK_OBSTACLE_TYPE))
	{
		RemoveTileList();
		m_param.eObstacle = param.eObstacle;
		CreateTileList();
	}
	if (param.IsSetMask(stObstacleParam::MASK_OBSTACLE_RADIUS))
		m_param.fRadius = param.fRadius;
	if (param.IsSetMask(stObstacleParam::MASK_OBSTACLE_SELECTPOINT))
	{
		if (param.iPoint < m_param.nPoint)
			m_param.iPoint = param.iPoint;
	}

	return 0;
}

BOOL cObstacle::IsPolygon()
{
	return GetPointNum() > 2 && GetPoint(0) == GetPoint(GetPointNum() - 1);
}

/////////////////////////////////////////////////////////////////
//draw
/////////////////////////////////////////////////////////////////
BOOL PointInRect(cRect r, cPoint pt)
{
	if (pt.x > r.left && pt.x < r.right && pt.y > r.top && pt.y < r.bottom)
		return TRUE;
	return FALSE;
}

void cObstacle::DrawLine(cPoint p1, cPoint p2, cColor cr)
{
	cRect rect = GetGround()->GetRectGround();
	if (PointInRect(rect,p1) || PointInRect(rect,p1))
	{
		DrawPoint(p1,cr);
		DrawPoint(p2,cr);
		GetGround()->DrawLine(p1,p2,cr);
	}
}

void cObstacle::DrawPoint(cPoint pt, cColor cr, float fRadius)
{
	cRect rect = GetGround()->GetRectGround();
	cRect rc(pt.x - fRadius, pt.y - fRadius, pt.x + fRadius, pt.y + fRadius);
	cRect r= rect & rc;
	if (r.IsEmpty())
		return;
	GetGround()->DrawRect(rc,cr);
}

HRESULT cObstacle::Draw()
{
	return 	Draw(GetOffset());
}

BOOL cObstacle::IsShow()
{
	return GetGraphics()->IsShow(iGraphics::GS_OBSTACLE);
}

bool cObstacle::CheckDisplay(const cRect* rc)
{
	m_bDisplay = true;
	return m_bDisplay;
}

cColor cObstacle::GetObstacleColor(ObstacleEnum e,BYTE brightness)
{
	cColor cr;
	switch (e)
	{
	case OBSTACLE_NORMAL:
		cr = cColor(brightness,0,0);
		break;
	case OBSTACLE_CANFLY:
		cr = cColor(0,brightness,0);
		break;
	case OBSTACLE_CANJUMP:
		cr = cColor(0,0,brightness);
		break;
	case OBSTACLE_CANFLYJUMP:
		cr = cColor(brightness,brightness,0);
		break;
	case OBSTACLE_NULL://这个可以使用在小地图上面
		cr = cColor(brightness,0,brightness);
		break;
	default:
//		GetObstacleParam()->eObstacle = OBSTACLE_NORMAL;
		break;
	}
	return cr;
}

BOOL cObstacle::IsInScreen(cPoint ptOffset)
{
	stObstacleParam& m_param = *GetObstacleParam();

	cRect rect = GetGround()->GetRectGround();
	for (int i=0; i<m_param.nPoint; i++)
	{
		cPoint p = ptOffset+m_param.aPoint[i]; 
		if (PointInRect(rect,p))
			return TRUE;
	}
	return FALSE;
}

HRESULT cObstacle::Draw(cPoint ptOffset)
{
	if (!IsShow())
		return S_OK;

	long brightness = 125;
	if (GetMapedit() & Mapedit_Select)
		brightness = 200;
	
	if (GetMapedit() & Mapedit_Trace)
	{
		char sMsg[128];
		sprintf(sMsg,"(F7)Obstacle: pos(%.1f,%.1f,%.1f) ",
			GetOffset().x,GetOffset().y,GetOffset().z);
		GetGraphics()->Trace(sMsg);		
	}

	if (!IsInScreen(ptOffset))
		return S_OK;

	cColor cr = GetObstacleColor(GetType(),brightness);
	
	stObstacleParam& m_param = *GetObstacleParam();

	if (m_param.nPoint == 1)
		DrawPoint(ptOffset+m_param.aPoint[0],cr,m_param.fRadius);
	else
	{
		stVertexGround v[MAX_POINT+1];
		cPoint aPoint[MAX_POINT+1];
		for (int i=0; i<m_param.nPoint; i++)
		{
			cPoint p = ptOffset+m_param.aPoint[i]; 
			aPoint[i] = p;
//			DrawPoint(p,cr);
			SetPoint(v[i],p);
			v[i].tu = 0;
			v[i].tv = 0;
			v[i].color = cr.Value();
		}

		GetGround()->DrawPoint(aPoint,m_param.nPoint,cr);
		GetGround()->DrawLine(v,m_param.nPoint);
//		DrawLine(ptOffset+m_param.aPoint[i], ptOffset+m_param.aPoint[i+1],cr);
	}

	if (GetGround()->GetObstacleType() == OT_TILE)
	{
		//画出格子来
		DrawTile(brightness);
	}

	if (GetMapedit() & Mapedit_Select)
	{
		if (m_param.iPoint != -1)
		{
			if (m_param.iPoint >= m_param.nPoint)
			{
				ASSERT(0);
				return -1;
			}
			if (m_param.nPoint != 1)
				DrawPoint(ptOffset+m_param.aPoint[m_param.iPoint],cColor(0,255,255));
			else
				DrawPoint(ptOffset+m_param.aPoint[m_param.iPoint],cColor(0,255,255),m_param.fRadius);
		}
	}

	return 0;
}

BOOL cObstacle::Contain(cPoint ptGround)
{
	if (!IsShow())
		return FALSE;

	stObstacleParam& m_param = *GetObstacleParam();
	if (m_param.nPoint == 1)
	{
		cPoint point = GetOffset() + m_param.aPoint[0];
		return DistanceSquare(point,ptGround) < m_param.fRadius*m_param.fRadius;
	}
	else
	{
		for (int i=0; i<m_param.nPoint-1; i++)
		{
			cPoint p1 = GetOffset() + m_param.aPoint[i];
			cPoint p2 = GetOffset() + m_param.aPoint[i+1];
			if (PointToSeam(ptGround,p1,p2) < 0.2f)
				return true;
		}
	}
	return false;
}

BOOL cObstacle::Contain(POINT ptScreen)
{
	cPoint ptGround;
	GetGround()->ScreenToGround(ptScreen,ptGround);
	return Contain(ptGround);
}

BOOL cObstacle::ContainedScreen(RECT rc)
{
	return FALSE;
}

BOOL cObstacle::ContainedGround(cRect rc)
{
	return FALSE;
}

BOOL cObstacle::ContainedDiamond(cRect rc)
{
	if (!IsShow())
		return FALSE;

	stObstacleParam& m_param = *GetObstacleParam();
	for (int i=0; i<m_param.nPoint; i++)
	{
		cPoint p1 = GetOffset() + m_param.aPoint[i];
		GroundToTile(p1);
		TileToDiamond(p1);
		if (!IsPointInRect(p1,rc))
			return FALSE;
	}
	return TRUE;
}

HRESULT	cObstacle::DrawTile(int brightness)
{
	return S_OK;
	cColor cr = GetObstacleColor(GetType(),brightness);
	GetGround()->DrawPointAsRect(m_listTile,cr);
	return S_OK;
}

BOOL cObstacle::IsBlock(cPoint pt,cPoint& p1,cPoint& p2)
{
	if (GetGround()->GetObstacleType() == OT_TILE)
	{
		pt.x = (int)pt.x + 0.5f;
		pt.y = (int)pt.y + 0.5f;
		pt.z = 0;
		if (std::find(m_listTile.begin(),m_listTile.end(),pt) == m_listTile.end())
			return FALSE;
		return TRUE;
	}

	stObstacleParam m_param;
	stObstacleParam& param = *GetObstacleParam();
	m_param = param;
	int nPoint = m_param.nPoint;
	if (m_param.nPoint == 1)
	{
		m_param.aPoint[0].x = -m_param.fRadius;
		m_param.aPoint[0].y = -m_param.fRadius;
		m_param.aPoint[1].x = m_param.fRadius;
		m_param.aPoint[1].y = -m_param.fRadius;
		m_param.aPoint[2].x = m_param.fRadius;
		m_param.aPoint[2].y = m_param.fRadius;
		m_param.aPoint[3].x = -m_param.fRadius;
		m_param.aPoint[3].y = m_param.fRadius;
		m_param.aPoint[4].x = -m_param.fRadius;
		m_param.aPoint[4].y = -m_param.fRadius;
		nPoint = 5;
	}

	bool bBlock = false;
	for (int i=0; i<nPoint-1; i++)
	{
		p1 = GetOffset() + m_param.aPoint[i];
		p2 = GetOffset() + m_param.aPoint[i+1];
		switch (GetGround()->GetObstacleType())
		{
		case OT_SEAM:
			if (PointToSeam(pt,p1,p2) < 0.2f)
			{
				bBlock = true;
				break;
			}
			break;
		case OT_TILE:
			bBlock = PointInSeamTile(pt,p1,p2);
			break;
		}
		if (bBlock == true)
			break;
	}
	return bBlock;
}

BOOL cObstacle::BlockObstacle(cPoint pt,ObstacleEnum e)
{	
	ObstacleEnum eType = GetType();
	if (eType == OBSTACLE_NULL)
		return FALSE;
	if (e == OBSTACLE_CANFLY)
	{
		if (eType == OBSTACLE_CANFLY || eType == OBSTACLE_CANFLYJUMP)
			return FALSE;
	}
	if (e == OBSTACLE_CANJUMP)
	{
		if (eType == OBSTACLE_CANJUMP || eType == OBSTACLE_CANFLYJUMP)
			return FALSE;
	}
	cPoint p1,p2;
	return IsBlock(pt,p1,p2);
}

ObstacleEnum cObstacle::CheckObstacle(cPoint pt,cPoint& vSpeed, bool bClockWise)
{
	cPoint p1,p2;
	if (!IsBlock(pt,p1,p2))
		return OBSTACLE_NULL;

	if (bClockWise)
	{
		vSpeed = p2 - p1;
	}
	else
	{
		vSpeed = p1 - p2;
	}

	return GetType();
}
