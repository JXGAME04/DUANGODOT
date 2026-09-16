// cPlaneVerDynamic.h: interface for the cPlaneVerDynamic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANEVERDYNAMIC_H__70248D32_0A21_4720_A9BA_DC9AFE468ACC__INCLUDED_)
#define AFX_CPLANEVERDYNAMIC_H__70248D32_0A21_4720_A9BA_DC9AFE468ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPlaneVer.h"

class cPlaneVerDynamic : public cPlaneVer  
{
public:
	cPlaneVerDynamic(cGraphics* p);
	virtual ~cPlaneVerDynamic();
	virtual void			Init(bool bCreateBuffer);

	virtual HRESULT			Draw();
	virtual HRESULT			DrawSub(cPoint p1,cPoint p2);
	virtual void			GetBottomSeam(cPoint& p1, cPoint& p2);
	virtual float			GetBottomSeamLengthSquare();

	stPlaneVerDynamicParam*	GetPlaneVerDynamicParam(){return (stPlaneVerDynamicParam*) m_pParam;};

	iTexture* m_aTexture[MAX_RESOURCE];
	stVertexGround m_aaVG[MAX_RESOURCE][4];
	
	HRESULT					UpdateVertexInfo();
	void					FindTexture();

	HRESULT					setParam(stObjParam& pm);
	stResourceArray&		GetResArray(){return GetPlaneVerDynamicParam()->stResArray;}
	int						GetResNum(){return GetPlaneVerDynamicParam()->stResArray.nResource;}
	stResource&				GetResA(int i){return GetPlaneVerDynamicParam()->stResArray.aResource[i];}
	iTexture*				GetTexture(int i);
	stVertexGround*			GetVG(int i);

	LightingEnum			GetLightingType(){return LIGHTING_POINT;};
	LightedEnum				GetLightedType(){return LIGHTED_NOLIGHTED;};
	VersortEnum				GetVersortType(){return VERSORT_POINT;}
};

#endif // !defined(AFX_CPLANEVERDYNAMIC_H__70248D32_0A21_4720_A9BA_DC9AFE468ACC__INCLUDED_)
