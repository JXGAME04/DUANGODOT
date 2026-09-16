// cPlaneVerNormal.cpp: implementation of the cPlaneVerNormal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cPlaneVerNormal.h"
#include "cGround.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cPlaneVerNormal::cPlaneVerNormal(cGraphics* p) : cPlaneVer(p)
{
}

cPlaneVerNormal::~cPlaneVerNormal()
{
}

bool cPlaneVerNormal::EndLighting()
{
	//modify the m_crLight!
	DWORD lColor;
	m_bLighting = GetGround()->GetColor(GetOffset(),lColor);
	m_crColorLight = cColor(lColor);
	return true;
}

HRESULT cPlaneVerNormal::DrawSub(cPoint p1,cPoint p2)
{
	return Draw();
}

void cPlaneVerNormal::Init(bool bCreateBuffer)
{
	if (bCreateBuffer)
		m_pParam = new stPlaneVerParam;
	cPlaneVer::Init(false);
	stPlaneVerParam& m_param = *GetPlaneVerParam();
	m_param.eVersort = VERSORT_POINT;
	m_param.eLighting = LIGHTING_POINT;
	m_pParam->eElement = GT_PLANEVERNORMAL;
}

void cPlaneVerNormal::GetSortPoint(cPoint& p)
{
	p = GetOffset();
	p.y += 0.2f;
	p.y += GetSortOffY();
	if (p.z != 0)
	{
//		p.y += p.z / 2;
	}
	p.z = 0;
}

