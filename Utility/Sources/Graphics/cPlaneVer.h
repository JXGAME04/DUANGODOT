// cPlaneVer.h: interface for the cPlaneVer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPLANEVER_H__71A5B7BE_1470_42C0_A910_DEB69B43B27A__INCLUDED_)
#define AFX_CPLANEVER_H__71A5B7BE_1470_42C0_A910_DEB69B43B27A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cPlane.h"
#include "GraphicsPublic.h"

class cPlaneVer : public cPlane  
{
public:
	cPlaneVer(cGraphics* p);
	virtual ~cPlaneVer();
	virtual void		Init(bool bCreateBuffer);
///////////////////////////////////////////////////////////////////
//Load and save
///////////////////////////////////////////////////////////////////
//param
	HRESULT				setParam(stObjParam& pm);

	HRESULT				SetPlaneProperty(stTemplateOne* pItem);

//	LPCSTR				GetDefaultTemplate();
	HRESULT				Load(const stSection* p);
	HRESULT				Save(stSection* p);
///////////////	////////////////////////////////////////////////////
//param
///////////////////////////////////////////////////////////////////
	stPlaneVerParam*	GetPlaneVerParam(){return (stPlaneVerParam*) m_pParam;};
	virtual LightedEnum		GetLightedType(){return GetPlaneVerParam()->eLighted;};
	virtual LightingEnum	GetLightingType(){return GetPlaneVerParam()->eLighting;};
	float				GetLightedRadius(){return GetPlaneVerParam()->fLightedRadius;};
	float				GetAngleXY(){return GetPlaneVerParam()->fAngleXY;}
//为了加云改了这一句，看来不行!
//	SortEnum			GetSortType(){return GetPlaneVerParam()->eSort;}
	virtual SortEnum	GetSortType(){return SORT_VER;}
	float				GetSortOffY(){return GetPlaneVerParam()->fSortOffY;}
	virtual VersortEnum	GetVersortType(){return GetPlaneVerParam()->eVersort;}
	bool				IsSortPoint();//按照重心来排序
	bool				IsLightingPoint(); //根据重心来受光，例如npc,使用这个节约一点

	virtual void		GetLightingSeam(cPoint& p1, cPoint& p2);
	virtual void		GetLightingPoint(cPoint& p);
	virtual void		GetLightedSeam(cPoint& p1, cPoint& p2);
	virtual void		GetLightedPoint(cPoint& p);
	virtual void		GetSortPoint(cPoint& p);
	virtual void		GetSortSeam(cPoint& p1, cPoint& p2);

	virtual void		GetBottomSeam(cPoint& p1, cPoint& p2);
	float				GetArea();//获得面积!
	ePlane2Plane		PositionPlane(cPlane* p);

	BOOL				GetSelectRect(cRect& rc);
	BOOL				ContainedDiamond(cRect rc);

//line function
	BOOL 				ComputeLineFunction(float& k, float& b);
	float				m_fK,m_fB;
	void 				GetLineFunction(float& k, float& b){k = m_fK; b = m_fB;};
	void				SetLineFunction(float k, float b){m_fK = k; m_fB = b;}
	virtual float		GetSortSeamLengthSquare();

//tree
	virtual bool		BeginBuildTree(){return true;};
	virtual bool		EndBuildTree(){return true;}

//lighting and mesh
//	virtual bool		Lighting(cLight* p,const cPoint& p1,const cPoint& p2,bool bLightit = true);	//传入的p1,p2是切分的点，
//	//不判断受光
//	virtual bool		Lighted(cLight* p, cPoint& p1, cPoint& p2);	//传出的p1,p2是挡光的seam

//p1,p2是plane的一段
	BOOL				IsShow();
	void				GetSubQuadrangle(stVertexGround v[4],cPoint p1,cPoint p2);
	virtual HRESULT		DrawSub(cPoint p1,cPoint p2);
	void				GetSelectVG(stVertexGround v[4], int i);
	HRESULT				DrawSelect();
	HRESULT				DrawTrace();
	HRESULT				DrawNum(cPoint p1,cPoint p2);
//Obstacle
	virtual void		UpdateObstacle(cObstacle* p);	//利用自身的几何信息来生成阻挡

//vertex info
	virtual HRESULT		FillVertexInfo();
	HRESULT FillTextureUV();
	virtual HRESULT		FillVertexInfoNotCallGraphics();

	BOOL				FillVertexInfoVerAngle0(const stTexInfo& info,int iResource ); 						
	BOOL				FillVertexInfoVer(const stTexInfo& info,int iResource );
	BOOL				FillVertexInfoVerFull(const stTexInfo& info,int iResource );
	
//
};

#endif // !defined(AFX_CPLANEVER_H__71A5B7BE_1470_42C0_A910_DEB69B43B27A__INCLUDED_)
