// cMesh3D.h: interface for the cMesh3D class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMESH3D_H__D94C4C2D_1F27_4CA7_ACBB_47AE4F14D83A__INCLUDED_)
#define AFX_CMESH3D_H__D94C4C2D_1F27_4CA7_ACBB_47AE4F14D83A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cMesh.h"

class cMesh3D : public cMesh  
{
public:
	cMesh3D(cGraphics* p);
	virtual ~cMesh3D();

	void Clear();
	virtual void SetOffset(cPoint pt);

	LPDIRECT3DVERTEXBUFFER8 m_pVB;
	LPDIRECT3DINDEXBUFFER8 m_pIB;	

	virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();                     
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
	virtual HRESULT FinalCleanup();
	LPDIRECT3DDEVICE8 GetDevice();

	virtual bool EndLighting();
	enum {D3DFVF_GROUNDVERTEX = D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1};

	int		m_nRun;
	float   m_fAlpha0;
	
	HRESULT Run();
	HRESULT Draw();
	HRESULT DrawHor();
	HRESULT DrawVer();
};

#endif // !defined(AFX_CMESH3D_H__D94C4C2D_1F27_4CA7_ACBB_47AE4F14D83A__INCLUDED_)
