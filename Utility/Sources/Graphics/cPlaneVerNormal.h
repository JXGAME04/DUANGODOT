// cPlaneVerNormal.h: interface for the cPlaneVerNormal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANEVERNORMAL_H__94B2841A_31F0_461A_A630_E688B018EC59__INCLUDED_)
#define AFX_CPLANEVERNORMAL_H__94B2841A_31F0_461A_A630_E688B018EC59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPlaneVer.h"

class cPlaneVerNormal : public cPlaneVer  
{
public:
	cPlaneVerNormal(cGraphics* p);
	virtual ~cPlaneVerNormal();
	virtual void		Init(bool bCreateBuffer);

	LightingEnum		GetLightingType(){return LIGHTING_POINT;};
	VersortEnum			GetVersortType(){return VERSORT_POINT;}
	void				GetSortPoint(cPoint& p);
	
	HRESULT				DrawSub(cPoint p1,cPoint p2);
	virtual	bool		EndLighting();
};

#endif // !defined(AFX_CPLANEVERNORMAL_H__94B2841A_31F0_461A_A630_E688B018EC59__INCLUDED_)
