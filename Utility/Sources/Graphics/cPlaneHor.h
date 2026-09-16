// cPlaneHor.h: interface for the cPlaneHor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANEHOR_H__4629CE39_D91F_4086_8764_31012710EF14__INCLUDED_)
#define AFX_CPLANEHOR_H__4629CE39_D91F_4086_8764_31012710EF14__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPlane.h"

class cPlaneHor : public cPlane  
{
public:
	cPlaneHor(cGraphics* p);
	virtual ~cPlaneHor();
	virtual void			Init(bool bCreateBuffer);

public:
	void			GetGroundRect(cRect& rc);//得到ground上的矩形
	
///////////////////////////////////////////////////////////////////
//param
///////////////////////////////////////////////////////////////////
	stPlaneHorParam*	GetPlaneHorParam(){return (stPlaneHorParam*) m_pParam;};
	HRESULT			setParam(stObjParam& pm);

//override
	BOOL			IsShow();
	HRESULT			Draw();
	void			UpdateObstacle(cObstacle* p);
	float			GetArea();//获得面积!
	ePlane2Plane	PositionPlane(cPlane* p);
	BOOL			ContainedDiamond(cRect rc);

	virtual HRESULT FillVertexInfo();
	HRESULT FillTextureUV();
	virtual HRESULT	FillVertexInfoNotCallGraphics();

	BOOL			FillVertexInfoHorAngle0(const stTexInfo& info,int iResource ); 
	
	HRESULT			DrawTrace()	;
};

#endif // !defined(AFX_CPLANEHOR_H__4629CE39_D91F_4086_8764_31012710EF14__INCLUDED_)
