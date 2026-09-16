// cScreen3D.h: interface for the cScreen3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSCREEN3D_H__09626752_ACD5_473B_B1F6_7ABB24F5E9F7__INCLUDED_)
#define AFX_CSCREEN3D_H__09626752_ACD5_473B_B1F6_7ABB24F5E9F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cScreen.h"

class cScreen3D : public cScreen  
{
public:
	cScreen3D(cGraphics* );
	virtual ~cScreen3D();

	//device
	LPDIRECT3DDEVICE8 GetDevice();
	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
//	virtual HRESULT FinalCleanup();
	LPDIRECT3DVERTEXBUFFER8 m_pVertexScreen;
	stVertexScreen m_aVertexPoint[MAX_POINT];
	enum {D3DFVF_SCREENVERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1};
	//draw
	virtual void SetTexture(iTexture* pTex, int frame = 0) ;
	virtual void Draw(stVertexScreen* pV, int n = 4); //strip
	virtual void DrawFan(stVertexScreen* pV, int n = 4) ;	
	virtual void DrawLine(CPoint pt1,CPoint pt2, cColor cr);
	virtual void DrawLine(stVertexScreen* pV, int n);
//	virtual void DrawLine(stVertexScreen* pV, cColor cr) ;
	virtual void DrawRect(stVertexScreen* pV,  cColor cr );
	virtual void DrawRect(cRect& rc, cColor cr);
	virtual void DrawPoint(CPoint pt, cColor cr) ;	
	virtual void DrawPoint(CPoint* aPoint, int nPoint, cColor cr);
	virtual void DrawPolygon(stVertexScreen* pV, int num, cColor cr);
	
	void Draw(iTexture* pTex, int frame, CPoint ptScreen);
	void Draw(iTexture* pTex, int frame, CPoint ptScreen, cColor cr) ;

	//Environment
	virtual HRESULT SetEnvironment();
	HRESULT SetStates();
	HRESULT SetVertexBuffer();
};

#endif // !defined(AFX_CSCREEN3D_H__09626752_ACD5_473B_B1F6_7ABB24F5E9F7__INCLUDED_)
