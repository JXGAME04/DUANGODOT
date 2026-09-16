// cGround3D.h: interface for the cGround3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGROUND3D_H__ECB2D155_6064_4A1D_A921_4F5C478C147D__INCLUDED_)
#define AFX_CGROUND3D_H__ECB2D155_6064_4A1D_A921_4F5C478C147D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cGround.h"

class cGround3D : public cGround  
{
public:
	cGround3D(cGraphics* p);
	virtual ~cGround3D();

/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
	
	//vertex buffer
	//device
	LPDIRECT3DDEVICE8 GetDevice();
	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
	
	//Environment
	HRESULT SetMatrix();
	HRESULT SetStates();
	HRESULT SetVertexBuffer();
	D3DMATERIAL8 m_mtrl;
	HRESULT SetMaterial();
	stVertexGround m_aVertexPoint[MAX_POINT];
	enum {D3DFVF_GROUNDVERTEX = D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1};
	LPDIRECT3DVERTEXBUFFER8 m_pVertexGround;
	virtual HRESULT SetEnvironment();

	//draw
	HRESULT SetTexture(iTexture* pTex, int frame = 0);
	void DrawQuadrangle(stVertexGround* pV);
	void DrawTriangle(stVertexGround* pV) ;
	void Draw(stVertexGround* pV, int n);
	void Draw3Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3);
	void Draw4Vertex(stVertexGround& v1,stVertexGround& v2,stVertexGround& v3,stVertexGround& v4);
	void DrawQuadrangle(stVertexGround* pV, int times);

	//2d no draw fan!
//	virtual void DrawFan(iTexture* pTex, int frame, stVertexGround* pV, int n = 4);
	
	void DrawPolygon(stVertexGround* pV, int num, cColor cr);
	void DrawRect(cRect r, cColor cr,float fZ);
	void DrawRect(stVertexGround* pV, cColor cr);
	void DrawLine(cPoint pt1,cPoint pt2, cColor cr);	
	void DrawLine(stVertexGround* pV, int n);

	void DrawPoint(cPoint pt,  cColor cr) ;	
	void DrawPoint(cPoint* aPoint, int nPoint, cColor cr);
	void DrawPointAsRect(std::list<cPoint>& ptlist, cColor cr);
	void DrawPoint(std::list<cPoint>& ptlist, cColor cr);
};

#endif // !defined(AFX_CGROUND3D_H__ECB2D155_6064_4A1D_A921_4F5C478C147D__INCLUDED_)
