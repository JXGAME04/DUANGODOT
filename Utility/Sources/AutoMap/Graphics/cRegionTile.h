// cRegionTile.h: interface for the cRegionTile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_cRegionTile_H__1EBCBBDF_0C5B_4348_80BE_E27823E679B8__INCLUDED_)
#define AFX_cRegionTile_H__1EBCBBDF_0C5B_4348_80BE_E27823E679B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "iGraphics.h"

class cRegionTile  
{
public:
	cRegionTile();
	virtual ~cRegionTile();

	stTile m_stTile;
	stTile* GetTile(){return &m_stTile;}
};

#endif // !defined(AFX_cRegionTile_H__1EBCBBDF_0C5B_4348_80BE_E27823E679B8__INCLUDED_)
