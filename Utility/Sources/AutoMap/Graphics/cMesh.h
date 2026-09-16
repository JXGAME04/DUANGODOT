// cMesh.h: interface for the cMesh class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMESH_H__4D959036_0696_428D_92B3_006E4D6A9567__INCLUDED_)
#define AFX_CMESH_H__4D959036_0696_428D_92B3_006E4D6A9567__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"

class cMesh 
{
public:
	cMesh(cGraphics* p);
	virtual ~cMesh();
	
	MeshEnum m_eType;
//	void SetPlaneType(iPlane::TypeEnum e){m_eType = e;}
//com
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;
//graphics	
	cGraphics* m_pGraphics;
	cGraphics* GetGraphics(){return m_pGraphics;}
	cGround* GetGround();
//animation
	enum AnimationEnum
	{
		ANI_BLEND_NULL,
		ANI_BLEND_SLIGHT,
	};
	AnimationEnum m_eAnimation;
	int		m_nSpeed;
	float   m_fDegree;
	void SetAnimation(AnimationEnum eAni){m_eAnimation = eAni;}
	void SetAnimationSpeed(int nSpeed){if (nSpeed >= 1)m_nSpeed = nSpeed;}
	void SetAnimationDegree(float f){if (f > 1.f)m_fDegree = f;}

//offset
	cPoint m_ptOffset;
	virtual void SetOffset(cPoint pt);

//light
	bool BeginLighting(cColor cr);
	cColor m_crLighting;
	virtual bool EndLighting();

//paint loop
	bool m_bDisplay;
	cRect m_rcDisplay;
	bool IsDisplay(){return m_bDisplay;}
	bool CheckDisplay(const cRect& r);
	cColor m_crColor;
	void SetColor(cColor cr){m_crColor = cr;}

//mesh
	int m_nXVertex,m_nYVertex;
	float m_fXPlane;
	float m_fYPlane;
	int m_nVertexs;
	int m_nFaces;
	stVertexGround* m_aVertex;
	virtual HRESULT CreateVer(stVertexGround v[4]);
	virtual HRESULT CreateHor(stVertexGround* v, int width, int height);
	
	void GetQuadrangle(stVertexGround v[4]);
	void GetBottomSeam(cPoint& p1, cPoint& p2);

	bool m_bLight;
	bool IsLighting(){return m_bLight;}
	virtual HRESULT Draw();

//hor only
	void GroundToMesh(float& x,float& y);
	void GroundToMesh(cPoint& pt){GroundToMesh(pt.x,pt.y);};
	void GetIndexRect(cRect& rcGround);
	void GetGroundRect(cRect & r);

//mapedit
	long m_lMapeditParam;
	void SetMapeditParam(long param){m_lMapeditParam = param;}

//device
	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
	virtual HRESULT FinalCleanup();
};

#endif // !defined(AFX_CMESH_H__4D959036_0696_428D_92B3_006E4D6A9567__INCLUDED_)
